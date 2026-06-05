#include "sys.h"
#include "str.h"
#include "htmldoc.h"
#include "wrapper-curl.h"
#include "generic.h"

/* internal linkage */

CURLUcode w_curl_get_part(CurlUrlPtr cu, CURLUPart part, char** content, unsigned flags);

/* external linkage */
size_t skip_header_callback(char *buffer, size_t size, size_t nitems, void *htmldoc) {
    (void) buffer;
    (void) htmldoc;
    return size * nitems;
}

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
            str_append_z(htmldoc_http_charset(hd), sv(buffer, chslen));
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
            StrView content_type = sv(buffer, contypelen);
            while (content_type.len && isspace(content_type.items[content_type.len-1]))
                --content_type.len;
            str_append(htmldoc_http_content_type(hd), content_type);
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
        try(msg__(cmd_out,(char*)cerr)); 
    } else {
        if (nbytes < 0)
            try(msg__(cmd_out, svl("CURLINFO_SIZE_DOWNLOAD_T is negative")));
        else *nptr += nbytes;
    }
    return Ok;
}

static char curl_perform_canceled_by_user[] = { "curl perform canceled by user\n" };

Err
w_curl_multi_perform_poll(CURLM* multi, ArlOf(CurlMultiSgPtr) failed[_1_]) {
    struct sigaction interrupt_action = get_interrupt_action();
    struct sigaction old_action = {0};
    sigaction(SIGINT,&interrupt_action, &old_action);

    int running;
    Err err = Ok;
    do {
        if (interrupt_flag()) {
            err = curl_perform_canceled_by_user;
            break;
        }

        CURLMcode code = curl_multi_perform(multi, &running);
        if (code != CURLM_OK) {
            err = err_fmt("curl_multi_perform failed: %s", curl_multi_strerror(code));
            break;
        }
        code = curl_multi_poll(multi, NULL, 0, 100, NULL);
        if (code != CURLM_OK) {
            err = err_fmt("curl_multi_poll failed: %s", curl_multi_strerror(code));
            break;
        }

        CurlMultiSgPtr msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(multi, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result != CURLE_OK) {
                    if (!arlfn(CurlMultiSgPtr, append)(failed,&msg))
                    {
                        err = err_internal("while processing curl failure, arl append failed");
                        goto Clean;
                    }
                }
            }
        }
    } while (running);

Clean:
    sigaction(SIGINT, &old_action, NULL);
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
            /*ignore e*/msg__(
                    cmd_out, svl("error: couldn't remove easy handle from multy "));
            char* msg = (char*)curl_multi_strerror(code); 
            /*ignore e*/msg__(cmd_out, msg);
        }
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
    tryjmp(e, Clean_Dup, w_curl_easy_init(&easy));
    ++destlist->len;
    tryjmp(e, Clean_Easy, w_curl_url_set(dup, CURLUPART_URL, urlstr, CURLU_DEFAULT_SCHEME));
    tryjmp(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_CURLU        , dup));
    tryjmp(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, str_append_flip));
    tryjmp(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_WRITEDATA , arlfn(Str,back)(destlist)));

    tryjmp(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_VERBOSE, url_client_verbose(uc)));
    tryjmp(e, Clean_Easy, w_curl_easy_setopt(easy, CURLOPT_USERAGENT, url_client_user_agent(uc)));

    //TODO: use CURLOPT_SHARE to share cookies etc between handles.
    //https://everything.curl.dev/helpers/sharing.html

    tryjmp(e, Clean_Easy, w_curl_multi_add_handle(multi, easy));

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
        return err_fmt("warn: getting url from CURLU: %s", curl_url_strerror(code));
    if (!*out) return "error: curl_url_get returned NULL with no error";
    return Ok;
}

void
w_curl_free(void* p) { if (p) curl_free(p); } 


CURLUcode w_curl_get_part(CURLU* cu, CURLUPart part, char** content, unsigned flags) {
    return curl_url_get(cu, part, content, flags);
}


Err w_curl_url_set(CURLU* u,  CURLUPart part, const char* cstr, unsigned flags) {
    if (!cstr || !*cstr) return "error: no contents for CURLUPart";
    CURLUcode code = curl_url_set(u, part, cstr, flags);
    return code == CURLUE_OK 
        ? Ok : err_fmt("warn: setting url with '%s': %s", cstr, curl_url_strerror(code));
}


Err w_curl_perform_with_cancel(CurlMuliPtr multi, CurlPtr easy, char* url) {
    ArlOf(CurlMultiSgPtr)* failed = &(ArlOf(CurlMultiSgPtr)){0};
    try(w_curl_multi_add_handle(multi, easy));
    Err err = w_curl_multi_perform_poll(multi, failed);

    CurlMultiSgPtr* f = arlfn(CurlMultiSgPtr,at)(failed,0);
    if (f) err = err_fmt("curl failed to fetch %s: %s\n", url, curl_easy_strerror((*f)->data.result));

    CURLMcode code = curl_multi_remove_handle(multi, easy);
    if (code != CURLM_OK) err = err_fmt("error: curl multi remove habdle failure: %s", curl_multi_strerror(code));
    arlfn(CurlMultiSgPtr,clean)(failed);
    return err;
}

Err
w_curl_set_basic_options(
    CurlPtr handle[_1_],
    long    verbose,
    char*   errbuf,
    char*   user_agent,
    char*   cookiefile
) {
    try(w_curl_easy_setopt(handle, CURLOPT_NOPROGRESS,     1L));
    try(w_curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L));

    try(w_curl_easy_setopt(handle, CURLOPT_VERBOSE, verbose));
    try(w_curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf));
    try(w_curl_easy_setopt(handle, CURLOPT_USERAGENT, user_agent));

    if (cookiefile) {
        try(w_curl_easy_setopt(handle, CURLOPT_COOKIEFILE, cookiefile));
        try(w_curl_easy_setopt(handle, CURLOPT_COOKIEJAR, cookiefile));
    }
    return Ok;
}


Err w_curl_set_write_fn_and_data_for_download(CurlPtr curl, FILE* fp) {
    if (0
    || curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL)
    || curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, skip_header_callback)
    || curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite)
    || curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp))
        return err_internal("configuring curl write fn/data");
    return Ok;
}


Err w_curl_set_method_from_http_method(CurlPtr handle, HttpMethod m) {
    CURLoption method = curlopt_method_from_http_method(m);
    if (curl_easy_setopt(handle, method, 1L)) 
        return err_internal("curl failed to set method");
    return Ok;
}

Err
w_curl_get_effective_url(CurlPtr curl, char* effective_url[1]) {
    if (CURLE_OK != curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, effective_url))
        fail_e("couldn't get effective url from curl");
    return Ok;
}
