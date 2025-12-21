#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>

#include "utils.h"


typedef struct UrlClient {
    CURL* curl;
    CURLM* curlm;
    char  errbuf[CURL_ERROR_SIZE];
    Str   postdata;
} UrlClient;

static inline CURL* url_client_curl(UrlClient url_client[_1_]) { return url_client->curl; }
static inline CURLM* url_client_multi(UrlClient url_client[_1_]) { return url_client->curlm; }

static inline Str* url_client_postdata(UrlClient uc[_1_]) { return &uc->postdata; }
/* ctor */
Err url_client_init(UrlClient url_client[_1_]);
/* dtor */


/* curl easy escape */
static inline char* url_client_escape_url(
    UrlClient url_client[_1_], const char* u, size_t len
) {
    return curl_easy_escape(url_client->curl, u, len);
}

static inline void url_client_cleanup(UrlClient* url_client) {
    curl_multi_cleanup(url_client->curlm);
    curl_easy_cleanup(url_client->curl);
    str_clean(url_client_postdata(url_client));
}

typedef struct Session Session;
static inline void url_client_curl_free_cstr(char* s) { curl_free(s); }
Err url_client_print_cookies(Session* s, UrlClient uc[_1_]) ;
Err url_client_reset(UrlClient url_client[_1_]);
Err url_client_set_basic_options(UrlClient url_client[_1_]);

#endif
