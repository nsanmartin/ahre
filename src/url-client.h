#ifndef __AHRE_AHCURL_H__
#define __AHRE_AHCURL_H__

#include <curl/curl.h>

#include "ahre.h"
#include "utils.h"
#include "writer.h"
#include "wrapper-curl.h"


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


#define URL_CLIENT_FLAG_VERBOSE 0x1
typedef struct UrlClient {
    CURL*     curl;
    CURLM*    curlm;
    char      errbuf[CURL_ERROR_SIZE];
    Str       postdata;

    StrView   cookies_fname;
    StrView   user_agent;
    unsigned  flags;
} UrlClient;

static inline CURL* url_client_curl(UrlClient url_client[_1_]) { return url_client->curl; }
static inline CURLM* url_client_multi(UrlClient url_client[_1_]) { return url_client->curlm; }

static inline Str* url_client_postdata(UrlClient uc[_1_]) { return &uc->postdata; }

static inline char* url_client_cookies_fname(UrlClient uc[_1_]) { return (char*)uc->cookies_fname.items; }
static inline char* url_client_user_agent(UrlClient uc[_1_]) { return (char*)uc->user_agent.items; }
static inline bool url_client_verbose(UrlClient uc[_1_]) { return uc->flags & URL_CLIENT_FLAG_VERBOSE; }

void url_client_set_verbose(UrlClient uc[_1_], bool value);
/* ctor */
Err url_client_init(
    UrlClient url_client[_1_],
    StrView   cookies_fname,
    StrView   user_agent,
    unsigned  flags
);
/* dtor */


/* curl easy escape */
static inline char* url_client_escape_url(UrlClient url_client[_1_], const char* u, int len) {
    return curl_easy_escape(url_client->curl, u, len);
}

static inline void url_client_cleanup(UrlClient* url_client) {
    curl_multi_cleanup(url_client->curlm);
    curl_easy_cleanup(url_client->curl);
    str_clean(url_client_postdata(url_client));
}

typedef struct Session Session;
static inline void url_client_curl_free_cstr(char* s) { curl_free(s); }
Err url_client_print_cookies(Session* s, UrlClient uc[_1_]) ;
Err url_client_reset(UrlClient url_client[_1_]);
Err url_client_set_basic_options(UrlClient url_client[_1_]);

Err url_client_multi_add_handles( 
    UrlClient        uc[_1_],
    CURLU*           curlu,
    ArlOf(Str)       urls[_1_],
    ArlOf(Str)       scripts[_1_],
    ArlOf(CurlPtr)*  easies,
    ArlOf(CurlUPtr)* curlus,
    Writer           msg_writer[_1_]
);
#endif
