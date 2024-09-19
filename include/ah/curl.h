#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>

#include <ah/mem.h>
#include <ah/utils.h>

typedef struct AhCurl {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} AhCurl;


AhCurl* AhCurlCreate(void);
void AhCurlFree(AhCurl* ac);

int AhCurlBufSummary(AhCurl ahcurl[static 1], BufOf(char)*buf) ;
#endif
