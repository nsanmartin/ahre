#include "htmldoc.h"
#include "draw.h" 
#include "bookmark.h"
#include "session.h"

/* internal linkage */


#define _htmldoc_fetch_bookmark_ htmldoc_fetch /* bookmark does not have js so it's simpler */
Err get_bookmarks_doc(
    UrlClient   url_client[_1_],
    StrView     bm_path,
    CmdOut      cmd_out[_1_],
    HtmlDoc     htmldoc_out[_1_]
) {
    Str* bm_url = &(Str){0};
    Err err = str_append_lit__(bm_url, "file://");
    try(err);
    try_or_jump(err, Clean_Bm_Url, str_append(bm_url, (char*)bm_path.items, bm_path.len));
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



/* external linkage */
Err bookmark_sections_body(HtmlDoc bookmark[_1_], lxb_dom_node_t* out[_1_]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(bookmark);
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    if (!node) return "error: no document";
    if (node->type != LXB_DOM_NODE_TYPE_DOCUMENT) return "not a bookmark file";
    lxb_dom_node_t* html = node->first_child;
    if (!html) return "not a bookmark file: expecting an html tag";
    if (html != node->last_child) return "not a bookmark file";
    if (html->local_name != LXB_TAG_HTML) return "not a bookmark file";
    if (!html->first_child) return "not a bookmark: expecting head";
    if (html->first_child->local_name != LXB_TAG_HEAD) return "not a bookmark file";
    if (!html->last_child) return "not a bookmark: expecting body";
    if (html->last_child->local_name != LXB_TAG_BODY) return "not a bookmark file";
    *out = html->last_child;
    return Ok;
}


Err cmd_bookmarks(CmdParams p[_1_]) {
    Session* session = p->s;
    const char* url = p->ln;
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));
    ArlOf(BufOf(char)) list = (ArlOf(BufOf(char))){0};
    lxb_dom_node_t* body;
    try(bookmark_sections_body(htmldoc, &body));
    Err err = Ok;
    if (!*url) {
        err = bookmark_sections(body, &list);
        if (!err) {
            BufOf(char)* it = arlfn(BufOf(char), begin)(&list);
            for (; it != arlfn(BufOf(char), end)(&list); ++it) {
                try( cmd_out_msg_append_str_ln(cmd_params_cmd_out(p), it));
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
                try(cmd_out_msg_append_ln(cmd_params_cmd_out(p), (char*)data, len));
            }
        } else err = "invalid section in bookmark";
    }
    return err;
}

