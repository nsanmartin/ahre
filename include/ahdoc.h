#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>

#include <lexbor/html/html.h>

#include <wrappers.h>


typedef struct AhCtx AhCtx;
typedef struct AhCurl AhCurl;

typedef int (*AhUserLineCallback)(AhCtx* ctx, const char*);

typedef struct {
    const char* url;
    lxb_html_document_t* doc;
} AhDoc;


typedef struct AhCtx {
    AhCurl* ahcurl;
    int (*user_line_callback)(AhCtx* ctx, const char*);
    AhDoc* ahdoc;
    bool quit;
} AhCtx;


AhCtx* AhCtxCreate(const char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;

AhDoc* AhDocCreate(const char* url);
void AhDocFree(AhDoc* ctx) ;
#endif
