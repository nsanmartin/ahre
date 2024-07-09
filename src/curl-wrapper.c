#include <wrappers.h>

int curl_set_callback_and_buffer(CURL* curl, curl_write_callback callback, void* docbuf) {
    return curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback)
        || curl_easy_setopt(curl, CURLOPT_WRITEDATA, docbuf)
        ;
}

