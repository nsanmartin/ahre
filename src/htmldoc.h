#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <lexbor/html/html.h>

#include "src/textbuf.h"
#include "src/error.h"
#include "src/mem.h"
#include "src/url-client.h"
#include "src/utils.h"
#include "src/wrapper-lexbor.h"
#include "src/wrapper-lexbor-curl.h"
#include "src/doc-elem.h"
#include "src/url.h"


typedef lxb_dom_node_t* LxbNodePtr;
#define T LxbNodePtr
#include <arl.h>

typedef struct {
    TextBuf sourcebuf;
    TextBuf textbuf;
    ArlOf(LxbNodePtr) anchors;
    ArlOf(LxbNodePtr) imgs;
    ArlOf(LxbNodePtr) inputs;
    LxbNodePtr title;
} DocCache;

typedef struct {
    Url url;
    HttpMethod method;
    lxb_html_document_t* lxbdoc;
    DocCache cache;
} HtmlDoc;

/* getters */
static inline lxb_html_document_t*
htmldoc_lxbdoc(HtmlDoc d[static 1]) { return d->lxbdoc; }

static inline TextBuf*
htmldoc_sourcebuf(HtmlDoc d[static 1]) { return &d->cache.sourcebuf; }

static inline TextBuf*
htmldoc_textbuf(HtmlDoc d[static 1]) { return &d->cache.textbuf; }

static inline ArlOf(LxbNodePtr)*
htmldoc_anchors(HtmlDoc d[static 1]) { return &d->cache.anchors; }

static inline ArlOf(LxbNodePtr)*
htmldoc_imgs(HtmlDoc d[static 1]) { return &d->cache.imgs; }

static inline ArlOf(LxbNodePtr)*
htmldoc_inputs(HtmlDoc d[static 1]) { return &d->cache.inputs; }

static inline LxbNodePtr*
htmldoc_title(HtmlDoc d[static 1]) { return &d->cache.title; }

static inline DocCache*
htmldoc_cache(HtmlDoc d[static 1]) { return &d->cache; }

static inline Url*
htmldoc_url(HtmlDoc d[static 1]) { return &d->url; }

static inline HttpMethod
htmldoc_method(HtmlDoc d[static 1]) { return d->method; }


/* ctors */
Err htmldoc_init(HtmlDoc d[static 1], const char* url);
Err htmldoc_init_from_curlu(HtmlDoc d[static 1], CURLU* cu, HttpMethod method);

/* dtors */
void htmldoc_reset(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
//TODO: this fn should cleanup instead the browse data
static inline void htmldoc_cache_cleanup(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    textbuf_cleanup(htmldoc_sourcebuf(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    *htmldoc_cache(htmldoc) = (DocCache){0};
}

/* 
 * Browse context
 */
#define T lxb_tag_id_enum_t
#include <arl.h>

typedef struct { 
    HtmlDoc* htmldoc;
    BufOf(char) lazy_str;
    bool color;
    bool dirty;
    ///ArlOf(lxb_tag_id_enum_t) stack;
} BrowseCtx;

static inline HtmlDoc* browse_ctx_htmldoc(BrowseCtx ctx[static 1]) { return ctx->htmldoc; }
static inline TextBuf* browse_ctx_textbuf(BrowseCtx ctx[static 1]) {
    return htmldoc_textbuf(browse_ctx_htmldoc(ctx));
}

static inline bool browse_ctx_color(BrowseCtx ctx[static 1]) { return ctx->color; }

static inline bool* browse_ctx_dirty(BrowseCtx ctx[static 1]) { return &ctx->dirty; }
static inline bool browse_ctx_dirty_get_set(BrowseCtx ctx[static 1], bool value) {
    bool was_dirty = *browse_ctx_dirty(ctx);
    *browse_ctx_dirty(ctx) = value;
    return was_dirty;
}

static inline bool browse_ctx_dirty_get_append(BrowseCtx ctx[static 1], bool value) {
    bool was_dirty = *browse_ctx_dirty(ctx);
    *browse_ctx_dirty(ctx) |= value;
    return was_dirty;
}
///static inline ArlOf(lxb_tag_id_enum_t)* browse_ctx_stack(BrowseCtx ctx[static 1]) {
///    return &ctx->stack;
///}

///static inline lxb_tag_id_enum_t* browse_ctx_stack_backp(BrowseCtx ctx[static 1]) {
///    ArlOf(lxb_tag_id_enum_t)* stack = browse_ctx_stack(ctx);
///    return arlfn(lxb_tag_id_enum_t, back)(stack);
///}
///static inline bool browse_ctx_pre_tag(BrowseCtx ctx[static 1]) {
///    lxb_tag_id_enum_t* backp = browse_ctx_stack_backp(ctx);
///    return backp && *backp == LXB_TAG_PRE;
///}

static inline BufOf(char)* browse_ctx_lazy_str(BrowseCtx ctx[static 1]) { return &ctx->lazy_str; }
static inline size_t browse_ctx_lazy_str_len(BrowseCtx ctx[static 1]) {
    return browse_ctx_lazy_str(ctx)->len;
}
static inline char* browse_ctx_lazy_str_items(BrowseCtx ctx[static 1]) { 
    return browse_ctx_lazy_str(ctx)->items;
}

static inline Err browse_ctx_lazy_str_append(BrowseCtx ctx[static 1], char* s, size_t len) { 
    return buffn(char, append)(browse_ctx_lazy_str(ctx), s, len)
        ? Ok
        : "error: failed to append to bufof (browse ctx lazy str)";
}

///static inline Err browse_ctx_push_tag(BrowseCtx ctx[static 1], lxb_tag_id_enum_t tag) {
///    return arlfn(lxb_tag_id_enum_t, append)(browse_ctx_stack(ctx), &tag)
///        ? Ok
///        : "error: arlfn failed to appned";
///}

///static inline Err browse_ctx_pop_tag(BrowseCtx ctx[static 1], lxb_tag_id_enum_t tag) {
///    lxb_tag_id_enum_t* backp = browse_ctx_stack_backp(ctx);
///    if ( !backp || tag != *backp ) return "error: expected tag not found";
///    --browse_ctx_stack(ctx)->len;
///    return Ok;
///}

///static inline bool browse_ctx_inside_tag(BrowseCtx ctx[static 1], lxb_tag_id_enum_t tag) {
///    ArlOf(lxb_tag_id_enum_t)* stack = browse_ctx_stack(ctx);
///    return stack->len && memchr(stack->items, (char)tag, stack->len);
///}


static inline Err
browse_ctx_lazy_str_serialize(BrowseCtx ctx[static 1], lxb_html_serialize_cb_f cb) {
    if (browse_ctx_lazy_str_len(ctx)) {

        lxb_status_t status = cb(
            (lxb_char_t*)browse_ctx_lazy_str_items(ctx), browse_ctx_lazy_str_len(ctx), ctx
        );
        buffn(char, reset)(browse_ctx_lazy_str(ctx));
        if (status != LXB_STATUS_OK) return "error serializing lazy str";
    }
    return Ok;
}


static inline Err browse_ctx_init(BrowseCtx ctx[static 1], HtmlDoc htmldoc[static 1], bool color) {
    *ctx = (BrowseCtx) {.htmldoc=htmldoc, .color=color};
    return Ok;
}

static inline void browse_ctx_cleanup(BrowseCtx ctx[static 1]) {
    buffn(char, clean)(&ctx->lazy_str);
    ///arlfn(lxb_tag_id_enum_t, clean)(browse_ctx_stack(ctx));
}

static inline void
browse_ctx_set_color(BrowseCtx ctx[static 1], bool value) {  ctx->color = value; }


/* external */
Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]);

/**/

Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);


HtmlDoc* htmldoc_create(const char* url);


static inline bool file_exists(const char* path) { return access(path, F_OK) == 0; }
Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) ;

static inline Err htmldoc_fetch(HtmlDoc htmldoc[static 1], UrlClient url_client[static 1]) {
        return curl_lexbor_fetch_document(url_client, htmldoc);
}

Err htmldoc_browse(HtmlDoc htmldoc[static 1]);

#define serialize_lit_str(LitStr, CallBack, Context) \
 ((LXB_STATUS_OK != CallBack((lxb_char_t*)LitStr, sizeof(LitStr)-1, Context)) \
 ?  "error serializing literal string" : Ok)

#define serialize_literal_color_str(EscSeq, CallBack, Context) \
    (browse_ctx_color(Context) ? serialize_lit_str(EscSeq, CallBack, Context) : Ok)

/* htmldoc_tag_a.c */
Err parse_href_attrs(lxb_dom_node_t* node, BrowseCtx ctx[static 1]);
Err parse_append_ahref(BrowseCtx ctx[static 1], const char* url, size_t len);
Err htmldoc_init_fetch_browse(HtmlDoc d[static 1], const char* url, UrlClient url_client[static 1]);

Err htmldoc_init_fetch_browse_from_curlu(
    HtmlDoc d[static 1], CURLU* cu, UrlClient url_client[static 1], HttpMethod method
);

static inline Err htmldoc_print_info(HtmlDoc d[static 1]) {
    try( dbg_print_title(*htmldoc_title(d)));
    char* buf;
    try(url_cstr(htmldoc_url(d), &buf));
    printf("%s\n", buf);
    //Err res = err_fmt("current url:\n%s", buf);
    curl_free(buf);
    return Ok;
}

Err serialize_mem_skipping_space(
    const char* data, size_t len, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]
);

Err browse_rec(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]);

static inline Err
browse_list(
    lxb_dom_node_t* it, lxb_dom_node_t* last, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]
) {
    for(; ; it = it->next) {
        try( browse_rec(it, cb, ctx));
        if (it == last) return Ok;
    }
    return Ok;
}

#endif
