#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <unistd.h>
#include <limits.h>
#include <strings.h>
#include <curl/curl.h>
#include "wrapper-curl.h"
#include "utils.h"
#include "config.h"
#include "error.h"
#include "url-client.h"


typedef struct Session Session;

/** CURL wrappers */
Err curl_url_to_filename_append(CURLU* cu, Str out[_1_]);
Err fopen_or_append_fopen(const char* fname, CURLU* cu, FILE* fp[_1_]);
/* CURL wrappers **/

typedef struct {
    CURLU* cu;
} Url;

/* getters */
static inline CURLU* url_cu(Url u[_1_]) { return u->cu; }

/* dtor */
static inline void url_cleanup(Url u[_1_]) {
    if (u->cu) curl_url_cleanup(u->cu);
    u->cu = NULL;
}
//


typedef struct {
    HttpMethod method;
    Url*       urlview;
    Url        url;
    Str        urlstr;
    ArlOf(Str) keys;
    ArlOf(Str) values;
    Str        postfields; /* used for post field and quey */
} Request;


static inline HttpMethod request_method(Request r[_1_]) { return r->method; }
static inline Str* request_urlstr(Request r[_1_]) { return &r->urlstr; }
static inline Url* request_url(Request r[_1_]) { return &r->url; }
static inline Str* request_postfields(Request r[_1_]) { return &r->postfields; }
static inline ArlOf(Str)* request_query_keys(Request r[_1_]) { return &r->keys; }
static inline ArlOf(Str)* request_query_values(Request r[_1_]) { return &r->values; }
static inline void request_clean(Request r[_1_]) {
    str_clean(request_urlstr(r));
    str_clean(request_postfields(r));
    arlfn(Str,clean)(request_query_keys(r));
    arlfn(Str,clean)(request_query_values(r));
    url_cleanup(request_url(r));
}

Err request_from_userln(Request r[_1_], const char* userln, HttpMethod method);

static inline Err request_query_append_key_value(
    Request r[_1_], const char*k, size_t klen, const char* v, size_t vlen
) {
        Str* key = &(Str){0};
        Str* value = &(Str){0};
        try(str_append(key, (char*)k, klen));
        try(str_append(value, (char*)v, vlen));
        if (!arlfn(Str,append)(request_query_keys(r), key) 
                || !arlfn(Str,append)(request_query_values(r), value))
            return "error: arl append failure";
        return Ok;
}

Err url_from_get_request(Request r[_1_]);
Err url_from_request(Request r[_1_], UrlClient uc[_1_]);


static inline Err
request_init_move_urlstr(Request r[_1_], HttpMethod method, Str urlstr[_1_], Url* url) {
    (void)url;
    *r = (Request) {
        .method=method,
        .urlstr=*urlstr,
        .urlview=url
    };
    return Ok;
}

/* Err request_to_file(Request r[_1_], UrlClient url_client[_1_], const char* fname); */

/* req. */


static inline Err url_cstr_malloc(Url u[_1_], char* out[_1_]) {
    return w_curl_url_get_malloc(u->cu, CURLUPART_URL, out);
}

static inline Err url_fragment(Url u[_1_], char* out[_1_]) {
    CURLUcode code = curl_url_get(u->cu, CURLUPART_FRAGMENT, out, 0);
    if (code == CURLUE_OK || code == CURLUE_NO_FRAGMENT)
        return Ok;
    return err_fmt("error getting url fragment from CURLU: %s", curl_url_strerror(code));
}


Err get_url_alias(Session* s, const char* cstr, BufOf(char)* out);


/* ctor */

static inline Err url_dup(Url u[_1_], Url out[_1_]) {
/*
 * Duplicates in place. In case of failure it has no effect.
 */
    CURLU* dup = curl_url_dup(url_cu(u));
    if (!dup) return "error: curl_url_dup failure";
    out->cu = dup;
    return Ok;
}


static inline Err url_init(Url u[_1_], Url* from) {
    if (from) return url_dup(from, u);
    *u = (Url){.cu=curl_url()};
    if (!u->cu) return "error initializing CURLU";
    return Ok;
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

static inline Err curlu_scheme_is_https(CURLU* cu, bool out[_1_]) {
    char *scheme;
    CURLUcode rc = curl_url_get(cu, CURLUPART_SCHEME, &scheme, 0);
    if(rc != CURLUE_OK)
        return "error: curl url get failure";

    *out = strcasecmp("https", scheme) == 0;
    curl_free(scheme);
    return Ok;
}

#endif
