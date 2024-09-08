#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include <ahutils.h>
#include <mem.h>

typedef struct {
    BufOf(char) buf;
    size_t current_line;
    ArlOf(size_t) lines_offs;
} AeBuf;

static inline void AeBufCleanup(AeBuf* b) {
    buffn(char, clean)(&b->buf);
    *b = (AeBuf){.current_line=1};
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
str_get_lines_offs(char* data, size_t len, ArlOf(size_t)* ptrs) {
    char* end = data + len;
    char* it = data;

    for(;;) {
        it = memchr(it, '\n', end-data);
        if (!it || it >= end) { break; }
        size_t off = it - data;
        if(arlfn(size_t, append)(ptrs, &off)) { return -1; }
        ++it;
    }

    return 0;
}


static inline int
AeBufAppend(AeBuf ab[static 1], char* data, size_t len) {
    if (buffn(char,append)(&ab->buf, (char*)data, len)) { return -1; }
    char* beg = ab->buf.items + ab->buf.len - len;
    if (str_get_lines_offs(beg, len, &ab->lines_offs)) { return -1; }
    return 0;
}

static inline int
AeBufAppendLn(AeBuf ab[static 1], char* data, size_t len) {
    if (buffn(char,append)(&ab->buf, (char*)data, len)) { return -1; }
    if(arlfn(size_t, append)(&ab->lines_offs, &ab->buf.len)) { return -1; }
    if (buffn(char,append)(&ab->buf, (char*)"\n", 1)) { return -1; }
    return 0;
}
#define AeBufReset AeBufCleanup

static inline bool AeBufIsEmpty(AeBuf ab[static 1]) {
    return !ab->buf.len;
}

static inline size_t AeBufNLines(AeBuf ab [static 1]) {
    return ab->lines_offs.len;
}

#endif
