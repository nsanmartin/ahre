#include <ah/mem.h>
#include <ah/session.h>


inline Doc* session_current_doc(Session session[static 1]) {
    return session->ahdoc;
}

inline TextBuf* session_current_buf(Session session[static 1]) {
    return &session_current_doc(session)->aebuf;
}


Session* session_create(char* url, UserLineCallback callback) {
    Session* rv = std_malloc(sizeof(Session));
    *rv = (Session){0};
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    UrlClient* url_client = url_client_create();
    if (!url_client) { goto free_rv; }

    Doc* ahdoc = doc_create(url);
    if (!ahdoc) { goto free_ahcurl; }

    if (url && ahdoc->doc) {
        ErrStr err = doc_fetch(url_client, ahdoc);
        if (err) {
            ah_log_error(err, ErrCurl);
            goto free_ahdoc;
        }
    }
    *rv = (Session) {
        .url_client=url_client,
        .user_line_callback=callback,
        .ahdoc=ahdoc,
        .quit=false
    };

    return rv;

free_ahdoc:
    doc_destroy(ahdoc);
free_ahcurl:
    url_client_destroy(url_client);
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

void session_destroy(Session* ah) {
    doc_destroy(ah->ahdoc);
    url_client_destroy(ah->url_client);
    destroy(ah);
}


