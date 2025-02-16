#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <unistd.h>
#include <limits.h>
#include <curl/curl.h>

#include "src/error.h"

typedef struct {
    CURLU* cu;
} Url;

/* getters */
static inline CURLU* url_cu(Url u[static 1]) { return u->cu; }

static inline Err url_cstr(Url u[static 1], char* out[static 1]) {
    CURLUcode code = curl_url_get(u->cu, CURLUPART_URL, out, 0);
    if (code == CURLUE_OK)
        return Ok;
    return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
}

static bool _file_exists_(const char* filename) {
    return !access(filename, F_OK);
}

static Err _prepend_file_schema_(const char* path, BufOf(char) buf[static 1]) {
    if (!*path) return "error: can't prepend schema to null path";
    try( bufofchar_append_lit__(buf, "file://"));
    if (*path != '/') {
        char cwdbuf[4000];
        if(!getcwd(cwdbuf, 4000)) return "error: gwtcwd failed";
        try( bufofchar_append(buf, cwdbuf, strlen(cwdbuf)));
        if (*path == '.' && path[1] != '/') {
            ++path;
        } else {
            try( bufofchar_append_lit__(buf, "/"));
        }
    }
    try( bufofchar_append(buf, (char*)path, strlen(path)));
    try( bufofchar_append(buf, "\0", 1));
    return Ok;
}

/* dtor */
static inline void url_cleanup(Url u[static 1]) { curl_url_cleanup(u->cu); }

/* ctor */
static inline Err url_init(Url u[static 1],  const char* cstr) {
    Err err;
    *u = (Url){.cu=curl_url()};
    if (!u->cu) return "error initializing CURLU";
    BufOf(char)* url_buf = &(BufOf(char)){0};
    if (_file_exists_(cstr)) {
        if((err=_prepend_file_schema_(cstr, url_buf))) {
            url_cleanup(u);
            return err;
        }
        cstr = url_buf->items;
    }

    CURLUcode code = curl_url_set(u->cu, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);
    Err res; 
    switch (code) {
        case CURLUE_OK:
            res = Ok;
            break;
        case CURLUE_BAD_HANDLE: case CURLUE_OUT_OF_MEMORY:
            res = err_fmt("error setting CURLU: %s", curl_url_strerror(code));
            url_cleanup(u);
            break;
        default:
            printf("(TODO: do something)warn: curl_url_set returned code: %s\n", curl_url_strerror(code));
            res = Ok;
            break;
    }
    buffn(char, clean)(url_buf);
    return res;
}

static inline Err url_init_from_curlu(Url u[static 1],  CURLU* cu) {
    u->cu = cu;
    return Ok;
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

#endif
