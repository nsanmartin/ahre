#ifndef WRAPPERS_A_HREF_H_
#define WRAPPERS_A_HREF_H_

#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#define FAILED(msg) { perror(msg); exit(EXIT_FAILURE); }

//TODO: rename for CurlCtx
typedef struct {
    CURL* curl;
    char errbuf[CURL_ERROR_SIZE];
} CurlWithErrors;

static inline  CurlWithErrors curl_create(void) {
    CurlWithErrors rv = (CurlWithErrors){
        .curl=curl_easy_init(),
        .errbuf={0}
    };
    return rv;
}

int ahre_tidy(const char* url);
int ahre_lexbor(const char* url);

int curl_set_all_options(CURL* curl, const char* url, char* errbuf);
int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf);

size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out);
#endif

