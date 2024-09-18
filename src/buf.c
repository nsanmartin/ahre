#include <ah/buf.h>

/* static functions */
static int AhBufAppendLinesIndexes(AeBuf ab[static 1], char* data, size_t len);


inline void AeBufCleanup(AeBuf* b) {
    buffn(char, clean)(&b->buf);
    *b = (AeBuf){.current_line=1};
}


inline void AeBufFree(AeBuf* b) {
    AeBufCleanup(b);
    ah_free(b);
}


inline size_t AhBufLen(AeBuf ab[static 1]) { return ab->buf.len; }

inline size_t AeBufNLines(AeBuf ab[static 1]) {
    return ab->lines_offs.len;
}


inline int AeBufAppend(AeBuf ab[static 1], char* data, size_t len) {

    //if (str_get_indexes(data, len, offset, '\n', &ab->lines_offs)) { return -1; }
    if (AhBufAppendLinesIndexes(ab, data, len)) { return -1; }
    return buffn(char,append)(&ab->buf, (char*)data, len) == NULL;
}

static int AhBufAppendLinesIndexes(AeBuf ab[static 1], char* data, size_t len) {
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
