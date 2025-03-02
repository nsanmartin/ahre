#include <string.h>

#include "src/generic.h"
#include "src/mem.h"
#include "src/url-client.h"


//TODO: use init
UrlClient* url_client_create(void) {
    UrlClient* rv = std_malloc(sizeof(UrlClient));
    if (!rv) { perror("Mem Error"); goto exit_fail; }
    CURL* handle = curl_easy_init();
    if (!handle) { perror("Curl init error"); goto free_rv; }
    //struct curl_slist *headerlist=NULL;

    *rv = (UrlClient) { .curl=handle, .errbuf={0} };

    /* default options to curl */
    if (   curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, rv->errbuf)
        || curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L)
        || curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1)
        || curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L)
        || curl_easy_setopt(handle, CURLOPT_USERAGENT, "ELinks/0.13.2 (textmode; Linux 6.6.62+rpt-rpi-2712 aarch64; 213x55-2)")
        //|| curl_easy_setopt(handle, CURLOPT_USERAGENT, "w3m/0.5.3")
        /* google does not want to responde properly when I use ahre's user agent */
        //|| curl_easy_setopt(handle, CURLOPT_USERAGENT, "Ahre/0.0.1")
        || curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "")
        //|| curl_easy_setopt(handle, CURLOPT_COOKIEJAR, "cookies.txt")
    ) { 
        perror("Error configuring curl, exiting."); goto cleanup_curl;
    }

    return rv;

cleanup_curl:
    curl_easy_cleanup(handle);
free_rv:
    std_free(rv);
exit_fail:
    return 0x0;
}

Err url_client_print_cookies(UrlClient uc[static 1]) {
    struct curl_slist* cookies = NULL;
    CURLcode curl_code = curl_easy_getinfo(uc->curl, CURLINFO_COOKIELIST, &cookies);
    if (curl_code != CURLE_OK) { return "error: could not retrieve cookies list"; }
    if (!cookies) { puts("No cookies"); return "no cookies"; }

    struct curl_slist* it = cookies;
    while (it) {
        printf("%s\n", it->data);
        it = it->next;
    }
    curl_slist_free_all(cookies);
    return Ok;
}

void url_client_destroy(UrlClient* url_client) {
    curl_easy_cleanup(url_client->curl);
    buffn(const_char, clean)(url_client_postdata(url_client));
    std_free(url_client);
}


const char* _parse_opt(const char* line, CURLoption opt[static 1]) {

    const char* rest;
    if ((rest = substr_match(line, "noprogress", 1))) { *opt=CURLOPT_NOPROGRESS; return rest; }
    if ((rest = substr_match(line, "useragent", 1))) { *opt=CURLOPT_USERAGENT; return rest; }
    if ((rest = substr_match(line, "verbose", 1))) { *opt=CURLOPT_VERBOSE; return rest; }
    return NULL;
}

static Err _parse_setopt_long_(CURL* handle, CURLoption opt, const char* line) {
    long value = 0;
    const char* rest = parse_l(line, &value);
    if (!rest) return "not value to setopt, expecting a long";
    if (*cstr_skip_space(rest)) return err_fmt("invalid opt: %s", line);
    CURLcode curl_code = curl_easy_setopt(handle, opt, value);
    if (CURLE_OK != curl_code) {
        return err_fmt("could not set curlopt: %s", curl_easy_strerror(curl_code));
    }
    return Ok;
}

static Err _curl_setopt_cstr_(CURL* handle, CURLoption opt, const char* rest) {
    if (!rest) return "not value to setopt, expecting a string";
    rest = cstr_skip_space(rest);
    if (!*rest) return "invalid opt, whitespace only";
    CURLcode curl_code = curl_easy_setopt(handle, opt, rest);
    if (CURLE_OK != curl_code) {
        return err_fmt("could not set curlopt: %s", curl_easy_strerror(curl_code));
    }
    return Ok;
}

Err cmd_set_session_monochrome(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    if (*line == '0') session_monochrome_set(session, false);
    else if (*line == '1') session_monochrome_set(session, true);
    else return "monochrome option should be '0' or '1'";
    return Ok;

}

Err cmd_set_session_input(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    HtmlDoc* htmldoc;
    try( session_current_doc(session, &htmldoc));

    UiIn uiin;
    const char* rest;
    if ((rest = substr_match(line, "fgets", 1)) && !*rest) uiin = uiin_fgets;
    else if ((rest = substr_match(line, "isocline", 1)) && !*rest) uiin = uiin_isocline;
    else return "input option should be 'getline' or 'isocline'";
    *session_conf_uiin(session) = uiin;
    return Ok;

}


Err cmd_set_session(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest = substr_match(line, "monochrome", 1))) return cmd_set_session_monochrome(session, rest);
    if ((rest = substr_match(line, "input", 1))) return cmd_set_session_input(session, rest);
    return "not a session option";
}

Err cmd_set_curl(Session session[static 1], const char* line) {
    CURLoption opt;
    const char* rest = _parse_opt(line, &opt);
    if (!rest) return "invalid curl opt";
    CURL* handle = session_url_client(session)->curl;
    switch(opt) {
        case CURLOPT_VERBOSE:
        case CURLOPT_NOPROGRESS:
            return _parse_setopt_long_(handle, opt, rest);
        case CURLOPT_USERAGENT:
            return _curl_setopt_cstr_(handle, opt, rest);
        default: return "not implemented curlopt";

    }
    return Ok;
}

Err cmd_set(Session session[static 1], const char* line) {
    line = cstr_skip_space(line);
    const char* rest;
    if ((rest = substr_match(line, "session", 1))) return cmd_set_session(session, rest);
    if ((rest = substr_match(line, "curl", 1))) return cmd_set_curl(session, rest);
    return "not a curl option";
}

