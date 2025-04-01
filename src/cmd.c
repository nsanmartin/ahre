#include "bookmark.h"
#include "cmd.h"
#include "draw.h"
#include "range_parse.h"
#include "readpass.h"


#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset


/* session commands */
Err cmd_open_url(Session session[static 1], const char* url) {
    url = cstr_trim_space((char*)url);
    if (*url == '\\') {
        Str u = (Str){0};
        try (get_url_alias(cstr_skip_space(url + 1), &u));
        Err err = session_open_url(session, u.items, session->url_client);
        str_clean(&u);
        return err;
    }
    return session_open_url(session, url, session->url_client);
}

Err cmd_set_session_winsz(Session session[static 1]) {
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(session) = nrows;
    *session_ncols(session) = ncols;
    
    try( session_write_msg_lit__(session, "win: nrows = ")); 
    try( session_write_unsigned_msg(session, nrows));
    try( session_write_msg_lit__(session, ", ncols = "));
    try( session_write_unsigned_msg(session, ncols));
    try( session_write_msg_lit__(session, "\n"));
    return Ok;
}

Err cmd_set_session_ncols(Session session[static 1], const char* line) {
    size_t ncols;
    parse_size_t_or_throw(&line, &ncols, 10);
    if (*cstr_skip_space(line)) return "invalid argument";
    *session_ncols(session) = ncols;
    return Ok;
}

Err cmd_set_session_monochrome(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    if (*line == '0') session_monochrome_set(session, false);
    else if (*line == '1') session_monochrome_set(session, true);
    else return "monochrome option should be '0' or '1'";
    return Ok;
}

Err cmd_set_session_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    UserInterface ui;
    const char* rest;
    if ((rest = csubstr_match(line, "fgets", 1)) && !*rest) ui = ui_fgets();
    else if ((rest = csubstr_match(line, "isocline", 1)) && !*rest) ui = ui_isocline();
    else if ((rest = csubstr_match(line, "vi", 1)) && !*rest) ui = ui_vi_mode();
    else return "input option should be 'getline' or 'isocline'";
    ui_switch(session_ui(session), &ui);
    return Ok;
}

Err cmd_set_session(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest=csubstr_match(line, "input", 1))) return cmd_set_session_input(session, rest);
    if ((rest=csubstr_match(line, "monochrome", 1))) return cmd_set_session_monochrome(session, rest);
    if ((rest=csubstr_match(line, "ncols", 1))) return cmd_set_session_ncols(session, rest);
    if ((rest=csubstr_match(line, "winsz", 1)) && !*rest) return cmd_set_session_winsz(session);
    return "not a session option";
}

Err cmd_set_curl(Session session[static 1], const char* line);

Err cmd_set(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest = csubstr_match(line, "session", 1))) return cmd_set_session(session, rest);
    if ((rest = csubstr_match(line, "curl", 1))) return cmd_set_curl(session, rest);
    return "not a curl option";
}


/* tabs commands */
Err cmd_tabs(Session session[static 1], const char* line) {
    TabList* f = session_tablist(session);

    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return tablist_print_info(session, f);
        case '-': return tablist_back(f);
        default: return tablist_move_to_node(f, line);
    }
    return Ok;
}

/* htmldoc commands */
Err cmd_doc(Session session[static 1], const char* line) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    if (!htmldoc_is_valid(d) || !session->url_client) return "no document";
    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return htmldoc_print_info(session, d);
        case 'A': return htmldoc_A(session, d);
        case '+': return bookmark_add_doc(d, cstr_skip_space(line + 1), session_url_client(session));
        case '%': return dbg_traversal(session, d, line + 1);
    }
    const char* rest;
    if ((rest = csubstr_match(line, "draw", 1))) { return cmd_draw(session, rest); }
    if ((rest = csubstr_match(line, "hide", 1))) { return cmd_doc_hide(d, rest); }
    if ((rest = csubstr_match(line, "show", 1))) { return cmd_doc_show(d, rest); }
    return "unknown doc command";
}

/*
 * These commands require a valid document, the caller should check this condition before
 */
static Err cmd_fetch(Session session[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    CURLU* new_cu;
    try( url_curlu_dup(htmldoc_url(htmldoc), &new_cu));
    HtmlDoc newdoc;
    Err err = htmldoc_init_fetch_draw_from_curlu(
        &newdoc,
        new_cu,
        session_url_client(session),
        http_get,
        session
    );
    if (err) {
        curl_url_cleanup(new_cu);
        return err;
    }
    htmldoc_cleanup(htmldoc);
    *htmldoc = newdoc;
    return err;
}

static Err shorcut_gg(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    *textbuf_current_offset(tb) = 0;
    if(textbuf_current_line(tb) > textbuf_line_count(tb)) return "No lines in buffer";
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    Range r = (Range){
        .beg=textbuf_current_line(tb),
        .end=textbuf_current_line(tb) + *session_nrows(session) - 2
    };
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    if (r.end < r.beg) r.end = r.beg;
    try( cmd_textbuf_eval(session, tb,cmd, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

static Err shorcut_zb(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if(textbuf_current_line(tb) == 1) return "No more lines at the beginning";

    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }

    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    size_t beg = textbuf_current_line(tb) <= *session_nrows(session) - 1
        ? 1
        : textbuf_current_line(tb) - *session_nrows(session) - 1
        ;
    Range r = (Range){
        .beg=beg,
        .end=textbuf_current_line(tb)
    };
    try( cmd_textbuf_eval(session, tb, cmd, &r));
    try( textbuf_get_offset_of_line(tb, r.beg,textbuf_current_offset(tb)));

    return Ok;
}

static Err shorcut_zf(Session session[static 1], const char* rest) {
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if(textbuf_current_line(tb) > textbuf_line_count(tb)) return "No more lines in buffer";
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";

    size_t range_beg = (textbuf_last_range(tb)->beg && textbuf_last_range(tb)->end) 
        ? textbuf_last_range(tb)->end
        : textbuf_current_line(tb)
        ;
    Range r = (Range){ .beg=range_beg, .end=range_beg + *session_nrows(session) - 2 };
    if (r.end < r.beg) r.end = r.beg;
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    try( cmd_textbuf_eval(session, tb,cmd, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

static Err shorcut_zz(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    size_t beg = textbuf_current_line(tb) <= (*session_nrows(session)-2) / 2
        ? 1
        : textbuf_current_line(tb) - (*session_nrows(session)-2) / 2
        ;

    Range r = (Range){
        .beg=beg, .end=beg + *session_nrows(session) - 2
    };
    if (r.end < r.beg) r.end = r.beg;
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    return cmd_textbuf_eval(session, tb, cmd, &r);
}

Err cmd_misc(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);
    //if ((rest = csubstr_match(line, "ahref", 2))) { return cmd_ahre(session, rest); } //TODO: deprecate, impl [%p
    if ((rest = csubstr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = csubstr_match(line, "class", 3))) { return "TODO: class"; }
    ///if ((rest = csubstr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = csubstr_match(line, "fetch", 1))) { return cmd_fetch(session); }
    if ((rest = csubword_match(line, "gg", 2))) { return shorcut_gg(session, rest); }
    if ((rest = csubstr_match(line, "tag", 2))) { return cmd_misc_tag(rest, session); }
    //TODO: define shorcut_z and pass the rest
    if ((rest = csubword_match(line, "zb", 2))) { return shorcut_zb(session, rest); }
    if ((rest = csubword_match(line, "zf", 2))) { return shorcut_zf(session, rest); }
    if ((rest = csubword_match(line, "zz", 2))) { return shorcut_zz(session, rest); }

    return "unknown cmd";
}

/* textbuf commands */

Err cmd_write(const char* fname, Session session[static 1]) {
    TextBuf* buf;
    try( session_current_buf(session, &buf));
    if (fname && *(fname = cstr_skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            return err_fmt("append: could not open file: %s", fname);
        }
        if (fwrite(textbuf_items(buf), 1, textbuf_len(buf), fp) != textbuf_len(buf)) {
            fclose(fp);
            return err_fmt("append: error writing to file: %s", fname);
        }
        fclose(fp);
        return 0;
    }
    return "append: fname missing";
}

//
//TODO: apply mods to text
Err ed_print_n(Session s[static 1], TextBuf textbuf[static 1], Range range[static 1]) {
    try(validate_range_for_buffer(textbuf, range));
    SessionMemWriter w = (SessionMemWriter){.s=s, .write=session_writer_write_std};
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        try( session_write_unsigned_std(s, linum));
        try( w.write(&w, "\t", 1));
        if (line.len) try( w.write(&w, (char*)line.items, line.len));
        else try( w.write(&w, "\n", 1));
    }

    if (textbuf_line_count(textbuf) == range->end) try( w.write(&w, "\n", 1));
    return Ok;
}

Err dbg_print_all_lines_nums(Session s[static 1], TextBuf textbuf[static 1]) {
    size_t lnum = 1;
    StrView line;
    SessionMemWriter w = (SessionMemWriter){.s=s, .write=session_writer_write_std};
    for (;textbuf_get_line(textbuf, lnum, &line); ++lnum) {
        try( session_write_unsigned_std(s, lnum));
        try( w.write(&w,"\t`", 2));
        if (line.len > 1) {
            try( w.write(&w, (char*)line.items, line.len-1));
        } 
        try( w.write(&w,"'\n", 2));
    }
    return Ok;
}

StrView parse_pattern(const char* tk) {
    StrView res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = cstr_skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) {
        res = (StrView){.items = tk, .len=strlen(tk)};
    } else {
        res = (StrView){.items = tk, .len=end-tk};
    }
    return res;
}

static const char* _mem_find_esc_code_(const char* s, size_t len) {
    const char* res = s;
    while((res = memchr(res, '\033', len))) {
        if (res + sizeof(EscCodeReset)-1 > s + len) return NULL;
        const char code_prefix[] = "\033[";
        size_t code_prefix_len = sizeof(code_prefix) - 1;
        if (memcmp(code_prefix, res, code_prefix_len)) {
            /* strings differ */
            ++res;
        } else break;
    }
    return res;
}


Err ed_write(const char* rest, TextBuf textbuf[static 1]) {
    if (!rest || !*rest) { return "cannot write without file arg"; }
    rest = cstr_trim_space((char*)rest);
    if (!*rest) return "invalid url name";
    FILE* fp = fopen(rest, "w");
    if (!fp) return err_fmt("%s: could not open file: %s", __func__, rest); 

    const char* items = textbuf_items(textbuf);
    const char* beg = items;
    size_t len = textbuf_len(textbuf);
    while (beg && beg < items + len) {
        const char* end = _mem_find_esc_code_(beg, items + len - beg);
        if (!end) end = items + len;
        size_t chunklen = end - beg;
        if (chunklen > 1 && fwrite(beg, 1, chunklen, fp) != chunklen) {
            fclose(fp);
            return err_fmt("%s: error writing to file: %s", __func__, rest);
        }
        if (end + 1 < items + len) beg = ((char*)memchr(end + 1, 'm', items+len-(end+1))) + 1;
        else break;
    }
    if (fclose(fp)) return err_fmt("error closing file '%s'", rest);
    return Ok;
}


Err ed_global(TextBuf textbuf[static 1],  const char* rest) {
    (void)textbuf;
    StrView pattern = parse_pattern(rest);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.items);
    return Ok;
}


Err cmd_textbuf_impl(Session s[static 1], TextBuf textbuf[static 1],  const char* line) {
    if (!line) { return Ok; }
    Range range = {0};
    RangeParseCtx ctx = range_parse_ctx_from_textbuf(textbuf);
    line = parse_range(line, &range, &ctx);
    if (range_parse_failure(line)) {
        return range_parse_failure_to_err(line);
    } else *textbuf_last_range(textbuf) = range;

    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }

    if (range.end == 0) return "error: unexpected range with end == 0";
    if (range.end > textbuf_line_count(textbuf)) return "error: range end too large";
    if (ctx.new_offset >= textbuf_len(textbuf))
        try( textbuf_get_offset_of_line(textbuf, range.end,textbuf_current_offset(textbuf)));
    else
        *textbuf_current_offset(textbuf) = ctx.new_offset;
    return cmd_textbuf_eval(s, textbuf, line, &range);
}


Err cmd_textbuf_eval(Session s[static 1], TextBuf textbuf[static 1], const char* line, Range range[static 1]) {
    line = cstr_skip_space(line);

    *textbuf_last_range(textbuf) = *range;

    const char* rest = 0x0;

    if (!*line) { return cmd_textbuf_print(s, textbuf, range); } /* default :NUM */
    if ((rest = csubstr_match(line, "a", 1)) && !*rest) { return cmd_textbuf_print_all(textbuf); } //TODO: depre: use :%
    if ((rest = csubstr_match(line, "l", 1)) && !*rest) { return dbg_print_all_lines_nums(s, textbuf); } //TODO: deprecate
    if ((rest = csubstr_match(line, "g", 1)) && *rest) { return ed_global(textbuf, rest); }
    if ((rest = csubstr_match(line, "n", 1)) && !*rest) { return ed_print_n(s, textbuf, range); }
    if ((rest = csubstr_match(line, "print", 1)) && !*rest) { return cmd_textbuf_print(s, textbuf, range); }
    if ((rest = csubstr_match(line, "write", 1))) { return ed_write(rest, textbuf); }
    return "unknown command";
}


/* input commands */


Err cmd_input_ix(Session session[static 1], const size_t ix, const char* line) {
    lxb_dom_node_t* node;
    try( _get_input_by_ix(session, ix, &node));

    const lxb_char_t* type;
    size_t len;
    lexbor_find_attr_value(node, "type", &type, &len);

    UserOutput* out = session_uout(session);
    Err err = Ok;
    if (!*line) {
        try( out->write_std("> ", 1, session));
        ArlOf(char) masked = (ArlOf(char)){0};

        SessionMemWriter writer = (SessionMemWriter){.s=session,.write=session_writer_write_std};
        err = readpass_term(&masked, &writer);
        ok_then(err, lexbor_set_attr_value(node, masked.items, masked.len));
        arlfn(char, clean)(&masked);
    } else {
        if (*line == ' ' || *line == '=') ++line;
        size_t len = strlen(line);
        if (len && line[len-1] == '\n') --len;
        err = lexbor_set_attr_value(node, line, len);
    }
    ok_then(err, cmd_draw(session, ""));
    return err;
}

Err cmd_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return cmd_input_print(session, linknum);
        case '\0': 
        case '*': return cmd_input_submit_ix(session, linknum);
        case '=': return cmd_input_ix(session, linknum, line + 1); 
        default: return "?";
    }
}

/* image commands */

Err cmd_image(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    switch (*line) {
        case '?': return cmd_image_print(session, linknum);
        default: return "?";
    }
    return Ok;
}

Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* images = htmldoc_imgs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err cmd_image_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);
    ok_then(err, session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}


/* anchor commands */

Err cmd_anchor_print(Session session[static 1], size_t linknum) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(htmldoc);

    LxbNodePtr* a = arlfn(LxbNodePtr, at)(anchors, (size_t)linknum);
    if (!a) return "line number invalid";
    
    BufOf(char)* buf = &(BufOf(char)){0};
    try( lexbor_node_to_str(*a, buf));

    Err err = session_write_msg(session, items__(buf), len__(buf));
    buffn(char, clean)(buf);
    return err;
}


Err cmd_anchor(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    long long unsigned linknum;
    try( parse_base36_or_throw(&line, &linknum));
    line = cstr_skip_space(line);
    //if (*line && *cstr_skip_space(line + 1)) return "error unexpected:..."; TODO: only check when necessary
    switch (*line) {
        case '\'': return cmd_anchor_print(session, (size_t)linknum); 
        case '?': return cmd_anchor_print(session, (size_t)linknum); 
        case '\0': 
        case '*': return cmd_anchor_asterisk(session, (size_t)linknum);
        default: return "?";
    }
}
