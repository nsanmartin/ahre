#ifndef __CURL_AHRE_H__
#define __CURL_AHRE_H__

#include <curl/curl.h>

#include "utils.h"
#include "error.h"

size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc);

typedef CURL* CurlPtr;
#define T CurlPtr
#include <arl.h>

typedef CURLU* CurlUPtr;
static inline void curlu_ptr_clean(CurlUPtr* p) { curl_url_cleanup((void*)*p); }
#define T CurlUPtr
#define TClean curlu_ptr_clean
#include <arl.h>


static inline Err err_from_curle(CURLcode code) {
    return code == CURLE_OK
        ? Ok : err_fmt("error: could not set long CURLOPT: %s", curl_easy_strerror(code));
}


static inline Err err_from_curlm(CURLMcode code) {
    return code == CURLM_OK
        ? Ok : err_fmt("error: could not set long CURLOPT: %s", curl_multi_strerror(code));
}


static inline Err w_curl_easy_init(CURL* cup[static 1]) {
    *cup = curl_easy_init();
    return *cup ? Ok : "error: curl_esy_init failure";
}


#define w_curl_easy_setopt(Curl, Opt, Val) err_from_curle(curl_easy_setopt(Curl, Opt, Val))


static inline Err w_curl_multi_add_handle(CURLM* multi, CURL* easy) {
    CURLMcode code = curl_multi_add_handle(multi, easy);
    return CURLM_OK == code
        ? Ok : err_fmt("error: curl_multi_add_handle failure: %s", curl_multi_strerror(code));
}


static inline Err w_curl_url_set(CURLU* u,  CURLUPart part, const char* cstr, unsigned flags) {
    if (!cstr || !*cstr) return "error: no contents for CURLUPart";
    CURLUcode code = curl_url_set(u, part, cstr, flags);
    return code == CURLUE_OK 
        ? Ok : err_fmt("error setting url with '%s': %s", cstr, curl_url_strerror(code));
}


static inline Err w_curl_url_dup(CURLU* u, CURLU* dup[static 1]) {
    return (*dup = curl_url_dup(u)) ? Ok : "error: curl_url_dup failure";
}


static inline Err w_curl_url_get_malloc(CURLU* cu, CURLUPart part, char* out[static 1]) {
/*
 * Get cu's part. The caller should curl_free out.
 */
    CURLUcode code = curl_url_get(cu, part, out, 0);
    if (code == CURLUE_OK)
        return Ok;
    return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
}


Err w_curl_multi_add(
    CURLM*         multi,
    CURLU*         baseurl,
    const char*    urlstr,
    ArlOf(CurlPtr) easies[static 1],
    ArlOf(Str)     destlist[static 1],
    ArlOf(CurlUPtr) curlus[static 1]

);


#include "user-out.h"
void
w_curl_multi_remove_handles(CURLM* multi, ArlOf(CurlPtr)  easies[static 1], SessionWriteFn wfnc);

Err w_curl_multi_add_handles( 
    CURLM*           multi,
    CURLU*           curlu,
    ArlOf(Str)       urls[static 1],
    ArlOf(Str)       scripts[static 1],
    ArlOf(CurlPtr)*  easies,
    ArlOf(CurlUPtr)* curlus,
    SessionWriteFn   wfnc
);
Err w_curl_multi_perform_poll(CURLM* multi);
#endif
