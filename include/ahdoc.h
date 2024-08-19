#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include <wrappers.h>
#include <mem.h>
//#include <ahctx.h>
#include <aherror.h>
#include <ahdoc-cache.h>


//typedef struct AhCtx AhCtx;
typedef struct AhCurl AhCurl;


typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    BufOf(char) buf;
    AhDocCache cache;
} AhDoc;


static inline void AhDocReset(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_clean(ahdoc->doc);
    ah_free((char*)ahdoc->url);
}

static inline void AhDocCleanup(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_destroy(ahdoc->doc);
    ah_free((char*)ahdoc->url);
}

static inline void AhDocFree(AhDoc* ahdoc) {
    AhDocCleanup(ahdoc);
    ah_free(ahdoc);
}

int AhDocInit(AhDoc d[static 1], char* url);


AhDoc* AhDocCreate(char* url);

static inline void AhDocUpdateUrl(AhDoc ad[static 1], char* url) {
        ah_free((char*)ad->url);
        ad->url = url;
}
ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) ;
char* ah_urldup(char* url) ;

int ahdoc_buffer_summary(AhDoc ahdoc[static 1], BufOf(char)* buf) ;

#endif
