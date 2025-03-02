#include "src/generic.h"
#include "src/session.h"
#include "src/user-interface.h"



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


Err session_init(Session s[static 1], char* url) {
    *s = (Session){0};
    UrlClient* url_client = url_client_create();
    if (!url_client) { return "error:  url_client_create failure"; }

    size_t nrows, ncols;
    Err err = ui_get_win_size(&nrows, &ncols);

    TabList f = (TabList){0};
    if (url) ok_then(err,tablist_init(&f, url, url_client, session_conf(s)));

    if (err) {
        tablist_cleanup(&f);
        return err;
    }

    *s = (Session) {
        .url_client   = url_client,
        .tablist      = f,
        .conf         = (SessionConf){
            .uiin        = uiin_isocline,
            .ncols       = 90 > ncols ? ncols : 90,
            .nrows       = nrows
        }
    };

    return Ok;
}


void session_destroy(Session* session) {
    session_cleanup(session);
    std_free(session);
}


