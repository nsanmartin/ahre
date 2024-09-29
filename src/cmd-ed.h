#ifndef __AHRE_CMD_ED_H__
#define __AHRE_CMD_ED_H__

#include "src/textbuf.h"
#include "src/ranges.h"

Err ed_global(TextBuf textbuf[static 1],  const char* rest);
Err textbuf_eval_cmd(TextBuf textbuf[static 1], const char* line, Range range[static 1]);

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

static inline Err ed_print_all(TextBuf textbuf[static 1]) {
    BufOf(char)* buf = &textbuf->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return NULL;
}

Err ed_write(const char* rest, TextBuf textbuf[static 1]);
Err dbg_print_all_lines_nums(TextBuf textbuf[static 1]);

static inline Err
ed_edit(TextBuf textbuf[static 1], const char* rest) { return textbuf_read_from_file(textbuf, rest); }
Err ed_eval(TextBuf textbuf[static 1], const char* line);
#endif
