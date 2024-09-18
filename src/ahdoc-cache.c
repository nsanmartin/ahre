#include <ah/doc-cache.h>
#include <ah/wrappers.h>

int ahdoc_cache_buffer_summary(AhDocCache c[static 1], BufOf(char)* buf) {
    buf_append_lit("AhDocCache: ", buf);
    if (buf_append_hexp(c, buf)) { return -1; }
    buf_append_lit("\n", buf);

    if (c->hrefs) {
        buf_append_lit(" hrefs: ", buf);
        if (buf_append_hexp(c->hrefs, buf)) { return -1; }
        buf_append_lit("\n", buf);

        if (c->hrefs) {
            if(lexbor_foreach_href(c->hrefs, ahre_append_href, buf)) {
                return -1;
            }
        }
    } else { buf_append_lit(" no hrefs\n", buf); }

    return 0;
}


