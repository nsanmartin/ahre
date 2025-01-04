#include "src/generic.h"
#include "src/session.h"


inline HtmlDoc* session_current_doc(Session session[static 1]) {
    return session->htmldoc;
}

inline TextBuf* session_current_buf(Session session[static 1]) {
    return htmldoc_textbuf(session_current_doc(session));
}


//Err session_init(Session s[static 1],char* url, UserLineCallback callback) { }
Session* session_create(char* url, UserLineCallback callback) {
    Session* rv = std_malloc(sizeof(Session));
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    *rv = (Session){0};
    UrlClient* url_client = url_client_create();
    if (!url_client) { goto free_rv; }

    HtmlDoc* htmldoc = htmldoc_create(cstr_trim_space((char*)url));
    if (!htmldoc) { goto free_ahcurl; }

    if (htmldoc_lxbdoc(htmldoc) && url_cu(htmldoc_url(htmldoc))) {
        Err err = htmldoc_fetch(htmldoc, url_client);
        if (err) { log_error(err); }
        err = htmldoc_browse(htmldoc);
        if (err) { log_error(err); }
    }
    *rv = (Session) {
        .url_client=url_client,
        .user_line_callback=callback,
        .htmldoc=htmldoc,
        .quit=false
    };

    return rv;

free_ahcurl:
    url_client_destroy(url_client);
free_rv:
    std_free(rv);
exit_fail:
    return 0x0;
}

void session_destroy(Session* session) {
    htmldoc_destroy(session->htmldoc);
    url_client_destroy(session->url_client);
    std_free(session);
}


