#include <mem.h>
#include <wrappers.h>
#include <ah-doc.h>

AhCtx* AhCtxMalloc(const char* url, AhUserLineCallback callback) {
    const size_t max_url_len = 1024;
    const char* p = url;
    for (size_t len = 0; p && max_url_len && *p; ++p, ++len) {
        if (max_url_len <= len) {
            perror("Url too large");
            goto exit_fail;
        }
    }
    const char* urldup = ah_strndup(url, max_url_len);
    if (!urldup) goto exit_fail;
    AhCtx* rv = ah_malloc(sizeof(AhCtx));
    if (!rv) { goto free_url; }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Failed to create HTML Document");
        goto free_ctx;
    }
    if (lexbor_fetch_document(document, url)) {
        perror("Failed to create HTML Document");
        goto destroy_doc;
    }
    *rv = (AhCtx) {
        .url=urldup,
        .user_line_callback=callback,
        .doc=document,
        .fetch=true,
        .quit=false
    };

    return rv;

destroy_doc:
    lxb_html_document_destroy(document);
free_ctx:
    ah_free(rv);
free_url:
    ah_free((char*)urldup);
exit_fail:
    return 0x0;
}

void AhCtxFree(AhCtx* ctx) {
    lxb_html_document_destroy(ctx->doc);
    ah_free((char*)ctx->url);
    ah_free(ctx);
}
