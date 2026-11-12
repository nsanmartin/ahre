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

#include <errno.h>

#include <curl/curl.h>

#include "sys.h"
#include "dom.h"
#include "str.h"
#include "utils.h"
#include "url.h"
#include "session.h"


Err
curl_url_append(Url u, Str out[_1_]) {
    char* url    = NULL;
    try( w_curl_url_get_malloc(u.ptr, CURLUPART_URL, &url));
    Err err = str_append(out, sv(url, strlen(url)));
    w_curl_free(url);
    return err;
}


/* Generates the relative file name replacing slashes */
Err
curl_url_to_filename_append(Url u, Str out[_1_]) {
    char* url = NULL;
    char* schema = NULL;
    try( w_curl_url_get_malloc(u.ptr, CURLUPART_URL, &url));
    Err err = w_curl_url_get_malloc(u.ptr, CURLUPART_SCHEME, &schema);
    if (err) {
        w_curl_free(url);
        return "error: could not get host from url";
    }
    char* fname = url;
    size_t schema_len = strlen(schema);
    if (!strncmp(fname, schema, strlen(schema))) fname += schema_len;
    if (!strncmp(fname, "://", 3)) fname += 3;
    cstr_replace_char_inplace(fname, '/', '_');
    err = str_append(out, sv(fname, strlen(fname) + 1));
    w_curl_free(schema);
    w_curl_free(url);
    return err;
}

static Err _append_fopen(Str path[_1_], Url u, FILE* fp[_1_]) {
/*
 * Append cu modified to dirname and open it in write mode.
 */
    Err err = Ok;
    char* last_char_p = str_at(path, len__(path)-1);
    if (!last_char_p) { return "error: unexpected out of range index"; }
    if (*last_char_p != '/') ok_then(err, str_append(path, svl("/")));

    if ((err = curl_url_to_filename_append(u, path))) { return "error: append failure"; }

    *fp = fopen(items__(path), "a");
    return err;
}

Err fopen_or_append_fopen(const char* fname, Url u, FILE* fp[_1_], Str actual_path[_1_]) {
/*
 * If fname is the path of an existing dir, then modify cu to modcu and open fname/modcu.
 * Otherwise, open fname.
 */
    if (!fname || !*fname) return "cannot open empty file name";
    bool path_exists;
    Err err = Ok;
    *fp = NULL; 
    try( resolve_path(fname, &path_exists, actual_path));
    if (path_exists && path_is_dir(actual_path->items))
        tryjmp(err, Clean_Actual_Path,  _append_fopen(actual_path, u, fp));
    else *fp = fopen(actual_path->items, "a");
    if (!*fp) err = err_fmt("could not open file '%s': %s\n", actual_path->items, strerror(errno));
    else
        return Ok;

Clean_Actual_Path:
    str_clean(actual_path);
    return err;
}


Err
url_cstr_malloc(Url u, char* out[_1_]) {
    return w_curl_url_get_malloc(u.ptr, CURLUPART_URL, out);
}


Err
url_append_host_to_str(Url u, char* out[_1_]) {
    return w_curl_url_get_malloc(u.ptr, CURLUPART_HOST, out);
}


Err
url_append_path_to_str(Url u, char* out[_1_]) {
    return w_curl_url_get_malloc(u.ptr, CURLUPART_PATH, out);
}


static Err
curlu_set_url_or_fragment(CURLU* u,  const char* cstr) {
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


Err
url_set_url_or_fragment(Url u[_1_], const char* cstr) {
    return curlu_set_url_or_fragment(url_cu(u), cstr);
}

Err url_is_https(Url u, bool is[1]) { return w_curl_url_is_https(u.ptr, is); }

Err
url_str_get_scheme_if_absent(StrView s, Str out[_1_]) {
    Err err      = Ok;
    char* scheme = NULL;
    Url u        = (Url){.ptr=curl_url()};
    if (isnull(u)) fail_e("out of memory calling curl_url");
    
    CURLUcode code = curl_url_set(u.ptr, CURLUPART_URL, s.items, 0);
    switch (code) {
        case CURLUE_NO_SCHEME: break;
        case CURLUE_BAD_SCHEME: break;
        case CURLUE_OK: goto Clean;
        default:
            err = err_fmt("error setting CURLU with '%s': %s", s.items, curl_url_strerror(code));
            goto Clean;
    }
    
    code = curl_url_set(u.ptr, CURLUPART_URL, s.items, CURLU_DEFAULT_SCHEME);
    switch (code) {
        case CURLUE_OK: break;
        default:
            err = err_fmt("error setting CURLU with '%s': %s", s.items, curl_url_strerror(code));
            goto Clean;
    }

    code = w_curl_get_part(u.ptr, CURLUPART_SCHEME, &scheme, CURLU_DEFAULT_SCHEME);
    switch (code) {
        case CURLUE_OK: tryjmp( err,Clean, str_append(out, sv(scheme)));
            break;
        default:
            err = err_fmt("warn: getting scheme from CURLU: %s", curl_url_strerror(code));
            break;
    }


Clean:
    if (scheme) w_curl_free(scheme);
    url_cleanup(&u);
    return err;
}

Err
urlstr_to_cache_path_append(StrView urlstr, Str out[_1_]) {
    Err err    = Ok;
    Str schema = (Str){0};
    try( url_str_get_scheme_if_absent(urlstr, &schema));
    if (schema.len) {
        tryjmp(err,Clean, str_append(out, sv(schema)));
        tryjmp(err,Clean, str_append(out, svl(":/")));
    }
    tryjmp(err,Clean, urlstr_to_path_append(urlstr, out));
Clean:
    str_clean(&schema);
    return err;
}

