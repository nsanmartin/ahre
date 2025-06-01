#include <utests.h>
#include <stdio.h>
#include <curl/urlapi.h>
// #include <curl/url.h>

#include "../src/error.c"

typedef CURLUcode (*CurlUrlGetFn)(const CURLU* u, CURLUPart p, char** c, unsigned int f);
static CurlUrlGetFn _mock_curl_url_get_ = NULL;
static unsigned _curl_free_calls = 0;
#define ignore

static inline void mock_curl_free(void* p) { (void)p; ++_curl_free_calls; }
static inline
CURLUcode mock_curl_url_get( const CURLU* url, CURLUPart part, char** content, unsigned int flags) {
    utest_ignore_params(url, part, content, flags);
    if (_mock_curl_url_get_) return _mock_curl_url_get_(url, part, content, flags);
    return CURLUE_OK;
}

static inline CURLUcode mock_curl_url_get_e(
    const CURLU* url,
    CURLUPart part,
    char** content,
    unsigned int flags
 ) {
    utest_ignore_params(url, part, content, flags);
    return CURLUE_BAD_HANDLE;
}


static char** _mock_curl_url_get_content = {0};
static size_t _mock_curl_url_get_ncall = 0;
static inline
CURLUcode mock_curl_url_get_1(const CURLU* url, CURLUPart part, char** content, unsigned int flags) {
    utest_ignore_params(url, part, content, flags);
    *content = _mock_curl_url_get_content
        ? _mock_curl_url_get_content[_mock_curl_url_get_ncall]
        : NULL;
    ++_mock_curl_url_get_ncall;
    return CURLUE_OK;
}

// Err fopen_or_append_fopen(const char* fname, CURLU* cu, FILE* fp[static 1]) {

#define curl_url_get mock_curl_url_get
#define curl_free mock_curl_free
#include "../src/str.c"
#include "../src/url.c"


int test_curl_url_to_filename_append(void) {
    CURLU* cu = NULL;
    Str res = (Str){0};

    _mock_curl_url_get_ = mock_curl_url_get_1;
    _mock_curl_url_get_content = (char*[]){ "FOO", "BAR" };
    Err e = curl_url_to_filename_append(cu, &res);

    // expecting error since curl path should start with '/'
    utest_assert(e, fail);
    utest_assert(_curl_free_calls == 2, fail);

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "FOO", (char[]){ '/', 'B', 'A', 'R', '\0' }};
    e = curl_url_to_filename_append(cu, &res);
    utest_assert(!e, fail);
    utest_assert(!strncmp(res.items, "FOO_BAR", res.len), fail);
    str_reset(&res);
    utest_assert(_curl_free_calls == 4, fail);

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "FOO", (char[]){'/','\0'}};
    e = curl_url_to_filename_append(cu, &res);
    utest_assert(!e, fail);
    utest_assert(!strncmp(res.items, "FOO", res.len), fail);
    str_clean(&res);
    utest_assert(_curl_free_calls == 6, fail);

    _mock_curl_url_get_ = mock_curl_url_get_e;
    e = curl_url_to_filename_append(cu, &res);
    utest_assert(e, fail);
    utest_assert(_curl_free_calls == 6, fail);

    return 0;
fail:
    return 1;
}
int main(void) {
    print_running_test(__FILE__);
    int errors = test_curl_url_to_filename_append();
    print_test_result(errors);
}
