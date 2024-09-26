#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include "src/textbuf.h"
#include "src/error.h"
#include "src/url-client.h"
#include "src/utils.h"
#include "src/wrappers.h"


typedef struct UrlClient UrlClient;

typedef struct {
    lxb_dom_collection_t* hrefs;
} DocCache;

static inline void doc_cache_cleanup(DocCache c[static 1]) {
    lxb_dom_collection_destroy(c->hrefs, true);
    *c = (DocCache){0};
}

Err doc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);

typedef struct {
    const char* url;
    lxb_html_document_t* lxbdoc;
    TextBuf textbuf;
    DocCache cache;
} HtmlDoc;


void doc_reset(HtmlDoc html_doc[static 1]) ;
void doc_cleanup(HtmlDoc html_doc[static 1]) ;
void doc_destroy(HtmlDoc* html_doc) ;

int doc_init(HtmlDoc d[static 1], const Str url[static 1]);


HtmlDoc* doc_create(char* url);

static inline bool doc_has_url(HtmlDoc html_doc[static 1]) {
    return html_doc->url != NULL;
}

void doc_update_url(HtmlDoc html_doc[static 1], char* url) ;
Err doc_fetch(HtmlDoc html_doc[static 1], UrlClient url_client[static 1]);
bool doc_is_valid(HtmlDoc html_doc[static 1]);
#endif
