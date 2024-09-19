#ifndef __AH_DOC_CACHE_AHRE_H__
#define __AH_DOC_CACHE_AHRE_H__
#include <lexbor/html/html.h>
#include <ah/utils.h>

typedef struct {
    lxb_dom_collection_t* hrefs;
} DocCache;

static inline void doc_cache_cleanup(DocCache* c) {
    lxb_dom_collection_destroy(c->hrefs, true);
    *c = (DocCache){0};
}

int ahdoc_cache_buffer_summary(DocCache c[static 1], BufOf(char)* buf) ;
#endif
