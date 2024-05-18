#include <wrappers.h>

int curl_set_all_options(CURL* curl, const char* url, char* errbuf) {
    return curl_easy_setopt(curl, CURLOPT_URL, url)
        || curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf)
        || curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L)
        || curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)
        ;
}

int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf) {
    return curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback)
        || curl_easy_setopt(curl, CURLOPT_WRITEDATA, docbuf)
        ;
}


size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out) {
    //TidyBuffer *out) {
  unsigned r = size * nmemb;
  tidyBufAppend(out, in, r);
  return r;
}
 
