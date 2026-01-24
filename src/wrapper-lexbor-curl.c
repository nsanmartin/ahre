#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "url-client.h"
#include "str.h"



/* internal linkage */


static Err _lexbor_parse_chunk_begin_(HtmlDoc htmldoc[_1_]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) 
        return "error: lex failed to init html document";
    textbuf_reset(htmldoc_sourcebuf(htmldoc));
    return Ok;
}


static Err _lexbor_parse_chunk_end_(HtmlDoc htmldoc[_1_]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    lexbor_status_t lxb_status = lxb_html_document_parse_chunk_end(lxbdoc);
    if (LXB_STATUS_OK != lxb_status) 
        return err_fmt("error: lbx failed to parse html, status: %d", lxb_status);

    try (textbuf_append_part(htmldoc_sourcebuf(htmldoc), "\0", 1));
    return textbuf_append_line_indexes(htmldoc_sourcebuf(htmldoc));
}


static Err _curl_set_write_fn_and_data_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_HEADERDATA, htmldoc)
    || curl_easy_setopt(url_client->curl, CURLOPT_HEADERFUNCTION, curl_header_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) {
        return "error configuring curl write fn/data";
    }
    return Ok;
}


static Err _curl_set_http_method_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
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

static Err _curl_set_curlu_(UrlClient url_client[_1_], HtmlDoc htmldoc[_1_]) {
    return curl_set_url(url_client, htmldoc_url(htmldoc));
}

static Err _curl_perform_error_( HtmlDoc htmldoc[_1_], CURLcode curl_code) {
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


static Err _fetch_tag_script_from_text_(lxb_dom_node_t node[_1_], ArlOf(Str) out[_1_]) {

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
    CmdOut                cmd_out[_1_]
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
               else try(
                   cmd_out_msg_append_lit__(cmd_out,  "script elem with more than one child??]n"));
               //Q? can script elem have mode that one (text) child?
            }
        }
        if (e) try(cmd_out_msg_append(cmd_out, (char*)e, strlen(e)));
    }
    return Ok;
}



static void _map_append_nullchar_(ArlOf(Str) strlist[_1_], CmdOut cmd_out[_1_]) {
    for ( Str* sp = arlfn(Str,begin)(strlist) ; sp != arlfn(Str,end)(strlist) ; ++sp) {
        Err e0 = str_append_lit__(sp, "\0");
        if (e0) {
            str_reset(sp);
            /*ignore e*/ cmd_out_msg_append_lit__(cmd_out, "could not append \\0 to str: ");
            /*ignore e*/ cmd_out_msg_append(cmd_out, (char*)e0, strlen(e0));
        }
    }
}

static Err curl_lexbor_fetch_scripts(
    HtmlDoc        htmldoc[_1_],
    UrlClient      url_client[_1_],
    CmdOut            cmd_out[_1_]
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
            _split_remote_local_(head_scripts, htmldoc_head_scripts(htmldoc), head_urls, cmd_out));
    try_or_jump(e, Clean_Head_Urls,
            _split_remote_local_(body_scripts, htmldoc_body_scripts(htmldoc), body_urls, cmd_out));

    ArlOf(CurlPtr)*  easies = &(ArlOf(CurlPtr)){0};
    ArlOf(CurlUPtr)* curlus = &(ArlOf(CurlUPtr)){0};
    CURLM*           multi  = url_client_multi(url_client);
    CURLU*           curlu  = url_cu(htmldoc_url(htmldoc));

    e = url_client_multi_add_handles(
        url_client, curlu, head_urls, htmldoc_head_scripts(htmldoc), easies, curlus, cmd_out);
    if (e) {
        try(cmd_out_msg_append_lit__(cmd_out, "could not add head handles: "));
        try(cmd_out_msg_append(cmd_out, (char*)e, strlen(e)));
    }
    e = url_client_multi_add_handles(
        url_client, curlu, body_urls, htmldoc_body_scripts(htmldoc), easies, curlus, cmd_out);
    if (e) {
        try(cmd_out_msg_append_lit__(cmd_out, "could not add head handles: "));
        try(cmd_out_msg_append(cmd_out, (char*)e, strlen(e)));
    }

    e = w_curl_multi_perform_poll(multi);

    for_htmldoc_size_download_append(
        easies, cmd_out, url_client_curl(url_client), htmldoc_curlinfo_sz_download(htmldoc));
    w_curl_multi_remove_handles(multi, easies, cmd_out);

    arlfn(CurlPtr,clean)(easies);
    arlfn(CurlUPtr,clean)(curlus);

    _map_append_nullchar_(htmldoc_head_scripts(htmldoc), cmd_out);
    _map_append_nullchar_(htmldoc_body_scripts(htmldoc), cmd_out);

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
    CmdOut            cmd_out[_1_],
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
    fetch_history_entry_update_curl(histentry, url_client->curl, cmd_out);
    str_reset(url_client_postdata(url_client));
    if (curl_code!=CURLE_OK) 
        return _curl_perform_error_(htmldoc, curl_code);

    if (histentry->size_download_t < 0)
        try(cmd_out_msg_append_lit__(cmd_out, "CURLINFO_SIZE_DOWNLOAD_T is negative"));
    else *htmldoc_curlinfo_sz_download(htmldoc) = histentry->size_download_t;

    try( _lexbor_parse_chunk_end_(htmldoc));
    try( _set_htmldoc_url_with_effective_url_(url_client, htmldoc));
    try( htmldoc_convert_sourcebuf_to_utf8(htmldoc));
    if (htmldoc_js_is_enabled(htmldoc))
        try(curl_lexbor_fetch_scripts(htmldoc, url_client, cmd_out));
    return Ok;
}




/* Err lexcurl_dup_curl_from_node_and_attr( */
/*     lxb_dom_node_t* node, const char* attr, size_t attr_len, CURLU* u[_1_] */
/* ) */
/* { */
/*     Err e = Ok; */

/*     StrView data = lexbor_get_attr(node, attr, attr_len); */
/*     if (!data.items || !data.len) */
/*         return "lexbor node does not have attr"; */

/*     CURLU* dup = curl_url_dup(*u); */
/*     if (!dup) return "error: memory failure (curl_url_dup)"; */
/*     Str* buf = &(Str){0}; */
/*     try_or_jump(e, failure, null_terminated_str_from_mem((char*)data.items, data.len, buf)); */
/*     try_or_jump(e, failure, curlu_set_url_or_fragment(dup, buf->items)); */
/*     str_clean(buf); */
/*     *u = dup; */
/*     return Ok; */
/* failure: */
/*     str_clean(buf); */
/*     curl_url_cleanup(dup); */
/*     return e; */
/* } */


/* char* mem_whitespace(char* s, size_t len) { */
/*     while(len && *s && !isspace(*s)) { ++s; --len; } */
/*     return len ? s : NULL; */
/* } */


