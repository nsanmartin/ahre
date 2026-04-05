#ifndef __LXB_WRAPPER_AHRE_H__
#define __LXB_WRAPPER_AHRE_H__

#include <strings.h>
#include <lexbor/html/html.h>

typedef lxb_html_document_t* DomPtr;
typedef lxb_dom_node_t*      DomNodePtr;
typedef lxb_dom_element_t*   DomElemPtr;


#include "textbuf.h"


bool lexbor_tag_is_valid(lxb_tag_id_enum_t tag);

static inline bool _lexbor_element_find_attr_value_(
    lxb_dom_element_t* element,
    const char* attr_name,
    size_t attr_len,
    const lxb_char_t* out[_1_],
    size_t* len
) 
{
    if (attr_name && attr_len) {
        *out = lxb_dom_element_get_attribute(element, (const lxb_char_t*)attr_name, attr_len, len);
        if (*out && *len) return true;
    }
    return false;

}

static inline bool _lexbor_node_find_attr_value_(
    lxb_dom_node_t* node,
    const char*     attr_name,
    size_t          attr_len,
    const char*     out[_1_],
    size_t*         len
) {
    return _lexbor_element_find_attr_value_(
        lxb_dom_interface_element(node), attr_name, attr_len, (const lxb_char_t**)out, len
    );
}


size_t lexbor_parse_chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);


// static inline bool lexbor_inside_tag(lxb_dom_node_t* node, lxb_tag_id_enum_t tag) {
//     while (node->parent) {
//         if (node->parent->local_name == tag) return true;
//         node = node->parent;
//     }
//     return false;
//}

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



static inline bool
lexbor_element_id_mem_match(lxb_dom_element_t* element, const char* id, size_t len) {
    size_t value_len = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)"id", 2, &value_len
    );
    if (!value_len || !value) return false;
    if (value_len != len) return false;
    return !strncmp((const char*)value, id, value_len);
}

static inline bool lexbor_node_id_mem_match(lxb_dom_node_t* node, const char* id, size_t len) {
    return lexbor_element_id_mem_match(lxb_dom_interface_element(node), id, len);
}

static inline
lxb_dom_node_t* lexbor_node_get_node_by_id(lxb_dom_node_t* node, const char* id, size_t idlen) {

    if (node) {

        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT && lexbor_node_id_mem_match(node, id, idlen))
            return node;

        for (lxb_dom_node_t* it = node->first_child; it; it = it->next) {
            lxb_dom_node_t* rec = lexbor_node_get_node_by_id(it, id, idlen);
            if (rec) return rec;
        }

    }
    return NULL;
}

static inline lxb_dom_element_t*
lexbor_doc_get_element_by_id (lxb_html_document_t* lxbdoc, const char* id, size_t idlen) {
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    if (!lxbdoc) return NULL;
    node = lexbor_node_get_node_by_id(node, id, idlen);
    if (!node) return NULL;
    return lxb_dom_interface_element(node);
}


#endif
