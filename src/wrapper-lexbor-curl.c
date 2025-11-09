#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "url-client.h"
#include "str.h"
#include "wrapper-lexbor-curl.h"
#include "wrapper-lexbor.h"
#include "wrapper-curl.h"

#include "debug.h"


/* internal linkage */


Err _lexbor_parse_chunk_begin_(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) 
        return "error: lex failed to init html document";
    textbuf_reset(htmldoc_sourcebuf(htmldoc));
    return Ok;
}


Err _lexbor_parse_chunk_end_(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    lexbor_status_t lxb_status = lxb_html_document_parse_chunk_end(lxbdoc);
    if (LXB_STATUS_OK != lxb_status) 
        return err_fmt("error: lbx failed to parse html, status: %d", lxb_status);

    try (textbuf_append_part(htmldoc_sourcebuf(htmldoc), "\0", 1));
    return textbuf_append_line_indexes(htmldoc_sourcebuf(htmldoc));
}


CURLoption _curlopt_method_from_htmldoc_(HtmlDoc htmldoc[static 1]) {
    return htmldoc_method(htmldoc) == http_post 
               ? CURLOPT_POST
               : CURLOPT_HTTPGET
               ;
}


Err _curl_set_write_fn_and_data_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_HEADERDATA, htmldoc)
    || curl_easy_setopt(url_client->curl, CURLOPT_HEADERFUNCTION, curl_header_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
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


static Err _set_htmldoc_url_with_effective_url_(
    UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]
) {
    char* effective_url = NULL;
    if (CURLE_OK != curl_easy_getinfo(url_client->curl, CURLINFO_EFFECTIVE_URL, &effective_url)) {
        return "error: couldn't get effective url from curl";
    }
    return curlu_set_url_or_fragment(url_cu(htmldoc_url(htmldoc)), effective_url);
}


Err
_fetch_tag_script_from_text_(lxb_dom_node_t node[static 1], ArlOf(Str) out[static 1]) {

    const char* data;
    size_t len;
    try( lexbor_node_get_text(node, &data, &len));
    if (mem_skip_space_inplace(&data, &len)) {
        Str* s = &(Str){0};
        try( null_terminated_str_from_mem(data, len, s));
        if (!arlfn(Str,append)(out,s)) {
            str_clean(s);
            return "error: append failure";
        }
    }
    return Ok;
}


static Err _get_scripts_collection_(
    lxb_html_document_t     doc[static 1],
    lxb_dom_element_t       elem[static 1],
    lxb_dom_collection_t*   arr_ptr[static 1]
) {
    *arr_ptr = lxb_dom_collection_make(&doc->dom_document, 32);
    if (*arr_ptr == NULL)
        return "error: lexbor failed to create Collection object";
    if (LXB_STATUS_OK !=
        lxb_dom_elements_by_tag_name(elem, *arr_ptr, (const lxb_char_t *) "script", 6))
        return  "error: lexbor failed to get scripts";
    return Ok;
}


static Err _split_remote_local_(
    lxb_dom_collection_t* elems, 
    ArlOf(Str)            scripts[static 1],
    ArlOf(Str)            urls[static 1],
    SessionWriteFn        wfnc
) {
    for (size_t i = 0; i < lxb_dom_collection_length(elems); i++) {
        Err e = Ok;
        lxb_dom_element_t* element = lxb_dom_collection_element(elems, i);
        if (!element) return "error: lexbor collection failed to retrieve element";
        lxb_dom_node_t*   node = lxb_dom_interface_node(element);
        const lxb_char_t* src;
        size_t            src_len;

        if (lexbor_find_lit_attr_value__(node, "src", &src, &src_len)) {
            Str* url = &(Str){0};
            try(null_terminated_str_from_mem((const char*)src, src_len, url));
            arlfn(Str, append)(urls, url);
        } else {
            for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
                e = _fetch_tag_script_from_text_(it, scripts);
               if (e || it == node->last_child) break;
               else log_warn__(wfnc, "%s\n", "script elem with more than one child??");
               //Q? can script elem have mode that one (text) child?
            }
        }
        if (e) log_warn__(wfnc, "%s\n", e);
    }
    return Ok;
}



static void _map_append_nullchar_(ArlOf(Str) strlist[static 1], SessionWriteFn wfnc) {
    for ( Str* sp = arlfn(Str,begin)(strlist) ; sp != arlfn(Str,end)(strlist) ; ++sp) {
        Err e0 = str_append_lit__(sp, "\0");
        if (e0) {
            str_reset(sp);
            log_warn__(wfnc, "could not append \\0 to str: %s", e0);
        }
    }
}

Err curl_lexbor_fetch_scripts(
    HtmlDoc        htmldoc[static 1],
    UrlClient      url_client[static 1],
    SessionWriteFn wfnc
) {
    Err e = Ok;
    //TODO!: evaluate scripts in order

    lxb_html_document_t* doc = htmldoc_lxbdoc(htmldoc);
    lxb_dom_collection_t* head_scripts;
    lxb_dom_collection_t* body_scripts;

    try(_get_scripts_collection_(doc, lxb_dom_interface_element(doc->head), &head_scripts));
    try_or_jump(e, Lxb_Array_Head_Destroy,
            _get_scripts_collection_(doc, lxb_dom_interface_element(doc->body), &body_scripts));


    ArlOf(Str)* head_urls = &(ArlOf(Str)){0};
    ArlOf(Str)* body_urls = &(ArlOf(Str)){0};

    try_or_jump(e, Lxb_Array_Body_Destroy,
            _split_remote_local_(head_scripts, htmldoc_head_scripts(htmldoc), head_urls, wfnc));
    try_or_jump(e, Clean_Head_Urls,
            _split_remote_local_(body_scripts, htmldoc_body_scripts(htmldoc), body_urls, wfnc));

    ArlOf(CurlPtr)*  easies = &(ArlOf(CurlPtr)){0};
    ArlOf(CurlUPtr)* curlus = &(ArlOf(CurlUPtr)){0};
    CURLM*           multi  = url_client_multi(url_client);
    CURLU*           curlu  = url_cu(htmldoc_url(htmldoc));

    e = w_curl_multi_add_handles(
        multi, curlu, head_urls, htmldoc_head_scripts(htmldoc), easies, curlus, wfnc);
    if (e) log_warn__(wfnc, "could not add head handles: %s", e);
    e = w_curl_multi_add_handles(
        multi, curlu, body_urls, htmldoc_body_scripts(htmldoc), easies, curlus, wfnc);
    if (e) log_warn__(wfnc, "could not add body handles: %s", e);

    e = w_curl_multi_perform_poll(multi);

    w_curl_multi_remove_handles(multi, easies, wfnc);

    arlfn(CurlPtr,clean)(easies);
    arlfn(CurlUPtr,clean)(curlus);

    _map_append_nullchar_(htmldoc_head_scripts(htmldoc), wfnc);
    _map_append_nullchar_(htmldoc_body_scripts(htmldoc), wfnc);

    arlfn(Str,clean)(body_urls);
Clean_Head_Urls:
    arlfn(Str,clean)(head_urls);

Lxb_Array_Body_Destroy:
    lxb_dom_collection_destroy(body_scripts, true);
Lxb_Array_Head_Destroy:
    lxb_dom_collection_destroy(head_scripts, true);
    return e;
}


Err curl_lexbor_fetch_document(
    UrlClient      url_client[static 1],
    HtmlDoc        htmldoc[static 1],
    SessionWriteFn wfnc,
    CurlLxbFetchCb cb
) {
    try( url_client_set_basic_options(url_client));
    try( _curl_set_write_fn_and_data_(url_client, htmldoc));
    try( _lexbor_parse_chunk_begin_(htmldoc));
    try( _curl_set_http_method_(url_client, htmldoc));
    try( _curl_set_curlu_(url_client, htmldoc));

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    str_reset(url_client_postdata(url_client));
    if (curl_code!=CURLE_OK) 
        return _curl_perform_error_(htmldoc, curl_code);

    try( _lexbor_parse_chunk_end_(htmldoc));
    try( _set_htmldoc_url_with_effective_url_(url_client, htmldoc));
    try( htmldoc_convert_sourcebuf_to_utf8(htmldoc));
    if (cb) try( cb(wfnc, url_client->curl));
    if (htmldoc_js_is_enabled(htmldoc))
        try(curl_lexbor_fetch_scripts(htmldoc, url_client, wfnc));
    return Ok;
}


Err url_client_curlu_to_str(
    UrlClient url_client[static 1], CURLU* curlu , Str out[static 1]
) {
    Err e = Ok;
    /* try( url_client_reset(url_client)); */
    char* url_str = NULL;
    /* try( curl_url_part_cstr(curlu, CURLUPART_URL, &url_str)); */
    if (curl_easy_setopt(url_client->curl, CURLOPT_URL, url_str)) {
        e = "error: curl failed to set url";
        goto cleanup;
    }

    if (
       curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, str_append_flip)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, out)
    || curl_easy_setopt(url_client->curl, CURLOPT_CURLU, curlu)
    ) {
        e = "error configuring curl write fn/data";
        goto cleanup;
    }

    CURLcode curl_code = curl_easy_perform(url_client->curl);

    curl_easy_reset(url_client->curl);
    if (curl_code!=CURLE_OK)  {
        e = err_fmt("curl failed to perform curl: %s", curl_easy_strerror(curl_code));
        goto cleanup;
    }
    e = str_append_lit__(out, "\0");
    if (e) str_clean(out);

cleanup:
    curl_free(url_str);
    return e;
}


/*TODO: rename to `url_client_curlu_to_file`*/
Err curl_save_url(UrlClient url_client[static 1], CURLU* curlu , const char* fname) {
    try( url_client_reset(url_client));
    FILE* fp;
    try(fopen_or_append_fopen(fname, curlu, &fp));
    if (!fp) return err_fmt("error opening file '%s': %s\n", fname, strerror(errno));
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, fwrite)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, fp)
    || curl_easy_setopt(url_client->curl, CURLOPT_CURLU, curlu)
    ) return "error configuring curl write fn/data";

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    fclose(fp);

    curl_easy_reset(url_client->curl);
    if (curl_code!=CURLE_OK) 
        return err_fmt("curl failed to perform curl: %s", curl_easy_strerror(curl_code));
    try( url_client_reset(url_client));
    return Ok;
}



static Err
_make_submit_get_curlu_rec( lxb_dom_node_t* node, Str buf[static 1], CURLU* out) {
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT && !_lexbor_attr_has_value(node, "type", "submit")) {

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

        str_reset(buf);
        try(str_append(buf, (char*)name, namelen));
        try(str_append(buf, "=", 1));
        try(str_append(buf, (char*)value, valuelen));
        try(str_append(buf, "\0", 1));


        CURLUcode curl_code = curl_url_set(
            out, CURLUPART_QUERY, items__(buf), CURLU_APPENDQUERY | CURLU_URLENCODE
        );
        if (curl_code != CURLUE_OK) {
            return err_fmt("curl url failed to appen query: %s", curl_url_strerror(curl_code));
        }
    } 

    for(lxb_dom_node_t* it = node->first_child; ; it = it->next) {
        try( _make_submit_get_curlu_rec(it, buf, out));
        if (it == node->last_child) { break; }
    }
    return Ok;
}

static Err _append_lexbor_name_value_attrs_if_both_(
    UrlClient url_client[static 1],
    lxb_dom_node_t node[static 1],
    bool is_https,
    Str buf[static 1]
) {
    char* escaped;
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

    escaped = url_client_escape_url(url_client, (char*)value, valuelen);
    if (!escaped) return "error: curl_escape failure";

    Err e = str_append(buf, "&", 1);
    ok_then(e, str_append(buf, (char*)name, namelen));
    ok_then(e, str_append(buf, "=", 1));
    ok_then(e, str_append(buf, escaped, strlen(escaped)));

    url_client_curl_free_cstr(escaped);
    return e;
}

static Err _make_submit_post_curlu_rec(
    UrlClient url_client[static 1],
    lxb_dom_node_t* node,
    Str buf[static 1],
    CURLU* out,
    bool is_https
) {
    if (!node) return Ok;
    if (node->local_name == LXB_TAG_FORM) {
       log_warn__( output_stdout__, NULL, "%s", "ignoring form nested inside another form"); //TODO: use a configurable log fn
       return Ok;
    }
    if (node->local_name == LXB_TAG_INPUT && !_lexbor_attr_has_value(node, "type", "submit"))
        return _append_lexbor_name_value_attrs_if_both_(url_client, node, is_https, buf);

    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
        try( _make_submit_post_curlu_rec(url_client, it, buf, out, is_https));
        if (it == node->last_child) { break; }
    }
    return Ok;
}

static Err _submit_url_set_action(
    Str buf[static 1],
    const lxb_char_t* action,
    size_t action_len,
    CURLU* out
) {
    try(str_append(buf, (char*)action, action_len));
    try(str_append(buf, "\0", 1));

    CURLUcode curl_code = curl_url_set(out, CURLUPART_URL, (const char*)buf->items, 0);
    if (curl_code != CURLUE_OK) {
        return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }
    
    str_reset(buf);
    return Ok;
}

static Err _mk_submit_get_(lxb_dom_node_t* form, CURLU* out[static 1]) {
    Str* buf = &(Str){0};
    Err err =_make_submit_get_curlu_rec(form, buf, *out);
    str_clean(buf);
    return err;
}

static Err
_mk_submit_post_(UrlClient url_client[static 1], lxb_dom_node_t* form, CURLU* out[static 1]) { 
    Err err;
    Str* buf = url_client_postdata(url_client);
    str_reset(buf);
    bool is_https;
    try( curlu_scheme_is_https(*out, &is_https));

    for(lxb_dom_node_t* it = form->first_child; it ; it = it->next) {
        try_or_jump(
            err,
            Clean_Buf,
            _make_submit_post_curlu_rec(url_client, it, buf, *out, is_https)
        );
        if (it == form->last_child) break;
    }

    //TODO: move this somehow to `curl_lexbor_fetch_document`
    if (len__(buf)) {
    
        CURLcode code;
        code = curl_easy_setopt(url_client->curl, CURLOPT_POSTFIELDSIZE, len__(buf)-1);
        if (CURLE_OK != code) {
            err = "error: curl postfields size set failure";
            goto Clean_Buf;
        }
        code = curl_easy_setopt(url_client->curl, CURLOPT_POSTFIELDS, items__(buf)+1);
        if (CURLE_OK != code) {
            err = "error: curl postfields set failure";
            goto Clean_Buf;
        }
    } else { 
        err = "warn: no post fields, not submiting form";
        goto Clean_Buf;
    }

    return Ok;
Clean_Buf:
    str_clean(buf);
    return err;
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
    lexbor_find_lit_attr_value__(form, "action", &action, &action_len);

    const lxb_char_t* method;
    size_t method_len;
    lexbor_find_lit_attr_value__(form, "method", &method, &method_len);

    if (action && action_len) {
        Str* buf = &(Str){0};
        Err err = _submit_url_set_action(buf, action, action_len, *out);
        str_clean(buf);
        if (err) return err;
    }

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



Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[static 1]
)
{
    Err e = Ok;
    const lxb_char_t* data;
    size_t data_len;
    if (!lexbor_find_attr_value(node, attr, attr_len, &data, &data_len))
        return "lexbor node does not have attr";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    Str* buf = &(Str){0};
    try_or_jump(e, failure, null_terminated_str_from_mem((char*)data, data_len, buf));
    try_or_jump(e, failure, curlu_set_url_or_fragment(dup, buf->items));
    str_clean(buf);
    *u = dup;
    return Ok;
failure:
    str_clean(buf);
    curl_url_cleanup(dup);
    return e;
}


char* mem_whitespace(char* s, size_t len) {
    while(len && *s && !isspace(*s)) { ++s; --len; }
    return len ? s : NULL;
}
