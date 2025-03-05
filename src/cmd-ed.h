#ifndef __AHRE_CMD_ED_H__
#define __AHRE_CMD_ED_H__

#include "src/generic.h"
#include "src/textbuf.h"
#include "src/ranges.h"
#include "src/user-out.h"

Err ed_global(TextBuf textbuf[static 1],  const char* rest);

static inline int
line_num_to_left_offset(size_t lnum, TextBuf textbuf[static 1], size_t out[static 1]) {

    if (lnum == 0 || lnum > textbuf_line_count(textbuf)) { return -1; }
    if (lnum == 1) { *out = 0; return 0; }

    size_t* tmp = textbuf_eol_at(textbuf, lnum-2);
    if (tmp) {
        *out = 1 + *tmp;
        return 0;
    }

    return -1;
}

static inline int
line_num_to_right_offset(size_t lnum, TextBuf textbuf[static 1], size_t out[static 1]) {

    if (lnum == 0 || lnum > textbuf_line_count(textbuf)) { return -1; }
    if (lnum == textbuf_line_count(textbuf)/*+1*/) {
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

static inline Err ed_print_last_range(TextBuf textbuf[static 1], UserOutput out[static 1]) {
    Range r = *textbuf_last_range(textbuf);

    try( uiw_lit__(out->write_msg, "last range: ("));
    try( ui_write_unsigned(out->write_msg, r.beg));
    try( uiw_lit__(out->write_msg, ", "));
    try( ui_write_unsigned(out->write_msg, r.end));
    try( uiw_lit__(out->write_msg, ")\n"));
    try( out->flush_msg());
    return Ok;
}

static inline Err ed_print_all(TextBuf textbuf[static 1]) {
    BufOf(char)* buf = &textbuf->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return NULL;
}

Err ed_eval(TextBuf textbuf[static 1], const char* line);
Err ed_write(const char* rest, TextBuf textbuf[static 1]);
Err dbg_print_all_lines_nums(TextBuf textbuf[static 1]);

static inline Err ed_edit(TextBuf textbuf[static 1], const char* rest) { return textbuf_read_from_file(textbuf, rest); }
Err shorcut_zf(Session session[static 1], const char* rest);
Err shorcut_zb(Session session[static 1], const char* rest);
Err shorcut_zn(Session session[static 1]);
Err shorcut_zz(Session session[static 1], const char* rest);
Err shorcut_gg(Session session[static 1], const char* rest);
#endif
