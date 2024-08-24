#include <ahutils.h>
#include <wrappers.h>
#include <user-interface.h>

/* ah cmds */

int ahcmd_w(char* fname, AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    if (fname && *(fname = skip_space(fname))) {
        FILE* fp = fopen(fname, "a");
        if (!fp) {
            fprintf(stderr, "append: could not open file: %s\n", fname);
            return -1;
        }
        if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
            fprintf(stderr, "append: error writing to file: %s\n", fname);
            return -1;
        }
        return 0;
    }
    puts("append: fname missing");
    return -1;
}


/* ed cmds */
int aecmd_write(char* rest, AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    if (!rest || !*rest) { puts("cannot write without file arg"); return 0; }
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        fprintf(stderr, "write: could not open file: %s\n", rest); return -1;
    }
    if (fwrite(buf->items, 1, buf->len, fp) != buf->len) {
        fprintf(stderr, "write: error writing to file: %s\n", rest); return -1;
    }
    return 0;
}
