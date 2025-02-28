#ifndef AHRE_BOOKMARK_H__
#define AHRE_BOOKMARK_H__

#include "src/utils.h"
#include "src/htmldoc.h"
#include "src/error.h"

static inline Err bookmark_to_source(HtmlDoc bookmark[static 1], BufOf(char)* out) {
    (void)out;
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(bookmark);
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    //Err err = browse_rec(lxb_dom_interface_node(lxbdoc), &ctx);
    if (node->type != LXB_DOM_NODE_TYPE_DOCUMENT) return "error: invalid bookmark file";

    return Ok;
}

static inline Err
bookmark_sections_body(HtmlDoc bookmark[static 1], lxb_dom_node_t* out[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(bookmark);
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    if (node->type != LXB_DOM_NODE_TYPE_DOCUMENT) return "not a bookmark file";
    if (node->first_child != node->last_child) return "not a bookmark file";
    if (node->first_child -> local_name != LXB_TAG_HTML) return "not a bookmark file";
    lxb_dom_node_t* html = node->first_child;
    if (html->first_child->local_name != LXB_TAG_HEAD) return "not a bookmark file";
    if (html->last_child->local_name != LXB_TAG_BODY) return "not a bookmark file";
    *out = html->last_child;
    return Ok;
}

static inline Err bookmark_sections(lxb_dom_node_t* body, ArlOf(BufOf(char))* out) {
    for (lxb_dom_node_t* it = body->first_child; it != body->last_child; it = it->next) {
        if (it->local_name == LXB_TAG_H2) {
            if (it->first_child != it->last_child) return "invalid bookmark file";

            BufOf(char)* buf = arlfn(BufOf(char),append)(out, &(BufOf(char)){0});
            if (!buf) return "error: arl append failure";

            const char* data;
            size_t len;
            try( lexbor_node_get_text(it->first_child, &data, &len));
            if (len)
                if (!buffn(char, append)(buf, (char*)data, len)) return "error: buf append failure";
        }
    }

    return Ok;
}

static inline void bookmark_sections_sort(ArlOf(BufOf(char))* list) {
    qsort(list->items, list->len, sizeof(BufOf(char)), buf_of_char_cmp);
}

static inline Err
bookmark_section_get(lxb_dom_node_t* body, const char* q, lxb_dom_node_t* out[static 1]) {
    lxb_dom_node_t* res = NULL;
    for (lxb_dom_node_t* it = body->first_child; it != body->last_child; it = it->next) {
        if (it->local_name == LXB_TAG_H2) {
            if (it->first_child != it->last_child) return "invalid bookmark file";

            const char* data;
            size_t len;
            try( lexbor_node_get_text(it->first_child, &data, &len));
            if (len ) {
                size_t qlen = strlen(q);
                size_t min = qlen == len ? len : qlen;
                if (qlen <= min && strncmp(q, data, min) == 0) {
                    if (res) return "unequivocal reference to bookmark section";
                    res = it;
                }
            }
        }
    }
    if (!res) return "section not found in bookmark";
    *out = res;
    return Ok;
}

#endif
