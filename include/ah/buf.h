#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include <ah/str.h>
#include <ah/utils.h>
#include <ah/mem.h>

typedef struct {
    BufOf(char) buf;
    size_t current_line;
    ArlOf(size_t) eol_idxs;
} AhBuf;

#define AhBufReset AhBufCleanup
void AhBufCleanup(AhBuf b[static 1]);
void AhBufFree(AhBuf* b);
int AhBufAppend(AhBuf ab[static 1], char* data, size_t len);
static inline int AhBufInit(AhBuf ab[static 1]) { *ab = (AhBuf){.current_line=1}; return 0; }
size_t AhBufLen(AhBuf ab[static 1]);


static inline int
AhBufAppendLn(AhBuf ab[static 1], char* data, size_t len) {
    return buffn(char,append)(&ab->buf, (char*)data, len)
        && arlfn(size_t, append)(&ab->lines_offs, &ab->buf.len)
        && buffn(char,append)(&ab->buf, (char*)"\n", 1)
        ? 0 : -1;
}


static inline bool AhBufIsEmpty(AhBuf ab[static 1]) {
    return !ab->buf.len;
}

size_t AhBufNLines(AhBuf ab [static 1]);

#endif
