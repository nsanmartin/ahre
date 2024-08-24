#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <ahutils.h>
#include <mem.h>

typedef struct {
    BufOf(char) buf;
    size_t current_line;
} AeBuf;

static inline void AeBufCleanup(AeBuf* b) {
    buffn(char, clean)(&b->buf);
    *b = (AeBuf){0};
}

static inline void AeBufFree(AeBuf* b) {
    AeBufCleanup(b);
    ah_free(b);
}

#define AeBufReset AeBufCleanup
#endif
