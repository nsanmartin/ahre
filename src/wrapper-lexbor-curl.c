#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "url-client.h"
#include "wrapper-lexbor-curl.h"
#include "wrapper-lexbor.h"

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

static Err
_set_htmldoc_url_with_effective_url_(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    char* effective_url = NULL;
    if (CURLE_OK != curl_easy_getinfo(url_client->curl, CURLINFO_EFFECTIVE_URL, &effective_url)) {
        return "error: couldn't get effective url from curl";
    }
    return curlu_set_url(url_cu(htmldoc_url(htmldoc)), effective_url);
}

//static Err
//_set_htmldoc_http_charset_(HtmlDoc htmldoc[static 1], CURL* curl) {
//    struct curl_header *type;
//    CURLHcode code = curl_easy_header(curl, "Content-Type", 0, CURLH_HEADER, -1, &type);
//    if (code != CURLHE_OK && code != CURLHE_NOHEADERS)
//        return err_fmt("error: curl header failed: %d", code);
//
//    if (code == CURLHE_OK) {
//        //printf("name: %s\n", type->name);
//        //printf("value: %s\n", type->value);
//#define CHARSET_KEY_ "charset="
//        char* charset = strstr(type->value, CHARSET_KEY_); /* is it case insentitive? */
//        if (charset) {
//            charset += lit_len__(CHARSET_KEY_);
//            try(str_append(
//                    htmldoc_http_charset(htmldoc),
//                    charset,
//                    strlen(charset)+1
//                )
//            );
//            printf("charset: '");
//            fwrite(
//                items__(htmldoc_http_charset(htmldoc)),
//                1,
//                len__(htmldoc_http_charset(htmldoc)),
//                stdout
//            );
//            puts("'");
//        }
//    }
//    return Ok;
//}

Err curl_lexbor_fetch_document(
    UrlClient url_client[static 1],
    HtmlDoc htmldoc[static 1],
    SessionWriteFn wfnc,
    CurlLxbFetchCb cb
) {
    try( _curl_set_write_fn_and_data_(url_client, htmldoc));
    try( _lexbor_parse_chunk_begin_(htmldoc));
    try( _curl_set_http_method_(url_client, htmldoc));
    try( _curl_set_curlu_(url_client, htmldoc));

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    buffn(const_char, reset)(url_client_postdata(url_client));
    if (curl_code!=CURLE_OK) 
        return _curl_perform_error_(htmldoc, curl_code);

    try( _lexbor_parse_chunk_end_(htmldoc));
    try( _set_htmldoc_url_with_effective_url_(url_client, htmldoc));
    try( htmldoc_convert_sourcebuf_to_utf8(htmldoc));
    if (cb) try( cb(wfnc, url_client->curl));
    return Ok;
}

Err curl_save_url(UrlClient url_client[static 1], CURLU* curlu , const char* fname) {
    try( url_client_reset(url_client));
    FILE* fp = fopen(fname, "wa");
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


static Err _make_submit_get_curlu_rec(
    lxb_dom_node_t* node, BufOf(lxb_char_t) buf[static 1], CURLU* out
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

        if (strcasecmp("password", (char*)name) == 0)
            return "warn: passwords not allowed in get requests";


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
    UrlClient url_client[static 1],
    lxb_dom_node_t node[static 1],
    bool is_https,
    BufOf(const_char) buf[static 1]
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
    if (strcasecmp("password", (char*)name) == 0 && !is_https)
        return "warn: passwords allowed only under https";

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
    bool is_https;
    try( curlu_scheme_is_https(*out, &is_https));

    for(lxb_dom_node_t* it = form->first_child; it ; it = it->next) {
        if ((err=_make_submit_post_curlu_rec(url_client, it, buf, *out, is_https))) {
            buffn(const_char, clean)(buf);
            return err;
        }
        if (it == form->last_child) break;
    }

    //TODO: move this somehow to `curl_lexbor_fetch_document`
    if (len__(buf)) {
    
        CURLcode code;
        code = curl_easy_setopt(url_client->curl, CURLOPT_POSTFIELDSIZE, len__(buf)-1);
        if (CURLE_OK != code) {
            buffn(const_char, clean)(buf);
            return "error: curl postfields size set failure";
        }
        code = curl_easy_setopt(url_client->curl, CURLOPT_POSTFIELDS, items__(buf)+1);
        if (CURLE_OK != code) {
            buffn(const_char, clean)(buf);
            return "error: curl postfields set failure";
        }
    } else return "warn: no post fields, not submiting form";

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

    if (action && action_len) {
        BufOf(const_char)* buf = &(BufOf(const_char)){0};
        Err err = _submit_url_set_action(buf, action, action_len, *out);
        buffn(const_char, clean)(buf);
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
) {

    size_t len;
    const char* value = (const char*)lxb_dom_element_get_attribute(
        lxb_dom_interface_element(node), (const lxb_char_t*)attr, attr_len, &len
    );
    if (!len || !value) return "attr does not have value";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    BufOf(const_char)*buf = &(BufOf(const_char)){0};
    if ( !buffn(const_char, append)(buf, value, len)
       ||!buffn(const_char, append)(buf, "\0", 1)
    ) {
        buffn(const_char, clean)(buf);
        curl_url_cleanup(dup);
        return "error: buffn append failure";
    }
    Err err = curlu_set_url(dup, buf->items);
    buffn(const_char, clean)(buf);
    if (err) {
        curl_url_cleanup(dup);
        return err;
    }
    *u = dup;
    return Ok;
}

//TODO: just call lexcurl_dup_curl_from_node_and_attr
Err lexcurl_dup_curl_with_anchors_href(lxb_dom_node_t* anchor, CURLU* u[static 1]) {
    const lxb_char_t* data;
    size_t data_len;
    lexbor_find_attr_value(anchor, "href", &data, &data_len);
    if (!data || !data_len)
        return "anchor does not have href";

    CURLU* dup = curl_url_dup(*u);
    if (!dup) return "error: memory failure (curl_url_dup)";
    BufOf(const_char)*buf = &(BufOf(const_char)){0};
    if ( !buffn(const_char, append)(buf, (const char*)data, data_len)
       ||!buffn(const_char, append)(buf, "\0", 1)
    ) {
        buffn(const_char, clean)(buf);
        curl_url_cleanup(dup);
        return "error: buffn append failure";
    }
    Err err = curlu_set_url(dup, buf->items);
    buffn(const_char, clean)(buf);
    if (err) {
        curl_url_cleanup(dup);
        return err;
    }
    *u = dup;
    return Ok;
}

char* mem_whitespace(char* s, size_t len) {
    while(len && *s && !isspace(*s)) { ++s; --len; }
    return len ? s : NULL;
}

#define lit_match__(Lit, Mem, Len) (lit_len__(Lit) <= Len && !strncasecmp(Lit, Mem, lit_len__(Lit)))
size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) {
    /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
    /* 'userdata' is set with CURLOPT_HEADERDATA */
#define CHARSET_K "charset="
#define CONTENT_TYPE_K "content-type:"

    HtmlDoc* hd = (HtmlDoc*)htmldoc;
    size_t len = size * nitems;
    while (len) {
        if (lit_match__(CHARSET_K, buffer, len)) {
            size_t matchlen =  lit_len__(CHARSET_K);
            buffer += matchlen;
            len -= matchlen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            char* ws = mem_whitespace(buffer, len);
            size_t chslen = ws ? (size_t)(ws - buffer) : len;
            str_reset(htmldoc_http_charset(hd));
            str_append(htmldoc_http_charset(hd), buffer, chslen);
            str_append_lit__(htmldoc_http_charset(hd), "\0");
            buffer += chslen;
            len -= chslen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            continue;
        } else if (lit_match__(CONTENT_TYPE_K, buffer, len)) {
            size_t matchlen = lit_len__(CONTENT_TYPE_K);
            buffer += matchlen;
            len -= matchlen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            char* colon = memchr(buffer, ';', len);
            size_t contypelen = colon ? (size_t)(colon - buffer) : len;
            str_reset(htmldoc_http_content_type(hd));
            //TODO: does not depend on null termoination
            str_append(htmldoc_http_content_type(hd), buffer, contypelen);
            if (colon) ++contypelen;
            buffer += contypelen;
            len -= contypelen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            continue;
        } else return size * nitems;
    }

    return size * nitems;
}
