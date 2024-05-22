#include <aherror.h>
#include <curl-lxb.h>
#include <mem.h>
#include <wrappers.h>
#include <ahdoc.h>

// this is just a "random" bound. TODO: think it better
enum { ah_max_url_len = 1024 };

bool is_url_too_long(const char* url) {
    if (!url) { return true; }
    const char* p = url;
    for (; (p - url) < ah_max_url_len && *p; ++p)
        ;
    return (p - url) > ah_max_url_len;
}

const char* ah_urldup(const char* url) {
        if (!url || !*url || is_url_too_long(url)) {
                perror("url too long");
                return 0x0;
        }
        return ah_strndup(url, ah_max_url_len);
}

AhDoc* AhDocCreate(const char* url) {
    AhDoc* rv = ah_malloc(sizeof(AhDoc));
    if (!rv) { goto exit_fail; }
    url = ah_urldup(url);
    if (!url) {
        perror("urldup error");
        goto free_rv;
    }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        goto free_url;
    }
    *rv = (AhDoc) { .url=url, .doc=document };
    return rv;

free_url:
    ah_free((char*)url);
free_rv:
    ah_free(rv);
exit_fail:
    return 0x0;
}

void AhDocFree(AhDoc* ahdoc) {
    lxb_html_document_destroy(ahdoc->doc);
    ah_free((char*)ahdoc->url);
    ah_free(ahdoc);
}


int AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) {
    if (Ok != curl_lexbor_fetch_document(ahcurl, ad)) {
        perror("Error fetching doc");
        return 1;
    }
    return 0;
}

AhCtx* AhCtxCreate(const char* url, AhUserLineCallback callback) {
    AhCtx* rv = ah_malloc(sizeof(AhCtx));
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    AhCurl* ahcurl = AhCurlCreate();
    if (!ahcurl) { goto free_rv; }

    AhDoc* ahdoc = AhDocCreate(url);
    if (!ahdoc) { goto free_ahcurl; }

    if (curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEFUNCTION, chunk_callback)
        || curl_easy_setopt(ahcurl->curl, CURLOPT_WRITEDATA, ahdoc->doc)) {
        perror("Error configuring curl write fn/data"); goto free_ahdoc;
    }

    if (Ok != curl_lexbor_fetch_document(ahcurl, ahdoc)) {
        perror("Error fetching doc"); goto free_ahdoc;
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

