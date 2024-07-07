#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include <wrappers.h>
#include <mem.h>
#include <aherror.h>


typedef struct AhCtx AhCtx;
typedef struct AhCurl AhCurl;

typedef int (*AhUserLineCallback)(AhCtx* ctx, char*);

typedef struct {
    const char* url;
    lxb_html_document_t* doc;
} AhDoc;


typedef struct AhCtx {
    AhCurl* ahcurl;
    int (*user_line_callback)(AhCtx* ctx, char*);
    AhDoc* ahdoc;
    bool quit;
} AhCtx;


AhCtx* AhCtxCreate(const char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;

AhDoc* AhDocCreate(const char* url);
void AhDocFree(AhDoc* ctx) ;

static inline void AhDocUpdateUrl(AhDoc ad[static 1], const char* url) {
        ah_free((char*)ad->url);
        ad->url = url;
}
ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) ;
const char* ah_urldup(const char* url) ;
#endif
