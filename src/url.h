#ifndef URL_AHRE_H__
#define URL_AHRE_H__

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



static inline Err url_fragment(Url u[_1_], char* out[_1_]) {
    CURLUcode code = curl_url_get(u->ptr, CURLUPART_FRAGMENT, out, 0);
    if (code == CURLUE_OK || code == CURLUE_NO_FRAGMENT)
        return Ok;
    return err_fmt("warn: getting url fragment from CURLU: %s", curl_url_strerror(code));
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


Err url_cstr_malloc(Url u, char* out[_1_]);

#endif
