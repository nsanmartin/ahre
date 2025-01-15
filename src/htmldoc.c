#include "src/constants.h"
#include "src/error.h"
#include "src/generic.h"
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/textbuf.h"
#include "src/utils.h"
#include "src/wrapper-lexbor.h"


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

//_Thread_local static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};


static Err browse_ctx_init(BrowseCtx ctx[static 1], HtmlDoc htmldoc[static 1], bool color) {
    //if (!htmldoc) return "error: expected non null htmldoc in browse ctx.";
    *ctx = (BrowseCtx) {.htmldoc=htmldoc, .color=color};
    return Ok;
}

static void browse_ctx_cleanup(BrowseCtx ctx[static 1]) {
    buffn(char, clean)(&ctx->lazy_str);
}


static Err
browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]);

static lxb_status_t
serialize_cb_browse(const lxb_char_t* data, size_t len, void* ctx) {
    TextBuf* textbuf = htmldoc_textbuf(browse_ctx_htmldoc(ctx));
    return textbuf_append_part(textbuf, (char*)data, len)
        ? LXB_STATUS_ERROR
        : LXB_STATUS_OK;
}

lxb_status_t htmldoc_lexbor_serialize_unsigned(
    BrowseCtx ctx[static 1], lxb_html_serialize_cb_f cb, uintmax_t ui
)
{
    return serialize_unsigned(cb, ui, ctx, LXB_STATUS_ERROR);
}


static Err browse_list(lxb_dom_node_t* it, lxb_dom_node_t* last, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    for(; ; it = it->next) {
        Err err = browse_rec(it, cb, ctx);
        if (err) return err;
        if (it == last) { break; }
    }
    return Ok;
}

static Err
browse_tag_br(lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    return serialize_lit_str("\n", cb, ctx);
}

static Err
browse_tag_center(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_lit_str("\n", cb, ctx));
    try (serialize_cstring_debug("%(center:\n\n", cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_cstring_debug("%%center)\n", cb, ctx));
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
_serialize_img_alt(lxb_dom_node_t* img, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {

    const lxb_char_t* alt;
    size_t alt_len;
    lexbor_find_attr_value(img, "alt", &alt, &alt_len);

    if (alt && alt_len) {
        try_lxb_serialize(" ", 1, cb, ctx);
        if (cb(alt, alt_len, ctx)) return "error serializing img's alf";
    }
    return Ok;
}

static Err
browse_tag_img(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_literal_color_str(EscCodeLightGreen, cb, ctx));
    HtmlDoc* d = browse_ctx_htmldoc(ctx);
    ArlOf(LxbNodePtr)* imgs = htmldoc_imgs(d);
    const size_t img_count = len__(imgs);
    try( append_to_arlof_lxb_node__(imgs, &node));
    try_lxb_serialize(IMAGE_OPEN_STR, sizeof(IMAGE_OPEN_STR)-1, cb, ctx);
    try_lxb (htmldoc_lexbor_serialize_unsigned(ctx, cb, img_count), "error serializing unsigned");
    try( _serialize_img_alt(node, cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try_lxb_serialize(IMAGE_CLOSE_STR, sizeof(IMAGE_CLOSE_STR)-1, cb, ctx);
    try (serialize_literal_color_str(EscCodeReset, cb, ctx));
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
        try (serialize_literal_color_str(EscCodeRed, cb, ctx));
        try (serialize_lit_str(INPUT_OPEN_STR, cb, ctx));
        try_lxb (htmldoc_lexbor_serialize_unsigned(ctx, cb, inputs->len-1),
            "error serializing unsigned");
        if (_input_is_text_type_(s, slen)) {
            lexbor_find_attr_value(node, "value", &s, &slen);
            if (slen) {
                try (serialize_lit_str(" ", cb, ctx));
                try (serialize_cstring(s, slen, cb, ctx));
            }
        } else if (_input_is_submit_type_(s, slen)) {

            lexbor_find_attr_value(node, "value", &s, &slen);
            if (slen) {
                try (serialize_lit_str("|", cb, ctx));
                try (serialize_cstring(s, slen, cb, ctx));
            }
        } else {
            try (serialize_lit_str("[not supported input]", cb, ctx));
        }
        try (serialize_lit_str(INPUT_CLOSE_STR, cb, ctx));
        try (serialize_literal_color_str(EscCodeReset, cb, ctx));
    }
    return Ok;
}


static Err
browse_tag_div(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (!buffn(char, append)(&ctx->lazy_str, "\n", 1)) return "error: failed to append to bufof";
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    buffn(char, reset)(&ctx->lazy_str);
    return Ok;
}
static Err
browse_tag_p(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_lit_str("\n", cb, ctx)); try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_ul(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_lit_str("\n", cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_h(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    try (serialize_lit_str("\n", cb, ctx));
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err browse_tag_blockquote(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (!buffn(char, append)(&ctx->lazy_str, "``", 2)) return "error: failed to append to bufof";
    try (browse_list(node->first_child, node->last_child, cb, ctx));
    try (serialize_lit_str("''", cb, ctx));
    return Ok;
}


static Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (node) {
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {

            switch(node->local_name) {
                case LXB_TAG_A: { return browse_tag_a(node, cb, ctx); }
                case LXB_TAG_BLOCKQUOTE: { return browse_tag_blockquote(node, cb, ctx); }
                case LXB_TAG_BR: { browse_tag_br(cb, ctx); break; }
                case LXB_TAG_CENTER: { return browse_tag_center(node, cb, ctx); } 
                case LXB_TAG_DIV: { return browse_tag_div(node, cb, ctx); }
                case LXB_TAG_FORM: { return browse_tag_form(node, cb, ctx); }
                case LXB_TAG_H1: case LXB_TAG_H2: case LXB_TAG_H3: case LXB_TAG_H4: case LXB_TAG_H5: case LXB_TAG_H6: { return browse_tag_h(node, cb, ctx); }
                case LXB_TAG_INPUT: { return browse_tag_input(node, cb, ctx); }
                case LXB_TAG_IMAGE: case LXB_TAG_IMG: { return browse_tag_img(node, cb, ctx); }
                case LXB_TAG_P: { return browse_tag_p(node, cb, ctx); }
                case LXB_TAG_SCRIPT: { /*printf("skip script\n");*/ return Ok; } 
                case LXB_TAG_STYLE: { /*printf("skip style\n");*/ return Ok; } 
                case LXB_TAG_TITLE: { /*printf("skip title\n");*/ return Ok; } 
                case LXB_TAG_UL: { return browse_tag_ul(node, cb, ctx); }
            }
        } else if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
            size_t len = lxb_dom_interface_text(node)->char_data.data.length;
            const char* data = (const char*)lxb_dom_interface_text(node)->char_data.data.data;

            if (!mem_is_all_space((char*)data, len)) {
                if (ctx->lazy_str.len) {
                    if(cb((lxb_char_t*)ctx->lazy_str.items, ctx->lazy_str.len, ctx))
                        return "error serializing nl";
                    buffn(char, reset)(&ctx->lazy_str);
                }

                while((data = cstr_skip_space((const char*)data))) {
                    if (!*data) break;
                    const char* end = cstr_next_space((const char*)data);
                    if (data == end) break;
                    if (cb((lxb_char_t*)data, end-data, ctx)) return "error serializing html text elem";
                    if (cb((lxb_char_t*)" ", 1, ctx)) return "error serializing space";
                    data = end;
                }
            }
        }

        Err err = browse_list(node->first_child, node->last_child, cb, ctx);
        if (err) return err;
    }   

    return Ok;
}


/* external linkage */

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
        .cache=(DocCache){.textbuf=(TextBuf){.current_line=1}}
    };
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


void htmldoc_reset(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
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


//deprecated
//static Err htmldoc_read_from_file(HtmlDoc htmldoc[static 1]) {
//    FILE* fp = fopen(htmldoc->url, "r");
//    if  (!fp) { return strerror(errno); }
//
//    Err err = NULL;
//
//    TextBuf* textbuf = htmldoc_textbuf(htmldoc);
//    size_t bytes_read = 0;
//    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
//        if ((err = textbuf_append_part(textbuf, (char*)read_from_file_buffer, READ_FROM_FILE_BUFFER_LEN))) {
//            return err;
//        }
//    }
//    //TODO: free mem?
//    if (ferror(fp)) { fclose(fp); return strerror(errno); }
//    fclose(fp);
//
//    return textbuf_append_null(textbuf);
//}


Err htmldoc_browse(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    BrowseCtx ctx;
    try(browse_ctx_init(&ctx, htmldoc, true));
    try(browse_rec(lxb_dom_interface_node(lxbdoc), serialize_cb_browse, &ctx));
    browse_ctx_cleanup(&ctx);
    return textbuf_append_null(htmldoc_textbuf(htmldoc));
}

