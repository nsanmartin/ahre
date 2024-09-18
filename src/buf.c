#include <ah/buf.h>

/* static functions */
static int AhBufAppendLinesIndexes(AhBuf ab[static 1], char* data, size_t len);


inline void AhBufCleanup(AhBuf b[static 1]) {
    buffn(char, clean)(&b->buf);
    *b = (AhBuf){.current_line=1};
}


inline void AhBufFree(AhBuf* b) {
    AhBufCleanup(b);
    ah_free(b);
}


inline size_t AhBufLen(AhBuf ab[static 1]) { return ab->buf.len; }

inline size_t AhBufNLines(AhBuf ab[static 1]) {
    return ab->lines_offs.len;
}


inline int AhBufAppend(AhBuf ab[static 1], char* data, size_t len) {
    return AhBufAppendLinesIndexes(ab, data, len)
        || !buffn(char,append)(&ab->buf, (char*)data, len);
}


static int AhBufAppendLinesIndexes(AhBuf ab[static 1], char* data, size_t len) {
    char* end = data + len;
    char* it = data;

    for(;it < end;) {
        it = memchr(it, '\n', end-it);
        if (!it || it >= end) { break; }
        size_t index = AhBufLen(ab) + (it - data);
        if(NULL == arlfn(size_t, append)(&ab->lines_offs, &index)) { return -1; }
        ++it;
    }

    return 0;
}
