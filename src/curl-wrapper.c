#include <ah/wrappers.h>
#include <ah/mem.h>


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


int curl_set_callback_and_buffer(
    CURL* curl, curl_write_callback callback, void* docbuf
) {
    return curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback)
        || curl_easy_setopt(curl, CURLOPT_WRITEDATA, docbuf)
        ;
}

