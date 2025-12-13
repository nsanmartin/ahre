#include "bookmark.h"
#include "cmd.h"
#include "draw.h"
#include "range-parse.h"
#include "readpass.h"


#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset

/* session commands */
Err cmd_get(CmdParams p[_1_]) {
    p->ln = cstr_trim_space((char*)p->ln);
    Request r;
    if (*p->ln == '\\') {
        r = (Request){.method=http_get};
        try (get_url_alias(p->s, cstr_skip_space(p->ln + 1), &r.url));
        Err err = session_fetch_request(p->s, &r, session_url_client(p->s));
        request_clean(&r);
        return err;
    }
    try (request_from_userln(&r, p->ln, http_get));
    return session_fetch_request(p->s, &r, session_url_client(p->s));
}


Err cmd_post(CmdParams p[_1_]) {
    p->ln = cstr_trim_space((char*)p->ln);
    Request r;
    if (*p->ln == '\\') {
        r = (Request){.method=http_post};
        try (get_url_alias(p->s, cstr_skip_space(p->ln + 1), &r.url));
        Err err = session_fetch_request(p->s, &r, session_url_client(p->s));
        request_clean(&r);
        return err;
    }
    try (request_from_userln(&r, p->ln, http_post));
    return session_fetch_request(p->s, &r, session_url_client(p->s));
}


Err cmd_set_session_winsz(CmdParams p[_1_]) {
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

Err cmd_set_session_ncols(CmdParams p[_1_]) {
    size_t ncols;
    try( parse_size_t_or_throw(&p->ln, &ncols, 10));
    if (*cstr_skip_space(p->ln)) return "invalid argument";
    *session_ncols(p->s) = ncols;
    return Ok;
}

Err cmd_set_session_monochrome(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    if (*p->ln == '0') session_monochrome_set(p->s, false);
    else if (*p->ln == '1') session_monochrome_set(p->s, true);
    else return "monochrome option should be '0' or '1'";
    return Ok;
}

Err cmd_set_session_js(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    if (*p->ln == '0') session_js_set(p->s, false);
    else if (*p->ln == '1') session_js_set(p->s, true);
    else return "js option should be '0' or '1'";
    return Ok;
}

Err cmd_set_session_forms(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    char c = p->ln[0]; 
    if (c != '0' && c != '1') return "js option should be '0' or '1'";
    session_conf_show_forms_set(session_conf(p->s), c == '1');
    return Ok;
}


Err cmd_set_session_bookmark(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    Str bookmark_fname = (Str){0};
    try(resolve_bookmarks_file(p->ln, &bookmark_fname));
    str_clean(session_bookmarks_fname(p->s));
    *session_bookmarks_fname(p->s) = bookmark_fname;
    return Ok;
}

Err cmd_set_session_input(CmdParams p[_1_]) {
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

Err cmd_fetch(Session session[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    Request r = (Request){
        .method=http_get,
    };
    HtmlDoc newdoc;
    try(htmldoc_init_from_request(
        &newdoc,
        &r,
        htmldoc_url(htmldoc),
        session_url_client(session),
        session
    ));
    htmldoc_cleanup(htmldoc);
    *htmldoc = newdoc;
    return Ok;
}


Err shortcut_z(Session session[_1_], const char* rest) {
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

    size_t range_beg = textbuf_current_line(tb);
    Range r = (Range){ .beg=range_beg, .end=range_beg + *session_nrows(session) - 3 };
    if (r.end < r.beg) r.end = r.beg;
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);


    try( session_write_std_range_mod(session, tb, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

//TODO: use it or delete it
Err _cmd_misc(Session session[_1_], const char* line) {
    const char* rest = 0x0;
    line = cstr_skip_space(line);
    if ((rest = csubstr_match(line, "attr", 2))) { return "TODO: attr"; }
    if ((rest = csubstr_match(line, "class", 3))) { return "TODO: class"; }
    ///if ((rest = csubstr_match(line, "clear", 3))) { return cmd_clear(session); }
    /* if ((rest = csubstr_match(line, "fetch", 1))) { return _cmd_fetch(session); } */
    if ((rest = csubstr_match(line, "tag", 2))) { return _cmd_misc_tag(rest, session); }
    if ((rest = csubword_match(line, "z", 1))) { return shortcut_z(session, rest); }

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
    Session s[_1_], TextBuf textbuf[_1_], Range range[_1_], const char* ln)
{
    (void)ln;
    try(validate_range_for_buffer(textbuf, range));
    Writer w;
    session_std_writer_init(&w, s);
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        try( session_write_unsigned_std(s, linum));
        try( session_write_std_lit__(s, "\t"));
        if (line.len) try( session_write_std(s, (char*)line.items, line.len));
        else try( session_write_std_lit__(s, "\n"));
    }

    if (textbuf_line_count(textbuf) == range->end) try( session_write_std_lit__(s, "\n"));
    return Ok;
}


Err dbg_print_all_lines_nums(
    Session s[_1_], TextBuf textbuf[_1_], Range r[_1_], const char* ln)
{
    (void)r;
    cmd_assert_no_params(ln);
    size_t lnum = 1;
    StrView line;
    for (;textbuf_get_line(textbuf, lnum, &line); ++lnum) {
        try( session_write_unsigned_std(s, lnum));
        try( session_write_std_lit__(s, "\t`"));
        if (line.len > 1) {
            try( session_write_std(s, (char*)line.items, line.len-1));
        } 
        try( session_write_std_lit__(s, "'\n"));
    }
    return Ok;
}


Err _cmd_textbuf_write_impl(
    Session s[_1_], TextBuf textbuf[_1_], Range r[_1_], const char* rest
) {
    if (!rest || !*rest) { return "cannot write without file arg"; }
    rest = cstr_trim_space((char*)rest);
    if (!*rest) return "invalid url name";
    FILE* fp = fopen(rest, "w");
    if (!fp) return err_fmt("%s: could not open file: %s", __func__, rest); 

    for (size_t linum = r->beg; linum <= r->end; ++linum) {
        StrView line;
        if (!textbuf_get_line(textbuf, linum, &line)) {
            file_close(fp);//TODO: do not ignore error
            return "error: invalid linum";
        }
        try(file_write_or_close(line.items, line.len, fp));
    }
    try(file_close(fp));
    return session_write_msg_lit__(s, "file written. ");
}

Err cmd_textbuf_global(CmdParams p[_1_]) {
    StrView pattern = parse_pattern(p->ln);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.items);
    return Ok;
}


/* input commands */


Err _cmd_input_text_set_(Session session[_1_], LxbNode n[_1_], const char* line) {
    UserOutput* out = session_uout(session);
    Err err = Ok;
    if (!*line) {
        try( out->write_std("> ", 1, session));
        ArlOf(char) masked = (ArlOf(char)){0};
        err = readpass_term(&masked, true);
        ok_then(err, lexbor_set_lit_attr__(lxbnode_node(n), "value", masked.items, masked.len));
        arlfn(char, clean)(&masked);
    } else {
        if (*line == ' ' || *line == '=') ++line;
        size_t len = strlen(line);
        if (len && line[len-1] == '\n') --len;
        err = lexbor_set_lit_attr__(lxbnode_node(n), "value", line, len);
    }
    ok_then(err, session_doc_draw(session));
    return err;
}

Err _cmd_input_select_set_(Session session[_1_], LxbNode n[_1_], const char* line) {
    ArlOf(LxbNodePtr)* matches = &(ArlOf(LxbNodePtr)){0};
    Err e = Ok;

    lxb_dom_node_t* first = lxbnode_node(n)->first_child;
    for(lxb_dom_node_t* it = first; it ; it = it->next) {
        if (it->type == LXB_DOM_NODE_TYPE_ELEMENT && it->local_name == LXB_TAG_OPTION) {
            StrView value = lexbor_get_lit_attr__(it, "value");
            size_t linelen = strlen(line);
            if (linelen <= value.len && value.items && !strncmp(line, value.items, linelen)) {
                if (!arlfn(LxbNodePtr, append)(matches, &it)) {
                    e = "error: arl append failure";
                    goto Clean_Matches;
                }
            }

            try_or_jump(e, Clean_Matches, lexbor_remove_lit_attr__(it, "selected"));
        }
    }

    if (len__(matches) == 0) e = "no matches";
    else if (len__(matches) == 1) {
        e = Ok;
        LxbNodePtr* selected = arlfn(LxbNodePtr,at)(matches,0);
        if (!selected) return "error: arlfn failure";
        e = lexbor_set_lit_attr__(*selected, "selected", "", 0);
        ok_then(e, session_doc_draw(session));
    } else e = "amiguous input";

Clean_Matches:
    arlfn(LxbNodePtr, clean)(matches);
    return e;
}


Err _cmd_input_ix_set_(Session session[_1_], const size_t ix, const char* ln) {
    HtmlDoc* d;
    LxbNode n = (LxbNode){0};
    try( session_current_doc(session, &d));
    try( _get_lexbor_node_ptr_by_ix(htmldoc_inputs(d), ix, &n.n));

    if (lexbor_lit_attr_has_lit_value(&n, "type", "text"))
        return _cmd_input_text_set_(session, &n, ln);
    else if (lexbor_lit_attr_has_lit_value(&n, "type", "search"))
        return _cmd_input_text_set_(session, &n, ln); //TODO: implement it properly
    else if (lexbor_lit_attr_has_lit_value(&n, "type", "password"))
        return _cmd_input_text_set_(session, &n, ln);
    else if (lexbor_node_tag_is_select(&n)) return _cmd_input_select_set_(session, &n, ln);

    return "input set not supported for element";
}

/* image commands */

Err _get_image_by_ix(Session session[_1_], size_t ix, lxb_dom_node_t* outnode[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* images = htmldoc_imgs(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err _cmd_image_print(Session session[_1_], size_t ix) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    Str* buf = &(Str){0};
    Err err = lexbor_node_to_str(node, buf);
    ok_then(err, session_write_msg(session, items__(buf), len__(buf)));
    str_clean(buf);
    return err;
}

Err _cmd_image_save(Session session[_1_], size_t ix, const char* fname) {
    lxb_dom_node_t* node;
    try( _get_image_by_ix(session, ix, &node));
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    CURLU* curlu = url_cu(htmldoc_url(htmldoc));
    try( lexcurl_dup_curl_from_node_and_attr(node, "src", 3, &curlu));
    Err err = url_client_curlu_to_file(session_url_client(session), curlu, fname);
    curl_url_cleanup(curlu);
    ok_then(err, session_write_msg_lit__(session, "data saved\n"));
    return err;
}


/* anchor commands */

Err _get_anchor_by_ix(Session session[_1_], size_t ix, lxb_dom_node_t* outnode[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(htmldoc);
    LxbNodePtr* nodeptr = arlfn(LxbNodePtr, at)(anchors, ix);
    if (!nodeptr) return "anchor number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err _cmd_anchor_print(Session session[_1_], size_t linknum) {
    lxb_dom_node_t* a;
    try( _get_anchor_by_ix(session, linknum, &a));
    
    Str* buf = &(Str){0};
    try( lexbor_node_to_str(a, buf));

    Err err = session_write_msg(session, items__(buf), len__(buf));
    str_clean(buf);
    return err;
}


Err _cmd_anchor_save(Session session[_1_], size_t ix, const char* fname) {
    lxb_dom_node_t* node;
    try( _get_anchor_by_ix(session, ix, &node));
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    CURLU* curlu = url_cu(htmldoc_url(htmldoc));
    try( lexcurl_dup_curl_from_node_and_attr(node, "href", 4, &curlu));
    Err err = url_client_curlu_to_file(session_url_client(session), curlu, fname);
    curl_url_cleanup(curlu);
    ok_then(err, session_write_msg_lit__(session, "file saved\n"));
    return err;
}

