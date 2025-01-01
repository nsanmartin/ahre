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
#include "src/wrapper-lexbor-curl.h"
#include "src/doc-elem.h"
#include "src/url.h"

//TODO: move to htmldoc-cache.h
typedef struct { const char* url; size_t off; } Ahref;

static inline int ahref_init_alloc(
    Ahref a[static 1], const char* url, size_t len, size_t off
)
{
    const char* copy = mem_to_dup_str(url, len);
    if (!copy) { return -1; }
    *a = (Ahref){.url=copy, .off=off};
    return 0;
}
static inline void
ahref_clean(Ahref a[static 1]) { std_free((char*)(a->url)); }

#define T Ahref
#define TClean ahref_clean
#include <arl.h>

typedef struct { const char* src; size_t off; } Img;
static inline int img_init_alloc(
    Img i[static 1], const char* src, size_t srclen, size_t off
)
{
    const char* srccopy = mem_to_dup_str(src, srclen);
    if (!srccopy) { return -1; }
    *i = (Img){.src=srccopy, .off=off};
    return 0;
}
static inline void
img_clean(Img i[static 1]) {
    std_free((char*)(i->src));
}
#define T Img
#define TClean img_clean
#include <arl.h>

#define T DocElem
#include <arl.h>

typedef lxb_dom_node_t* LxbNodePtr;
#define T LxbNodePtr
#include <arl.h>

typedef struct {
    BufOf(char) sourcebuf;
    TextBuf textbuf;
    ArlOf(Ahref) ahrefs;
    ArlOf(Img) imgs;
    ArlOf(LxbNodePtr) inputs;
} DocCache;


typedef struct {
    const char* url;
    Url curlu;
    lxb_html_document_t* lxbdoc;
    DocCache cache;
} HtmlDoc;

/* getters */
static inline const char*
htmldoc_url(const HtmlDoc d[static 1]) { return d->url; }

static inline lxb_html_document_t*
htmldoc_lxbdoc(HtmlDoc d[static 1]) { return d->lxbdoc; }

static inline BufOf(char)*
htmldoc_sourcebuf(HtmlDoc d[static 1]) { return &d->cache.sourcebuf; }

static inline TextBuf*
htmldoc_textbuf(HtmlDoc d[static 1]) { return &d->cache.textbuf; }

static inline ArlOf(Ahref)*
htmldoc_ahrefs(HtmlDoc d[static 1]) { return &d->cache.ahrefs; }

static inline ArlOf(Img)*
htmldoc_imgs(HtmlDoc d[static 1]) { return &d->cache.imgs; }

static inline ArlOf(LxbNodePtr)*
htmldoc_inputs(HtmlDoc d[static 1]) { return &d->cache.inputs; }

static inline DocCache*
htmldoc_cache(HtmlDoc d[static 1]) { return &d->cache; }

static inline Url*
htmldoc_curlu(HtmlDoc d[static 1]) { return &d->curlu; }


/* ctors */
Err htmldoc_init(HtmlDoc d[static 1], const char* url);

/* dtors */
void htmldoc_reset(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
//TODO: this fn should cleanup instead the browse data
static inline void htmldoc_cache_cleanup(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    buffn(char, clean)(htmldoc_sourcebuf(htmldoc));
    arlfn(Ahref,clean)(htmldoc_ahrefs(htmldoc));
    arlfn(Img,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    *htmldoc_cache(htmldoc) = (DocCache){0};
}


typedef struct { 
    HtmlDoc* htmldoc;
    bool color;
    BufOf(char) lazy_str;
} BrowseCtx;

static inline HtmlDoc* browse_ctx_htmldoc(BrowseCtx ctx[static 1]) { return ctx->htmldoc; }
static inline TextBuf* browse_ctx_textbuf(BrowseCtx ctx[static 1]) {
    return htmldoc_textbuf(browse_ctx_htmldoc(ctx));
}

/* external */
Err
curl_lexbor_fetch_document(UrlClient url_client[static 1], HtmlDoc htmldoc[static 1]);

/**/

Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);


HtmlDoc* htmldoc_create(const char* url);

static inline bool htmldoc_has_url(HtmlDoc htmldoc[static 1]) {
    return htmldoc->url != NULL;
}

void htmldoc_update_url(HtmlDoc htmldoc[static 1], char* url) ;
bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]);


static inline bool file_exists(const char* path) { return access(path, F_OK) == 0; }
Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) ;

static inline Err htmldoc_fetch(HtmlDoc htmldoc[static 1], UrlClient url_client[static 1]) {
    //TODO: remove, not needed since CURLU
    //if (file_exists(htmldoc->url)) {
    //    return lexbor_read_doc_from_file(htmldoc);
    //}
    return curl_lexbor_fetch_document(url_client, htmldoc);
}

Err htmldoc_browse(HtmlDoc htmldoc[static 1]);

static inline const char* htmldoc_abs_url_dup(const HtmlDoc htmldoc[static 1], const char* url) {
    if (cstr_starts_with(url, "//"))
        return cstr_cat_dup("https://", url); ///TODO: suppoert for http
    else if (cstr_starts_with(url, "/"))
        return cstr_cat_dup(htmldoc_url(htmldoc), url);
    return strdup(url);
}

#define serialize_lit_str(LitStr, CallBack, Context) \
 ((LXB_STATUS_OK != CallBack((lxb_char_t*)LitStr, sizeof LitStr, Context)) \
 ?  "error serializing literal string" : Ok)

#define serialize_literal_color_str(EscSeq, CallBack, Context) \
    ((Context)->color ? serialize_lit_str(EscSeq, CallBack, Context) : Ok)

/* htmldoc_tag_a.c */
Err browse_tag_a(lxb_dom_node_t* node, lxb_html_serialize_cb_f cb, BrowseCtx ctx[static 1]);
Err parse_href_attrs(lxb_dom_node_t* node, BrowseCtx ctx[static 1]);
Err parse_append_ahref(BrowseCtx ctx[static 1], const char* url, size_t len);
#endif
