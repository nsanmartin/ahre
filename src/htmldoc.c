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

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[static 1]);
Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[static 1]);

/* internal linkage */
#define MAX_URL_LEN 2048u
#define READ_FROM_FILE_BUFFER_LEN 4096u
#define LAZY_STR_BUF_LEN 1600u

#define append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(LxbNodePtr,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")


static Err _hypertext_id_open_(
    DrawCtx ctx[static 1],
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
    DrawCtx ctx[static 1], DrawEffectCb visual_effect, StrViewProvider prefix_str_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (prefix_str_provider) try( draw_ctx_buf_append(ctx, prefix_str_provider()));
    return Ok;
}


static Err _hypertext_id_close_(
    DrawCtx ctx[static 1],
    DrawEffectCb visual_effect,
    StrViewProvider close_str_provider
) {
    if (close_str_provider) try( draw_ctx_buf_append(ctx, close_str_provider()));
    if (visual_effect) try( visual_effect(ctx));
    return Ok;
}

static Err _hypertext_close_(
    DrawCtx ctx[static 1],
    DrawEffectCb visual_effect,
    StrViewProvider postfix_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (postfix_provider) try( draw_ctx_buf_append(ctx, postfix_provider()));
    return Ok;
}



static Err
draw_tag_br(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    try( draw_ctx_buf_append_lit__(ctx, "\n"));
    return draw_list(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_center(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    //TODO: store center boundaries (start = len on enter, end = len on ret) and then
    // when fitting to width center those lines image.

    //try( draw_ctx_buf_append_lit__(ctx, "%{[center]:\n"));
    try( draw_list(node->first_child, node->last_child, ctx));
    //try( draw_ctx_buf_append_lit__(ctx, "%}[center]\n"));
    return Ok;
}

static Err
browse_ctx_append_img_alt_(lxb_dom_node_t* img, DrawCtx ctx[static 1]) {

    const lxb_char_t* alt;
    size_t alt_len;
    if (lexbor_find_lit_attr_value__(img, "alt", &alt, &alt_len)) {
        try( draw_ctx_buf_append(ctx, strview__((char*)alt, alt_len)));
    }
    return Ok;
}

static Err
draw_tag_img(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_select(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_form(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_button(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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


static Err draw_tag_input(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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


static Err draw_tag_div(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_p(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    return draw_list_block(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_tr(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    try( draw_ctx_buf_append_lit__(ctx, "\n"));
    try( draw_list(node->first_child, node->last_child, ctx));
    return Ok;
}

static Err
draw_tag_ul(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    if(draw_ctx_hide_tags(ctx, HIDE_UL)) return Ok;
    return draw_list_block(node->first_child, node->last_child, ctx);
}

static Err
draw_tag_li(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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


static Err draw_tag_h(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_code(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    try( draw_ctx_buf_append_lit__(ctx, " `"));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( draw_ctx_buf_append_lit__(ctx, "` "));
    return Ok;
}

static Err
draw_tag_b(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    if (node->prev && draw_ctx_buf_last_isgraph(ctx)) try( draw_ctx_buf_append_lit__(ctx, " "));
    try(_hypertext_open_(ctx, draw_ctx_push_bold, space_str));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_close_(ctx, draw_ctx_reset_color, space_str));
    return Ok;
}

static Err draw_tag_em(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    if (node->prev && draw_ctx_buf_last_isgraph(ctx)) try( draw_ctx_buf_append_lit__(ctx, " "));
    try(_hypertext_open_(ctx, draw_ctx_push_underline, space_str));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_close_(ctx, draw_ctx_reset_color, space_str));
    return Ok;
}

static Err
draw_tag_i(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
draw_tag_blockquote(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    try( draw_ctx_buf_append_lit__(ctx, "``"));
    try (draw_list_block(node->first_child, node->last_child, ctx));
    try( draw_ctx_buf_append_lit__(ctx, "''"));
    return Ok;
}

static Err draw_tag_title(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    *htmldoc_title(d) = node;
    if (!(ctx->flags & DRAW_CTX_FLAG_TITLE)) return Ok;
    Str title = (Str){0};
    Err err = lexbor_get_title_text_line(node, &title);
    if (!title.len)  return Ok;
    ok_then(err, str_append_lit__(&title, "\n\0"));
    str_clean(&title); 
    return err;
}

static Err draw_mem_skipping_space(
    const char* data, size_t len, DrawCtx ctx[static 1], lxb_dom_node_t* prev
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


static const char* _dbg_node_types_[] = {
    [0x00] = "LXB_DOM_NODE_TYPE_UNDEF"                 ,
    [0x01] = "LXB_DOM_NODE_TYPE_ELEMENT"               ,
    [0x02] = "LXB_DOM_NODE_TYPE_ATTRIBUTE"             ,
    [0x03] = "LXB_DOM_NODE_TYPE_TEXT"                  ,
    [0x04] = "LXB_DOM_NODE_TYPE_CDATA_SECTION"         ,
    [0x05] = "LXB_DOM_NODE_TYPE_ENTITY_REFERENCE"      , // historical
    [0x06] = "LXB_DOM_NODE_TYPE_ENTITY"                , // historical
    [0x07] = "LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION",
    [0x08] = "LXB_DOM_NODE_TYPE_COMMENT"               ,
    [0x09] = "LXB_DOM_NODE_TYPE_DOCUMENT"              ,
    [0x0A] = "LXB_DOM_NODE_TYPE_DOCUMENT_TYPE"         ,
    [0x0B] = "LXB_DOM_NODE_TYPE_DOCUMENT_FRAGMENT"     ,
    [0x0C] = "LXB_DOM_NODE_TYPE_NOTATION"              , // historical
    [0x0D] = "LXB_DOM_NODE_TYPE_LAST_ENTRY"
};


static const char* _dbg_tags_[] = {
    [0x0000] = "LXB_TAG__UNDEF",
    [0x0001] = "LXB_TAG__END_OF_FILE",
    [0x0002] = "LXB_TAG__TEXT",
    [0x0003] = "LXB_TAG__DOCUMENT",
    [0x0004] = "LXB_TAG__EM_COMMENT",
    [0x0005] = "LXB_TAG__EM_DOCTYPE",
    [0x0006] = "LXB_TAG_A",
    [0x0007] = "LXB_TAG_ABBR",
    [0x0008] = "LXB_TAG_ACRONYM",
    [0x0009] = "LXB_TAG_ADDRESS",
    [0x000a] = "LXB_TAG_ALTGLYPH",
    [0x000b] = "LXB_TAG_ALTGLYPHDEF",
    [0x000c] = "LXB_TAG_ALTGLYPHITEM",
    [0x000d] = "LXB_TAG_ANIMATECOLOR",
    [0x000e] = "LXB_TAG_ANIMATEMOTION",
    [0x000f] = "LXB_TAG_ANIMATETRANSFORM",
    [0x0010] = "LXB_TAG_ANNOTATION_XML",
    [0x0011] = "LXB_TAG_APPLET",
    [0x0012] = "LXB_TAG_AREA",
    [0x0013] = "LXB_TAG_ARTICLE",
    [0x0014] = "LXB_TAG_ASIDE",
    [0x0015] = "LXB_TAG_AUDIO",
    [0x0016] = "LXB_TAG_B",
    [0x0017] = "LXB_TAG_BASE",
    [0x0018] = "LXB_TAG_BASEFONT",
    [0x0019] = "LXB_TAG_BDI",
    [0x001a] = "LXB_TAG_BDO",
    [0x001b] = "LXB_TAG_BGSOUND",
    [0x001c] = "LXB_TAG_BIG",
    [0x001d] = "LXB_TAG_BLINK",
    [0x001e] = "LXB_TAG_BLOCKQUOTE",
    [0x001f] = "LXB_TAG_BODY",
    [0x0020] = "LXB_TAG_BR",
    [0x0021] = "LXB_TAG_BUTTON",
    [0x0022] = "LXB_TAG_CANVAS",
    [0x0023] = "LXB_TAG_CAPTION",
    [0x0024] = "LXB_TAG_CENTER",
    [0x0025] = "LXB_TAG_CITE",
    [0x0026] = "LXB_TAG_CLIPPATH",
    [0x0027] = "LXB_TAG_CODE",
    [0x0028] = "LXB_TAG_COL",
    [0x0029] = "LXB_TAG_COLGROUP",
    [0x002a] = "LXB_TAG_DATA",
    [0x002b] = "LXB_TAG_DATALIST",
    [0x002c] = "LXB_TAG_DD",
    [0x002d] = "LXB_TAG_DEL",
    [0x002e] = "LXB_TAG_DESC",
    [0x002f] = "LXB_TAG_DETAILS",
    [0x0030] = "LXB_TAG_DFN",
    [0x0031] = "LXB_TAG_DIALOG",
    [0x0032] = "LXB_TAG_DIR",
    [0x0033] = "LXB_TAG_DIV",
    [0x0034] = "LXB_TAG_DL",
    [0x0035] = "LXB_TAG_DT",
    [0x0036] = "LXB_TAG_EM",
    [0x0037] = "LXB_TAG_EMBED",
    [0x0038] = "LXB_TAG_FEBLEND",
    [0x0039] = "LXB_TAG_FECOLORMATRIX",
    [0x003a] = "LXB_TAG_FECOMPONENTTRANSFER",
    [0x003b] = "LXB_TAG_FECOMPOSITE",
    [0x003c] = "LXB_TAG_FECONVOLVEMATRIX",
    [0x003d] = "LXB_TAG_FEDIFFUSELIGHTING",
    [0x003e] = "LXB_TAG_FEDISPLACEMENTMAP",
    [0x003f] = "LXB_TAG_FEDISTANTLIGHT",
    [0x0040] = "LXB_TAG_FEDROPSHADOW",
    [0x0041] = "LXB_TAG_FEFLOOD",
    [0x0042] = "LXB_TAG_FEFUNCA",
    [0x0043] = "LXB_TAG_FEFUNCB",
    [0x0044] = "LXB_TAG_FEFUNCG",
    [0x0045] = "LXB_TAG_FEFUNCR",
    [0x0046] = "LXB_TAG_FEGAUSSIANBLUR",
    [0x0047] = "LXB_TAG_FEIMAGE",
    [0x0048] = "LXB_TAG_FEMERGE",
    [0x0049] = "LXB_TAG_FEMERGENODE",
    [0x004a] = "LXB_TAG_FEMORPHOLOGY",
    [0x004b] = "LXB_TAG_FEOFFSET",
    [0x004c] = "LXB_TAG_FEPOINTLIGHT",
    [0x004d] = "LXB_TAG_FESPECULARLIGHTING",
    [0x004e] = "LXB_TAG_FESPOTLIGHT",
    [0x004f] = "LXB_TAG_FETILE",
    [0x0050] = "LXB_TAG_FETURBULENCE",
    [0x0051] = "LXB_TAG_FIELDSET",
    [0x0052] = "LXB_TAG_FIGCAPTION",
    [0x0053] = "LXB_TAG_FIGURE",
    [0x0054] = "LXB_TAG_FONT",
    [0x0055] = "LXB_TAG_FOOTER",
    [0x0056] = "LXB_TAG_FOREIGNOBJECT",
    [0x0057] = "LXB_TAG_FORM",
    [0x0058] = "LXB_TAG_FRAME",
    [0x0059] = "LXB_TAG_FRAMESET",
    [0x005a] = "LXB_TAG_GLYPHREF",
    [0x005b] = "LXB_TAG_H1",
    [0x005c] = "LXB_TAG_H2",
    [0x005d] = "LXB_TAG_H3",
    [0x005e] = "LXB_TAG_H4",
    [0x005f] = "LXB_TAG_H5",
    [0x0060] = "LXB_TAG_H6",
    [0x0061] = "LXB_TAG_HEAD",
    [0x0062] = "LXB_TAG_HEADER",
    [0x0063] = "LXB_TAG_HGROUP",
    [0x0064] = "LXB_TAG_HR",
    [0x0065] = "LXB_TAG_HTML",
    [0x0066] = "LXB_TAG_I",
    [0x0067] = "LXB_TAG_IFRAME",
    [0x0068] = "LXB_TAG_IMAGE",
    [0x0069] = "LXB_TAG_IMG",
    [0x006a] = "LXB_TAG_INPUT",
    [0x006b] = "LXB_TAG_INS",
    [0x006c] = "LXB_TAG_ISINDEX",
    [0x006d] = "LXB_TAG_KBD",
    [0x006e] = "LXB_TAG_KEYGEN",
    [0x006f] = "LXB_TAG_LABEL",
    [0x0070] = "LXB_TAG_LEGEND",
    [0x0071] = "LXB_TAG_LI",
    [0x0072] = "LXB_TAG_LINEARGRADIENT",
    [0x0073] = "LXB_TAG_LINK",
    [0x0074] = "LXB_TAG_LISTING",
    [0x0075] = "LXB_TAG_MAIN",
    [0x0076] = "LXB_TAG_MALIGNMARK",
    [0x0077] = "LXB_TAG_MAP",
    [0x0078] = "LXB_TAG_MARK",
    [0x0079] = "LXB_TAG_MARQUEE",
    [0x007a] = "LXB_TAG_MATH",
    [0x007b] = "LXB_TAG_MENU",
    [0x007c] = "LXB_TAG_META",
    [0x007d] = "LXB_TAG_METER",
    [0x007e] = "LXB_TAG_MFENCED",
    [0x007f] = "LXB_TAG_MGLYPH",
    [0x0080] = "LXB_TAG_MI",
    [0x0081] = "LXB_TAG_MN",
    [0x0082] = "LXB_TAG_MO",
    [0x0083] = "LXB_TAG_MS",
    [0x0084] = "LXB_TAG_MTEXT",
    [0x0085] = "LXB_TAG_MULTICOL",
    [0x0086] = "LXB_TAG_NAV",
    [0x0087] = "LXB_TAG_NEXTID",
    [0x0088] = "LXB_TAG_NOBR",
    [0x0089] = "LXB_TAG_NOEMBED",
    [0x008a] = "LXB_TAG_NOFRAMES",
    [0x008b] = "LXB_TAG_NOSCRIPT",
    [0x008c] = "LXB_TAG_OBJECT",
    [0x008d] = "LXB_TAG_OL",
    [0x008e] = "LXB_TAG_OPTGROUP",
    [0x008f] = "LXB_TAG_OPTION",
    [0x0090] = "LXB_TAG_OUTPUT",
    [0x0091] = "LXB_TAG_P",
    [0x0092] = "LXB_TAG_PARAM",
    [0x0093] = "LXB_TAG_PATH",
    [0x0094] = "LXB_TAG_PICTURE",
    [0x0095] = "LXB_TAG_PLAINTEXT",
    [0x0096] = "LXB_TAG_PRE",
    [0x0097] = "LXB_TAG_PROGRESS",
    [0x0098] = "LXB_TAG_Q",
    [0x0099] = "LXB_TAG_RADIALGRADIENT",
    [0x009a] = "LXB_TAG_RB",
    [0x009b] = "LXB_TAG_RP",
    [0x009c] = "LXB_TAG_RT",
    [0x009d] = "LXB_TAG_RTC",
    [0x009e] = "LXB_TAG_RUBY",
    [0x009f] = "LXB_TAG_S",
    [0x00a0] = "LXB_TAG_SAMP",
    [0x00a1] = "LXB_TAG_SCRIPT",
    [0x00a2] = "LXB_TAG_SECTION",
    [0x00a3] = "LXB_TAG_SELECT",
    [0x00a4] = "LXB_TAG_SLOT",
    [0x00a5] = "LXB_TAG_SMALL",
    [0x00a6] = "LXB_TAG_SOURCE",
    [0x00a7] = "LXB_TAG_SPACER",
    [0x00a8] = "LXB_TAG_SPAN",
    [0x00a9] = "LXB_TAG_STRIKE",
    [0x00aa] = "LXB_TAG_STRONG",
    [0x00ab] = "LXB_TAG_STYLE",
    [0x00ac] = "LXB_TAG_SUB",
    [0x00ad] = "LXB_TAG_SUMMARY",
    [0x00ae] = "LXB_TAG_SUP",
    [0x00af] = "LXB_TAG_SVG",
    [0x00b0] = "LXB_TAG_TABLE",
    [0x00b1] = "LXB_TAG_TBODY",
    [0x00b2] = "LXB_TAG_TD",
    [0x00b3] = "LXB_TAG_TEMPLATE",
    [0x00b4] = "LXB_TAG_TEXTAREA",
    [0x00b5] = "LXB_TAG_TEXTPATH",
    [0x00b6] = "LXB_TAG_TFOOT",
    [0x00b7] = "LXB_TAG_TH",
    [0x00b8] = "LXB_TAG_THEAD",
    [0x00b9] = "LXB_TAG_TIME",
    [0x00ba] = "LXB_TAG_TITLE",
    [0x00bb] = "LXB_TAG_TR",
    [0x00bc] = "LXB_TAG_TRACK",
    [0x00bd] = "LXB_TAG_TT",
    [0x00be] = "LXB_TAG_U",
    [0x00bf] = "LXB_TAG_UL",
    [0x00c0] = "LXB_TAG_VAR",
    [0x00c1] = "LXB_TAG_VIDEO",
    [0x00c2] = "LXB_TAG_WBR",
    [0x00c3] = "LXB_TAG_XMP",
    [0x00c4] = "LXB_TAG__LAST_ENTRY" 
};

const char* _dbg_get_node_type_name_(size_t node_type) {
    if (node_type >= LXB_DOM_NODE_TYPE_LAST_ENTRY) return "ERROR: bad node type";
    return _dbg_node_types_[node_type];
}

const char* _dbg_get_tag_name_(size_t local_name) {
    if (local_name >= LXB_TAG__LAST_ENTRY) return "ERROR: bad tag";
    return _dbg_tags_[local_name];
}



/*
 * This implementation was a quick solution to just render the text parts without caring 
 * too much about the DOM specification, and we started using a context to be able to pass
 * some information to the recursive calls. It should be progressively replaced by functions
 * that instead of calling the same draw_rec_tag, resolve only the element they are made to
 * draw.
 * 
 */
Err draw_rec_tag(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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

Err draw_text(lxb_dom_node_t* node,  DrawCtx ctx[static 1]) {
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

Err draw_rec(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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

Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    draw_ctx_pre_set(ctx, true);
    try( draw_list_block(node->first_child, node->last_child, ctx));
    draw_ctx_pre_set(ctx, false);
    return Ok;
}

static Err _htmldoc_draw_with_flags_(HtmlDoc htmldoc[static 1], Session s[static 1], unsigned flags) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    DrawCtx ctx;
    Err err = draw_ctx_init(&ctx, htmldoc, s, flags);
    try(err);
    ok_then(err, draw_rec(lxb_dom_interface_node(lxbdoc), &ctx));
    ok_then(err, draw_ctx_buf_commit(&ctx));
    ok_then(err, textbuf_fit_lines(htmldoc_textbuf(htmldoc), *session_conf_ncols(session_conf(s))));

    draw_ctx_cleanup(&ctx);
    if (err) arlfn(ModAt,clean) (draw_ctx_mods(&ctx));
    //TODO: if a redraw was solicited, we may instead cleanup and re init.
    return err;
}


static Err _htmldoc_draw_(HtmlDoc htmldoc[static 1], Session s[static 1]) {
    unsigned flags = draw_ctx_flags_from_session(s) | DRAW_CTX_FLAG_TITLE;
    return _htmldoc_draw_with_flags_(htmldoc, s, flags);
}


#define HTMLDOC_FLAG_JS 0x1u

Err _dup_url_from_request_(Request r[static 1], Url* u, Url out[static 1]) {
    Err e = Ok;
    if (u && len__(request_url_str(r))) {
        try(url_dup(u, out));
        try_or_jump(e, Failure_Url_Cleanup,
            curlu_set_url_or_fragment(url_cu(out), items__(request_url_str(r))));
    } else if (len__(request_url_str(r))) {
        try(url_init_from_cstr(out, items__(request_url_str(r))));
    } else if (u) {
        try(url_dup(u, out));
    } else
        return "error: cannot initialize htmldoc with nonurl nor urlstr";

    return Ok;

Failure_Url_Cleanup:
    url_cleanup(out);
    return e;
}


Err htmldoc_init_from_request(
    HtmlDoc   d[static 1],
    Request   r[static 1],
    Url       u[static 1],
    UrlClient uc[static 1],
    Session*  s
) {
    if (!s) return "error: expecting a session, recived NULL";
    Err e = Ok;
    Writer w;
    try(session_msg_writer_init(&w, s));
    unsigned flags = session_js(s) ? HTMLDOC_FLAG_JS : 0x0;

    /* lexbor doc should be initialized before jse_init */
    *d = (HtmlDoc){
        .method = request_method(r),
        .lxbdoc = lxb_html_document_create()
    };
    if (!d->lxbdoc) return "error: lxb failed to create html document";
    if (flags & HTMLDOC_FLAG_JS) 
        try_or_jump(e, Failure, jse_init(d));
    try_or_jump(e, Failure, _dup_url_from_request_(r, u, htmldoc_url(d)));
    try_or_jump(e, Failure, url_from_request(r, uc, htmldoc_url(d)));

    ArlOf(FetchHistoryEntry)* hist = session_fetch_history(s);
     if (!arlfn(FetchHistoryEntry,append)(hist, &(FetchHistoryEntry){0}))
     { e = "error: arl append to fetch history failure"; goto Failure; }

    try_or_jump(e, Failure, htmldoc_fetch(d, uc, &w, arlfn(FetchHistoryEntry,back)(hist)));

    htmldoc_eval_js_scripts_or_continue(d, s);
    try_or_jump( e, Failure, _htmldoc_draw_(d, s));
    try_or_jump( e, Failure,
        fetch_history_entry_update_title(arlfn(FetchHistoryEntry,back)(hist),htmldoc_title(d)));


    return Ok;

Failure:
    htmldoc_cleanup(d);

    return e;
}


Err htmldoc_init_bookmark(HtmlDoc d[static 1], const char* urlstr) {
    Err e = Ok;

    *d = (HtmlDoc){ .lxbdoc = lxb_html_document_create() };
    if (!d->lxbdoc) return "error: lxb failed to create html document";
    Url dup;
    if (!urlstr) return "error: cannot initialize bookmark with not path";
    try_or_jump(e, Failure_Lxb_Html_Document_Destroy, url_init_from_cstr(&dup, urlstr));

    d->url    = dup;
    d->method = http_get;

    return Ok;

Failure_Lxb_Html_Document_Destroy:
    lxb_html_document_destroy(d->lxbdoc);
    return e;
}


/* external linkage */


void htmldoc_reset_draw(HtmlDoc htmldoc[static 1]) {
    textbuf_reset(htmldoc_textbuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_forms(htmldoc));
}


void htmldoc_fetchcache_cleanup(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_sourcebuf(htmldoc));
    arlfn(Str,clean)(htmldoc_head_scripts(htmldoc));
    arlfn(Str,clean)(htmldoc_body_scripts(htmldoc));
    htmldoc->fetch_cache = (DocFetchCache){0};
}

void htmldoc_drawcache_cleanup(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_forms(htmldoc));

    str_clean(htmldoc_screen(htmldoc));
    htmldoc->draw_cache = (DocDrawCache){0};
}

void htmldoc_cache_cleanup(HtmlDoc htmldoc[static 1]) {
    htmldoc_drawcache_cleanup(htmldoc);
    htmldoc_fetchcache_cleanup(htmldoc);
}



void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) {
    http_header_clean(htmldoc_http_header(htmldoc));
    htmldoc_cache_cleanup(htmldoc);
    lxb_html_document_destroy(htmldoc_lxbdoc(htmldoc));
    url_cleanup(htmldoc_url(htmldoc));

    if (jse_rt(htmldoc_js(htmldoc))) jse_clean(htmldoc_js(htmldoc));
}


inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}



Err htmldoc_redraw(HtmlDoc htmldoc[static 1], Session s[static 1]) {
    htmldoc_reset_draw(htmldoc);
    unsigned flags = draw_ctx_flags_from_session(s);
    return _htmldoc_draw_with_flags_(htmldoc, s, flags);
}

Err htmldoc_A(Session* s, HtmlDoc d[static 1]) {
    if (!s) return "error: no session";
    Str* buf = &(Str){0};
    str_append_lit__(buf, "<li><a href=\"");
    char* url_buf;
    Err err = url_cstr_malloc(htmldoc_url(d), &url_buf);
    if (err) {
        str_clean(buf);
        return err;
    }
    str_append(buf, url_buf, strlen(url_buf));
    curl_free(url_buf);
    str_append_lit__(buf, "\">");
    try( lexbor_get_title_text_line(*htmldoc_title(d), buf));
    str_append_lit__(buf, "</a>");
    str_append_lit__(buf, "\n");
    try( session_write_msg(s, items__(buf), len__(buf)));
    str_clean(buf);
    return Ok;
}

Err htmldoc_print_info(Session* s, HtmlDoc d[static 1]) {
    Err err = Ok;
    LxbNodePtr* title = htmldoc_title(d);
    try (session_write_msg_lit__(s, "DOWNLOAD SIZE: "));
    try (session_write_unsigned_msg(s, *htmldoc_curlinfo_sz_download(d)));
    try (session_write_msg_lit__(s, "\n"));
    try (session_write_msg_lit__(s, "html size: "));
    try (session_write_unsigned_msg(s, htmldoc_sourcebuf(d)->buf.len));
    try (session_write_msg_lit__(s, "\n"));

    if (title) {
        Str buf = (Str){0};
        try (lexbor_get_title_text(*title, &buf));
        if (!buf.len) err = session_write_msg_lit__(s,  "<NO TITLE>");
        else err = session_write_msg(s, buf.items, buf.len);
        str_clean(&buf);
        try(err);
        ok_then(err, session_write_msg_lit__(s, "\n"));
    } else try(session_write_msg_lit__(s, "\n"));
    
    char* url = NULL;
    ok_then(err, url_cstr_malloc(htmldoc_url(d), &url));
    if (url) {
        ok_then(err, session_write_msg(s, url, strlen(url)));
        curl_free(url);
        ok_then(err, session_write_msg_lit__(s, "\n"));
    }
    Str* charset = htmldoc_http_charset(d);
    if (len__(charset)) {
        ok_then(err, session_write_msg(s, items__(charset), len__(charset)));
        ok_then(err, session_write_msg_lit__(s, "\n"));
    }
    Str* content_type = htmldoc_http_content_type(d);
    if (len__(content_type)) {
        ok_then(err, session_write_msg(s, items__(content_type), len__(content_type)));
        ok_then(err, session_write_msg_lit__(s, "\n"));
    }

    return err;
}

static bool _node_has_href(lxb_dom_node_t* node) {
    const lxb_char_t* data;
    size_t data_len;
    return lexbor_find_lit_attr_value__(node, "href", &data, &data_len);
}

static bool _prev_is_separable_(lxb_dom_node_t n[static 1]) {
    return n->prev  && 
        n->prev->local_name != LXB_TAG_LI
        ;
}

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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

Err htmldoc_reparse_source(HtmlDoc d[static 1]) {
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
Err htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[static 1]) {
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

Err htmldoc_console(HtmlDoc d[static 1], Session* s, const char* line) {
    if (!s) return "error: no session";
    return jse_eval(htmldoc_js(d), s, line);
}

static Err jse_eval_doc_scripts(Session* s, HtmlDoc d[static 1]) {

    for ( Str* it = arlfn(Str,begin)(htmldoc_head_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_head_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, items__(it));
        if (e) session_write_msg(s, (char*)e, strlen(e));
    }

    for ( Str* it = arlfn(Str,begin)(htmldoc_body_scripts(d))
        ; it != arlfn(Str,end)(htmldoc_body_scripts(d))
        ; ++it) {
        Err e = jse_eval(htmldoc_js(d), s, items__(it));
        if (e) session_write_msg(s, (char*)e, strlen(e));
    }

    return Ok;
}

//TODO: make this fn not Err and rename it
Err htmldoc_js_enable(HtmlDoc d[static 1], Session* s) {
    try( jse_init(d));
    Err e = jse_eval_doc_scripts(s, d);
    if (e) session_write_msg(s, (char*)e, strlen(e));
    return Ok;
}

void htmldoc_eval_js_scripts_or_continue(HtmlDoc d[static 1], Session* s) {
    if (htmldoc_js_is_enabled(d)) {
        Err e = jse_eval_doc_scripts(s, d);
        if (e) session_write_msg(s, (char*)e, strlen(e));
    }
}

Err _htmldoc_scripts_range_from_parsed_range_(
    HtmlDoc          h[static 1],
    RangeParse p[static 1],
    Range            r[static 1]
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
        case range_addr_beg_tag: r->beg = 0 + p->beg.delta;
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
        case range_addr_beg_tag: r->end = 0 + p->end.delta;
            break;
        case range_addr_none_tag: r->end = r->beg;
            break;
        case range_addr_end_tag: r->end = script_max + p->end.delta;
            break;
        case range_addr_num_tag: r->end = p->end.n + p->end.delta;
            break;
        default: return "error: invalid RangeParse end tag";
    }
    return Ok;
}


Err htmldoc_scripts_write(HtmlDoc h[static 1], RangeParse rp[static 1], Writer w[static 1]) {
    if (!htmldoc_js_is_enabled(h)) return "enable js to get the scripts";

    Range r;
    try(_htmldoc_scripts_range_from_parsed_range_(h, rp, &r));
    for (size_t it = r.beg; it <= r.end; ++it) {
        char buf[UINT_TO_STR_BUFSZ] = {0};
        size_t len;
        try( unsigned_to_str(it, buf, UINT_TO_STR_BUFSZ, &len));
        Str* sc;
        try( htmldoc_script_at(h, it, &sc));
        try(writer_write_lit__(w, "// script: "));
        try(writer_write(w, buf, len));
        try(writer_write_lit__(w, "\n"));
        try(writer_write(w, items__(sc), len__(sc) - 1));
        try(writer_write_lit__(w, "\n"));
    }
    return Ok;
}
