#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "htmldoc.h"

//Err mk_submit_url (lxb_dom_node_t* form, HtmlDoc d[static 1], CURLU* out[static 1]) ;
Err lexcurl_dup_curl_with_anchors_href(lxb_dom_node_t* anchor, CURLU* u[static 1]);
size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) ;
Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[static 1]
);
Err curl_save_url(UrlClient url_client[static 1], CURLU* curlu , const char* fname);
#endif


