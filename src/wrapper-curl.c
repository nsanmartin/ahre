#include "str.h"
#include "htmldoc.h"
#include "wrapper-curl.h"

/* internal linkage */

CURLUcode w_curl_get_part(CurlUrlPtr cu, CURLUPart part, char** content, unsigned flags);

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
            str_append(htmldoc_http_charset(hd), sv(buffer, chslen));
            str_append(htmldoc_http_charset(hd), svl("\0"));
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
            str_append(htmldoc_http_content_type(hd), sv(buffer, contypelen));
            if (colon) ++contypelen;
            buffer += contypelen;
            len -= contypelen;
            mem_skip_space_inplace((const char**)&buffer, &len);
            continue;
        } else return size * nitems;
    }

    return size * nitems;
}


Err curlinfo_sz_download_incr(
    CmdOut    cmd_out[_1_],
    CURL*     curl,
    uintmax_t nptr[_1_]
) {
    curl_off_t nbytes;
    CURLcode curl_code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &nbytes);
    if (curl_code!=CURLE_OK) {
        const char* cerr = curl_easy_strerror(curl_code);
        try(cmd_out_msg_append(cmd_out,(char*)cerr)); 
    } else {
        if (nbytes < 0)
            try(cmd_out_msg_append(cmd_out, svl("CURLINFO_SIZE_DOWNLOAD_T is negative")));
        else *nptr += nbytes;
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

Err for_htmldoc_size_download_append(
    ArlOf(CurlPtr) easies[_1_],
    CmdOut         cmd_out[_1_],
    CURL*          curl,
    uintmax_t      out[_1_]
) {
    for (CurlPtr* cup = arlfn(CurlPtr,begin)(easies) ; cup != arlfn(CurlPtr,end)(easies) ; ++cup) {
        try(curlinfo_sz_download_incr(cmd_out, curl, out));
    }
    return Ok;
}


void w_curl_multi_remove_handles(
    CURLM*         multi,
    ArlOf(CurlPtr) easies[_1_],
    CmdOut         cmd_out[_1_]
) {
    for (CurlPtr* cup = arlfn(CurlPtr,begin)(easies) ; cup != arlfn(CurlPtr,end)(easies) ; ++cup) {
        CURLMcode code = curl_multi_remove_handle(multi, *cup);
        if (code != CURLM_OK) {
            /*ignore e*/cmd_out_msg_append(
                    cmd_out, svl("error: couldn't remove easy handle from multy "));
            char* msg = (char*)curl_multi_strerror(code); 
            /*ignore e*/cmd_out_msg_append(cmd_out, msg);
        }
        curl_easy_cleanup(*cup);
    }
}

Err w_curl_multi_add(
    UrlClient       uc[_1_],
    CURLU*          baseurl,
    const char*     urlstr,
    ArlOf(CurlPtr)  easies[_1_],
    ArlOf(Str)      destlist[_1_],
    ArlOf(CurlUrlPtr) curlus[_1_]
) {
    Err    e    = Ok;
    CURLU* dup;
    CURL*  easy;
    CURLM* multi = url_client_multi(uc);

    //TODO!: dont do it this way. Array should had been filed by the caller.
    try( w_curl_url_dup(baseurl, &dup));
    try_or_jump(e, Clean_Dup, w_curl_easy_init(&easy));
    ++destlist->len;
    try_or_jump(e, Clean_Easy, w_curl_url_set(dup, CURLUPART_URL, urlstr, CURLU_DEFAULT_SCHEME));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_CURLU        , dup));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, str_append_flip));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt( easy, CURLOPT_WRITEDATA , arlfn(Str,back)(destlist)));

    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_VERBOSE, url_client_verbose(uc)));
    try_or_jump(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_USERAGENT, url_client_user_agent(uc)));

    //TODO: use CURLOPT_SHARE to share cookies etc between handles.
    //https://everything.curl.dev/helpers/sharing.html

    try_or_jump(e, Clean_Easy, w_curl_multi_add_handle(multi, easy));

    if (!arlfn(CurlPtr,append)(easies, &easy)) {
        e = "error: arlfn(CurlPtr,append) failure";
        goto Remove_Easy;
    }


    if (!arlfn(CurlUrlPtr,append)(curlus, &dup)) {
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



CURLcode
w_curl_global_init() { return curl_global_init(CURL_GLOBAL_DEFAULT); }

void
w_curl_global_cleanup() { curl_global_cleanup(); }

Err
w_curl_url_get_malloc(CURLU* cu, CURLUPart part, char* out[_1_]) {
/*
 * Get cu's part. The caller should w_curl_free out.
 */
    CURLUcode code = w_curl_get_part(cu, part, out, 0);
    if (code != CURLUE_OK)
        return err_fmt("error getting url from CURLU: %s", curl_url_strerror(code));
    if (!*out) return "error: curl_url_get returned NULL wioth no error";
    return Ok;
}

void
w_curl_free(void* p) { curl_free(p); } 


CURLUcode w_curl_get_part(CURLU* cu, CURLUPart part, char** content, unsigned flags) {
    return curl_url_get(cu, part, content, flags);
}


Err w_curl_url_set(CURLU* u,  CURLUPart part, const char* cstr, unsigned flags) {
    if (!cstr || !*cstr) return "error: no contents for CURLUPart";
    CURLUcode code = curl_url_set(u, part, cstr, flags);
    return code == CURLUE_OK 
        ? Ok : err_fmt("error setting url with '%s': %s", cstr, curl_url_strerror(code));
}

