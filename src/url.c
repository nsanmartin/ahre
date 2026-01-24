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

static Err _append_fopen(const char* dirname, CURLU* cu, FILE* fp[_1_]) {
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

static Err _url_from_post_request_(Request r[_1_], UrlClient uc[_1_]) {
    Err err = Ok;
    try(url_init(request_url(r), r->urlview));
    CURL* curl = url_client_curl(uc);
    CURLU* cu = url_cu(&r->url);
    if (len__(request_urlstr(r))) {
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, items__(request_urlstr(r)), CURLU_DEFAULT_SCHEME);
        if (curl_code != CURLUE_OK) {
            err = err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
            goto Clean_Url;
        }
    }
    
    const char* postfields;
    size_t      len;
    try (_url_fill_postfields_(r, uc, &postfields, &len));

    if (!len) {
        err = "error: unexpected empty postfields";
        goto Clean_Url;
    }

    CURLcode code;
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);
    if (CURLE_OK != code) {
        err = "error: curl postfields size set failure";
        goto Clean_Url;
    }
    code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    if (CURLE_OK != code) {
        err = "error: curl postfields set failure";
        goto Clean_Url;
    }
    return Ok;
Clean_Url:
    url_cleanup(request_url(r));
    return err;
}

#define FILE_SCHEMA "file://"
static Err _prepend_file_schema_if_file_exists_(Str url[_1_], Str out[_1_]) {
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

Err url_from_get_request(Request r[_1_]) {
    try(url_init(&r->url, r->urlview));
    CURLU* cu = url_cu(&r->url);
    Err err = Ok;
    Str file = (Str){0};
    if (len__(request_urlstr(r))) {
        try_or_jump(err, Failure_Clean_File_Url,
                _prepend_file_schema_if_file_exists_(request_urlstr(r), &file));
        const char* url_str = file.len ? file.items : items__(request_urlstr(r));
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
    url_cleanup(request_url(r));
    return err;
}


Err url_from_request(Request r[_1_], UrlClient* uc) {
    switch (r->method) {
        case http_post:
            if (!uc) return "error: expetincg an UrlClient, got NULL";
            return _url_from_post_request_(r, uc);
        case http_get: return url_from_get_request(r);
        default: return "error: could not initialize htmldoc, unsupported http method";
    }
}

static Err
_make_submit_get_request_rec_( lxb_dom_node_t* node, Request r[_1_]) {
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT
            && !lexbor_lit_attr_has_lit_value(node, "type", "submit")) {

        const lxb_char_t* value;
        size_t valuelen;
        if (!lexbor_find_lit_attr_value__(node, "value", &value, &valuelen))
            return Ok;

        const lxb_char_t* name;
        size_t namelen;
        if (!lexbor_find_lit_attr_value__(node, "name", &name, &namelen))
            return Ok;

        if (strcasecmp("password", (char*)name) == 0)
            return "warn: passwords not allowed in get requests";

        try(request_query_append_key_value(r, (char*)name, namelen, (char*)value, valuelen));
    } 

    for(lxb_dom_node_t* it = node->first_child; ; it = it->next) {
        try( _make_submit_get_request_rec_(it, r));
        if (it == node->last_child) { break; }
    }
    return Ok;
}


static Err _request_append_select_(lxb_dom_node_t node[_1_], Request r[_1_]) {
    StrView key = lexbor_get_lit_attr__(node, "name");
    if (!key.len) return Ok;

    lxb_dom_node_t* selected = NULL;
    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
        if (it->local_name == LXB_TAG_OPTION && lexbor_has_lit_attr__(it, "selected")) {
            selected = it;
        }
    }
    if (selected) {
        StrView value = lexbor_get_lit_attr__(selected, "value");
        if (value.len) {
            try(request_query_append_key_value(r, key.items, key.len, value.items, value.len));
        }
    }
    return Ok;
}

static Err _request_append_lexbor_name_value_attrs_if_both_(
    lxb_dom_node_t node[_1_],
    bool is_https,
    Request r[_1_]
) {
    const lxb_char_t* value;
    size_t valuelen;
    if (!lexbor_find_lit_attr_value__(node, "value", &value, &valuelen))
        return Ok;

    const lxb_char_t* name;
    size_t namelen;
    if (!lexbor_find_lit_attr_value__(node, "name", &name, &namelen))
        return Ok;
    if (strcasecmp("password", (char*)name) == 0 && !is_https)
        return "warn: passwords allowed only under https";

    return request_query_append_key_value(r, (char*)name, namelen, (char*)value, valuelen);
}


static Err _make_submit_post_request_rec(
    lxb_dom_node_t* node,
    bool is_https,
    Request r[_1_]
) {
    if (!node) return Ok;
    if (node->local_name == LXB_TAG_FORM) {
       /* ignoring form nested inside another form */
       //TODO: receive a msg callback to notify user
       return Ok;
    }

    if (node->local_name == LXB_TAG_INPUT 
        && !lexbor_lit_attr_has_lit_value(node, "type", "submit"))
        return _request_append_lexbor_name_value_attrs_if_both_(node, is_https, r);
    else if (node->local_name == LXB_TAG_SELECT) {
        return _request_append_select_(node, r);
    }

    /* recursive case */
    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next)
        try( _make_submit_post_request_rec(it, is_https, r));
    return Ok;
}



static Err
_mk_submit_post_request_(lxb_dom_node_t* form, bool is_https, Request r[_1_]) { 

    for(lxb_dom_node_t* it = form->first_child; it ; it = it->next) {
        try(_make_submit_post_request_rec(it, is_https, r));
        if (it == form->last_child) break;
    }

    return Ok;
}


/* external linkage */

Err mk_submit_request (lxb_dom_node_t* form, bool is_https, Request r[_1_]) {
    const lxb_char_t* action;
    size_t action_len;
    lexbor_find_lit_attr_value__(form, "action", &action, &action_len);

    const lxb_char_t* method;
    size_t method_len;
    lexbor_find_lit_attr_value__(form, "method", &method, &method_len);

    if (action && action_len) 
        try(str_append_z(request_urlstr(r), (char*)action, action_len));


    if (!method_len || lexbor_str_eq("get", method, method_len)) {
        r->method = http_get;
        return _make_submit_get_request_rec_(form, r);
    }
    if (method_len && lexbor_str_eq("post", method, method_len)) {
        r->method = http_post;
        return _mk_submit_post_request_(form, is_https, r);
    }
    return "not yet supported method";
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

    *r = (Request){ .method=method };

    try(str_append_z(&r->urlstr, (char*)url, url_len));
    if (params_len) try(str_append_z(&r->postfields, (char*)params, params_len));
    return Ok;
}


Err get_url_alias(Session* s, const char* cstr, BufOf(char)* out) {
    if (cstr_starts_with("bookmarks", cstr)) {
        if (!len__(session_bookmarks_fname(s))) return "no bookmarks file configured";
        try(str_append_lit__(out, "file://"));
        return resolve_bookmarks_file(items__(session_bookmarks_fname(s)), out);
    }
    return err_fmt("not a url alias: %s", cstr);
}

Err request_to_file(Request r[_1_], UrlClient url_client[_1_], const char* fname) {
    /* try( url_client_set_basic_options(url_client)); */
    try( url_client_reset(url_client));/* why is this needed here while the htmldoc fetch does not? */
    try( curl_set_method_from_http_method(url_client, request_method(r)));
    try( curl_set_url(url_client, request_url(r)));

    CURLU* curlu = url_cu(request_url(r));
    FILE* fp;
    try(fopen_or_append_fopen(fname, curlu, &fp));
    if (!fp) return err_fmt("error opening file '%s': %s\n", fname, strerror(errno));
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, fwrite)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, fp)
    ) return "error configuring curl write fn/data";

    CURLcode curl_code = curl_easy_perform(url_client->curl);

    /* curl_easy_reset(url_client->curl); */
    if (curl_code!=CURLE_OK) 
        return err_fmt("curl failed to perform curl: %s", curl_easy_strerror(curl_code));
    /* try( url_client_reset(url_client)); */
    fclose(fp);
    return Ok;
}

