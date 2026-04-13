#ifndef __LXB_CURL_AHRE_H__
#define __LXB_CURL_AHRE_H__

#include "wrapper-lexbor.h"
#include "wrapper-curl.h"

Err w_curl_set_url(UrlClient url_client[_1_], Url url[_1_]);

static inline CURLoption curlopt_method_from_http_method(HttpMethod m) {
    return m == http_post ? CURLOPT_POST : CURLOPT_HTTPGET ;
}


#endif 
