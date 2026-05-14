#include "sys.h"
#include "bookmark.h"
#include "cmd.h"
#include "draw.h"
#include "range-parse.h"
#include "readpass.h"
#include "url.h"
#include "generic.h"


#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset


Err _get_image_by_ix(Session session[_1_], size_t ix, DomNode outnode[_1_]);
static Err save_request_to_file(CmdParams p[_1_], Request r[_1_]);

static bool _is_url_alias_(const char* cmd) { return cmd[0] == '\\'; }

Err
dom_node_range_to_request_arl(
    Range          range[_1_],
    ArlOf(DomNode) ns[_1_],
    CmdParams      p[_1_],
    HttpMethod     method,
    StrView        url_attr,
    ArlOf(Request) reqs[_1_]
) {
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    for_range(range, i) {
        Request*r ;
        DomNode* nodeptr = arlfn(DomNode, at)(ns, i);
        if (!nodeptr) return err_fmt("error: unexpected invalid index (%ld) for node list", i);

        StrView url = dom_node_attr_value(*nodeptr, url_attr);

        try(arl_append_zero(Request,reqs,r));
        Err e = request_init(r, method, url, htmldoc_url(d));
        if (e) {
            msg_ln__(p, e);
            request_clean(r);
            if (!reqs->len) return err_internal("arl empry after append");
            --reqs->len;//TODO: use arl_pop
        }
    }
    return Ok;
}

Err
foreach_request_save_to_file(CmdParams p[_1_], ArlOf(Request) rs[_1_]) {
    foreach__(Request,rs,r) {
        Err e = save_request_to_file(p, r);
        if (e) msg_ln__(p, e);
    }
    return Ok;
}

static Err
cmd_save_url(CmdParams p[_1_], StrView url) {
    HtmlDoc* htmldoc;
    try( session_current_doc(p->s, &htmldoc));

    Err     e   = Ok;
    Request r   = (Request){0};

    tryjmp(e, Clean, request_init(&r, http_get, url, htmldoc_url(htmldoc)));
    tryjmp(e, Clean, save_request_to_file(p, &r));

Clean:
    request_clean(&r);
    return e;
}

/* session commands */
static Err
cmd_fetch_request_(CmdParams p[_1_], HttpMethod method) {
    Request r = (Request){0};
    Str     u = (Str){0};
    Err     e = Ok;
    p->ln     = cstr_trim_space((char*)p->ln);

    if (_is_url_alias_(p->ln)) {
        try (get_url_alias(p->s, cstr_skip_space(p->ln + 1), &u));
        tryjmp(e,Fail, request_init(&r, method, sv(u), NULL));
    } else
        tryjmp(e,Fail, request_from_userln(&r, p->ln, method));

    tryjmp(e,Fail, session_fetch_request(p->s, &r, session_url_client(p->s), cmd_params_cmd_out(p)));
    str_clean(&u);
    return Ok;
Fail:
    str_clean(&u);
    request_clean(&r);
    return e;
}

Err cmd_get(CmdParams p[_1_]) { return cmd_fetch_request_(p, http_get); }
Err cmd_post(CmdParams p[_1_]) { return cmd_fetch_request_(p, http_post); }


Err cmd_set_session_winsz(CmdParams p[_1_]) {
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    *session_nrows(p->s) = nrows;
    *session_ncols(p->s) = ncols;
    
    CmdOut* out = cmd_params_cmd_out(p);
    try( msg__(out, svl("win: nrows = "))); 
    try( cmd_out_msg_append_ui_as_base10(out, nrows));
    try( msg__(out, svl(", ncols = ")));
    try( cmd_out_msg_append_ui_as_base10(out, ncols));
    try( msg__(out, svl("\n")));
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
    msg_ln__(p, svl("session colors updated"));
    return Ok;
}

Err cmd_set_session_js(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    if (*p->ln == '0') session_js_set(p->s, false);
    else if (*p->ln == '1') session_js_set(p->s, true);
    else return "js option should be '0' or '1'";
    msg_ln__(p, svl("js updated"));
    return Ok;
}

Err cmd_set_session_forms(CmdParams p[_1_]) {
    p->ln = cstr_skip_space(p->ln);
    char c = p->ln[0]; 
    if (c != '0' && c != '1') return "set forms option should be '0' or '1'";
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
    if ((rest = cmd_params_match(p, "fgets", 1)) && !*rest) ui = ui_fgets();
    else if ((rest = cmd_params_match(p, "isocline", 1)) && !*rest) ui = ui_isocline();
    else if ((rest = cmd_params_match(p, "visual", 1)) && !*rest) ui = ui_vi_mode();
    else return "input option should be 'getline', 'isocline' or 'visual'";
    ui_switch(session_ui(p->s), &ui);
    return Ok;
}


/* curl commands */
Err
cmd_curl_cookies(CmdParams p[_1_]) {
    return url_client_print_cookies(p->s, session_url_client(p->s), cmd_params_cmd_out(p));
}

Err
cmd_curl_version(CmdParams p[_1_]) {
    char* version = curl_version();
    return msg__(cmd_params_cmd_out(p), version);
}


/* doc commands */

Err
cmd_doc_draw(CmdParams p[_1_]) { return session_doc_draw(p->s, cmd_params_cmd_out(p)); }

Err
cmd_doc_js(CmdParams p[_1_]) { return session_doc_js(p->s, cmd_params_cmd_out(p)); }

Err
cmd_doc_console(CmdParams p[_1_]) {
    return session_doc_console(p->s, p->ln, cmd_params_cmd_out(p));
}


/* tabs commands */


Err
cmd_tabs_info(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_info(f, cmd_params_cmd_out(p));
}


Err
cmd_tabs_back(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    TabList* f = session_tablist(p->s);
    return tablist_back(f);
}


Err
cmd_tabs_goto(CmdParams p[_1_]) {
    TabList* f = session_tablist(p->s);
    return tablist_move_to_node(f, p->ln);
}
/* HtmlDoc commands */
/*
 * These commands require a valid document, the caller should check this condition before
 */

Err cmd_doc_info(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_print_info(d, cmd_params_cmd_out(p));
}


Err cmd_doc_A(CmdParams p[_1_]) {
    cmd_assert_no_params(p->ln);
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return htmldoc_A(p->s, d, cmd_params_cmd_out(p));
}


Err cmd_doc_bookmark_add(CmdParams p[_1_]) {
    return bookmark_add_to_section(p->s, p->ln, session_url_client(p->s), cmd_params_cmd_out(p));
}


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



/* TextBuf commands */


Err cmd_textbuf_print(CmdParams p[_1_]) {
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return session_write_screen_range_mod(p->s, p->tb, &rng, cmd_params_cmd_out(p));
}


Err cmd_textbuf_print_n(CmdParams p[_1_]) {
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return  _textbuf_print_n_(p->tb, &rng, p->ln, cmd_params_cmd_out(p));
}


Err cmd_textbuf_write(CmdParams p[_1_]) {
//TODO: allow >/>> modes
    const char* filename = cstr_trim_space((char*)p->ln);
    if (range_parse_is_none(&p->rp)) {
        try( textbuf_to_file(p->tb, filename, "w"));
        return msg__(cmd_params_cmd_out(p), svl("file written."));
    }
    Range rng;
    try (textbuf_range_from_parsed_range(p->tb, &p->rp, &rng));
    return _cmd_textbuf_write_impl(p->tb, &rng, filename, cmd_params_cmd_out(p));
}


//TODO: apply mods to text
Err _textbuf_print_n_(TextBuf textbuf[_1_], Range range[_1_], const char* ln, CmdOut* out) {
    (void)ln;
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        try( cmd_out_screen_append_ui_as_base10(out, linum));
        try( screen__(out, svl("\t")));
        if (line.len) try( screen__(out, line));
        else try( screen__(out, svl("\n")));
    }

    if (textbuf_line_count(textbuf) == range->end) try( screen__(out, svl("\n")));
    return Ok;
}



Err _cmd_textbuf_write_impl(TextBuf textbuf[_1_], Range r[_1_], const char* rest, CmdOut* out) {
    FILE* fp;
    try(file_open(rest, "w", &fp));

    for (size_t linum = r->beg; linum <= r->end; ++linum) {
        StrView line;
        if (!textbuf_get_line(textbuf, linum, &line)) {
            file_close(fp);//TODO: do not ignore error
            return "error: invalid linum";
        }
        try(file_write_or_close(line.items, line.len, fp));
    }
    try(file_close(fp));
    return msg__(out, svl("file written. "));
}


static StrView parse_pattern(const char* tk) {
    StrView res = {0};
    if (!tk) { return res; }
    char delim = '/';
    tk = cstr_skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) res = (StrView){.items = tk, .len = strlen(tk)};
    else {
        res = (StrView){.items = tk, .len = cast__(size_t)(end-tk)};
        *(char*)end = '\0';
    }

    return res;
}


Err cmd_textbuf_global(CmdParams p[_1_]) {
    ArlOf(size_t) lines = (ArlOf(size_t)){0};
    Err err = Ok;

    if (!p->tb) return err_internal("expectind textbuf set at this point");
    StrView pattern = parse_pattern(p->ln);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }

    tryjmp(err,Clean, textbuf_get_lines_matching_regex(p->tb, pattern, &lines));

    StrView line;
    foreach__(size_t,&lines,linenum) {
        if (!textbuf_get_line(p->tb, *linenum, &line)) {err=err_internal("expecting matched line");goto Clean;}
        if (!line.len || !line.items || !*line.items) {err=err_internal("expecting matched nonempty line");goto Clean;}
        msg__(cmd_params_cmd_out(p), line);
    }

Clean:
    arlfn(size_t,clean)(&lines);
    return err;
}

/* input commands */


Err
_cmd_form_print(CmdParams p[_1_], size_t ix) {
    HtmlDoc* d;
    try( session_current_doc(p->s, &d));
    return _cmd_lexbor_node_print_(htmldoc_forms(d), ix, cmd_params_cmd_out(p));
}


static
Err cmd_select_elem_show_options(DomNode lbn[_1_], CmdOut out [_1_]) {
    if (isnull(*lbn)) return "error: expecting lexbor node, not NULL";
    for(DomNode it = dom_node_first_child(*lbn); !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_tag(it) == HTML_TAG_OPTION) {

            if (dom_node_has_attr(it, svl("selected"))) msg__(out, svl(" *\t"));
            else msg__(out, svl("\t"));

            msg__(out, svl(" | value: ")); 
            StrView value = dom_node_attr_value(it, svl("value"));
            if (value.len) msg_ln__(out, value);
            else msg__(out, svl("\"\"\n")); 

        }
    }
    return Ok;
}


Err
cmd_input_default_node(CmdParams p[_1_], DomNode node) {
    TabNode* tab;
    try( tablist_current_tab(session_tablist(p->s), &tab));

    if (dom_node_tag(node) == HTML_TAG_INPUT
    &&  dom_node_attr_has_value(node, svl("type"), svl("submit"))) 
        return tab_node_tree_append_submit_input_node(
            tab, node, session_url_client(p->s), p->s, cmd_params_cmd_out(p)
        );
    else if (dom_node_tag(node) == HTML_TAG_BUTTON
    &&  (dom_node_attr_has_value(node, svl("type"), svl("submit"))
        || !dom_node_has_attr(node, svl("type"))))
        return tab_node_tree_append_submit_input_node(
            tab, node, session_url_client(p->s), p->s, cmd_params_cmd_out(p)
        );
    else if (dom_node_tag(node) == HTML_TAG_SELECT)
        return cmd_select_elem_show_options(&node, cmd_params_cmd_out(p));
    
    return "error: invalid input node";
}


static Err
_get_dom_node_at_(ArlOf(DomNode) node_arl[_1_], size_t ix, DomNode outnode[_1_]) {
    DomNode* nodeptr = arlfn(DomNode, at)(node_arl, ix);
    if (!nodeptr) return "input element number invalid";
    *outnode = *nodeptr;
    return Ok;
}


static Err
_cmd_input_text_set_(Session session[_1_], DomNode n[_1_], const char* line, CmdOut cout[_1_]) {
    Err err = Ok;
    if (!*line) {
        try( screen__(cout, svl("> ")));
        try( session_flush_cmd_out(session, cout));
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
    ok_then(err, session_doc_draw(session, cout));
    return err;
}


static Err _cmd_input_select_set_(Session session[_1_], DomNode n[_1_], const char* line, CmdOut cmd_out[_1_]) {
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

            tryjmp(e, Clean_Matches, dom_node_remove_attr(it, svl("selected")));
        }
    }

    if (len__(matches) == 0) e = "no matches";
    else if (len__(matches) == 1) {
        e = Ok;
        DomNode* selected = arlfn(DomNode,at)(matches,0);
        if (!selected) return "error: arlfn failure";
        e = dom_node_set_attr(*selected, svl("selected"), svl(""));
        ok_then(e, session_doc_draw(session, cmd_out));
    } else e = "amiguous input";

Clean_Matches:
    arlfn(DomNode, clean)(matches);
    return e;
}


Err
cmd_input_info_node(CmdParams p[_1_], DomNode n) {
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(n, buf);
    ok_then(err,  msg__(cmd_params_cmd_out(p), buf));
    str_clean(buf);
    return err;
}


Err cmd_input_set_node(CmdParams p[_1_], DomNode node) {
    Session* session = p->s;
    const char* ln   = p->ln;

    if (dom_node_attr_has_value( node, svl("type"), svl("text")))
        return _cmd_input_text_set_(session, &node, ln, cmd_params_cmd_out(p));
    else if (dom_node_attr_has_value(node, svl("type"), svl("search")))
        return _cmd_input_text_set_(session, &node, ln, cmd_params_cmd_out(p)); //TODO: implement it properly
    else if (dom_node_attr_has_value(node, svl("type"), svl("password")))
        return _cmd_input_text_set_(session, &node, ln, cmd_params_cmd_out(p));
    else if (dom_node_tag(node) == HTML_TAG_SELECT) return _cmd_input_select_set_(session, &node, ln, cmd_params_cmd_out(p));
    else if (!dom_node_has_attr(node, svl("type")))
        return _cmd_input_text_set_(session, &node, ln, cmd_params_cmd_out(p));

    return "input set not supported for element";
}


Err cmd_input_save_node(CmdParams p[_1_], DomNode node) {
    if (!dom_node_attr_has_value(node, svl("type"), svl("submit")))
        return "warn: save command only applicable to submit inputs\n";

    HtmlDoc* htmldoc;
    try( session_current_doc(p->s, &htmldoc));
    DomNode form = dom_node_find_parent_form(node);
    if (isnull(form)) return "expected form, not found";
    Err err = Ok;
    Request r = (Request){0};
    tryjmp(err,Clean, request_from_form_node(&r, form, true, htmldoc_url(htmldoc)));
    err = save_request_to_file(p, &r);

Clean:
    request_clean(&r);
    return err;
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

Err cmd_image_print(CmdParams p[_1_], DomNode node) {
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(node, buf);
    ok_then(err, msg__(cmd_params_cmd_out(p), buf));
    str_clean(buf);
    return err;
}


Err cmd_image_save(CmdParams p[_1_], DomNode node) {
    return cmd_save_url(p, dom_node_attr_value(node, svl("src")));
}


/* anchor commands */


Err cmd_anchor_asterisk(CmdParams p[_1_], DomNode node) {
    Session*  s = p->s;
    if (!s) return "error: expecting a session";
    CmdOut* out = cmd_params_cmd_out(p);
    TabNode* current_tab;
    try( tablist_current_tab(session_tablist(s), &current_tab));
    if(current_tab) {
        try( tab_node_tree_append_ahref_from_node(current_tab , node, session_url_client(s), s, out));
        //TODO: the will depend on whther is a single url or a range
        return tablist_back(session_tablist(s));
    }
    
    return "error: where is the href if current tree is empty?";
}

Err
cmd_anchor_save(CmdParams p[_1_], DomNode node) {
    return cmd_save_url(p, dom_node_attr_value(node, svl("href")));
}


Err cmd_print_node(CmdParams p[_1_], DomNode node) {
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(node, buf);
    ok_then(err,  msg__(cmd_params_cmd_out(p), buf));
    str_clean(buf);
    return err;
}

Err _cmd_lexbor_node_print_(ArlOf(DomNode) node_arl[_1_], size_t ix, CmdOut out[_1_]) {
    DomNode node;
    try( _get_dom_node_at_(node_arl, ix, &node));
    Str* buf = &(Str){0};
    Err err = dom_node_to_str(node, buf);

    ok_then(err,  msg__(out, buf));
    str_clean(buf);
    return err;
}


static Err save_request_to_file(CmdParams p[_1_], Request r[_1_]) {
    Err     e           = Ok;
    FILE*   fp          = NULL;
    Str     actual_path = (Str){0};
    CmdOut* out         = cmd_params_cmd_out(p);

    
    tryjmp(e,Clean, fopen_or_append_fopen(p->ln, *request_url(r), &fp, &actual_path));
    tryjmp(e,Clean, request_to_file(r, session_url_client(p->s), fp));

    tryjmp(e,Clean, msg__(out, svl("file saved: ")));
    tryjmp(e,Clean, msg_ln__(out, sv(actual_path)));

Clean:
    file_close(fp);
    str_clean(&actual_path);
    return e;
}

