#ifndef __URL_AHRE_H__
#define __URL_AHRE_H__

#include <unistd.h>
#include <limits.h>
#include <strings.h>

#include "wrapper-curl.h"
#include "dom.h"
#include "utils.h"
#include "config.h"
#include "error.h"
#include "url-client.h"


typedef struct Session Session;

typedef struct { CurlUrlPtr ptr; } Url;

Err curl_url_to_filename_append(Url u, Str out[_1_]);
Err fopen_or_append_fopen(const char* fname, Url u, FILE* fp[_1_], Str actual_path[_1_]);

static inline CurlUrlPtr url_cu(Url u[_1_]) { return u->ptr; }

/* dtor */
static inline void url_cleanup(Url u[_1_]) {
    if (u->ptr) curl_url_cleanup(u->ptr);
    u->ptr = NULL;
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
Err request_to_file(Request r[_1_], UrlClient url_client[_1_], FILE* fp);

static inline Err request_query_append_key_value(
    Request r[_1_], const char*k, size_t klen, const char* v, size_t vlen
) {
    //TODO1 manage errors properly
        Str* key = &(Str){0};
        Str* value = &(Str){0};
        try(str_append(key, sv(k, klen)));
        try(str_append(value, sv(v, vlen)));
        if (!arlfn(Str,append)(request_query_keys(r), key) 
                || !arlfn(Str,append)(request_query_values(r), value))
            return "error: arl append failure";
        return Ok;
}

Err url_from_get_request(Request r[_1_]);
Err url_from_request(Request r[_1_], UrlClient* uc);


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


/* req. */


static inline Err url_fragment(Url u[_1_], char* out[_1_]) {
    CURLUcode code = curl_url_get(u->ptr, CURLUPART_FRAGMENT, out, 0);
    if (code == CURLUE_OK || code == CURLUE_NO_FRAGMENT)
        return Ok;
    return err_fmt("error getting url fragment from CURLU: %s", curl_url_strerror(code));
}


Err get_url_alias(Session* s, const char* cstr, BufOf(char)* out);


/* ctor */

static inline Err url_dup(Url u, Url out[_1_]) {
/*
 * Duplicates in place. In case of failure it has no effect.
 */

    CURLU* dup = curl_url_dup(u.ptr);
    if (!dup) return "error: curl_url_dup failure";
    out->ptr = dup;
    return Ok;
}


static inline Err url_init(Url u[_1_], Url* from) {
    if (from) return url_dup(*from, u);
    *u = (Url){.ptr=curl_url()};
    if (!u->ptr) return "error initializing CURLU";
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


Err mk_submit_request (DomNode form, bool is_https, Request r[_1_]);
Err url_cstr_malloc(Url u, char* out[_1_]);

#endif
