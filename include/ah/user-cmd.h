#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include <ah/ranges.h>
#include <ah/user-interface.h>

int edcmd_global(Session session[static 1],  const char* rest);

int ahcmd_write(char* fname, Session session[static 1]);
int ahcmd_fetch(Session session[static 1]) {
    Doc* ahdoc = session_current_doc(session);
    if (ahdoc->url) {
        ErrStr err_str = doc_fetch(session->url_client, ahdoc);
        if (err_str) { return ah_log_error(err_str, ErrCurl); }
        return Ok;
    }
    puts("Not url to fech");
    return -1;
}


static inline int ahcmd_ahre(Session session[static 1]) {
    Doc* ad = session_current_doc(session);
    return lexbor_href_write(ad->doc, &ad->cache.hrefs, &ad->aebuf);
}

static inline int ahcmd_clear(Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    if (buf->len) { free(buf->items); *buf = (BufOf(char)){0}; }
    return 0;
}

static inline int ahcmd_tag(const char* rest, Session session[static 1]) {
    Doc* ahdoc = session_current_doc(session);
    return lexbor_cp_tag(rest, ahdoc->doc, &ahdoc->aebuf.buf);
}

static inline int ahcmd_text(Session* session) {
    Doc* ahdoc = session_current_doc(session);
    return lexbor_html_text_append(ahdoc->doc, &ahdoc->aebuf);
}

static inline int
line_num_to_left_offset(size_t lnum, TextBuf* aebuf, size_t* out) {
    ArlOf(size_t)* offs = &aebuf->eols;

    if (lnum == 0 || lnum > offs->len) { return -1; }
    if (lnum == 1) { *out = 0; return 0; }

    size_t* tmp = arlfn(size_t, at)(offs, lnum-2);
    if (tmp) {
        *out = *tmp;
        return 0;
    }

    return -1;
}

static inline int
line_num_to_right_offset(size_t lnum, TextBuf* aebuf, size_t* out) {
    ArlOf(size_t)* offs = &aebuf->eols;

    if (lnum == 0 || lnum > offs->len) { return -1; }
    if (lnum == offs->len+1) {
        *out = aebuf->buf.len;
        return 0;
    }

    size_t* tmp = arlfn(size_t, at)(offs, lnum-1);
    if (tmp) {
        *out = 1 + *tmp;
        return 0;
    }

    return -1;
}

static inline int aecmd_print(Session session[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) {
        fprintf(stderr, "! bad range\n");
        return -1;
    }

    TextBuf* aebuf = session_current_buf(session);
    if (textbuf_is_empty(aebuf)) {
        fprintf(stderr, "? empty buffer\n");
        return -1;
    }

    size_t beg_off_p;
    if (line_num_to_left_offset(range->beg, aebuf, &beg_off_p)) {
        fprintf(stderr, "? bag range beg\n");
        return -1;
    }
    size_t end_off_p;
    if (line_num_to_right_offset(range->end, aebuf, &end_off_p)) {
        fprintf(stderr, "? bag range end\n");
        return -1;
    }

    if (beg_off_p >= end_off_p || end_off_p > aebuf->buf.len) {
        fprintf(stderr, "!Error check offsets");
        return -1;
    }
    fwrite(aebuf->buf.items + beg_off_p, 1, end_off_p-beg_off_p, stdout);
    fwrite("\n", 1, 1, stdout);
    return 0;
}
static inline int aecmd_print_(Session session[static 1], Range range[static 1]) {
    TextBuf* aebuf = session_current_buf(session);
    if (textbuf_is_empty(aebuf)) {
        fprintf(stderr, "? empty buffer\n");
        return -1;
    }
    BufOf(char)* buf = &aebuf->buf;
    if (!buf->len) { return 0; }
    char* beg = buf->items;
    size_t max_len = buf->len;
    const char* eobuf = beg + max_len;
    size_t beg_line = range->beg;

    /* find first line offset */
    for (;(const char*)beg < eobuf && beg_line; beg_line--) {
        char* next = memchr(beg, '\n', max_len);
        if (!next) {
            puts("invalid line. TODO: compute #lines in buffer");
            return(-1);
        }
        max_len -= 1 + (next - beg);
        beg = next + 1;
    }
    char* end = beg;

    size_t end_line = range->end;
    end_line -= range->beg - 1;

    /* find last line end offset */
    for (;(const char*)end < eobuf && end_line; end_line--) {
        char* next = memchr(end, '\n', max_len);
        if (!end) {
            puts("invalid line. TODO: compute #lines in buffer");
            return(-1);
        }
        max_len -= 1 + (next - end);
        end = next + 1;
    }

    fwrite(beg, 1, end-beg, stdout);

    if (end_line) { puts("Error: bad end of range!"); return -1; }
    return 0;
}


int edcmd_write(const char* rest, Session session[static 1]);

#endif

