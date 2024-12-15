#include "src/wrapper-lexbor-curl.h"
#include "src/error.h"
#include "src/url-client.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"


Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc->lxbdoc;
    const char* url = htmldoc->url;
    if (!url || !*url) return "error: unexpected null url";
    if (curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
        || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, htmldoc)) { return "error configuring curl write fn/data"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(lxbdoc)) { return "error: lex failed to init html document"; }
    if (curl_easy_setopt(url_client->curl, CURLOPT_URL, url)) { return "error: curl failed to set url"; }
    if (curl_easy_perform(url_client->curl)) { return err_fmt("error: curl failed to perform curl on '%s'", url); }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(lxbdoc)) { return "error: lbx failed to parse html"; }

    return Ok;
}



