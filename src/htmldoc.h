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


typedef struct {
    lxb_dom_collection_t* hrefs;
} DocCache;

static inline void htmldoc_cache_cleanup(DocCache c[static 1]) {
    lxb_dom_collection_destroy(c->hrefs, true);
    *c = (DocCache){0};
}

Err htmldoc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);

typedef struct {
    const char* url;
    lxb_html_document_t* lxbdoc;
    TextBuf textbuf;
    DocCache cache;
} HtmlDoc;


static inline lxb_html_document_t* htmldoc_lxbdoc(HtmlDoc d[static 1]) {
    return d->lxbdoc;
}

void htmldoc_reset(HtmlDoc htmldoc[static 1]) ;
void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) ;
void htmldoc_destroy(HtmlDoc* htmldoc) ;

int htmldoc_init(HtmlDoc d[static 1], const Str url[static 1]);


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
    return curl_lexbor_fetch_document(url_client, htmldoc->lxbdoc, htmldoc->url);
}

Err htmldoc_browse(HtmlDoc htmldoc[static 1]);
Err htmldoc_browse0(HtmlDoc htmldoc[static 1]) ;

#endif
