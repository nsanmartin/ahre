#ifndef AHRE_DRAW_H__
#define AHRE_DRAW_H__

#include "escape-codes.h"
#include "generic.h"
#include "htmldoc.h"
#include "session-conf.h"
#include "session.h"
#include "url-client.h"
#include "utils.h"

#define T EscCode
#include <arl.h>


typedef struct {
    HtmlDoc* htmldoc;
    char* fragment;
    ArlOf(EscCode) esc_code_stack;
#define DRAW_CTX_FLAG_MONOCHROME 0x1u
#define DRAW_CTX_FLAG_PRE        0x2u
#define DRAW_CTX_FLAG_TITLE      0x4u
#define DRAW_CTX_FLAG_JS         0x8u
    unsigned flags;
    Session* session;
} DrawCtx;


static inline unsigned draw_ctx_flags_from_session(Session s[_1_]) {
    unsigned flags
        = (session_monochrome(s) ? DRAW_CTX_FLAG_MONOCHROME : 0)
        | (session_js(s)         ? DRAW_CTX_FLAG_JS         : 0)
        ;
    return flags;
}

static inline ArlOf(EscCode)* draw_ctx_esc_code_stack(DrawCtx ctx[_1_]) { return &ctx->esc_code_stack; }
static inline HtmlDoc* draw_ctx_htmldoc(DrawCtx ctx[_1_]) { return ctx->htmldoc; }
static inline Session* draw_ctx_session(DrawCtx ctx[_1_]) { return ctx->session; }
static inline TextBuf* draw_ctx_textbuf(DrawCtx ctx[_1_]) { return htmldoc_textbuf(draw_ctx_htmldoc(ctx)); }
static inline BufOf(char)* draw_ctx_textbuf_buf_(DrawCtx ctx[_1_]) { return &draw_ctx_textbuf(ctx)->buf; }
static inline UrlClient* draw_ctx_url_client(DrawCtx ctx[_1_]) { return session_url_client(ctx->session); }
static inline bool draw_ctx_color(DrawCtx ctx[_1_]) { return !(ctx->flags & DRAW_CTX_FLAG_MONOCHROME); }
static inline bool draw_ctx_pre(DrawCtx ctx[_1_]) { return (ctx->flags & DRAW_CTX_FLAG_PRE); }
static inline void draw_ctx_pre_set(DrawCtx ctx[_1_], bool value) {
    if (value) ctx->flags |= DRAW_CTX_FLAG_PRE;
    else ctx->flags &= ~DRAW_CTX_FLAG_PRE;
}

static inline Err
draw_ctx_init(DrawCtx ctx[_1_], HtmlDoc htmldoc[_1_], Session s[_1_], unsigned flags) {
    char* fragment = NULL;
    try( url_fragment(htmldoc_url(htmldoc), &fragment));
    *ctx = (DrawCtx) {
        .htmldoc  = htmldoc,
        .fragment = fragment,
        .flags    = flags,
        .session  = s
    };
    return Ok;
}

static inline void draw_ctx_cleanup(DrawCtx ctx[_1_]) {
    std_free(ctx->fragment);
    arlfn(EscCode, clean)(draw_ctx_esc_code_stack(ctx));
}

static inline void draw_ctx_set_color(DrawCtx ctx[_1_], bool value) {
    ctx->flags = ctx->flags ^ (value ? 0 : DRAW_CTX_FLAG_MONOCHROME);
}
#endif
