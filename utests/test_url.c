#include <utests.h>
#include <stdio.h>
#include <curl/urlapi.h>

#include "../src/error.c"

typedef CURLUcode (*CurlUrlGetFn)(const CURLU* u, CURLUPart p, char** c, unsigned int f);
static CurlUrlGetFn _mock_curl_url_get_ = NULL;
static unsigned _curl_free_calls = 0;

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

static inline CURLUcode mock_curl_url_get_only_once(
        const CURLU* url, CURLUPart part, char** content, unsigned int flags) {
    utest_ignore_params(url, part, content, flags);
    static size_t mock_curl_url_get_only_once_calls = 0;
    if (mock_curl_url_get_only_once_calls == 0) {
        ++mock_curl_url_get_only_once_calls;
        return CURLUE_OK;
    }
    return CURLUE_BAD_HANDLE;
}

static const char* _mock_fopen_pathname_param = NULL;
static inline FILE* _mock_fopen_with(const char *restrict pathname, const char *restrict mode) {
    utest_ignore_params(mode);
    if (strcmp(_mock_fopen_pathname_param, pathname)) return NULL;
    return (FILE*)-1;
}

#define curl_url_get mock_curl_url_get
#define curl_free mock_curl_free
#define fopen _mock_fopen_with
#include "../src/str.c"
#include "../src/url.c"


int test_curl_url_to_filename_append_0(void) {
    CURLU* cu = NULL;
    Str res = (Str){0};

    _mock_curl_url_get_ = mock_curl_url_get_only_once;
    Err e = curl_url_to_filename_append(cu, &res);
    utest_assert(e, fail);
    utest_assert(_curl_free_calls == 1, fail);
    return 0;
fail:
    return 1;
}

int test_curl_url_to_filename_append(void) {
    CURLU* cu = NULL;
    Str res = (Str){0};

    _curl_free_calls = 0;
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

Err _append_fopen(const char* dirname, CURLU* cu, FILE* fp[static 1]);

int test__append_fopen(void) {
    const char* dirname = "foo";
    CURLU* cu         = NULL;
    FILE* fp          = 0x0;

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "foo", (char[]){'/','\0'}};
    _mock_curl_url_get_ = mock_curl_url_get_1;
    _mock_fopen_pathname_param = dirname;
    Err e = _append_fopen(dirname, cu, &fp);
    utest_assert(!e, fail);

    return 0;
fail:
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_curl_url_to_filename_append_0()
        + test_curl_url_to_filename_append()
        + test__append_fopen();
    print_test_result(errors);
}
