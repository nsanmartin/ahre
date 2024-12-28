#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>

#include "src/utils.h"

typedef struct UrlClient {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} UrlClient;


/* ctor */
UrlClient* url_client_create(void);
/* dtor */
void url_client_destroy(UrlClient* url_client);


/* curl easy escape */
static inline char* url_client_escape_url(
    UrlClient url_client[static 1], const char* u, size_t len
) {
    return curl_easy_escape(url_client->curl, u, len);
}

static inline void url_client_curl_free_cstr(char* s) { curl_free(s); }
#endif
