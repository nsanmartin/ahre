#ifndef __AHRE_USER_CMD_H__
#define __AHRE_USER_CMD_H__

#include <ae-ranges.h>

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
    return lexbor_html_text_append(ahdoc->doc, &ahdoc->aebuf);
}

static inline int aecmd_print(AhCtx ctx[static 1], AeRange range[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    if (!buf->len) { return 0; }
    char* beg = buf->items;
    size_t max_len = buf->len;
    const char* eobuf = beg + max_len;
    size_t beg_line = range->beg;

    // find first char
    for (;(const char*)beg < eobuf && beg_line; beg_line--) {
        char* next = memchr(beg, '\n', max_len);
        if (!next) {
            puts("invalid line. TODO: compute #lines in buffer");
            return(-1);
        }
        max_len -= 1 + (next - beg);
        beg = next + 1;
    }
    char* end = beg;
    //TODO
    size_t end_line = range->end;
    end_line -= range->beg;

    // find last char
    for (;(const char*)end < eobuf && end_line; end_line--) {
        char* next = memchr(end, '\n', max_len);
        if (!end) {
            puts("invalid line. TODO: compute #lines in buffer");
            return(-1);
        }
        max_len -= 1 + (next - end);
        end = next + 1;
    }

    fwrite(beg, 1, end-beg, stdout);

    if (end_line) { puts("Error: bad end of range!"); return -1; }
    return 0;
}

static inline int aecmd_print_all(AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    fwrite(buf->items, 1, buf->len, stdout);
    return 0;
}

static inline int aecmd_print_all_lines_nums(AhCtx ctx[static 1]) {
    BufOf(char)* buf = &ahctx_current_buf(ctx)->buf;
    char* items = buf->items;
    size_t len = buf->len;

    

    for (size_t lnum = 1; items && len && lnum < 40; ++lnum) {
        char* next = memchr(items, '\n', len);
        printf("%ld: ", lnum);

        if (next) {
            size_t line_len = 1+next-items;
            fwrite(items, 1, line_len, stdout);
            items += line_len;
            len -= line_len;
        } else {
            fwrite(items, 1, len, stdout);
        }
    }
    return 0;
}

int aecmd_write(char* rest, AhCtx ctx[static 1]);
#endif

