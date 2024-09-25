#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include "src/ranges.h"
#include "src/user-interface.h"

Err ed_global(Session session[static 1],  const char* rest);

Err cmd_write(char* fname, Session session[static 1]);

Err cmd_fetch(Session session[static 1]) {
    Doc* doc = session_current_doc(session);
    if (doc->url) {
        return doc_fetch(doc, session->url_client);
    }
    return "Not url to fech";
}


static inline Err cmd_ahre(Session session[static 1]) {
    Doc* doc = session_current_doc(session);
    return lexbor_href_write(doc->lxbdoc, &doc->cache.hrefs, &doc->textbuf)
        ? "could not fetch href"
        : Ok
        ;
}

static inline Err cmd_clear(Session session[static 1]) {
    textbuf_cleanup(session_current_buf(session));
    return Ok;
}

static inline Err cmd_tag(const char* rest, Session session[static 1]) {
    Doc* doc = session_current_doc(session);
    return lexbor_cp_tag(rest, doc->lxbdoc, &doc->textbuf.buf);
}

static inline Err cmd_text(Session* session) {
    Doc* doc = session_current_doc(session);
    return lexbor_html_text_append(doc->lxbdoc, &doc->textbuf)
        ? "could not append text"
        : Ok
        ;
}

static inline int
line_num_to_left_offset(size_t lnum, TextBuf textbuf[static 1], size_t out[static 1]) {

    if (lnum == 0 || lnum > textbuf_eol_count(textbuf)) { return -1; }
    if (lnum == 1) { *out = 0; return 0; }

    size_t* tmp = textbuf_eol_at(textbuf, lnum-2);
    if (tmp) {
        *out = *tmp;
        return 0;
    }

    return -1;
}

static inline int
line_num_to_right_offset(size_t lnum, TextBuf textbuf[static 1], size_t out[static 1]) {

    if (lnum == 0 || lnum > textbuf_eol_count(textbuf)) { return -1; }
    if (lnum == textbuf_eol_count(textbuf)+1) {
        *out = len(textbuf);
        return 0;
    }

    size_t* tmp = textbuf_eol_at(textbuf, lnum-1);
    if (tmp) {
        *out = 1 + *tmp;
        return 0;
    }

    return -1;
}

static inline Err ed_print(Session session[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) {
        fprintf(stderr, "! bad range\n");
        return  "bad range";
    }

    TextBuf* textbuf = session_current_buf(session);
    if (textbuf_is_empty(textbuf)) {
        fprintf(stderr, "? empty buffer\n");
        return "empty buffer";
    }

    size_t beg_off_p;
    if (line_num_to_left_offset(range->beg, textbuf, &beg_off_p)) {
        fprintf(stderr, "? bag range beg\n");
        return "bad range beg";
    }
    size_t end_off_p;
    if (line_num_to_right_offset(range->end, textbuf, &end_off_p)) {
        fprintf(stderr, "? bag range end\n");
        return "bad range end";
    }

    if (beg_off_p >= end_off_p || end_off_p > textbuf->buf.len) {
        fprintf(stderr, "!Error check offsets");
        return "error: check offsets";
    }
    fwrite(textbuf->buf.items + beg_off_p, 1, end_off_p-beg_off_p, stdout);
    fwrite("\n", 1, 1, stdout);
    return Ok;
}


Err ed_write(const char* rest, Session session[static 1]);

#endif
