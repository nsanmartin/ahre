#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "htmldoc.h"
#include "wrapper-curl.h"
#include "wrapper-lexbor.h"

Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[_1_]
);

Err request_to_file(Request r[_1_], UrlClient url_client[_1_], const char* fname);

Err curl_set_method_from_http_method(UrlClient url_client[_1_], HttpMethod m);
#endif
