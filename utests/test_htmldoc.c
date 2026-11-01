#include <utests.h>

#include "../src/htmldoc.h"

static char* MOCK_URL_STR_ = NULL;

static Err
mock_w_curl_get_effective_url(CurlPtr curl, char* effective_url[1]) {
    (void)curl;
    *effective_url = MOCK_URL_STR_;
    return  Ok;
}

#define w_curl_get_effective_url mock_w_curl_get_effective_url
#include "../src/htmldoc.c"
#undef request_urlstr


static int
check_save_path(StrView url, StrView expected) {
    Str     out = (Str){0};
    HtmlDoc doc = (HtmlDoc){0};
    Err     err;
    int     rc  = 1;

    MOCK_URL_STR_ = (char*)url.items;
    err = htmldoc_append_cache_path(&doc, &out);
    MOCK_URL_STR_ = NULL;

    if (err) {
        fprintf(stderr, "url='%.*s' unexpected error: %s\n", (int)url.len, url.items, err);
        goto done;
    }
    if (!str_eq(sv(&out), expected)) {
        fprintf(stderr, "url='%.*s' expected '%.*s', got '%.*s'\n",
                (int)url.len, url.items,
                (int)expected.len, expected.items ? expected.items : "",
                (int)out.len,      out.items      ? out.items      : "");
        goto done;
    }
    rc = 0;

done:
    str_clean(&out);
    return rc;
}


/* --- tests --- */

static int
test_htmldoc_append_cache_path_simple(void) {
    return check_save_path(
        svl("https://example.com/dir/file"),
        svl("https:/example.com/dir/file.html"));
}

static int
test_htmldoc_append_cache_path_already_html(void) {
    return check_save_path(
        svl("https://example.com/dir/page.html"),
        svl("https:/example.com/dir/page.html"));
}

static int
test_htmldoc_append_cache_path_trailing_slash(void) {
    return check_save_path(
        svl("https://example.com/dir/"),
        svl("https:/example.com/dir.html"));
}

/* Single-character *final* segment. Current `len != 1 && *beg != '/'`
 * with && drops it: bug gives "http:/x.html", correct gives
 * "http:/x/y.html". Regression guard for the && / || bug. */
static int
test_htmldoc_append_cache_path_single_char_leaf(void) {
    return check_save_path(
        svl("http://x/y"),
        svl("http:/x/y.html"));
}

/* Single-character *intermediate* segment must also be preserved. */
static int
test_htmldoc_append_cache_path_single_char_dir(void) {
    return check_save_path(
        svl("http://x/a/y.html"),
        svl("http:/x/a/y.html"));
}

static int
test_htmldoc_append_cache_path_single_char_last_no_ext(void) {
    return check_save_path(
        svl("http://x/a"),
        svl("http:/x/a.html"));
}

static int
test_htmldoc_append_cache_path_dotted_host(void) {
    return check_save_path(
        svl("https://example.com/a.b/c"),
        svl("https:/example.com/a.b/c.html"));
}

static int
test_htmldoc_append_cache_path_with_port(void) {
    return check_save_path(
        svl("http://example.com:8080/x"),
        svl("http:/example.com:8080/x.html"));
}

static int
test_htmldoc_append_cache_path_query_kept(void) {
    return check_save_path(
        svl("https://example.com/search?q=1"),
        svl("https:/example.com/search?q=1.html"));
}


int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_htmldoc_append_cache_path_simple()
        + test_htmldoc_append_cache_path_already_html()
        + test_htmldoc_append_cache_path_trailing_slash()
        + test_htmldoc_append_cache_path_single_char_leaf()
        + test_htmldoc_append_cache_path_single_char_dir()
        + test_htmldoc_append_cache_path_single_char_last_no_ext()
        + test_htmldoc_append_cache_path_dotted_host()
        + test_htmldoc_append_cache_path_with_port()
        + test_htmldoc_append_cache_path_query_kept()
        ;
    print_test_result(errors);
    return errors ? 1 : 0;
}
