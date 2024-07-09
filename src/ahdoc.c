#include <unistd.h>
#include <aherror.h>
#include <curl-lxb.h>
#include <mem.h>
#include <wrappers.h>

#include <ahutils.h>
#include <ahdoc.h>

// this is just a "random" bound. TODO: think it better
enum { ah_max_url_len = 1024, ah_initial_buf_sz };


ErrStr lexbor_read_doc_from_url_or_file (AhCurl ahcurl[static 1], AhDoc ad[static 1]); 



bool is_url_too_long(const char* url) {
    if (!url) { return true; }
    const char* p = url;
    for (; (p - url) < ah_max_url_len && *p; ++p)
        ;
    return (p - url) > ah_max_url_len;
}

char* ah_urldup(char* url) {
	if (!url) { return 0x0; }
        if (!*url || is_url_too_long(url)) {
                perror("url too long");
                return 0x0;
        }
        return ah_strndup(url, ah_max_url_len);
}

//int AhBufInit(AhBuf buf[static 1]) {
//    char* buffer = ah_malloc(ah_initial_buf_sz);
//    if (!buffer) { return 1; }
//    *buf = (AhBuf) {
//        .data=buffer,
//        .len=ah_initial_buf_sz
//    };
//    return 0;
//}

AhDoc* AhDocCreate(char* url) {
    AhDoc* rv = ah_malloc(sizeof(AhDoc));
    if (!rv) { goto exit_fail; }
    url = ah_urldup(url);
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        goto free_url;
    }
    *rv = (AhDoc) { .url=url, .doc=document, .buf={0} };
    //if (AhBufInit(&rv->buf)) { goto free_doc; };
    return rv;

//free_doc:
//    lxb_html_document_destroy(document);
free_url:
    ah_free((char*)url);
    ah_free(rv);
exit_fail:
    return 0x0;
}

void AhDocFree(AhDoc* ahdoc) {
    lxb_html_document_destroy(ahdoc->doc);
    ah_free((char*)ahdoc->url);
    ah_free(ahdoc);
}


ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) {
    return lexbor_read_doc_from_url_or_file (ahcurl, ad);
}

AhCtx* AhCtxCreate(char* url, AhUserLineCallback callback) {
    AhCtx* rv = ah_malloc(sizeof(AhCtx));
    if (!rv) {
        perror("Mem error");
        goto exit_fail;
    }
    AhCurl* ahcurl = AhCurlCreate();
    if (!ahcurl) { goto free_rv; }

    AhDoc* ahdoc = AhDocCreate(url);
    if (!ahdoc) { goto free_ahcurl; }

    if (url) {
        ErrStr err = AhDocFetch(ahcurl, ahdoc);
        if (err) {
            ah_log_error(err, ErrCurl);
            goto free_ahcurl;
        }
    }
    *rv = (AhCtx) {
        .ahcurl=ahcurl,
        .user_line_callback=callback,
        .ahdoc=ahdoc,
        .quit=false
    };

    return rv;

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

bool file_exists(const char* path) { return access(path, F_OK) == 0; }

enum { READ_FROM_FILE_BUFFER_LEN = 4096 };
unsigned char _read_from_file_bufer[READ_FROM_FILE_BUFFER_LEN] = {0};

ErrStr lexbor_read_doc_from_file(AhDoc ahdoc[static 1]) {
    FILE* fp = fopen(ahdoc->url, "r");
    if  (!fp) { return strerror(errno); }

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(ahdoc->doc)) {
        return "Lex failed to init html document";
    }

    size_t bytes_read = 0;
    while ((bytes_read = fread(_read_from_file_bufer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if (LXB_STATUS_OK != lxb_html_document_parse_chunk(
                ahdoc->doc,
                _read_from_file_bufer,
                bytes_read
            )
        ) {
            return "Failed to parse HTML chunk";
        }
    }
    //if (ferror(fp)) { perror("error reading file"); fclose(fp); return ErrFile; }
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(ahdoc->doc)) {
        return "Lbx failed to parse html";
    }
    return Ok;
}

ErrStr lexbor_read_doc_from_url_or_file (AhCurl ahcurl[static 1], AhDoc ad[static 1]) {
    if (file_exists(ad->url)) {
        return lexbor_read_doc_from_file(ad);
    }
    return curl_lexbor_fetch_document(ahcurl, ad);
}

int ah_ed_cmd_print(AhCtx ctx[static 1]) {
    if (!ctx || !ctx->ahdoc || !ctx->ahdoc->buf.data) { return -1; }
    fwrite(ctx->ahdoc->buf.data, 1, ctx->ahdoc->buf.len, stdout);
    return 0;
}
