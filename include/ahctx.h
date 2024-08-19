#ifndef __AHRE_Ctx_H__
#define __AHRE_Ctx_H__

#include <ahutils.h>
#include <ahdoc.h>
#include <wrappers.h>

typedef struct AhCtx AhCtx;
//typedef struct AhDoc AhDoc;
typedef int (*AhUserLineCallback)(AhCtx* ctx, char*);


typedef struct AhCtx {
    AhCurl* ahcurl;
    AhDoc* ahdoc;
    BufOf(char) buf;
    bool quit;
    int (*user_line_callback)(AhCtx* ctx, char*);
} AhCtx;

AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;
int ah_ed_cmd_print(AhCtx ctx[static 1]);

int ahctx_print_summary(AhCtx ctx[static 1], FILE f[static 1]) ;
int ahctx_buffer_summary(AhCtx ctx[static 1], BufOf(char)*buf) ;
#endif
