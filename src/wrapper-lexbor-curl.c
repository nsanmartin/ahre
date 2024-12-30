#include "src/error.h"
#include "src/url-client.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"
#include "src/wrapper-lexbor-curl.h"


Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    //const char* url = htmldoc->url;
    //if (!url || !*url) return "error: unexpected null url";
    if (curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
        || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) { return "error configuring curl write fn/data"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) { return "error: lex failed to init html document"; }
    //if (curl_easy_setopt(url_client->curl, CURLOPT_URL, url)) { return "error: curl failed to set url"; }
    if (curl_easy_setopt(url_client->curl, CURLOPT_CURLU, url_curlu(htmldoc_curlu(htmldoc)))) {
        return "error: curl failed to set url";
    }

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    if (curl_code!=CURLE_OK) {
        return err_fmt("curl failed to perform curl: %s", curl_easy_strerror(curl_code));
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


Err mk_submit_url (lxb_dom_node_t* form, HtmlDoc d[static 1], CURLU* out[static 1]) {

    Err err;
    *out = curl_url_dup(url_curlu(htmldoc_curlu(d)));
    if (!*out) return "error: memory failure (curl_url_dup)";

    BufOf(lxb_char_t)* buf = &(BufOf(lxb_char_t)){0};

    const lxb_char_t* action;
    size_t action_len;
    lexbor_find_attr_value(form, "action", &action, &action_len);
    if (!action_len) return "no action in form (formaction in button not supported yet)";
    if (!buffn(lxb_char_t, append)(buf, (lxb_char_t*)action, action_len)
        || !buffn(lxb_char_t, append)(buf, (lxb_char_t*)"\0", 1)) {
        err = "error: memory failure (BufOf)";
        goto free_url_dup;
    }
    

    CURLUcode curl_code = curl_url_set(*out, CURLUPART_PATH, (const char*)buf->items, 0);
    if (curl_code != CURLUE_OK) {
        err = err_fmt("error: curl_url_set failed: %s\n", curl_url_strerror(curl_code));
        goto clean_buf;
    }
    
    buffn(lxb_char_t, reset)(buf);

    if ((err=_make_submit_curlu_rec(form, buf, *out))) {
        goto clean_buf;
    }

    buffn(lxb_char_t, clean)(buf);
    return Ok;
clean_buf:
    buffn(lxb_char_t, clean)(buf);
free_url_dup:
    curl_url_cleanup(*out);
    return err;
}

