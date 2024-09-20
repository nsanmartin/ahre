#include <ah/mem.h>

#include <ah/textbuf.h>

/*  internal linkage */
static int textbuf_append_line_indexes(TextBuf ab[static 1], char* data, size_t len);

static int textbuf_append_line_indexes(TextBuf ab[static 1], char* data, size_t len) {
    char* end = data + len;
    char* it = data;

    for(;it < end;) {
        it = memchr(it, '\n', end-it);
        if (!it || it >= end) { break; }
        size_t index = textbuf_len(ab) + (it - data);
        if(NULL == arlfn(size_t, append)(&ab->eols, &index)) { return -1; }
        ++it;
    }

    return 0;
}

/* external linkage  */

inline void textbuf_cleanup(TextBuf b[static 1]) {
    buffn(char, clean)(&b->buf);
    *b = (TextBuf){.current_line=1};
}


inline void textbuf_destroy(TextBuf* b) {
    textbuf_cleanup(b);
    std_free(b);
}


inline size_t textbuf_len(TextBuf ab[static 1]) { return ab->buf.len; }

inline size_t textbuf_line_count(TextBuf ab[static 1]) {
    return ab->eols.len;
}


inline int textbuf_append(TextBuf ab[static 1], char* data, size_t len) {
    return textbuf_append_line_indexes(ab, data, len)
        || !buffn(char,append)(&ab->buf, (char*)data, len);
}

