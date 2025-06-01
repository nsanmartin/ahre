#include <errno.h>
#include <sys/stat.h>

#include "url.h"


Err curl_url_to_filename_append(CURLU* cu, Str out[static 1]) {
    char* curl_host = NULL;
    char* curl_path = NULL;
    try( curl_url_part_cstr(cu, CURLUPART_HOST, &curl_host));
    Err err = curl_url_part_cstr(cu, CURLUPART_PATH, &curl_path);
    if (err) {
        curl_free(curl_host);
        return "error: could not get host from url";
    }
    size_t path_len = strlen(curl_path);
    if (path_len < 1 || curl_path[0] != '/') {
        /* from: https://curl.se/libcurl/c/curl_url_get.html
         * CURLUPART_PATH 
         * The part is always at least a slash ('/') even if no path was
         * supplied in the URL. A URL path always starts with a slash.
         * */
        curl_free(curl_host);
        curl_free(curl_path);
        return "error: bad part retrieved from url";
    };
    replace_char_inplace(curl_host, '/', '_');
    replace_char_inplace(curl_path, '/', '_');
    err = str_append(out, curl_host, strlen(curl_host));
    if (path_len > 1) ok_then(err, str_append(out, curl_path, strlen(curl_path)));
    curl_free(curl_host); curl_free(curl_path);
    return err;
}

Err _append_fopen(const char* dirname, CURLU* cu, FILE* fp[static 1]) {
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
    struct stat st;
    if (!fname || !*fname) return "cannot open empty file name";
    *fp = NULL; 
    if (stat(fname, &st) == 0 && S_ISDIR(st.st_mode)) try(_append_fopen(fname, cu, fp));
    else *fp = fopen(fname, "wa");
    if (!*fp) return err_fmt("could not open file '%s'", strerror(errno));
    return Ok;
}

