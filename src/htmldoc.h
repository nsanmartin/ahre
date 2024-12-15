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


typedef struct {
    lxb_dom_collection_t* hrefs;
} DocCache;


typedef struct {
    const char* url;
    lxb_html_document_t* lxbdoc;
    TextBuf textbuf;
    BufOf(char) sourcebuf;
    DocCache cache;
    ArlOf(Ahref) ahrefs;
    ArlOf(Img) imgs;
} HtmlDoc;

/* getters */
static inline const char*
htmldoc_url(HtmlDoc d[static 1]) { return d->url; }

static inline lxb_html_document_t*
htmldoc_lxbdoc(HtmlDoc d[static 1]) { return d->lxbdoc; }

static inline TextBuf*
htmldoc_textbuf(HtmlDoc d[static 1]) { return &d->textbuf; }

static inline ArlOf(Ahref)*
htmldoc_ahrefs(HtmlDoc d[static 1]) { return &d->ahrefs; }

static inline ArlOf(Img)*
htmldoc_imgs(HtmlDoc d[static 1]) { return &d->imgs; }

/* ctors */
int htmldoc_init(HtmlDoc d[static 1], const char* url);

/* dtors */
//void htmldoc_reset(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;
static inline void htmldoc_cache_cleanup(DocCache c[static 1]) {
    lxb_dom_collection_destroy(c->hrefs, true);
    *c = (DocCache){0};
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
    if (file_exists(htmldoc->url)) {
        return lexbor_read_doc_from_file(htmldoc);
    }
    return curl_lexbor_fetch_document(url_client, htmldoc);
}

Err htmldoc_browse(HtmlDoc htmldoc[static 1]);

static inline const char* htmldoc_abs_url_dup(const HtmlDoc htmldoc[static 1], const char* url) {
    if (cstr_starts_with(url, "//"))
        return cstr_cat_dup("https://", url); ///TODO: suppoert for http
    else if (cstr_starts_with(url, "/"))
        return cstr_cat_dup(htmldoc->url, url);
    return strdup(url);
}
#endif
