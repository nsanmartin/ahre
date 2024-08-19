#include <ahutils.h>
#include <wrappers.h>
#include <user-interface.h>

/* ah cmds */
int ahre_cmd_ahre(AhCtx ctx[static 1]) {
    return lexbor_href_write(
        ctx->ahdoc->doc, &ctx->ahdoc->cache.hrefs, &ctx->buf
    );
}

int ahre_cmd_w(char* fname, BufOf(char) buf[static 1]) {
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
int ahed_cmd_print(AhCtx ctx[static 1]) {
    if (ctx->buf.len) {
        fwrite(ctx->buf.items, 1, ctx->buf.len, stdout);
    }
    return 0;
}

