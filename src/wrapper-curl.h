#ifndef __CURL_AHRE_H__
#define __CURL_AHRE_H__

#include <curl/curl.h>

#include "utils.h"
#include "error.h"
#include "writer.h"
#include "cmd-out.h"

size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc);

typedef CURL* CurlPtr;
typedef CURLU* CurlUrlPtr;
typedef CURLM* CurlMuliPtr;


#define T CurlPtr
#include <arl.h>

static inline void curlu_ptr_clean(CurlUrlPtr* p) { curl_url_cleanup((void*)*p); }
#define T CurlUrlPtr
#define TClean curlu_ptr_clean
#include <arl.h>


typedef curl_off_t w_curl_off_t;

static inline Err err_from_curle(CURLcode code) {
    return code == CURLE_OK
        ? Ok : err_fmt("error: could not set CURLOPT: %s", curl_easy_strerror(code));
}


static inline Err err_from_curlm(CURLMcode code) {
    return code == CURLM_OK
        ? Ok : err_fmt("error: could not set multi CURLOPT: %s", curl_multi_strerror(code));
}


static inline Err w_curl_easy_init(CURL* cup[_1_]) {
    *cup = curl_easy_init();
    return *cup ? Ok : "error: curl_esy_init failure";
}


#define w_curl_easy_setopt(Curl, Opt, Val) err_from_curle(curl_easy_setopt(Curl, Opt, Val))


static inline Err w_curl_multi_add_handle(CURLM* multi, CURL* easy) {
    CURLMcode code = curl_multi_add_handle(multi, easy);
    return CURLM_OK == code
        ? Ok : err_fmt("error: curl_multi_add_handle failure: %s", curl_multi_strerror(code));
}


Err w_curl_url_set(CURLU* u,  CURLUPart part, const char* cstr, unsigned flags);


static inline Err w_curl_url_dup(CURLU* u, CURLU* dup[_1_]) {
    return (*dup = curl_url_dup(u)) ? Ok : "error: curl_url_dup failure";
}



void w_curl_multi_remove_handles(CURLM* multi, ArlOf(CurlPtr)  easies[_1_], CmdOut cmd_out[_1_]);

Err w_curl_multi_perform_poll(CURLM* multi, ArlOf(CurlPtr) failed[_1_]);

Err for_htmldoc_size_download_append(
    ArlOf(CurlPtr) easies[_1_],
    CmdOut         cmd_out[_1_],
    CURL*          curl,
    uintmax_t      out[_1_]
);
Err curlinfo_sz_download_incr(
    CmdOut    cmd_out[_1_],
    CURL*     curl,
    uintmax_t nptr[_1_]
);


CURLcode w_curl_global_init();
void w_curl_global_cleanup();
void w_curl_free(void* p);
Err w_curl_url_get_malloc(CurlUrlPtr cu, CURLUPart part, char* out[_1_]);

#endif
