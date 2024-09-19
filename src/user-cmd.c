#include <ah/utils.h>
#include <ah/wrappers.h>
#include <ah/user-interface.h>

Str parse_pattern(const char* tk) {
    Str res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) {
        res = (Str){.s = tk, .len=strlen(tk)};
    } else {
        res = (Str){.s = tk, .len=end-tk};
    }
    return res;
}
/* ah cmds */

int ahcmd_w(const char* fname, AhCtx ctx[static 1]) {
    BufOf(char)* buf = &AhCtxCurrentBuf(ctx)->buf;
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
int aecmd_write(const char* rest, AhCtx ctx[static 1]) {
    BufOf(char)* buf = &AhCtxCurrentBuf(ctx)->buf;
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

int aecmd_print_all_lines_nums(AhCtx ctx[static 1]) {
    TextBuf* ab = AhCtxCurrentBuf(ctx);
    BufOf(char)* buf = &ab->buf;
    char* items = buf->items;
    size_t len = buf->len;

    

    for (size_t lnum = 1; /*items && len && lnum < 40*/ ; ++lnum) {
        char* next = memchr(items, '\n', len);
        if (next >= buf->items + buf->len) {
            fprintf(stderr, "Error: print all lines nums\n");
        }
        printf("%ld: ", lnum);

        if (next) {
            size_t line_len = 1+next-items;
            fwrite(items, 1, line_len, stdout);
            items += line_len;
            len -= line_len;
        } else {
            fwrite(items, 1, len, stdout);
            break;
        }
    }
    return 0;
}

int aecmd_global(AhCtx ctx[static 1],  const char* rest) {
    (void)ctx;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return -1; }
    printf("pattern: %s\n", pattern.s);
    return 0;
}


