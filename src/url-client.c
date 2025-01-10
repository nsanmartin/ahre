#include <string.h>

#include "src/generic.h"
#include "src/mem.h"
#include "src/url-client.h"


UrlClient* url_client_create(void) {
    UrlClient* rv = std_malloc(sizeof(UrlClient));
    if (!rv) { perror("Mem Error"); goto exit_fail; }
    CURL* handle = curl_easy_init();
    if (!handle) { perror("Curl init error"); goto free_rv; }

    *rv = (UrlClient) { .curl=handle, .errbuf={0} };

    /* default options to curl */
    if (curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, rv->errbuf)
            || curl_easy_setopt(handle, CURLOPT_VERBOSE, 1)
            || curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "")
            //|| curl_easy_setopt(handle, CURLOPT_COOKIEJAR, "cookies.txt")
            || curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L)
            || curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1)) {
        perror("Error configuring curl"); goto cleanup_curl;
    }

    return rv;

cleanup_curl:
    curl_easy_cleanup(handle);
free_rv:
    std_free(rv);
exit_fail:
    return 0x0;
}

Err url_client_print_cookies(UrlClient uc[static 1]) {
    struct curl_slist* cookies = NULL;
    CURLcode curl_code = curl_easy_getinfo(uc->curl, CURLINFO_COOKIELIST, &cookies);
    if (curl_code != CURLE_OK) { return "error: could not retrieve cookies list"; }
    if (!cookies) { puts("No cookies"); return "no cookies"; }

    struct curl_slist* it = cookies;
    while (it) {
        printf("%s\n", it->data);
        it = it->next;
    }
    curl_slist_free_all(cookies);
    return Ok;
}

void url_client_destroy(UrlClient* url_client) {
    curl_easy_cleanup(url_client->curl);
    buffn(const_char, clean)(url_client_postdata(url_client));
    std_free(url_client);
}


