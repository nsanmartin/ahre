#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <ahutils.h>
#include <mem.h>

typedef struct {
    BufOf(char) buf;
    size_t current_line;
    size_t nlines;
} AeBuf;

static inline void AeBufCleanup(AeBuf* b) {
    buffn(char, clean)(&b->buf);
    *b = (AeBuf){0};
}

static inline void AeBufFree(AeBuf* b) {
    AeBufCleanup(b);
    ah_free(b);
}

static inline size_t
str_count_ocurrencies(char* data, size_t len, char c) {
    //TODO: consider using strchr
    size_t count = 0;
    for (size_t i = 0; i < len; ++i) {
        count += data[i] == c ? 1 : 0;
    }
    return count;
}

static inline int
AeBufAppend(AeBuf ab[static 1], char* data, size_t len) {
    ab->nlines += str_count_ocurrencies(data, len, '\n');
    return buffn(char,append)(&ab->buf, (char*)data, len);
}
#define AeBufReset AeBufCleanup
#endif
