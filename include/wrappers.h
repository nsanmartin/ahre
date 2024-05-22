#ifndef WRAPPERS_A_HREF_H_
#define WRAPPERS_A_HREF_H_

#include <stdio.h>
#include <tidy/tidy.h>
#include <tidy/tidybuffio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

//#include <ahdoc.h>

#define FAILED(msg) { perror(msg); exit(EXIT_FAILURE); }


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

int ah_tidy(const char* url);
int lexbor_print_a_href(lxb_html_document_t* document);
int lexbor_print_tag(const char* tag, lxb_html_document_t* document);

int curl_set_all_options(CURL* curl, const char* url, char* errbuf);
int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf);

size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out);

AhCurl* AhCurlCreate(void);
void AhCurlFree(AhCurl* ac);
size_t chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);

#endif

