#include <utests.h>
#include <stdio.h>
#include <curl/urlapi.h>

#include "../src/error.c"

static Str* _mock_fopen_pathname_param = NULL;
static unsigned _curl_free_calls = 0;

static inline FILE* _mock_fopen_with(const char *restrict pathname, const char *restrict mode) {
    utest_ignore_params(mode);
    if (strcmp(items__(_mock_fopen_pathname_param), pathname)) return NULL;
    return (FILE*)-1;
}

static inline void mock_curl_free(void* p) {
    (void)p;
    ++_curl_free_calls;
}


#include "../src/url.h"
#include "../src/htmldoc.h"
Err mock_w_curl_url_get_malloc(CurlUrlPtr u, CURLUPart part, char* out[_1_]);
Err mock_w_curl_multi_add(
    UrlClient       uc[_1_],
    CURLU*          baseurl,
    const char*     urlstr,
    ArlOf(CurlPtr)  easies[_1_],
    ArlOf(Str)      destlist[_1_],
    ArlOf(CurlUrlPtr) curlus[_1_]
) ;

Err mock_curl_lexbor_fetch_document(
    UrlClient         url_client[_1_],
    HtmlDoc           htmldoc[_1_],
    CmdOut            out[_1_],
    FetchHistoryEntry histentry[_1_]
);

Err mock_w_curl_set_url(UrlClient url_client[_1_], Url url[_1_]) ;
#define w_curl_url_get_malloc mock_w_curl_url_get_malloc
#define w_curl_free mock_curl_free
#define w_curl_set_url mock_w_curl_set_url
#define w_curl_multi_add mock_w_curl_multi_add
#define curl_lexbor_fetch_document mock_curl_lexbor_fetch_document
#define fopen _mock_fopen_with
#include "../src/str.c"
#include "../src/url.c"
#include "../src/url-client.c"
#include "../src/fetch-history.c"
#include "../src/htmldoc.c"
#include "../src/tab-node.c"

Err mock_w_curl_set_url(UrlClient url_client[_1_], Url url[_1_]) {
    utest_ignore_params(url_client, url);
    return Ok;
}

Err mock_w_curl_multi_add(
    UrlClient       uc[_1_],
    CURLU*          baseurl,
    const char*     urlstr,
    ArlOf(CurlPtr)  easies[_1_],
    ArlOf(Str)      destlist[_1_],
    ArlOf(CurlUrlPtr) curlus[_1_]
) {
    utest_ignore_params(uc,baseurl, urlstr, easies);
    utest_ignore_params(destlist, curlus);
    return Ok;
}

Err mock_curl_lexbor_fetch_document(
    UrlClient url_client[_1_], HtmlDoc htmldoc[_1_], CmdOut out[_1_], FetchHistoryEntry histentry[_1_]
) {
    utest_ignore_params(url_client, htmldoc, out, histentry);
    return Ok;
}

int test__append_fopen(void);
int test_curl_url_to_filename_append(void);
int test_curl_url_to_filename_append_0(void);

typedef Err (*CurlUrlGetFn)(CurlUrlPtr u, CURLUPart part, char* out[_1_]);

static CurlUrlGetFn _mock_curl_url_get_ = NULL;



Err mock_w_curl_url_get_malloc(CurlUrlPtr u, CURLUPart part, char* out[_1_]) {
    utest_ignore_params(u, part, out);
    if (_mock_curl_url_get_) return _mock_curl_url_get_(u, part, out);
    return Ok;
}

static inline Err mock_curl_url_get_e(CurlUrlPtr u, CURLUPart part, char* out[_1_]) {
    utest_ignore_params(u, part, out);
    return "error: bad handle";
}


static char** _mock_curl_url_get_content = {0};
static size_t _mock_curl_url_get_ncall = 0;
static inline
Err mock_curl_url_get_1(CurlUrlPtr u, CURLUPart part, char* out[_1_]) {
    utest_ignore_params(u, part, out);
    *out = _mock_curl_url_get_content
        ? _mock_curl_url_get_content[_mock_curl_url_get_ncall]
        : NULL;
    ++_mock_curl_url_get_ncall;
    return Ok;
}

static inline Err mock_curl_url_get_only_once(CurlUrlPtr u, CURLUPart part, char* out[_1_]) {
    utest_ignore_params(u, part, out);
    static size_t mock_curl_url_get_only_once_calls = 0;
    if (mock_curl_url_get_only_once_calls == 0) {
        ++mock_curl_url_get_only_once_calls;
        return Ok;
    }
    return "error: bad handle";
}



int test_curl_url_to_filename_append_0(void) {
    Url u   = (Url){0};
    Str res = (Str){0};

    _mock_curl_url_get_ = mock_curl_url_get_only_once;
    Err e = curl_url_to_filename_append(u, &res);
    utest_assert(e, fail);
    utest_assert(_curl_free_calls == 1, fail);
    return 0;
fail:
    return 1;
}

int test_curl_url_to_filename_append(void) {
    Url u   = (Url){0};
    Str res = (Str){0};

    _curl_free_calls = 0;
    _mock_curl_url_get_ = mock_curl_url_get_1;
    _mock_curl_url_get_content = (char*[]){ "FOO", "BAR" };
    Err e = curl_url_to_filename_append(u, &res);

    // expecting error since curl path should start with '/'
    utest_assert(!e, fail);
    utest_assert(_curl_free_calls == 2, fail);
    str_reset(&res);

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "FOO", (char[]){ '/', 'B', 'A', 'R', '\0' }};
    e = curl_url_to_filename_append(u, &res);
    utest_assert(!e, fail);
    utest_assert(!strncmp(res.items, "FOO", res.len), fail);
    str_reset(&res);
    utest_assert(_curl_free_calls == 4, fail);

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "FOO", (char[]){'/','\0'}};
    e = curl_url_to_filename_append(u, &res);
    utest_assert(!e, fail);
    utest_assert(!strncmp(res.items, "FOO", res.len), fail);
    str_reset(&res);
    utest_assert(_curl_free_calls == 6, fail);

    _mock_curl_url_get_ = mock_curl_url_get_e;
    e = curl_url_to_filename_append(u, &res);
    utest_assert(e, fail);
    utest_assert(_curl_free_calls == 6, fail);
    str_reset(&res);

    _mock_curl_url_get_ = mock_curl_url_get_1;
    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){
        (char[]){ 'h', 't', 't', 'p', ':', '/', '/', 'F', 'o', 'o', '\0' }, "http"
    };
    e = curl_url_to_filename_append(u, &res);
    utest_assert(!e, fail);
    utest_assert(!strcmp(res.items, "Foo"), fail);
    utest_assert(_curl_free_calls == 8, fail);
    str_reset(&res);

    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){
        (char[]){ 'h', 't', 't', 'p', ':', '/', '/', 'F', 'o', 'o', '/', 'b', 'a', 'r', '\0' },
        "http"
    };
    e = curl_url_to_filename_append(u, &res);
    utest_assert(!e, fail);
    utest_assert(!strcmp(res.items, "Foo_bar"), fail);
    utest_assert(_curl_free_calls == 10, fail);
    str_clean(&res);

    return 0;
fail:
    str_clean(&res);
    return 1;
}

Err _append_fopen(Str path[_1_], Url u, FILE* fp[_1_]);

int test__append_fopen(void) {
    Str   dirname = (Str){};
    Url   u       = (Url){0};
    FILE* fp      = 0x0;

    str_append_z(&dirname, svl("foo"));
    _mock_curl_url_get_ncall = 0;
    _mock_curl_url_get_content = (char*[]){ "foo", (char[]){'/','\0'}};
    _mock_curl_url_get_ = mock_curl_url_get_1;
    _mock_fopen_pathname_param = &dirname;
    Err e = _append_fopen(&dirname, u, &fp);
    str_clean(&dirname);
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
