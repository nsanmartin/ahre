#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include <ah/wrappers.h>
#include <ah/mem.h>

#include <ah/error.h>
#include <ah/doc-cache.h>
#include <ah/buf.h>


//typedef struct AhCtx AhCtx;
typedef struct AhCurl AhCurl;


typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    AhBuf aebuf;
    AhDocCache cache;
} AhDoc;


static inline void AhDocReset(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_clean(ahdoc->doc);
    AhBufReset(&ahdoc->aebuf);
    ah_free((char*)ahdoc->url);
}

static inline void AhDocCleanup(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_destroy(ahdoc->doc);
    AhBufCleanup(&ahdoc->aebuf);
    ah_free((char*)ahdoc->url);
}

static inline void AhDocFree(AhDoc* ahdoc) {
    AhDocCleanup(ahdoc);
    ah_free(ahdoc);
}

int AhDocInit(AhDoc d[static 1], const Str* url);


AhDoc* AhDocCreate(char* url);

static inline bool AhDocHasUrl(AhDoc ad[static 1]) {
    return ad->url != NULL;
}

static inline void AhDocUpdateUrl(AhDoc ad[static 1], char* url) {
        ah_free((char*)ad->url);
        ad->url = url;
}
ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) ;
//char* ah_urldup(Str* url) ;

int ahdoc_buffer_summary(AhDoc ahdoc[static 1], BufOf(char)* buf) ;

#endif
