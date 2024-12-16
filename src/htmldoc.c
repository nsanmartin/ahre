#include "src/textbuf.h"
#include "src/htmldoc.h"
#include "src/error.h"
#include "src/generic.h"
#include "src/mem.h"
#include "src/utils.h"
#include "src/wrapper-lexbor.h"

/* internal linkage */
//static constexpr size_t MAX_URL_LEN = 2048;
#define MAX_URL_LEN 2048
//static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
#define READ_FROM_FILE_BUFFER_LEN 4096

#define serialize_lit_str(EscSeq, CallBack, Context) \
 ((LXB_STATUS_OK != CallBack((lxb_char_t*)EscSeq, sizeof EscSeq, Context)) \
 ?  "error serializing literal string" : Ok)

#define serialize_literal_color_str(EscSeq, CallBack, Context) \
    ((Context)->color ? serialize_lit_str(EscSeq, CallBack, Context) : Ok)

#define serialize_cstring(Ptr, Len, CallBack, Context) \
 ((LXB_STATUS_OK != CallBack((lxb_char_t*)Ptr, Len, Context)) \
 ?  "error serializing data" : Ok)

#define try_lxb_serialize(Ptr, Len, CallBack, Context) \
     if (LXB_STATUS_OK != CallBack((lxb_char_t*)Ptr, Len, Context)) \
        return "error serializing data"

#define serialize_cstring_debug(LitStr, CallBack, Context) \
    serialize_cstring(LitStr, sizeof LitStr, CallBack, Context)

_Thread_local static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};

typedef struct { 
    HtmlDoc* htmldoc;
    bool color;
} BrowseCtx;

static Err browse_ctx_init(BrowseCtx ctx[static 1], HtmlDoc htmldoc[static 1], bool color) {
    if (!htmldoc) return "error: expected non null htmldoc in browse ctx.";
    *ctx = (BrowseCtx) {.htmldoc=htmldoc, .color=color};
    return Ok;
}

static HtmlDoc* browse_ctx_htmldoc(BrowseCtx ctx[static 1]) { return ctx->htmldoc; }
static TextBuf* browse_ctx_textbuf(BrowseCtx ctx[static 1]) {
    return htmldoc_textbuf(browse_ctx_htmldoc(ctx));
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

static Err 
parse_append_ahref(BrowseCtx ctx[static 1], const char* url, size_t len, lxb_html_serialize_cb_f cb) {
    if (!url) return "error: NULL url";

    ArlOf(Ahref)* ahrefs = htmldoc_ahrefs(browse_ctx_htmldoc(ctx));
    if (
        LXB_STATUS_OK != cb((const lxb_char_t*) "[", 1, ctx)
        || LXB_STATUS_OK != htmldoc_lexbor_serialize_unsigned(ctx, cb, ahrefs->len)
        || LXB_STATUS_OK != cb((const lxb_char_t*) "]", 1, ctx)
    ) { return "error serializing data"; }

    Ahref a = (Ahref){0};
    if (ahref_init_alloc(&a, url, len, textbuf_len(browse_ctx_textbuf(ctx))))
        return "error: intialiazing Ahref";
    if (!arlfn(Ahref,append)(ahrefs, &a)) {
        free((char*)a.url);
        return "error: lip set";
    }
    return Ok;
}


/*
 * This is obviously innefficient, for each time an attr is queried, 
 * all of them are iterated, but: there are more priority tasks and
 * there are ussually few attrs. 
 */
static void parse_attr_value(
    lxb_dom_node_t* node, const char* attr_name, const lxb_char_t* out[static 1], size_t* len
) 
{
    lxb_dom_attr_t* attr;

    attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));
    while (attr) {
        size_t data_len;
        const lxb_char_t* data = lxb_dom_attr_qualified_name(attr, &data_len);
        if (!strncmp(attr_name, (char*)data, data_len))  {

            *out = lxb_dom_attr_value(attr, len);
            return;

        }
        attr = lxb_dom_element_next_attribute(attr);
    }

    *out = NULL; *len = 0;
}


static Err
parse_href_attrs(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1])
{
    const lxb_char_t* data;
    size_t data_len;
    parse_attr_value(node, "href", &data, &data_len);
    if (data != NULL) {
        Err err = parse_append_ahref(ctx, (const char*)data, data_len, cb);
        if (err) return err;
    } else
        puts("warn log: expecting 'href' but not found");
    return Ok;

}


//static Err 
//htmldoc_append_img(HtmlDoc d[static 1], const char* url, size_t len, lxb_html_serialize_cb_f cb) {
//    if (!url) return "error: NULL url";
//
//    ArlOf(Img)* imgs = htmldoc_imgs(d);
//    if (
//        LXB_STATUS_OK != cb((const lxb_char_t*) "<", 1, d)
//        || LXB_STATUS_OK != htmldoc_lexbor_serialize_unsigned(d, cb, imgs->len)
//        || LXB_STATUS_OK != cb((const lxb_char_t*) ">", 1, d)
//    ) { return "error serializing data"; }
//
//    Img i = (Ahref){0};
//    if (img_init_alloc(&a, url, len, textbuf_len(htmldoc_textbuf(d))))
//        return "error: intialiazing Img";
//    if (!arlfn(Img,append)(img, &i)) {
//        free((char*)i.src);
//        return "error: lip set";
//    }
//    return Ok;
//}


static Err
parse_img_attrs(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1])
{
    const lxb_char_t* src;
    size_t src_len;
    parse_attr_value(node, "src", &src, &src_len);

    const lxb_char_t* alt;
    size_t alt_len;
    parse_attr_value(node, "alt", &alt, &alt_len);

    HtmlDoc* d = browse_ctx_htmldoc(ctx);

    ArlOf(Img)* imgs = htmldoc_imgs(d);
    try_lxb_serialize("(", 1, cb, ctx);
    try_lxb (htmldoc_lexbor_serialize_unsigned(ctx, cb, imgs->len),
            "error serializing unsigned");
    try_lxb_serialize(")", 1, cb, ctx);
    if (alt && cb(alt, alt_len, ctx)) return "error serializing alt";
    

    Img i = (Img){0};
    if (img_init_alloc(&i, (const char*)src, src_len, textbuf_len(htmldoc_textbuf(d))))
        return "error: intialiazing Img";
    if (!arlfn(Img,append)(imgs, &i)) {
        free((char*)i.src);
        return "error: lip set";
    }
    return Ok;

}

//static Err
//parse_attr_img(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1])
//{
//    const lxb_char_t* data;
//    size_t data_len;
//    parse_attr_value(node, "img", &data, &data_len);
//    if (data != NULL) {
//        Err err = htmldoc_append_ahref(ctx, (const char*)data, data_len, cb);
//        if (err) return err;
//    }
//    puts("warn log: expecting 'href' but not found");
//    return Ok;
//
//}


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
    Err err = Ok;
    try (err, serialize_lit_str("\n", cb, ctx));
    try (err, serialize_cstring_debug("%(center:\n\n", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_cstring_debug("%center)\n", cb, ctx));
    try (err, serialize_lit_str("\n", cb, ctx));
    return err;
}

//TODO: move instance of Ahref here as well as appending it
static Err
browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_literal_color_str(EscCodeBlue, cb, ctx));
    try (err, parse_href_attrs(node, cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_literal_color_str(EscCodeBlue, cb, ctx));
    try (err, serialize_literal_color_str(EscCodeReset, cb, ctx));
    return err;
}

static Err
browse_tag_img(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_literal_color_str(EscCodeLightGreen, cb, ctx));
    try (err, parse_img_attrs(node, cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_literal_color_str(EscCodeReset, cb, ctx));
    return err;
}

static Err
browse_tag_form(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    //(void)node;
    //(void)cb;
    //(void)ctx;
    //return err_fmt("%s not implemented", __func__);;
    try (err, serialize_cstring_debug("\n", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_cstring_debug("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_input(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    (void)node;
    (void)cb;
    (void)ctx;
    const lxb_char_t* s;
    size_t slen;
    parse_attr_value(node, "type", &s, &slen);
    if (!slen || !strcmp("text", (char*)s)) {
        parse_attr_value(node, "value", &s, &slen);
        try (err, serialize_lit_str("[", cb, ctx));
        if (slen) {
            try (err, serialize_cstring(s, slen, cb, ctx));
        }
        else {
            try (err, serialize_lit_str("        ", cb, ctx));
        }
        try (err, serialize_lit_str("]", cb, ctx));
    } else if (!strcmp("submit", (char*)s)) {

        try (err, serialize_literal_color_str(EscCodeRed, cb, ctx));
        try (err, serialize_lit_str("[", cb, ctx));
        parse_attr_value(node, "value", &s, &slen);
        if (slen) {
            try (err, serialize_cstring(s, slen, cb, ctx));
        }
        else {
            try (err, serialize_lit_str("        ", cb, ctx));
        }
        try (err, serialize_lit_str("]", cb, ctx));
        try (err, serialize_literal_color_str(EscCodeReset, cb, ctx));
    } else {
        try (err, serialize_lit_str("[]", cb, ctx));
    }
    //try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    //try (err, serialize_lit_str("]", cb, ctx));
    return Ok;
}


static Err
browse_tag_p(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_lit_str("\n", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_ul(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_lit_str("\n", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err
browse_tag_h(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_lit_str("\n", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_lit_str("\n", cb, ctx));
    return Ok;
}

static Err browse_tag_blockquote(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    Err err = Ok;
    try (err, serialize_lit_str("\n", cb, ctx));
    try (err, serialize_cstring_debug("%(blockquote: ", cb, ctx));
    try (err, browse_list(node->first_child, node->last_child, cb, ctx));
    try (err, serialize_cstring_debug("%blockquote)", cb, ctx));
    try (err, serialize_lit_str("\n", cb, ctx));
    return err;
}


static Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]) {
    if (node) {
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {

            switch(node->local_name) {
                case LXB_TAG_A: { return browse_tag_a(node, cb, ctx); }
                case LXB_TAG_BLOCKQUOTE: { return browse_tag_blockquote(node, cb, ctx); }
                case LXB_TAG_BR: { browse_tag_br(cb, ctx); break; }
                case LXB_TAG_CENTER: { return browse_tag_center(node, cb, ctx); } 
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
            unsigned char* data = lxb_dom_interface_text(node)->char_data.data.data;

            //TODO: print lines skppig newline

            if (!is_all_space((char*)data, len)) {
                //HtmlDoc htmldoc = ctx;
                //TextBuf* textbuf = htmldoc_textbuf(ctx);
                ///size_t beg = textbuf_len(textbuf);
                StrView s = strview((char*)data, len);
                strview_trim_space(&s);
                cb((lxb_char_t*)s.s, s.len, ctx);
                //TODO: do not append newline once each element is parsed propperly
                //cb((lxb_char_t*)"\n", 1, ctx);
                ///size_t end = textbuf_len(textbuf);
                ///DocElem elem = (DocElem){ .tag=DOC_ELEM_TEXT,.offset=beg,.len=end-beg };
                ///if (!arlfn(DocElem,append)(htmldoc_elems(ctx), &elem)) { return "error: lip set"; }
                ///fwrite( textbuf_items(textbuf) + beg, 1, end-beg, stdout);
            }
            
        }

        Err err = browse_list(node->first_child, node->last_child, cb, ctx);
        if (err) return err;
    }   

    return Ok;
}


/* external linkage */

Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    try_lxb (lxb_html_document_parse_chunk_begin(htmldoc->lxbdoc),
            "Lex failed to init html document");


    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        try_nonzero( lexbor_parse_chunk_callback((char*)read_from_file_buffer, 1, bytes_read, htmldoc),
                     "Failed to parse HTML chunk");

    }
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    try_lxb (lxb_html_document_parse_chunk_end(htmldoc->lxbdoc),
            "Lbx failed to parse html");

    return 0x0;
}




int htmldoc_init(HtmlDoc d[static 1], const char* url) {
    //TODO: trim space in url 
    if (url && *url) {
        if (strlen(url) > MAX_URL_LEN) {
            perror("Url large is not supported.");
            return -1;
        }
        url = strdup(url);
        if (!url || !*url) {
            perror("memory error: url cannot be copied");
            return -1;
        }
    } else url = 0x0;

    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        return -1;
    }

    *d = (HtmlDoc){
        .url=url, .lxbdoc=document, .textbuf=(TextBuf){.current_line=1}
    };
    return 0;
}

HtmlDoc* htmldoc_create(const char* url) {
    HtmlDoc* rv = std_malloc(sizeof(HtmlDoc));
    if (!rv) return NULL;
    //Str u;
    //if (str_init(&u, url)) { return NULL; }
    if (htmldoc_init(rv, url)) {
        destroy(rv);
        return NULL;
    }
    return rv;
}


void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(&htmldoc->cache);
    lxb_html_document_destroy(htmldoc->lxbdoc);
    textbuf_cleanup(&htmldoc->textbuf);
    buffn(char, clean)(&htmldoc->sourcebuf);
    destroy((char*)htmldoc->url);
    arlfn(Ahref,clean)(htmldoc_ahrefs(htmldoc));
    arlfn(Img,clean)(htmldoc_imgs(htmldoc));
}

inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}


inline void htmldoc_update_url(HtmlDoc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}


bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    return htmldoc->url && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

Err htmldoc_read_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    Err err = NULL;

    TextBuf* textbuf = htmldoc_textbuf(htmldoc);
    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if ((err = textbuf_append_part(textbuf, (char*)read_from_file_buffer, READ_FROM_FILE_BUFFER_LEN))) {
            return err;
        }
    }
    //TODO: free mem?
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    return textbuf_append_null(textbuf);
}


Err htmldoc_browse(HtmlDoc htmldoc[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(htmldoc);
    Err err = Ok;
    BrowseCtx ctx;
    try(err, browse_ctx_init(&ctx, htmldoc, true));
    try(err, browse_rec(lxb_dom_interface_node(lxbdoc), serialize_cb_browse, &ctx));
    return textbuf_append_null(htmldoc_textbuf(htmldoc));
}


