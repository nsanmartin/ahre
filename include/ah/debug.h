#ifndef __AHRE_DEBUG_H__
#define __AHRE_DEBUG_H__

#include <ah/session.h>

static inline int aecmd_print_all(Session session[static 1]) {
    BufOf(char)* buf = &session_current_buf(session)->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return 0;
}
int dbg_print_all_lines_nums(Session session[static 1]);
#endif
