#include "bookmark.h"
#include "cmd.h"
#include "draw.h"
#include "range-parse.h"
#include "readpass.h"


#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset

Err _get_image_by_ix(Session session[_1_], size_t ix, DomNode outnode[_1_]);

static bool _is_url_alias_(const char* cmd) { return *cmd == '\\'; }

/* session commands */
Err cmd_get(CmdParams p[_1_]) {
    p->ln = cstr_trim_space((char*)p->ln);
    Request r;
    if (_is_url_alias_(p->ln)) {
        Str u = (Str){0};
        try (get_url_alias(p->s, cstr_skip_space(p->ln + 1), &u));
        Err err = request_init_move_urlstr(&r, http_get, &u, NULL);
        ok_then(err,
            session_fetch_request(p->s, &r, session_url_client(p->s), cmd_params_cmd_out(p)));
        request_clean(&r);
        return err;
    }
    try (request_from_userln(&r, p->ln, http_get));
    Err err = session_fetch_request(p->s, &r, session_url_client(p->s), cmd_params_cmd_out(p));
    if (err) request_clean(&r);
    return err;
}


Err cmd_post(CmdParams p[_1_]) {
    p->ln = cstr_trim_space((char*)p->ln);
    Request r;
    if (_is_url_alias_(p->ln)) {
        Str u = (Str){0};
        try (get_url_alias(p->s, cstr_skip_space(p->ln + 1), &u));
        Err err = request_init_move_urlstr(&r, http_get, &u, NULL);
        ok_then(err,
            session_fetch_request(p->s, &r, session_url_client(p->s), cmd_params_cmd_out(p)));
        request_clean(&r);
        return err;
    }
    try (request_from_userln(&r, p->ln, http_post));
    Err err = session_fetch_request(p->s, &r, session_url_client(p->s), cmd_params_cmd_out(p));
    if (err) request_clean(&r);
    return err;
}


Err cmd_set_session_winsz(CmdParams p[_1_]) {
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(p->s) = nrows;
    *session_ncols(p->s) = ncols;
    
    CmdOut* out = cmd_params_cmd_out(p);
    try( cmd_out_msg_append(out, svl("win: nrows = "))); 
    try( cmd_out_msg_append_ui_as_base10(out, nrows));
    try( cmd_out_msg_append(out, svl(", ncols = ")));
    try( cmd_out_msg_append_ui_as_base10(out, ncols));
    try( cmd_out_msg_append(out, svl("\n")));
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


Err cmd_fetch(Session session[_1_], CmdOut* out) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    HtmlDoc newdoc;
    /* we reuse the request, but the Url is duplicated on fetch. We could
     * instead try to not duplicate it in this specific case, but this is
     * easier because the rest of the fetchs need to duplicate. */
    Url u = htmldoc_request(htmldoc)->url;
    try(htmldoc_init_move_request(
        &newdoc,
        htmldoc_request(htmldoc),
        session_url_client(session),
        session,
        out
    ));
    url_cleanup(&u);
    htmldoc_cleanup(htmldoc);
    *htmldoc = newdoc;
    return Ok;
}


Err shortcut_z(Session session[_1_], const char* rest, CmdOut cmd_out[_1_]) {
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


    try( session_write_screen_range_mod(session, tb, &r, cmd_out));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

/* //TODO: use it or delete it */
/* Err _cmd_misc(Session session[_1_], const char* line, CmdOut cout[_1_]) { */
/*     const char* rest = 0x0; */
/*     line = cstr_skip_space(line); */
/*     if ((rest = csubstr_match(line, "attr", 2))) { return "TODO: attr"; } */
/*     if ((rest = csubstr_match(line, "class", 3))) { return "TODO: class"; } */
/*     ///if ((rest = csubstr_match(line, "clear", 3))) { return cmd_clear(session); } */
/*     /1* if ((rest = csubstr_match(line, "fetch", 1))) { return _cmd_fetch(session); } *1/ */
/*     if ((rest = csubstr_match(line, "tag", 2))) { return _cmd_misc_tag(rest, session); } */
/*     if ((rest = csubword_match(line, "z", 1))) { return shortcut_z(session, rest, cout); } */
/*     return "unknown cmd"; */
/* } */

/* textbuf commands */

StrView parse_pattern(const char* tk) {
    StrView res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = cstr_skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) res = (StrView){.items = tk, .len = strlen(tk)};
    else      res = (StrView){.items = tk, .len = cast__(size_t)(end-tk)};

    return res;
}


//
//TODO: apply mods to text
Err _textbuf_print_n_(TextBuf textbuf[_1_], Range range[_1_], const char* ln, CmdOut* out) {
    (void)ln;
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        try( cmd_out_screen_append_ui_as_base10(out, linum));
        try( cmd_out_screen_append(out, svl("\t")));
        if (line.len) try( cmd_out_screen_append(out, line));
        else try( cmd_out_screen_append(out, svl("\n")));
    }

    if (textbuf_line_count(textbuf) == range->end) try( cmd_out_screen_append(out, svl("\n")));
    return Ok;
}


Err dbg_print_all_lines_nums(TextBuf textbuf[_1_], Range r[_1_], const char* ln, CmdOut* out) {
    (void)r;
    cmd_assert_no_params(ln);
    size_t lnum = 1;
    StrView line;
    for (;textbuf_get_line(textbuf, lnum, &line); ++lnum) {
        try( cmd_out_screen_append_ui_as_base10(out, lnum));
        try( cmd_out_screen_append(out, svl("\t`")));
        if (line.len > 1) {
            try( cmd_out_screen_append(out, sv(line.items, line.len-1)));
        } 
        try( cmd_out_screen_append(out, svl("'\n")));
    }
    return Ok;
}


Err _cmd_textbuf_write_impl(TextBuf textbuf[_1_], Range r[_1_], const char* rest, CmdOut* out) {
    (void)out;
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
    return cmd_out_msg_append(out, svl("file written. "));
}

Err cmd_textbuf_global(CmdParams p[_1_]) {
    StrView pattern = parse_pattern(p->ln);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.items);
    return Ok;
}


/* input commands */


static Err
_cmd_input_text_set_(Session session[_1_], DomNode n[_1_], const char* line, CmdOut cout[_1_]) {
    Err err = Ok;
    if (!*line) {
        try( cmd_out_screen_append(cout, svl("> ")));
        ArlOf(char) masked = (ArlOf(char)){0};
        err = readpass_term(&masked, true);
        ok_then(err, dom_node_set_attr(*n, svl("value"), sv(&masked)));
        arlfn(char, clean)(&masked);
    } else {
        if (*line == ' ' || *line == '=') ++line;
        size_t len = strlen(line);
        if (len && line[len-1] == '\n') --len;
        err = dom_node_set_attr(*n, svl("value"), sv(line, len));
    }
    ok_then(err, session_doc_draw(session));
    return err;
}

static Err _cmd_input_select_set_(Session session[_1_], DomNode n[_1_], const char* line) {
    ArlOf(DomNode)* matches = &(ArlOf(DomNode)){0};
    Err e = Ok;

    DomNode first = dom_node_first_child(*n);
    for(DomNode it = first; !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_type(it) == DOM_NODE_TYPE_ELEMENT && dom_node_tag(it) == HTML_TAG_OPTION) {
            StrView value = dom_node_attr_value(it, svl("value"));
            size_t linelen = strlen(line);
            if (linelen <= value.len && value.items && !strncmp(line, value.items, linelen)) {
                if (!arlfn(DomNode, append)(matches, &it)) {
                    e = "error: arl append failure";
                    goto Clean_Matches;
                }
            }

            try_or_jump(e, Clean_Matches, dom_node_remove_attr(it, svl("selected")));
        }
    }

    if (len__(matches) == 0) e = "no matches";
    else if (len__(matches) == 1) {
        e = Ok;
        DomNode* selected = arlfn(DomNode,at)(matches,0);
        if (!selected) return "error: arlfn failure";
        e = dom_node_set_attr(*selected, svl("selected"), svl(""));
        ok_then(e, session_doc_draw(session));
    } else e = "amiguous input";

Clean_Matches:
    arlfn(DomNode, clean)(matches);
    return e;
}


Err _cmd_input_ix_set_(CmdParams p[_1_], const size_t ix) {
    Session* session = p->s;
    const char* ln   = p->ln;
    HtmlDoc* d;
    DomNode n = (DomNode){0};
    try( session_current_doc(session, &d));
    try( _get_dom_node_at_(htmldoc_inputs(d), ix, &n));

    if (dom_node_attr_has_value( n, svl("type"), svl("text")))
        return _cmd_input_text_set_(session, &n, ln, cmd_params_cmd_out(p));
    else if (dom_node_attr_has_value(n, svl("type"), svl("search")))
        return _cmd_input_text_set_(session, &n, ln, cmd_params_cmd_out(p)); //TODO: implement it properly
    else if (dom_node_attr_has_value(n, svl("type"), svl("password")))
        return _cmd_input_text_set_(session, &n, ln, cmd_params_cmd_out(p));
    else if (dom_node_tag(n) == HTML_TAG_SELECT) return _cmd_input_select_set_(session, &n, ln);
    else if (!dom_node_has_attr(n, svl("type")))
        return _cmd_input_text_set_(session, &n, ln, cmd_params_cmd_out(p));

    return "input set not supported for element";
}

/* image commands */

Err _get_image_by_ix(Session session[_1_], size_t ix, DomNode outnode[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(DomNode)* images = htmldoc_imgs(htmldoc);
    DomNode* nodeptr = arlfn(DomNode, at)(images, ix);
    if (!nodeptr) return "image number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err _cmd_image_print(CmdParams p[_1_], size_t ix) {
    DomNode node;
    try( _get_image_by_ix(p->s, ix, &node));
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(node, buf);
    ok_then(err, cmd_out_msg_append_str(cmd_params_cmd_out(p), buf));
    str_clean(buf);
    return err;
}

Err _cmd_image_save(CmdParams p[_1_], size_t ix) {
    const char* fname = p->ln;
    DomNode node;
    try( _get_image_by_ix(p->s, ix, &node));
    HtmlDoc* htmldoc;
    try( session_current_doc(p->s, &htmldoc));

    Str urlstr = (Str){0};
    try (dom_node_append_null_terminated_attr_to_str(node, svl("src"), &urlstr));

    Err err = Ok;
    Request r;
    try_or_jump(err, Clean_Url_Str,
        request_init_move_urlstr(&r,http_get, &urlstr, htmldoc_url(htmldoc)));
    try_or_jump(err, Clean_Request, url_from_request(&r, session_url_client(p->s)));

    err = request_to_file(&r, session_url_client(p->s), fname);

    ok_then(err, cmd_out_msg_append(cmd_params_cmd_out(p), svl("data saved\n")));
Clean_Request:
    request_clean(&r);
    return err;
Clean_Url_Str:
    str_clean(&urlstr);
    return err;
}


/* anchor commands */

static Err _get_anchor_by_ix(Session session[_1_], size_t ix, DomNode outnode[_1_]) {
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(DomNode)* anchors = htmldoc_anchors(htmldoc);
    DomNode* nodeptr = arlfn(DomNode, at)(anchors, ix);
    if (!nodeptr) return "anchor number invalid";
    *outnode = *nodeptr;
    return Ok;
}

Err _cmd_anchor_print(CmdParams p[_1_], size_t linknum) {
    DomNode a;
    try( _get_anchor_by_ix(p->s, linknum, &a));
    
    Str* buf = &(Str){0};
    try( dom_node_to_str(a, buf));

    //>< write to CmdOut
    Err err = cmd_out_msg_append_str(cmd_params_cmd_out(p), buf);
    str_clean(buf);
    return err;
}


Err _cmd_anchor_save(Session session[_1_], size_t ix, const char* fname, CmdOut* out) {
    DomNode node;
    try( _get_anchor_by_ix(session, ix, &node));
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    Str urlstr = (Str){0};
    try (dom_node_append_null_terminated_attr_to_str(node, svl("href"), &urlstr));

    Err err = Ok;
    Request r;
    try_or_jump(err, Clean_Url_Str,
        request_init_move_urlstr(&r, http_get, &urlstr, htmldoc_url(htmldoc)));
    try_or_jump(err, Clean_Request, url_from_request(&r, session_url_client(session)));

    err = request_to_file(&r, session_url_client(session), fname);

    ok_then(err, cmd_out_msg_append(out, svl("file saved\n")));
Clean_Request:
    request_clean(&r);
    return err;
Clean_Url_Str:
    str_clean(&urlstr);
    return err;
}


Err _cmd_lexbor_node_print_(ArlOf(DomNode) node_arl[_1_], size_t ix, CmdOut out[_1_]) {
    DomNode node;
    try( _get_dom_node_at_(node_arl, ix, &node));
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(node, buf);

    ok_then(err,  cmd_out_msg_append_str(out, buf));
    str_clean(buf);
    return err;
}

Err bookmark_add_to_section(
    Session s[_1_], const char* line, UrlClient url_client[_1_], CmdOut cmd_out[_1_]
) {
    HtmlDoc* d;
    try( session_current_doc(s, &d));

    line = cstr_skip_space(line);
    bool create_section_if_not_found = true;
    bool match_prefix                = false;
    if (*line == '/') {
        create_section_if_not_found = false;
        match_prefix                = true;
        line                        = cstr_skip_space(++line);
    }
    if (!*line) return "not a valid bookmark section";
    Err err = Ok; 
    
    HtmlDoc bm;
    Str bm_path = (Str){0};
    try(resolve_bookmarks_file(items__(session_bookmarks_fname(s)), &bm_path));
    try_or_jump(err, Fail_Clean_Bm,
            get_bookmarks_doc(url_client, sv(&bm_path), cmd_out,  &bm));

    char* url;
    if ((err = url_cstr_malloc(htmldoc_url(d), &url))) goto Clean_Bm_Path_And_BmDoc;

    DomNode body;
    if ((err = bookmark_sections_body(&bm, &body))) goto Free_Curl_Url;

    Str* title = &(Str){0};
    if ((err = htmldoc_title_or_url(d, url, title))) goto Free_Curl_Url;

    DomElem bm_entry;
    if ((err = bookmark_mk_entry(bm.dom, url, title, &bm_entry))) goto Clean_Title;

    DomNode section_ul;
    if ((err = bookmark_section_ul_get(body, line, &section_ul, match_prefix))) goto Clean_Title;
    //TODO: wrap this
    if (!isnull(section_ul)) {
        dom_node_insert_child(section_ul, bm_entry);
    } else if(create_section_if_not_found) {
        err = bookmark_section_insert(bm.dom, body, line, bm_entry);
    } else err = "section not found in bookmarks file";

    ok_then(err, bookmarks_save_to_disc(&bm, sv(&bm_path)));
    ok_then(err, cmd_out_msg_append(cmd_out, svl("bookmark added\n")));

Clean_Title:
    str_clean(title);
Free_Curl_Url:
    curl_free(url);
Clean_Bm_Path_And_BmDoc:
    htmldoc_cleanup(&bm);
    str_clean(&bm_path);
    return err;
Fail_Clean_Bm:
    str_clean(&bm_path);
    return err;
}
