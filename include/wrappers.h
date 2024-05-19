#ifndef WRAPPERS_A_HREF_H_
#define WRAPPERS_A_HREF_H_

#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

#include <ah-doc.h>

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

int ah_tidy(const char* url);
//int ah_lexbor_doc(AhCtx ctx[static 1]);
int lexbor_fetch_document(lxb_html_document_t* document, const char* url);
int lexbor_print_a_href(lxb_html_document_t* document);

int curl_set_all_options(CURL* curl, const char* url, char* errbuf);
int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf);

size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out);
#endif

