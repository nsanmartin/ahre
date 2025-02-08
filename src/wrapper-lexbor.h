#ifndef __LXB_WRAPPER_AHRE_H__
#define __LXB_WRAPPER_AHRE_H__

#include <lexbor/html/html.h>

#include "src/textbuf.h"

#define BT lxb_char_t
#include <buf.h>

Err lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf);

lxb_inline lxb_status_t append_to_buf_callback(const lxb_char_t *data, size_t len, void *bufptr) {
    BufOf(char)* buf = bufptr;
    if (buffn(char,append)(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    return LXB_STATUS_OK;
}


size_t lexbor_parse_chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);

void lexbor_find_attr_value(
    lxb_dom_node_t* node, const char* attr_name, const lxb_char_t* out[static 1], size_t* len
); 
Err lexbor_set_attr_value( lxb_dom_node_t* node, const char* attr_name, const char* value);

lxb_dom_node_t* _find_parent_form(lxb_dom_node_t* node) ;

bool _lexbor_attr_has_value(
    lxb_dom_node_t node[static 1], const char* attr, const char* expected_value
) ;
Err lexbor_node_to_str(lxb_dom_node_t* node, BufOf(const_char)* buf);

static inline bool lexbor_str_eq(const char* s, const lxb_char_t* lxb_str, size_t len) {
    return lxb_str && !strncmp(s, (const char*)lxb_str, len);
}

Err dbg_print_title(lxb_dom_node_t* title) ;
Err mk_submit_url (
    UrlClient url_client[static 1],
    lxb_dom_node_t* form,
    CURLU* out[static 1],
    HttpMethod doc_method[static 1] 
);

static inline Err
lexbor_node_get_text(lxb_dom_node_t* node, const char* data[static 1], size_t len[static 1]) {
    lxb_dom_text_t* text = lxb_dom_interface_text(node);
    if(!text) return "error: expecting not null lxb_dom_interface_text(node)";
    *data = (const char*)text->char_data.data.data;
    *len = text->char_data.data.length;
    return Ok;
}

static inline bool lexbor_inside_tag(lxb_dom_node_t* node, lxb_tag_id_enum_t tag) {
    while (node) {
        if (node->parent->local_name == tag) return true;
        node = node->parent;
    }
    return false;
}
#endif
