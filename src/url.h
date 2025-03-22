#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <unistd.h>
#include <limits.h>
#include <curl/curl.h>

#include "utils.h"
#include "config.h"
#include "error.h"

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

static Err _prepend_file_schema_(const char* path, BufOf(char) buf[static 1]) {
    if (!*path) return "error: can't prepend schema to null path";
    try( bufofchar_append_lit__(buf, "file://"));
    if (*path != '/') {
        char cwdbuf[4000];
        if(!getcwd(cwdbuf, 4000)) return "error: gwtcwd failed";
        try( bufofchar_append(buf, cwdbuf, strlen(cwdbuf)));
        if (*path == '.' && path[1] == '/') {
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

static inline Err get_url_alias(const char* cstr, BufOf(char)* out) {
    if (cstr_starts_with("bookmarks", cstr)) {
        return get_bookmark_filename_if_it_exists(out);
    }
    return err_fmt("not a url alias: %s", cstr);
}

/* ctor */
static inline Err url_init(Url u[static 1],  const char* cstr) {
    if (!*(cstr=cstr_skip_space(cstr))) return "error: expecting an url";
    Err err;
    *u = (Url){.cu=curl_url()};
    if (!u->cu) return "error initializing CURLU";
    BufOf(char)* url_buf = &(BufOf(char)){0};
    if (file_exists(cstr)) {
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
        default:
            res = err_fmt("error setting url '%s', CURLU: %s", cstr, curl_url_strerror(code));
            url_cleanup(u);
            break;
    }
    buffn(char, clean)(url_buf);
    return res;
}

static inline Err url_curlu_dup(Url u[static 1], CURLU* out[static 1]) {
    *out = curl_url_dup(url_cu(u));
    if (!*out) return "error: curl_url_dup failure";
    return Ok;
}

static inline Err url_init_from_curlu(Url u[static 1],  CURLU* cu) {
    u->cu = cu;
    return Ok;
}

//TODO make url_int call this one
static inline Err curlu_set_url(CURLU* u,  const char* cstr) {
    if (!*cstr) return "error: no url";
    CURLUcode code = curl_url_set(u, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);
    switch (code) {
        case CURLUE_OK:
            return Ok;
        default:
            return err_fmt("error setting CURLU with '%s': %s", cstr, curl_url_strerror(code));
    }
}

static inline Err curlu_scheme_is_https(CURLU* cu, bool out[static 1]) {
    char *scheme;
    CURLUcode rc = curl_url_get(cu, CURLUPART_SCHEME, &scheme, 0);
    if(rc != CURLUE_OK)
        return "error: curl url get failure";

    *out = strcasecmp("https", scheme) == 0;
    curl_free(scheme);
    return Ok;
}

#endif
