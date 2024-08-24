#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

int ahcmd_w(char* fname, AhCtx ctx[static 1]);
int ahcmd_fetch(AhCtx ctx[static 1]) {
    AhDoc* ahdoc = ahctx_current_doc(ctx);
    if (ahdoc->url) {
        ErrStr err_str = AhDocFetch(ctx->ahcurl, ahdoc);
        if (err_str) { return ah_log_error(err_str, ErrCurl); }
        return Ok;
    }
    puts("Not url to fech");
    return -1;
}


static inline int ahcmd_ahre(AhCtx ctx[static 1]) {
    AhDoc* ad = ahctx_current_doc(ctx);
    return lexbor_href_write(ad->doc, &ad->cache.hrefs, &ad->aebuf.buf);
}

static inline int ahcmd_clear(AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    if (buf->len) { free(buf->items); *buf = (BufOf(char)){0}; }
    return 0;
}

static inline int ahcmd_tag(char* rest, AhCtx ctx[static 1]) {
    AhDoc* ahdoc = ahctx_current_doc(ctx);
    return lexbor_cp_tag(rest, ahdoc->doc, &ahdoc->aebuf.buf);
}

static inline int ahcmd_text(AhCtx* ctx) {
    AhDoc* ahdoc = ahctx_current_doc(ctx);
    return lexbor_print_html_text(ahdoc->doc, &ahdoc->aebuf.buf);
}

static inline int aecmd_print(AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    if (buf->len) {
        fwrite(buf->items, 1, buf->len, stdout);
    }
    return 0;
}


int aecmd_write(char* rest, AhCtx ctx[static 1]);
#endif

