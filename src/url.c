#include <errno.h>

#include <curl/curl.h>

#include "sys.h"
#include "dom.h"
#include "str.h"
#include "utils.h"
#include "url.h"
#include "session.h"


Err curl_url_to_filename_append(Url u, Str out[_1_]) {
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
    err = str_append_z(out, sv(fname, strlen(fname) + 1));
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

    *fp = fopen(items__(path), "wa");
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
    else *fp = fopen(actual_path->items, "wa");
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
