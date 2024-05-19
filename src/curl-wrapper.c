#include <wrappers.h>

int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf) {
    return curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback)
        || curl_easy_setopt(curl, CURLOPT_WRITEDATA, docbuf)
        ;
}


size_t tidy_callback(char *in, size_t size, size_t nmemb, void* out) {
  unsigned r = size * nmemb;
  tidyBufAppend(out, in, r);
  return r;
}
 
