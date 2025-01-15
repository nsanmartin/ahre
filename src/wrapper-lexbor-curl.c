#include "src/error.h"
#include "src/generic.h"
#include "src/htmldoc.h"
#include "src/url-client.h"
#include "src/wrapper-lexbor-curl.h"
#include "src/wrapper-lexbor.h"

#include "src/debug.h"

/* internal linkage */

void _print_fetch_info_(CURL* handle) {
    curl_off_t nbytes;
    CURLcode curl_code = curl_easy_getinfo(handle, CURLINFO_SIZE_DOWNLOAD_T, &nbytes);
    if (curl_code!=CURLE_OK) 
        printf("warn: %s", curl_easy_strerror(curl_code));
     else 
        printf("%"CURL_FORMAT_CURL_OFF_T"\n", nbytes);
}

Err _lexbor_parse_chunk_begin_(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) 
        return "error: lex failed to init html document";
    return Ok;
}

Err _lexbor_parse_chunk_end_(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    lexbor_status_t lxb_status = lxb_html_document_parse_chunk_end(lxbdoc);
    if (LXB_STATUS_OK != lxb_status) 
        return err_fmt("error: lbx failed to parse html, status: %d", lxb_status);
    return Ok;
}

CURLoption _curlopt_method_from_htmldoc_(HtmlDoc htmldoc[static 1]) {
    return htmldoc_method(htmldoc) == http_post 
               ? CURLOPT_POST
               : CURLOPT_HTTPGET
               ;
}

Err _curl_set_write_fn_and_data_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    if (  curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
       || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) {
        return "error configuring curl write fn/data";
    }
    return Ok;
}

Err _curl_set_http_method_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    CURLoption method = _curlopt_method_from_htmldoc_(htmldoc);
    if (curl_easy_setopt(url_client->curl, method, 1L)) 
        return "error: curl failed to set method";
    return Ok;
}

Err _curl_set_curlu_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    char* url_str = NULL;
    try( url_cstr(htmldoc_url(htmldoc), &url_str));
    if (curl_easy_setopt(url_client->curl, CURLOPT_URL, url_str)) 
        return "error: curl failed to set url";
    curl_free(url_str);
    return Ok;

    // TODO: once this fix 
    // https://github.com/curl/curl/commit/5ffc73c78ec48f6ddf38c68ae7e8ff41f54801e6 
    // is available replace this implementation
    //CURL* curlu = url_cu(htmldoc_url(htmldoc));
    //if (curl_easy_setopt(url_client->curl, CURLOPT_CURLU, curlu)) 
    //    return "error: curl failed to set url";
    //return Ok;
}

Err _curl_perform_error_( HtmlDoc htmldoc[static 1], CURLcode curl_code) {
    Url* url = htmldoc_url(htmldoc);
    char* u;
    Err e = url_cstr(url, &u);
    if (e) u = "and url could not be obtained due to another error :/";
    Err err = err_fmt(
        "curl failed to perform curl: %s (%s)", curl_easy_strerror(curl_code), u
    );
    if (!e) curl_free(u);
    return err;
}

static Err
_set_htmldoc_url_with_effective_url_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    char* effective_url = NULL;
    if (CURLE_OK != curl_easy_getinfo(url_client->curl, CURLINFO_EFFECTIVE_URL, &effective_url)) {
        return "error: couldn't get effective url from curl";
    }
    return curlu_set_url(url_cu(htmldoc_url(htmldoc)), effective_url);
}

Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    try( _curl_set_write_fn_and_data_(url_client, htmldoc));//TODO: move?
    try( _lexbor_parse_chunk_begin_(htmldoc));
    try( _curl_set_http_method_(url_client, htmldoc));
    try( _curl_set_curlu_(url_client, htmldoc));

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    buffn(const_char, reset)(url_client_postdata(url_client));
    if (curl_code!=CURLE_OK) 
        return _curl_perform_error_(htmldoc, curl_code);

    try( _lexbor_parse_chunk_end_(htmldoc));
    try(_set_htmldoc_url_with_effective_url_(url_client, htmldoc));
    _print_fetch_info_(url_client->curl);
    return Ok;
}


static Err _make_submit_get_curlu_rec(
    lxb_dom_node_t* node,
    BufOf(lxb_char_t) buf[static 1],
    CURLU* out
) {
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT && !_lexbor_attr_has_value(node, "type", "submit")) {

        const lxb_char_t* value;
        size_t valuelen;
        lexbor_find_attr_value(node, "value", &value, &valuelen);
        if (!value || valuelen == 0) return Ok;

        const lxb_char_t* name;
        size_t namelen;
        lexbor_find_attr_value(node, "name", &name, &namelen);
        if (!name || namelen == 0) return Ok;

        buffn(lxb_char_t, reset)(buf);
        if (  !buffn(lxb_char_t, append)(buf, (lxb_char_t*)name, namelen)
           || !buffn(lxb_char_t, append)(buf, (lxb_char_t*)"=", 1)
           || !buffn(lxb_char_t, append)(buf, (lxb_char_t*)value, valuelen)
           || !buffn(lxb_char_t, append)(buf, (lxb_char_t*)"\0", 1)
        ) return "error: could not BufOf.append";

        CURLUcode curl_code = curl_url_set(
            out, CURLUPART_QUERY, (const char*)buf->items, CURLU_APPENDQUERY | CURLU_URLENCODE
        );
        if (curl_code != CURLUE_OK) {
            return err_fmt("curl url failed to appen query: %s", curl_url_strerror(curl_code));
        }
    } 

    for(lxb_dom_node_t* it = node->first_child; ; it = it->next) {
        Err err = _make_submit_get_curlu_rec(it, buf, out);
        if (err) return err;
        if (it == node->last_child) { break; }
    }
    return Ok;
}

static Err _append_lexbor_name_value_attrs_if_both_(
    UrlClient url_client[static 1], lxb_dom_node_t node[static 1], BufOf(const_char) buf[static 1]
) {
    char* escaped;
    const lxb_char_t* value;
    size_t valuelen;
    lexbor_find_attr_value(node, "value", &value, &valuelen);
    if (!value || valuelen == 0) return Ok;

    const lxb_char_t* name;
    size_t namelen;
    lexbor_find_attr_value(node, "name", &name, &namelen);
    if (!name || namelen == 0) return Ok;

    escaped = url_client_escape_url(url_client, (char*)value, valuelen);
    if (!escaped) return "error: curl_escape failure";

    if (!buffn(const_char, append)(buf, "&", 1)) goto free_escaped;
    if (!buffn(const_char, append)(buf, (const char*)name, namelen)) goto free_escaped;
    if (!buffn(const_char, append)(buf, "=", 1)) goto free_escaped;
    if (!buffn(const_char, append)(buf, escaped, strlen(escaped))) goto free_escaped;
    url_client_curl_free_cstr(escaped);
    return Ok;
free_escaped:
    url_client_curl_free_cstr(escaped);
    return err_fmt("error: failure in %s", __func__);
}

static Err _make_submit_post_curlu_rec(
    UrlClient url_client[static 1],
    lxb_dom_node_t* node,
    BufOf(const_char) buf[static 1],
    CURLU* out
) {
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT && !_lexbor_attr_has_value(node, "type", "submit")) {
        try( _append_lexbor_name_value_attrs_if_both_(url_client, node, buf));
    } 

    for(lxb_dom_node_t* it = node->first_child; ; it = it->next) {
        try( _make_submit_post_curlu_rec(url_client, it, buf, out));
        if (it == node->last_child) { break; }
    }
    return Ok;
}

static Err _submit_url_set_action(
    BufOf(const_char)* buf,
    const lxb_char_t* action,
    size_t action_len,
    CURLU* out
) {
    if (  !buffn(const_char, append)(buf, (const char*)action, action_len)
       || !buffn(const_char, append)(buf, "\0", 1)) {
        return "error: memory failure (BufOf)";
    }

    CURLUcode curl_code = curl_url_set(out, CURLUPART_URL, (const char*)buf->items, 0);
    if (curl_code != CURLUE_OK) {
        return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }
    
    buffn(const_char, reset)(buf);
    return Ok;
}

//Err _get_form_method_(lxb_dom_node_t* form, HttpMethod out[static 1]) {
//    const lxb_char_t* method;
//    size_t len;
//    lexbor_find_attr_value(form, "action", &method, &len);
//    if (!method || !len || lexbor_str_eq("get", method, len)) {
//        *out = http_get;
//        return Ok;
//    }
//
//    if (method && len && lexbor_str_eq("post", method, len)) {
//        *out = http_post;
//        return Ok;
//    }
//    return "error: unsuported method";
//}

static Err _mk_submit_get_(lxb_dom_node_t* form, CURLU* out[static 1]) {
    Err err;
    BufOf(lxb_char_t)* buf = &(BufOf(lxb_char_t)){0};

    if ((err=_make_submit_get_curlu_rec(form, buf, *out))) {
        buffn(lxb_char_t, clean)(buf);
        return err;
    }

    buffn(lxb_char_t, clean)(buf);
    return Ok;
}

static Err
_mk_submit_post_(UrlClient url_client[static 1], lxb_dom_node_t* form, CURLU* out[static 1]) { 
    Err err;
    BufOf(const_char)* buf = url_client_postdata(url_client);
    buffn(const_char, reset)(buf);

    if ((err=_make_submit_post_curlu_rec(url_client, form, buf, *out))) {
        buffn(const_char, clean)(buf);
        return err;
    }

    if (!buffn(const_char, append)(buf, "\0", 1)) return "error: BufOf.append failure";
    //TODO: mode this somehow to `curl_lexbor_fetch_document`
    if (len__(buf) /*check why len des not work*/) {
        if (curl_easy_setopt(url_client->curl, CURLOPT_POSTFIELDS, buf->items+1)) {
            ///buffn(const_char, clean)(buf);
            return "error: curl postfields set failure";

        }
    }

    return Ok;
}

/* external linkage */

Err mk_submit_url (
    UrlClient url_client[static 1],
    lxb_dom_node_t* form,
    CURLU* out[static 1],
    HttpMethod doc_method[static 1] 
) {
    const lxb_char_t* action;
    size_t action_len;
    lexbor_find_attr_value(form, "action", &action, &action_len);

    const lxb_char_t* method;
    size_t method_len;
    lexbor_find_attr_value(form, "method", &method, &method_len);

    BufOf(const_char)* buf = &(BufOf(const_char)){0};
    if (action && action_len) {
        Err err = _submit_url_set_action(buf, action, action_len, *out);
        if (err) {
            buffn(const_char, clean)(buf);
            return err;
        }
    }
    buffn(const_char, clean)(buf);

    if (!method_len || lexbor_str_eq("get", method, method_len)) {
        *doc_method = http_get;
        return _mk_submit_get_(form, out);
    }
    if (!method_len || lexbor_str_eq("post", method, method_len)) {
        *doc_method = http_post;
        return _mk_submit_post_(url_client, form, out);
    }
    return "not yet supported method";
}

