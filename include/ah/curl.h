#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>
#include <ah/utils.h>

typedef struct AhCurl {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} AhCurl;

static inline  AhCurl curl_create(void) {
    AhCurl rv = (AhCurl){
        .curl=curl_easy_init(),
        .errbuf={0}
    };
    return rv;
}

int ahcurl_buffer_summary(AhCurl ahcurl[static 1], BufOf(char)*buf) ;
#endif
