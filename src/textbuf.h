#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include "src/error.h"
#include "src/str.h"
#include "src/utils.h"


typedef struct {
    BufOf(char) buf;
    size_t current_line;
    ArlOf(size_t) eols;
} TextBuf;


/* getters */
static inline BufOf(char)*
textbuf_buf(TextBuf t[static 1]) { return &t->buf; }

/* ctor */
static inline int textbuf_init(TextBuf ab[static 1]) { *ab = (TextBuf){.current_line=1}; return 0; }

/* dtors */
void textbuf_cleanup(TextBuf b[static 1]);
void textbuf_reset(TextBuf b[static 1]);
void textbuf_destroy(TextBuf* b);

Err textbuf_append_part(TextBuf ab[static 1], char* data, size_t len);
size_t textbuf_len(TextBuf ab[static 1]);
char* textbuf_items(TextBuf ab[static 1]);
char* textbuf_line_offset(TextBuf ab[static 1], size_t line);
static inline size_t* textbuf_current_line(TextBuf tb[static 1]) { return &tb->current_line; }

static inline ArlOf(size_t)* textbuf_eols(TextBuf tb[static 1]) { return &tb->eols; }
size_t* textbuf_eol_at(TextBuf tb[static 1], size_t i);
size_t textbuf_line_count(TextBuf textbuf [static 1]);
size_t textbuf_eol_count(TextBuf textbuf[static 1]);

static inline Err
textbuf_append_line(TextBuf ab[static 1], char* data, size_t len) {
    if (!len || !data || !*data)
        return err_fmt("error: invalid param: %s", __func__);
    if (!buffn(char,append)(&ab->buf, (char*)data, len))
        return err_fmt("error in %s: could not append data", __func__);
    if (!arlfn(size_t, append)(&ab->eols, &ab->buf.len))
        return err_fmt("error in %s: could not append eol", __func__);
    if (!buffn(char,append)(&ab->buf, (char*)"\n", 1))
        return err_fmt("error in %s: could not append newline, data: %s",__func__, data);

    return NULL;
}


static inline bool textbuf_is_empty(TextBuf ab[static 1]) {
    return !ab->buf.len;
}

static inline Err textbuf_append_null(TextBuf textbuf[static 1]) { return textbuf_append_part(textbuf, (char[]){0}, 1); }
Err textbuf_read_from_file(TextBuf textbuf[static 1], const char* filename) ;
Err textbuf_get_line_of(TextBuf tb[static 1], const char* ch, size_t* out) ;

static inline char* textbuf_current_line_offset(TextBuf tb[static 1]) {
    return textbuf_line_offset(tb, *textbuf_current_line(tb));
}
Err textbuf_append_part(TextBuf textbuf[static 1], char* data, size_t len);
Err textbuf_fit_lines(TextBuf tb[static 1], size_t maxlen);

Err textbuf_append_line_indexes(TextBuf tb[static 1]);

#endif
