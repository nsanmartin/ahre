#include "src/doc-cache.h"
#include "src/wrappers.h"

Err doc_cache_buffer_summary(DocCache c[static 1], BufOf(char) buf[static 1]) {
    Err err = Ok;
    buf_append_lit("DocCache: ", buf);
    if (buf_append_hexp(c, buf)) { return "could nor append"; }
    buf_append_lit("\n", buf);

    if (c->hrefs) {
        buf_append_lit(" hrefs: ", buf);
        if (buf_append_hexp(c->hrefs, buf)) { return  "could nor append"; }
        buf_append_lit("\n", buf);

        if (c->hrefs) {
            if((err=lexbor_foreach_href(c->hrefs, ahre_append_href, buf))) {
                return err; ///lexbod foreach href";
            }
        }
    } else { buf_append_lit(" no hrefs\n", buf); }

    return err;
}


