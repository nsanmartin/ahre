#ifndef REQUEST_AHRE_H__
#define REQUEST_AHRE_H__

#include "url.h"
#include "sys.h"


typedef struct {
    HttpMethod method;
    Url*       urlview;
    Url        url;
    Str        urlstr;
    ArlOf(Str) keys;
    ArlOf(Str) values;
    Str        postfields; /* used for post field and quey */
    bool       local;
} Request;

bool request_is_local(Request r[_1_]);
void request_set_local(Request r[_1_], bool value);
static inline HttpMethod request_method(Request r[_1_]) { return r->method; }
static inline Str* request_urlstr(Request r[_1_]) { return &r->urlstr; }
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

/* ctors */
Err request_init(Request r[_1_], HttpMethod method, StrView urlstr, Url* url);
Err request_from_form_node (Request r[_1_], DomNode form, bool is_https, Url* urlview);
Err request_from_cli_params(Request r[_1_], HttpMethod method, StrView urlstr, StrView postfields);
Err request_to_handle(
    Request     r[_1_],
    UrlClient   url_client[_1_],
    const char* path,
    FilePtr     fpp[_1_],
    Str         actual_path[_1_],
    CurlPtr     out[_1_]
);
Err url_client_perform_with_cancel(UrlClient uc[_1_], CurlPtr easy, Request req[_1_]);
Err set_post_fields(Request r[_1_], CurlPtr curl);
#endif
