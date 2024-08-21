#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__
int ahre_cmd_w(char* fname, BufOf(char) buf[static 1]);
int ahre_cmd_ahre(AhCtx ctx[static 1], BufOf(char)* buf);
int ahed_cmd_print(AhCtx ctx[static 1]) ;

int ahre_fetch(AhCtx ctx[static 1]);
static inline int ahre_cmd_clear(BufOf(char)* buf) {
    if (buf->len) { free(buf->items); *buf = (BufOf(char)){0}; }
    return 0;
}

static inline int ahre_cmd_tag(char* rest, AhCtx* ctx, BufOf(char)* buf) {
    return lexbor_cp_tag(rest, ctx->ahdoc->doc, buf);
}

static inline int ahre_cmd_text(AhCtx* ctx, BufOf(char)*buf) {
    return lexbor_print_html_text(ctx->ahdoc->doc, buf);
}

#endif

