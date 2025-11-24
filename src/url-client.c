#include <string.h>

#include "ahre.h"
#include "generic.h"
#include "mem.h"
#include "session.h"
#include "url-client.h"
#include "cmd.h"

/*
 * User agent samples:
 *
 * "ELinks/0.13.2 (textmode; Linux 6.6.62+rpt-rpi-2712 aarch64; 213x55-2)"
 * "w3m/0.5.3+git20230718"
 *
 */
#define _W3M_USER_AGENT_ "w3m/0.5.3+git20230718"
#define _AHRE_USER_AGENT_ "ahre/" AHRE_VERSION
#define USER_AGENT_USED_ _AHRE_USER_AGENT_

Err url_client_setopt_long(UrlClient url_client[static 1], CURLoption opt, long value) {
    CURLcode code = curl_easy_setopt(url_client->curl, opt, value);
    if (code != CURLE_OK)
        return err_fmt("error: could not set long CURLOPT: %s", curl_easy_strerror(code));
    return Ok;
}



Err url_client_setopt_cstr(UrlClient url_client[static 1], CURLoption opt, char* value) {
    CURLcode code = curl_easy_setopt(url_client->curl, opt, value);
    if (code != CURLE_OK)
        return err_fmt("error: could not set cstr CURLOPT: %s", curl_easy_strerror(code));
    return Ok;
}


Err url_client_set_cookies(UrlClient url_client[static 1]) {
    Err e = Ok;
    Str* buf = &(Str){0};
    char* cookiefile = "";
    /* if an error occurs getting the cookies filename, just ignore it, we'll not use one. */
    if (Ok == get_cookies_filename(buf)) cookiefile = items__(buf);
    /* this call leaks in old curl versoins */
    try_or_jump(e, cleanup, url_client_setopt_cstr(url_client, CURLOPT_COOKIEFILE, cookiefile));
    try_or_jump(e, cleanup, url_client_setopt_cstr(url_client, CURLOPT_COOKIEJAR, cookiefile));
cleanup:
    str_clean(buf);
    return e;
}

Err url_client_set_basic_options(UrlClient url_client[static 1]) {
    if ( 0
        || curl_easy_setopt(url_client->curl, CURLOPT_NOPROGRESS, 1L)
        || curl_easy_setopt(url_client->curl, CURLOPT_FOLLOWLOCATION, 1)
        || curl_easy_setopt(url_client->curl, CURLOPT_VERBOSE, 0L)
        || curl_easy_setopt(url_client->curl, CURLOPT_USERAGENT, USER_AGENT_USED_)
    ) {
        return "error: curl setopt failed (url_client_set_basic_options)";
    }
    return url_client_set_cookies(url_client);
}


Err url_client_reset(UrlClient url_client[static 1]) {
 
    curl_easy_reset(url_client->curl);
    if (curl_easy_setopt(url_client->curl, CURLOPT_ERRORBUFFER, url_client->errbuf))
        return "error: curl errorbuffer configuration failed";
    return url_client_set_basic_options(url_client);
}


Err url_client_init(UrlClient url_client[static 1]) {
    Err e = Ok;

    *url_client = (UrlClient){0};

    url_client->curl = curl_easy_init();
    if (!url_client->curl) return "error: curl_easy_init failure";

    url_client->curlm = curl_multi_init();
    if (!url_client->curlm) {
        e = "error: curl_multi_init failure";
        goto Failure_Curl_Easy_Cleanup;
    }

    try_or_jump(e, Failure_Curl_Multi_Cleanup, url_client_reset(url_client));

    return Ok;

Failure_Curl_Multi_Cleanup:
    curl_easy_cleanup(url_client->curl);
Failure_Curl_Easy_Cleanup:
    curl_multi_cleanup(url_client->curlm);
    return e;
}


Err url_client_print_cookies(Session* s, UrlClient uc[static 1]) {
    if (!s) return "error: session is null";
    struct curl_slist* cookies = NULL;
    CURLcode curl_code = curl_easy_getinfo(uc->curl, CURLINFO_COOKIELIST, &cookies);
    if (curl_code != CURLE_OK) { return "error: could not retrieve cookies list"; }
    if (!cookies) { puts("No cookies"); return "no cookies"; }

    struct curl_slist* it = cookies;
    while (it) {
        try(session_write_msg_ln(s, it->data, strlen(it->data)));
        it = it->next;
    }
    curl_slist_free_all(cookies);
    return Ok;
}


const char* _parse_opt(const char* line, CURLoption opt[static 1]) {

    const char* rest;
    if ((rest = csubstr_match(line, "noprogress", 1))) { *opt=CURLOPT_NOPROGRESS; return rest; }
    if ((rest = csubstr_match(line, "useragent", 1))) { *opt=CURLOPT_USERAGENT; return rest; }
    if ((rest = csubstr_match(line, "verbose", 1))) { *opt=CURLOPT_VERBOSE; return rest; }
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


Err cmd_curl_set(CmdParams p[static 1]) {
    CURLoption opt;
    const char* rest = _parse_opt(p->ln, &opt);
    if (!rest) return "invalid curl opt";
    CURL* handle = session_url_client(p->s)->curl;
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

Err url_client_curlu_to_file(UrlClient url_client[static 1], CURLU* curlu , const char* fname) {
    try( url_client_reset(url_client));
    FILE* fp;
    try(fopen_or_append_fopen(fname, curlu, &fp));
    if (!fp) return err_fmt("error opening file '%s': %s\n", fname, strerror(errno));
    if (
       curl_easy_setopt(url_client->curl, CURLOPT_WRITEFUNCTION, fwrite)
    || curl_easy_setopt(url_client->curl, CURLOPT_WRITEDATA, fp)
    || curl_easy_setopt(url_client->curl, CURLOPT_CURLU, curlu)
    ) return "error configuring curl write fn/data";

    CURLcode curl_code = curl_easy_perform(url_client->curl);
    fclose(fp);

    curl_easy_reset(url_client->curl);
    if (curl_code!=CURLE_OK) 
        return err_fmt("curl failed to perform curl: %s", curl_easy_strerror(curl_code));
    try( url_client_reset(url_client));
    return Ok;
}


