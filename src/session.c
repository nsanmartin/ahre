#include "src/generic.h"
#include "src/session.h"


inline HtmlDoc* session_current_doc(Session session[static 1]) {
    return session->html_doc;
}

inline TextBuf* session_current_buf(Session session[static 1]) {
    return &session_current_doc(session)->textbuf;
}


Session* session_create(char* url, UserLineCallback callback) {
    Session* rv = std_malloc(sizeof(Session));
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    *rv = (Session){0};
    UrlClient* url_client = url_client_create();
    if (!url_client) { goto free_rv; }

    HtmlDoc* html_doc = doc_create(cstr_trim_space((char*)url));
    if (!html_doc) { goto free_ahcurl; }

    if (url && html_doc->lxbdoc) {
        Err err = doc_fetch(html_doc, url_client);
        if (err) { log_error(err); }
    }
    *rv = (Session) {
        .url_client=url_client,
        .user_line_callback=callback,
        .html_doc=html_doc,
        .quit=false
    };

    return rv;

free_ahcurl:
    url_client_destroy(url_client);
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

void session_destroy(Session* session) {
    doc_destroy(session->html_doc);
    url_client_destroy(session->url_client);
    std_free(session);
}


