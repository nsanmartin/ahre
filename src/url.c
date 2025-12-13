#include <errno.h>
#include <sys/stat.h>

#include "str.h"
#include "url.h"
#include "session.h"


Err curl_url_to_filename_append(CURLU* cu, Str out[_1_]) {
    char* url = NULL;
    char* schema = NULL;
    try( w_curl_url_get_malloc(cu, CURLUPART_URL, &url));
    Err err = w_curl_url_get_malloc(cu, CURLUPART_SCHEME, &schema);
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

Err _append_fopen(const char* dirname, CURLU* cu, FILE* fp[_1_]) {
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

Err fopen_or_append_fopen(const char* fname, CURLU* cu, FILE* fp[_1_]) {
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

static Err _url_fill_postfields_(
    Request r[_1_],
    CURL* curl,
    const char* postfields[_1_],
    size_t len[_1_]
) {
    ArlOf(Str)* ks = request_query_keys(r);
    ArlOf(Str)* vs = request_query_values(r);

    if ((len__(ks) || len__(vs)) && len__(request_postfields(r)))
        return "error: request must have either postfiels or keys and values, not both";

    if (len__(request_postfields(r))) {
        *postfields = items__(request_postfields(r));
        *len        = len__(request_postfields(r));
        return Ok;
    }

    if (len__(ks) != len__(vs))
        return "error: key/value lists must have the same len";
    if (!len__(ks))
        return "submit request must have post fields";

    Str* kit = arlfn(Str,begin)(ks);
    Str* vit = arlfn(Str,begin)(vs);
    Err e = Ok;
    char* escaped = NULL;
    for ( ; kit != arlfn(Str,end)(ks) && vit != arlfn(Str,end)(vs) ; ++kit, ++vit) {

        escaped = curl_easy_escape(curl, items__(vit), len__(vit));
        if (!escaped) return "error: curl_escape failure";
        try_or_jump( e, Failure_Free_Escaped,
            str_append_lit__(request_postfields(r), "&"));
        try_or_jump(e, Failure_Free_Escaped,
            str_append(request_postfields(r), items__(kit), len__(kit)));
        try_or_jump(e, Failure_Free_Escaped,
            str_append_lit__(request_postfields(r), "="));
        try_or_jump(e, Failure_Free_Escaped,
            str_append(request_postfields(r), escaped, strlen(escaped)));
        curl_free(escaped);
    }
    //TODO: write postfields in htmldoc?
    try(str_append_lit__(request_postfields(r), "\0"));
    *postfields = items__(request_postfields(r)) + 1; /* ignore the first '&'! */
    *len        = len__(request_postfields(r)) - 2; /* ignore first '&' and '\0' ! */
    return Ok;
Failure_Free_Escaped:
    curl_free(escaped);
    return e;
}

static Err _url_from_post_request_(
    Url u[_1_], Request r[_1_], UrlClient uc[_1_], Url* other
) {
    try(url_init(u, other));
    CURL* curl = url_client_curl(uc);
    CURLU* cu = url_cu(u);
    if (len__(request_url_str(r))) {
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, items__(request_url_str(r)), 0);
        if (curl_code != CURLUE_OK)
            return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }
    
    const char* postfields;
    size_t      len;
    try (_url_fill_postfields_(r, uc, &postfields, &len));

    if (!len)
        return "error: unexpected empty postfields";

    CURLcode code;
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
    if (CURLE_OK != code) return "error: curl postfields size set failure";
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    if (CURLE_OK != code) return "error: curl postfields set failure";
    return Ok;
}

Err _prepend_file_schema_if_file_exists_(Str url[_1_], Str out[_1_]) {
    if (!len__(url)) return Ok;
    try(str_append_lit__(out, FILE_SCHEMA));

    const char* path = items__(url);
    if (path[0] != '/') {
         char cwdbuf[4000];
         if(!getcwd(cwdbuf, 4000)) { return "error: gwtcwd failed"; }
         try(str_append(out, cwdbuf, strlen(cwdbuf)));
         if (*path == '.' && path[1] == '/') ++path;
         else try(str_append_lit__(out, "/"));
    }
    try( str_append_z(out, (char*)path, strlen(path)));

    if (!file_exists(items__(out) + lit_len__(FILE_SCHEMA)))
        str_reset(out);
    return Ok;
}

Err url_from_get_request(Url u[_1_], Request r[_1_], Url* other) {
    try(url_init(u, other));
    CURLU* cu = url_cu(u);
    Err err = Ok;
    Str file = (Str){0};
    if (len__(request_url_str(r))) {
        try_or_jump(err, Failure_Clean_File_Url,
                _prepend_file_schema_if_file_exists_(request_url_str(r), &file));
        const char* url_str = file.len ? file.items : items__(request_url_str(r));
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, url_str, CURLU_DEFAULT_SCHEME);
        if (curl_code != CURLUE_OK) {
            err = err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
            goto Failure_Clean_File_Url;
        }
    }


    ArlOf(Str)* ks = request_query_keys(r);
    ArlOf(Str)* vs = request_query_values(r);
    if (len__(ks) != len__(vs)) {
        err = "error: key/value lists must have the same len";
        goto Failure_Clean_File_Url;
    }

    Str* kit = arlfn(Str,begin)(ks);
    Str* vit = arlfn(Str,begin)(vs);
    for ( ; kit != arlfn(Str,end)(ks) && vit != arlfn(Str,end)(vs) ; ++kit, ++vit) {
        str_reset(request_postfields(r));
        try_or_jump(err, Failure_Clean_File_Url, str_append_str(request_postfields(r), kit));
        try_or_jump(err, Failure_Clean_File_Url, str_append_lit__(request_postfields(r), "="));
        try_or_jump(err, Failure_Clean_File_Url, str_append_str(request_postfields(r), vit));
        try_or_jump(err, Failure_Clean_File_Url, str_append_lit__(request_postfields(r), "\0"));
        CURLUcode curl_code = curl_url_set(
            cu,
            CURLUPART_QUERY,
            items__(request_postfields(r)),
            CURLU_APPENDQUERY | CURLU_URLENCODE
        );
        if (curl_code != CURLUE_OK) {
            err = err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
            goto Failure_Clean_File_Url;
        }
    }
    str_reset(request_postfields(r));
    str_clean(&file);

    return err;
Failure_Clean_File_Url:
    str_clean(&file);
/* Failure_Clean_Curlu: */
    url_cleanup(u);
    return err;
}


Err url_from_request(Url u[_1_], Request r[_1_], UrlClient uc[_1_], Url* other) {
    switch (r->method) {
        case http_post: return _url_from_post_request_(u, r, uc, other);
        case http_get: return url_from_get_request(u, r, other);
        default: return "error: could not initialize htmldoc, unsupported http method";
    }
}


Err request_from_userln(Request r[_1_], const char* userln, HttpMethod method) {
    const char* url    = cstr_trim_space((char*)userln);
    char* params = (char*)cstr_next_space(url);
    if (params <= url) return "url has no length";
    size_t url_len     = params - url;
    size_t params_len  = 0;
    if (isspace(params[0])) {
        params[0] = '\0';
        ++params;
        params_len = strlen(params);
    }

    *r = (Request){
        .method=method,
        .url=(Str){.items=(char*)url, .len=url_len},
        .postfields=(Str){.items=(char*)params, .len=params_len}
    };
    return Ok;
}


Err get_url_alias(Session* s, const char* cstr, BufOf(char)* out) {
    if (cstr_starts_with("bookmarks", cstr)) {
        try(str_append_lit__(out, "file://"));
        return get_bookmark_filename_if_it_exists(items__(session_bookmarks_fname(s)), out);
    }
    return err_fmt("not a url alias: %s", cstr);
}

