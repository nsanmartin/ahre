#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <lexbor/html/html.h>

#include "constants.h"
#include "doc-elem.h"
#include "error.h"
#include "mem.h"
#include "session-conf.h"
#include "textbuf.h"
#include "url-client.h"
#include "url.h"
#include "utils.h"
#include "wrapper-lexbor-curl.h"
#include "wrapper-lexbor.h"
#include "js-engine.h"

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
    ArlOf(Str) head_scripts;
    ArlOf(Str) body_scripts;
} DocFetchCache;


typedef struct {
    TextBuf           textbuf;
    Str               screen;
    LxbNodePtr        title;
    ArlOf(LxbNodePtr) anchors;
    ArlOf(LxbNodePtr) imgs;
    ArlOf(LxbNodePtr) inputs;
    ArlOf(LxbNodePtr) forms;
} DocDrawCache;


typedef struct {
    Str charset;
    Str content_type;
} HttpHeader;

static inline void http_header_clean(HttpHeader hh[static 1]) {
    str_clean(&hh->charset);
    str_clean(&hh->content_type);
}



typedef struct HtmlDoc {
    Url                  url;
    HttpMethod           method;
    lxb_html_document_t* lxbdoc;
    DocFetchCache        fetch_cache;
    DocDrawCache         draw_cache;
    size_t               hide_tags;
    HttpHeader           http_header;
    JsEngine             jsdoc;
} HtmlDoc;

/* getters */
static inline HttpHeader* htmldoc_http_header(HtmlDoc h[static 1]) { return &h->http_header; }
static inline Str*
htmldoc_http_content_type(HtmlDoc h[static 1]) { return &htmldoc_http_header(h)->content_type; }
static inline Str*
htmldoc_http_charset(HtmlDoc h[static 1]) { return &htmldoc_http_header(h)->charset; }
static inline size_t* htmldoc_hide_tags(HtmlDoc d[static 1]) { return &d->hide_tags; }
static inline lxb_html_document_t* htmldoc_lxbdoc(HtmlDoc d[static 1]) { return d->lxbdoc; }
static inline TextBuf* htmldoc_sourcebuf(HtmlDoc d[static 1]) { return &d->fetch_cache.sourcebuf; }
static inline TextBuf* htmldoc_textbuf(HtmlDoc d[static 1]) { return &d->draw_cache.textbuf; }
static inline Str* htmldoc_screen(HtmlDoc d[static 1]) { return &d->draw_cache.screen; }
static inline ArlOf(LxbNodePtr)*
htmldoc_anchors(HtmlDoc d[static 1]) { return &d->draw_cache.anchors; }
static inline ArlOf(LxbNodePtr)* htmldoc_imgs(HtmlDoc d[static 1]) { return &d->draw_cache.imgs; }
static inline ArlOf(LxbNodePtr)* htmldoc_inputs(HtmlDoc d[static 1]) { return &d->draw_cache.inputs; }
static inline ArlOf(LxbNodePtr)* htmldoc_forms(HtmlDoc d[static 1]) { return &d->draw_cache.forms; }

static inline ArlOf(Str)* htmldoc_head_scripts(HtmlDoc d[static 1]) {
    return &d->fetch_cache.head_scripts;
}

static inline ArlOf(Str)* htmldoc_body_scripts(HtmlDoc d[static 1]) {
    return &d->fetch_cache.body_scripts;
}

static inline LxbNodePtr* htmldoc_title(HtmlDoc d[static 1]) { return &d->draw_cache.title; }
static inline Url* htmldoc_url(HtmlDoc d[static 1]) { return &d->url; }
static inline HttpMethod htmldoc_method(HtmlDoc d[static 1]) { return d->method; }
static inline JsEngine* htmldoc_js(HtmlDoc d[static 1]) { return &d->jsdoc; }

static inline bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    ////TODO: remove, not needed since URLU
    return htmldoc && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

static inline bool htmldoc_http_charset_is_utf8(HtmlDoc d[static 1]) {
    char* from_charset = items__(htmldoc_http_charset(d));
    return (!from_charset || !strcasecmp(from_charset, "UTF-8"));
}

#define TXT_ "text/"
static inline bool htmldoc_http_content_type_text_or_undef(HtmlDoc d[static 1]) {
    Str* content_type = htmldoc_http_content_type(d);
    const size_t len = lit_len__(TXT_);
    const size_t ctlen = len__(content_type);
    return !ctlen || (ctlen > len && !strncmp(items__(content_type), TXT_, len));
}

Err htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[static 1]);

/* ctors */
Err htmldoc_init(
    HtmlDoc     d[static 1],
    const char* urlstr,
    Url*        url,
    HttpMethod  method,
    unsigned    flags
);



/* dtors */
void htmldoc_reset_draw(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
//TODO: this fn should cleanup instead the draw data
void htmldoc_cache_cleanup(HtmlDoc htmldoc[static 1]) ;


/* external */
Err curl_lexbor_fetch_document(
    UrlClient url_client[static 1], HtmlDoc htmldoc[static 1], SessionWriteFn wcb, CurlLxbFetchCb cb
);

static inline bool htmldoc_js_is_enabled(HtmlDoc d[static 1]) {
    return jse_is_enabled(htmldoc_js(d));
}

static inline void htmldoc_js_disable(HtmlDoc d[static 1]) { jse_clean(htmldoc_js(d)); }
Err htmldoc_console(HtmlDoc d[static 1], Session* s, const char* line);
Err htmldoc_js_enable(HtmlDoc d[static 1], Session* s);
/**/

// Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);


Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) ;


static inline Err htmldoc_fetch(
    HtmlDoc        htmldoc[static 1],
    UrlClient      url_client[static 1],
    SessionWriteFn wfnc,
    CurlLxbFetchCb cb
) {
    return curl_lexbor_fetch_document(url_client, htmldoc, wfnc, cb);
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

static inline Err htmldoc_switch_js(HtmlDoc htmldoc[static 1], Session* s) {
    if (!s) return "error: NULL session";
    JsEngine* js = htmldoc_js(htmldoc);
    bool is_enabled = jse_rt(js);
    if (is_enabled) jse_clean(js);
    else try( htmldoc_js_enable(htmldoc, s));//TODO:REDRAW!
   
    return Ok;
}


void htmldoc_eval_js_scripts_or_continue(HtmlDoc d[static 1], Session* s);

#endif
