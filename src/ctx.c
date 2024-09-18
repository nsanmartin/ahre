#include <ah/ctx.h>


inline AhDoc* AhCtxCurrentDoc(AhCtx ctx[static 1]) {
    return ctx->ahdoc;
}

inline AhBuf* AhCtxCurrentBuf(AhCtx ctx[static 1]) {
    return &AhCtxCurrentDoc(ctx)->aebuf;
}


AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback) {
    AhCtx* rv = ah_malloc(sizeof(AhCtx));
    *rv = (AhCtx){0};
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    AhCurl* ahcurl = AhCurlCreate();
    if (!ahcurl) { goto free_rv; }

    AhDoc* ahdoc = AhDocCreate(url);
    if (!ahdoc) { goto free_ahcurl; }

    if (url && ahdoc->doc) {
        ErrStr err = AhDocFetch(ahcurl, ahdoc);
        if (err) {
            ah_log_error(err, ErrCurl);
            goto free_ahdoc;
        }
    }
    *rv = (AhCtx) {
        .ahcurl=ahcurl,
        .user_line_callback=callback,
        .ahdoc=ahdoc,
        .quit=false
    };

    return rv;

free_ahdoc:
    AhDocFree(ahdoc);
free_ahcurl:
    AhCurlFree(ahcurl);
free_rv:
    ah_free(rv);
exit_fail:
    return 0x0;
}

void AhCtxFree(AhCtx* ah) {
    AhDocFree(ah->ahdoc);
    AhCurlFree(ah->ahcurl);
    ah_free(ah);
}


/* debug function */
int AhCtxBufSummary(AhCtx ctx[static 1]) {
    BufOf(char)* buf = &AhCtxCurrentBuf(ctx)->buf;
    if (ctx->ahcurl) {
        if(AhCurlBufSummary(ctx->ahcurl, buf)) { return -1; }
    } else { buf_append_lit("Not ahcurl\n", buf); }

    if (ctx->ahdoc) {
        AhDocBufSummary(ctx->ahdoc, buf);
        ahdoc_cache_buffer_summary(&ctx->ahdoc->cache, buf);
    } else { buf_append_lit("Not ahdoc\n", buf); }

    return 0;
}
