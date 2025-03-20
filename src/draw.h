#ifndef AHRE_DRAW_H__
#define AHRE_DRAW_H__

#include "src/escape_codes.h"
#include "src/htmldoc.h"
#include "src/utils.h"
#include "src/generic.h"
#include "src/session-conf.h"
#include "src/session.h"

#define T EscCode
#include <arl.h>

#define DRAW_CTX_FLAG_MONOCHROME 0x1u
#define DRAW_CTX_FLAG_PRE   0x2u

typedef struct { 
    HtmlDoc* htmldoc;
    BufOf(char) buf;
    ArlOf(EscCode) esc_code_stack;
    unsigned flags;
    SessionWriteFn logfn;
} DrawCtx;

typedef Err (*ImpureDrawProcedure)(DrawCtx ctx[static 1]);

static inline HtmlDoc* draw_ctx_htmldoc(DrawCtx ctx[static 1]) { return ctx->htmldoc; }

static inline SessionWriteFn draw_ctx_logfn(DrawCtx ctx[static 1]) { return ctx->logfn; }

static inline bool draw_ctx_hide_tags(DrawCtx ctx[static 1], size_t tags) {
    return *htmldoc_hide_tags(draw_ctx_htmldoc(ctx)) & tags;
}


static inline TextBuf* draw_ctx_textbuf(DrawCtx ctx[static 1]) {
    return htmldoc_textbuf(draw_ctx_htmldoc(ctx));
}


static inline BufOf(char)* draw_ctx_textbuf_buf_(DrawCtx ctx[static 1]) {
    return &draw_ctx_textbuf(ctx)->buf;
}

static inline bool draw_ctx_color(DrawCtx ctx[static 1]) { return !(ctx->flags & DRAW_CTX_FLAG_MONOCHROME); }
static inline bool draw_ctx_pre(DrawCtx ctx[static 1]) { return (ctx->flags & DRAW_CTX_FLAG_PRE); }
static inline void draw_ctx_pre_set(DrawCtx ctx[static 1], bool value) {
    if (value) ctx->flags |= DRAW_CTX_FLAG_PRE;
    else ctx->flags &= ~DRAW_CTX_FLAG_PRE;
}

static inline BufOf(char)* draw_ctx_buf(DrawCtx ctx[static 1]) { return &ctx->buf; }


static inline BufOf(char) draw_ctx_buf_get_reset(DrawCtx ctx[static 1]) {
    BufOf(char) tmp = *draw_ctx_buf(ctx);
    ctx->buf = (BufOf(char)){0};
    return tmp;
}

static inline void draw_ctx_swap_buf(DrawCtx ctx[static 1], BufOf(char) buf[static 1]) {
    BufOf(char) tmp = *buf;
    *buf = *draw_ctx_buf(ctx);
    ctx->buf = tmp;
}

        
static inline ArlOf(EscCode)* draw_ctx_esc_code_stack(DrawCtx ctx[static 1]) {
    return &ctx->esc_code_stack;
}

static inline Err draw_ctx_esc_code_push(DrawCtx ctx[static 1], EscCode code) {
    if (arlfn(EscCode, append)(draw_ctx_esc_code_stack(ctx), &code)) return Ok;
    return "error: arlfn append failure";
}

static inline Err draw_ctx_esc_code_pop(DrawCtx ctx[static 1]) {
    return arlfn(EscCode, pop)(draw_ctx_esc_code_stack(ctx)) ? Ok : "error: empty stack";
}

static inline EscCode* draw_ctx_esc_code_stack_backp(DrawCtx ctx[static 1]) {
    ArlOf(EscCode)* stack = draw_ctx_esc_code_stack(ctx);
    return arlfn(EscCode, back)(stack);
}

static inline Err
draw_ctx_buf_commit(DrawCtx ctx[static 1]) {
    BufOf(char)* buf = draw_ctx_buf(ctx);
    if (len__(buf)) {
        BufOf(char)* tb_buf = draw_ctx_textbuf_buf_(ctx);
        if (!buffn(char, append)(tb_buf, items__(buf), len__(buf)))
            return "error: could not append empty line to TextBuffer";
        buffn(char, reset)(buf);
    }
    return Ok;
}

static inline Err draw_ctx_buf_append_mem(DrawCtx ctx[static 1], char* s, size_t len) { 
    return buffn(char, append)(draw_ctx_buf(ctx), s, len)
        ? Ok
        : "error: failed to append to bufof (draw ctx buf)";
}

static inline Err draw_ctx_buf_append(DrawCtx ctx[static 1], StrView s) { 
    return draw_ctx_buf_append_mem(ctx, (char*)s.items, s.len);
}

#define draw_ctx_buf_append_lit__(Ctx, Str) draw_ctx_buf_append_mem(Ctx, Str, sizeof(Str)-1)

static inline void draw_ctx_buf_reset(DrawCtx ctx[static 1]) {
    buffn(char, reset)(draw_ctx_buf(ctx));
}

static inline Err
draw_ctx_init(DrawCtx ctx[static 1], HtmlDoc htmldoc[static 1], Session s[static 1]) {
    unsigned flags = (session_monochrome(s)? DRAW_CTX_FLAG_MONOCHROME: 0);
    *ctx = (DrawCtx) {
        .htmldoc=htmldoc,
        .flags=flags,
        .logfn=session_doc_msg_fn(s, htmldoc)
    };
    return Ok;
}

static inline void draw_ctx_cleanup(DrawCtx ctx[static 1]) {
    buffn(char, clean)(&ctx->buf);
    arlfn(EscCode, clean)(draw_ctx_esc_code_stack(ctx));
}

static inline void draw_ctx_set_color(DrawCtx ctx[static 1], bool value) {
    ctx->flags = ctx->flags ^ (value ? 0 : DRAW_CTX_FLAG_MONOCHROME);
}

Err draw_rec(lxb_dom_node_t* node, DrawCtx ctx[static 1]);

static inline Err
draw_list( lxb_dom_node_t* it, lxb_dom_node_t* last, DrawCtx ctx[static 1]) {
    for(; it ; it = it->next) {
        try( draw_rec(it, ctx));
        if (it == last) break;
    }
    return Ok;
}


static inline Err
draw_list_block( lxb_dom_node_t* it, lxb_dom_node_t* last, DrawCtx ctx[static 1]) {
    Err err;
    BufOf(char) buf = draw_ctx_buf_get_reset(ctx);

    err = draw_list(it, last, ctx);

    draw_ctx_swap_buf(ctx, &buf);
    if (err) {
        buffn(char, clean)(&buf);
        return err;
    }
    if (buf.len) {
        if (   (err=draw_ctx_buf_append_lit__(ctx, "\n"))
            || (err=draw_ctx_buf_append_mem(ctx, (char*)buf.items, buf.len))
            || (err=draw_ctx_buf_append_lit__(ctx, "\n"))
        ) {
            buffn(char, clean)(&buf);
            return err;
        }

    }
    buffn(char, clean)(&buf);
    return Ok;
}

static inline Err draw_ctx_buf_append_color_esc_code(DrawCtx ctx[static 1], EscCode code) {
    if (draw_ctx_color(ctx)) {
        StrView code_str;
        try( esc_code_to_str(code, &code_str));
        try( draw_ctx_buf_append_mem(ctx, (char*)code_str.items, code_str.len));
    }
    return Ok;
}
 

static inline Err draw_ctx_buf_append_color_(DrawCtx ctx[static 1], EscCode code) {
    if (draw_ctx_color(ctx)) {
        try( draw_ctx_esc_code_push(ctx, code));
        StrView code_str;
        try( esc_code_to_str(code, &code_str));
        try( draw_ctx_buf_append_mem(ctx, (char*)code_str.items, code_str.len));
    }
    return Ok;
}
 
#define _push_esc_code__ draw_ctx_buf_append_color_
static inline
Err draw_ctx_push_italic(DrawCtx ctx[static 1]) { return _push_esc_code__(ctx, esc_code_italic); }

static inline
Err draw_ctx_push_bold(DrawCtx ctx[static 1]) { return _push_esc_code__(ctx, esc_code_bold); }

static inline
Err draw_ctx_push_underline(DrawCtx ctx[static 1]){return _push_esc_code__(ctx, esc_code_underline);}

static inline Err draw_ctx_color_blue(DrawCtx ctx[static 1]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_blue);
}

static inline Err draw_ctx_color_red(DrawCtx ctx[static 1]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_red);
}

static inline Err draw_ctx_color_light_green(DrawCtx ctx[static 1]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_light_green);
}

static inline Err draw_ctx_color_purple(DrawCtx ctx[static 1]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_purple);
}

static inline Err draw_ctx_buf_append_ui_(DrawCtx ctx[static 1], uintmax_t ui) {
    BufOf(char)* buf = draw_ctx_buf(ctx);
    return bufofchar_append_ui_as_str(buf, ui);
}

static inline Err draw_ctx_buf_append_ui_base36_(DrawCtx ctx[static 1], uintmax_t ui) {
    BufOf(char)* buf = draw_ctx_buf(ctx);
    return bufofchar_append_ui_base36_as_str(buf, ui);
}

static inline Err draw_ctx_reset_color(DrawCtx ctx[static 1]) {
    if (draw_ctx_color(ctx)) {
        try( draw_ctx_buf_append_lit__(ctx, EscCodeReset));
        ArlOf(EscCode)* stack = draw_ctx_esc_code_stack(ctx);
        try( draw_ctx_esc_code_pop(ctx));
        EscCode* backp =  arlfn(EscCode, back)(stack);
        if (backp) {
            StrView code_str;
            try( esc_code_to_str(*backp, &code_str));
            try( draw_ctx_buf_append_mem(ctx, (char*)code_str.items, code_str.len));
        }
    }
    return Ok;
}

#endif
