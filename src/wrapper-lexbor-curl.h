#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "htmldoc.h"
#include "wrapper-curl.h"
#include "wrapper-lexbor.h"

Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[_1_]
);

#endif
