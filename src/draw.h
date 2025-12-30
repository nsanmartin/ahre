#ifndef AHRE_DRAW_H__
#define AHRE_DRAW_H__

#include "escape-codes.h"
#include "htmldoc.h"
#include "utils.h"
#include "generic.h"
#include "session-conf.h"
#include "session.h"
#include "url-client.h"

#define T EscCode
#include <arl.h>

static size_t _strview_trim_left_count_newlines_(StrView s[_1_]) {
    size_t newlines = 0;
    while(s->len && isspace(*(items__(s)))) {
        newlines += *(items__(s)) == '\n';
        ++s->items;
        --s->len;
    }
    return newlines;
}

static size_t _strview_trim_right_count_newlines_(StrView s[_1_]) {
    size_t newlines = 0;
    while(s->len && isspace(items__(s)[len__(s)-1])) {
        newlines += items__(s)[s->len-1] == '\n';
        --s->len;
    }
    return newlines;
}

#define DRAW_SUBCTX_FLAG_DIV 0x1u
typedef struct {
    BufOf(char) buf;
    size_t left_trim;
    size_t left_newlines;
    size_t right_newlines;
    TextBufMods mods;
    size_t fragment_offset;
    size_t flags;
} DrawSubCtx;


static inline void draw_subctx_clean(DrawSubCtx subctx[_1_]) {
    buffn(char, clean)(&subctx->buf);
    arlfn(ModAt, clean)(&subctx->mods);
}

static inline void draw_subctx_trim_left(DrawSubCtx sub[_1_]) {
    StrView content = strview_from_mem(sub->buf.items, sub->buf.len);
    sub->left_newlines = _strview_trim_left_count_newlines_(&content);
    sub->left_trim = sub->buf.len-content.len;
    textmod_trim_left(&sub->mods, sub->left_trim);
}

static inline void draw_subctx_trim_right(DrawSubCtx sub[_1_]) {
    StrView content = strview_from_mem(sub->buf.items, sub->buf.len);
    sub->right_newlines = _strview_trim_right_count_newlines_(&content);
    sub->buf.len = content.len;
}

typedef struct {
    HtmlDoc* htmldoc;
    char* fragment;
    ArlOf(EscCode) esc_code_stack;
#define DRAW_CTX_FLAG_MONOCHROME 0x1u
#define DRAW_CTX_FLAG_PRE        0x2u
#define DRAW_CTX_FLAG_TITLE      0x4u
#define DRAW_CTX_FLAG_JS         0x8u
    unsigned flags;
    SessionWriteFn logfn;
    Session* session;
    DrawSubCtx sub;
} DrawCtx;

static inline unsigned draw_ctx_flags_from_session(Session s[_1_]) {
    unsigned flags
        = (session_monochrome(s) ? DRAW_CTX_FLAG_MONOCHROME : 0)
        | (session_js(s)         ? DRAW_CTX_FLAG_JS         : 0)
        ;
    return flags;
}

/* sub ctx flags */
static inline bool
draw_subctx_div(DrawCtx ctx[_1_]) { return ctx->sub.flags & DRAW_SUBCTX_FLAG_DIV; }
static inline bool
draw_subctx_div_set(DrawCtx ctx[_1_]) { return ctx->sub.flags |= DRAW_SUBCTX_FLAG_DIV; }
static inline bool
draw_subctx_div_clear(DrawCtx ctx[_1_]) { return ctx->sub.flags &= ~DRAW_SUBCTX_FLAG_DIV; }
/***/
static inline Session* draw_ctx_session(DrawCtx ctx[_1_]) { return ctx->session; }
typedef Err (*DrawEffectCb)(DrawCtx ctx[_1_]);

static inline HtmlDoc* draw_ctx_htmldoc(DrawCtx ctx[_1_]) { return ctx->htmldoc; }

static inline SessionWriteFn draw_ctx_logfn(DrawCtx ctx[_1_]) { return ctx->logfn; }

static inline UrlClient* draw_ctx_url_client(DrawCtx ctx[_1_]) {
    return session_url_client(ctx->session);
}
static inline bool draw_ctx_hide_tags(DrawCtx ctx[_1_], size_t tags) {
    return *htmldoc_hide_tags(draw_ctx_htmldoc(ctx)) & tags;
}


static inline TextBuf* draw_ctx_textbuf(DrawCtx ctx[_1_]) {
    return htmldoc_textbuf(draw_ctx_htmldoc(ctx));
}



static inline BufOf(char)* draw_ctx_textbuf_buf_(DrawCtx ctx[_1_]) {
    return &draw_ctx_textbuf(ctx)->buf;
}

static inline bool draw_ctx_color(DrawCtx ctx[_1_]) { return !(ctx->flags & DRAW_CTX_FLAG_MONOCHROME); }
static inline bool draw_ctx_pre(DrawCtx ctx[_1_]) { return (ctx->flags & DRAW_CTX_FLAG_PRE); }
static inline void draw_ctx_pre_set(DrawCtx ctx[_1_], bool value) {
    if (value) ctx->flags |= DRAW_CTX_FLAG_PRE;
    else ctx->flags &= ~DRAW_CTX_FLAG_PRE;
}

static inline BufOf(char)* draw_ctx_buf(DrawCtx ctx[_1_]) { return &ctx->sub.buf; }

static inline TextBufMods* draw_ctx_mods(DrawCtx ctx[_1_]) { return &ctx->sub.mods; }
static inline size_t* draw_ctx_fragment_offset(DrawCtx ctx[_1_]) { 
    return &ctx->sub.fragment_offset;
}


static inline void draw_ctx_swap_sub(DrawCtx ctx[_1_], DrawSubCtx subctx[_1_]) {
    DrawSubCtx tmp = *subctx;
    *subctx = ctx->sub;
    ctx->sub = tmp;
}

        
static inline ArlOf(EscCode)* draw_ctx_esc_code_stack(DrawCtx ctx[_1_]) {
    return &ctx->esc_code_stack;
}

static inline Err draw_ctx_esc_code_push(DrawCtx ctx[_1_], EscCode code) {
    if (arlfn(EscCode, append)(draw_ctx_esc_code_stack(ctx), &code)) return Ok;
    return "error: arlfn append failure";
}

static inline Err draw_ctx_esc_code_pop(DrawCtx ctx[_1_]) {
    return arlfn(EscCode, pop)(draw_ctx_esc_code_stack(ctx)) ? Ok : "error: empty stack";
}

static inline EscCode* draw_ctx_esc_code_stack_backp(DrawCtx ctx[_1_]) {
    ArlOf(EscCode)* stack = draw_ctx_esc_code_stack(ctx);
    return arlfn(EscCode, back)(stack);
}

static inline Err
draw_ctx_buf_commit(DrawCtx ctx[_1_]) {
    BufOf(char)* buf = draw_ctx_buf(ctx);
    TextBuf* tb = draw_ctx_textbuf(ctx);

    if (len__(buf)) {
        BufOf(char)* tb_buf = textbuf_buf(tb);
        if (!buffn(char, append)(tb_buf, items__(buf), len__(buf)))
            return "error: could not append empty line to TextBuffer";
        buffn(char, reset)(buf);

        *textbuf_mods(draw_ctx_textbuf(ctx)) = *draw_ctx_mods(ctx);

        try( textbuf_append_line_indexes(tb));
        try( textbuf_append_null(tb));
        *textbuf_current_offset(tb) = *draw_ctx_fragment_offset(ctx);
    }
    return Ok;
}

static inline bool draw_ctx_buf_last_isgraph(DrawCtx ctx[_1_]) {
    Str* buf = draw_ctx_buf(ctx);
    return len__(buf) && items__(buf)[len__(buf) - 1];
}

static inline Err
draw_ctx_append_subctx(DrawCtx ctx[_1_], DrawSubCtx sub[_1_]) {
    if (sub->fragment_offset) *draw_ctx_fragment_offset(ctx)
        = len__(draw_ctx_buf(ctx)) + sub->fragment_offset;
    if (len__(&sub->mods))
        try( textmod_concatenate(draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), &sub->mods));
    return buffn(char, append)(
        draw_ctx_buf(ctx),
        sub->buf.items + sub->left_trim,
        sub->buf.len - sub->left_trim
    )
    ? Ok
    : "error: failed to append to buf (draw ctx)";
}

static inline Err
draw_ctx_buf_append_mem_mods(DrawCtx ctx[_1_], char* s, size_t len, TextBufMods* mods) { 
    if (mods) try( textmod_concatenate(draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), mods));
    return buffn(char, append)(draw_ctx_buf(ctx), s, len)
        ? Ok
        : "error: failed to append to bufof (draw ctx buf)";
}

static inline Err draw_ctx_buf_append(DrawCtx ctx[_1_], StrView s) {
    return draw_ctx_buf_append_mem_mods(ctx, (char*)s.items, s.len, NULL);
}


#define draw_ctx_buf_append_lit__(Ctx, Str) \
    draw_ctx_buf_append_mem_mods(Ctx, Str, sizeof(Str)-1, NULL)

static inline void draw_ctx_buf_reset(DrawCtx ctx[_1_]) {
    buffn(char, reset)(draw_ctx_buf(ctx));
}

static inline Err
draw_ctx_init(DrawCtx ctx[_1_], HtmlDoc htmldoc[_1_], Session s[_1_], unsigned flags) {
    //unsigned flags = (session_monochrome(s)? DRAW_CTX_FLAG_MONOCHROME: 0) | DRAW_CTX_FLAG_TITLE;
    char* fragment = NULL;
    try( url_fragment(htmldoc_url(htmldoc), &fragment));
    *ctx = (DrawCtx) {
        .htmldoc  = htmldoc,
        .fragment = fragment,
        .flags    = flags,
        .logfn    = session_doc_msg_fn(s, htmldoc),
        .session  = s
    };
    return Ok;
}

static inline void draw_ctx_cleanup(DrawCtx ctx[_1_]) {
    buffn(char, clean)(&ctx->sub.buf);
    std_free(ctx->fragment);
    arlfn(EscCode, clean)(draw_ctx_esc_code_stack(ctx));
}

static inline void draw_ctx_set_color(DrawCtx ctx[_1_], bool value) {
    ctx->flags = ctx->flags ^ (value ? 0 : DRAW_CTX_FLAG_MONOCHROME);
}

Err draw_rec(lxb_dom_node_t* node, DrawCtx ctx[_1_]);

static inline Err
draw_list( lxb_dom_node_t* it, lxb_dom_node_t* last, DrawCtx ctx[_1_]) {
    for(; it ; it = it->next) {
        try( draw_rec(it, ctx));
        if (it == last) break;
    }
    return Ok;
}


static inline Err
draw_list_block( lxb_dom_node_t* it, lxb_dom_node_t* last, DrawCtx ctx[_1_]) {
    Err err;
    DrawSubCtx sub = (DrawSubCtx){0};
    draw_ctx_swap_sub(ctx, &sub);

    err = draw_list(it, last, ctx);

    draw_ctx_swap_sub(ctx, &sub);

    if (!err && sub.buf.len) {
        ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        ok_then(err, draw_ctx_append_subctx(ctx, &sub));
        ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));

    }
    draw_subctx_clean(&sub);
    return Ok;
}

static inline Err draw_ctx_textmod(DrawCtx ctx[_1_], EscCode code) {
    if (draw_ctx_color(ctx)) 
        return textmod_append(
            draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), esc_code_to_text_mod(code));
    return Ok;
}
 
static inline Err draw_ctx_buf_append_color_(DrawCtx ctx[_1_], EscCode code) {
    if (draw_ctx_color(ctx)) {
        try( draw_ctx_esc_code_push(ctx, code));
        try( textmod_append(
                draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), esc_code_to_text_mod(code)));
    }
    return Ok;
}
 
#define _push_esc_code__ draw_ctx_buf_append_color_
static inline Err
draw_ctx_push_italic(DrawCtx ctx[_1_]) { return _push_esc_code__(ctx, esc_code_italic); }

static inline Err
draw_ctx_push_bold(DrawCtx ctx[_1_]) { return _push_esc_code__(ctx, esc_code_bold); }

static inline Err
draw_ctx_push_underline(DrawCtx ctx[_1_]){ return _push_esc_code__(ctx, esc_code_underline); }

static inline Err
draw_ctx_push_blue(DrawCtx ctx[_1_]) { return _push_esc_code__(ctx, esc_code_blue); }

static inline Err
draw_ctx_push_red(DrawCtx ctx[_1_]) { return _push_esc_code__(ctx, esc_code_red); }

static inline Err
draw_ctx_textmod_blue(DrawCtx ctx[_1_]) { return draw_ctx_textmod(ctx, esc_code_blue); }

static inline Err draw_ctx_color_red(DrawCtx ctx[_1_]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_red);
}

static inline Err draw_ctx_color_light_green(DrawCtx ctx[_1_]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_light_green);
}

static inline Err draw_ctx_color_purple(DrawCtx ctx[_1_]) {
    return draw_ctx_buf_append_color_(ctx, esc_code_purple);
}


static inline Err draw_ctx_buf_append_ui_base36_(DrawCtx ctx[_1_], uintmax_t ui) {
    Str* buf = draw_ctx_buf(ctx);
    return str_append_ui_as_base36(buf, ui);
}

static inline Err draw_ctx_reset_color(DrawCtx ctx[_1_]) {
    if (draw_ctx_color(ctx)) {
        try( textmod_append(draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), esc_code_to_text_mod(esc_code_reset)));
        ArlOf(EscCode)* stack = draw_ctx_esc_code_stack(ctx);
        try( draw_ctx_esc_code_pop(ctx));
        EscCode* backp =  arlfn(EscCode, back)(stack);
        if (backp) {
            try( textmod_append(draw_ctx_mods(ctx), len__(draw_ctx_buf(ctx)), esc_code_to_text_mod(*backp)));
        }
    }
    return Ok;
}

#endif
