#include <ah/session.h>


inline Doc* AhCtxCurrentDoc(Session session[static 1]) {
    return session->ahdoc;
}

inline TextBuf* AhCtxCurrentBuf(Session session[static 1]) {
    return &AhCtxCurrentDoc(session)->aebuf;
}


Session* AhCtxCreate(char* url, AhUserLineCallback callback) {
    Session* rv = ah_malloc(sizeof(Session));
    *rv = (Session){0};
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    UrlClient* ahcurl = url_client_create();
    if (!ahcurl) { goto free_rv; }

    Doc* ahdoc = doc_create(url);
    if (!ahdoc) { goto free_ahcurl; }

    if (url && ahdoc->doc) {
        ErrStr err = doc_fetch(ahcurl, ahdoc);
        if (err) {
            ah_log_error(err, ErrCurl);
            goto free_ahdoc;
        }
    }
    *rv = (Session) {
        .ahcurl=ahcurl,
        .user_line_callback=callback,
        .ahdoc=ahdoc,
        .quit=false
    };

    return rv;

free_ahdoc:
    doc_destroy(ahdoc);
free_ahcurl:
    url_client_destroy(ahcurl);
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

void AhCtxFree(Session* ah) {
    doc_destroy(ah->ahdoc);
    url_client_destroy(ah->ahcurl);
    destroy(ah);
}


/* debug function */
int AhCtxBufSummary(Session session[static 1]) {
    BufOf(char)* buf = &AhCtxCurrentBuf(session)->buf;
    if (session->ahcurl) {
        if(AhCurlBufSummary(session->ahcurl, buf)) { return -1; }
    } else { buf_append_lit("Not ahcurl\n", buf); }

    if (session->ahdoc) {
        AhDocBufSummary(session->ahdoc, buf);
        ahdoc_cache_buffer_summary(&session->ahdoc->cache, buf);
    } else { buf_append_lit("Not ahdoc\n", buf); }

    return 0;
}
