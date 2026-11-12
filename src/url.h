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
    if (!isnull(*u)) curl_url_cleanup(u->ptr);
    *u = (Url){0};
}
//



static inline Err url_fragment(Url u[_1_], char* out[_1_]) {
    CURLUcode code = curl_url_get(u->ptr, CURLUPART_FRAGMENT, out, 0);
    if (code == CURLUE_OK || code == CURLUE_NO_FRAGMENT)
        return Ok;
    return err_fmt("warn: getting url fragment from CURLU: %s", curl_url_strerror(code));
}




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
    if (from && !isnull(*from)) return url_dup(*from, u);
    *u = (Url){.ptr=curl_url()};
    if (!u->ptr) return "error initializing CURLU";
    return Ok;
}


Err url_set_url_or_fragment(Url u[_1_], const char* s);

Err url_cstr_malloc(Url u, char* out[_1_]);
Err url_append_host_to_str(Url u, char* out[_1_]);
Err url_append_path_to_str(Url u, char* out[_1_]);
Err url_is_https(Url u, bool is[1]);
Err curl_url_append(Url u, Str out[_1_]);
Err url_str_get_scheme(Url* from, StrView s, Str out[_1_]);
Err url_str_get_scheme_if_absent(StrView s, Str out[_1_]);
Err urlstr_to_cache_path_append(StrView urlstr, Str out[_1_]);

#endif
