#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <lexbor/html/html.h>

#include "constants.h"
#include "textbuf.h"
#include "error.h"
#include "mem.h"
#include "url-client.h"
#include "utils.h"
#include "wrapper-lexbor.h"
#include "wrapper-lexbor-curl.h"
#include "doc-elem.h"
#include "url.h"
#include "session-conf.h"

#define HIDE_OL 0x1u
#define HIDE_UL 0x2u

_Static_assert(sizeof(size_t) >= 2, "size_t type too small");
_Static_assert(sizeof(size_t) >= sizeof(HIDE_OL), "size_t type too small");

static inline size_t strview_to_hide_tag(StrView s) { 
    if (!s.items || !s.len) return 0x0;
    if (strncmp("ol", s.items, s.len) == 0) return HIDE_OL;
    if (strncmp("ul", s.items, s.len) == 0) return HIDE_UL;
    return 0x0;
}

typedef lxb_dom_node_t* LxbNodePtr;
#define T LxbNodePtr
#include <arl.h>

typedef struct {
    TextBuf sourcebuf;
    TextBuf textbuf;
    Str screen;
    ArlOf(LxbNodePtr) anchors;
    ArlOf(LxbNodePtr) imgs;
    ArlOf(LxbNodePtr) inputs;
    ArlOf(LxbNodePtr) forms;
    LxbNodePtr title;
} DocCache;

typedef struct {
    Url url;
    HttpMethod method;
    lxb_html_document_t* lxbdoc;
    DocCache cache;
    size_t hide_tags;
} HtmlDoc;

/* getters */

static inline size_t* htmldoc_hide_tags(HtmlDoc d[static 1]) { return &d->hide_tags; }

static inline lxb_html_document_t*
htmldoc_lxbdoc(HtmlDoc d[static 1]) { return d->lxbdoc; }

static inline TextBuf*
htmldoc_sourcebuf(HtmlDoc d[static 1]) { return &d->cache.sourcebuf; }

static inline TextBuf*
htmldoc_textbuf(HtmlDoc d[static 1]) { return &d->cache.textbuf; }

static inline Str*
htmldoc_screen(HtmlDoc d[static 1]) { return &d->cache.screen; }

static inline ArlOf(LxbNodePtr)*
htmldoc_anchors(HtmlDoc d[static 1]) { return &d->cache.anchors; }

static inline ArlOf(LxbNodePtr)*
htmldoc_imgs(HtmlDoc d[static 1]) { return &d->cache.imgs; }

static inline ArlOf(LxbNodePtr)*
htmldoc_inputs(HtmlDoc d[static 1]) { return &d->cache.inputs; }

static inline ArlOf(LxbNodePtr)*
htmldoc_forms(HtmlDoc d[static 1]) { return &d->cache.forms; }

static inline LxbNodePtr*
htmldoc_title(HtmlDoc d[static 1]) { return &d->cache.title; }

static inline DocCache*
htmldoc_cache(HtmlDoc d[static 1]) { return &d->cache; }

static inline Url*
htmldoc_url(HtmlDoc d[static 1]) { return &d->url; }

static inline HttpMethod
htmldoc_method(HtmlDoc d[static 1]) { return d->method; }

static inline bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    ////TODO: remove, not needed since URLU
    return htmldoc && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

/* ctors */
Err htmldoc_init(HtmlDoc d[static 1], const char* url);
Err htmldoc_init_from_curlu(HtmlDoc d[static 1], CURLU* cu, HttpMethod method);

/* dtors */
void htmldoc_reset(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
//TODO: this fn should cleanup instead the draw data
static inline void htmldoc_cache_cleanup(HtmlDoc htmldoc[static 1]) {
    textbuf_cleanup(htmldoc_textbuf(htmldoc));
    textbuf_cleanup(htmldoc_sourcebuf(htmldoc));
    str_clean(htmldoc_screen(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_anchors(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_imgs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_inputs(htmldoc));
    arlfn(LxbNodePtr,clean)(htmldoc_forms(htmldoc));
    *htmldoc_cache(htmldoc) = (DocCache){0};
}


/* external */
Err curl_lexbor_fetch_document(
    UrlClient url_client[static 1], HtmlDoc htmldoc[static 1], SessionWriteFn wcb
);

/**/

Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);


HtmlDoc* htmldoc_create(const char* url);


Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) ;

static inline Err
htmldoc_fetch(
    HtmlDoc htmldoc[static 1], UrlClient url_client[static 1], SessionWriteFn wfnc
) {
        return curl_lexbor_fetch_document(url_client, htmldoc, wfnc);
}


#define serialize_lit_str(LitStr, CallBack, Context) \
 ((LXB_STATUS_OK != CallBack((lxb_char_t*)LitStr, sizeof(LitStr)-1, Context)) \
 ?  "error serializing literal string" : Ok)

#define serialize_literal_color_str(EscSeq, CallBack, Context) \
    (draw_ctx_color(Context) ? serialize_lit_str(EscSeq, CallBack, Context) : Ok)

/* htmldoc_tag_a.c */

Err htmldoc_A(Session* s, HtmlDoc d[static 1]) ;
Err htmldoc_print_info(Session* s, HtmlDoc d[static 1]) ;


static inline Err htmldoc_tags_str_reduce_size_t(const char* tags, size_t ts[static 1]) {
    Err err = "no tags to reduce";
    do {
        StrView t = cstr_split_word(&tags);
        if (!t.items || !t.len) return err;
        size_t hide_tag = strview_to_hide_tag(t);
        if (!hide_tag) return err_fmt("invalid tag: '%s'", t.items);
        *ts |= hide_tag;
        err = Ok;
    } while (1);
}


static inline Err lxb_mk_title_or_url(HtmlDoc d[static 1], char* url, Str title[static 1]) {
    Err err = Ok;
    lxb_dom_node_t* title_node;
    try( lexbor_get_title_node(htmldoc_lxbdoc(d), &title_node));
    if (title_node) err = lexbor_get_title_text_line(title_node, title);
    else if (!*url) err = "no title nor url";
    else err = str_append(title, url, strlen(url));

    return err;
}

static inline void textmod_trim_left(TextBufMods mods[static 1], size_t n) {
    if (n && len__(mods)) {
        foreach__(ModAt,it,mods) {
            if (it->offset < n) it->offset = 0;
            else it->offset -= n;
        }
    }
}

#endif
