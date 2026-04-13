#include "htmldoc.h"
#include "draw.h" 
#include "bookmark.h"
#include "session.h"

static Err _bm_to_source_rec_tag_(DomNode node, Str out[_1_]);
static Err _bm_to_source_append_text_(DomNode node, Str out[_1_]);
static Err _bm_to_source_rec_(DomNode node, Str out[_1_]) ;


/* external linkage */

#define _htmldoc_fetch_bookmark_ htmldoc_fetch /* bookmark does not have js so it's simpler */
Err get_bookmarks_doc(
    UrlClient   url_client[_1_],
    StrView     bm_path,
    CmdOut      cmd_out[_1_],
    HtmlDoc     htmldoc_out[_1_]
) {
    Str* bm_url = &(Str){0};
    Err err = str_append(bm_url, svl("file://"));
    try(err);
    try_or_jump(err, Clean_Bm_Url, str_append(bm_url, bm_path));
    try_or_jump(err, Clean_Bm_Url,
        htmldoc_init_bookmark_move_urlstr(htmldoc_out, bm_url));
    FetchHistoryEntry e = (FetchHistoryEntry){0};
    err = _htmldoc_fetch_bookmark_(htmldoc_out, url_client, cmd_out, &e);
    fetch_history_entry_clean(&e);
    if (err) {
        htmldoc_cleanup(htmldoc_out);
Clean_Bm_Url:
        str_clean(bm_url);
    }
    return err;
}



Err bookmark_sections_body(HtmlDoc bookmark[_1_], DomNode out[_1_]) {
    Dom dom = htmldoc_dom(bookmark);
    DomNode node = dom_root(dom);
    if (isnull(node)) return "error: no document";
    if (!dom_node_has_type_document(node)) return "not a bookmark file";
    DomNode html = dom_node_first_child(node);
    if (isnull(html)) return "not a bookmark file: expecting an html tag";
    if (!dom_node_eq(html, dom_node_last_child(node))) return "not a bookmark file";
    if (!dom_node_has_tag_html(html)) return "not a bookmark file";
    DomNode first_child = dom_node_first_child(html);
    if (isnull(first_child)) return "not a bookmark: expecting head";
    if (!dom_node_has_tag_head(first_child)) return "not a bookmark file";
    DomNode html_last_child = dom_node_last_child(html);
    if (isnull(html_last_child)) return "not a bookmark: expecting body";
    if (!dom_node_has_tag_body(html_last_child)) return "not a bookmark file";
    *out = html_last_child;
    return Ok;
}


Err
cmd_bookmarks(CmdParams p[_1_]) {
    Session* session = p->s;
    const char* url = p->ln;
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(BufOf(char)) list = (ArlOf(BufOf(char))){0};
    DomNode body;
    try(bookmark_sections_body(htmldoc, &body));
    Err err = Ok;
    if (!*url) {
        err = bookmark_sections(body, &list);
        if (!err) {
            BufOf(char)* it = arlfn(BufOf(char), begin)(&list);
            for (; it != arlfn(BufOf(char), end)(&list); ++it) {
                try( cmd_out_msg_append_ln(cmd_params_cmd_out(p), it));
            }
        }
        arlfn(BufOf(char),clean)(&list);
    } else {
        DomNode section;
        try( bookmark_section_get(body, url, &section, /*match_prefix*/false));

        if (!isnull(section)) {
            StrView data = dom_node_text_view(dom_node_first_child(section));
            if (data.len) {
                try(cmd_out_msg_append_ln(cmd_params_cmd_out(p), &data));
            }
        } else err = "invalid section in bookmark";
    }
    return err;
}


Err
bookmark_sections(DomNode body, ArlOf(Str)* out) {
    for (DomNode it = dom_node_first_child(body)
        ; !dom_node_eq(it,dom_node_last_child(body))
        ; it = dom_node_next(it)
    ) {
        if (dom_node_has_tag(it, HTML_TAG_H2)) {
            if (!dom_node_eq(dom_node_first_child(it), dom_node_last_child(it)))
                return "invalid bookmark file";

            Str* buf = arlfn(Str,append)(out, &(Str){0});
            if (!buf) return "error: arl append failure";

            StrView data = dom_node_text_view(dom_node_first_child(it));
            if (data.len) try( str_append(buf, &data));
        }
    }

    return Ok;
}


Err
bookmark_section_insert(Dom dom, DomNode body, const char* q, DomElem bm_entry) {
    DomElem section;
    try(dom_elem_init(&section, dom, svl("h2")));
    DomText text;
    Err err =  dom_text_init(&text, dom, sv(q));
    if (err) goto Clean_Section;

    dom_node_insert_child(section, text);
    dom_node_insert_child(body, section);

    DomElem ul;
    err = dom_elem_init(&ul, dom, svl("ul"));
    if (err) goto Clean_Text;
    dom_node_insert_child(ul, bm_entry);
    dom_node_insert_child(body, ul);

    return Ok;

Clean_Text:
    dom_text_cleanup(text);
Clean_Section:
    dom_elem_cleanup(section);

    return err;
}


Err
bookmark_section_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix) {
    DomNode res = (DomNode){0};
    for (DomNode it = dom_node_first_child(body)
        ; !dom_node_eq(it, dom_node_last_child(body))
        ; it = dom_node_next(it)
    ) {
        if (dom_node_has_tag(it, HTML_TAG_H2)) {
            if (!dom_node_eq(dom_node_first_child(it),dom_node_last_child(it)))
                return "invalid bookmark file";

            StrView data = dom_node_text_view(dom_node_first_child(it));
            if (data.len) {
                size_t qlen = strlen(q);
                size_t len = match_prefix ? qlen : data.len;
                if (qlen <= len && strncmp(q, data.items, len) == 0) {
                    if (!isnull(res)) return "unequivocal reference to bookmark section";
                    res = it;
                }
            }
        }
    }
    *out = res;
    return Ok;
}


Err
bookmark_section_ul_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix) {
    *out = (DomNode){0};
    DomNode section;
    try( bookmark_section_get(body, q, &section, match_prefix));
    if (!isnull(section)) {
        if (isnull(dom_node_next(section))) return "error: expecting section's next (TEXT)";
        DomNode ul = dom_node_next(dom_node_next(section));//section->next->next;
        if (isnull(ul)) return "error: expecting section's next next";
        if (!dom_node_has_tag(ul, HTML_TAG_UL))
            return "error: expecting section's next next to be ul";
        *out = ul;
    }
    return Ok;
}


Err
bookmark_mk_anchor (Dom dom, char* href, Str text[_1_], DomElem out[_1_]) {
    try(dom_elem_init(out, dom, svl("a")));
    try(dom_elem_set_attr(*out, svl("href"), sv(href, strlen(href))));
    DomText dom_text;
    try(dom_text_init(&dom_text, dom, sv(text)));
    if (isnull(dom_text)) return "error: lxb text create failure";//TODO: clean *out

    dom_node_insert_child((*out), dom_text);
    return Ok;
}


Err
bookmark_mk_entry(Dom document, char* href, Str text[_1_], DomElem out[_1_]) {
    try( dom_elem_init(out, document, svl("li")));

    DomElem a;
    Err err = bookmark_mk_anchor(document, href, text, &a);
    if (err) {
        dom_elem_cleanup(*out);
        return err;
    }

    dom_node_insert_child((*out), a);
    return Ok;
}


Err _bm_to_source_rec_(DomNode node, Str out[_1_]) {
    if (!isnull(node)) {
        switch(dom_node_type(node)) {
            case DOM_NODE_TYPE_ELEMENT: return _bm_to_source_rec_tag_(node, out);
            case DOM_NODE_TYPE_TEXT: return _bm_to_source_append_text_(node, out);
            case DOM_NODE_TYPE_COMMENT: return Ok;
            default: {
                 //TODO: log this or do anythong else?
                //if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                //    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                //else log_warn__("%s\n", "Ignored Node Type in Bookmar file");
                return Ok;
            }
        }
    }
    return Ok;
}


/* internal linkage */

/* static void bookmark_sections_sort(ArlOf(BufOf(char))* list) { */
/*     qsort(list->items, list->len, sizeof(BufOf(char)), buf_of_char_cmp); */
/* } */


static Err _bm_to_source_rec_childs_(DomNode node, Str out[_1_]) {
    for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it)) {
        try( _bm_to_source_rec_(it, out));
        if (dom_node_eq(it, dom_node_last_child(node))) break;
    }
    return Ok;
}


static Err _bm_to_source_rec_childs_no_text_(DomNode node, Str out[_1_]) { for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_has_type_text(it)) continue; 
        try( _bm_to_source_rec_(it, out));
        if (dom_node_eq(it, dom_node_last_child(node))) break;
    }
    return Ok;
}


static Err _bm_to_source_rec_tag_a_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<a")));
    DomAttr attr = dom_node_first_attr(node);
    while (!isnull(attr)) {
        StrView name = dom_attr_name_view(attr);
        if (name.len)  {
            try( str_append(out, svl(" ")));
            try( str_append(out, &name));
        }
        StrView value = dom_attr_value_view(attr);
        if (value.len)  {
            try( str_append(out, svl("=")));
            try( str_append(out, &value));
        }
        attr = dom_attr_next(attr);
    }
    try( str_append(out, svl(">")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</a>")));
    return Ok;
}

static Err _bm_to_source_rec_tag_ul_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<ul>\n")));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    char* end = "<!--End of section (do not delete this comment)-->\n</ul>\n";
    try( str_append(out, end));
    return Ok;
}

static Err _bm_to_source_rec_tag_li_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<li>")));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    try( str_append(out, svl("\n")));
    return Ok;
}

static Err _bm_to_source_rec_tag_h1_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<h1>")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</h1>\n")));
    return Ok;
}

static Err _bm_to_source_rec_tag_h2_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<h2>")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</h2>\n")));
    return Ok;
}

static Err _bm_to_source_rec_tag_(DomNode node, Str out[_1_]) {
    switch(dom_node_tag(node)) {
        case HTML_TAG_A: return _bm_to_source_rec_tag_a_(node, out);
        case HTML_TAG_H1: return _bm_to_source_rec_tag_h1_(node, out);
        case HTML_TAG_H2: return _bm_to_source_rec_tag_h2_(node, out);
        case HTML_TAG_UL: return _bm_to_source_rec_tag_ul_(node, out);
        case HTML_TAG_LI: return _bm_to_source_rec_tag_li_(node, out);
        default: return _bm_to_source_rec_childs_no_text_(node, out);
    }
    return Ok;
}

static Err _bm_to_source_append_text_(DomNode node, Str out[_1_]) {
    StrView data = dom_node_text_view(node);
    return str_append(out, &data);
}

static Err bookmark_to_source(HtmlDoc bm[_1_], Str out[_1_]) {
    DomNode body;
    try( bookmark_sections_body(bm, &body));

    Err err = str_append(out, svl(AHRE_BOOKMARK_HEAD));
    if (err) {
        str_clean(out);
        return err;
    }
    try( _bm_to_source_rec_childs_no_text_(body, out));
    if (!err) {
        err = str_append(out, svl(AHRE_BOOKMARK_TAIL));
    }
    return err;
}


Err
bookmarks_save_to_disc(HtmlDoc bm[_1_], StrView bm_path) {
    Str source = (Str){0};
    try( bookmark_to_source(bm, &source));

    FILE* fp;
    try(file_open(bm_path.items, "w", &fp));

    Err err = Ok;
    size_t written = fwrite(source.items, 1, source.len, fp);
    if (written != source.len) err = "error: could not write to bookmarks file";
    try(file_close(fp));
    str_clean(&source);
    return err;
}


Err
bookmark_add_to_section(Session s[_1_], const char* line, UrlClient url_client[_1_], CmdOut cmd_out[_1_]) {
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
    if ((err = url_cstr_malloc(*htmldoc_url(d), &url))) goto Clean_Bm_Path_And_BmDoc;

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
    w_curl_free(url);
Clean_Bm_Path_And_BmDoc:
    htmldoc_cleanup(&bm);
    str_clean(&bm_path);
    return err;
Fail_Clean_Bm:
    str_clean(&bm_path);
    return err;
}
