#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__
int ahre_cmd_w(char* fname, BufOf(char) buf[static 1]);
int ahre_cmd_ahre(AhCtx ctx[static 1]);
int ahed_cmd_print(AhCtx ctx[static 1]) ;

static inline void ahre_cmd_clear(AhCtx* ctx) {
    free(ctx->buf.items);
    ctx->buf.len = 0;
}
#endif

