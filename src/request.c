#include "request.h"
#include "session.h"
#include "generic.h"


static Err _url_from_post_request_(Request r[_1_]);
Err url_from_get_request(Request r[_1_]) ;


static Err request_url_init(Request r[_1_]) {
    switch (r->method) {
        case http_post: return _url_from_post_request_(r);
        case http_get : return url_from_get_request(r);
        default: return err_internal("could not initialize htmldoc, unsupported http method");
    }
}

static Err
_make_submit_get_request_rec_(DomNode node, Request r[_1_]) {
    if (isnull(node)) return Ok;
    else if (dom_node_has_tag_input(node)
            && !dom_node_attr_has_value(node, svl("type"), svl("submit"))) {

        StrView value = dom_node_attr_value(node, svl("value"));
        if (!value.len) return Ok;

        StrView name = dom_node_attr_value(node, svl("name"));
        if (!name.len) return Ok;

        if (str_eq_case(name, svl("password")))
            return "warn: passwords not allowed in get requests";

        try(request_query_append_key_value(
            r, (char*)name.items, name.len, (char*)value.items, value.len)
        );
    } 

    for(DomNode it = dom_node_first_child(node); ; it = dom_node_next(it)) {
        try( _make_submit_get_request_rec_(it, r));
        if (dom_node_eq(it, dom_node_last_child(node))) { break; }
    }
    return Ok;
}


static Err _request_append_select_(DomNode node, Request r[_1_]) {
    StrView key = dom_node_attr_value(node, svl("name"));
    if (!key.len) return Ok;

    DomNode selected = (DomNode){0};
    for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_has_tag_option(it) && dom_node_has_attr(it, svl("selected"))) {
            selected = it;
        }
    }
    if (!isnull(selected)) {
        StrView value = dom_node_attr_value(selected, svl("value"));
        if (value.len) {
            try(request_query_append_key_value(r, key.items, key.len, value.items, value.len));
        }
    }
    return Ok;
}

static Err _request_append_lexbor_name_value_attrs_if_both_(
    DomNode node,
    bool    is_https,
    Request r[_1_]
) {

    StrView value = dom_node_attr_value(node, svl("value"));
    if (!value.len || !value.items) return Ok;

    StrView name = dom_node_attr_value(node, svl("name"));
    if (!name.len || !name.items) return Ok;

    if (str_eq_case(svl("password"), name) && !is_https)
        return "warn: passwords allowed only under https";

    return request_query_append_key_value(
        r, (char*)name.items, name.len, (char*)value.items, value.len
    );
}


static Err _make_submit_post_request_rec(
    DomNode node,
    bool is_https,
    Request r[_1_]
) {
    if (isnull(node)) return Ok;
    if (dom_node_has_tag_form(node)) {
       /* ignoring form nested inside another form */
       //TODO: receive a msg callback to notify user
       return Ok;
    }

    if (dom_node_has_tag_input(node)
        && !str_eq_case(svl("submit"), dom_node_attr_value(node, svl("type"))))
        return _request_append_lexbor_name_value_attrs_if_both_(node, is_https, r);
    else if (dom_node_has_tag_select(node)) {
        return _request_append_select_(node, r);
    }

    /* recursive case */
    for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it))
        try( _make_submit_post_request_rec(it, is_https, r));
    return Ok;
}



static Err
_mk_submit_post_request_(DomNode form, bool is_https, Request r[_1_]) { 

    for(DomNode it = dom_node_first_child(form); !isnull(it) ; it = dom_node_next(it)) {
        try(_make_submit_post_request_rec(it, is_https, r));
        if (dom_node_eq(it, dom_node_last_child(form))) break;
    }

    return Ok;
}


/* external linkage */


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

    try(str_append_z(&r->urlstr, sv(url, url_len))); 
    if (params_len) try(str_append_z(&r->postfields, sv(params, params_len)));
    return request_url_init(r);
}


Err get_url_alias(Session* s, const char* cstr, BufOf(char)* out) {
    if (cstr_starts_with("bookmarks", cstr)) {
        if (!len__(session_bookmarks_fname(s))) return "no bookmarks file configured";
        try(str_append(out, svl("file://")));
        return resolve_bookmarks_file(items__(session_bookmarks_fname(s)), out);
    }
    return err_fmt("not a url alias: %s", cstr);
}

Err request_to_file(Request r[_1_], UrlClient url_client[_1_], FILE* fp) {
    if (!fp) return "error: expectinf FILE* received NULL";
    try( url_client_set_basic_options(url_client));
    try( curl_set_method_from_http_method(url_client, request_method(r)));
    try( w_curl_set_url(url_client_curl(url_client), request_url(r)));

    if (
       curl_easy_setopt(url_client->curl, CURLOPT_HEADERDATA, NULL)
    || curl_easy_setopt(url_client->curl, CURLOPT_HEADERFUNCTION, skip_header_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, fwrite)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, fp)
    ) return "error configuring curl write fn/data";
    str_reset(url_client_postdata(url_client));

    return url_client_perform_with_cancel(url_client);
}

Err
request_to_handle(
    Request     r[_1_],
    UrlClient   url_client[_1_],
    const char* path,
    FilePtr     fpp[_1_],
    Str         actual_path[_1_],
    CurlPtr     out[_1_]
) {
    *out = curl_easy_init();
    if (!out) return err_internal("curl_easy_init failure");
    try(url_client_set_basic_options_to_handle(url_client, *out));
    try(w_curl_set_method_from_http_method(*out, request_method(r)));
    try( w_curl_set_url(*out, request_url(r)));

    try(fopen_or_append_fopen(path, *request_url(r), &fpp->ptr, actual_path));
    try(w_curl_set_write_fn_and_data_for_download(*out, fpp->ptr));
    return Ok;
}


Err request_from_form_node (Request r[_1_], DomNode form, bool is_https, Url* urlview) {
    *r = (Request){.urlview=urlview};
    StrView action = dom_node_attr_value(form, svl("action"));
    StrView method = dom_node_attr_value(form, svl("method"));

    if (action.items && action.len) 
        try(str_append_z(request_urlstr(r), &action));


    if (!method.len || str_eq_case(svl("get"), method)) {
        r->method = http_get;
        try( _make_submit_get_request_rec_(form, r));
    }
    if (method.len && str_eq_case(svl("post"), method)) {
        r->method = http_post;
        try( _mk_submit_post_request_(form, is_https, r));
    }

    return request_url_init(r);
}


static Err
_url_from_post_request_(Request r[_1_]) {
    Err err = Ok;
    try(url_init(request_url(r), r->urlview));
    CURLU* cu = url_cu(&r->url);
    if (len__(request_urlstr(r))) {
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, items__(request_urlstr(r)), CURLU_DEFAULT_SCHEME);
        if (curl_code != CURLUE_OK) {
            err = err_fmt("warn: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
            goto Clean_Url;
        }
    }
    
    return Ok;
Clean_Url:
    url_cleanup(request_url(r));
    return err;
}

#define FILE_SCHEMA "file://"
#define HTTP_SCHEMA "http://"
#define HTTPS_SCHEMA "https://"
static Err _prepend_file_schema_if_file_exists_(Str url[_1_], Str out[_1_]) {
    if (!len__(url)) return Ok;
    if (str_startswith(url, svl(FILE_SCHEMA))) return Ok;
    if (str_startswith(url, svl(HTTP_SCHEMA))) return Ok;
    if (str_startswith(url, svl(HTTPS_SCHEMA))) return Ok;
    try(str_append(out, svl(FILE_SCHEMA)));

    bool file_exists;
    try(resolve_path(items__(url), &file_exists,out));
    if (!file_exists) str_reset(out);

    return Ok;
}



Err url_from_get_request(Request r[_1_]) {
    Url u = (Url){0};
    try(url_init(&u, r->urlview));
    CURLU* cu = url_cu(&u);
    Err err = Ok;
    Str file = (Str){0};
    if (len__(request_urlstr(r)) && !str_eq_case(svl("/"),request_urlstr(r))) {
        tryjmp(err, Failure_Clean_File_Url,
                _prepend_file_schema_if_file_exists_(request_urlstr(r), &file));
        const char* url_str = file.len ? file.items : items__(request_urlstr(r));
        CURLUcode curl_code = curl_url_set(cu, CURLUPART_URL, url_str, CURLU_DEFAULT_SCHEME);
        if (curl_code != CURLUE_OK) {
            err = err_fmt("warn: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
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
        tryjmp(err, Failure_Clean_File_Url, str_append(request_postfields(r), kit));
        tryjmp(err, Failure_Clean_File_Url, str_append(request_postfields(r), svl("=")));
        tryjmp(err, Failure_Clean_File_Url, str_append(request_postfields(r), vit));
        tryjmp(err, Failure_Clean_File_Url, str_append(request_postfields(r), svl("\0")));
        CURLUcode curl_code = curl_url_set(
            cu,
            CURLUPART_QUERY,
            items__(request_postfields(r)),
            CURLU_APPENDQUERY | CURLU_URLENCODE
        );
        if (curl_code != CURLUE_OK) {
            err = err_fmt("warn: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
            goto Failure_Clean_File_Url;
        }
    }
    str_reset(request_postfields(r));
    str_clean(&file);
    r->url = u;

    return err;
Failure_Clean_File_Url:
    str_clean(&file);
    url_cleanup(&u);
    return err;
}



Err
request_init(Request r[_1_], HttpMethod method, StrView urlstr, Url* url) {
    *r = (Request) {
        .method=method,
        .urlview=url
    };
    if (urlstr.len) try(str_append_z(request_urlstr(r), urlstr));
    return request_url_init(r);
}

Err
request_from_cli_params(Request r[_1_], HttpMethod method, StrView urlstr, StrView postfields) {
    *r = (Request) { .method=method, };
    try(str_append_z(request_urlstr(r), urlstr));
    try(str_append(request_postfields(r), postfields));
    return request_url_init(r);
}


Err
request_query_append_key_value(Request r[_1_], const char*k, size_t klen, const char* v, size_t vlen) {
    Str* key   = NULL;
    Str* value = NULL;
    try(arl_append_zero(Str,request_query_keys(r),key));
    try(arl_append_zero(Str,request_query_values(r),value));
    try(str_append(key, sv(k, klen)));
    try(str_append(value, sv(v, vlen)));
    return Ok;
}
