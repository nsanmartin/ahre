#ifndef __LXB_WRAPPER_AHRE_H__
#define __LXB_WRAPPER_AHRE_H__

#include <strings.h>
#include <lexbor/html/html.h>

#include "textbuf.h"

#define T lxb_char_t
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
Err lexbor_set_attr_value( lxb_dom_node_t* node, const char* value, size_t valuelen);

lxb_dom_node_t* _find_parent_form(lxb_dom_node_t* node) ;

bool _lexbor_attr_has_value(
    lxb_dom_node_t node[static 1], const char* attr, const char* expected_value
) ;
Err lexbor_node_to_str(lxb_dom_node_t* node, BufOf(char)* buf);

static inline bool lexbor_str_eq(const char* s, const lxb_char_t* lxb_str, size_t len) {
    return lxb_str && !strncasecmp(s, (const char*)lxb_str, len);
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
    while (node->parent) {
        if (node->parent->local_name == tag) return true;
        node = node->parent;
    }
    return false;
}

static inline bool lexbor_inside_coloured_tag(lxb_dom_node_t* node) {
    while (node->parent) {
        if ( node->parent->local_name == LXB_TAG_IMG
          || node->parent->local_name == LXB_TAG_IMAGE
          || node->parent->local_name == LXB_TAG_INPUT
          || node->parent->local_name == LXB_TAG_B
          || node->parent->local_name == LXB_TAG_A
        )
            return true;
        node = node->parent;
    }
    return false;
}

static inline bool lexbor_tag_is_block(lxb_dom_node_t* node) {
    return node
        && (  node->local_name == LXB_TAG_ADDRESS
           || node->local_name == LXB_TAG_ARTICLE
           || node->local_name == LXB_TAG_ASIDE
           || node->local_name == LXB_TAG_BLOCKQUOTE
           || node->local_name == LXB_TAG_CANVAS
           || node->local_name == LXB_TAG_DD
           || node->local_name == LXB_TAG_DIV
           || node->local_name == LXB_TAG_DL
           || node->local_name == LXB_TAG_DT
           || node->local_name == LXB_TAG_FIELDSET
           || node->local_name == LXB_TAG_FIGCAPTION
           || node->local_name == LXB_TAG_FIGURE
           || node->local_name == LXB_TAG_FOOTER
           || node->local_name == LXB_TAG_FORM
           || node->local_name == LXB_TAG_H1
           || node->local_name == LXB_TAG_H2
           || node->local_name == LXB_TAG_H3
           || node->local_name == LXB_TAG_H4
           || node->local_name == LXB_TAG_H5
           || node->local_name == LXB_TAG_H6
           || node->local_name == LXB_TAG_HEADER
           || node->local_name == LXB_TAG_HR
           || node->local_name == LXB_TAG_LI
           || node->local_name == LXB_TAG_MAIN
           || node->local_name == LXB_TAG_NAV
           || node->local_name == LXB_TAG_NOSCRIPT
           || node->local_name == LXB_TAG_OL
           || node->local_name == LXB_TAG_P
           || node->local_name == LXB_TAG_PRE
           || node->local_name == LXB_TAG_SECTION
           || node->local_name == LXB_TAG_TABLE
           || node->local_name == LXB_TAG_TFOOT
           || node->local_name == LXB_TAG_UL
           || node->local_name == LXB_TAG_VIDEO
        );
}


Err lexbor_get_title_text_line(lxb_dom_node_t* node, BufOf(char)* out) ;

static inline void
_search_title_rec_(lxb_dom_node_t* node, lxb_dom_node_t* title[static 1]) {
    if (!node) return;
    else if (node->local_name == LXB_TAG_TITLE) *title = node; 
    else {
        for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
            _search_title_rec_(it, title);
            if (*title) break;
        }
    }
    return;
}

static inline Err
lexbor_get_title_node(lxb_html_document_t* lxbdoc, lxb_dom_node_t* title[static 1]) {
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    if (!node) return "error: no document";
    *title = NULL;
    _search_title_rec_(node, title);
    return Ok;
}

static inline bool lexbor_element_id_match(lxb_dom_element_t* element, const char* elem_id) {
    size_t value_len = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)"id", 2, &value_len
    );
    if (!value_len || !value) return false;
    return strcmp(elem_id, (char*)value) == 0;
}

Err lexbor_get_title_text(lxb_dom_node_t* title, Str out[static 1]);
#endif
