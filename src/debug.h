#ifndef __AHRE_DEBUG_H__
#define __AHRE_DEBUG_H__

#include "src/session.h"

static inline Err ed_print_all(TextBuf textbuf[static 1]) {
    BufOf(char)* buf = &textbuf->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return NULL;
}
Err dbg_print_all_lines_nums(TextBuf textbuf[static 1]);
#endif
