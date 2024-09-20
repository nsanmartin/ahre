#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include <ah/buf.h>
#include <ah/doc-cache.h>
#include <ah/error.h>
#include <ah/mem.h>
#include <ah/wrappers.h>


typedef struct UrlClient UrlClient;


typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    TextBuf aebuf;
    DocCache cache;
} Doc;


static inline void doc_reset(Doc* ahdoc) {
    doc_cache_cleanup(&ahdoc->cache);
    lxb_html_document_clean(ahdoc->doc);
    textbuf_reset(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

static inline void doc_cleanup(Doc* ahdoc) {
    doc_cache_cleanup(&ahdoc->cache);
    lxb_html_document_destroy(ahdoc->doc);
    textbuf_cleanup(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

static inline void doc_destroy(Doc* ahdoc) {
    doc_cleanup(ahdoc);
    destroy(ahdoc);
}

int doc_init(Doc d[static 1], const Str* url);


Doc* doc_create(char* url);

static inline bool doc_has_url(Doc ad[static 1]) {
    return ad->url != NULL;
}

static inline void doc_update_url(Doc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}
ErrStr doc_fetch(UrlClient ahcurl[static 1], Doc ad[static 1]) ;

#endif
