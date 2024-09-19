#include <unistd.h>

#include <ah/buf.h>
#include <ah/curl-lxb.h>
#include <ah/doc.h>
#include <ah/error.h>
#include <ah/mem.h>
#include <ah/utils.h>
#include <ah/wrappers.h>

constexpr size_t ah_max_url_len = 2048;


static ErrStr lexbor_read_doc_from_url_or_file (UrlClient ahcurl[static 1], Doc ad[static 1]); 


char* ah_urldup(const Str* url) {
    if (StrIsEmpty(url)) { return 0x0; }
    if (url->len >= ah_max_url_len) {
            perror("url too long");
            return 0x0;
    }

    char* res = malloc(url->len + 1);
    if (!res) { return NULL; }
    res[url->len] = '\0';
    memcpy(res, url->s, url->len);
    return res;
}


int doc_init(Doc d[static 1], const Str* url) {
    if (!d) { return -1; }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        return -1;
    }

    const char* u = NULL;
    if (!StrIsEmpty(url)){
        u = ah_urldup(url);
        if (!u) {
            lxb_html_document_destroy(d->doc);
            return -1;
        }
    }

    *d = (Doc){ .url=u, .doc=document, .aebuf=(TextBuf){.current_line=1} };
    return 0;
}

Doc* doc_create(char* url) {
    Doc* rv = ah_malloc(sizeof(Doc));
    Str u;
    if (StrInit(&u, url)) { return NULL; }
    if (doc_init(rv, &u)) {
        destroy(rv);
        return NULL;
    }
    return rv;
}


ErrStr doc_fetch(UrlClient ahcurl[static 1], Doc ad[static 1]) {
    return lexbor_read_doc_from_url_or_file (ahcurl, ad);
}

/*
 * Functions that append summary of the content for debugging purposes
 */
int AhDocBufSummary(Doc ahdoc[static 1], BufOf(char)* buf) {

    buf_append_lit("Doc: ", buf);
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


static bool file_exists(const char* path) { return access(path, F_OK) == 0; }

static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
unsigned char _read_from_file_bufer[READ_FROM_FILE_BUFFER_LEN] = {0};

static ErrStr lexbor_read_doc_from_file(Doc ahdoc[static 1]) {
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

static ErrStr lexbor_read_doc_from_url_or_file (UrlClient ahcurl[static 1], Doc ad[static 1]) {
    if (file_exists(ad->url)) {
        return lexbor_read_doc_from_file(ad);
    }
    return curl_lexbor_fetch_document(ahcurl, ad);
}

