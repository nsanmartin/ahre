#include "generic.h"
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
#include "cmd-out.h"


static inline
Err draw_ctx_esc_code_pop(DrawCtx ctx[_1_]) {
    return arlfn(EscCode, pop)(draw_ctx_esc_code_stack(ctx)) ? Ok : "error: empty stack";
}
static bool ends_with_newline(StrView v) { return !v.len || v.items[v.len-1] == '\n'; }
static bool requires_separation(StrView v) {
    return v.len && !isspace(v.items[v.len-1]) && !ispunct(v.items[v.len-1]);
}



/* internal linkage */
#define MAX_URL_LEN 2048u
#define READ_FROM_FILE_BUFFER_LEN 4096u
#define LAZY_STR_BUF_LEN 1600u

#define \
append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(DomNode,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")


static size_t _strview_trim_right_count_newlines_(StrView s[_1_]) {
#define last_char__(S) items__(S)[len__(S)-1]
//TODO1: use unicode space
#define is_space_or_not_print__(C) (isspace(C) || !isprint(C))
    size_t newlines = 0;
    while(s->len && is_space_or_not_print__(last_char__(s))) {
        newlines += last_char__(s) == '\n';
        --s->len;
    }
    return newlines;
#undef is_space_or_not_print__
#undef last_char__
}


#define DRAW_SUBCTX_FLAG_DIV 0x1u
typedef struct {
    Str         buf;
    TextBufMods mods;
    size_t      flags;
    size_t      fragment_offset;
    bool        left_newlines;
    size_t      left_trim;
    size_t      right_newlines;
} DrawTextBuf;


typedef struct { size_t col, row; } Coordinates;
#define T Coordinates
#include "arl.h"

#define KT Coordinates
#define VT unsigned
#include "lip.h"

typedef LipOf(Coordinates,unsigned) CoordToUint;
typedef struct { CoordToUint map; ArlOf(Coordinates) lst; } ColSpan;

static inline CoordToUint* colspan_map(ColSpan cs[_1_]) { return &cs->map; }
static inline ArlOf(Coordinates)* colspan_lst(ColSpan cs[_1_]) { return &cs->lst; }

static Err colspan_init(ColSpan cs[_1_]) {
    *cs              = (ColSpan){0};
    if (lipfn(Coordinates,unsigned,init)(colspan_map(cs), (LipInitArgs){.sz=4}))
        return err_internal("lip init failure)");
    return Ok;
}

static inline void colspan_clean(ColSpan cs[_1_]) {
    lipfn(Coordinates,unsigned,clean)(colspan_map(cs));
    arlfn(Coordinates,clean)(colspan_lst(cs));
}

static Err colspan_set(ColSpan cs[_1_], Coordinates k, unsigned v) {
    if (lipfn(Coordinates,unsigned,set)(&cs->map, &k, &v))
        return err_internal("lip set failure");

    if ( v > 1 && !arlfn(Coordinates,append)(&cs->lst, &k))
        return err_internal("arl append failure");
    return Ok;
}

static unsigned* colspan_get(ColSpan cs[_1_], Coordinates k[_1_]) {
    return lipfn(Coordinates,unsigned,get)(&cs->map, k);
}

static unsigned colspan_get_interpreted(ColSpan cs[_1_], Coordinates k) {
    unsigned* v = colspan_get(cs, &k);
    if (!v) return 1;
    return *v;
}

/* sub text flags */
static inline bool
draw_text_buf_div(DrawTextBuf text[_1_]) { return text->flags & DRAW_SUBCTX_FLAG_DIV; }


typedef Err (*DrawEffectCb)(DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
Err draw_rec(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static Err draw_rec_tag(DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static Err draw_text(DomNode node,  DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static inline Err draw_list( DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static inline Err draw_list_block( DomNode it, DomNode last, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);
static Err draw_tag_table (DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]);


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


static StrView strview_from_draw_text_buf(DrawTextBuf dtb[_1_]) {
    Str* buf = draw_text_buf_buf(dtb);
    return (StrView){
        .items = buf->items + dtb->left_trim,
        .len   = buf->len - size_t_min(buf->len,dtb->left_trim)
    };
}

static size_t
draw_text_buf_hlen(DrawTextBuf text[_1_]) {
    size_t res = 0;
    StrView buf = strview_from_draw_text_buf(text);
    for (;buf.len;) {
        StrView line = strview_split_line(&buf);
        strview_trim_left_utf8_space(&line);
        size_t utf8len = strview_count_utf8(line);
        if (utf8len > res) res = utf8len;
    }
    return res;
}

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

        arlfn(ModAt, clean)(textbuf_mods(tb));
        *textbuf_mods(tb)         = *draw_text_buf_mods(text);
        *draw_text_buf_mods(text) = (TextBufMods){0};


        try( textbuf_append_line_indexes(tb));
        try( textbuf_append_null(tb));
        *textbuf_current_offset(tb) = *draw_text_buf_fragment_offset(text);
    }
    return Ok;
}


static Err
draw_ctx_append_sub_text(DrawTextBuf text[_1_], DrawTextBuf sub[_1_]) {
    if (sub->buf.len < sub->left_trim) return err_internal("left trim larger than buffer len");
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


static Err
draw_text_buf_trim_left(DrawTextBuf sub[_1_]) {
    StrView content    = sv(sub->buf);
    if (sub->buf.len < content.len) return err_internal("buffer smaller than its content?");
    size_t skipped     = strview_trim_left_utf8_space(&content);
    sub->left_newlines = memchr(sub->buf.items, '\n', skipped) != NULL;
    sub->left_trim     = sub->buf.len - content.len;
    textmod_trim_left(&sub->mods, sub->left_trim);
    return Ok;
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
    return draw_iter_childs(node, ctx, text);//TODO1: br can't have child, isn't it?
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
    if (str_eq_case(type, svl("hidden")))
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

    /* "input" type is text | search | email | url | tel */
    //TODO2: validate input format for email
    if (html_input_type_is_text_like(type)) {
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
    Err err = Ok;
    DrawTextBuf sub = (DrawTextBuf){0};

    try( draw_block_iter_childs(node,  ctx, &sub));

    tryjmp(err, Clean, draw_text_buf_trim_left(&sub));
    draw_text_buf_trim_right(&sub);

    tryjmp(err,Clean,_hypertext_open_(ctx, text, draw_ctx_push_bold, h_tag_open_str));
    if (sub.buf.len) tryjmp(err, Clean, draw_ctx_append_sub_text(text, &sub));
    tryjmp(err, Clean, _hypertext_close_(ctx, text, draw_ctx_reset_color, newline_str));

Clean:
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
    (void)ctx; (void)node;
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


static Err
htmldoc_cache_script_nodes(HtmlDoc d[_1_]) {
    return dom_get_tag_nodes_in_doc(htmldoc_dom(d), svl("script"), htmldoc_scripts(d));
}


/*TODO: decouple sesion and htmldoc here */
Err
htmldoc_draw_with_flags(HtmlDoc htmldoc[_1_], Session* s, unsigned flags, CmdOut cmd_out[_1_]) {
    if (!s) return "error: expecting not null Session";
    Dom dom = htmldoc_dom(htmldoc);
    DrawCtx     ctx;
    DrawTextBuf text = (DrawTextBuf){0};
    Err err = draw_ctx_init(&ctx, htmldoc, s, flags, cmd_out);
    try(err);
    ok_then(err, draw_rec(dom_root(dom), &ctx, &text));
    ok_then(err, draw_text_buf_commit(&ctx, &text));
    ok_then(err, textbuf_fit_lines(htmldoc_textbuf(htmldoc), *session_ncols(s)));

    ok_then(err, htmldoc_cache_script_nodes(htmldoc));
    draw_ctx_cleanup(&ctx);
    draw_text_buf_clean(&text);
    return err;
}


static Err
_htmldoc_draw_(HtmlDoc htmldoc[_1_], Session s[_1_], CmdOut cmd_out[_1_]) {
    unsigned flags = draw_ctx_flags_from_session(s) | DRAW_CTX_FLAG_TITLE;
    return htmldoc_draw_with_flags(htmldoc, s, flags, cmd_out);
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
    const bool js_enabled = session_js(s);

    /* lexbor doc should be initialized before jse_init */
    *d = (HtmlDoc){.req=*r};
    tryjmp(e,Fail, dom_init(&d->dom));

    FetchHistoryEntry* entry;
    tryjmp(e,Fail, arl_append_zero(FetchHistoryEntry, session_fetch_history(s), entry));

    tryjmp(e,Fail, htmldoc_fetch(d, uc, js_enabled, cmd_out, entry));
    if (js_enabled)
        tryjmp(e, Fail, jse_init(d));
    htmldoc_eval_js_scripts_or_continue(d, s, cmd_out);
    tryjmp(e,Fail, _htmldoc_draw_(d, s, cmd_out));
    tryjmp(e,Fail, fetch_history_entry_update_title(entry, htmldoc_dom(d)));

    *r = (Request){0};
    return Ok;

Fail:
    /* In case of failure, the caller keeps ownership. In case of success it loses it */
    *r = d->req;
    d->req = (Request){0};

    return e;
}


Err
htmldoc_init_bookmark_move_urlstr(HtmlDoc d[_1_], Str urlstr[_1_]) {
    Err e = Ok;

    *d = (HtmlDoc){ 0 };
    try(dom_init(&d->dom));
    if (!urlstr) return err_internal("cannot initialize bookmark with not path");
    tryjmp(e, Fail, request_init(htmldoc_request(d), http_get, sv(urlstr), NULL));

    return Ok;
Fail:
    dom_cleanup(d->dom);
    return e;
}


/* external linkage */


void
htmldoc_reset_draw(HtmlDoc htmldoc[_1_]) {
    textbuf_reset(htmldoc_textbuf(htmldoc));
    arlfn(DomNode,reset)(htmldoc_anchors(htmldoc));
    arlfn(DomNode,reset)(htmldoc_imgs(htmldoc));
    arlfn(DomNode,reset)(htmldoc_inputs(htmldoc));
    arlfn(DomNode,reset)(htmldoc_forms(htmldoc));
    arlfn(DomNode,reset)(htmldoc_scripts(htmldoc));
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
    arlfn(DomNode,clean)(htmldoc_scripts(htmldoc));

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


static bool
is_cookie_in_domain(StrView host, StrView line) {
    if (str_startswith(line, svl("#HttpOnly"))) return false;
    StrView domain = strview_split_word(&line);
    if (!domain.len && !host.len) return false;
    do {
        StrView dpart = strview_rsplit(&domain, '.');
        StrView hpart = strview_rsplit(&host, '.');
        if (!str_eq_case(dpart, hpart)) return false;
    } while (domain.len && host.len);
    //TODO0: should subdomins be actually included, if not return !domain.len && !host.len;
    return !domain.len;
}


Err
htmldoc_get_cookies(HtmlDoc d[_1_], ArlOf(Str) out[_1_]) {
    Err      e    = Ok;
    CurlPtr* curl = request_curl_handle(htmldoc_request(d));
    Url*      url = request_url(htmldoc_request(d));
    char*    host;
    struct curl_slist* cookies = NULL;
    if (!curl|| !*curl) fail("expecting a curl handle here");
    if (!url) fail("expecting an url");
    try(url_append_host_to_str(*url, &host));
    CURLcode curl_code = curl_easy_getinfo(*curl, CURLINFO_COOKIELIST, &cookies);
    if (curl_code != CURLE_OK) { e="error: could not retrieve cookies list"; goto Clean; }
    if (!cookies) { e="no cookies"; goto Clean; }

    for(struct curl_slist* it = cookies; it; it = it->next) {
        StrView data = sv(it->data);
        if (is_cookie_in_domain(sv(host), data)) {
            Str* k;
            tryjmp(e,Clean, arl_append_zero(Str, out, k));
            tryjmp(e,Clean, str_append(k, data));
        }
    }
Clean:
    curl_free(host);
    curl_slist_free_all(cookies);
    return Ok;
}


Err
htmldoc_set_cookielist(HtmlDoc d[_1_], StrView cookie) {
    CurlPtr* curl = request_curl_handle(htmldoc_request(d));
    if (!curl|| !*curl) fail("expecting a curl handle here");

    CURLcode curl_code = curl_easy_setopt(*curl, CURLOPT_COOKIELIST, cookie.items);
    if (curl_code != CURLE_OK) fail("could not set cookie");
    return Ok;
}


Err
htmldoc_print_info(HtmlDoc d[_1_], CmdOut* out) {
    try( msg_fmt(out, "download size: %ld\n", *htmldoc_curlinfo_sz_download(d)));
    try( msg_fmt(out, "html size: %ld\n", htmldoc_sourcebuf(d)->buf.len));

    {
        Err e = Ok;
        try(msg__(out, "title: "));
        Str title = (Str){0};
        try(dom_get_title_text_line(htmldoc_dom(d), &title));
        if (!title.len) e = msg__(out,  svl("<NO TITLE>\n"));
        else e = msg_ln__(out, title);
        str_clean(&title);
        try(e);
    }
    
    {
        //TODO1: remove the url in favour of effective Url
        try(msg__(out, "url: "));
        char* url = NULL;
        Err   e   = Ok;
        tryjmp(e,CleanUrl, url_cstr_malloc(*htmldoc_url(d), &url));
        if (url) tryjmp(e,CleanUrl, msg_ln__(out, url));
        else tryjmp(e, CleanUrl, msg_ln__(out, svl("<NO URL>")));

CleanUrl:
        w_curl_free(url);
        try(e);
    }

    char* effective_url;
    try(msg__(out, "effective url: "));
    try(w_curl_get_effective_url(*request_curl_handle(htmldoc_request(d)), &effective_url));
    if (effective_url) try(msg_ln__(out, sv(effective_url)));
    else try(msg_ln__(out, svl("<NO EFFECTIVE URL>")));

    {
        Err e = Ok;
        ArlOf(Str) cookies  = (ArlOf(Str)){0};
        tryjmp(e,Clean_Cookies, htmldoc_get_cookies(d, &cookies));
        try(msg__(out, "cookies: "));
        if (!cookies.len) try(msg_ln__(out, svl("<NO COOKIES>")));
        foreach__(Str,&cookies, it) {
            tryjmp(e,CleanUrl, msg_ln__(out, it));
        }

Clean_Cookies:
        arlfn(Str,clean)(&cookies);
    }

    Str* charset = htmldoc_http_charset(d);
    if (len__(charset)) {
        try( msg__(out, svl("charset: ")));
        try( msg_ln__(out, charset));
    }

    Str* content_type = htmldoc_http_content_type(d);
    if (len__(content_type)) {
        try( msg__(out, svl("content-type: ")));
        try( msg_ln__(out, content_type));
    }

    try( msg_fmt(out, "script count: %ld\n", len__(htmldoc_scripts(d))));
    try( msg_fmt(out, "downloaded script count: %ld\n", len__(htmldoc_body_scripts(d)) + len__(htmldoc_head_scripts(d))));
    return Ok;
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

        HtmlDoc*        d          = draw_ctx_htmldoc(ctx);
        ArlOf(DomNode)* anchors    = htmldoc_anchors(d);
        const size_t    anchor_num = anchors->len;
        DrawTextBuf*    sub_text   = &(DrawTextBuf){0};

        if (!arlfn(DomNode,append)(anchors, &node)) 
            return "error: lip set";

        if (draw_ctx_color(ctx)) try( draw_ctx_esc_code_push(ctx, esc_code_blue));

        Err err = draw_iter_childs(node, ctx, sub_text);

        if (err) {
            draw_text_buf_clean(sub_text);
            return err;
        }

        bool div_child = draw_text_buf_div(sub_text); /* check whether any child is div */

        ok_then(err, draw_text_buf_trim_left(sub_text));
        draw_text_buf_trim_right(sub_text);

        if (requires_separation(sv(draw_text_buf_buf(text)))
        && _prev_is_separable_(node)) err = draw_text_buf_append_lit__(text, " ");
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
        case HTML_TAG_TABLE: { return draw_tag_table(node, ctx, text); } 
        case HTML_TAG_TITLE: { return draw_tag_title(node, ctx); } 
        case HTML_TAG_TR: { return draw_tag_tr(node, ctx, text); }
        case HTML_TAG_TT: { return draw_tag_code(node, ctx, text); }
        case HTML_TAG_UL: { return draw_tag_ul(node, ctx, text); }
        //TODO: implement all these tags, meanwhile we just add
        // spaces when needed to separate from previous word
        case HTML_TAG_TD: case HTML_TAG_TH: //TODO delete once table is implemented
        case HTML_TAG_TIME:
        case HTML_TAG_SAMP:
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


/*
 * DrawRow == ArlOf(DrawTextBuf)
 */

#define T DrawTextBuf
#define TClean draw_text_buf_clean
#include <arl.h>
typedef ArlOf(DrawTextBuf) DrawRow;

#define draw_row_append(R,T) ((!arlfn(DrawTextBuf,append)(R,T)) ? "error: draw_row_append failure": Ok)

/********************************/


/*
 * DrawTable == ArlOf(DrawRow)
 */

#define T DrawRow
#define TClean arlfn(DrawTextBuf,clean)
#include <arl.h>


typedef ArlOf(DrawRow) DrawTable;
#define draw_table_clean arlfn(DrawRow,clean)
#define draw_table_append(T,R) ((!arlfn(DrawRow,append)(T,R)) ? "error: draw_table_append failure":Ok)
static Err draw_table_get_coords(DrawTable t[_1_], Coordinates coord[_1_], DrawTextBuf* out[_1_]) {
    DrawRow* r = arlfn(DrawRow,at)(t,coord->row);
    if (!r) return err_internal("expecting row at index");
    DrawTextBuf* cell = arlfn(DrawTextBuf,at)(r,coord->col);
    if (!cell) return err_internal("expecting col at row index");
    *out = cell; 
    return Ok;
}

/********************************/


typedef struct { size_t nrows, ncols; } CellDims;


/*
 * ArlOf(CellDims)
 */
#define T CellDims
#include <arl.h>
/********************************/


typedef DrawTextBuf CellPart;
/* This function does not clean buf since it does not owns it */
static void cell_part_clean(DrawTextBuf c[_1_]) { arlfn(ModAt, clean)(&c->mods); }

/*
 * SplittedCell == ArlOf(CellPart) == ArlOf(DrawTextBuf)
 */
#define T CellPart
#define TClean cell_part_clean
#include <arl.h>
typedef ArlOf(CellPart) SplittedCell;
#define splitted_cell_append(C,T) ((!arlfn(CellPart,append)(C,T))?"error: arl append":Ok)
#define splitted_cell_clean arlfn(CellPart,clean)

/********************************/


/*
 * SplittedRow == ArlOf(SplittedCell)
 */

#define T SplittedCell
#define TClean splitted_cell_clean
#include <arl.h>
typedef ArlOf(SplittedCell) SplittedRow;
#define splitted_row_append_cell(C,T) ((!arlfn(SplittedCell,append)(C,T))?"error: arl append":Ok)
#define splitted_row_clean arlfn(SplittedCell,clean)

/********************************/


/*
 * SplittedTable == ArlOf(SplittedRow)
 */

#define T SplittedRow
#define TClean arlfn(SplittedCell,clean)
#include <arl.h>
typedef ArlOf(SplittedRow) SplittedTable;


#define splitted_table_clean arlfn(SplittedRow,clean)
#define splitted_table_append_row(R,C) ((!arlfn(SplittedRow,append)(R,C))?"error: arl":Ok)

/********************************/


typedef struct  { size_t w,ix,splits; } ColWidth;
#define T ColWidth
#include <arl.h>
static int colwidth_cmp(const void* a, const void* b) {
#define splitted_col_width__(C) _Generic((C),\
        ColWidth*: (C)->w / (1+(C)->splits), const ColWidth*: (C)->w / (1+(C)->splits))
    const ColWidth* A = a;
    const ColWidth* B = b;
    return (splitted_col_width__(A) + (B)->splits) - (splitted_col_width__(B) + (A)->splits);
}
static int colwidth_cmp_ix(const void* a, const void* b) {
    const ColWidth* A = a;
    const ColWidth* B = b;
    return A->ix - B->ix;
}

#define col_width__(C) _Generic((C),ColWidth*: ((C)->w / (1+(C)->splits)))
static size_t compute_colspan_len(size_t ncol, size_t span, ArlOf(ColWidth) cwidths[_1_]) {
    size_t res = 0;
    size_t last_col = min_size_t(ncol + span, len__(cwidths));
    for (size_t col = ncol; col < last_col; ++col) {
        ColWidth* w = arlfn(ColWidth,at)(cwidths, col);
        if (w) res += col_width__(w);
    }
    return res;
}

static size_t colspan_get_actual_length(ColSpan cs[_1_], Coordinates coord, ColWidth cw[_1_], ArlOf(ColWidth) cwidths[_1_]) {
    unsigned span = colspan_get_interpreted(cs, coord);
    switch (span) {
        case 0 : return len__(cwidths);
        case 1 : return col_width__(cw);
        default: return compute_colspan_len(coord.col, span, cwidths);
    }
} 
#undef col_width__


/*
 * Computes the maximum line inside each column and assigns the column that
 * width. If the cell spans other columns, it is ignored and computed in a
 * posterior pass.
 */
#define nrow__(T,R) (R)-arlfn(DrawRow,begin)(T)
#define ncol__(R,C) (C)-arlfn(DrawTextBuf,begin)(R)
static Err
get_unsplitted_columns_widths(DrawTable table[_1_], ArlOf(ColWidth) out[_1_]) {
#define mk_coordinates(R,C) (Coordinates){.row=(R),.col=(C)}
    foreach__(DrawRow, table, row) {
        foreach__(DrawTextBuf, row, cell) {
            size_t ncol = ncol__(row, cell);
            if (len__(out) < ncol) return err_internal("expecting col width calculated already");

            /* get the col, or append it if it wasnt already appended */
            ColWidth* cw = arlfn(ColWidth,at)(out,ncol);
            if (!cw) cw = arlfn(ColWidth,append)(out,&(ColWidth){.ix=ncol});
            if (!cw) return "arl append failure";
            if (cw->ix != ncol) return err_internal("expecting index match ncol here");

            StrView buf = strview_from_draw_text_buf(cell);
            strview_trim_left_utf8_space(&buf);
            for (;buf.len;) {
                StrView line = strview_split_line(&buf);
                strview_trim_left_utf8_space(&line);
                size_t utf8len = strview_count_utf8(line);
                if (utf8len > cw->w) {
                    cw->w  = utf8len;
                    cw->ix = ncol;
                }
            }
        }
    }
    return Ok;
#undef coordinates_for_cell
}

#define ERR_FIT_MSG__ "could not fit table in screen at " __FILE__ ":"
static char err_could_not_fit_the_table__[ sizeof(ERR_FIT_MSG__) +  /*linenum*/ INT_TO_STR_BUFSZ + 2 ] = {
    ERR_FIT_MSG__
};

static Err err_could_not_fit_the_table_impl(int linenum) {
    size_t len;
    try(int_to_str(linenum, err_could_not_fit_the_table__ + lit_len__(ERR_FIT_MSG__), INT_TO_STR_BUFSZ, &len));
    err_could_not_fit_the_table__[lit_len__(ERR_FIT_MSG__) + len] = '\0';
    return err_could_not_fit_the_table__;
}

#define err_could_not_fit_the_table err_could_not_fit_the_table_impl(__LINE__)


#define ERR_TAG_MSG__ "unexpected tag in table: 0x" 
#define TAG_STR_MAX_LEN sizeof(HtmlTag) 
static char err_unexpected_tag_in_table[ sizeof(ERR_TAG_MSG__) + TAG_STR_MAX_LEN ] = {  ERR_TAG_MSG__ };
static Err err_from_unexpected_tag_in_table(HtmlTag tag) {
    size_t len;
    try(int_to_str(tag, err_unexpected_tag_in_table + lit_len__(ERR_TAG_MSG__), TAG_STR_MAX_LEN, &len));
    err_unexpected_tag_in_table[lit_len__(ERR_TAG_MSG__) + len] = '\0';
    return err_unexpected_tag_in_table;
}


static Err
split_cell(size_t length, DrawTextBuf cell[_1_]) {
    if (!length) return err_internal("not expecting zero length cell");
    StrView buf = strview_from_draw_text_buf(cell);
    strview_trim_left_utf8_space(&buf);
    while (strview_count_utf8(buf) > length) {

        const char* nl = memchr(buf.items, '\n', buf.len);
        if (nl && cast__(size_t)(nl - buf.items) <= length) {
            strview_split_line(&buf);
            continue;
        }

        char* p = (char*)buf.items + size_t_min(buf.len, length - 1);
        while (p > buf.items && is_visible(*p)) p--;
        if (p < buf.items) return err_internal("why?");
        if (p == buf.items) {
            if (buf.len <=3) return err_could_not_fit_the_table;
            if (buf.len < length) return err_internal("this was not expected");
            p = (char*)buf.items + length - 1;
        }
        *p = '\n';
        strview_split_line(&buf);
    }
    return Ok;
}


/*
 * Splits each column's cell
 */
static Err
split_column(DrawTable table[_1_], ColWidth cw[_1_], ArlOf(ColWidth) cwidths[_1_], ColSpan colspan[_1_]) {
    foreach__(DrawRow, table, row) {
        size_t ncol = cw->ix;
        size_t nrow = nrow__(table,row);
        DrawTextBuf* cell = arlfn(DrawTextBuf,at)(row, cw->ix);
        if (!cell || !cell->buf.len) continue;
        size_t length  = colspan_get_actual_length(colspan, (Coordinates){.row=nrow, .col=ncol}, cw, cwidths);
        try(split_cell(length, cell));
    }
    return Ok;
}
#undef ncol__
#undef nrow__


/*
 * Resize the column widths so that the column fits the screen and "split" the cells
 * accordinly, ie, insert newlines (not *insert* but replace spaces by them).
 */ 
static Err
split_columns(DrawTable table[_1_], size_t exceeding_screen_cols, ColSpan colspan[_1_], ArlOf(ColWidth) ws[_1_]) {
    static size_t MIN_COL_WIDTH = 3;

    if (!len__(ws)) return Ok;
    do {
        ColWidth* max_cw = arlfn(ColWidth,begin)(ws);
        if (!max_cw) return err_internal("expecting elements in arl");
        for(ColWidth* it = max_cw + 1; it < arlfn(ColWidth,end)(ws) ; ++it)
            if (colwidth_cmp(max_cw, it) < 0) max_cw = it;


        size_t prev_col_width = splitted_col_width__(max_cw);
        max_cw->splits        += 1;
        if (!(max_cw->w / max_cw->splits)) return err_could_not_fit_the_table;
        size_t width          = splitted_col_width__(max_cw);
        if (width <= MIN_COL_WIDTH) return err_could_not_fit_the_table;
        size_t reduction      = prev_col_width - width;

        if (reduction <= exceeding_screen_cols) exceeding_screen_cols -= reduction;
        else {
            max_cw->w      = prev_col_width - exceeding_screen_cols;
            max_cw->splits = 0;
            if (max_cw->w == 0) return err_internal(":(");
            break;
        }
    } while(exceeding_screen_cols);

    qsort(items__(ws), len__(ws), sizeof(*ws->items), colwidth_cmp_ix);
    foreach__(ColWidth,ws,split_it) try(split_column(table, split_it, ws, colspan));
    return Ok;
}


static Err
split_colspan_columns(DrawTable table[_1_], ArlOf(ColWidth) colwidths[_1_], ColSpan colspan[_1_]) {
    foreach__(Coordinates, colspan_lst(colspan), coord) {
        unsigned* v = colspan_get(colspan, coord);
        if (!v) return err_internal("expecting colspan for coord");
        if (*v < 2) return err_internal("expecting colspan gt 2");
        size_t length = compute_colspan_len(coord->col, *v, colwidths);

        if (!length) return Ok;

        DrawRow* row  = arlfn(DrawRow,at)(table, coord->row);
        if (!row) return err_internal("arl failure: empty at");
        DrawTextBuf* cell = arlfn(DrawTextBuf,at)(row, coord->col);
        if (!cell) return err_internal("arl failure: empty at");

        try(split_cell(length, cell));
    }

    return Ok;
}


static Err
expand_columns_for_colspans(DrawTable table[_1_], size_t screen_width, ColSpan colspan[_1_], ArlOf(size_t) cols_hlen[_1_]) {
    Err err = Ok;
    if (!len__(cols_hlen)) return Ok;//TODO1 is this possible and acceptable?
    size_t col_count   = len__(cols_hlen);
    size_t borders_len = col_count - 1;
    ArlOf(size_t) col_increases = (ArlOf(size_t)){0};
    if (arlfn(size_t,init_reserve)(&col_increases, col_count)) return err_internal("arl reserve failure");
    col_increases.len = col_increases.capacity;

    size_t totalwidth = 0;
    foreach__(size_t,cols_hlen,chl) totalwidth += *chl;
    if (totalwidth + borders_len > screen_width) { err=err_could_not_fit_the_table; goto Clean; }
    size_t remaining_width = screen_width - (totalwidth + borders_len);

    foreach__(Coordinates, colspan_lst(colspan), coord) {
        unsigned* span = colspan_get(colspan,coord);
        if (!span || *span < 2) { err=err_internal("ColSpan precondition failure"); goto Clean; }

        size_t* chl = arlfn(size_t,at)(cols_hlen,coord->col);
        if (!chl) { err=err_internal("expecting column in arl"); goto Clean; }


        DrawTextBuf* cell;
        try( draw_table_get_coords(table, coord, &cell));
        size_t cell_hlen = draw_text_buf_hlen(cell);

        if (cell_hlen <= *chl) continue;

        size_t spanlen = 0;
        size_t* it = arlfn(size_t,at)(cols_hlen,coord->col);
        size_t* end   = arlfn(size_t,at)(cols_hlen,coord->col + *span);
        if (!it) { err=err_internal("expecting col at index"); goto Clean; }
        if (!end) end = arlfn(size_t,end)(cols_hlen);
        for (; it < end; ++it) spanlen += *it;

        /* column horizontal len is not zero and text fits the span */
        if (cell_hlen <= spanlen) continue;

        size_t incr = cell_hlen - spanlen;
        if (remaining_width < incr) { err=err_could_not_fit_the_table; goto Clean; }
        remaining_width -= incr;
        *chl            += incr;
    }
Clean:
    arlfn(size_t,clean)(&col_increases);
    return err;
}


static Err
fit_table_to_screen(DrawTable table[_1_], size_t screen_width, ColSpan colspan[_1_]) {
    Err err                 = Ok;
    ArlOf(ColWidth) colwidths = (ArlOf(ColWidth)){0};

    tryjmp(err, Clean, get_unsplitted_columns_widths(table, &colwidths));
    size_t totalwidth  = 0;

    foreach__(ColWidth, &colwidths, w) totalwidth += w->w + 1;

    if (totalwidth > screen_width) {
        size_t exceeding_screen_cols = totalwidth - screen_width;
        tryjmp(err, Clean, split_columns(table, exceeding_screen_cols, colspan, &colwidths));
    }

    err = split_colspan_columns(table, &colwidths, colspan);
Clean:
    arlfn(ColWidth,clean)(&colwidths);
    return err;
}


static Err
draw_text_buf_split(DrawTextBuf b[_1_], SplittedCell textviews[_1_]) {
    TextBufMods partmods = (TextBufMods){0};
    StrView     buf      = strview_from_draw_text_buf(b);
    size_t      offset   = strview_trim_left_utf8_space(&buf);
    ModAt*      modbeg   = arlfn(ModAt,begin)(draw_text_buf_mods(b));
    ModAt*      modend   = arlfn(ModAt,end)(draw_text_buf_mods(b));
    ModAt*      mod      = modbeg;
    Err         err      = Ok;

    if (!buf.len) return Ok;

    for (;buf.len;) {
        StrView line = strview_split_line(&buf);

        if (mod > modbeg) {
            ModAt* prevmod = mod - 1;
            for(; prevmod > modbeg && prevmod->tmod != text_mod_reset; --prevmod)
                ;

            for(;prevmod <= mod - 1; ++prevmod)
                tryjmp(err, ErrClean, textmod_append(&partmods, 0, prevmod->tmod));
        }
        
        for (;mod && mod < modend && mod->offset <= offset + line.len; ++mod) 
            tryjmp(err, ErrClean, textmod_append_left_displaced(&partmods, *mod, offset));

        tryjmp(err, ErrClean, textmod_append(&partmods, line.len, text_mod_reset));

        /* not setting capacity since it is a view, but it is not tidy, maybe i'd change it*/
        CellPart part = (DrawTextBuf){ .buf={.items=(char*)line.items, .len=line.len}, .mods=partmods }; 

        tryjmp(err,ErrClean,splitted_cell_append(textviews, &part));
        partmods = (TextBufMods){0};

        offset += line.len + 1;
    }

    if (mod && mod < modend) { /* apply mods to last part if some are remainig */
        CellPart* last_part = arlfn(CellPart,back)(textviews);
        if (last_part)
            for (;mod && mod < modend; ++mod)
                tryjmp(err, ErrClean, textmod_append_left_displaced(&last_part->mods, *mod, offset));
    }

    /* Checks: */
    if ( offset - 1 != strview_from_draw_text_buf(b).len) 
    { err="error: splitting cell did not keet the offset"; goto ErrClean; }
    if ( mod != modend) { err="error: splitting cell did not apply all mods"; goto ErrClean; }

    return Ok; 

ErrClean:
    arlfn(ModAt,clean)(&partmods);
    return err;
}

static size_t splitted_celL_vertical_len(SplittedCell c[_1_]) { return len__(c); }
static size_t cell_part_horizontal_len(CellPart c[_1_]) {
    StrView part = sv(c->buf);
    strview_trim_left_utf8_space(&part);
    return strview_count_utf8(part);

}

static Err splitted_table_row_vertical_lengths(SplittedTable t[_1_], ArlOf(size_t) rows_vlengths[_1_]) {
    SplittedRow* table_beg = arlfn(SplittedRow,begin)(t);
    foreach__(SplittedRow, t, row) {
        size_t nrow = row - table_beg;

        if (len__(rows_vlengths) < nrow) return "error: missing row lenght";
        if (len__(rows_vlengths) == nrow && !arlfn(size_t,append)(rows_vlengths, &((size_t){0})))
            return "error: arl append failure";

        size_t* maxlen = arlfn(size_t,at)(rows_vlengths, nrow);
        if (!maxlen) return "error: could noit compute vrow vertical lengths";

        foreach__(SplittedCell, row, cell)
            if (splitted_celL_vertical_len(cell) > *maxlen) *maxlen = splitted_celL_vertical_len(cell);
    }
    return Ok;
}


static Err
splitted_table_col_horizonal_lengths(SplittedTable t[_1_], ColSpan colspan[_1_], ArlOf(size_t) cols_hlengths[_1_]) {

    foreach__(SplittedRow, t, row) {
        SplittedCell* row_beg = arlfn(SplittedCell,begin)(row);
        size_t nrow = row - arlfn(SplittedRow,begin)(t);

        foreach__(SplittedCell, row, cell) {
            size_t ncol = cell - row_beg;
            if (len__(cols_hlengths) < ncol) return err_internal("missing row length");
            if (len__(cols_hlengths) == ncol
            && !arlfn(size_t,append)(cols_hlengths, &((size_t){0})))
                return err_internal("arl append failure");

            if (colspan_get_interpreted(colspan, (Coordinates){.row=nrow,.col=ncol}) > 1) { continue; }

            size_t* maxlen = arlfn(size_t,at)(cols_hlengths, ncol);

            foreach__(CellPart,cell,cellpart) {
                if (cell_part_horizontal_len(cellpart) > *maxlen) *maxlen = cell_part_horizontal_len(cellpart);
            }
        }
    }
    return Ok;
}


static unsigned parse_colspan(DomNode n) {
    StrView colspan = dom_node_attr_value(n, svl("colspan"));
    long    res     = 0;
    Str     buf     = (Str){0};

    if (colspan.len && !str_append_z(&buf, colspan) && !parse_l(buf.items, &res))
        res = 0;
    str_clean(&buf);
    return (res < 0 || res > UINT_MAX) ? 0 : res;
}

static Err draw_tag_td(DomNode node, DrawCtx ctx[_1_], DrawRow r[_1_]) {
    Err err = Ok;
    DrawTextBuf cell = (DrawTextBuf){0};

    for (DomNode txt = dom_node_first_child(node); !isnull(txt); txt = dom_node_next(txt)) {
        tryjmp(err, Clean, draw_rec(txt, ctx, &cell));
    }
    tryjmp(err, Clean, draw_text_buf_trim_left(&cell));
    draw_text_buf_trim_right(&cell);
    tryjmp(err, Clean, draw_row_append(r, &cell));
    cell = (DrawTextBuf){0};

Clean:
    draw_text_buf_clean(&cell);
    return err;
}

/*
 * reads row table from dom.
 * rv:
 *   the cell(s), appended to tis row (it'd be more than only if colspan > 1)
 *   the colspan entry in the colspan map
 */
static Err
dom_read_table_row(DomNode n, DrawCtx ctx[_1_], DrawRow r[_1_], ColSpan colspan[_1_], size_t nrow) {
    if (!dom_node_has_tag(n, HTML_TAG_TR))
        return err_from_unexpected_tag_in_table(dom_node_tag(n));

    for (DomNode it = dom_node_first_elem_child(n); !isnull(it); it = dom_node_next_elem(it)) {
        if (!dom_node_has_type(it, DOM_NODE_TYPE_ELEMENT)) continue; //TODO0 log this?
        switch(dom_node_tag(it)) {
            case HTML_TAG_TH:
            case HTML_TAG_TD: {
                size_t   ncol = len__(r); /* cell is about to be appended */
                unsigned span = parse_colspan(it);
                if (span > 1) try(colspan_set(colspan, (Coordinates){.row=nrow,.col=ncol}, span));

                try(draw_tag_td(it, ctx, r));

                for (size_t i = 1; i < span; ++i) {
                    try(colspan_set(colspan, (Coordinates){.row=nrow, .col=ncol + i}, 0));
                    try(draw_row_append(r, &(DrawTextBuf){0}));
                }
                break;
            }
            default: return "error: expecting either th or td";
        }
    }

    return Ok;
}

#define _WHITE_SPACE_ "                                                                                   "\
    "                                                                                                     "\
    "                                                                                                     "\
    "                                                                                                     "

#define whitespace(N) _Generic((N),size_t: sv(_WHITE_SPACE_,size_t_min(N,lit_len__(_WHITE_SPACE_))))

/*
 * We "split" the columns so that we can prserve the mods (color, etc.) between the lines
 * in which they are split.
 */
static Err
draw_table_to_splitted_view(DrawTable table[_1_], SplittedTable table_view[_1_]) {
    Err          err           = Ok;
    SplittedCell splitted_cell = (SplittedCell){0};
    SplittedRow  splitted_row  = (SplittedRow){0};

    foreach__(DrawRow, table, row) {
        foreach__(DrawTextBuf, row, cell) {
            splitted_cell = (SplittedCell){0};
            tryjmp(err, ErrClean, draw_text_buf_split(cell, &splitted_cell));
            tryjmp(err, ErrClean, splitted_row_append_cell(&splitted_row, &splitted_cell));
            splitted_cell = (SplittedCell){0};
        }

        tryjmp(err, ErrClean, splitted_table_append_row(table_view, &splitted_row));
        splitted_row  = (SplittedRow){0};
    }
    return Ok;
ErrClean:
    splitted_cell_clean(&splitted_cell);
    splitted_row_clean(&splitted_row);
    return err;
}


static Err
get_cell_horizontal_len(size_t ncol, size_t span, ArlOf(size_t) cols_hlengths[_1_], size_t out[_1_]) {
    *out = 0;
    size_t last_col = min_size_t(ncol + span, len__(cols_hlengths));
    for (size_t col = ncol; col < last_col; ++col) {
        size_t* clen = arlfn(size_t,at)(cols_hlengths, col);
        if (!clen) return err_internal("expecting value at index");
        *out += *clen;
    }
    return Ok;
}


static size_t
is_last_column(size_t ncols, SplittedCell cell[_1_], SplittedRow row[_1_], size_t span) {
    return cast__(size_t)(cell - arlfn(SplittedCell,begin)(row)) + span >= ncols;
}

static Err
draw_splitted_table(
    SplittedTable splitted_table[_1_],
    ArlOf(size_t) rows_vlengths[_1_],
    ArlOf(size_t) cols_hlengths[_1_],
    ColSpan       colspan[_1_],
    DrawTextBuf   text[_1_]
) {
    foreach__(SplittedRow, splitted_table, row) {
        size_t nrow = row - arlfn(SplittedRow,begin)(splitted_table);
        size_t* row_vertical_len = arlfn(size_t,at)(rows_vlengths,nrow);
        if (!row_vertical_len) return err_internal("unexpected empty arl");

        size_t subrow = 0;
        for (; subrow < *row_vertical_len; ++subrow) {

            foreach__(SplittedCell, row, cell) {
                size_t ncol = cell - arlfn(SplittedCell,begin)(row);

                unsigned span_value = colspan_get_interpreted(colspan, (Coordinates){.row=nrow,.col=ncol});
                if (!span_value) continue;

                size_t col_hlen;
                try(get_cell_horizontal_len(ncol, span_value, cols_hlengths, &col_hlen));

                CellPart* part = arlfn(CellPart,at)(cell, subrow);
                size_t cell_part_hlen = 0;
                if (part && strview_from_draw_text_buf(part).len) {
                    try(draw_ctx_append_sub_text(text, part));
                    cell_part_hlen = cell_part_horizontal_len(part);
                }

                if (col_hlen < cell_part_hlen) return err_internal("column length computation failed");
                bool   is_last_col = is_last_column(len__(cols_hlengths), cell, row, span_value);
                size_t spaces      = (span_value - is_last_col) + col_hlen - cell_part_hlen;
                if (spaces)
                    try(draw_text_buf_append(text, whitespace(spaces)));
            }
            try(draw_text_buf_append(text, svl("\n")));
        }
    }
    return Ok;
}


static Err _expecting_err_could_not_fit_the_table(Err expr, StrView caption, DrawCtx ctx[_1_]) {
    if (expr == err_could_not_fit_the_table__) { 
        StrView url = draw_ctx_url_strview(ctx);
        if (url.len) {
            msg__(ctx, svl("drawing "));
            msg__(ctx, url);
            msg__(ctx, svl(": "));
        }
        msg__(ctx, sv(expr));
        if (caption.len) msg_ln__(ctx, caption);
        else  msg_ln__(ctx, svl(": <TABLE WITH NO CAPTION>"));
        return Ok;
    } else return expr;
}


static Err
draw_tag_table_impl (DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    Err           err                     = Ok;
    DrawTextBuf*  caption                 = &(DrawTextBuf){0};
    DrawTable     table                   = (DrawTable){0};
    DomNode       it                      = dom_skip_text(dom_node_first_child(node));
    SplittedTable splitted_table          = (SplittedTable){0};
    ArlOf(size_t) rows_vlengths   = (ArlOf(size_t)){0};
    ArlOf(size_t) cols_hlengths = (ArlOf(size_t)){0};
    ColSpan       colspan                 = (ColSpan){0};
    DrawRow       r                       = (DrawRow){0};
    size_t        screen_width            = *session_ncols(draw_ctx_session(ctx));

    if (isnull(it)) return Ok;

    try(colspan_init(&colspan));

    /* If included, the <caption> element must be the first child of its parent <table> element. */
    if (dom_node_has_tag(it, HTML_TAG_CAPTION)) {
        tryjmp(err, Clean, draw_rec_tag(it, ctx, caption));
        it = dom_skip_text(dom_node_next(it));
    }


    for (; !isnull(it); it = dom_node_next_elem(it)) {
        if (!dom_node_has_type(it, DOM_NODE_TYPE_ELEMENT)) continue; //TODO0 log this?
        switch(dom_node_tag(it)) {
            case HTML_TAG_THEAD: /*TODO1: differentiate head, body and foot */
            case HTML_TAG_TBODY:
            case HTML_TAG_TFOOT:
            {
                for (DomNode th_it = dom_node_first_elem_child(it); !isnull(th_it); th_it = dom_node_next_elem(th_it)) {
                    size_t  nrow = len__(&table);
                    tryjmp(err, Clean,  dom_read_table_row(th_it, ctx, &r, &colspan, nrow));
                    tryjmp(err, Clean,  draw_table_append(&table, &r));
                    r = (DrawRow){0};
                }
            }
            break;
            case HTML_TAG_COLGROUP: continue;
            default: { err = err_from_unexpected_tag_in_table(dom_node_tag(it)); goto Clean; }
        }
    }

    tryjmp(err, Clean, _expecting_err_could_not_fit_the_table(
        fit_table_to_screen(&table, screen_width, &colspan),
        strview_from_draw_text_buf(caption),
        ctx
    ));


    tryjmp(err, Clean, draw_table_to_splitted_view(&table, &splitted_table));
    tryjmp(err, Clean, splitted_table_row_vertical_lengths(&splitted_table, &rows_vlengths));
    tryjmp(err, Clean, splitted_table_col_horizonal_lengths(&splitted_table, &colspan, &cols_hlengths));
    tryjmp(err, Clean, _expecting_err_could_not_fit_the_table(
        expand_columns_for_colspans(&table, screen_width, &colspan, &cols_hlengths),
        strview_from_draw_text_buf(caption),
        ctx
    ));

    draw_text_buf_append(caption, svl("\n"));
    draw_ctx_append_sub_text(text, caption);

    if (!ends_with_newline(strview_from_draw_text_buf(text))) draw_text_buf_append(text, svl("\n"));
    tryjmp(err, Clean,
        draw_splitted_table(&splitted_table, &rows_vlengths, &cols_hlengths, &colspan, text));
Clean:
    arlfn(DrawTextBuf,clean)(&r);
    colspan_clean(&colspan);
    arlfn(size_t,clean)(&cols_hlengths);
    arlfn(size_t,clean)(&rows_vlengths);
    splitted_table_clean(&splitted_table);
    draw_table_clean(&table);
    draw_text_buf_clean(caption);
    return err;
}


static Err
draw_tag_table (DomNode node, DrawCtx ctx[_1_], DrawTextBuf text[_1_]) {
    Err err = draw_tag_table_impl(node, ctx, text);
    if (err != err_unexpected_tag_in_table) return err;
    try(msg_ln__(get_cmd_out_(ctx), err));
    return draw_iter_childs(node, ctx, text);
}


Err
htmldoc_reparse_source(HtmlDoc d[_1_]) {
    Dom dom = htmldoc_dom(d);
    dom_reset(dom);
    Str* html = textbuf_buf(htmldoc_sourcebuf(d));
    return dom_parse(dom, sv(html));
}


Err
htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[_1_]) {
    if (!htmldoc_content_is_html(d)) return Ok;
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
    StrView lineview = sv(line);
    if (!lineview.len) fail("{console command requires some javascript input line}");
    return jse_eval(htmldoc_js(d), s, lineview, out);
}


static Err
jse_eval_doc_scripts(Session* s, HtmlDoc d[_1_], CmdOut* out) {

    foreach__(Str, htmldoc_head_scripts(d), it) {
        if (len__(it)) {
            Err e = jse_eval(htmldoc_js(d), s, sv(it), out);
            if (e) msg__(out, e);
        }
    }
    foreach__(Str, htmldoc_body_scripts(d), it) {
        if (len__(it)) {
            Err e = jse_eval(htmldoc_js(d), s, sv(it), out);
            if (e) msg__(out, e);
        }
    }

    return Ok;
}


Err
htmldoc_js_enable(HtmlDoc d[_1_], Session* s, CmdOut* out) {
    if (!s) return err_internal("expecting non empty session");
    try( jse_init(d));

    if (0 == len__(htmldoc_head_scripts(d)) + len__(htmldoc_body_scripts(d))) {
        CurlPtr easy;
        try(w_curl_easy_init(&easy));
        Err e = htmldoc_fetch_scripts(d, session_url_client(s), easy, out);
        curl_ptr_clean(&easy);
        try (e);
    }

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
    if (head_script_count + body_script_count != len__(htmldoc_scripts(h)))
        return "scripts not fetched";
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
    bool              fetch_scripts,
    CmdOut            out[_1_],
    FetchHistoryEntry histentry[_1_],
    CurlPtr           easy
);

Err
htmldoc_fetch(
    HtmlDoc           htmldoc[_1_],
    UrlClient         url_client[_1_],
    bool              fetch_scripts,
    CmdOut            cmd_out[_1_],
    FetchHistoryEntry he[_1_]
)
{
    CurlPtr* easy = request_curl_handle(htmldoc_request(htmldoc));
    if (!easy) fail("error: expecting an easy handled");
    Err e = curl_lexbor_fetch_document(url_client, htmldoc, fetch_scripts, cmd_out, he, *easy);
    return e;
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
        foreach__(ModAt,mods,it) {
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
#define TEXT_HTML__ "text/html"
    StrView content_type = sv(htmldoc_http_content_type(d));
    return !content_type.len || str_eq_case(content_type, svl(TEXT_HTML__));
#undef TEXT_HTML__
}

bool htmldoc_content_is_html(HtmlDoc d[_1_]) {
    Request* r = htmldoc_request(d);
    if (!request_is_local(r)) return htmldoc_http_content_type_text_or_undef(d);

    char*  url  = items__(request_urlstr(r));
    size_t len  = len__(request_urlstr(r));
    if (!len || !url) return false;
    size_t dot = len - 1;
    for (;dot; --dot) if (url[dot] == '.') break;
    return url[dot] == '.' && str_eq_case(svl(".html"), sv(url + dot, len - dot));
}


Err
htmldoc_title(HtmlDoc d[_1_], DomNode title[_1_]) { return dom_get_title_node(htmldoc_dom(d), title); }
