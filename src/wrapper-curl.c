#include "debug.h"
#include "str.h"
#include "htmldoc.h"
#include "wrapper-curl.h"

/* internal linkage */


/* external linkage */
#define lit_match__(Lit, Mem, Len) (lit_len__(Lit) <= Len && !strncasecmp(Lit, Mem, lit_len__(Lit)))
size_t curl_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) {
    /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
    /* 'userdata' is set with CURLOPT_HEADERDATA */
#define CHARSET_K "charset="
#define CONTENT_TYPE_K "content-type:"

    HtmlDoc* hd = (HtmlDoc*)htmldoc;
    size_t len = size * nitems;
    while (len) {
        if (lit_match__(CHARSET_K, buffer, len)) {
            size_t matchlen =  lit_len__(CHARSET_K);
            buffer += matchlen;
            len -= matchlen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            char* ws = mem_is_whitespace(buffer, len);
            size_t chslen = ws ? (size_t)(ws - buffer) : len;
            str_reset(htmldoc_http_charset(hd));
            str_append(htmldoc_http_charset(hd), buffer, chslen);
            str_append_lit__(htmldoc_http_charset(hd), "\0");
            buffer += chslen;
            len -= chslen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            continue;
        } else if (lit_match__(CONTENT_TYPE_K, buffer, len)) {
            size_t matchlen = lit_len__(CONTENT_TYPE_K);
            buffer += matchlen;
            len -= matchlen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            char* colon = memchr(buffer, ';', len);
            size_t contypelen = colon ? (size_t)(colon - buffer) : len;
            str_reset(htmldoc_http_content_type(hd));
            //TODO: does not depend on null termoination
            str_append(htmldoc_http_content_type(hd), buffer, contypelen);
            if (colon) ++contypelen;
            buffer += contypelen;
            len -= contypelen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            continue;
        } else return size * nitems;
    }

    return size * nitems;
}


Err w_curl_multi_add_handles( 
    CURLM*           multi,
    CURLU*           curlu,
    ArlOf(Str)       urls[static 1],
    ArlOf(Str)       scripts[static 1],
    ArlOf(CurlPtr)*  easies,
    ArlOf(CurlUPtr)* curlus,
    SessionWriteFn   wfnc
) {

    //TODO: do it in one op
    for (size_t i = 0; i < len__(urls); ++i) {
        if (!arlfn(Str,append)(scripts, &(Str){0}))
            return "error: arlfn(Str,append) failure";
    }
    scripts->len -= len__(urls);
    /*^*/

    for (Str* u = arlfn(Str,begin)(urls) ; u != arlfn(Str,end)(urls) ; ++u) {
        Err e = w_curl_multi_add(multi, curlu, items__(u), easies, scripts, curlus);
        if (e) {
            log_warn__(wfnc, "couldn't get script: %s\n", e);
        }
    }
    return Ok;
}

Err w_curl_multi_perform_poll(CURLM* multi){
    int running;
    Err err = Ok;
    do {
        CURLMcode code = curl_multi_perform(multi, &running);
        if (code != CURLM_OK) {
            err = "error: curl_multi_perform failed";
            break;
        }
        code = curl_multi_poll(multi, NULL, 0, 1000, NULL);
        if (code != CURLM_OK) {
            err = err_fmt("error: curl_multi_poll failed: %s", curl_multi_strerror(code));
            break;
        }
    } while (running);
    return err;
}

void
w_curl_multi_remove_handles(CURLM* multi, ArlOf(CurlPtr)  easies[static 1], SessionWriteFn wfnc) {
    for (CurlPtr* cup = arlfn(CurlPtr,begin)(easies) ; cup != arlfn(CurlPtr,end)(easies) ; ++cup) {
        CURLMcode code = curl_multi_remove_handle(multi, *cup);
        if (code != CURLM_OK) {
            log_warn__(
                wfnc, "error: couldn't remove easy handle from multy %s", curl_multi_strerror(code)
            );
        }
        curl_easy_cleanup(*cup);
    }
}

Err w_curl_multi_add(
    CURLM*          multi,
    CURLU*          baseurl,
    const char*     urlstr,
    ArlOf(CurlPtr)  easies[static 1],
    ArlOf(Str)      destlist[static 1],
    ArlOf(CurlUPtr) curlus[static 1]
) {
    Err    e    = Ok;
    CURLU* dup;
    CURL*  easy;

    //TODO!: dont do it this way. Array should had been filed by the caller.
    try( w_curl_url_dup(baseurl, &dup));
    try_or_jump(e, Clean_Dup, w_curl_easy_init(&easy));
    ++destlist->len;
    try_or_jump(e, Clean_Easy, w_curl_url_set(dup, CURLUPART_URL, urlstr, CURLU_DEFAULT_SCHEME));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_CURLU        , dup));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, str_append_flip));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(
        easy, CURLOPT_WRITEDATA , arlfn(Str,back)(destlist)
    ));
    try_or_jump(e, Clean_Easy, w_curl_multi_add_handle(multi, easy));

    if (!arlfn(CurlPtr,append)(easies, &easy)) {
        e = "error: arlfn(CurlPtr,append) failure";
        goto Remove_Easy;
    }


    if (!arlfn(CurlUPtr,append)(curlus, &dup)) {
        e = "error: arlfn(Str,append) failure";
        goto Pop_Easy;
    }

    return e;

Pop_Easy:
    if (len__(easies)) --easies->len;
Remove_Easy:
    curl_multi_remove_handle(multi, easy);
Clean_Easy:
    curl_easy_cleanup(easy);
    if (len__(destlist)) --destlist->len;
Clean_Dup:
    curl_url_cleanup(dup);

    return e;
}
