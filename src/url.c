#include <errno.h>
#include <sys/stat.h>

#include "url.h"


Err curl_url_to_filename_append(CURLU* cu, Str out[static 1]) {
    char* url = NULL;
    char* schema = NULL;
    try( curl_url_part_cstr(cu, CURLUPART_URL, &url));
    Err err = curl_url_part_cstr(cu, CURLUPART_SCHEME, &schema);
    if (err) {
        curl_free(url);
        return "error: could not get host from url";
    }
    char* fname = url;
    size_t schema_len = strlen(schema);
    if (!strncmp(fname, schema, strlen(schema))) fname += schema_len;
    if (!strncmp(fname, "://", 3)) fname += 3;
    replace_char_inplace(fname, '/', '_');
    err = str_append(out, fname, strlen(fname) + 1);
    curl_free(schema); curl_free(url);
    return err;
}

Err _append_fopen(const char* dirname, CURLU* cu, FILE* fp[static 1]) {
/*
 * Append cu modified to dirname and open it in write mode.
 */
    Err err = Ok;
    Str* path = &(Str){0};
    try( str_append(path, (char*)dirname, strlen(dirname)));
    char* last_char_p = str_at(path, len__(path)-1);
    if (!last_char_p) { str_clean(path); return "error: unexpected out of range index"; }
    if (*last_char_p != '/') ok_then(err, str_append_lit__(path, "/"));

    if ((err = curl_url_to_filename_append(cu, path)))
    { str_clean(path); return "error: append failure"; }
    if ((err = str_append_lit__(path, "\0"))) { str_clean(path); return "error: append failure"; }

    *fp = fopen(items__(path), "wa");
    str_clean(path);
    return err;
}

Err fopen_or_append_fopen(const char* fname, CURLU* cu, FILE* fp[static 1]) {
/*
 * If fname is the path of an existing dir, then modify cu to modcu and open fname/modcu.
 * Otherwise, open fname.
 */
    struct stat st;
    if (!fname || !*fname) return "cannot open empty file name";
    *fp = NULL; 
    if (stat(fname, &st) == 0 && S_ISDIR(st.st_mode)) try(_append_fopen(fname, cu, fp));
    else *fp = fopen(fname, "wa");
    if (!*fp) return err_fmt("could not open file '%s'", strerror(errno));
    return Ok;
}

