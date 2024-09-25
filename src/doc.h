#ifndef __AH_DOC_AHRE_H__
#define __AH_DOC_AHRE_H__

#include <stdbool.h>
#include <errno.h>

#include <lexbor/html/html.h>

#include "src/textbuf.h"
#include "src/doc-cache.h"
#include "src/error.h"
#include "src/url-client.h"
#include "src/wrappers.h"


typedef struct UrlClient UrlClient;


typedef struct {
    const char* url;
    lxb_html_document_t* lxbdoc;
    TextBuf textbuf;
    DocCache cache;
} Doc;


void doc_reset(Doc doc[static 1]) ;
void doc_cleanup(Doc doc[static 1]) ;
void doc_destroy(Doc* doc) ;

int doc_init(Doc d[static 1], const Str url[static 1]);


Doc* doc_create(char* url);

static inline bool doc_has_url(Doc doc[static 1]) {
    return doc->url != NULL;
}

void doc_update_url(Doc doc[static 1], char* url) ;
Err doc_fetch(Doc doc[static 1], UrlClient url_client[static 1]);
bool doc_is_valid(Doc doc[static 1]);
#endif
