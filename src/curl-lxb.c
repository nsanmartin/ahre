#include "src/mem.h"
#include "src/wrappers.h"

#include "src/utils.h"
#include "src/html-doc.h"
#include "src/error.h"


Err curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc html_doc[static 1]) {
    if (curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, lexbor_parse_chunk_callback)
        || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, html_doc->lxbdoc)) { return "Error configuring curl write fn/data"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(html_doc->lxbdoc)) { return "Lex failed to init html document"; }
    if (curl_easy_setopt(url_client->curl, CURLOPT_URL, html_doc->url)) { return "Curl failed to set url"; }
    if (curl_easy_perform(url_client->curl)) { return "Curl failed to perform curl"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(html_doc->lxbdoc)) { return "Lbx failed to parse html"; }

    log_info("url fetched");
    return Ok;
}
