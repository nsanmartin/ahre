#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "htmldoc.h"
#include "wrapper-lexbor.h"

size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) ;
Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[static 1]
);
Err curl_save_url(UrlClient url_client[static 1], CURLU* curlu , const char* fname);
Err url_client_curlu_to_str(UrlClient url_client[static 1], CURLU* curlu , Str out[static 1]);

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
