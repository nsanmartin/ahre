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
    char* end = data + len;
    size_t count = 0;

    for(;;) {
        data = memchr(data, c, end-data);
        if (!data || data >= end) { break; }
        ++count;
        ++data;
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
