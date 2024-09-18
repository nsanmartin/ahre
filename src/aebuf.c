#include <ah/buf.h>

inline void AeBufCleanup(AeBuf* b) {
    buffn(char, clean)(&b->buf);
    *b = (AeBuf){.current_line=1};
}


inline void AeBufFree(AeBuf* b) {
    AeBufCleanup(b);
    ah_free(b);
}


inline size_t AeBufNLines(AeBuf ab[static 1]) {
    return ab->lines_offs.len;
}


inline int AeBufAppend(AeBuf ab[static 1], char* data, size_t len) {
    size_t base_off = ab->buf.len;
    if (str_get_lines_offs(data, base_off, len, &ab->lines_offs)) { return -1; }
    return buffn(char,append)(&ab->buf, (char*)data, len) == NULL;
}

