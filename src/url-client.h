#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>

#include "src/utils.h"

typedef struct UrlClient {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} UrlClient;


UrlClient* url_client_create(void);
void url_client_destroy(UrlClient* url_client);

#endif
