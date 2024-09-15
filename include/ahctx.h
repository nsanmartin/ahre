#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include <ahutils.h>
#include <ahdoc.h>
#include <wrappers.h>

typedef struct AhCtx AhCtx;

typedef int (*AhUserLineCallback)(AhCtx* ctx, const char*);


typedef struct AhCtx {
    AhCurl* ahcurl;
    AhDoc* ahdoc;
    bool quit;
    int (*user_line_callback)(AhCtx* ctx, const char*);
} AhCtx;

static inline AeBuf* AhCtxCurrentBuf(AhCtx ctx[static 1]) {
    return &ctx->ahdoc->aebuf;
}

static inline AhDoc* AhCtxCurrentDoc(AhCtx ctx[static 1]) {
    return ctx->ahdoc;
}

static inline AhDoc* ahctx_current_doc(AhCtx ctx[static 1]) { return ctx->ahdoc; }

AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;
int ah_ed_cmd_print(AhCtx ctx[static 1]);

int ahctx_print_summary(AhCtx ctx[static 1], FILE f[static 1]) ;
int ahctx_buffer_summary(AhCtx ctx[static 1]);
#endif
