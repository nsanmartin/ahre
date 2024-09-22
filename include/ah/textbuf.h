#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include <ah/error.h>
#include <ah/str.h>
#include <ah/utils.h>


typedef struct {
    BufOf(char) buf;
    size_t current_line;
    ArlOf(size_t) eols;
} TextBuf;

#define textbuf_reset textbuf_cleanup
void textbuf_cleanup(TextBuf b[static 1]);
void textbuf_destroy(TextBuf* b);
static inline int textbuf_init(TextBuf ab[static 1]) { *ab = (TextBuf){.current_line=1}; return 0; }

Err textbuf_append(TextBuf ab[static 1], char* data, size_t len);
size_t textbuf_len(TextBuf ab[static 1]);
char* textbuf_items(TextBuf ab[static 1]);

size_t * textbuf_eol_at(TextBuf tb[static 1], size_t i);
size_t textbuf_line_count(TextBuf textbuf [static 1]);
size_t textbuf_eol_count(TextBuf textbuf[static 1]);

static inline Err
textbuf_append_line(TextBuf ab[static 1], char* data, size_t len) {
    return buffn(char,append)(&ab->buf, (char*)data, len)
        && arlfn(size_t, append)(&ab->eols, &ab->buf.len)
        && buffn(char,append)(&ab->buf, (char*)"\n", 1)
        ? Ok
        : __func__
        ;
}


static inline bool textbuf_is_empty(TextBuf ab[static 1]) {
    return !ab->buf.len;
}

#endif
