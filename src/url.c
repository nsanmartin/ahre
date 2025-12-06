#include <errno.h>
#include <sys/stat.h>

#include "str.h"
#include "url.h"


Err curl_url_to_filename_append(CURLU* cu, Str out[static 1]) {
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

static Err _url_fill_postfields_(
    Request r[static 1],
    CURL* curl,
    const char* postfields[static 1],
    size_t len[static 1]
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

static Err _url_from_post_request_(Request r[static 1], UrlClient uc[static 1], Url u[static 1]) {
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

static Err _url_from_get_request_(Request r[static 1], UrlClient uc[static 1], Url u[static 1]) {
    (void)r; (void)uc; (void)u;
    /* CURL* curl = url_client_curl(uc); */
    CURLU* cu = url_cu(u);
    if (len__(request_url_str(r))) {
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, items__(request_url_str(r)), 0);
        if (curl_code != CURLUE_OK)
            return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }


    ArlOf(Str)* ks = request_query_keys(r);
    ArlOf(Str)* vs = request_query_values(r);
    if (len__(ks) != len__(vs))
        return "error: key/value lists must have the same len";

    Str* kit = arlfn(Str,begin)(ks);
    Str* vit = arlfn(Str,begin)(vs);
    for ( ; kit != arlfn(Str,end)(ks) && vit != arlfn(Str,end)(vs) ; ++kit, ++vit) {
        str_reset(request_postfields(r));
        try(str_append_str(request_postfields(r), kit));
        try(str_append_lit__(request_postfields(r), "="));
        try(str_append_str(request_postfields(r), vit));
        try(str_append_lit__(request_postfields(r), "\0"));
        CURLUcode curl_code = curl_url_set(
            cu,
            CURLUPART_QUERY,
            items__(request_postfields(r)),
            CURLU_APPENDQUERY | CURLU_URLENCODE
        );
        if (curl_code != CURLUE_OK)
            return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }
    str_reset(request_postfields(r));

    return Ok;
}

Err url_from_request(Request r[static 1], UrlClient uc[static 1], Url u[static 1]) {
    switch (r->method) {
        case http_post: return _url_from_post_request_(r, uc, u);
        case http_get: return _url_from_get_request_(r, uc, u);
        default: return "error: could not initialize htmldoc, unsupported http method";
    }
}


Err request_from_userln(Request r[static 1], const char* userln, HttpMethod method) {
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
