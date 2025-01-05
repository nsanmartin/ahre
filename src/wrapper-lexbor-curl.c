#include "src/error.h"
#include "src/url-client.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"
#include "src/wrapper-lexbor-curl.h"


Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;

    if (  curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
       || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) {
        return "error configuring curl write fn/data";
    }

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) {
        return "error: lex failed to init html document";
    }

    Url* url = htmldoc_url(htmldoc);

    CURLoption method = htmldoc_method(htmldoc) == http_post 
               ? CURLOPT_POST
               : CURLOPT_HTTPGET
               ;
    if (curl_easy_setopt(url_client->curl, method, url_cu(url))) {
        return "error: curl failed to set method";
    }

    if (curl_easy_setopt(url_client->curl, CURLOPT_CURLU, url_cu(url))) {
        return "error: curl failed to set url";
    }

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    if (curl_code!=CURLE_OK) {
        char* u;
        Err e = url_cstr(url, &u);
        if (e) u = "and url could not be obtained due to another error :/";
        Err err = err_fmt(
            "curl failed to perform curl: %s (%s)", curl_easy_strerror(curl_code), u
        );
        if (!e) curl_free(u);
        return err;
    }

    lexbor_status_t lxb_status = lxb_html_document_parse_chunk_end(lxbdoc);
    if (LXB_STATUS_OK != lxb_status) {
        return err_fmt("error: lbx failed to parse html, status: %d", lxb_status);
    }

    return Ok;
}


static Err _make_submit_curlu_rec(
    lxb_dom_node_t node[static 1],
    BufOf(lxb_char_t) buf[static 1],
    CURLU* out
) {
    if (!node) return Ok;
    else if (node->local_name == LXB_TAG_INPUT && !_lexbor_attr_has_value(node, "type", "submit")) {
        //try(_append_lexbor_name_value_attrs_if_both(url_client, node, submit_url));

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
        Err err = _make_submit_curlu_rec(it, buf, out);
        if (err) return err;
        if (it == node->last_child) { break; }
    }
    return Ok;
}

static Err _submit_url_set_action(
    BufOf(lxb_char_t)* buf,
    const lxb_char_t* action,
    size_t action_len,
    CURLU* out
) {
    if (  !buffn(lxb_char_t, append)(buf, (lxb_char_t*)action, action_len)
       || !buffn(lxb_char_t, append)(buf, (lxb_char_t*)"\0", 1)) {
        return "error: memory failure (BufOf)";
    }

    CURLUcode curl_code = curl_url_set(out, CURLUPART_URL, (const char*)buf->items, 0);
    if (curl_code != CURLUE_OK) {
        return err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
    }
    
    buffn(lxb_char_t, reset)(buf);
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

static Err mk_submit_get(
    lxb_dom_node_t* form, const lxb_char_t* action, size_t action_len, CURLU* out[static 1]
) {
    Err err;
    BufOf(lxb_char_t)* buf = &(BufOf(lxb_char_t)){0};
    if (action && action_len) {
        if ((err = _submit_url_set_action(buf, action, action_len, *out))) {
            buffn(lxb_char_t, clean)(buf);
            return err;
        }
    }

    if ((err=_make_submit_curlu_rec(form, buf, *out))) {
        buffn(lxb_char_t, clean)(buf);
        return err;
    }

    buffn(lxb_char_t, clean)(buf);
    return Ok;
}

Err mk_submit_url (lxb_dom_node_t* form, CURLU* out[static 1], HttpMethod http_method[static 1]) {
    //Err err;

    const lxb_char_t* action;
    size_t action_len;
    lexbor_find_attr_value(form, "action", &action, &action_len);

    const lxb_char_t* method;
    size_t method_len;
    lexbor_find_attr_value(form, "method", &method, &method_len);

    if (!method_len || lexbor_str_eq("get", method, method_len)) {
        *http_method = http_get;
        return mk_submit_get(form, action, action_len, out);
    }
    return "not a get method";
}

