#include <string.h>
#include <ah/curl.h>


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


/* debug */
int AhCurlBufSummary(AhCurl ahcurl[static 1], BufOf(char)*buf) {
   buf_append_lit("AhCurl: ", buf);
   if (buf_append_hexp(ahcurl, buf)) { return -1; }
   buf_append_lit("\n", buf);

   if (ahcurl->curl) {
       buf_append_lit(" curl: ", buf);
       if (buf_append_hexp(ahcurl->curl, buf)) { return -1; }
       buf_append_lit("\n", buf);

       if (ahcurl->errbuf[0]) {
           size_t slen = strlen(ahcurl->errbuf);
           if (slen > CURL_ERROR_SIZE) { slen = CURL_ERROR_SIZE; }
           if (buffn(char,append)(buf, ahcurl->errbuf, slen)) {
               return -1;
           }
           buf_append_lit("\n", buf);
       }
   } else {
       buf_append_lit( " not curl\n", buf);
   }
   return 0;
}

