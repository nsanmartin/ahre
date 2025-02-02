#include "src/generic.h"
#include "src/session.h"


Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]) {
    TabList* f = session_tablist(session);
    HtmlDoc* d;
    try( tablist_current_doc(f, &d));
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

    TabList f = (TabList){0};
    if (url) {
        Err err = tablist_init(&f, url, url_client);
        if (err) {
            tablist_cleanup(&f);
            f = (TabList){0};
            puts(err);
        }
    }

    *s = (Session) {
        .url_client         = url_client,
        .user_line_callback = callback,
        .tablist            = f,
        .quit               = false,
        .conf               = mkSessionConf
    };

    return Ok;
}


void session_destroy(Session* session) {
    session_cleanup(session);
    std_free(session);
}


