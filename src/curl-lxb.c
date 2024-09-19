#include <ah/mem.h>
#include <ah/wrappers.h>

#include <ah/utils.h>
#include <ah/doc.h>
#include <ah/error.h>


ErrStr curl_lexbor_fetch_document(UrlClient ahcurl[static 1], Doc* ahdoc) {
    if (curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEFUNCTION, chunk_callback)
        || curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEDATA, ahdoc->doc)) { return "Error configuring curl write fn/data"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(ahdoc->doc)) { return "Lex failed to init html document"; }
    if (curl_easy_setopt(ahcurl->curl, CURLOPT_URL, ahdoc->url)) { return "Curl failed to set url"; }
    if (curl_easy_perform(ahcurl->curl)) { return "Curl failed to perform curl"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(ahdoc->doc)) { return "Lbx failed to parse html"; }

    ah_log_info("url fetched");
    return Ok;
}
