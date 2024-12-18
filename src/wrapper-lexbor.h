#ifndef __LXB_WRAPPER_AHRE_H__
#define __LXB_WRAPPER_AHRE_H__

#include <lexbor/html/html.h>

#include "src/textbuf.h"


Err lexbor_href_write(
    lxb_html_document_t document[static 1],
    lxb_dom_collection_t** hrefs,
    TextBuf* textbuf
);
Err ahre_append_href(lxb_dom_element_t* element, void* buf) ;
Err lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf);
size_t lexbor_parse_chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);
Err lexbor_foreach_href(
    lxb_dom_collection_t collection[static 1],
    Err (*callback)(lxb_dom_element_t* element, void* textbuf),
    void* textbuf
);

lxb_inline lxb_status_t append_to_buf_callback(const lxb_char_t *data, size_t len, void *bufptr) {
    BufOf(char)* buf = bufptr;
    if (buffn(char,append)(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    return LXB_STATUS_OK;
}

void lexbor_find_attr_value(
    lxb_dom_node_t* node, const char* attr_name, const lxb_char_t* out[static 1], size_t* len
); 
#endif
