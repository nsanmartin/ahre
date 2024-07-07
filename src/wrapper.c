#include <mem.h>
#include <wrappers.h>

#include <ahutils.h>
#include <ahdoc.h>
#include <aherror.h>

AhCurl* AhCurlCreate(void) {
    AhCurl* rv = ah_malloc(sizeof(AhCurl));
    if (!rv) { perror("Mem Error"); goto exit_fail; }
    CURL* handle = curl_easy_init();
    if (!handle) { perror("Curl init error"); goto free_rv; }

    *rv = (AhCurl) { .curl=handle, .errbuf={0} };

    /* default options to curl */
    if (curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, rv->errbuf)
            || curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L)
            || curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1)) {
        perror("Error configuring curl"); goto cleanup_curl;
    }

    return rv;

cleanup_curl:
    curl_easy_cleanup(handle);
free_rv:
    ah_free(rv);
exit_fail:
    return 0x0;
}

void AhCurlFree(AhCurl* ac) {
    curl_easy_cleanup(ac->curl);
     ah_free(ac);
}

ErrStr curl_lexbor_fetch_document(AhCurl ahcurl[static 1], AhDoc* ahdoc) {
    if (curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEFUNCTION, chunk_callback)
        || curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEDATA, ahdoc->doc)) { return "Error configuring curl write fn/data"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(ahdoc->doc)) { return "Lex failed to init html document"; }
    if (curl_easy_setopt(ahcurl->curl, CURLOPT_URL, ahdoc->url)) { return "Curl failed to set url"; }
    if (curl_easy_perform(ahcurl->curl)) { return "Curl failed to perform curl"; }
    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(ahdoc->doc)) { return "Lbx failed to parse html"; }

    ah_log_info("url fetched");
    return Ok;
}
