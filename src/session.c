#include <ah/mem.h>
#include <ah/session.h>


inline Doc* session_current_doc(Session session[static 1]) {
    return session->doc;
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

    Doc* doc = doc_create(url);
    if (!doc) { goto free_ahcurl; }

    if (url && doc->lxbdoc) {
        ErrStr err = doc_fetch(url_client, doc);
        if (err) {
            ah_log_error(err, ErrCurl);
            //goto free_ahdoc;
        }
    }
    *rv = (Session) {
        .url_client=url_client,
        .user_line_callback=callback,
        .doc=doc,
        .quit=false
    };

    return rv;

///free_ahdoc:
///    doc_destroy(doc);
free_ahcurl:
    url_client_destroy(url_client);
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

void session_destroy(Session* session) {
    doc_destroy(session->doc);
    url_client_destroy(session->url_client);
    std_free(session);
}


