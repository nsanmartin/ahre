#include "constants.h"
#include "draw.h"
#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "mem.h"
#include "textbuf.h"
#include "utils.h"
#include "wrapper-lexbor.h"
#include "session.h"
#include "writer.h"
#include "dom.h"


static inline
Err draw_ctx_esc_code_pop(DrawCtx ctx[_1_]) {
    return arlfn(EscCode, pop)(draw_ctx_esc_code_stack(ctx)) ? Ok : "error: empty stack";
}

/* internal linkage */
#define MAX_URL_LEN 2048u
#define READ_FROM_FILE_BUFFER_LEN 4096u
#define LAZY_STR_BUF_LEN 1600u

#define \
append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(DomNode,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")


static size_t 
_strview_trim_left_count_newlines_(StrView s[_1_]) {
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
    Str         buf;
    TextBufMods mods;
    size_t      flags;
    size_t      fragment_offset;
    size_t      left_newlines;
    size_t      left_trim;
    size_t      right_newlines;
} DrawTextBuf;

/* sub text flags */
static inline bool
draw_text_buf_div(DrawTextBuf text[_1_]) { return text->flags & DRAW_SUBCTX_FLAG_DIV; }


typedef Err (*DrawEffectCb)(DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
Err draw_rec(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static Err draw_rec_tag(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static Err draw_text(DomNode node,  DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static inline Err draw_list( DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static inline Err draw_list_block( DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);


static Err
draw_iter_childs(DomNode n, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_list(dom_node_first_child(n), dom_node_last_child(n), ctx, text);
}


static Err
draw_block_iter_childs(DomNode n, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_list_block(dom_node_first_child(n), dom_node_last_child(n), ctx, text);
}


static void
draw_text_buf_trim_right(DrawTextBuf sub[_1_]) {
    StrView content = strview_from_mem(sub->buf.items, sub->buf.len);
    sub->right_newlines = _strview_trim_right_count_newlines_(&content);
    sub->buf.len = content.len;
}

static Str*
draw_text_buf_buf(DrawTextBuf sub_text[_1_]) { return &sub_text->buf; }


static TextBufMods*
draw_text_buf_mods(DrawTextBuf text[_1_]) { return &text->mods; }


static size_t*
draw_text_buf_fragment_offset(DrawTextBuf text[_1_]) { return &text->fragment_offset; }


static void
draw_text_buf_clean(DrawTextBuf sub_text[_1_]) {
    /* std_free(sub->fragment); */
    buffn(char, clean)(&sub_text->buf);
    arlfn(ModAt, clean)(&sub_text->mods);
}


static Err
draw_text_buf_append_mem_mods(DrawTextBuf text[_1_], const char* s, size_t len, TextBufMods* mods) { 
    if (mods)
        try( textmod_concatenate(draw_text_buf_mods(text), len__(draw_text_buf_buf(text)), mods));
    return buffn(char, append)(draw_text_buf_buf(text), (char*)s, len)
        ? Ok
        : "error: failed to append to bufof (draw ctx buf)";
}

static Err draw_text_buf_append(DrawTextBuf text[_1_], StrView s) {
    return draw_text_buf_append_mem_mods(text, s.items, s.len, NULL);
}

#define \
draw_text_buf_append_lit__(Ctx, Str) \
    draw_text_buf_append_mem_mods(Ctx, Str, sizeof(Str)-1, NULL)

static Err
draw_text_buf_commit(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    Str* buf = draw_text_buf_buf(text);
    TextBuf* tb = draw_ctx_textbuf(ctx);

    if (len__(buf)) {
        Str* tb_buf = textbuf_buf(tb);
        /* Do not swap memory here since the beg pointer of the source
         * may have been displaced
         */
        if (!buffn(char, append)(tb_buf, items__(buf), len__(buf)))
            return "error: could not append empty line to TextBuffer";
        buffn(char, reset)(buf);

        *textbuf_mods(draw_ctx_textbuf(ctx)) = *draw_text_buf_mods(text);
        *draw_text_buf_mods(text) = (TextBufMods){0};


        try( textbuf_append_line_indexes(tb));
        try( textbuf_append_null(tb));
        *textbuf_current_offset(tb) = *draw_text_buf_fragment_offset(text);
    }
    return Ok;
}


static Err
draw_ctx_append_sub_text(DrawTextBuf text[_1_], DrawTextBuf sub[_1_]) {
    if (sub->fragment_offset)
        *draw_text_buf_fragment_offset(text) = len__(draw_text_buf_buf(text)) + sub->fragment_offset;
    if (len__(&sub->mods))
        try( textmod_concatenate(
            draw_text_buf_mods(text), len__(draw_text_buf_buf(text)), &sub->mods
        ));
    return buffn(char, append)(
        draw_text_buf_buf(text),
        sub->buf.items + sub->left_trim,
        sub->buf.len - sub->left_trim
    )
    ? Ok
    : "error: failed to append to buf (draw ctx)";
}


static bool
draw_text_buf_last_isalnum(DrawTextBuf text[_1_]) {
    Str* buf = draw_text_buf_buf(text);
    return len__(buf) 
        && isalnum(items__(buf)[len__(buf) - 1]);
}


static Err
draw_text_buf_textmod(DrawCtx ctx[_1_], DrawTextBuf text[_1_], EscCode code) {
    if (draw_ctx_color(ctx)) 
        return textmod_append(
            draw_text_buf_mods(text), len__(draw_text_buf_buf(text)), esc_code_to_text_mod(code));
    return Ok;
}


static Err
draw_ctx_esc_code_push(DrawCtx ctx[_1_], EscCode code) {
    if (arlfn(EscCode, append)(draw_ctx_esc_code_stack(ctx), &code)) return Ok;
    return "error: arlfn append failure";
}

 
static Err
draw_text_buf_append_color_(DrawCtx ctx[_1_], DrawTextBuf text[_1_], EscCode code) {
    if (draw_ctx_color(ctx)) {
        try( draw_ctx_esc_code_push(ctx, code));
        try( textmod_append(
            draw_text_buf_mods(text), len__(draw_text_buf_buf(text)), esc_code_to_text_mod(code)));
    }
    return Ok;
}

 
static inline
Err draw_ctx_reset_color(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (draw_ctx_color(ctx)) {
        try( textmod_append(
                draw_text_buf_mods(text),
                len__(draw_text_buf_buf(text)),
                esc_code_to_text_mod(esc_code_reset)
        ));
        ArlOf(EscCode)* stack = draw_ctx_esc_code_stack(ctx);
        try( draw_ctx_esc_code_pop(ctx));
        EscCode* backp =  arlfn(EscCode, back)(stack);
        if (backp) {
            try( textmod_append(
                    draw_text_buf_mods(text),
                    len__(draw_text_buf_buf(text)),
                    esc_code_to_text_mod(*backp)
            ));
        }
    }
    return Ok;
}


#define _push_esc_code__ draw_text_buf_append_color_ 
static inline Err
draw_ctx_push_italic(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return _push_esc_code__(ctx, text, esc_code_italic);
}


static inline Err
draw_ctx_push_bold(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return _push_esc_code__(ctx, text, esc_code_bold);
}


static inline Err
draw_ctx_push_underline(DrawCtx ctx[_1_], DrawTextBuf text[_1_]){
    return _push_esc_code__(ctx, text, esc_code_underline);
}


static inline Err __attribute__((unused))
draw_ctx_push_blue(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return _push_esc_code__(ctx, text, esc_code_blue);
}

static inline Err __attribute__((unused))
draw_ctx_push_red(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return _push_esc_code__(ctx, text, esc_code_red);
}


static inline Err
draw_text_buf_textmod_blue(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_text_buf_textmod(ctx, text, esc_code_blue);
}


static inline Err draw_ctx_color_red(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_text_buf_append_color_(ctx, text, esc_code_red);
}


static inline Err draw_ctx_color_light_green(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_text_buf_append_color_(ctx, text, esc_code_light_green);
}


static inline Err
draw_ctx_color_purple(DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_text_buf_append_color_(ctx, text, esc_code_purple);
}


static inline Err
draw_text_buf_append_ui_base36_(DrawTextBuf text[_1_], uintmax_t ui) {
    Str* buf = draw_text_buf_buf(text);
    return str_append_ui_as_base36(buf, ui);
}

        
/* end of DrawTextBuf */


static Err
draw_tag_a(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
Err draw_tag_pre(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);


static void
draw_text_buf_trim_left(DrawTextBuf sub[_1_]) {
    StrView content = strview_from_mem(sub->buf.items, sub->buf.len);
    sub->left_newlines = _strview_trim_left_count_newlines_(&content);
    sub->left_trim = sub->buf.len-content.len;
    textmod_trim_left(&sub->mods, sub->left_trim);
}


static Err
_hypertext_id_open_(
    DrawCtx         ctx[_1_],
    DrawTextBuf     text[_1_],
    DrawEffectCb    visual_effect,
    StrViewProvider open_str_provider,
    const size_t*   id_num_ptr,
    StrViewProvider sep_str_provider
) {
    if (visual_effect) try( visual_effect(ctx, text));
    if (open_str_provider) try( draw_text_buf_append(text, open_str_provider()));
    if (id_num_ptr) try( draw_text_buf_append_ui_base36_(text, *id_num_ptr));
    if (sep_str_provider) try( draw_text_buf_append(text, sep_str_provider()));
    return Ok;
}


static Err
_hypertext_open_(
    DrawCtx         ctx[_1_],
    DrawTextBuf     text[_1_],
    DrawEffectCb    visual_effect,
    StrViewProvider prefix_str_provider
) {
    if (visual_effect) try( visual_effect(ctx, text));
    if (prefix_str_provider) try( draw_text_buf_append(text, prefix_str_provider()));
    return Ok;
}


static Err
_hypertext_id_close_(
    DrawCtx         ctx[_1_],
    DrawTextBuf     text[_1_],
    DrawEffectCb    visual_effect,
    StrViewProvider close_str_provider
) {
    if (close_str_provider) try( draw_text_buf_append(text, close_str_provider()));
    if (visual_effect) try( visual_effect(ctx, text));
    return Ok;
}


static Err
_hypertext_close_(
    DrawCtx         ctx[_1_],
    DrawTextBuf     text[_1_],
    DrawEffectCb    visual_effect,
    StrViewProvider postfix_provider
) {
    if (visual_effect) try( visual_effect(ctx, text));
    if (postfix_provider) try( draw_text_buf_append(text, postfix_provider()));
    return Ok;
}


Err
draw_rec(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (!isnull(node)) {
        switch(dom_node_type(node)) {
            case DOM_NODE_TYPE_ELEMENT: return draw_rec_tag(node, ctx, text);
            case DOM_NODE_TYPE_TEXT: return draw_text(node, ctx, text);
            //TODO: do not ignore these types?
            case DOM_NODE_TYPE_DOCUMENT: 
            case DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case DOM_NODE_TYPE_COMMENT:
                return draw_iter_childs(node, ctx, text);
            default: {
                if (!dom_node_type_is_valid(node))
                    return "error:""dom node type is invalid";
                /* else TODO: "Ignored Node Type: %s\n", _dbg_node_types_[node->type]*/
                return Ok;
            }
        }
    }
    return Ok;
}


static inline Err
draw_list (DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    for(; !isnull(it) ; it = dom_node_next(it)) {
        try( draw_rec(it, ctx, text));
        if (dom_node_eq(it, last)) break;
    }
    return Ok;
}


static Err
draw_tag_br(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    try( draw_text_buf_append_lit__(text, "\n"));
    return draw_iter_childs(node, ctx, text);
}


static Err
draw_tag_center(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    //TODO: store center boundaries (start = len on enter, end = len on ret) and then
    // when fitting to width center those lines image.

    //try( draw_text_buf_append_lit__(ctx, "%{[center]:\n"));
    try( draw_iter_childs(node, ctx, text));
    //try( draw_text_buf_append_lit__(ctx, "%}[center]\n"));
    return Ok;
}


static Err
browse_ctx_append_img_alt_(DomNode img, DrawTextBuf text[_1_]) {

    StrView alt = dom_node_attr_value(img, svl("alt"));
    if (alt.items && alt.len)
        try( draw_text_buf_append(text, alt));

    return Ok;
}

 
static Err
draw_tag_img(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(DomNode)* imgs = htmldoc_imgs(d);
    const size_t img_count = len__(imgs);

    try( append_to_arlof_lxb_node__(imgs, &node));

    try( _hypertext_id_open_(
            ctx, text, draw_ctx_color_light_green, image_open_str, &img_count, image_close_str));

    try( browse_ctx_append_img_alt_(node, text));
    try( draw_iter_childs(node, ctx, text));
    try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, NULL));
    return Ok;
}


static Err
draw_tag_select(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    DomNode selected = dom_node_first_child(node);
    if (isnull(selected)) return Ok;
    for(DomNode it = selected; !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_type(it) == DOM_NODE_TYPE_ELEMENT 
            && dom_node_tag(it) == HTML_TAG_OPTION
            && dom_node_has_attr(it, svl("selected"))
        ) {
            selected = it;
            break;
        }
    }

    ArlOf(DomNode)* inputs = htmldoc_inputs(draw_ctx_htmldoc(ctx));
    if (!arlfn(DomNode,append)(inputs, &node)) { return "error: arl set"; }
    size_t input_id = len__(inputs)-1;
    try( _hypertext_id_open_(
        ctx, text, draw_ctx_color_red, input_text_open_str, &input_id, input_select_sep_str));

    for(DomNode txt = dom_node_first_child(selected); !isnull(txt) ; txt = dom_node_next(txt)) {
        StrView node_text = dom_node_text_view(txt);
        if (node_text.len) try( draw_text_buf_append(text, node_text));
    }
    try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, input_submit_close_str));

    return Ok;
}


static Err
draw_tag_form(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    ArlOf(DomNode)* forms = htmldoc_forms(draw_ctx_htmldoc(ctx));
    if (!arlfn(DomNode,append)(forms, &node)) return "error: lip set";
    const size_t form_count = len__(forms);

    bool show = session_conf_show_forms(session_conf(draw_ctx_session(ctx)));
    if (show) {
        try( _hypertext_id_open_(
                ctx, text, draw_ctx_color_purple, form_open_str, &form_count, form_sep_str));

        try( draw_ctx_reset_color(ctx, text));
    }

    try( draw_block_iter_childs(node, ctx, text));

    if (show) {
        try( draw_text_buf_append_color_(ctx, text, esc_code_purple));
        try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, form_close_str));
    }
    return Ok;
}


static Err
draw_tag_button(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(DomNode)* inputs = htmldoc_inputs(d);
    if (!arlfn(DomNode,append)(inputs, &node)) return "error: lip set";
    size_t input_id =len__(inputs)-1;

    try( _hypertext_id_open_(
        ctx, text, draw_ctx_color_red, button_open_str, &input_id, button_sep_str));
    

    try( draw_iter_childs(node, ctx, text));

    try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, button_close_str));
    return Ok;
}


static Err draw_tag_input(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {

    StrView type = dom_node_attr_value(node, svl("type"));
    if (!strncmp("hidden", type.items, type.len))
        return Ok;

    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(DomNode)* inputs = htmldoc_inputs(d);
    if (!arlfn(DomNode,append)(inputs, &node)) { return "error: arl set"; }
    size_t input_id = len__(inputs)-1;


    /* submit */
    if (str_eq_case(type, svl("submit"))) {

        StrView value = dom_node_attr_value(node, svl("value"));

        StrViewProvider sep = value.len ? input_submit_sep_str : NULL;
        try( _hypertext_id_open_(ctx, text, draw_ctx_color_red, input_text_open_str, &input_id, sep));
        if (value.len) try( draw_text_buf_append(text, value));
        try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, input_submit_close_str));
        return Ok;

    } 

    /* other */
    try( _hypertext_id_open_(
        ctx, text, draw_ctx_color_red, input_text_open_str, &input_id, NULL));

    /* "input" type is text | search */
    if (str_eq_case(svl("text"), type) || str_eq_case(svl("search"), type)) {
        try( draw_text_buf_append_lit__(text, "="));
        StrView value = dom_node_attr_value(node, svl("value"));
        if (value.len) try( draw_text_buf_append(text, value));

    } else if (str_eq_case(svl("password"), type)) {

        StrView value = dom_node_attr_value(node, svl("value"));
        if (value.len) {
            try( draw_text_buf_append_lit__(text, "=********"));
        } else try( draw_text_buf_append_lit__(text, "=________"));
    } else {
        try( draw_text_buf_append_lit__(text, "[input not supported yet]"));
    }
    try( _hypertext_id_close_(ctx, text, draw_ctx_reset_color, input_text_close_str));
    return Ok;
}


static Err draw_tag_div(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    DrawTextBuf sub = (DrawTextBuf){.flags=DRAW_SUBCTX_FLAG_DIV};

    Err err = draw_iter_childs(node, ctx, &sub);

    if (!err && sub.buf.len) {
        if (len__(draw_text_buf_buf(text))) {
            ok_then(err, draw_text_buf_append_lit__(text, "\n"));
        }
        ok_then(err, draw_ctx_append_sub_text(text, &sub));
    }
        
    draw_text_buf_clean(&sub);
    return err;
}


static Err
draw_tag_p(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_block_iter_childs(node, ctx, text);
}


static Err
draw_tag_tr(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    try( draw_text_buf_append_lit__(text, "\n"));
    try( draw_iter_childs(node, ctx, text));
    return Ok;
}


static Err
draw_tag_ul(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    return draw_block_iter_childs(node,  ctx, text);
}


static Err
draw_tag_li(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    DrawTextBuf sub = (DrawTextBuf){0};

    Err err = draw_iter_childs(node, ctx, &sub);
    if (!err) draw_text_buf_trim_left(&sub);

    if (!err && sub.buf.len) {
        ok_then(err, draw_text_buf_append_lit__(text, " * "));
        ok_then(err, draw_ctx_append_sub_text(text, &sub));
        if (!err && sub.buf.items[sub.buf.len-1] != '\n')
            ok_then(err, draw_text_buf_append_lit__(text, "\n"));
    }

    draw_text_buf_clean(&sub);
    return Ok;
}


static Err
draw_tag_h(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    DrawTextBuf sub = (DrawTextBuf){0};

    try( draw_block_iter_childs(node,  ctx, &sub));

    draw_text_buf_trim_left(&sub);
    draw_text_buf_trim_right(&sub);

    Err err = _hypertext_open_(ctx, text, draw_ctx_push_bold, h_tag_open_str);
    if (!err && sub.buf.len) ok_then(err, draw_ctx_append_sub_text(text, &sub));
    ok_then(err, _hypertext_close_(ctx, text, draw_ctx_reset_color, newline_str));

    draw_text_buf_clean(&sub);
    return Ok;
}


static Err
draw_tag_code(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {

    DrawTextBuf sub = (DrawTextBuf){0};

    try (draw_iter_childs(node, ctx, &sub));

    bool contains_newline = str_contains(draw_text_buf_buf(&sub), '\n');

    Err err = Ok;
    if (contains_newline)  err = draw_text_buf_append_lit__(text, "\n```code:\n");
    else err = draw_text_buf_append_lit__(text, " `");

    ok_then(err, draw_ctx_append_sub_text(text, &sub));

    if (contains_newline)  ok_then(err, draw_text_buf_append_lit__(text, "\n```\n"));
    else ok_then(err, draw_text_buf_append_lit__(text, "` "));

    draw_text_buf_clean(&sub);

    return Ok;
}


static Err
draw_tag_b(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (!isnull(dom_node_prev(node)) && draw_text_buf_last_isalnum(text))
        try( draw_text_buf_append_lit__(text, " "));
    try(_hypertext_open_(ctx, text, draw_ctx_push_bold, space_str));
    try( draw_iter_childs(node, ctx, text));
    try( _hypertext_close_(ctx, text, draw_ctx_reset_color, space_str));
    return Ok;
}


static Err
draw_tag_separating_with_space(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (!isnull(dom_node_prev(node)) && draw_text_buf_last_isalnum(text))
        try( draw_text_buf_append_lit__(text, " "));
    try( draw_iter_childs(node, ctx, text));
    return Ok;
}


static Err
draw_tag_em(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (!isnull(dom_node_prev(node)) && draw_text_buf_last_isalnum(text))
        try( draw_text_buf_append_lit__(text, " "));
    try(_hypertext_open_(ctx, text, draw_ctx_push_underline, space_str));
    try( draw_iter_childs(node, ctx, text));
    try( _hypertext_close_(ctx, text, draw_ctx_reset_color, space_str));
    return Ok;
}


static Err
draw_tag_i(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    try(_hypertext_open_(ctx, text, draw_ctx_push_italic, space_str));
    try( draw_iter_childs(node, ctx, text));
    try( _hypertext_close_(ctx, text, draw_ctx_reset_color, space_str));
    return Ok;
}


static Err
draw_tag_blockquote(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    //TODO: use `>  ` or `   ` instead?
    try( draw_text_buf_append_lit__(text, "``"));
    try (draw_block_iter_childs(node, ctx, text));
    try( draw_text_buf_append_lit__(text, "''"));
    return Ok;
}


static Err
draw_tag_title(DomNode node, DrawCtx ctx[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    *htmldoc_title(d) = node;
    return Ok;
}


static Err
draw_strview_skipping_space(StrView s, DomNode prev, DrawTextBuf text[_1_]) {
    /* if it starts with a punctuation mark we undo the space added in draw space */
    if (!isnull(prev) && !ispunct(s.items[0])) try( draw_text_buf_append_lit__(text, " "));
    while(s.len) {
        StrView word = strview_split_word(&s);
        if (!word.len) break;
        try( draw_text_buf_append(text, word));
        strview_trim_space_left(&s);
        if (!s.len) break;
        try( draw_text_buf_append_lit__(text, " "));
    }
    return Ok;
}


/*
 * This implementation was a quick solution to just render the text parts without caring 
 * too much about the DOM specification, and we started using a context to be able to pass
 * some information to the recursive calls. It should be progressively replaced by functions
 * that instead of calling the same draw_rec_tag, resolve only the element they are made to
 * draw.
 * 
 */
static Err
draw_rec_tag(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    if (ctx->fragment
    && str_eq_case(sv(ctx->fragment), dom_node_attr_value(node, svl("id")))) {
        *draw_text_buf_fragment_offset(text) = len__(draw_text_buf_buf(text));
    }
    switch(dom_node_tag(node)) {
        case HTML_TAG_A: { return draw_tag_a(node, ctx, text); }
        case HTML_TAG_B: { return draw_tag_b(node, ctx, text); }
        case HTML_TAG_BLOCKQUOTE: { return draw_tag_blockquote(node, ctx, text); }
        case HTML_TAG_BR: { return draw_tag_br(node, ctx, text); }
        case HTML_TAG_BUTTON: { return draw_tag_button(node, ctx, text); }
        case HTML_TAG_CENTER: { return draw_tag_center(node, ctx, text); } 
        case HTML_TAG_CODE: { return draw_tag_code(node, ctx, text); } 
        case HTML_TAG_DIV: { return draw_tag_div(node, ctx, text); }
        case HTML_TAG_DL: { return draw_block_iter_childs(node, ctx, text); }
        case HTML_TAG_DT: { return draw_block_iter_childs(node, ctx, text); }
        case HTML_TAG_EM: { return draw_tag_em(node, ctx, text); }
        case HTML_TAG_FORM: { return draw_tag_form(node, ctx, text); }
        case HTML_TAG_H1: case HTML_TAG_H2: case HTML_TAG_H3: case HTML_TAG_H4: case HTML_TAG_H5: case HTML_TAG_H6: { return draw_tag_h(node, ctx, text); }
        case HTML_TAG_I: { return draw_tag_i(node, ctx, text); }
        case HTML_TAG_INPUT: { return draw_tag_input(node, ctx, text); }
        case HTML_TAG_IMAGE: case HTML_TAG_IMG: { return draw_tag_img(node, ctx, text); }
        case HTML_TAG_LI: { return draw_tag_li(node, ctx, text); }
        case HTML_TAG_META: { return Ok; }
        case HTML_TAG_OL: { return draw_tag_ul(node, ctx, text); }
        case HTML_TAG_P: { return draw_tag_p(node, ctx, text); }
        case HTML_TAG_PRE: { return draw_tag_pre(node, ctx, text); }
        case HTML_TAG_SCRIPT: {
            return Ok; /*
                          draw_tag_script(node, ctx, text);
                          Here draw only body scripts
                          */
        } 
        case HTML_TAG_SELECT: { return draw_tag_select(node, ctx, text); }
        case HTML_TAG_STYLE: {
            /* Does it make any sense that wo dosomething with this in ahre? */
            return Ok;
        } 
        case HTML_TAG_TITLE: { return draw_tag_title(node, ctx); } 
        case HTML_TAG_TR: { return draw_tag_tr(node, ctx, text); }
        case HTML_TAG_TT: { return draw_tag_code(node, ctx, text); }
        case HTML_TAG_UL: { return draw_tag_ul(node, ctx, text); }
        //TODO: implement all these tags, meanwhile we just add
        // spaces when needed to separate from previous word
        case HTML_TAG_TD: case HTML_TAG_TH: //TODO delete once table is implemented
        case HTML_TAG_TIME:
        case HTML_TAG_SAMP:
        case HTML_TAG_SMALL:
        case HTML_TAG_SPAN:
        case HTML_TAG_STRONG:
        case HTML_TAG_VAR: { return draw_tag_separating_with_space(node, ctx, text); }
        default: {
            /* if (node->local_name >= HTML_TAG__LAST_ENTRY) */
            /*     return err_fmt( */
            /*         "error: node local name (TAG) greater than last entry: %lx\n", node->local_name */
            /*     ); */
            /* else TODO: "TAG 'NOT' IMPLEMENTED: %s", node->local_name */
            return draw_iter_childs(node, ctx, text);
        }
    }
}


static Err
draw_text(DomNode node,  DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    StrView data = dom_node_text_view(node);
    if (!data.len) return Ok;

    if (draw_ctx_pre(ctx)) {
        //TODO Whitespace inside this element is displayed as written,
        //with one exception. If one or more leading newline characters
        //are included immediately following the opening <pre> tag, the
        //first newline character is stripped. 
        //https://developer.mozilla.org/en-US/docs/Web/HTML/Element/pre
        try( draw_text_buf_append(text, data));
    } else if (strview_skip_space_inplace(&data)) {
        /* If it's not the first then separate with previous with space */
        try( draw_strview_skipping_space(data, dom_node_prev(node), text));
    } 

    if (!isnull(dom_node_first_child(node)) || !isnull(dom_node_last_child(node)))
        return "error: unexpected text elem with childs";
    return Ok;
}


static inline Err
draw_list_block(DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    DrawTextBuf sub = (DrawTextBuf){0};

    Err err = draw_list(it, last, ctx, &sub);
    if (!err && sub.buf.len) {
        ok_then(err, draw_text_buf_append_lit__(text, "\n"));
        ok_then(err, draw_ctx_append_sub_text(text, &sub));
        ok_then(err, draw_text_buf_append_lit__(text, "\n"));

    }
    draw_text_buf_clean(&sub);
    return Ok;
}


Err
draw_tag_pre(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    draw_ctx_pre_set(ctx, true);
    try( draw_block_iter_childs(node, ctx, text));
    draw_ctx_pre_set(ctx, false);
    return Ok;
}


/*TODO: decouple sesion and htmldoc here */
Err
htmldoc_draw_with_flags(HtmlDoc htmldoc[_1_], Session* s, unsigned flags) {
    if (!s) return "error: expecting not null Session";
    Dom dom = htmldoc_dom(htmldoc);
    DrawCtx     ctx;
    DrawTextBuf text = (DrawTextBuf){0};
    Err err = draw_ctx_init(&ctx, htmldoc, s, flags);
    try(err);
    ok_then(err, draw_rec(dom_root(dom), &ctx, &text));
    ok_then(err, draw_text_buf_commit(&ctx, &text));
    ok_then(err, textbuf_fit_lines(htmldoc_textbuf(htmldoc), *session_ncols(s)));

    draw_ctx_cleanup(&ctx);
    draw_text_buf_clean(&text);
    return err;
}


static Err
_htmldoc_draw_(HtmlDoc htmldoc[_1_], Session s[_1_]) {
    unsigned flags = draw_ctx_flags_from_session(s) | DRAW_CTX_FLAG_TITLE;
    return htmldoc_draw_with_flags(htmldoc, s, flags);
}


#define HTMLDOC_FLAG_JS 0x1u


Err
htmldoc_init_move_request(
    HtmlDoc   d[_1_],
    Request   r[_1_],
    UrlClient uc[_1_],
    Session*  s,
    CmdOut*   cmd_out
) {
    if (!s) return "error: expecting a session, recived NULL";
    Err e = Ok;
    unsigned flags = session_js(s) ? HTMLDOC_FLAG_JS : 0x0;

    /* lexbor doc should be initialized before jse_init */
    *d = (HtmlDoc){
        .req    = *r,
    };
    try(dom_init(&d->dom));
    if (flags & HTMLDOC_FLAG_JS) 
        try_or_jump(e, Failure, jse_init(d));

    ArlOf(FetchHistoryEntry)* hist = session_fetch_history(s);
     if (!arlfn(FetchHistoryEntry,append)(hist, &(FetchHistoryEntry){0}))
     { e = "error: arl append to fetch history failure"; goto Failure; }

    try_or_jump(e, Failure, htmldoc_fetch(d, uc, cmd_out, arlfn(FetchHistoryEntry,back)(hist)));
    htmldoc_eval_js_scripts_or_continue(d, s, cmd_out);
    try_or_jump( e, Failure, _htmldoc_draw_(d, s));
    try_or_jump( e, Failure,
        fetch_history_entry_update_title(arlfn(FetchHistoryEntry,back)(hist),htmldoc_title(d)));

    *r = (Request){0};
    return Ok;

Failure:
    /* In case of failure, the caller keeps ownership. In case of success it loses it */
    *r = d->req;
    d->req = (Request){0};
    htmldoc_cleanup(d);

    return e;
}


Err
htmldoc_init_bookmark_move_urlstr(HtmlDoc d[_1_], Str urlstr[_1_]) {
    Err e = Ok;


    *d = (HtmlDoc){ 0 };
    try(dom_init(&d->dom));
    if (!urlstr) return "error: cannot initialize bookmark with not path";
    try_or_jump(e, Failure_Lxb_Html_Document_Destroy,
        request_init_move_urlstr(htmldoc_request(d), http_get, urlstr, NULL));
    return Ok;

Failure_Lxb_Html_Document_Destroy:
    dom_cleanup(d->dom);
    return e;
}


/* external linkage */


void
htmldoc_reset_draw(HtmlDoc htmldoc[_1_]) {
    textbuf_reset(htmldoc_textbuf(htmldoc));
    arlfn(DomNode,clean)(htmldoc_anchors(htmldoc));
    arlfn(DomNode,clean)(htmldoc_imgs(htmldoc));
    arlfn(DomNode,clean)(htmldoc_inputs(htmldoc));
    arlfn(DomNode,clean)(htmldoc_forms(htmldoc));
}


static void
htmldoc_fetchcache_cleanup(HtmlDoc htmldoc[_1_]) {
    textbuf_cleanup(htmldoc_sourcebuf(htmldoc));
    arlfn(Str,clean)(htmldoc_head_scripts(htmldoc));
    arlfn(Str,clean)(htmldoc_body_scripts(htmldoc));
    htmldoc->fetch_cache = (DocFetchCache){0};
}


static void
htmldoc_drawcache_cleanup(HtmlDoc htmldoc[_1_]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    arlfn(DomNode,clean)(htmldoc_anchors(htmldoc));
    arlfn(DomNode,clean)(htmldoc_imgs(htmldoc));
    arlfn(DomNode,clean)(htmldoc_inputs(htmldoc));
    arlfn(DomNode,clean)(htmldoc_forms(htmldoc));

    str_clean(htmldoc_screen(htmldoc));
    htmldoc->draw_cache = (DocDrawCache){0};
}


void
htmldoc_cache_cleanup(HtmlDoc htmldoc[_1_]) {
    htmldoc_drawcache_cleanup(htmldoc);
    htmldoc_fetchcache_cleanup(htmldoc);
}


void
htmldoc_cleanup(HtmlDoc htmldoc[_1_]) {
    http_header_clean(htmldoc_http_header(htmldoc));
    htmldoc_cache_cleanup(htmldoc);
    dom_cleanup(htmldoc_dom(htmldoc));
    url_cleanup(htmldoc_url(htmldoc));
    request_clean(htmldoc_request(htmldoc));

    if (jse_rt(htmldoc_js(htmldoc))) jse_clean(htmldoc_js(htmldoc));
}


void
htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}


Err
htmldoc_A(Session* s, HtmlDoc d[_1_], CmdOut* out) {
    if (!s) return "error: no session";
    msg__(out, svl("<li><a href=\""));
    char* url_buf;
    try( url_cstr_malloc(*htmldoc_url(d), &url_buf));
    msg__(out, url_buf);
    w_curl_free(url_buf);
    msg__(out, svl("\">"));
    /* try( dom_get_title_text_line(*htmldoc_title(d), msg_str(cmd_out_msg(out)))); */
    try(strview_join_lines_to_str(
        dom_node_text_view(*htmldoc_title(d)),
        msg_str(cmd_out_msg(out))
    ));
    msg__(out, svl("</a>"));
    msg__(out, svl("\n"));
    return Ok;
}


Err
htmldoc_print_info(HtmlDoc d[_1_], CmdOut* out) {
    Err err = Ok;
    DomNode* title = htmldoc_title(d);

    try (msg__(out, svl("DOWNLOAD SIZE: ")));
    try (cmd_out_msg_append_ui_as_base10(out, *htmldoc_curlinfo_sz_download(d)));
    try (msg__(out, svl("\n")));
    try (msg__(out, svl("html size: ")));
    try (cmd_out_msg_append_ui_as_base10(out, htmldoc_sourcebuf(d)->buf.len));
    try (msg__(out, svl("\n")));

    if (title) {
        Str buf = (Str){0};
        try(strview_join_lines_to_str(dom_node_text_view(*htmldoc_title(d)), &buf));
        if (!buf.len) err = msg__(out,  svl("<NO TITLE>"));
        else err = msg__(out, buf);
        str_clean(&buf);
        try(err);
    }

    try(msg__(out, svl("\n")));
    
    char* url = NULL;
    ok_then(err, url_cstr_malloc(*htmldoc_url(d), &url));
    if (url) {
        ok_then(err, msg_ln__(out, url));
        w_curl_free(url);
    }

    Str* charset = htmldoc_http_charset(d);
    if (len__(charset))
        ok_then(err, msg_ln__(out, charset));

    Str* content_type = htmldoc_http_content_type(d);
    if (len__(content_type))
        ok_then(err, msg_ln__(out, content_type));

    return err;
}


static bool
_prev_is_separable_(DomNode n) {
    DomNode prev = dom_node_prev(n);
    return !isnull(prev) && dom_node_tag(prev) != HTML_TAG_LI ;
}


static Err
draw_tag_a(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    /* https://html.spec.whatwg.org/multipage/links.html#attr-hyperlink-href
     * The href attribute on a and area elements is not required; when those
     * elements do not have href attributes they do not create hyperlinks. */
    bool is_hyperlink = dom_node_has_attr(node, svl("href"));
    if (is_hyperlink) {

        HtmlDoc*           d          = draw_ctx_htmldoc(ctx);
        ArlOf(DomNode)* anchors    = htmldoc_anchors(d);
        const size_t       anchor_num = anchors->len;
        DrawTextBuf*       sub_text   = &(DrawTextBuf){0};

        if (!arlfn(DomNode,append)(anchors, &node)) 
            return "error: lip set";

        if (draw_ctx_color(ctx)) try( draw_ctx_esc_code_push(ctx, esc_code_blue));

        Err err = draw_iter_childs(node, ctx, sub_text);

        if (err) {
            draw_text_buf_clean(sub_text);
            return err;
        }

        bool div_child = draw_text_buf_div(sub_text); /* check whether any child is div */

        draw_text_buf_trim_left(sub_text);
        draw_text_buf_trim_right(sub_text);

        if (_prev_is_separable_(node)) err = draw_text_buf_append_lit__(text, " ");
        else if (text->left_newlines) err = draw_text_buf_append_lit__(text, "\n");
        ok_then(err, _hypertext_id_open_(
            ctx, text, draw_text_buf_textmod_blue, anchor_open_str, &anchor_num,anchor_sep_str ));

        if (!err && sub_text->buf.len) ok_then(err, draw_ctx_append_sub_text(text, sub_text));
        draw_text_buf_clean(sub_text);

        ok_then(err, _hypertext_id_close_(ctx, text, draw_ctx_reset_color, anchor_close_str));
        if (text->right_newlines) ok_then(err, draw_text_buf_append_lit__(text, "\n"));
        if (div_child) ok_then(err, draw_text_buf_append_lit__(text, "\n"));

        try(err);

    } else try( draw_iter_childs(node, ctx, text));//TODO: is this needed?
   
    return Ok;
}


static Err
htmldoc_reparse_source(HtmlDoc d[_1_]) {
    Dom dom = htmldoc_dom(d);
    dom_reset(dom);
    Str* html = textbuf_buf(htmldoc_sourcebuf(d));
    return dom_parse(dom, sv(html));
}


Err
htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[_1_]) {
    if (!htmldoc_http_content_type_text_or_undef(d)) return Ok;
    const char* utf8s;
    size_t utf8slen;
    if (!htmldoc_http_charset_is_utf8(d)) {
        Str* buf = textbuf_buf(htmldoc_sourcebuf(d));
        char* from_charset = items__(htmldoc_http_charset(d));
        try( mem_convert_to_utf8(items__(buf), len__(buf), from_charset, &utf8s, &utf8slen));
        std_free(buf->items);
        buf->items = (char*)utf8s;
        buf->len = utf8slen;
        buf->capacity = utf8slen;
        try( htmldoc_reparse_source(d));
    }
    return Ok;
}


Err
htmldoc_console(HtmlDoc d[_1_], Session* s, const char* line, CmdOut* out) {
    if (!s) return "error: no session";
    return jse_eval(htmldoc_js(d), s, sv(line), out);
}


static Err
jse_eval_doc_scripts(Session* s, HtmlDoc d[_1_], CmdOut* out) {

    for ( Str* it = arlfn(Str,begin)(htmldoc_head_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_head_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, sv(it), out);
        if (e) msg__(out, (char*)e);
    }

    for ( Str* it = arlfn(Str,begin)(htmldoc_body_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_body_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, sv(it), out);
        if (e) msg__(out, (char*)e);
    }

    return Ok;
}


//TODO: make this fn not Err and rename it
Err
htmldoc_js_enable(HtmlDoc d[_1_], Session* s, CmdOut* out) {
    try( jse_init(d));
    Err e = jse_eval_doc_scripts(s, d, out);
    if (e) msg__(out, (char*)e);
    return Ok;
}


void
htmldoc_eval_js_scripts_or_continue(HtmlDoc d[_1_], Session* s, CmdOut* out) {
    if (htmldoc_js_is_enabled(d)) {
        Err e = jse_eval_doc_scripts(s, d, out);
        if (e) msg__(out, (char*)e);
    }
}


static Err
_htmldoc_scripts_range_from_parsed_range_(
    HtmlDoc          h[_1_],
    RangeParse p[_1_],
    Range            r[_1_]
) {
    *r = (Range){0};
    size_t head_script_count = len__(htmldoc_head_scripts(h));
    size_t body_script_count = len__(htmldoc_body_scripts(h));
    if (!head_script_count && !body_script_count) return "HtmlDoc has no scripts";
    size_t script_max = head_script_count + body_script_count - 1;
    switch (p->beg.tag) {
        case range_addr_curr_tag:
        case range_addr_none_tag:
        case range_addr_prev_tag: 
        case range_addr_search_tag:
             return "invalid range.beg address for scripts";
        case range_addr_beg_tag:
             try(set__(&r->beg, p->beg.delta));
            break;
        case range_addr_end_tag: r->beg = script_max + p->beg.delta;
            break;
        case range_addr_num_tag: r->beg = p->beg.n + p->beg.delta;
            break;
        default: return "error: invalid RangeParse beg tag";
    }
    switch (p->end.tag) {
        case range_addr_curr_tag:
        case range_addr_prev_tag: 
        case range_addr_search_tag:
             return "invalid range.end address for scripts";
        case range_addr_beg_tag: r->end = 1 + p->end.delta;
            break;
        case range_addr_none_tag: r->end = 1 + r->beg;
            break;
        case range_addr_end_tag: r->end = 1 + script_max + p->end.delta;
            break;
        case range_addr_num_tag: r->end = 1 + p->end.n + p->end.delta;
            break;
        default: return "error: invalid RangeParse end tag";
    }
    return Ok;
}


Err
htmldoc_scripts_write(HtmlDoc h[_1_], RangeParse rp[_1_], Writer w[_1_]) {
    if (!htmldoc_js_is_enabled(h)) return "enable js to get the scripts";

    Range r;
    try(_htmldoc_scripts_range_from_parsed_range_(h, rp, &r));
    for (size_t it = r.beg; it < r.end; ++it) {
        char buf[UINT_TO_STR_BUFSZ] = {0};
        size_t len;
        try( unsigned_to_str(it, buf, UINT_TO_STR_BUFSZ, &len));
        Str* sc;
        try( htmldoc_script_at(h, it, &sc));
        try(writer_write_lit__(w, "// script: "));
        try(writer_write(w, buf, len));
        if (len__(sc) <= 1) {
            try(writer_write_lit__(w, " is empty!"));
        } else {
            try(writer_write_lit__(w, "\n"));
            try(writer_write(w, items__(sc), len__(sc) - 1));
        }
        try(writer_write_lit__(w, "\n"));
    }
    return Ok;
}


Err curl_lexbor_fetch_document(
    UrlClient         url_client[_1_],
    HtmlDoc           htmldoc[_1_],
    CmdOut            out[_1_],
    FetchHistoryEntry histentry[_1_]
);

Err
htmldoc_fetch(HtmlDoc htmldoc[_1_], UrlClient url_client[_1_], CmdOut cmd_out[_1_], FetchHistoryEntry he[_1_]) {
    return curl_lexbor_fetch_document(url_client, htmldoc, cmd_out, he);
}


Err
htmldoc_script_at(HtmlDoc d[_1_], size_t ix, Str* sptr[_1_]) {
    *sptr = arlfn(Str,at)(htmldoc_head_scripts(d), ix);
    if (*sptr) return Ok;
    *sptr = arlfn(Str,at)(htmldoc_body_scripts(d), ix - len__(htmldoc_head_scripts(d)));
    if (*sptr) return Ok;
    return err_fmt("not script at %d", ix);
}


Err
htmldoc_input_at(HtmlDoc d[_1_], size_t ix, DomNode out[_1_]) {
    DomNode* np = arlfn(DomNode, at)(htmldoc_inputs(d), ix);
    if (!np) return  "link number invalid";
    *out = *np;
    return Ok;
}



Err
htmldoc_title_or_url(HtmlDoc d[_1_], char* url, Str title[_1_]) {
    Err err = Ok;
    DomNode title_node;
    try( dom_get_title_node(htmldoc_dom(d), &title_node));
    if (!isnull(title_node)) err = dom_get_title_text_line(htmldoc_dom(d), title);
    else if (!*url) err = "no title nor url";
    else err = str_append(title, url);

    return err;
}


void
textmod_trim_left(TextBufMods mods[_1_], size_t n) {
    if (n && len__(mods)) {
        foreach__(ModAt,it,mods) {
            if (it->offset < n) it->offset = 0;
            else it->offset -= n;
        }
    }
}


Err
htmldoc_switch_js(HtmlDoc htmldoc[_1_], Session* s, CmdOut* out) {
    if (!s) return "error: NULL session";
    JsEngine* js = htmldoc_js(htmldoc);
    bool is_enabled = jse_rt(js);
    if (is_enabled) jse_clean(js);
    else try( htmldoc_js_enable(htmldoc, s, out));//TODO:REDRAW!
   
    return Ok;
}


bool
htmldoc_http_charset_is_utf8(HtmlDoc d[_1_]) {
    Str* from_charset = htmldoc_http_charset(d);
    return !len__(from_charset) || str_eq_case(svl("UTF-8"), from_charset);
}


bool
htmldoc_http_content_type_text_or_undef(HtmlDoc d[_1_]) {
#define TXT_ "text/"
    Str* content_type = htmldoc_http_content_type(d);
    const size_t len = lit_len__(TXT_);
    const size_t ctlen = len__(content_type);
    return !ctlen || (ctlen > len && !strncmp(items__(content_type), TXT_, len));
#undef TXT_
}

