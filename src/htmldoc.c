#include "constants.h"
#include "debug.h"
#include "draw.h"
#include "error.h"
#include "generic.h"
#include "htmldoc.h"
#include "mem.h"
#include "textbuf.h"
#include "utils.h"
#include "wrapper-lexbor.h"
#include "ahre__.h"
#include "session.h"

Err draw_tag_a(lxb_dom_node_t* node, DrawCtx ctx[static 1]);
Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[static 1]);

/* internal linkage */
#define MAX_URL_LEN 2048u
#define READ_FROM_FILE_BUFFER_LEN 4096u
#define LAZY_STR_BUF_LEN 1600u

#define append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(LxbNodePtr,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")


Err _strview_trim_left_count_newlines_(
    StrView s[static 1], TextBufMods mods[static 1], size_t* out
) {
    size_t newlines = 0;
    while(s->len && isspace(*(items__(s)))) {
        newlines += *(items__(s)) == '\n';
        ++s->items;
        --s->len;
    }
    foreach__(ModsAt, it, mods) {
        if (it->offset < newlines) return "error: bad offset :(";
        it->offset -= newlines;
    }
    if (out) *out = newlines;
    return Ok;
}

size_t _strview_trim_right_count_newlines_(StrView s[static 1]) {
    size_t newlines = 0;
    while(s->len && isspace(items__(s)[len__(s)-1])) {
        newlines += items__(s)[s->len-1] == '\n';
        --s->len;
    }
    return newlines;
}

Err _hypertext_id_open_(
    DrawCtx ctx[static 1],
    ImpureDrawProcedure visual_effect,
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

Err _hypertext_open_(
    DrawCtx ctx[static 1], ImpureDrawProcedure visual_effect, StrViewProvider prefix_str_provider
) {
    if (visual_effect) try( visual_effect(ctx));
    if (prefix_str_provider) try( draw_ctx_buf_append(ctx, prefix_str_provider()));
    return Ok;
}


Err _hypertext_id_close_(
    DrawCtx ctx[static 1],
    ImpureDrawProcedure visual_effect,
    StrViewProvider close_str_provider
) {
    if (close_str_provider) try( draw_ctx_buf_append(ctx, close_str_provider()));
    if (visual_effect) try( visual_effect(ctx));
    return Ok;
}

Err _hypertext_close_(
    DrawCtx ctx[static 1],
    ImpureDrawProcedure visual_effect,
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

    try( draw_ctx_buf_append_lit__(ctx, "%{[center]:\n"));
    try( draw_list(node->first_child, node->last_child, ctx));
    try( draw_ctx_buf_append_lit__(ctx, "%}[center]\n"));
    return Ok;
}

static Err
brose_ctx_append_img_alt_(lxb_dom_node_t* img, DrawCtx ctx[static 1]) {

    const lxb_char_t* alt;
    size_t alt_len;
    lexbor_find_attr_value(img, "alt", &alt, &alt_len);

    if (alt && alt_len) {
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

    try( brose_ctx_append_img_alt_(node, ctx));
    try (draw_list(node->first_child, node->last_child, ctx));
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, NULL));
    return Ok;
}

static Err
draw_tag_form(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    ArlOf(LxbNodePtr)* forms = htmldoc_forms(draw_ctx_htmldoc(ctx));
    if (!arlfn(LxbNodePtr,append)(forms, &node)) return "error: lip set";
    size_t form_id = len__(forms)-1;
    try( _hypertext_id_open_(ctx, draw_ctx_color_purple, form_open_str, &form_id, form_sep_str));

    try( draw_ctx_reset_color(ctx));

    try (draw_list_block(node->first_child, node->last_child, ctx));

    try( draw_ctx_buf_append_color_(ctx, esc_code_purple));
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, form_close_str));
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
        ctx, draw_ctx_color_red, button_open_str, &input_id, elem_id_sep_default));
    

    try (draw_list(node->first_child, node->last_child, ctx));

    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, button_close_str));
    return Ok;
}


static Err draw_tag_input(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    const lxb_char_t* s;
    size_t slen;

    lexbor_find_attr_value(node, "type", &s, &slen);
    if (slen && !strcmp("hidden", (char*)s)) 
        return Ok;

    HtmlDoc* d = draw_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
    if (!arlfn(LxbNodePtr,append)(inputs, &node)) { return "error: arl set"; }
    size_t input_id = len__(inputs)-1;


    /* submit */
    if (_input_is_submit_type_(s, slen)) {

        lexbor_find_attr_value(node, "value", &s, &slen);
        if (slen)  {
            try( _hypertext_id_open_(
                ctx, draw_ctx_color_red, input_text_open_str, &input_id, input_submit_sep_str));

            try( draw_ctx_buf_append(ctx, strview__((char*)s, slen)));
        } else try( _hypertext_id_open_(
                ctx, draw_ctx_color_red, input_text_open_str, &input_id, NULL));
        try( _hypertext_id_close_(ctx, draw_ctx_reset_color, input_submit_close_str));
        return Ok;

    } 

    /* other */
    try( _hypertext_id_open_(
        ctx, draw_ctx_color_red, input_text_open_str, &input_id, NULL));

    if (_input_is_text_type_(s, slen)) {
        lexbor_find_attr_value(node, "value", &s, &slen);
        if (slen) {
            try( draw_ctx_buf_append_lit__(ctx, "="));
            try( draw_ctx_buf_append(ctx, strview__((char*)s, slen)));
        }
    } else if (lexbor_str_eq("password", s, slen)) {
        lexbor_find_attr_value(node, "value", &s, &slen);
        if (slen) try( draw_ctx_buf_append_lit__(ctx, "=********"));
    } else {
        try( draw_ctx_buf_append_lit__(ctx, "[input not supported yet]"));
    }
    try( _hypertext_id_close_(ctx, draw_ctx_reset_color, input_text_close_str));
    return Ok;
}


static Err
draw_tag_div(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    BufOf(char) buf = (BufOf(char)){0};
    TextBufMods mods = (TextBufMods){0};
    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    Err err = draw_list(node->first_child, node->last_child, ctx);

    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    if (!err) {
        //TODO^: trim and rebase offset
        if (buf.len) {
            if (len__(draw_ctx_buf(ctx))) ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
            ok_then(err, draw_ctx_buf_append_mem_mods(ctx, (char*)buf.items, buf.len, &mods));
        }
        
        //StrView view = strview_from_mem_trim(buf.items, buf.len);
        //if (view.len) {
        //    if (len__(draw_ctx_buf(ctx))) ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        //    ok_then(err, draw_ctx_buf_append_mem(ctx, (char*)view.items, view.len));
        //    ///ok_then(err, draw_ctx_buf_append_lit__(ctx, "\n"));
        //}
    }
    buffn(char, clean)(&buf);
    arlfn(ModsAt, clean)(&mods);
    return Ok;
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
    BufOf(char) buf = (BufOf(char)){0};
    TextBufMods mods = (TextBufMods){0};
    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    Err err = draw_list(node->first_child, node->last_child, ctx);

    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    if (!err && buf.len) {
        //TODO^: trim&rebase offset
        //StrView s = strview_from_mem_trim(buf.items, buf.len);
        if (buf.len) {
            ///?if (buf.items < s.items) err = draw_ctx_buf_append_lit__(ctx, "\n");
            ok_then( err, draw_ctx_buf_append_lit__(ctx, " * "));
            ok_then( err, draw_ctx_buf_append_mem_mods(ctx, (char*)buf.items, buf.len, &mods));
            if (buf.items[buf.len-1] != '\n') ok_then( err, draw_ctx_buf_append_lit__(ctx, "\n"));
        }
    }

    buffn(char, clean)(&buf);
    arlfn(ModsAt, clean)(&mods);
    return Ok;
}


static Err draw_tag_h(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    BufOf(char) buf = (BufOf(char)){0};
    TextBufMods mods = (TextBufMods){0};
    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    try( draw_list_block(node->first_child, node->last_child, ctx));

    draw_ctx_swap_buf_mods(ctx, &buf, &mods);

    StrView content = strview_from_mem(buf.items, buf.len);
    try(_strview_trim_left_count_newlines_(&content, &mods, NULL));
    _strview_trim_right_count_newlines_(&content);

    Err err;
    if ((err=_hypertext_open_(ctx, draw_ctx_push_bold, h_tag_open_str))) {
        buffn(char, clean)(&buf);
        return err;
    }

    if (content.len) try( draw_ctx_buf_append_mem_mods(ctx, (char*)content.items, content.len, &mods));
    buffn(char, clean)(&buf);
    arlfn(ModsAt, clean)(&mods);
    try( _hypertext_close_(ctx, draw_ctx_reset_color, newline_str));

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
    Str title = (Str){0};
    try( lexbor_get_title_text_line(node, &title));
    if (!title.len)  {
        try( log_msg__(draw_ctx_logfn(ctx), "%s\n", "<%> no title <%>"));
        return Ok;
    }
    if (!buffn(char,append)(&title, "\n\0", 2)) return "error: bufn append failure";
    Err err = log_msg__(draw_ctx_logfn(ctx), "%s", title.items);
    buffn(char,clean)(&title); 
    return err;
}

Err draw_mem_skipping_space(
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


Err draw_rec_tag(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
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
            log_todo__( draw_ctx_logfn(ctx), "%s", "script tag not implemented (skipping)");
            return Ok;
        } 
        case LXB_TAG_STYLE: {
            log_todo__( draw_ctx_logfn(ctx), "%s\n", "skiping style (not implemented)");
            return Ok;
        } 
        case LXB_TAG_TITLE: { return draw_tag_title(node, ctx); } 
        case LXB_TAG_TR: { return draw_tag_tr(node, ctx); }
        case LXB_TAG_TT: { return draw_tag_code(node, ctx); }
        case LXB_TAG_UL: { return draw_tag_ul(node, ctx); }
        default: {
            if (node->local_name >= LXB_TAG__LAST_ENTRY)
                log_warn__(
                    draw_ctx_logfn(ctx),
                    "node local name (TAG) greater than last entry: %lx\n",
                    node->local_name
                );
            else log_todo__(
                draw_ctx_logfn(ctx),
                "TAG 'NOT' IMPLEMENTED: %s",
                _dbg_tags_[node->local_name]
            );
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
        log_warn__(
            draw_ctx_logfn(ctx),
            "%s\n",
            "LXB_DOM_NODE_TYPE_TEXT with actual childs"
        );
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
                    log_warn__(
                        draw_ctx_logfn(ctx),
                        "lexbor node type greater than last entry: %lx\n",
                        node->type
                    );
                else log_warn__(
                    draw_ctx_logfn(ctx),
                    "Ignored Node Type: %s\n",
                    _dbg_node_types_[node->type]
                );
                return Ok;
            }
        }
    }
    return Ok;
}

//TODO: is this fn needed?
Err draw_tag_pre(lxb_dom_node_t* node, DrawCtx ctx[static 1]) {
    draw_ctx_pre_set(ctx, true);
    try( draw_list_block(node->first_child, node->last_child, ctx));
    draw_ctx_pre_set(ctx, false);
    return Ok;
}

/* external linkage */

//TODO: deprecate, always init with fetch ? (and if bad url?)
Err htmldoc_init(HtmlDoc d[static 1], const char* cstr_url) {
    Url url = {0};
    if (cstr_url && *cstr_url) {
        if (strlen(cstr_url) > MAX_URL_LEN) return "cstr_url large is not supported.";
        try( url_init(&url, cstr_url));
    } else { cstr_url = 0x0; }

    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        if (cstr_url) {
            url_cleanup(htmldoc_url(d));
        }

        return "error: lxb failed to create html document";
    }

    *d = (HtmlDoc){
        .url=url,
        .method=http_get,
        .lxbdoc=document
        //.cache=(DocCache){ .textbuf=(TextBuf){0}, .sourcebuf=(TextBuf){0} }
    };
    return Ok;
}
Err htmldoc_init_from_curlu(HtmlDoc d[static 1], CURLU* cu, HttpMethod method) {
    Url url = {0};
    try( url_init_from_curlu(&url, cu));
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        curl_url_cleanup(cu);
        return "error: lxb failed to create html document";
    }

    *d = (HtmlDoc){
        .url=url,
        .method=method,
        .lxbdoc=document
        //.cache=(DocCache){ .textbuf=(TextBuf){0}, .sourcebuf=(TextBuf){0} };
    };
    return Ok;
}

Err htmldoc_init_fetch_draw(
    HtmlDoc d[static 1],
    const char* url,
    UrlClient url_client[static 1],
    Session s[static 1]
) {
    try(htmldoc_init(d, url));
    Err err = htmldoc_fetch(d, url_client, session_doc_msg_fn(s, d));
    ok_then(err, htmldoc_draw(d, s));
    if (err) {
        htmldoc_cleanup(d);
        return err;
    }
    return Ok;
}

Err htmldoc_init_fetch_draw_from_curlu(
    HtmlDoc d[static 1],
    CURLU* cu,
    UrlClient url_client[static 1],
    HttpMethod method,
    Session s[static 1]
) {
    try(htmldoc_init_from_curlu(d, cu, method));
    Err err = htmldoc_fetch(d, url_client, session_doc_msg_fn(s,d)); 
    ok_then(err, htmldoc_draw(d, s));
    if (err) {
        d->url = (Url){0}; /* on failure do not free cu owned by caller */
        htmldoc_cleanup(d);
        return err;
    }
    return Ok;
}

HtmlDoc* htmldoc_create(const char* url) {
    HtmlDoc* rv = std_malloc(sizeof(HtmlDoc));
    if (!rv) return NULL;
    if (htmldoc_init(rv, url)) {
        std_free(rv);
        return NULL;
    }
    return rv;
}

//TODO: rename this fn
void htmldoc_reset(HtmlDoc htmldoc[static 1]) {
    textbuf_reset(htmldoc_textbuf(htmldoc));
    textbuf_reset(htmldoc_sourcebuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
}

void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(htmldoc);
    lxb_html_document_destroy(htmldoc_lxbdoc(htmldoc));
    url_cleanup(htmldoc_url(htmldoc));
}

inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}

Err htmldoc_draw(HtmlDoc htmldoc[static 1], Session s[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    DrawCtx ctx;
    Err err = draw_ctx_init(&ctx, htmldoc, s);
    ok_then(err, draw_rec(lxb_dom_interface_node(lxbdoc), &ctx));
    ok_then(err, draw_ctx_buf_commit(&ctx));
    ok_then(err, textbuf_fit_lines(htmldoc_textbuf(htmldoc), *session_conf_ncols(session_conf(s))));

    draw_ctx_cleanup(&ctx);
    return err;
}


Err htmldoc_A(Session* s, HtmlDoc d[static 1]) {
    if (!s) return "error: no session";
    BufOf(char)* buf = &(BufOf(char)){0};
    buffn(char,append)(buf, "<li><a href=\"", sizeof( "<li><a href=\"")-1);
    char* url_buf;
    Err err = url_cstr(htmldoc_url(d), &url_buf);
    if (err) {
        buffn(char,clean)(buf);
        return err;
    }
    buffn(char,append)(buf, url_buf, strlen(url_buf));
    curl_free(url_buf);
    buffn(char,append)(buf, "\">", 2);
    try( lexbor_get_title_text_line(*htmldoc_title(d), buf));
    buffn(char,append)(buf, "</a>", 4);
    try( session_write_msg(s, items__(buf), len__(buf)));
    buffn(char,clean)(buf);
    return Ok;
}

Err htmldoc_print_info(Session* s, HtmlDoc d[static 1]) {
    LxbNodePtr* title = htmldoc_title(d);
    if (!title) return "no title in doc";
    Str buf = (Str){0};
    try (lexbor_get_title_text(*title, &buf));
    Err err = session_write_msg(s, buf.items, buf.len);
    str_clean(&buf);
    ok_then(err, session_write_msg_lit__(s, "\n"));
    
    char* url = NULL;
    ok_then(err, url_cstr(htmldoc_url(d), &url));
    if (url) {
        ok_then(err, session_write_msg(s, url, strlen(url)));
        curl_free(url);
        ok_then(err, session_write_msg_lit__(s, "\n"));
    }
    return err;
}

