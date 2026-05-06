#include "draw.h"


StrView
draw_ctx_url_strview(DrawCtx ctx[_1_]) {
    return sv(request_urlstr(htmldoc_request(draw_ctx_htmldoc(ctx))));
}
