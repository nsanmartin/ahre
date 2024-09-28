#include "src/utils.h"
#include "src/user-interface.h"

/* ah cmds */

Err cmd_write(const char* fname, Session session[static 1]) {
    TextBuf* buf = session_current_buf(session);
    if (fname && *(fname = cstr_skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            return err_fmt("append: could not open file: %s", fname);
        }
        if (fwrite(textbuf_items(buf), 1, len(buf), fp) != len(buf)) {
            return err_fmt("append: error writing to file: %s", fname);
        }
        return 0;
    }
    return "append: fname missing";
}


