#include "ahre.h"
#include "generic.h"
#include "mem.h"
#include "session.h"
#include "url-client.h"
#include "cmd.h"


//TODO: move
Err w_curl_multi_add(
    UrlClient       uc[_1_],
    CURLU*          baseurl,
    const char*     urlstr,
    ArlOf(CurlPtr)  easies[_1_],
    ArlOf(Str)      destlist[_1_],
    ArlOf(CurlUPtr) curlus[_1_]
);

#define url_client_setopt(Uc, Opt, Val) w_curl_easy_setopt((Uc)->curl, Opt, Val)


Err url_client_set_cookies(UrlClient uc[_1_]) {
    if (uc->cookies_fname.len) {
        char* cookiefile = url_client_cookies_fname(uc);
        /* this call leaks in old curl versions */
        try(url_client_setopt(uc, CURLOPT_COOKIEFILE, cookiefile));
        try(url_client_setopt(uc, CURLOPT_COOKIEJAR, cookiefile));
    }
    return Ok;
}


Err url_client_setopt_verbose(UrlClient uc[_1_]) {
    return url_client_setopt(uc, CURLOPT_VERBOSE, url_client_verbose(uc));
}

Err url_client_setopt_error_buffer(UrlClient uc[_1_]) {
    return url_client_setopt(uc, CURLOPT_ERRORBUFFER, uc->errbuf);
}

Err url_client_setopt_user_agent(UrlClient uc[_1_]) {
    return url_client_setopt(uc, CURLOPT_USERAGENT, url_client_user_agent(uc));
}

Err url_client_set_basic_options(UrlClient uc[_1_]) {
    try(url_client_setopt(uc, CURLOPT_NOPROGRESS,     1L));
    try(url_client_setopt(uc, CURLOPT_FOLLOWLOCATION, 1L));

    try(url_client_setopt_verbose(uc));
    try(url_client_setopt_error_buffer(uc));
    try(url_client_setopt_user_agent(uc));

    return url_client_set_cookies(uc);
}


Err url_client_reset(UrlClient url_client[_1_]) {
    curl_easy_reset(url_client->curl);
    return url_client_set_basic_options(url_client);
}


Err url_client_init(
    UrlClient url_client[_1_],
    StrView   cookies_fname,
    StrView   user_agent,
    unsigned  flags
) {
    Err e = Ok;

    *url_client = (UrlClient){
        .cookies_fname = cookies_fname,
        .user_agent   = user_agent,
        .flags        = flags
    };

    url_client->curl = curl_easy_init();
    if (!url_client->curl) return "error: curl_easy_init failure";

    url_client->curlm = curl_multi_init();
    if (!url_client->curlm) {
        e = "error: curl_multi_init failure";
        goto Failure_Curl_Easy_Cleanup;
    }

    try_or_jump(e, Failure_Curl_Multi_Cleanup, url_client_set_basic_options(url_client));

    return Ok;

Failure_Curl_Multi_Cleanup:
    curl_easy_cleanup(url_client->curl);
Failure_Curl_Easy_Cleanup:
    curl_multi_cleanup(url_client->curlm);
    return e;
}


Err url_client_print_cookies(Session* s, UrlClient uc[_1_]) {
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


const char* _parse_opt(const char* line, CURLoption opt[_1_]) {

    const char* rest;
    if ((rest = csubstr_match(line, "noprogress", 1))) { *opt=CURLOPT_NOPROGRESS; return rest; }
    if ((rest = csubstr_match(line, "useragent", 1))) { *opt=CURLOPT_USERAGENT; return rest; }
    if ((rest = csubstr_match(line, "verbose", 1))) { *opt=CURLOPT_VERBOSE; return rest; }
    return NULL;
}


/* static Err _curl_setopt_cstr_(CURL* handle, CURLoption opt, const char* rest) { */
/*     if (!rest) return "not value to setopt, expecting a string"; */
/*     rest = cstr_skip_space(rest); */
/*     if (!*rest) return "invalid opt, whitespace only"; */
/*     CURLcode curl_code = curl_easy_setopt(handle, opt, rest); */
/*     if (CURLE_OK != curl_code) { */
/*         return err_fmt("could not set curlopt: %s", curl_easy_strerror(curl_code)); */
/*     } */
/*     return Ok; */
/* } */


Err cmd_curl_set(CmdParams p[_1_]) {
    CURLoption opt;
    const char* rest = _parse_opt(p->ln, &opt);
    if (!rest) return "invalid curl opt";

    long value = -1;

    switch(opt) {
        case CURLOPT_VERBOSE:
            rest = parse_l(rest, &value);
            if (*cstr_skip_space(rest)) return err_fmt("invalid opt: %s", rest);
            session_set_verbose(p->s, value);
            break;

        /* TODO: user agent and other string should be owned by the UrlClient */
        /* case CURLOPT_USERAGENT: */
        /*     return _curl_setopt_cstr_(handle, opt, rest); */
        default: return "not implemented curlopt";

    }
    return Ok;
}

Err url_client_multi_add_handles( 
    UrlClient        uc[_1_],
    CURLU*           curlu,
    ArlOf(Str)       urls[_1_],
    ArlOf(Str)       scripts[_1_],
    ArlOf(CurlPtr)*  easies,
    ArlOf(CurlUPtr)* curlus,
    Writer           msg_writer[_1_]
) {

    //TODO: do it in one op, of implement reserve/capacity in hotl
    for (size_t i = 0; i < len__(urls); ++i) {
        if (!arlfn(Str,append)(scripts, &(Str){0}))
            return "error: arlfn(Str,append) failure";
    }
    scripts->len -= len__(urls);
    /*^*/

    for (Str* u = arlfn(Str,begin)(urls) ; u != arlfn(Str,end)(urls) ; ++u) {
        Err e = w_curl_multi_add(uc, curlu, items__(u), easies, scripts, curlus);
        if (e) {
            try(writer_write_lit__(msg_writer, "couldn't get script: "));
            try(writer_write(msg_writer, (char*)e, strlen(e)));
        }
    }
    return Ok;
}

void url_client_set_verbose(UrlClient uc[_1_], bool value) {
    set_flag(&uc->flags, URL_CLIENT_FLAG_VERBOSE, value);
}
