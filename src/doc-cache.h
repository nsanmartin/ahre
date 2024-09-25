#ifndef __AH_DOC_CACHE_AHRE_H__
#define __AH_DOC_CACHE_AHRE_H__
#include <lexbor/html/html.h>
#include "src/utils.h"

typedef struct {
    lxb_dom_collection_t* hrefs;
} DocCache;

static inline void doc_cache_cleanup(DocCache c[static 1]) {
    lxb_dom_collection_destroy(c->hrefs, true);
    *c = (DocCache){0};
}

Err doc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]);
#endif
