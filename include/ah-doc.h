#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>

#include <lexbor/html/html.h>


typedef struct AhCtx AhCtx;
typedef int (*AhUserLineCallback)(AhCtx* ctx, const char*);

typedef struct AhCtx {
    const char* url;
    int (*user_line_callback)(AhCtx* ctx, const char*);
    lxb_html_document_t* doc;
    //TODO: replace this with fetching in ctor
    bool fetch;
    bool quit;
} AhCtx;

AhCtx* AhCtxMalloc(const char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;
#endif
