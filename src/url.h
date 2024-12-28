#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <curl/curl.h>

#include "src/error.h"

typedef struct {
    CURLU* urlu;
} Url;

/* getters */
static inline CURLU* url_curlu(Url u[static 1]) { return u->urlu; }

static inline Err url_cstr(Url u[static 1], char* out[static 1]) {
    CURLUcode code = curl_url_get(u->urlu, CURLUPART_URL, out, 0);
    if (code == CURLUE_OK)
        return Ok;
    return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
}

/* ctor */
static inline Err url_init(Url u[static 1],  const char* cstr) {
    *u = (Url){.urlu=curl_url()};
    if (!u->urlu) return "error initializing CURLU";

    CURLUcode code = curl_url_set(u->urlu, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);
    switch (code) {
        case CURLUE_OK:
            return Ok;
        case CURLUE_BAD_HANDLE: case CURLUE_OUT_OF_MEMORY:
            return err_fmt("error setting CURLU: %s", curl_url_strerror(code));
        default:
            printf("warn: curl_url_set returned code: %s\n", curl_url_strerror(code));
            return Ok;
    }
}

//TODO make url_int call this one
static inline Err curlu_set_url(CURLU* u,  const char* cstr) {
    CURLUcode code = curl_url_set(u, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);
    switch (code) {
        case CURLUE_OK:
            return Ok;
        case CURLUE_BAD_HANDLE: case CURLUE_OUT_OF_MEMORY:
            return err_fmt("error setting CURLU: %s", curl_url_strerror(code));
        default:
            printf("warn: curl_url_set returned code: %s\n", curl_url_strerror(code));
            return Ok;
    }
}

/* dtor */
static inline void url_cleanup(Url u[static 1]) { curl_url_cleanup(u->urlu); }
#endif
