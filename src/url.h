#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <unistd.h>
#include <limits.h>
#include <strings.h>
#include <curl/curl.h>

#include "utils.h"
#include "config.h"
#include "error.h"

/** CURL wrappers */
Err curl_url_to_filename_append(CURLU* cu, Str out[static 1]);
Err fopen_or_append_fopen(const char* fname, CURLU* cu, FILE* fp[static 1]);
/* CURL wrappers **/

typedef struct {
    CURLU* cu;
} Url;

typedef enum { curlu_tag, cstr_tag } CurluOrCstrTag;
typedef struct {
    CurluOrCstrTag tag;
    union {
        CURLU* curlu;
        const char* cstr;
    };
} CurluOrCstr;

//TODO: check type with _Generic
#define mk_union_curlu(CU) &(CurluOrCstr){.tag=curlu_tag,.curlu=CU}
#define mk_union_cstr(CS) &(CurluOrCstr){.tag=cstr_tag,.cstr=CS}

/* getters */
static inline CURLU* url_cu(Url u[static 1]) { return u->cu; }

static inline Err curl_url_part_cstr(CURLU* cu, CURLUPart part, char* out[static 1]) {
/*
 * Get cu's part. The caller should curl_free out.
 */
    CURLUcode code = curl_url_get(cu, part, out, 0);
    if (code == CURLUE_OK)
        return Ok;
    return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
}

static inline Err url_cstr(Url u[static 1], char* out[static 1]) {
    return curl_url_part_cstr(u->cu, CURLUPART_URL, out);
}

static inline Err url_fragment(Url u[static 1], char* out[static 1]) {
    CURLUcode code = curl_url_get(u->cu, CURLUPART_FRAGMENT, out, 0);
    if (code == CURLUE_OK || code == CURLUE_NO_FRAGMENT)
        return Ok;
    return err_fmt("error getting url fragment from CURLU: %s", curl_url_strerror(code));
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

static inline bool _file_exists_ignore_fragment_(const char* path) {
    char* numeral = strchr(path, '#'); 
    if (numeral) *numeral = '\0';
    bool res = file_exists(path);
    if (numeral) *numeral = '#';
    return res;
}


/* ctor */
static inline Err url_init_from_curlu(Url u[static 1],  CURLU* cu) {
    u->cu = cu;
    return Ok;
}

static inline Err url_init_from_cstr(Url u[static 1],  const char* cstr) {
    if (!*(cstr=cstr_skip_space(cstr))) return "error: expecting an url";
    Str* url_buf = &(Str){0};
    //TODO? why ignore fragment?
    if (_file_exists_ignore_fragment_(cstr)) {
        try(_prepend_file_schema_(cstr, url_buf));
        cstr = url_buf->items;
    }

    *u = (Url){.cu=curl_url()};
    if (!u->cu) return "error initializing CURLU";

    Err res = Ok;
    CURLUcode code = curl_url_set(u->cu, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);
    if (CURLUE_OK != code) {
        res = err_fmt("error setting url '%s', CURLU: %s", cstr, curl_url_strerror(code));
        url_cleanup(u);
    }
    str_clean(url_buf);
    return res;
}

static inline Err url_init(Url u[static 1], CurluOrCstr cu_or_cstr [static 1]) {
    switch (cu_or_cstr->tag) {
        case curlu_tag: return url_init_from_curlu(u, cu_or_cstr->curlu);
        case cstr_tag: return url_init_from_cstr(u, cu_or_cstr->cstr);
        default: return "error: invalid tag of CurluOrCstr";
    }
}

static inline Err url_dup(Url u[static 1], Url out[static 1]) {
/*
 * Duplicates in place. In case of failure it has no effect.
 */
    CURLU* dup = curl_url_dup(url_cu(u));
    if (!dup) return "error: curl_url_dup failure";
    out->cu = dup;
    return Ok;
}

static inline Err url_curlu_dup(Url u[static 1], CURLU* out[static 1]) {
    *out = curl_url_dup(url_cu(u));
    if (!*out) return "error: curl_url_dup failure";
    return Ok;
}

static inline Err curlu_set_url_path(CURLU* u,  const char* cstr) {
    if (!cstr || !*cstr) return "error: no url part";
    CURLUcode code = curl_url_set(u, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);

    switch (code) {
        case CURLUE_OK:
            return Ok;
        default:
            return err_fmt("error setting CURLU PATH with '%s': %s", cstr, curl_url_strerror(code));
    }
}

//TODO make url_int call this one
static inline Err curlu_set_url_or_fragment(CURLU* u,  const char* cstr) {
    if (!*cstr) return "error: no url";
    CURLUcode code = (*cstr == '#' && cstr[1])
        ? curl_url_set(u, CURLUPART_FRAGMENT, cstr + 1, CURLU_DEFAULT_SCHEME)
        : curl_url_set(u, CURLUPART_URL, cstr, CURLU_DEFAULT_SCHEME);

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
