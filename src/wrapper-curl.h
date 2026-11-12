/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

#ifndef __CURL_AHRE_H__
#define __CURL_AHRE_H__

#include <curl/curl.h>

#include "utils.h"
#include "error.h"
#include "writer.h"
#include "cmd-out.h"

size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc);

typedef CURL*    CurlPtr;
typedef CURLM*   CurlMuliPtr;
typedef CURLU*   CurlUrlPtr;
typedef CURLMsg* CurlMultiSgPtr;


static inline void curl_ptr_clean(CurlPtr* p) {
    if(*p) {
        curl_easy_cleanup((void*)*p);
        *p = (CURL*){0};
    }
}
#define Hotl_Arl_T CurlPtr
#define Hotl_Arl_T_Clean curl_ptr_clean
#include <arl.h>

#define Hotl_Arl_T CurlMultiSgPtr
#include <arl.h>

static inline void curlu_ptr_clean(CurlUrlPtr* p) { curl_url_cleanup((void*)*p); }
#define Hotl_Arl_T CurlUrlPtr
#define Hotl_Arl_T_Clean curlu_ptr_clean
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

Err w_curl_multi_perform_poll(CURLM* multi, ArlOf(CurlMultiSgPtr) failed[_1_]);

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

size_t skip_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) ;
Err w_curl_perform_with_cancel(CurlMuliPtr multi, CurlPtr easy, char* url);
Err w_curl_set_basic_options(
    CurlPtr handle[_1_], long verbose, char* errbuf, char* user_agent, char* cookiefile
);
Err w_curl_set_method_from_http_method(CurlPtr handle, HttpMethod m);
Err w_curl_set_write_fn_and_data_for_download(CurlPtr curl, FILE* fp);
Err w_curl_get_effective_url(CurlPtr curl, char* effective_url[_1_]);
Err w_curl_url_is_https(CURLU* cu, bool is[1]);
CURLUcode w_curl_get_part(CurlUrlPtr cu, CURLUPart part, char** content, unsigned flags);
#endif
