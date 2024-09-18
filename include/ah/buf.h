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

void AeBufCleanup(AeBuf* b);
void AeBufFree(AeBuf* b);
int AeBufAppend(AeBuf ab[static 1], char* data, size_t len);



static inline int
AeBufAppendLn(AeBuf ab[static 1], char* data, size_t len) {
    if (!buffn(char,append)(&ab->buf, (char*)data, len)) { return -1; }
    if (!arlfn(size_t, append)(&ab->lines_offs, &ab->buf.len)) { return -1; }
    if (!buffn(char,append)(&ab->buf, (char*)"\n", 1)) { return -1; }
    return 0;
}
#define AeBufReset AeBufCleanup

static inline bool AeBufIsEmpty(AeBuf ab[static 1]) {
    return !ab->buf.len;
}

//static inline size_t AeBufNLines(AeBuf ab [static 1]) { return ab->lines_offs.len; }
size_t AeBufNLines(AeBuf ab [static 1]);

#endif
