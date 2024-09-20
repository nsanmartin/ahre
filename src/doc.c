#include <unistd.h>

#include <ah/textbuf.h>
#include <ah/curl-lxb.h>
#include <ah/doc.h>
#include <ah/error.h>
#include <ah/mem.h>
#include <ah/utils.h>
#include <ah/wrappers.h>

/* internal linkage */
static constexpr size_t MAX_URL_LEN = 2048;
static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};


static ErrStr lexbor_read_doc_from_url_or_file (UrlClient url_client[static 1], Doc ad[static 1]); 


static char* str_url_dup(const Str* url) {
    if (str_is_empty(url)) { return 0x0; }
    if (url->len >= MAX_URL_LEN) {
            perror("url too long");
            return 0x0;
    }

    char* res = malloc(url->len + 1);
    if (!res) { return NULL; }
    res[url->len] = '\0';
    memcpy(res, url->s, url->len);
    return res;
}


static bool file_exists(const char* path) { return access(path, F_OK) == 0; }

static ErrStr lexbor_read_doc_from_file(Doc ahdoc[static 1]) {
    FILE* fp = fopen(ahdoc->url, "r");
    if  (!fp) { return strerror(errno); }

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(ahdoc->doc)) {
        return "Lex failed to init html document";
    }

    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if (LXB_STATUS_OK != lxb_html_document_parse_chunk(
                ahdoc->doc,
                read_from_file_buffer,
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

static ErrStr lexbor_read_doc_from_url_or_file (UrlClient url_client[static 1], Doc ad[static 1]) {
    if (file_exists(ad->url)) {
        return lexbor_read_doc_from_file(ad);
    }
    return curl_lexbor_fetch_document(url_client, ad);
}


/* external linkage */

int doc_init(Doc d[static 1], const Str* url) {
    if (!d) { return -1; }
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        perror("Lxb failed to create html document");
        return -1;
    }

    const char* u = NULL;
    if (!str_is_empty(url)){
        u = str_url_dup(url);
        if (!u) {
            lxb_html_document_destroy(d->doc);
            return -1;
        }
    }

    *d = (Doc){ .url=u, .doc=document, .aebuf=(TextBuf){.current_line=1} };
    return 0;
}

Doc* doc_create(char* url) {
    Doc* rv = std_malloc(sizeof(Doc));
    Str u;
    if (str_init(&u, url)) { return NULL; }
    if (doc_init(rv, &u)) {
        destroy(rv);
        return NULL;
    }
    return rv;
}

inline void doc_reset(Doc* ahdoc) {
    doc_cache_cleanup(&ahdoc->cache);
    lxb_html_document_clean(ahdoc->doc);
    textbuf_reset(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

inline void doc_cleanup(Doc* ahdoc) {
    doc_cache_cleanup(&ahdoc->cache);
    lxb_html_document_destroy(ahdoc->doc);
    textbuf_cleanup(&ahdoc->aebuf);
    destroy((char*)ahdoc->url);
}

inline void doc_destroy(Doc* ahdoc) {
    doc_cleanup(ahdoc);
    std_free(ahdoc);
}

ErrStr doc_fetch(UrlClient url_client[static 1], Doc ad[static 1]) {
    return lexbor_read_doc_from_url_or_file (url_client, ad);
}


inline void doc_update_url(Doc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}
