#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "url-client.h"
#include "str.h"
#include "wrapper-lexbor-curl.h"
#include "wrapper-lexbor.h"
#include "wrapper-curl.h"



/* internal linkage */


Err _lexbor_parse_chunk_begin_(HtmlDoc htmldoc[_1_]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) 
        return "error: lex failed to init html document";
    textbuf_reset(htmldoc_sourcebuf(htmldoc));
    return Ok;
}


Err _lexbor_parse_chunk_end_(HtmlDoc htmldoc[_1_]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    lexbor_status_t lxb_status = lxb_html_document_parse_chunk_end(lxbdoc);
    if (LXB_STATUS_OK != lxb_status) 
        return err_fmt("error: lbx failed to parse html, status: %d", lxb_status);

    try (textbuf_append_part(htmldoc_sourcebuf(htmldoc), "\0", 1));
    return textbuf_append_line_indexes(htmldoc_sourcebuf(htmldoc));
}

CURLoption curlopt_method_from_http_method(HttpMethod m) {
    return m == http_post ? CURLOPT_POST : CURLOPT_HTTPGET ;
}


Err _curl_set_write_fn_and_data_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_HEADERDATA, htmldoc)
    || curl_easy_setopt(url_client->curl, CURLOPT_HEADERFUNCTION, curl_header_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) {
        return "error configuring curl write fn/data";
    }
    return Ok;
}

Err curl_set_method_from_http_method(UrlClient url_client[_1_], HttpMethod m) {
    CURLoption method = curlopt_method_from_http_method(m);
    if (curl_easy_setopt(url_client->curl, method, 1L)) 
        return "error: curl failed to set method";
    return Ok;
}


Err _curl_set_http_method_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
    return curl_set_method_from_http_method(url_client, htmldoc_method(htmldoc));
}


Err curl_set_url(UrlClient url_client[_1_], Url url[_1_]) {
    char* url_str = NULL;
    try( url_cstr_malloc(url, &url_str));
    CURLcode code = curl_easy_setopt(url_client->curl, CURLOPT_URL, url_str);
    curl_free(url_str);
    if (CURLE_OK == code) return Ok;
    return err_fmt("error: curl failed to set url %s", curl_easy_strerror(code));

    // TODO: once this fix 
    // https://github.com/curl/curl/commit/5ffc73c78ec48f6ddf38c68ae7e8ff41f54801e6 
    // is available replace this implementation
    /* CURL* curlu = url_cu(htmldoc_url(htmldoc)); */
    /* CURLcode code = curl_easy_setopt(url_client->curl, CURLOPT_CURLU, curlu); */
    /* if (CURLE_OK == code) return Ok; */
    /* return err_fmt("error: curl failed to set urlL %s", curl_easy_strerror(code)); */
}

Err _curl_set_curlu_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
    return curl_set_url(url_client, htmldoc_url(htmldoc));
}

Err _curl_perform_error_( HtmlDoc htmldoc[_1_], CURLcode curl_code) {
    Url* url = htmldoc_url(htmldoc);
    char* u;
    Err e = url_cstr_malloc(url, &u);
    if (e) u = "and url could not be obtained due to another error :/";
    Err err = err_fmt(
        "curl failed to perform curl: %s (%s)", curl_easy_strerror(curl_code), u
    );
    if (!e) curl_free(u);
    return err;
}


static Err _set_htmldoc_url_with_effective_url_(
    UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]
) {
    char* effective_url = NULL;
    if (CURLE_OK != curl_easy_getinfo(url_client->curl, CURLINFO_EFFECTIVE_URL, &effective_url)) {
        return "error: couldn't get effective url from curl";
    }
    return curlu_set_url_or_fragment(url_cu(htmldoc_url(htmldoc)), effective_url);
}


Err
_fetch_tag_script_from_text_(lxb_dom_node_t node[_1_], ArlOf(Str) out[_1_]) {

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
    lxb_html_document_t     doc[_1_],
    lxb_dom_element_t       elem[_1_],
    lxb_dom_collection_t*   arr_ptr[_1_]
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
    ArlOf(Str)            scripts[_1_],
    ArlOf(Str)            urls[_1_],
    Writer                msg_writer[_1_]
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
               else try(writer_write_lit__(msg_writer,  "script elem with more than one child??]n"));
               //Q? can script elem have mode that one (text) child?
            }
        }
        if (e) try(writer_write(msg_writer, (char*)e, strlen(e)));
    }
    return Ok;
}



static void _map_append_nullchar_(ArlOf(Str) strlist[_1_], Writer msg_writer[_1_]) {
    for ( Str* sp = arlfn(Str,begin)(strlist) ; sp != arlfn(Str,end)(strlist) ; ++sp) {
        Err e0 = str_append_lit__(sp, "\0");
        if (e0) {
            str_reset(sp);
            /*ignore e*/ writer_write_lit__(msg_writer, "could not append \\0 to str: ");
            /*ignore e*/ writer_write(msg_writer, (char*)e0, strlen(e0));
        }
    }
}

Err curl_lexbor_fetch_scripts(
    HtmlDoc        htmldoc[_1_],
    UrlClient      url_client[_1_],
    Writer         msg_writer[_1_]
) {
    Err e = Ok;
    //TODO!: evaluate scripts in order

    lxb_html_document_t* doc = htmldoc_lxbdoc(htmldoc);
    lxb_dom_collection_t* head_scripts;
    lxb_dom_collection_t* body_scripts;

    if (len__(htmldoc_head_scripts(htmldoc)) + len__(htmldoc_body_scripts(htmldoc)))
        return "error: htmldoc must have no scripts before fetching them";
    try(_get_scripts_collection_(doc, lxb_dom_interface_element(doc->head), &head_scripts));
    try_or_jump(e, Lxb_Array_Head_Destroy,
            _get_scripts_collection_(doc, lxb_dom_interface_element(doc->body), &body_scripts));


    ArlOf(Str)* head_urls = &(ArlOf(Str)){0};
    ArlOf(Str)* body_urls = &(ArlOf(Str)){0};

    try_or_jump(e, Lxb_Array_Body_Destroy,
            _split_remote_local_(head_scripts, htmldoc_head_scripts(htmldoc), head_urls, msg_writer));
    try_or_jump(e, Clean_Head_Urls,
            _split_remote_local_(body_scripts, htmldoc_body_scripts(htmldoc), body_urls, msg_writer));

    ArlOf(CurlPtr)*  easies = &(ArlOf(CurlPtr)){0};
    ArlOf(CurlUPtr)* curlus = &(ArlOf(CurlUPtr)){0};
    CURLM*           multi  = url_client_multi(url_client);
    CURLU*           curlu  = url_cu(htmldoc_url(htmldoc));

    e = url_client_multi_add_handles(
        url_client, curlu, head_urls, htmldoc_head_scripts(htmldoc), easies, curlus, msg_writer);
    if (e) {
        try(writer_write_lit__(msg_writer, "could not add head handles: "));
        try(writer_write(msg_writer, (char*)e, strlen(e)));
    }
    e = url_client_multi_add_handles(
        url_client, curlu, body_urls, htmldoc_body_scripts(htmldoc), easies, curlus, msg_writer);
    if (e) {
        try(writer_write_lit__(msg_writer, "could not add head handles: "));
        try(writer_write(msg_writer, (char*)e, strlen(e)));
    }

    e = w_curl_multi_perform_poll(multi);

    for_htmldoc_size_download_append(
        easies, msg_writer, url_client_curl(url_client), htmldoc_curlinfo_sz_download(htmldoc));
    w_curl_multi_remove_handles(multi, easies, msg_writer);

    arlfn(CurlPtr,clean)(easies);
    arlfn(CurlUPtr,clean)(curlus);

    _map_append_nullchar_(htmldoc_head_scripts(htmldoc), msg_writer);
    _map_append_nullchar_(htmldoc_body_scripts(htmldoc), msg_writer);

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
    UrlClient         url_client[_1_],
    HtmlDoc           htmldoc[_1_],
    Writer            msg_writer[_1_],
    FetchHistoryEntry histentry[_1_]
) {
    try( url_from_request(htmldoc_request(htmldoc), url_client));
    try( url_client_set_basic_options(url_client));
    try( _curl_set_write_fn_and_data_(url_client, htmldoc));
    try( _lexbor_parse_chunk_begin_(htmldoc));
    try( _curl_set_http_method_(url_client, htmldoc));
    try( _curl_set_curlu_(url_client, htmldoc));

    try( fetch_history_entry_init(histentry));
    CURLcode curl_code = curl_easy_perform(url_client->curl);
    fetch_history_entry_update_curl(histentry, url_client->curl, msg_writer);
    str_reset(url_client_postdata(url_client));
    if (curl_code!=CURLE_OK) 
        return _curl_perform_error_(htmldoc, curl_code);

    if (histentry->size_download_t < 0)
        try(writer_write_lit__(msg_writer, "CURLINFO_SIZE_DOWNLOAD_T is negative"));
    else *htmldoc_curlinfo_sz_download(htmldoc) = histentry->size_download_t;

    try( _lexbor_parse_chunk_end_(htmldoc));
    try( _set_htmldoc_url_with_effective_url_(url_client, htmldoc));
    try( htmldoc_convert_sourcebuf_to_utf8(htmldoc));
    if (htmldoc_js_is_enabled(htmldoc))
        try(curl_lexbor_fetch_scripts(htmldoc, url_client, msg_writer));
    return Ok;
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


Err lexcurl_dup_curl_from_node_and_attr(
    lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[_1_]
)
{
    Err e = Ok;

    StrView data = lexbor_get_attr(node, attr, attr_len);
    if (!data.items || !data.len)
        return "lexbor node does not have attr";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    Str* buf = &(Str){0};
    try_or_jump(e, failure, null_terminated_str_from_mem((char*)data.items, data.len, buf));
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

