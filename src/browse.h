#ifndef AHRE_BROWSE_H__
#define AHRE_BROWSE_H__

#include "src/escape_codes.h"
#include "src/htmldoc.h"
#include "src/utils.h"
#include "src/generic.h"

#define T EscCode
#include <arl.h>

typedef struct { 
    HtmlDoc* htmldoc;
    bool color;
    //bool empty;
    BufOf(char) buf;
    ArlOf(EscCode) esc_code_stack;
} BrowseCtx;

static inline HtmlDoc* browse_ctx_htmldoc(BrowseCtx ctx[static 1]) { return ctx->htmldoc; }

static inline TextBuf* browse_ctx_textbuf(BrowseCtx ctx[static 1]) {
    return htmldoc_textbuf(browse_ctx_htmldoc(ctx));
}

static inline BufOf(char)* browse_ctx_textbuf_buf_(BrowseCtx ctx[static 1]) {
    return &browse_ctx_textbuf(ctx)->buf;
}

static inline bool browse_ctx_color(BrowseCtx ctx[static 1]) { return ctx->color; }
//static inline bool* browse_ctx_empty(BrowseCtx ctx[static 1]) { return &ctx->empty; }
static inline BufOf(char)* browse_ctx_buf(BrowseCtx ctx[static 1]) { return &ctx->buf; }


static inline BufOf(char) browse_ctx_buf_get_reset(BrowseCtx ctx[static 1]) {
    BufOf(char) tmp = *browse_ctx_buf(ctx);
    ctx->buf = (BufOf(char)){0};
    return tmp;
}

static inline void browse_ctx_swap_buf(BrowseCtx ctx[static 1], BufOf(char) buf[static 1]) {
    BufOf(char) tmp = *buf;
    *buf = *browse_ctx_buf(ctx);
    ctx->buf = tmp;
}

        
//static inline bool browse_ctx_empty_get_set(BrowseCtx ctx[static 1], bool value) {
//    bool was_empty = *browse_ctx_empty(ctx);
//    *browse_ctx_empty(ctx) = value;
//    return was_empty;
//}

//static inline bool browse_ctx_empty_get_set_and(BrowseCtx ctx[static 1], bool value) {
//    bool was_empty = *browse_ctx_empty(ctx);
//    *browse_ctx_empty(ctx) &= value;
//    return was_empty;
//}

static inline ArlOf(EscCode)* browse_ctx_esc_code_stack(BrowseCtx ctx[static 1]) {
    return &ctx->esc_code_stack;
}

static inline Err browse_ctx_esc_code_push(BrowseCtx ctx[static 1], EscCode code) {
    if (arlfn(EscCode, append)(browse_ctx_esc_code_stack(ctx), &code)) return Ok;
    return "error: arlfn append failure";
}

static inline Err browse_ctx_esc_code_pop(BrowseCtx ctx[static 1]) {
    return arlfn(EscCode, pop)(browse_ctx_esc_code_stack(ctx)) ? Ok : "error: empty stack";
}

static inline EscCode* browse_ctx_esc_code_stack_backp(BrowseCtx ctx[static 1]) {
    ArlOf(EscCode)* stack = browse_ctx_esc_code_stack(ctx);
    return arlfn(EscCode, back)(stack);
}

static inline Err
browse_ctx_buf_commit(BrowseCtx ctx[static 1]) {
    BufOf(char)* buf = browse_ctx_buf(ctx);
    if (len__(buf)) {
        BufOf(char)* tb_buf = browse_ctx_textbuf_buf_(ctx);
        if (!buffn(char, append)(tb_buf, items__(buf), len__(buf)))
            return "error: could not append empty line to TextBuffer";
        buffn(char, reset)(buf);
    }
    return Ok;
}

static inline Err browse_ctx_buf_append(BrowseCtx ctx[static 1], char* s, size_t len) { 
    return buffn(char, append)(browse_ctx_buf(ctx), s, len)
        ? Ok
        : "error: failed to append to bufof (browse ctx buf)";
}

#define browse_ctx_buf_append_lit__(Ctx, Str) browse_ctx_buf_append(Ctx, Str, sizeof(Str)-1)

static inline void browse_ctx_buf_reset(BrowseCtx ctx[static 1]) {
    buffn(char, reset)(browse_ctx_buf(ctx));
}

static inline Err browse_ctx_init(BrowseCtx ctx[static 1], HtmlDoc htmldoc[static 1], bool color) {
    *ctx = (BrowseCtx) {.htmldoc=htmldoc, .color=color};
    ///*ctx = (BrowseCtx) {.htmldoc=htmldoc, .color=color, .empty=true};
    return Ok;
}

static inline void browse_ctx_cleanup(BrowseCtx ctx[static 1]) {
    buffn(char, clean)(&ctx->buf);
    arlfn(EscCode, clean)(browse_ctx_esc_code_stack(ctx));
}

static inline void browse_ctx_set_color(BrowseCtx ctx[static 1], bool value) {  ctx->color = value; }
Err browse_rec(lxb_dom_node_t* node, BrowseCtx ctx[static 1]);

static inline Err
browse_list( lxb_dom_node_t* it, lxb_dom_node_t* last, BrowseCtx ctx[static 1]) {
    for(; it ; it = it->next) {
        try( browse_rec(it, ctx));
        if (it == last) break;
    }
    return Ok;
}


static inline Err
browse_list_inline( lxb_dom_node_t* it, lxb_dom_node_t* last, BrowseCtx ctx[static 1]) {
    ///bool empty_so_far = browse_ctx_empty_get_set(ctx, true);
    for(; it ; it = it->next) {
        try( browse_rec(it, ctx));
        //if (!browse_ctx_empty_get_set(ctx, true)) empty_so_far = false;
        if (it == last) break;
    }
    ///*browse_ctx_empty(ctx) = empty_so_far;
    return Ok;
}

static inline Err
browse_list_block( lxb_dom_node_t* it, lxb_dom_node_t* last, BrowseCtx ctx[static 1]) {
    Err err;
    BufOf(char) buf = browse_ctx_buf_get_reset(ctx);

    for(; it ; it = it->next) {
        if ((err=browse_rec(it, ctx))) {
            buffn(char, clean)(&buf);
            return err;
        }
        if ( it == last ) break;
    }

    browse_ctx_swap_buf(ctx, &buf);
    if (buf.len) {
        if ((err=browse_ctx_buf_append_lit__(ctx, "\n"))) {
            buffn(char, clean)(&buf);
            return err;
        }
        if ((err=browse_ctx_buf_append(ctx, (char*)buf.items, buf.len))) {
            buffn(char, clean)(&buf);
            return err;
        }
        if ((err=browse_ctx_buf_append_lit__(ctx, "\n"))) {
            buffn(char, clean)(&buf);
            return err;
        }

    }
    buffn(char, clean)(&buf);
    return Ok;
}

static inline Err browse_ctx_buf_append_color_esc_code(BrowseCtx ctx[static 1], EscCode code) {
    if (browse_ctx_color(ctx)) {
        Str code_str;
        try( esc_code_to_str(code, &code_str));
        try( browse_ctx_buf_append(ctx, (char*)code_str.s, code_str.len));
    }
    return Ok;
}
 

static inline Err browse_ctx_buf_append_color_(BrowseCtx ctx[static 1], EscCode code) {
    if (browse_ctx_color(ctx)) {
        try( browse_ctx_esc_code_push(ctx, code));
        Str code_str;
        try( esc_code_to_str(code, &code_str));
        try( browse_ctx_buf_append(ctx, (char*)code_str.s, code_str.len));
    }
    return Ok;
}
 
static inline Err browse_ctx_buf_append_ui_(BrowseCtx ctx[static 1], uintmax_t ui) {
    BufOf(char)* buf = browse_ctx_buf(ctx);
    return bufofchar_append_ui_as_str(buf, ui);
}

static inline Err browse_ctx_buf_append_ui_base36_(BrowseCtx ctx[static 1], uintmax_t ui) {
    BufOf(char)* buf = browse_ctx_buf(ctx);
    return bufofchar_append_ui_base36_as_str(buf, ui);
}

static inline Err browse_ctx_reset_color(BrowseCtx ctx[static 1]) {
    if (browse_ctx_color(ctx)) {
        try( browse_ctx_buf_append_lit__(ctx, EscCodeReset));
        ArlOf(EscCode)* stack = browse_ctx_esc_code_stack(ctx);
        try( browse_ctx_esc_code_pop(ctx));
        EscCode* backp =  arlfn(EscCode, back)(stack);
        if (backp) {
            Str code_str;
            try( esc_code_to_str(*backp, &code_str));
            try( browse_ctx_buf_append(ctx, (char*)code_str.s, code_str.len));
        }
    }
    return Ok;
}

#endif
