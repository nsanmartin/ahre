#include <unistd.h>
#include <aherror.h>
#include <curl-lxb.h>
#include <mem.h>
#include <wrappers.h>

#include <ahutils.h>
#include <ahdoc.h>
#include <ae-buf.h>

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


int AhDocInit(AhDoc d[static 1], char* url) {
    if (!d) { return -1; }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        return -1;
    }

    if (url){
        url = ah_urldup(url);
        if (!url) {
            lxb_html_document_destroy(d->doc);
            return -1;
        }
    }

    *d = (AhDoc){ .url=url, .doc=document };
    return 0;
}

AhDoc* AhDocCreate(char* url) {
    AhDoc* rv = ah_malloc(sizeof(AhDoc));
    if (AhDocInit(rv, url)) {
        ah_free(rv);
        return NULL;
    }
    return rv;
}


ErrStr AhDocFetch(AhCurl ahcurl[static 1], AhDoc ad[static 1]) {
    return lexbor_read_doc_from_url_or_file (ahcurl, ad);
}

/*
 * Functions that append summary of the contet for ebugging purposes
 */
int ahdoc_buffer_summary(AhDoc ahdoc[static 1], BufOf(char)* buf) {

    buf_append_lit("AhDoc: ", buf);
    if (buf_append_hexp(ahdoc, buf)) { return -1; }
    buf_append_lit("\n", buf);

    if (ahdoc->url) {
        buf_append_lit(" url: ", buf);
       if (buffn(char,append)(buf, (char*)ahdoc->url, strlen(ahdoc->url))) {
           return -1;
       }
       buf_append_lit("\n", buf);

    } else {
       buf_append_lit(" none\n", buf);

    }
    if (ahdoc->doc) {
        buf_append_lit(" doc: ", buf);
        if (buf_append_hexp(ahdoc->doc, buf)) { return -1; }
        buf_append_lit("\n", buf);

    }
    return 0;
}


///int ahcurl_print_summary(AhCurl ahcurl[static 1], FILE f[static 1]) {
///   fprintf(f, "AhCurl: %p\n", (void*)ahcurl);
///   if (ahcurl->curl) {
///       fprintf(f, " curl: %p\n", (void*)ahcurl->curl);
///       if (ahcurl->errbuf[0]) {
///           fprintf(f, " curl errbuf: %s\n", ahcurl->errbuf);
///       }
///   } else {
///       fprintf(f, " not curl\n");
///   }
///   return 0;
///}
///
///int ahdoc_print_summary(AhDoc ahdoc[static 1], FILE f[static 1]) {
///    fprintf(f, "AhDoc: %p\n", (void*)ahdoc);
///    if (ahdoc->url) {
///        fprintf(f," url: %s\n", ahdoc->url);
///    } else {
///        fprintf(f, " none\n");
///    }
///    if (ahdoc->doc) {
///        fprintf(f, " doc: %p\n", (void*)ahdoc->doc);
///    }
///    return 0;
///}
///
///int ahdoc_cache_print_summary(AhDocCache c[static 1], FILE f[static 1]) {
///    fprintf(f, "AhDocCache: %p\n", (void*)c);
///    if (c->hrefs) {
///        fprintf(f," hrefsp: %p\n", (void*)c->hrefs);
///        if (c->hrefs) {
///            if(lexbor_foreach_href(c->hrefs, ahre_print_href, 0x0)) {
///                return -1;
///            }
///        }
///    } else {
///        fprintf(f, " no hrefs\n");
///    }
///    return 0;
///}
///
///int ahctx_print_summary(AhCtx ctx[static 1], FILE f[static 1]) {
///    if (ctx->ahcurl) {
///        if(ahcurl_print_summary(ctx->ahcurl, f)) { return -1; }
///    } else { fprintf(f, "Not ahcurl\n"); }
///
///    if (ctx->ahdoc) {
///        ahdoc_print_summary(ctx->ahdoc, f);
///        ahdoc_cache_print_summary(&ctx->ahdoc->cache, f);
///    } else { fprintf(f, "Not ahcdoc\n"); }
///
///    return 0;
///}
// _summary fns

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

