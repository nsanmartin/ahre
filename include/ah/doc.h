#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include <ah/textbuf.h>
#include <ah/doc-cache.h>
#include <ah/error.h>
#include <ah/url-client.h>
#include <ah/wrappers.h>


typedef struct UrlClient UrlClient;


typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    TextBuf aebuf;
    DocCache cache;
} Doc;


void doc_reset(Doc* ahdoc) ;
void doc_cleanup(Doc* ahdoc) ;
void doc_destroy(Doc* ahdoc) ;

int doc_init(Doc d[static 1], const Str* url);


Doc* doc_create(char* url);

static inline bool doc_has_url(Doc ad[static 1]) {
    return ad->url != NULL;
}

void doc_update_url(Doc ad[static 1], char* url) ;
ErrStr doc_fetch(UrlClient url_client[static 1], Doc ad[static 1]) ;

#endif
