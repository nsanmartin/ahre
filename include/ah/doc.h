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


typedef struct AhCurl AhCurl;


typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    TextBuf aebuf;
    AhDocCache cache;
} AhDoc;


static inline void AhDocReset(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_clean(ahdoc->doc);
    textbuf_reset(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

static inline void AhDocCleanup(AhDoc* ahdoc) {
    AhDocCacheCleanup(&ahdoc->cache);
    lxb_html_document_destroy(ahdoc->doc);
    textbuf_cleanup(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

static inline void AhDocFree(AhDoc* ahdoc) {
    AhDocCleanup(ahdoc);
    destroy(ahdoc);
}

int AhDocInit(AhDoc d[static 1], const Str* url);


AhDoc* AhDocCreate(char* url);

static inline bool AhDocHasUrl(AhDoc ad[static 1]) {
    return ad->url != NULL;
}

static inline void AhDocUpdateUrl(AhDoc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}
ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) ;

int AhDocBufSummary(AhDoc ahdoc[static 1], BufOf(char)* buf) ;

#endif
