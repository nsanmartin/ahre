#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "src/error.h"
#include "src/url-client.h"
#include "src/wrapper-lexbor.h"


Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], lxb_html_document_t lxbdoc[static 1], const char* url);
#endif


