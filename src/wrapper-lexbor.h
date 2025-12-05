#ifndef __LXB_WRAPPER_AHRE_H__
#define __LXB_WRAPPER_AHRE_H__

#include <strings.h>
#include <lexbor/html/html.h>

#include "textbuf.h"

#define T lxb_char_t
#include <buf.h>


typedef struct { lxb_dom_node_t* n; } LxbNode;
static inline lxb_dom_node_t* lxbnode_node(LxbNode lbn[static 1]) { return lbn->n; }

static inline bool
lexbor_node_tag_is_select(LxbNode n[static 1]) { return n->n->local_name == LXB_TAG_SELECT; }
static inline bool
lexbor_node_tag_is_input(LxbNode n[static 1]) { return n->n->local_name == LXB_TAG_INPUT; }
static inline bool
lexbor_node_tag_is_text(LxbNode n[static 1]) { return n->n->local_name == LXB_TAG__TEXT; }

static inline bool _lexbor_element_find_attr_value_(
    lxb_dom_element_t* element,
    const char* attr_name,
    size_t attr_len,
    const lxb_char_t* out[static 1],
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
    lxb_dom_node_t*   node,
    const char*       attr_name,
    size_t            attr_len,
    const lxb_char_t* out[static 1],
    size_t*           len
) {
    return _lexbor_element_find_attr_value_(
        lxb_dom_interface_element(node), attr_name, attr_len, out, len
    );
}
#define lexbor_find_attr_value(X, AttrName, AttrLen, OutStr, OutLen) \
    _Generic((X), \
        lxb_dom_element_t*: _lexbor_element_find_attr_value_, \
        lxb_dom_node_t*   : _lexbor_node_find_attr_value_ \
    )(X, AttrName, AttrLen, OutStr, OutLen) 

static inline bool lexbor_elem_attr_has_value(
     lxb_dom_element_t* elem,
     const char*        attr,
     size_t             attrlen,
     const char*        expected_value,
     size_t             expected_valuelen
) {
    const lxb_char_t* value;
    size_t valuelen;
    return attr
        && lexbor_find_attr_value(elem, attr, attrlen, &value, &valuelen)
        && value
        && valuelen == expected_valuelen
        && !strncasecmp((const char*)value, expected_value, valuelen);
}
static inline bool lexbor_node_attr_has_value(
     lxb_dom_node_t* node,
     const char*     attr,
     size_t          attrlen,
     const char*     expected_value,
     size_t          expected_valuelen
) {
    return lexbor_elem_attr_has_value(
        lxb_dom_interface_element(node), attr, attrlen, expected_value, expected_valuelen
    );
}
static inline bool lxbnode_attr_has_value(
     LxbNode     node[static 1],
     const char* attr,
     size_t      attrlen,
     const char* expected_value,
     size_t      expected_valuelen
) {
    return lexbor_elem_attr_has_value(
        lxb_dom_interface_element(lxbnode_node(node)),
        attr,
        attrlen,
        expected_value,
        expected_valuelen
    );
}


#define lexbor_lit_attr_has_lit_value(X, Attr, Val) _Generic((X),\
    LxbNode*          : lxbnode_attr_has_value,\
    lxb_dom_element_t*: lexbor_elem_attr_has_value,\
    lxb_dom_node_t*   : lexbor_node_attr_has_value\
    )(X, Attr, lit_len__(Attr), Val, lit_len__(Val))


// static inline bool
// lxbnode_attr_has_value(LxbNode n[static 1], const char* attr, const char* value) {
//          return lexbor_node_attr_has_value(lxbnode_node(n), attr, value);
// }

Err lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf);

lxb_inline lxb_status_t append_to_buf_callback(const lxb_char_t *data, size_t len, void *bufptr) {
    BufOf(char)* buf = bufptr;
    if (buffn(char,append)(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    return LXB_STATUS_OK;
}


size_t lexbor_parse_chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);

static inline StrView
lexbor_element_get_attr_value(lxb_dom_element_t* elem, const char* attr_name, size_t attr_len) {
    StrView res = (StrView){0};
    if (attr_name && attr_len) {
        size_t len;
        res.items = (const char*)lxb_dom_element_get_attribute(
            elem, (const lxb_char_t*)attr_name, attr_len, &len
        );
        res.len   = len;
    }
    return res;
}

static inline StrView lexbor_node_get_attr_value(
    lxb_dom_node_t*   node,
    const char* attr_name,
    size_t attr_len
) {
    return lexbor_element_get_attr_value(lxb_dom_interface_element(node), attr_name, attr_len);
}

#define lexbor_get_lit_attr__(NodeOrElem, Lit) _Generic((NodeOrElem),\
    lxb_dom_element_t*: lexbor_element_get_attr_value,\
    lxb_dom_node_t*   : lexbor_node_get_attr_value\
    )(NodeOrElem, Lit, lit_len__(Lit))


#define lexbor_get_attr(NodeOrElem, Attr, AttrLen) _Generic((NodeOrElem),\
    lxb_dom_element_t*: lexbor_element_get_attr_value,\
    lxb_dom_node_t*   : lexbor_node_get_attr_value\
    )(NodeOrElem, Attr, AttrLen)


static inline Err lexbor_elem_remove_attr(lxb_dom_element_t* elem, const char* attr, size_t len) {
    lxb_status_t status = lxb_dom_element_remove_attribute(elem, (lxb_char_t*)attr, len);
    return LXB_STATUS_OK == status ? Ok : "error: could not set element's attribte";
}

static inline Err lexbor_node_remove_attr(lxb_dom_node_t* node, const char* attr, size_t len) {
    return lexbor_elem_remove_attr(lxb_dom_interface_element(node), attr, len);
}

#define lexbor_remove_lit_attr__(NodeOrElem, Lit) _Generic((NodeOrElem),\
    lxb_dom_element_t*: lexbor_elem_remove_attr,\
    lxb_dom_node_t*   : lexbor_node_remove_attr\
    )(NodeOrElem, Lit, lit_len__(Lit))


static inline bool lexbor_elem_has_attr(lxb_dom_element_t* elem, const char* name, size_t len) {
    return lxb_dom_element_has_attribute(elem, (const lxb_char_t*)name, len);
}

static inline bool lexbor_node_has_attr(lxb_dom_node_t* node, const char* name, size_t len) {
    return lexbor_elem_has_attr(lxb_dom_interface_element(node), name, len);
}

#define lexbor_has_lit_attr__(X, AttrNameLit) \
    _Generic((X), \
        lxb_dom_element_t*: lexbor_elem_has_attr, \
        lxb_dom_node_t*   : lexbor_node_has_attr \
    )(X, AttrNameLit, lit_len__(AttrNameLit))



#define lexbor_find_lit_attr_value__(X, AttrName, OutPtr, LenPtr) \
    lexbor_find_attr_value(X, AttrName, lit_len__(AttrName), OutPtr, LenPtr)


static inline Err lexbor_elem_set_attr(
    lxb_dom_element_t* elem,
    const char*        attr,
    size_t             attrlen,
    const char*        value,
    size_t             valuelen
)  {
    if (!attr || !attrlen) return "error: attr is NULL or len is 0";
    lxb_dom_attr_t * after = lxb_dom_element_set_attribute(
        elem,
        (const lxb_char_t*)attr,
        attrlen,
        (const lxb_char_t*)value,
        valuelen
    );
    if (!after) return "error: value attr could not be set";
    return Ok;
}

static inline Err lexbor_node_set_attr(
    lxb_dom_node_t* node, const char* attr, size_t attrlen, const char* value, size_t valuelen
)  { return lexbor_elem_set_attr(lxb_dom_interface_element(node), attr, attrlen, value, valuelen); }

#define lexbor_set_lit_attr__(X, AttrName, Val, ValLen) \
    _Generic((X), \
        lxb_dom_element_t*: lexbor_elem_set_attr, \
        lxb_dom_node_t*   : lexbor_node_set_attr \
    )(X, AttrName, lit_len__(AttrName), Val, ValLen) 



lxb_dom_node_t* _find_parent_form(lxb_dom_node_t* node) ;

Err lexbor_node_to_str(lxb_dom_node_t* node, BufOf(char)* buf);

static inline bool lexbor_str_eq(const char* s, const lxb_char_t* lxb_str, size_t len) {
    return lxb_str && !strncasecmp(s, (const char*)lxb_str, len);
}

Err dbg_print_title(lxb_dom_node_t* title) ;
Err mk_submit_request (lxb_dom_node_t* form, bool is_https, Request req[static 1]);
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

#define lexbor_get_text lexbor_node_get_text //TODO: use _Generics and match node/elem

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

static inline bool lexbor_element_id_cstr_match(lxb_dom_element_t* element, const char* elem_id) {
    size_t value_len = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)"id", 2, &value_len
    );
    if (!value_len || !value) return false;
    return !memstr_cmp((const char*)value, value_len, elem_id);
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

Err lexbor_get_title_text(lxb_dom_node_t* title, Str out[static 1]);

static inline Err lexbor_append_null_terminated_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, Str s[static 1]
) {
    const lxb_char_t* data;
    size_t data_len;
    if (!lexbor_find_attr_value(node, attr, attr_len, &data, &data_len))
        return "lexbor node does not have attr";
    return null_terminated_str_from_mem((char*)data, data_len, s);
}
#endif
