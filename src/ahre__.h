#ifndef AHRE_AHRE___H__
#define AHRE_AHRE___H__

Err htmldoc_init_fetch_draw_from_curlu(
    HtmlDoc d[static 1],
    CURLU* cu,
    UrlClient url_client[static 1],
    HttpMethod method,
    Session s[static 1]
);

Err htmldoc_draw(HtmlDoc htmldoc[static 1], Session s[static 1]);
#endif
