#include "src/generic.h"
#include "src/session.h"


Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]) {
    //return session->htmldoc;
    //(gdb) p session->htmldoc_forest .trees .items[0].head.doc.cache.textbuf

    HtmlDocForest* f = session_htmldoc_forest(session);
    HtmlDoc* d;
    try( htmldoc_forest_current_doc(f, &d));
    *out = d;
    return Ok;

}

Err session_current_buf(Session session[static 1], TextBuf* out[static 1]) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    *out = htmldoc_textbuf(d);
    return Ok;
}


Err session_init(Session s[static 1], char* url, UserLineCallback callback) {
    *s = (Session){0};
    UrlClient* url_client = url_client_create();
    if (!url_client) { return "error:  url_client_create failure"; }

    HtmlDocForest f = (HtmlDocForest){0};
    Err err = htmldoc_forest_init(&f, url, url_client);
    if (err) {
        htmldoc_forest_cleanup(&f);
        f = (HtmlDocForest){0};
        puts(err);
    }

    *s = (Session) {
        .url_client         = url_client,
        .user_line_callback = callback,
        .htmldoc_forest     = f,
        .quit               = false,
        .conf               = mkSessionConf
    };

    return Ok;
}


void session_destroy(Session* session) {
    session_cleanup(session);
    std_free(session);
}


