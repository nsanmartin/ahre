#ifndef __CURL_AHRE_H__
#define __CURL_AHRE_H__

#include <curl/curl.h>

#include "utils.h"
#include "error.h"
#include "writer.h"

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


static inline Err w_curl_url_set(CURLU* u,  CURLUPart part, const char* cstr, unsigned flags) {
    if (!cstr || !*cstr) return "error: no contents for CURLUPart";
    CURLUcode code = curl_url_set(u, part, cstr, flags);
    return code == CURLUE_OK 
        ? Ok : err_fmt("error setting url with '%s': %s", cstr, curl_url_strerror(code));
}


static inline Err w_curl_url_dup(CURLU* u, CURLU* dup[_1_]) {
    return (*dup = curl_url_dup(u)) ? Ok : "error: curl_url_dup failure";
}


static inline Err w_curl_url_get_malloc(CURLU* cu, CURLUPart part, char* out[_1_]) {
/*
 * Get cu's part. The caller should curl_free out.
 */
    CURLUcode code = curl_url_get(cu, part, out, 0);
    if (code == CURLUE_OK)
        return Ok;
    return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
}



#include "user-out.h"
void w_curl_multi_remove_handles(
    CURLM* multi, ArlOf(CurlPtr)  easies[_1_], Writer msg_writer[_1_]);

Err w_curl_multi_perform_poll(CURLM* multi);

Err for_htmldoc_size_download_append(
    ArlOf(CurlPtr) easies[_1_],
    Writer         msg_writer[_1_],
    CURL*          curl,
    uintmax_t      out[_1_]
);
Err curlinfo_sz_download_incr(
    Writer    msg_writer[_1_],
    CURL*     curl,
    uintmax_t nptr[_1_]
);
#endif
