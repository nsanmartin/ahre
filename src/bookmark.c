#include "draw.h" 
#include "debug.h"
#include "bookmark.h"
#include "cmd.h"

Err draw_bookmark_rec(lxb_dom_node_t* node) {
    if (node) {
        switch(node->type) {
            //case LXB_DOM_NODE_TYPE_ELEMENT: return draw_rec_tag(node);
            //case LXB_DOM_NODE_TYPE_TEXT: return draw_text(node);
            //TODO: do not ignore these types?
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                //return draw_list(node->first_child, node->last_child, NULL);
                puts("g"); return Ok;
            default: {
                 //TODO: log this or do anythong else?
                //if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                //    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                //else log_warn__("Ignored Node Type: %s\n", _dbg_node_types_[node->type]);
                return Ok;
            }
        }
    }
    return Ok;
}

Err cmd_bookmarks(CmdParams p[static 1]) {
    Session* session = p->s;
    const char* url = p->ln;
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(BufOf(char)) list = (ArlOf(BufOf(char))){0};
    lxb_dom_node_t* body;
    try(bookmark_sections_body(htmldoc, &body));
    SessionMemWriter w = (SessionMemWriter){.s=session, .write=session_writer_write_msg};
    Err err = Ok;
    if (!*url) {
        err = bookmark_sections(body, &list);
        if (!err) {
            BufOf(char)* it = arlfn(BufOf(char), begin)(&list);
            for (; it != arlfn(BufOf(char), end)(&list); ++it) {
                try( w.write(&w, items__(it), len__(it)));
                try( w.write(&w, "\n", 1));
            }
        }
        arlfn(BufOf(char),clean)(&list);
    } else {
        lxb_dom_node_t* section;
        try( bookmark_section_get(body, url, &section));

        if (section) {
            const char* data;
            size_t len;
            try( lexbor_node_get_text(section->first_child, &data, &len));
            if (len) {
                try(w.write(&w, (char*)data, len));
                try(w.write(&w, "\n", 1));
            }
        } else err = "invalid section in bookmark";
    }
    return err;
}

Err bookmark_add_to_section(HtmlDoc d[static 1], const char* line, UrlClient url_client[static 1]) {
    line = cstr_skip_space(line);
    bool create_section_if_not_found = true;
    if (*line == '/') {
        ++line;
        create_section_if_not_found = false;
        line = cstr_skip_space(line);
    }
    if (!*line) return "not a valid bookmark section";
    Err err = Ok; 
    
    HtmlDoc bm;
    Str bmfile = (Str){0};
    try(_get_bookmarks_doc_(url_client, &bmfile, &bm));

    char* url;
    if ((err = url_cstr(htmldoc_url(d), &url))) goto clean_bmfile_and_bmdoc;

    lxb_dom_node_t* body;
    if ((err = bookmark_sections_body(&bm, &body))) goto free_curl_url;

    Str* title = &(Str){0};
    if ((err = lxb_mk_title_or_url(d, url, title))) goto free_curl_url;

    lxb_dom_element_t* bm_entry;
    if ((err = bookmark_mk_entry(bm.lxbdoc, url, title, &bm_entry))) goto clean_title;

    lxb_dom_node_t* section_ul;
    if ((err = bookmark_section_ul_get(body, line, &section_ul))) goto clean_title;
    //TODO: wrap this
    if (section_ul) {
        lxb_dom_node_insert_child(
                lxb_dom_interface_node(section_ul), lxb_dom_interface_node(bm_entry)
            );
    } else if(create_section_if_not_found) {
        err = bookmark_section_insert(&(bm.lxbdoc->dom_document), body, line, bm_entry);
    } else err = "section not found in bookmarks file";

    ok_then(err, bookmarks_save_to_disc(&bm, &bmfile));

clean_title:
    str_clean(title);
free_curl_url:
    curl_free(url);
clean_bmfile_and_bmdoc:
    str_clean(&bmfile);
    htmldoc_cleanup(&bm);
    return err;
}
