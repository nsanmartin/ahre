#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>
#include <unistd.h>


#include "constants.h"
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
#include "fetch-history.h"
#include "cmd-out.h"


_Static_assert(sizeof(size_t) >= 2, "size_t type too small");


typedef struct {
    TextBuf sourcebuf;
    ArlOf(Str) head_scripts;
    ArlOf(Str) body_scripts;
    uintmax_t  curlinfo_sz_download; /* each download will be updated */
} DocFetchCache;


typedef struct {
    TextBuf        textbuf;
    Str            screen;
    DomNode        title;
    ArlOf(DomNode) anchors;
    ArlOf(DomNode) imgs;
    ArlOf(DomNode) inputs;
    ArlOf(DomNode) forms;
} DocDrawCache;


typedef struct {
    Str charset;
    Str content_type;
} HttpHeader;

static inline void http_header_clean(HttpHeader hh[_1_]) {
    str_clean(&hh->charset);
    str_clean(&hh->content_type);
}



typedef struct HtmlDoc {
    Request              req;
    Dom                  dom;
    DocFetchCache        fetch_cache;
    DocDrawCache         draw_cache;
    HttpHeader           http_header;
    JsEngine             jsdoc;
} HtmlDoc;

/* getters */
static inline ArlOf(DomNode)* htmldoc_anchors(HtmlDoc d[_1_]) { return &d->draw_cache.anchors; }
static inline ArlOf(DomNode)* htmldoc_forms(HtmlDoc d[_1_]) { return &d->draw_cache.forms; }
static inline ArlOf(DomNode)* htmldoc_imgs(HtmlDoc d[_1_]) { return &d->draw_cache.imgs; }
static inline ArlOf(DomNode)* htmldoc_inputs(HtmlDoc d[_1_]) { return &d->draw_cache.inputs; }
static inline ArlOf(Str)* htmldoc_body_scripts(HtmlDoc d[_1_]) { return &d->fetch_cache.body_scripts; }
static inline ArlOf(Str)* htmldoc_head_scripts(HtmlDoc d[_1_]) { return &d->fetch_cache.head_scripts; }
static inline Dom htmldoc_dom(HtmlDoc d[_1_]) { return d->dom; }
static inline Dom* htmldoc_dom_ptr(HtmlDoc d[_1_]) { return &d->dom; }
static inline DomNode* htmldoc_title(HtmlDoc d[_1_]) { return &d->draw_cache.title; }
static inline HttpHeader* htmldoc_http_header(HtmlDoc h[_1_]) { return &h->http_header; }
static inline HttpMethod htmldoc_method(HtmlDoc d[_1_]) { return d->req.method; }
static inline JsEngine* htmldoc_js(HtmlDoc d[_1_]) { return &d->jsdoc; }
static inline Request* htmldoc_request(HtmlDoc h[_1_]) { return &h->req; }
static inline Str* htmldoc_http_charset(HtmlDoc h[_1_]) { return &htmldoc_http_header(h)->charset; }
static inline Str* htmldoc_http_content_type(HtmlDoc h[_1_]) { return &htmldoc_http_header(h)->content_type; }
static inline Str* htmldoc_screen(HtmlDoc d[_1_]) { return &d->draw_cache.screen; }
static inline TextBuf* htmldoc_sourcebuf(HtmlDoc d[_1_]) { return &d->fetch_cache.sourcebuf; }
static inline TextBuf* htmldoc_textbuf(HtmlDoc d[_1_]) { return &d->draw_cache.textbuf; }
static inline Url* htmldoc_url(HtmlDoc d[_1_]) { return &d->req.url; }
static inline uintmax_t* htmldoc_curlinfo_sz_download(HtmlDoc d[_1_]) { return &d->fetch_cache.curlinfo_sz_download; }

static inline bool htmldoc_js_is_enabled(HtmlDoc d[_1_]) { return jse_is_enabled(htmldoc_js(d)); }
static inline void htmldoc_js_disable(HtmlDoc d[_1_]) { jse_clean(htmldoc_js(d)); }




/* dtors */
void htmldoc_reset_draw(HtmlDoc htmldoc[_1_]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[_1_]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
//TODO: this fn should cleanup instead the draw data
void htmldoc_cache_cleanup(HtmlDoc htmldoc[_1_]) ;



Err htmldoc_draw_with_flags(HtmlDoc htmldoc[_1_], Session* s, unsigned flags);
Err htmldoc_A(Session* s, HtmlDoc d[_1_], CmdOut* out);
Err htmldoc_console(HtmlDoc d[_1_], Session* s, const char* line, CmdOut* out);
Err htmldoc_convert_sourcebuf_to_utf8(HtmlDoc d[_1_]);
Err htmldoc_fetch(HtmlDoc htmldoc[_1_], UrlClient url_client[_1_], CmdOut cmd_out[_1_], FetchHistoryEntry he[_1_]);
Err htmldoc_init_bookmark_move_urlstr(HtmlDoc d[_1_], Str urlstr[_1_]);
Err htmldoc_init_move_request(HtmlDoc d[_1_], Request r[_1_], UrlClient uc[_1_], Session* s, CmdOut* out);
Err htmldoc_input_at(HtmlDoc d[_1_], size_t ix, DomNode out[_1_]);
Err htmldoc_js_enable(HtmlDoc d[_1_], Session* s, CmdOut* out);
Err htmldoc_print_info(HtmlDoc d[_1_], CmdOut* out);
Err htmldoc_script_at(HtmlDoc d[_1_], size_t ix, Str* sptr[_1_]);
Err htmldoc_scripts_write(HtmlDoc h[_1_], RangeParse rp[_1_], Writer w[_1_]);
Err htmldoc_switch_js(HtmlDoc htmldoc[_1_], Session* s, CmdOut* out);
Err htmldoc_title_or_url(HtmlDoc d[_1_], char* url, Str title[_1_]);
Err lexbor_read_doc_from_file(HtmlDoc htmldoc[_1_]) ;
bool htmldoc_http_charset_is_utf8(HtmlDoc d[_1_]);
bool htmldoc_http_content_type_text_or_undef(HtmlDoc d[_1_]);
void htmldoc_eval_js_scripts_or_continue(HtmlDoc d[_1_], Session* s, CmdOut* out);
void textmod_trim_left(TextBufMods mods[_1_], size_t n);

#endif
