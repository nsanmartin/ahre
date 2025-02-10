#include "src/constants.h"
#include "src/debug.h"
#include "src/error.h"
#include "src/generic.h"
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/textbuf.h"
#include "src/utils.h"
#include "src/wrapper-lexbor.h"

Err browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]);
Err browse_tag_pre(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]);

/* internal linkage */
//static constexpr size_t MAX_URL_LEN = 2048;
#define MAX_URL_LEN 2048
//static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
#define READ_FROM_FILE_BUFFER_LEN 4096
#define LAZY_STR_BUF_LEN 1600

#define serialize_cstring(Ptr, Len, CallBack, Context) \
    ((LXB_STATUS_OK != CallBack((lxb_char_t*)Ptr, Len, Context)) \
    ?  "error serializing data" : Ok)

#define try_lxb_serialize(Ptr, Len, CallBack, Context) \
     if (LXB_STATUS_OK != CallBack((lxb_char_t*)Ptr, Len, Context)) \
        return "error serializing data"

#define serialize_cstring_debug(LitStr, CallBack, Context) \
    serialize_cstring(LitStr, sizeof(LitStr) - 1, CallBack, Context)

#define append_to_arlof_lxb_node__(ArrayList, NodePtr) \
    (arlfn(LxbNodePtr,append)(ArrayList, (NodePtr)) ? Ok : "error: lip set")

#define append_to_bufof_char_lit_(Buffer, LitStr) \
    (buffn(char, append)(Buffer,LitStr, sizeof(LitStr) - 1) ? Ok : "error: failed to append to bufof")

//_Thread_local static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};


static lxb_status_t
serialize_cb_browse(const lxb_char_t* data, size_t len, void* ctx) {
    TextBuf* textbuf = htmldoc_textbuf(browse_ctx_htmldoc(ctx));
    return textbuf_append_part(textbuf, (char*)data, len)
        ? LXB_STATUS_ERROR
        : LXB_STATUS_OK;
}

Err _serialize_lazy_str_(
    BrowseCtx ctx[static 1], lxb_html_serialize_cb_f cb, uintmax_t ui
)
{
    lxb_status_t status = serialize_unsigned(cb, ui, ctx, LXB_STATUS_ERROR);
    *browse_ctx_dirty(ctx) = true;
    return status == LXB_STATUS_OK 
        ? Ok
        : "error: could not serialize unsigned";
}

static Err _serialize_color_(lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1], EscCode code) {
    if (browse_ctx_color(ctx)) {
        try( browse_ctx_exc_code_push(ctx, code));
        Str code_str;
        try( esc_code_to_str(code, &code_str));
        if (LXB_STATUS_OK != cb((lxb_char_t*)code_str.s, code_str.len, ctx))
            return "error serializing literal string";
    }
    return Ok;
}

static Err _serialize_color_reset_(lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (browse_ctx_color(ctx)) {
        if (LXB_STATUS_OK != cb((lxb_char_t*)EscCodeReset, sizeof(EscCodeReset)-1, ctx))
            return "error serializing literal string";
        ArlOf(EscCode)* stack = browse_ctx_esc_code_stack(ctx);
        try( browse_ctx_esc_code_pop(ctx));
        EscCode* backp =  arlfn(EscCode, back)(stack);
        if (backp) {
            Str code_str;
            try( esc_code_to_str(*backp, &code_str));
            if (LXB_STATUS_OK != cb((lxb_char_t*)code_str.s, code_str.len, ctx))
                return "error serializing literal string";
        }
    }
    return Ok;
}

static Err
browse_tag_br(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( serialize_lit_str("\n", cb, ctx));
    return browse_list(node->first_child, node->last_child, cb, ctx);
}

static Err
browse_tag_center(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    //TODO: store center boundaries (start = len on enter, end = len on ret) and then
    // when fitting to width center those lines image.

    try( serialize_lit_str("\n", cb, ctx));
    try( serialize_cstring_debug("%(center:\n\n", cb, ctx));
    try( browse_list(node->first_child, node->last_child, cb, ctx));
    try( serialize_cstring_debug("%%center)\n", cb, ctx));
    try( serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
_serialize_img_alt(lxb_dom_node_t* img, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {

    const lxb_char_t* alt;
    size_t alt_len;
    lexbor_find_attr_value(img, "alt", &alt, &alt_len);

    if (alt && alt_len) {
        try_lxb_serialize(ELEM_ID_SEP, sizeof(ELEM_ID_SEP)-1, cb, ctx);
        if (cb(alt, alt_len, ctx)) return "error serializing img's alf";
    }
    return Ok;
}

static Err
browse_tag_img(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( browse_ctx_lazy_str_serialize(ctx, cb));
    try( _serialize_color_(cb, ctx, esc_code_light_green));
    HtmlDoc* d = browse_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* imgs = htmldoc_imgs(d);
    const size_t img_count = len__(imgs);
    try( append_to_arlof_lxb_node__(imgs, &node));
    try_lxb_serialize(IMAGE_OPEN_STR, sizeof(IMAGE_OPEN_STR)-1, cb, ctx);
    try( _serialize_lazy_str_(ctx, cb, img_count));
    try( _serialize_img_alt(node, cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try_lxb_serialize(IMAGE_CLOSE_STR, sizeof(IMAGE_CLOSE_STR)-1, cb, ctx);
    try( _serialize_color_reset_(cb, ctx));
    return Ok;
}

static Err
browse_tag_form(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_cstring_debug("\n", cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_cstring_debug("\n", cb, ctx));
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
browse_tag_button(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( browse_ctx_lazy_str_serialize(ctx, cb));
    try( _serialize_color_(cb, ctx, esc_code_red));
    try (serialize_lit_str(BUTTON_OPEN_STR, cb, ctx));

    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str(BUTTON_CLOSE_STR, cb, ctx));
    try (serialize_lit_str(" % button not supported yet % ", cb, ctx));
    try( _serialize_color_reset_(cb, ctx));
    return Ok;
}

static Err
browse_tag_input(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    const lxb_char_t* s;
    size_t slen;

    lexbor_find_attr_value(node, "type", &s, &slen);
    if (slen && !strcmp("hidden", (char*)s)) {
    } else {
        HtmlDoc* d = browse_ctx_htmldoc(ctx);
        ArlOf(LxbNodePtr)* inputs = htmldoc_inputs(d);
        if (!arlfn(LxbNodePtr,append)(inputs, &node)) {
            return "error: lip set";
        }
        try( browse_ctx_lazy_str_serialize(ctx, cb));
        try( _serialize_color_(cb, ctx, esc_code_red));
        try (serialize_lit_str(INPUT_OPEN_STR, cb, ctx));
        try (_serialize_lazy_str_(ctx, cb, inputs->len-1));
        if (_input_is_text_type_(s, slen)) {
            lexbor_find_attr_value(node, "value", &s, &slen);
            if (slen) {
                try (serialize_lit_str(" ", cb, ctx));
                try (serialize_cstring(s, slen, cb, ctx));
            }
        } else if (_input_is_submit_type_(s, slen)) {

            lexbor_find_attr_value(node, "value", &s, &slen);
            if (slen) {
                try (serialize_lit_str(ELEM_ID_SEP, cb, ctx));
                try (serialize_cstring(s, slen, cb, ctx));
            }
        } else {
            try (serialize_lit_str("[input not supported yet]", cb, ctx));
        }
        try (serialize_lit_str(INPUT_CLOSE_STR, cb, ctx));
        try( _serialize_color_reset_(cb, ctx));
    }
    return Ok;
}


static Err
browse_tag_div(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( browse_ctx_lazy_str_append(ctx, "\n", 1));
    bool was_dirty = browse_ctx_dirty_get_set(ctx, false);
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    bool dirty = browse_ctx_dirty_get_append(ctx, was_dirty);
    if (!dirty) 
        buffn(char, reset)(browse_ctx_lazy_str(ctx));
    return Ok;
}

static Err
browse_tag_p(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "\n"));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    if (browse_ctx_lazy_str_len(ctx)) {
        buffn(char, reset)(browse_ctx_lazy_str(ctx));
    } else {
        try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "\n"));
    }
    return Ok;
}

static Err
browse_tag_tr(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "\n"));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    if (browse_ctx_lazy_str_len(ctx)) {
        buffn(char, reset)(browse_ctx_lazy_str(ctx));
    } else {
        try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "\n"));
    }
    return Ok;
}
static Err
browse_tag_ul(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_lit_str("\n\n", cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_li(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    // TODO: implement this more decently
    ////try (serialize_lit_str("\n * ", cb, ctx));
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), " * "));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    if (browse_ctx_lazy_str_len(ctx)) {
        buffn(char, reset)(browse_ctx_lazy_str(ctx));
    }
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_h(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "h1`"));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    if (browse_ctx_lazy_str_len(ctx)) {
        try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "\n"));
    } else try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_b(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    bool preexisting_lazy_str = browse_ctx_lazy_str_len(ctx);
    if (preexisting_lazy_str) try( serialize_color_lazy_(ctx, esc_code_bold));
    else try( _serialize_color_(cb, ctx, esc_code_bold));

    try (browse_list(node->first_child, node->last_child, cb, ctx));

    if( browse_ctx_lazy_str_len(ctx)) log_warn__("%s", "we expect non empty <b> but there are here");
    try( _serialize_color_reset_(cb, ctx));
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), " "));
    return Ok;
}

static Err
browse_tag_blockquote(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( append_to_bufof_char_lit_(browse_ctx_lazy_str(ctx), "``"));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("''", cb, ctx));
    return Ok;
}

static Err
browse_tag_title(lxb_dom_node_t* node, BrowseCtx ctx[static 1]) {
    HtmlDoc* d = browse_ctx_htmldoc(ctx);
    *htmldoc_title(d) = node;
    try( dbg_print_title(node));
    return Ok;
}

Err serialize_mem_skipping_space(
    const char* data, size_t len, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]
) {
    StrView s = strview(data, len);

    while(s.len) {
        StrView word = strview_split_word(&s);
        if (!word.len) break;
        if (cb((lxb_char_t*)word.s, word.len, ctx)) return "error serializing html text elem";
        strview_trim_space_left(&s);
        if (!s.len) break;
        if (cb((lxb_char_t*)" ", 1, ctx)) return "error serializing space";
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
    [0x0000] = "    LXB_TAG__UNDEF             ",
    [0x0001] = "    LXB_TAG__END_OF_FILE       ",
    [0x0002] = "    LXB_TAG__TEXT              ",
    [0x0003] = "    LXB_TAG__DOCUMENT          ",
    [0x0004] = "    LXB_TAG__EM_COMMENT        ",
    [0x0005] = "    LXB_TAG__EM_DOCTYPE        ",
    [0x0006] = "    LXB_TAG_A                  ",
    [0x0007] = "    LXB_TAG_ABBR               ",
    [0x0008] = "    LXB_TAG_ACRONYM            ",
    [0x0009] = "    LXB_TAG_ADDRESS            ",
    [0x000a] = "    LXB_TAG_ALTGLYPH           ",
    [0x000b] = "    LXB_TAG_ALTGLYPHDEF        ",
    [0x000c] = "    LXB_TAG_ALTGLYPHITEM       ",
    [0x000d] = "    LXB_TAG_ANIMATECOLOR       ",
    [0x000e] = "    LXB_TAG_ANIMATEMOTION      ",
    [0x000f] = "    LXB_TAG_ANIMATETRANSFORM   ",
    [0x0010] = "    LXB_TAG_ANNOTATION_XML     ",
    [0x0011] = "    LXB_TAG_APPLET             ",
    [0x0012] = "    LXB_TAG_AREA               ",
    [0x0013] = "    LXB_TAG_ARTICLE            ",
    [0x0014] = "    LXB_TAG_ASIDE              ",
    [0x0015] = "    LXB_TAG_AUDIO              ",
    [0x0016] = "    LXB_TAG_B                  ",
    [0x0017] = "    LXB_TAG_BASE               ",
    [0x0018] = "    LXB_TAG_BASEFONT           ",
    [0x0019] = "    LXB_TAG_BDI                ",
    [0x001a] = "    LXB_TAG_BDO                ",
    [0x001b] = "    LXB_TAG_BGSOUND            ",
    [0x001c] = "    LXB_TAG_BIG                ",
    [0x001d] = "    LXB_TAG_BLINK              ",
    [0x001e] = "    LXB_TAG_BLOCKQUOTE         ",
    [0x001f] = "    LXB_TAG_BODY               ",
    [0x0020] = "    LXB_TAG_BR                 ",
    [0x0021] = "    LXB_TAG_BUTTON             ",
    [0x0022] = "    LXB_TAG_CANVAS             ",
    [0x0023] = "    LXB_TAG_CAPTION            ",
    [0x0024] = "    LXB_TAG_CENTER             ",
    [0x0025] = "    LXB_TAG_CITE               ",
    [0x0026] = "    LXB_TAG_CLIPPATH           ",
    [0x0027] = "    LXB_TAG_CODE               ",
    [0x0028] = "    LXB_TAG_COL                ",
    [0x0029] = "    LXB_TAG_COLGROUP           ",
    [0x002a] = "    LXB_TAG_DATA               ",
    [0x002b] = "    LXB_TAG_DATALIST           ",
    [0x002c] = "    LXB_TAG_DD                 ",
    [0x002d] = "    LXB_TAG_DEL                ",
    [0x002e] = "    LXB_TAG_DESC               ",
    [0x002f] = "    LXB_TAG_DETAILS            ",
    [0x0030] = "    LXB_TAG_DFN                ",
    [0x0031] = "    LXB_TAG_DIALOG             ",
    [0x0032] = "    LXB_TAG_DIR                ",
    [0x0033] = "    LXB_TAG_DIV                ",
    [0x0034] = "    LXB_TAG_DL                 ",
    [0x0035] = "    LXB_TAG_DT                 ",
    [0x0036] = "    LXB_TAG_EM                 ",
    [0x0037] = "    LXB_TAG_EMBED              ",
    [0x0038] = "    LXB_TAG_FEBLEND            ",
    [0x0039] = "    LXB_TAG_FECOLORMATRIX      ",
    [0x003a] = "    LXB_TAG_FECOMPONENTTRANSFER",
    [0x003b] = "    LXB_TAG_FECOMPOSITE        ",
    [0x003c] = "    LXB_TAG_FECONVOLVEMATRIX   ",
    [0x003d] = "    LXB_TAG_FEDIFFUSELIGHTING  ",
    [0x003e] = "    LXB_TAG_FEDISPLACEMENTMAP  ",
    [0x003f] = "    LXB_TAG_FEDISTANTLIGHT     ",
    [0x0040] = "    LXB_TAG_FEDROPSHADOW       ",
    [0x0041] = "    LXB_TAG_FEFLOOD            ",
    [0x0042] = "    LXB_TAG_FEFUNCA            ",
    [0x0043] = "    LXB_TAG_FEFUNCB            ",
    [0x0044] = "    LXB_TAG_FEFUNCG            ",
    [0x0045] = "    LXB_TAG_FEFUNCR            ",
    [0x0046] = "    LXB_TAG_FEGAUSSIANBLUR     ",
    [0x0047] = "    LXB_TAG_FEIMAGE            ",
    [0x0048] = "    LXB_TAG_FEMERGE            ",
    [0x0049] = "    LXB_TAG_FEMERGENODE        ",
    [0x004a] = "    LXB_TAG_FEMORPHOLOGY       ",
    [0x004b] = "    LXB_TAG_FEOFFSET           ",
    [0x004c] = "    LXB_TAG_FEPOINTLIGHT       ",
    [0x004d] = "    LXB_TAG_FESPECULARLIGHTING ",
    [0x004e] = "    LXB_TAG_FESPOTLIGHT        ",
    [0x004f] = "    LXB_TAG_FETILE             ",
    [0x0050] = "    LXB_TAG_FETURBULENCE       ",
    [0x0051] = "    LXB_TAG_FIELDSET           ",
    [0x0052] = "    LXB_TAG_FIGCAPTION         ",
    [0x0053] = "    LXB_TAG_FIGURE             ",
    [0x0054] = "    LXB_TAG_FONT               ",
    [0x0055] = "    LXB_TAG_FOOTER             ",
    [0x0056] = "    LXB_TAG_FOREIGNOBJECT      ",
    [0x0057] = "    LXB_TAG_FORM               ",
    [0x0058] = "    LXB_TAG_FRAME              ",
    [0x0059] = "    LXB_TAG_FRAMESET           ",
    [0x005a] = "    LXB_TAG_GLYPHREF           ",
    [0x005b] = "    LXB_TAG_H1                 ",
    [0x005c] = "    LXB_TAG_H2                 ",
    [0x005d] = "    LXB_TAG_H3                 ",
    [0x005e] = "    LXB_TAG_H4                 ",
    [0x005f] = "    LXB_TAG_H5                 ",
    [0x0060] = "    LXB_TAG_H6                 ",
    [0x0061] = "    LXB_TAG_HEAD               ",
    [0x0062] = "    LXB_TAG_HEADER             ",
    [0x0063] = "    LXB_TAG_HGROUP             ",
    [0x0064] = "    LXB_TAG_HR                 ",
    [0x0065] = "    LXB_TAG_HTML               ",
    [0x0066] = "    LXB_TAG_I                  ",
    [0x0067] = "    LXB_TAG_IFRAME             ",
    [0x0068] = "    LXB_TAG_IMAGE              ",
    [0x0069] = "    LXB_TAG_IMG                ",
    [0x006a] = "    LXB_TAG_INPUT              ",
    [0x006b] = "    LXB_TAG_INS                ",
    [0x006c] = "    LXB_TAG_ISINDEX            ",
    [0x006d] = "    LXB_TAG_KBD                ",
    [0x006e] = "    LXB_TAG_KEYGEN             ",
    [0x006f] = "    LXB_TAG_LABEL              ",
    [0x0070] = "    LXB_TAG_LEGEND             ",
    [0x0071] = "    LXB_TAG_LI                 ",
    [0x0072] = "    LXB_TAG_LINEARGRADIENT     ",
    [0x0073] = "    LXB_TAG_LINK               ",
    [0x0074] = "    LXB_TAG_LISTING            ",
    [0x0075] = "    LXB_TAG_MAIN               ",
    [0x0076] = "    LXB_TAG_MALIGNMARK         ",
    [0x0077] = "    LXB_TAG_MAP                ",
    [0x0078] = "    LXB_TAG_MARK               ",
    [0x0079] = "    LXB_TAG_MARQUEE            ",
    [0x007a] = "    LXB_TAG_MATH               ",
    [0x007b] = "    LXB_TAG_MENU               ",
    [0x007c] = "    LXB_TAG_META               ",
    [0x007d] = "    LXB_TAG_METER              ",
    [0x007e] = "    LXB_TAG_MFENCED            ",
    [0x007f] = "    LXB_TAG_MGLYPH             ",
    [0x0080] = "    LXB_TAG_MI                 ",
    [0x0081] = "    LXB_TAG_MN                 ",
    [0x0082] = "    LXB_TAG_MO                 ",
    [0x0083] = "    LXB_TAG_MS                 ",
    [0x0084] = "    LXB_TAG_MTEXT              ",
    [0x0085] = "    LXB_TAG_MULTICOL           ",
    [0x0086] = "    LXB_TAG_NAV                ",
    [0x0087] = "    LXB_TAG_NEXTID             ",
    [0x0088] = "    LXB_TAG_NOBR               ",
    [0x0089] = "    LXB_TAG_NOEMBED            ",
    [0x008a] = "    LXB_TAG_NOFRAMES           ",
    [0x008b] = "    LXB_TAG_NOSCRIPT           ",
    [0x008c] = "    LXB_TAG_OBJECT             ",
    [0x008d] = "    LXB_TAG_OL                 ",
    [0x008e] = "    LXB_TAG_OPTGROUP           ",
    [0x008f] = "    LXB_TAG_OPTION             ",
    [0x0090] = "    LXB_TAG_OUTPUT             ",
    [0x0091] = "    LXB_TAG_P                  ",
    [0x0092] = "    LXB_TAG_PARAM              ",
    [0x0093] = "    LXB_TAG_PATH               ",
    [0x0094] = "    LXB_TAG_PICTURE            ",
    [0x0095] = "    LXB_TAG_PLAINTEXT          ",
    [0x0096] = "    LXB_TAG_PRE                ",
    [0x0097] = "    LXB_TAG_PROGRESS           ",
    [0x0098] = "    LXB_TAG_Q                  ",
    [0x0099] = "    LXB_TAG_RADIALGRADIENT     ",
    [0x009a] = "    LXB_TAG_RB                 ",
    [0x009b] = "    LXB_TAG_RP                 ",
    [0x009c] = "    LXB_TAG_RT                 ",
    [0x009d] = "    LXB_TAG_RTC                ",
    [0x009e] = "    LXB_TAG_RUBY               ",
    [0x009f] = "    LXB_TAG_S                  ",
    [0x00a0] = "    LXB_TAG_SAMP               ",
    [0x00a1] = "    LXB_TAG_SCRIPT             ",
    [0x00a2] = "    LXB_TAG_SECTION            ",
    [0x00a3] = "    LXB_TAG_SELECT             ",
    [0x00a4] = "    LXB_TAG_SLOT               ",
    [0x00a5] = "    LXB_TAG_SMALL              ",
    [0x00a6] = "    LXB_TAG_SOURCE             ",
    [0x00a7] = "    LXB_TAG_SPACER             ",
    [0x00a8] = "    LXB_TAG_SPAN               ",
    [0x00a9] = "    LXB_TAG_STRIKE             ",
    [0x00aa] = "    LXB_TAG_STRONG             ",
    [0x00ab] = "    LXB_TAG_STYLE              ",
    [0x00ac] = "    LXB_TAG_SUB                ",
    [0x00ad] = "    LXB_TAG_SUMMARY            ",
    [0x00ae] = "    LXB_TAG_SUP                ",
    [0x00af] = "    LXB_TAG_SVG                ",
    [0x00b0] = "    LXB_TAG_TABLE              ",
    [0x00b1] = "    LXB_TAG_TBODY              ",
    [0x00b2] = "    LXB_TAG_TD                 ",
    [0x00b3] = "    LXB_TAG_TEMPLATE           ",
    [0x00b4] = "    LXB_TAG_TEXTAREA           ",
    [0x00b5] = "    LXB_TAG_TEXTPATH           ",
    [0x00b6] = "    LXB_TAG_TFOOT              ",
    [0x00b7] = "    LXB_TAG_TH                 ",
    [0x00b8] = "    LXB_TAG_THEAD              ",
    [0x00b9] = "    LXB_TAG_TIME               ",
    [0x00ba] = "    LXB_TAG_TITLE              ",
    [0x00bb] = "    LXB_TAG_TR                 ",
    [0x00bc] = "    LXB_TAG_TRACK              ",
    [0x00bd] = "    LXB_TAG_TT                 ",
    [0x00be] = "    LXB_TAG_U                  ",
    [0x00bf] = "    LXB_TAG_UL                 ",
    [0x00c0] = "    LXB_TAG_VAR                ",
    [0x00c1] = "    LXB_TAG_VIDEO              ",
    [0x00c2] = "    LXB_TAG_WBR                ",
    [0x00c3] = "    LXB_TAG_XMP                ",
    [0x00c4] = "    LXB_TAG__LAST_ENTRY        " 
};

Err browse_rec_tag(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    switch(node->local_name) {
        case LXB_TAG_A: { return browse_tag_a(node, cb, ctx); }
        case LXB_TAG_B: { return browse_tag_b(node, cb, ctx); }
        case LXB_TAG_BLOCKQUOTE: { return browse_tag_blockquote(node, cb, ctx); }
        case LXB_TAG_BR: { return browse_tag_br(node, cb, ctx); }
        case LXB_TAG_BUTTON: { return browse_tag_button(node, cb, ctx); }
        case LXB_TAG_CENTER: { return browse_tag_center(node, cb, ctx); } 
        case LXB_TAG_DIV: { return browse_tag_div(node, cb, ctx); }
        case LXB_TAG_FORM: { return browse_tag_form(node, cb, ctx); }
        case LXB_TAG_H1: case LXB_TAG_H2: case LXB_TAG_H3: case LXB_TAG_H4: case LXB_TAG_H5: case LXB_TAG_H6: { return browse_tag_h(node, cb, ctx); }
        case LXB_TAG_INPUT: { return browse_tag_input(node, cb, ctx); }
        case LXB_TAG_IMAGE: case LXB_TAG_IMG: { return browse_tag_img(node, cb, ctx); }
        case LXB_TAG_LI: { return browse_tag_li(node, cb, ctx); }
        case LXB_TAG_OL: { return browse_tag_ul(node, cb, ctx); }
        case LXB_TAG_P: { return browse_tag_p(node, cb, ctx); }
        case LXB_TAG_PRE: { return browse_tag_pre(node, cb, ctx); }
        case LXB_TAG_SCRIPT: { log_todo__("%s", "[todo] implement script tag (skipping)"); return Ok; } 
        case LXB_TAG_STYLE: { log_todo__("%s\n", "[todo?] skiping style"); return Ok; } 
        case LXB_TAG_TITLE: { return browse_tag_title(node, ctx); } 
        case LXB_TAG_TR: { return browse_tag_tr(node, cb, ctx); }
        case LXB_TAG_UL: { return browse_tag_ul(node, cb, ctx); }
        default: {
            if (node->local_name >= LXB_TAG__LAST_ENTRY)
                log_warn__("node local name (TAG) greater than last entry: %lx\n", node->local_name);
            else log_todo__("TAG 'NOT' IMPLEMENTED: %s\n", _dbg_tags_[node->local_name]);
            return browse_list(node->first_child, node->last_child, cb, ctx);
        }
    }
}

Err browse_text(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    const char* data;
    size_t len;
    try( lexbor_node_get_text(node, &data, &len));

    if (node->parent->local_name == LXB_TAG_PRE) { //if(browse_ctx_pre_tag(ctx)) {
        //TODO Whitespace inside this element is displayed as written,
        //with one exception. If one or more leading newline characters
        //are included immediately following the opening <pre> tag, the
        //first newline character is stripped. 
        //https://developer.mozilla.org/en-US/docs/Web/HTML/Element/pre

        *browse_ctx_dirty(ctx) = true;
        try( browse_ctx_lazy_str_serialize(ctx, cb));
        if (cb((lxb_char_t*)data, len, ctx)) return "error serializing html text elem";
    } else if (mem_skip_space_inplace(&data, &len)) {
        *browse_ctx_dirty(ctx) = true;
        try( browse_ctx_lazy_str_serialize(ctx, cb));
        try( serialize_mem_skipping_space(data, len, cb, ctx));
    } 
    if (node->first_child || node->last_child)
        log_warn__("%s\n", "LXB_DOM_NODE_TYPE_TEXT with actual childs");
    return Ok;
}

Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (node) {
        switch(node->type) {
            case LXB_DOM_NODE_TYPE_ELEMENT: return browse_rec_tag(node, cb, ctx);
            case LXB_DOM_NODE_TYPE_TEXT: return browse_text(node, cb, ctx);
            //TODO: do not ignore these types?
            case LXB_DOM_NODE_TYPE_DOCUMENT: 
            case LXB_DOM_NODE_TYPE_DOCUMENT_TYPE: 
            case LXB_DOM_NODE_TYPE_COMMENT:
                return browse_list(node->first_child, node->last_child, cb, ctx);
            default: {
                if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                else log_warn__("Ignored Node Type: %s\n", _dbg_node_types_[node->type]);
                return Ok;
            }
        }
    }
    return Ok;
}

Err browse_tag_pre(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try( browse_list(node->first_child, node->last_child, cb, ctx));
    return Ok;
}

/* external linkage */

//TODO: eprecate, always init woit fetch ? (and if bad url?)
Err htmldoc_init(HtmlDoc d[static 1], const char* cstr_url) {
    Url url = {0};
    if (cstr_url && *cstr_url) {
        if (strlen(cstr_url) > MAX_URL_LEN) {
            return "cstr_url large is not supported.";
        }
        Err err = url_init(&url, cstr_url);
        if (err) {
            std_free((char*)cstr_url);
            return err;
        }
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
        .lxbdoc=document,
        .cache=(DocCache){
            .textbuf=(TextBuf){.current_line=1},
            .sourcebuf=(TextBuf){.current_line=1}
        }
    };
    return Ok;
}
Err htmldoc_init_from_curlu(HtmlDoc d[static 1], CURLU* cu, HttpMethod method) {
    Url url = {0};
    try( url_init_from_curlu(&url, cu));
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        return "error: lxb failed to create html document";
    }

    *d = (HtmlDoc){
        .url=url,
        .method=method,
        .lxbdoc=document,
        .cache=(DocCache){
            .textbuf=(TextBuf){.current_line=1},
            .sourcebuf=(TextBuf){.current_line=1}
        }
    };
    return Ok;
}

Err htmldoc_init_fetch_browse(HtmlDoc d[static 1], const char* url, UrlClient url_client[static 1]) {
    try(htmldoc_init(d, url));
    try(htmldoc_fetch(d, url_client));//TODO: clean on failure
    return htmldoc_browse(d);
}

Err htmldoc_init_fetch_browse_from_curlu(
    HtmlDoc d[static 1], CURLU* cu, UrlClient url_client[static 1], HttpMethod method
) {
    try(htmldoc_init_from_curlu(d, cu, method));
    try(htmldoc_fetch(d, url_client));//TODO: clean on failure
    return htmldoc_browse(d);
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

//TODO: pass the max cols and color from session conf
Err htmldoc_browse(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    BrowseCtx ctx;
    try(browse_ctx_init(&ctx, htmldoc, true));
    try(browse_rec(lxb_dom_interface_node(lxbdoc), serialize_cb_browse, &ctx));
    browse_ctx_cleanup(&ctx);
    //TODO: join append-null and fit-lines together in a single static method so that
    //we always call all .
    if (textbuf_len(htmldoc_textbuf(htmldoc))) {
        try( textbuf_append_null(htmldoc_textbuf(htmldoc)));
        return textbuf_fit_lines(htmldoc_textbuf(htmldoc), 90);
    }
    return Ok;
}

