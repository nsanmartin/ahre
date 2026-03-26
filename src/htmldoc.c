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

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[_1_]);
Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[_1_]);

/* internal linkage */
#define MAX_URL_LEN 2048u
#define READ_FROM_FILE_BUFFER_LEN 4096u
#define LAZY_STR_BUF_LEN 1600u

#define append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(LxbNodePtr,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")


static size_t _strview_trim_left_count_newlines_(StrView s[_1_]) {
    size_t newlines = 0;
    while(s->len && isspace(*(items__(s)))) {
        newlines += *(items__(s)) == '\n';
        ++s->items;
        --s->len;
    }
    return newlines;
}

static void draw_subctx_trim_left(DrawSubCtx sub[_1_]) {
    StrView content = strview_from_mem(sub->buf.items, sub->buf.len);
    sub->left_newlines = _strview_trim_left_count_newlines_(&content);
    sub->left_trim = sub->buf.len-content.len;
    textmod_trim_left(&sub->mods, sub->left_trim);
}


static Err _hypertext_id_open_(
    DrawCtx ctx[_1_],
    DrawEffectCb visual_effect,
    StrViewProvider open_str_provider,
    const size_t* id_num_ptr,
    StrViewProvider sep_str_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (open_str_provider) try( draw_ctx_buf_append(ctx, open_str_provider()));
    if (id_num_ptr) try( draw_ctx_buf_append_ui_base36_(ctx, *id_num_ptr));
    if (sep_str_provider) try( draw_ctx_buf_append(ctx, sep_str_provider()));
    return Ok;
}

static Err _hypertext_open_(
    DrawCtx ctx[_1_], DrawEffectCb visual_effect, StrViewProvider prefix_str_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (prefix_str_provider) try( draw_ctx_buf_append(ctx, prefix_str_provider()));
    return Ok;
}


static Err _hypertext_id_close_(
    DrawCtx ctx[_1_],
    DrawEffectCb visual_effect,
    StrViewProvider close_str_provider
) {
    if (close_str_provider) try( draw_ctx_buf_append(ctx, close_str_provider()));
    if (visual_effect) try( visual_effect(ctx));
    return Ok;
}

static Err _hypertext_close_(
    DrawCtx ctx[_1_],
    DrawEffectCb visual_effect,
    StrViewProvider postfix_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (postfix_provider) try( draw_ctx_buf_append(ctx, postfix_provider()));
    return Ok;
}



static Err
draw_tag_br(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    try( draw_ctx_buf_append_lit__(ctx, "\n"));
    return draw_list(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_center(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    //TODO: store center boundaries (start = len on enter, end = len on ret) and then
    // when fitting to width center those lines image.

    //try( draw_ctx_buf_append_lit__(ctx, "%{[center]:\n"));
    try( draw_list(node->first_child, node->last_child, ctx));
    //try( draw_ctx_buf_append_lit__(ctx, "%}[center]\n"));
    return Ok;
}

static Err
browse_ctx_append_img_alt_(lxb_dom_node_t* img, DrawCtx ctx[_1_]) {

    const lxb_char_t* alt;
    size_t alt_len;
    if (lexbor_find_lit_attr_value__(img, "alt", &alt, &alt_len)) {
        try( draw_ctx_buf_append(ctx, strview__((char*)alt, alt_len)));
    }
    return Ok;
}

static Err
draw_tag_img(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* imgs = htmldoc_imgs(d);
    const size_t img_count = len__(imgs);

    try( append_to_arlof_lxb_node__(imgs, &node));

    try( _hypertext_id_open_(
            ctx, draw_ctx_color_light_green, image_open_str, &img_count, image_close_str));

    try( browse_ctx_append_img_alt_(node, ctx));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, NULL));
    return Ok;
}


static Err
draw_tag_select(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    lxb_dom_node_t* selected = node->first_child;
    if (!selected) return Ok;
    for(lxb_dom_node_t* it = selected; it ; it = it->next) {
        if (it->type == LXB_DOM_NODE_TYPE_ELEMENT 
            && it->local_name == LXB_TAG_OPTION
            && lexbor_has_lit_attr__(it, "selected")
        ) {
            selected = it;
            break;
        }
    }

    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(draw_ctx_htmldoc(ctx));
    if (!arlfn(LxbNodePtr,append)(inputs, &node)) { return "error: arl set"; }
    size_t input_id = len__(inputs)-1;
    try( _hypertext_id_open_(
        ctx, draw_ctx_color_red, input_text_open_str, &input_id, input_select_sep_str));

    size_t len;
    const char* text;
    for(lxb_dom_node_t* txt = selected->first_child; txt ; txt = txt->next) {
        try( lexbor_node_get_text(txt, &text, &len));
        if (len) try( draw_ctx_buf_append(ctx, strview__((char*)text, len)));
    }
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, input_submit_close_str));

    return Ok;
}


static Err
draw_tag_form(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    ArlOf(LxbNodePtr)* forms = htmldoc_forms(draw_ctx_htmldoc(ctx));
    if (!arlfn(LxbNodePtr,append)(forms, &node)) return "error: lip set";
    const size_t form_count = len__(forms);

    bool show = session_conf_show_forms(session_conf(draw_ctx_session(ctx)));
    if (show) {
        try( _hypertext_id_open_(
                ctx, draw_ctx_color_purple, form_open_str, &form_count, form_sep_str));

        try( draw_ctx_reset_color(ctx));
    }

    try (draw_list_block(node->first_child, node->last_child, ctx));

    if (show) {
        try( draw_ctx_buf_append_color_(ctx, esc_code_purple));
        try( _hypertext_id_close_(ctx, draw_ctx_reset_color, form_close_str));
    }
    return Ok;
}


static bool _input_is_text_type_(const lxb_char_t* name, size_t len) {
    return !len
        || lexbor_str_eq("text", name, len)
        || lexbor_str_eq("search", name, len)
        ;
}

static bool _input_is_submit_type_(const lxb_char_t* name, size_t len) {
    return lexbor_str_eq("submit", name, len);
}

static Err
draw_tag_button(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
    if (!arlfn(LxbNodePtr,append)(inputs, &node)) return "error: lip set";
    size_t input_id =len__(inputs)-1;

    try( _hypertext_id_open_(
        ctx, draw_ctx_color_red, button_open_str, &input_id, button_sep_str));
    

    try (draw_list(node->first_child, node->last_child, ctx));

    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, button_close_str));
    return Ok;
}


static Err draw_tag_input(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    const lxb_char_t* s;
    size_t slen;

    if (lexbor_find_lit_attr_value__(node, "type", &s, &slen) && !strcmp("hidden", (char*)s))
        return Ok;

    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
    if (!arlfn(LxbNodePtr,append)(inputs, &node)) { return "error: arl set"; }
    size_t input_id = len__(inputs)-1;


    /* submit */
    if (_input_is_submit_type_(s, slen)) {

        lexbor_find_lit_attr_value__(node, "value", &s, &slen);

        StrViewProvider sep = slen ? input_submit_sep_str : NULL;
        try( _hypertext_id_open_(ctx, draw_ctx_color_red, input_text_open_str, &input_id, sep));
        if (slen) try( draw_ctx_buf_append(ctx, strview__((char*)s, slen)));
        try( _hypertext_id_close_(ctx, draw_ctx_reset_color, input_submit_close_str));
        return Ok;

    } 

    /* other */
    try( _hypertext_id_open_(
        ctx, draw_ctx_color_red, input_text_open_str, &input_id, NULL));

    if (_input_is_text_type_(s, slen)) {
        try( draw_ctx_buf_append_lit__(ctx, "="));
        if (lexbor_find_lit_attr_value__(node, "value", &s, &slen)) {
            try( draw_ctx_buf_append(ctx, strview__((char*)s, slen)));
        }
    } else if (lexbor_str_eq("password", s, slen)) {
        if (lexbor_find_lit_attr_value__(node, "value", &s, &slen)) {
            try( draw_ctx_buf_append_lit__(ctx, "=********"));
        } else try( draw_ctx_buf_append_lit__(ctx, "=________"));
    } else {
        try( draw_ctx_buf_append_lit__(ctx, "[input not supported yet]"));
    }
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, input_text_close_str));
    return Ok;
}


static Err draw_tag_div(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    draw_subctx_div_set(ctx);
    DrawSubCtx sub = (DrawSubCtx){0};
    draw_ctx_swap_sub(ctx, &sub);

    Err err = draw_list(node->first_child, node->last_child, ctx);

    if (!err) draw_ctx_swap_sub(ctx, &sub);

    if (sub.buf.len) {
        if (len__(draw_ctx_buf(ctx))) {
            ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
            if(!err) draw_subctx_div_clear(ctx);
        }
        ok_then(err, draw_ctx_append_subctx(ctx, &sub));
    }
        
    draw_subctx_clean(&sub);
    return err;
}

static Err
draw_tag_p(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    return draw_list_block(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_tr(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    try( draw_ctx_buf_append_lit__(ctx, "\n"));
    try( draw_list(node->first_child, node->last_child, ctx));
    return Ok;
}

static Err
draw_tag_ul(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    if(draw_ctx_hide_tags(ctx, HIDE_UL)) return Ok;
    return draw_list_block(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_li(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    DrawSubCtx sub = (DrawSubCtx){0};
    draw_ctx_swap_sub(ctx, &sub);

    Err err = draw_list(node->first_child, node->last_child, ctx);

    draw_ctx_swap_sub(ctx, &sub);

    draw_subctx_trim_left(&sub);

    if (!err && sub.buf.len) {
        ok_then(err, draw_ctx_buf_append_lit__(ctx, " * "));
        ok_then(err, draw_ctx_append_subctx(ctx, &sub));
        if (!err && sub.buf.items[sub.buf.len-1] != '\n')
            ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
    }

    draw_subctx_clean(&sub);
    return Ok;
}


static Err draw_tag_h(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    DrawSubCtx sub = (DrawSubCtx){0};
    draw_ctx_swap_sub(ctx, &sub);

    try( draw_list_block(node->first_child, node->last_child, ctx));

    draw_ctx_swap_sub(ctx, &sub);

    draw_subctx_trim_left(&sub);
    draw_subctx_trim_right(&sub);

    Err err = _hypertext_open_(ctx, draw_ctx_push_bold, h_tag_open_str);
    if (!err && sub.buf.len) ok_then(err, draw_ctx_append_subctx(ctx, &sub));
    ok_then(err, _hypertext_close_(ctx, draw_ctx_reset_color, newline_str));

    draw_subctx_clean(&sub);
    return Ok;
}

static Err
draw_tag_code(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    try( draw_ctx_buf_append_lit__(ctx, " `"));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( draw_ctx_buf_append_lit__(ctx, "` "));
    return Ok;
}

static Err
draw_tag_b(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    if (node->prev && draw_ctx_buf_last_isgraph(ctx)) try( draw_ctx_buf_append_lit__(ctx, " "));
    try(_hypertext_open_(ctx, draw_ctx_push_bold, space_str));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_close_(ctx, draw_ctx_reset_color, space_str));
    return Ok;
}

static Err draw_tag_em(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    if (node->prev && draw_ctx_buf_last_isgraph(ctx)) try( draw_ctx_buf_append_lit__(ctx, " "));
    try(_hypertext_open_(ctx, draw_ctx_push_underline, space_str));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_close_(ctx, draw_ctx_reset_color, space_str));
    return Ok;
}

static Err
draw_tag_i(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    try(_hypertext_open_(ctx, draw_ctx_push_italic, space_str));
    //try( draw_ctx_buf_append_lit__(ctx, " "));
    //try( draw_ctx_buf_append_color_(ctx, esc_code_italic));
    try (draw_list(node->first_child, node->last_child, ctx));
    //try( draw_ctx_reset_color(ctx));
    //try( draw_ctx_buf_append_lit__(ctx, " "));
    try( _hypertext_close_(ctx, draw_ctx_reset_color, space_str));
    return Ok;
}

static Err
draw_tag_blockquote(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    try( draw_ctx_buf_append_lit__(ctx, "``"));
    try (draw_list_block(node->first_child, node->last_child, ctx));
    try( draw_ctx_buf_append_lit__(ctx, "''"));
    return Ok;
}

static Err draw_tag_title(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    *htmldoc_title(d) = node;
    return Ok;
}

static Err draw_mem_skipping_space(
    const char* data, size_t len, DrawCtx ctx[_1_], lxb_dom_node_t* prev
) {
    StrView s = strview_from_mem(data, len);
    /* if it starts with a punctiayion mark we undo the space added in draw space */
    if (prev && !ispunct(s.items[0])) try( draw_ctx_buf_append_lit__(ctx, " "));
    while(s.len) {
        StrView word = strview_split_word(&s);
        if (!word.len) break;
        try( draw_ctx_buf_append(ctx, word));
        strview_trim_space_left(&s);
        if (!s.len) break;
        try( draw_ctx_buf_append_lit__(ctx, " "));
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
static Err draw_rec_tag(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    if (ctx->fragment
    && lexbor_element_id_cstr_match(lxb_dom_interface_element(node), ctx->fragment)) {
        *draw_ctx_fragment_offset(ctx) = len__(draw_ctx_buf(ctx));
    }
    switch(node->local_name) {
        case LXB_TAG_A: { return draw_tag_a(node, ctx); }
        case LXB_TAG_B: { return draw_tag_b(node, ctx); }
        case LXB_TAG_BLOCKQUOTE: { return draw_tag_blockquote(node, ctx); }
        case LXB_TAG_BR: { return draw_tag_br(node, ctx); }
        case LXB_TAG_BUTTON: { return draw_tag_button(node, ctx); }
        case LXB_TAG_CENTER: { return draw_tag_center(node, ctx); } 
        case LXB_TAG_CODE: { return draw_tag_code(node, ctx); } 
        case LXB_TAG_DIV: { return draw_tag_div(node, ctx); }
        case LXB_TAG_DL: { return draw_list_block(node->first_child, node->last_child, ctx); }
        case LXB_TAG_DT: { return draw_list_block(node->first_child, node->last_child, ctx); }
        case LXB_TAG_EM: { return draw_tag_em(node, ctx); }
        case LXB_TAG_FORM: { return draw_tag_form(node, ctx); }
        case LXB_TAG_H1: case LXB_TAG_H2: case LXB_TAG_H3: case LXB_TAG_H4: case LXB_TAG_H5: case LXB_TAG_H6: { return draw_tag_h(node, ctx); }
        case LXB_TAG_I: { return draw_tag_i(node, ctx); }
        case LXB_TAG_INPUT: { return draw_tag_input(node, ctx); }
        case LXB_TAG_IMAGE: case LXB_TAG_IMG: { return draw_tag_img(node, ctx); }
        case LXB_TAG_LI: { return draw_tag_li(node, ctx); }
        case LXB_TAG_META: { return Ok; }
        case LXB_TAG_OL: { return draw_tag_ul(node, ctx); }
        case LXB_TAG_P: { return draw_tag_p(node, ctx); }
        case LXB_TAG_PRE: { return draw_tag_pre(node, ctx); }
        case LXB_TAG_SCRIPT: {
            return Ok; /*
                          draw_tag_script(node, ctx);
                          Here draw only body scripts
                          */
        } 
        case LXB_TAG_SELECT: { return draw_tag_select(node, ctx); }
        case LXB_TAG_STYLE: {
            /* Does it make any sense that wo dosomething with this in ahre? */
            return Ok;
        } 
        case LXB_TAG_TITLE: { return draw_tag_title(node, ctx); } 
        case LXB_TAG_TR: { return draw_tag_tr(node, ctx); }
        case LXB_TAG_TT: { return draw_tag_code(node, ctx); }
        case LXB_TAG_UL: { return draw_tag_ul(node, ctx); }
        default: {
            /* if (node->local_name >= LXB_TAG__LAST_ENTRY) */
            /*     return err_fmt( */
            /*         "error: node local name (TAG) greater than last entry: %lx\n", node->local_name */
            /*     ); */
            /* else TODO: "TAG 'NOT' IMPLEMENTED: %s", node->local_name */
            return draw_list(node->first_child, node->last_child, ctx);
        }
    }
}

static Err draw_text(lxb_dom_node_t* node,  DrawCtx ctx[_1_]) {
    const char* data;
    size_t len;
    try( lexbor_node_get_text(node, &data, &len));

    if (draw_ctx_pre(ctx)) {
        //TODO Whitespace inside this element is displayed as written,
        //with one exception. If one or more leading newline characters
        //are included immediately following the opening <pre> tag, the
        //first newline character is stripped. 
        //https://developer.mozilla.org/en-US/docs/Web/HTML/Element/pre
        try( draw_ctx_buf_append(ctx, strview__((char*)data, len)));
    } else if (mem_skip_space_inplace(&data, &len)) {
        /* If it's not the first then separate with previous with space */
        try( draw_mem_skipping_space(data, len, ctx, node->prev));
    } 

    if (node->first_child || node->last_child)
        return "error: unexpected text elem with childs";
    return Ok;
}

Err draw_rec(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    if (node) {
        switch(node->type) {
            case LXB_DOM_NODE_TYPE_ELEMENT: return draw_rec_tag(node, ctx);
            case LXB_DOM_NODE_TYPE_TEXT: return draw_text(node, ctx);
            //TODO: do not ignore these types?
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                return draw_list(node->first_child, node->last_child, ctx);
            default: {
                if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                    return err_fmt("error:""lexbor node type greater than last entry: %lx\n", node->type
                    );
                /* else TODO: "Ignored Node Type: %s\n", _dbg_node_types_[node->type]*/
                return Ok;
            }
        }
    }
    return Ok;
}

Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    draw_ctx_pre_set(ctx, true);
    try( draw_list_block(node->first_child, node->last_child, ctx));
    draw_ctx_pre_set(ctx, false);
    return Ok;
}

/*TODO: decouple sesion and htmldoc here */
Err _htmldoc_draw_with_flags_(HtmlDoc htmldoc[_1_], Session* s, unsigned flags) {
    if (!s) return "error: expecting not null Session";
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    DrawCtx ctx;
    Err err = draw_ctx_init(&ctx, htmldoc, s, flags);
    try(err);
    ok_then(err, draw_rec(lxb_dom_interface_node(lxbdoc), &ctx));
    ok_then(err, draw_ctx_buf_commit(&ctx));
    ok_then(err, textbuf_fit_lines(htmldoc_textbuf(htmldoc), *session_ncols(s)));

    draw_ctx_cleanup(&ctx);
    if (err) arlfn(ModAt,clean) (draw_ctx_mods(&ctx));
    //TODO: if a redraw was solicited, we may instead cleanup and re init.
    return err;
}


static Err _htmldoc_draw_(HtmlDoc htmldoc[_1_], Session s[_1_]) {
    unsigned flags = draw_ctx_flags_from_session(s) | DRAW_CTX_FLAG_TITLE;
    return _htmldoc_draw_with_flags_(htmldoc, s, flags);
}


#define HTMLDOC_FLAG_JS 0x1u


Err htmldoc_init_move_request(
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
        .lxbdoc = lxb_html_document_create()
    };
    if (!d->lxbdoc) return "error: lxb failed to create html document";
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


Err htmldoc_init_bookmark_move_urlstr(HtmlDoc d[_1_], Str urlstr[_1_]) {
    Err e = Ok;

    *d = (HtmlDoc){ .lxbdoc = lxb_html_document_create() };
    if (!d->lxbdoc) return "error: lxb failed to create html document";
    if (!urlstr) return "error: cannot initialize bookmark with not path";
    try_or_jump(e, Failure_Lxb_Html_Document_Destroy,
        request_init_move_urlstr(htmldoc_request(d), http_get, urlstr, NULL));
    return Ok;

Failure_Lxb_Html_Document_Destroy:
    lxb_html_document_destroy(d->lxbdoc);
    return e;
}


/* external linkage */


void htmldoc_reset_draw(HtmlDoc htmldoc[_1_]) {
    textbuf_reset(htmldoc_textbuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_forms(htmldoc));
}


static void htmldoc_fetchcache_cleanup(HtmlDoc htmldoc[_1_]) {
    textbuf_cleanup(htmldoc_sourcebuf(htmldoc));
    arlfn(Str,clean)(htmldoc_head_scripts(htmldoc));
    arlfn(Str,clean)(htmldoc_body_scripts(htmldoc));
    htmldoc->fetch_cache = (DocFetchCache){0};
}

static void htmldoc_drawcache_cleanup(HtmlDoc htmldoc[_1_]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_forms(htmldoc));

    str_clean(htmldoc_screen(htmldoc));
    htmldoc->draw_cache = (DocDrawCache){0};
}

void htmldoc_cache_cleanup(HtmlDoc htmldoc[_1_]) {
    htmldoc_drawcache_cleanup(htmldoc);
    htmldoc_fetchcache_cleanup(htmldoc);
}



void htmldoc_cleanup(HtmlDoc htmldoc[_1_]) {
    http_header_clean(htmldoc_http_header(htmldoc));
    htmldoc_cache_cleanup(htmldoc);
    lxb_html_document_destroy(htmldoc_lxbdoc(htmldoc));
    url_cleanup(htmldoc_url(htmldoc));
    request_clean(htmldoc_request(htmldoc));

    if (jse_rt(htmldoc_js(htmldoc))) jse_clean(htmldoc_js(htmldoc));
}


inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}



Err htmldoc_A(Session* s, HtmlDoc d[_1_], CmdOut* out) {
    if (!s) return "error: no session";
    cmd_out_msg_append_lit__(out, "<li><a href=\"");
    char* url_buf;
    try( url_cstr_malloc(htmldoc_url(d), &url_buf));
    cmd_out_msg_append(out, url_buf, strlen(url_buf));
    curl_free(url_buf);
    cmd_out_msg_append_lit__(out, "\">");
    try( lexbor_get_title_text_line(*htmldoc_title(d), msg_str(cmd_out_msg(out))));
    cmd_out_msg_append_lit__(out, "</a>");
    cmd_out_msg_append_lit__(out, "\n");
    return Ok;
}

Err htmldoc_print_info(HtmlDoc d[_1_], CmdOut* out) {
    Err err = Ok;
    LxbNodePtr* title = htmldoc_title(d);

    try (cmd_out_msg_append_lit__(out, "DOWNLOAD SIZE: "));
    try (cmd_out_msg_append_ui_as_base10(out, *htmldoc_curlinfo_sz_download(d)));
    try (cmd_out_msg_append_lit__(out, "\n"));
    try (cmd_out_msg_append_lit__(out, "html size: "));
    try (cmd_out_msg_append_ui_as_base10(out, htmldoc_sourcebuf(d)->buf.len));
    try (cmd_out_msg_append_lit__(out, "\n"));

    if (title) {
        Str buf = (Str){0};
        try (lexbor_get_title_text(*title, &buf));
        if (!buf.len) err = cmd_out_msg_append_lit__(out,  "<NO TITLE>");
        else err = cmd_out_msg_append(out, buf.items, buf.len);
        str_clean(&buf);
        try(err);
    }

    try(cmd_out_msg_append_lit__(out, "\n"));
    
    char* url = NULL;
    ok_then(err, url_cstr_malloc(htmldoc_url(d), &url));
    if (url) {
        ok_then(err, cmd_out_msg_append_ln(out, url, strlen(url)));
        curl_free(url);
    }

    Str* charset = htmldoc_http_charset(d);
    if (len__(charset))
        ok_then(err, cmd_out_msg_append_str_ln(out, charset));

    Str* content_type = htmldoc_http_content_type(d);
    if (len__(content_type))
        ok_then(err, cmd_out_msg_append_str_ln(out, content_type));

    return err;
}

static bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    return lexbor_find_lit_attr_value__(node, "href", &data, &data_len);
}

static bool _prev_is_separable_(lxb_dom_node_t n[_1_]) {
    return n->prev  && 
        n->prev->local_name != LXB_TAG_LI
        ;
}

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[_1_]) {
    /* https://html.spec.whatwg.org/multipage/links.html#attr-hyperlink-href
     * The href attribute on a and area elements is not required; when those
     * elements do not have href attributes they do not create hyperlinks. */
    bool is_hyperlink = _node_has_href(node);
    if (is_hyperlink) {

        HtmlDoc* d = draw_ctx_htmldoc(ctx);
        ArlOf(LxbNodePtr)* anchors = htmldoc_anchors(d);
        const size_t anchor_num = anchors->len;
        if (!arlfn(LxbNodePtr,append)(anchors, &node)) 
            return "error: lip set";

        DrawSubCtx sub = (DrawSubCtx){0};
        draw_ctx_swap_sub(ctx, &sub);

        if (draw_ctx_color(ctx)) try( draw_ctx_esc_code_push(ctx, esc_code_blue));
        Err err = draw_list(node->first_child, node->last_child, ctx);
        if (err) {
            draw_subctx_clean(&sub);
            return err;
        }

        bool div_child = draw_subctx_div(ctx);
        draw_ctx_swap_sub(ctx, &sub);

        draw_subctx_trim_left(&sub);
        draw_subctx_trim_right(&sub);

        if (_prev_is_separable_(node)) err = draw_ctx_buf_append_lit__(ctx, " ");
        else if (sub.left_newlines) err = draw_ctx_buf_append_lit__(ctx, "\n");
        ok_then(err, _hypertext_id_open_(
                ctx, draw_ctx_textmod_blue, anchor_open_str, &anchor_num,anchor_sep_str ));

        if (!err && sub.buf.len) ok_then(err, draw_ctx_append_subctx(ctx, &sub));
        ok_then(err, _hypertext_id_close_(ctx, draw_ctx_reset_color, anchor_close_str));
        if (sub.right_newlines) ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        if (div_child) ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        draw_subctx_div_clear(ctx);

        draw_subctx_clean(&sub);
        try(err);

    } else try( draw_list(node->first_child, node->last_child, ctx));//TODO: do this?
   
    return Ok;
}

static Err htmldoc_reparse_source(HtmlDoc d[_1_]) {
    lxb_html_document_t* document = htmldoc_lxbdoc(d);
    lxb_html_document_clean(document);
    Str* html = textbuf_buf(htmldoc_sourcebuf(d));
    if (LXB_STATUS_OK !=
        lxb_html_document_parse(document, (const lxb_char_t*)items__(html), len__(html)))
        return "error: lexbor reparse failed";
    return Ok;
}

//TODO: 
//  liblexbor supports encoding convertion (https://github.com/lexbor/lexbor/issues/271)
Err htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[_1_]) {
    if (!htmldoc_http_content_type_text_or_undef(d)) return Ok;
    const char* utf8s;
    size_t utf8slen;
    if (!htmldoc_http_charset_is_utf8(d)) {
        Str* buf = textbuf_buf(htmldoc_sourcebuf(d));
        char* from_charset = items__(htmldoc_http_charset(d));
        try( _convert_to_utf8_(items__(buf), len__(buf), from_charset, &utf8s, &utf8slen));
        std_free(buf->items);
        buf->items = (char*)utf8s;
        buf->len = utf8slen;
        buf->capacity = utf8slen;
        try( htmldoc_reparse_source(d));
    }
    return Ok;
}

Err htmldoc_console(HtmlDoc d[_1_], Session* s, const char* line, CmdOut* out) {
    if (!s) return "error: no session";
    return jse_eval(htmldoc_js(d), s, line, out);
}

static Err jse_eval_doc_scripts(Session* s, HtmlDoc d[_1_], CmdOut* out) {

    for ( Str* it = arlfn(Str,begin)(htmldoc_head_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_head_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, items__(it), out);
        if (e) cmd_out_msg_append(out, (char*)e, strlen(e));
    }

    for ( Str* it = arlfn(Str,begin)(htmldoc_body_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_body_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, items__(it), out);
        if (e) cmd_out_msg_append(out, (char*)e, strlen(e));
    }

    return Ok;
}

//TODO: make this fn not Err and rename it
Err htmldoc_js_enable(HtmlDoc d[_1_], Session* s, CmdOut* out) {
    try( jse_init(d));
    Err e = jse_eval_doc_scripts(s, d, out);
    if (e) cmd_out_msg_append(out, (char*)e, strlen(e));
    return Ok;
}

void htmldoc_eval_js_scripts_or_continue(HtmlDoc d[_1_], Session* s, CmdOut* out) {
    if (htmldoc_js_is_enabled(d)) {
        Err e = jse_eval_doc_scripts(s, d, out);
        if (e) cmd_out_msg_append(out, (char*)e, strlen(e));
    }
}

static Err _htmldoc_scripts_range_from_parsed_range_(
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


Err htmldoc_scripts_write(HtmlDoc h[_1_], RangeParse rp[_1_], Writer w[_1_]) {
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

