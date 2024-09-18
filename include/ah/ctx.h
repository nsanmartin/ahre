#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include <ah/utils.h>
#include <ah/doc.h>
#include <ah/wrappers.h>

typedef struct AhCtx AhCtx;

typedef int (*AhUserLineCallback)(AhCtx* ctx, const char*);


typedef struct AhCtx {
    AhCurl* ahcurl;
    AhDoc* ahdoc;
    bool quit;
    int (*user_line_callback)(AhCtx* ctx, const char*);
} AhCtx;

AhBuf* AhCtxCurrentBuf(AhCtx ctx[static 1]);
AhDoc* AhCtxCurrentDoc(AhCtx ctx[static 1]);

AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;
int ah_ed_cmd_print(AhCtx ctx[static 1]);

int AhCtxBufSummary(AhCtx ctx[static 1]);
#endif
