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


void url_client_destroy(UrlClient* url_client) {
    curl_easy_cleanup(url_client->curl);
    buffn(const_char, clean)(url_client_postdata(url_client));
    std_free(url_client);
}


