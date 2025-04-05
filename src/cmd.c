#include "bookmark.h"
#include "cmd.h"
#include "draw.h"
#include "range_parse.h"
#include "readpass.h"


#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset

/* session commands */
Err cmd_go(CmdParams p[static 1]) {
    p->ln = cstr_trim_space((char*)p->ln);
    if (*p->ln == '\\') {
        Str u = (Str){0};
        try (get_url_alias(cstr_skip_space(p->ln + 1), &u));
        Err err = session_open_url(p->s, u.items, p->s->url_client);
        str_clean(&u);
        return err;
    }
    return session_open_url(p->s, p->ln, p->s->url_client);
}

Err cmd_set_session_winsz(CmdParams p[static 1]) {
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(p->s) = nrows;
    *session_ncols(p->s) = ncols;
    
    try( session_write_msg_lit__(p->s, "win: nrows = ")); 
    try( session_write_unsigned_msg(p->s, nrows));
    try( session_write_msg_lit__(p->s, ", ncols = "));
    try( session_write_unsigned_msg(p->s, ncols));
    try( session_write_msg_lit__(p->s, "\n"));
    return Ok;
}

Err cmd_set_session_ncols(CmdParams p[static 1]) {
    size_t ncols;
    parse_size_t_or_throw(&p->ln, &ncols, 10);
    if (*cstr_skip_space(p->ln)) return "invalid argument";
    *session_ncols(p->s) = ncols;
    return Ok;
}

Err cmd_set_session_monochrome(CmdParams p[static 1]) {
    p->ln = cstr_skip_space(p->ln);
    if (*p->ln == '0') session_monochrome_set(p->s, false);
    else if (*p->ln == '1') session_monochrome_set(p->s, true);
    else return "monochrome option should be '0' or '1'";
    return Ok;
}

Err cmd_set_session_input(CmdParams p[static 1]) {
    p->ln = cstr_skip_space(p->ln);
    UserInterface ui;
    const char* rest;
    if ((rest = csubstr_match(p->ln, "fgets", 1)) && !*rest) ui = ui_fgets();
    else if ((rest = csubstr_match(p->ln, "isocline", 1)) && !*rest) ui = ui_isocline();
    else if ((rest = csubstr_match(p->ln, "visual", 1)) && !*rest) ui = ui_vi_mode();
    else return "input option should be 'getline', 'isocline' or 'visual'";
    ui_switch(session_ui(p->s), &ui);
    return Ok;
}


/* tabs commands */
/* htmldoc commands */
/*
 * These commands require a valid document, the caller should check this condition before
 */
static Err _cmd_fetch(Session session[static 1]) {
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


static Err shorcut_z(Session session[static 1], const char* rest) {
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


    SessionMemWriter writer = (SessionMemWriter){.s=session, .write=session_writer_write_std};
    if (*rest == 'n') try( session_write_range_mod(&writer, tb, &r));
    else try( session_write_range_mod(&writer, tb, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

Err _cmd_misc(Session session[static 1], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);
    if ((rest = csubstr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = csubstr_match(line, "class", 3))) { return "TODO: class"; }
    ///if ((rest = csubstr_match(line, "clear", 3))) { return cmd_clear(session); }
    if ((rest = csubstr_match(line, "fetch", 1))) { return _cmd_fetch(session); }
    if ((rest = csubstr_match(line, "tag", 2))) { return _cmd_misc_tag(rest, session); }
    if ((rest = csubword_match(line, "z", 1))) { return shorcut_z(session, rest); }

    return "unknown cmd";
}

/* textbuf commands */

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


//
//TODO: apply mods to text
Err _textbuf_print_n_(
    Session s[static 1], TextBuf textbuf[static 1], Range range[static 1], const char* ln)
{
    (void)ln;
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


Err dbg_print_all_lines_nums(
    Session s[static 1], TextBuf textbuf[static 1], Range r[static 1], const char* ln)
{
    (void)r;
    cmd_assert_no_params(ln);
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


Err _cmd_textbuf_write_impl(
    Session s[static 1], TextBuf textbuf[static 1], Range r[static 1], const char* rest
)
{
    (void)s; (void)r;
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

Err cmd_textbuf_global(CmdParams p[static 1]) {
    StrView pattern = parse_pattern(p->ln);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.items);
    return Ok;
}


Err _cmd_parse_range(Session s[static 1], Range range[static 1],  const char* inputline[static 1]) {
    *range = (Range){0};
    const char* line = *inputline;
    if (!line) { return Ok; }
    TextBuf* textbuf;
    try( session_current_buf(s, &textbuf));
    RangeParseCtx ctx = range_parse_ctx_from_textbuf(textbuf);
    line = parse_range(line, range, &ctx);
    if (range_parse_failure(line)) {
        return range_parse_failure_to_err(line);
    } else *textbuf_last_range(textbuf) = *range;
    *inputline = line;
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }

    if (range->end == 0) return "error: unexpected range with end == 0";
    if (range->end > textbuf_line_count(textbuf)) return "error: range end too large";
    if (ctx.new_offset >= textbuf_len(textbuf))
        try( textbuf_get_offset_of_line(textbuf, range->end,textbuf_current_offset(textbuf)));
    else
        *textbuf_current_offset(textbuf) = ctx.new_offset;
    return Ok;
}




/* input commands */


Err _cmd_input_ix(Session session[static 1], const size_t ix, const char* line) {
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
    ok_then(err, _cmd_doc_draw(session, ""));
    return err;
}

/* image commands */

Err _get_image_by_ix(Session session[static 1], size_t ix, lxb_dom_node_t* outnode[static 1]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* images = htmldoc_imgs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err _cmd_image_print(Session session[static 1], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    BufOf(char)* buf = &(BufOf(char)){0};
    Err err = lexbor_node_to_str(node, buf);
    ok_then(err, session_write_msg(session, items__(buf), len__(buf)));
    buffn(char, clean)(buf);
    return err;
}


/* anchor commands */

Err _cmd_anchor_print(Session session[static 1], size_t linknum) {
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


