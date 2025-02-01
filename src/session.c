#include "src/generic.h"
#include "src/session.h"


HtmlDoc* session_current_doc(Session session[static 1]) {
    return session->htmldoc;
}

TextBuf* session_current_buf(Session session[static 1]) {
    return htmldoc_textbuf(session_current_doc(session));
}


Err session_init(Session s[static 1], char* url, UserLineCallback callback) {
    *s = (Session){0};
    UrlClient* url_client = url_client_create();
    if (!url_client) { return "error:  url_client_create failure"; }

    HtmlDoc* htmldoc = htmldoc_create(cstr_trim_space((char*)url));
    if (!htmldoc) {
        url_client_destroy(url_client);
        return "error: htmldoc_create failure";
    }

    HtmlDocForest f = (HtmlDocForest){0};

    if (htmldoc_lxbdoc(htmldoc) && url_cu(htmldoc_url(htmldoc))) {
        Err err = htmldoc_fetch(htmldoc, url_client);
        if (err) { log_error(err); }
        err = htmldoc_browse(htmldoc);
        if (err) { log_error(err); }
        //// ><M
        try( htmldoc_forest_init(&f, url));
            
    }
    *s = (Session) {
        .url_client=url_client,
        .user_line_callback=callback,
        .htmldoc=htmldoc,
        .htmldoc_forest=f,
        .quit=false,
        .conf=mkSessionConf
    };

    return Ok;
}

Session* session_create(char* url, UserLineCallback callback) {
    Session* rv = std_malloc(sizeof(Session));
    if (!rv) {
        perror("Mem error");
        return NULL;
    }
    Err err = session_init(rv, url, callback);
    if (err) {
        puts(err);
        return NULL;
    }
    return rv;
}

void session_destroy(Session* session) {
    htmldoc_destroy(session->htmldoc);
    url_client_destroy(session->url_client);
    htmldoc_forest_cleanup(session_htmldoc_forest(session));
    std_free(session);
}


