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



/* external linkage */
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


Err cmd_bookmarks(CmdParams p[_1_]) {
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
                try( cmd_out_msg_append_str_ln(cmd_params_cmd_out(p), it));
            }
        }
        arlfn(BufOf(char),clean)(&list);
    } else {
        DomNode section;
        try( bookmark_section_get(body, url, &section, /*match_prefix*/false));

        if (!isnull(section)) {
            StrView data = dom_node_text_view(dom_node_first_child(section));
            if (data.len) {
                try(cmd_out_msg_append_str_ln(cmd_params_cmd_out(p), &data));
            }
        } else err = "invalid section in bookmark";
    }
    return err;
}

