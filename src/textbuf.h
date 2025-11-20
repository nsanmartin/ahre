#ifndef __AE_BUF_H__
#define __AE_BUF_H__

#include <stdbool.h>

#include "error.h"
#include "escape-codes.h"
#include "ranges.h"
#include "str.h"
#include "textmod.h"
#include "utils.h"


typedef struct {
    Range last_range;
} TextBufCache;


typedef struct {
    BufOf(char) buf;
    size_t current_offset;
    ArlOf(size_t) eols;
    TextBufCache cache;
    TextBufMods mods;
} TextBuf;


/* getters */
static inline size_t* textbuf_current_offset(TextBuf tb[static 1]) { return &tb->current_offset; }
static inline BufOf(char)*
textbuf_buf(TextBuf t[static 1]) { return &t->buf; }
static inline TextBufMods* textbuf_mods(TextBuf tb[static 1]) { return &tb->mods; }

static inline Range* textbuf_last_range(TextBuf t[static 1]) { return &t->cache.last_range; }
static inline ArlOf(size_t)* textbuf_eols(TextBuf tb[static 1]) { return &tb->eols; }

/* ctor */
static inline int textbuf_init(TextBuf ab[static 1]) { *ab = (TextBuf){0}; return 0; }

/* dtors */
void textbuf_cleanup(TextBuf b[static 1]);
void textbuf_reset(TextBuf b[static 1]);
void textbuf_destroy(TextBuf* b);

Err textbuf_append_part(TextBuf ab[static 1], char* data, size_t len);
static inline size_t textbuf_len(TextBuf textbuf[static 1]) { return textbuf->buf.len; }
static inline char* textbuf_items(TextBuf textbuf[static 1]) { return textbuf->buf.items; }

//char* textbuf_line_offset(TextBuf ab[static 1], size_t line);


Err textbuf_get_line_of_offset(TextBuf tb[static 1], size_t off, size_t* out);

/*todo return err*/
static inline size_t textbuf_current_line(TextBuf tb[static 1]) {
    size_t line;
    Err err = textbuf_get_line_of_offset(tb, *textbuf_current_offset(tb), &line);
    if (err) {
        fprintf(stderr, "ERROR MSG: %s\n", "invalid current offset!");
        return SIZE_MAX;
    }
    return line;
}

static inline size_t textbuf_eol_count(TextBuf textbuf[static 1]) {
    return textbuf->eols.len;
}

static size_t textbuf_line_count(TextBuf textbuf[static 1]) {
    size_t len = textbuf_len(textbuf);
    if (!len) return 0;
    return textbuf_eol_count(textbuf) + 1;
}

static inline Err textbuf_get_offset_of_line(TextBuf tb[static 1], size_t line, size_t* out) {
    if (!line) return "error: unexpected invalid number (0)";
    if (textbuf_line_count(tb) < line) return "error: unexpected invalid line number (too large)";
    if (line == 1) { *out = 0; return Ok; }
    size_t* nth_eol = arlfn(size_t,at)(textbuf_eols(tb), line - 2);
    if (nth_eol) { *out = *nth_eol + 1; return Ok; }
    return "ERROR: fix get offset of line";
}

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

static inline Err textbuf_append_null(TextBuf textbuf[static 1]) {
    return textbuf_append_part(textbuf, (char[]){0}, 1);
}
Err textbuf_get_line_of(TextBuf tb[static 1], const char* ch, size_t* out) ;

//static inline char* textbuf_current_line_offset(TextBuf tb[static 1]) {
//    return textbuf_line_offset(tb, textbuf_current_line(tb));
//}
Err textbuf_append_part(TextBuf textbuf[static 1], char* data, size_t len);
Err textbuf_fit_lines(TextBuf tb[static 1], size_t maxlen);

Err textbuf_append_line_indexes(TextBuf tb[static 1]);

/* line indexes start at 1! */
bool textbuf_get_line(TextBuf tb[static 1], size_t n, StrView out[static 1]);

static inline Err validate_range_for_buffer(TextBuf textbuf[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) { return  "error: unexpected bad range"; }
    if (textbuf_line_count(textbuf) < range->end) return "error: unexpected invalid range end";
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }
    return Ok;
}

Err textbuf_line_offset(TextBuf tb[static 1], size_t line, size_t out[static 1]);

Err textbuf_range_from_parsed_range(
    TextBuf          textbuf[static 1],
    RangeParse rres[static 1],
    Range            range[static 1]
);
Err textbuf_to_file(TextBuf tb[static 1], const char* filename, const char* mode);
#endif
