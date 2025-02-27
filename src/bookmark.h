#ifndef AHRE_BOOKMARK_H__
#define AHRE_BOOKMARK_H__

#include "src/utils.h"
#include "src/htmldoc.h"
#include "src/error.h"

Err bookmark_to_source(HtmlDoc bookmark[static 1], BufOf(char)* out) {
    (void)out;
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(bookmark);
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    //Err err = browse_rec(lxb_dom_interface_node(lxbdoc), &ctx);
    if (node->type != LXB_DOM_NODE_TYPE_DOCUMENT) return "error: invalid bookmark file";

    return Ok;
}
#endif
