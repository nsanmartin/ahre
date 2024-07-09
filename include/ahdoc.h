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

typedef struct { char* data; size_t len; } AhBuf;

typedef struct { AhBuf* bufs; size_t len; } AhBufLst;

typedef struct {
    const char* url;
    lxb_html_document_t* doc;
    AhBuf buf;
} AhDoc;


typedef struct AhCtx {
    AhCurl* ahcurl;
    int (*user_line_callback)(AhCtx* ctx, char*);
    AhDoc* ahdoc;
    bool quit;
} AhCtx;


AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback);
void AhCtxFree(AhCtx* ctx) ;

AhDoc* AhDocCreate(char* url);
void AhDocFree(AhDoc* ctx) ;

static inline void AhDocUpdateUrl(AhDoc ad[static 1], char* url) {
        ah_free((char*)ad->url);
        ad->url = url;
}
ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) ;
char* ah_urldup(char* url) ;

// static inline int buflst_append_buffer(BufLst* bl) { }

int ah_ed_cmd_print(AhCtx ctx[static 1]);

static inline int AhBufInit(AhBuf b[static 1], size_t len) {
    *b = (AhBuf) {.data=ah_malloc(len), .len=len};
    if (!b->data) { return -1; }
    return 0;
}

static inline int AhBufAppend(AhBuf b[static 1], const char* data, size_t len) {
    b->data = realloc(b->data, b->len + len);
    if (!b->data) { return -1; }
    memcpy(b->data + b->len, data, len);
    b->len += len;
    return 0;
}
#endif
