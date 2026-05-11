#ifndef REQUEST_AHRE_H__
#define REQUEST_AHRE_H__

#include "url.h"


typedef struct {
    HttpMethod method;
    Url*       urlview;
    Url        url;
    StrZ       urlstr;
    ArlOf(Str) keys;
    ArlOf(Str) values;
    Str        postfields; /* used for post field and quey */
} Request;


static inline HttpMethod request_method(Request r[_1_]) { return r->method; }
static inline StrZ* request_urlstr(Request r[_1_]) { return &r->urlstr; }
static inline Url* request_url(Request r[_1_]) { return &r->url; }
static inline Str* request_postfields(Request r[_1_]) { return &r->postfields; }
static inline ArlOf(Str)* request_query_keys(Request r[_1_]) { return &r->keys; }
static inline ArlOf(Str)* request_query_values(Request r[_1_]) { return &r->values; }
static inline void request_clean(Request r[_1_]) {
    str_clean(request_urlstr(r));
    str_clean(request_postfields(r));
    arlfn(Str,clean)(request_query_keys(r));
    arlfn(Str,clean)(request_query_values(r));
    url_cleanup(request_url(r));
}

#define T Request
#define TClean request_clean
#include <arl.h>

Err request_from_userln(Request r[_1_], const char* userln, HttpMethod method);
Err request_to_file(Request r[_1_], UrlClient url_client[_1_], FILE* fp);

Err request_query_append_key_value(Request r[_1_], const char*k, size_t klen, const char* v, size_t vlen);

Err url_from_get_request(Request r[_1_]);
Err url_from_request(Request r[_1_], UrlClient* uc);


static inline Err
request_init_move_urlstr(Request r[_1_], HttpMethod method, StrZ urlstr[_1_], Url* url) {
    (void)url;
    *r = (Request) {
        .method=method,
        .urlstr=*urlstr,
        .urlview=url
    };
    return Ok;
}


Err mk_submit_request (DomNode form, bool is_https, Request r[_1_]);
#endif
