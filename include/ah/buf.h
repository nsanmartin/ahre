#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include <ah/str.h>
#include <ah/utils.h>
#include <ah/mem.h>

typedef struct {
    BufOf(char) buf;
    size_t current_line;
    ArlOf(size_t) lines_offs;
} AeBuf;

#define AeBufReset AeBufCleanup
void AeBufCleanup(AeBuf* b);
void AeBufFree(AeBuf* b);
int AeBufAppend(AeBuf ab[static 1], char* data, size_t len);
static inline int AhBufInit(AeBuf ab[static 1]) { *ab = (AeBuf){.current_line=1}; return 0; }
size_t AhBufLen(AeBuf ab[static 1]);


static inline int
AeBufAppendLn(AeBuf ab[static 1], char* data, size_t len) {
    return buffn(char,append)(&ab->buf, (char*)data, len)
        && arlfn(size_t, append)(&ab->lines_offs, &ab->buf.len)
        && buffn(char,append)(&ab->buf, (char*)"\n", 1)
        ? 0 : -1;
}


static inline bool AeBufIsEmpty(AeBuf ab[static 1]) {
    return !ab->buf.len;
}

size_t AeBufNLines(AeBuf ab [static 1]);

#endif
