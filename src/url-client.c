#include "ahre.h"
#include "generic.h"
#include "mem.h"
#include "session.h"
#include "url-client.h"
#include "cmd.h"
#include "error.h"


#define url_client_setopt(Uc, Opt, Val) w_curl_easy_setopt((Uc)->curl, Opt, Val)


Err url_client_set_basic_options_to_handle(UrlClient uc[_1_], CurlPtr handle) {
    return w_curl_set_basic_options(
        handle, 
        url_client_verbose(uc),
        url_client_errbuf(uc),
        url_client_user_agent(uc),
        url_client_cookies_fname(uc)
    );
}


Err url_client_init(
    UrlClient url_client[_1_],
    StrView   cookies_fname,
    StrView   user_agent,
    unsigned  flags
) {
    *url_client = (UrlClient){
        .cookies_fname = cookies_fname,
        .user_agent   = user_agent,
        .flags        = flags
    };

    url_client->curlm = curl_multi_init();
    if (!url_client->curlm) return err_internal("curl_multi_init failure");

    return Ok;
}


Err url_client_print_cookies(Session* s, UrlClient uc[_1_], CmdOut* out) {
    if (!s) return "error: session is null";
    CurlPtr            curl;
    struct curl_slist* cookies = NULL;
    Err                e       = Ok;
    try(w_curl_easy_init(&curl));
    tryjmp(e,Clean, url_client_set_basic_options_to_handle(uc, curl));
    CURLcode curl_code = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
    if (curl_code != CURLE_OK) { e="error: could not retrieve cookies list"; goto Clean; }
    if (!cookies) { msg_ln__(out, svl("No cookies")); e="no cookies"; goto Clean; }

    struct curl_slist* it = cookies;
    while (it) {
        tryjmp(e, Clean, msg_ln__(out, cast__(const char*)it->data));
        it = it->next;
    }
Clean:
    curl_slist_free_all(cookies);
    curl_easy_cleanup(curl);
    return Ok;
}


static const char* _parse_opt(CmdParams p[_1_], CURLoption opt[_1_]) {

    const char* rest;
    if ((rest = cmd_params_match(p, "noprogress", 1))) { *opt=CURLOPT_NOPROGRESS; return rest; }
    if ((rest = cmd_params_match(p, "useragent", 1))) { *opt=CURLOPT_USERAGENT; return rest; }
    if ((rest = cmd_params_match(p, "verbose", 1))) { *opt=CURLOPT_VERBOSE; return rest; }
    return NULL;
}


Err cmd_curl_set(CmdParams p[_1_]) {
    CURLoption opt;
    const char* rest = _parse_opt(p, &opt);
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
    ArlOf(CurlUrlPtr)* curlus,
    CmdOut           cmd_out[_1_]
) {

    if (!len__(urls)) return Ok;
    //TODO1: this is awful, improve this fn and w_curl_multi_add as well
    for (size_t i = 0; i < len__(urls); ++i) {
        if (!arlfn(Str,append)(scripts, &(Str){0}))
            return "error: arlfn(Str,append) failure";
    }
    scripts->len -= len__(urls);

    for (Str* u = arlfn(Str,begin)(urls) ; u != arlfn(Str,end)(urls) ; ++u) {
        Err e = w_curl_multi_add(uc, curlu, items__(u), easies, scripts, curlus);
        if (e) {
            try(msg__(cmd_out, svl("couldn't get script: ")));
            try(msg__(cmd_out, e));
        }
    }
    return Ok;
}

void url_client_set_verbose(UrlClient uc[_1_], bool value) {
    set_flag(&uc->flags, URL_CLIENT_FLAG_VERBOSE, value);
}


Err url_client_perform_with_cancel(UrlClient uc[_1_], CurlPtr easy, Request req[_1_]) {
    Err e = Ok;
    Str url = (Str){0};
    tryjmp(e,Clean, curl_url_to_filename_append(*request_url(req), &url));
    tryjmp(e,Clean, w_curl_perform_with_cancel(url_client_multi(uc), easy, url.items));
Clean:
    str_clean(&url);
    return e;
}

