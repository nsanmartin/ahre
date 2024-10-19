#include "src/textbuf.h"
#include "src/htmldoc.h"
#include "src/error.h"
#include "src/generic.h"
#include "src/mem.h"
#include "src/utils.h"

/* internal linkage */
//static constexpr size_t MAX_URL_LEN = 2048;
#define MAX_URL_LEN 2048
//static constexpr size_t READ_FROM_FILE_BUFFER_LEN = 4096;
#define READ_FROM_FILE_BUFFER_LEN 4096
_Thread_local static unsigned char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {0};


static TextBuf* htmldoc_textbuf(HtmlDoc htmldoc[static 1]) { return &htmldoc->textbuf; }

//TODO: use str ndup cstr
static char* str_url_dup(const Str* url) {
    if (str_is_empty(url)) { return 0x0; }
    if (len(url) >= MAX_URL_LEN) {
        perror("url too long");
        return 0x0;
    }

    char* res = malloc(len(url) + 1);
    if (!res) { return NULL; }
    res[len(url)] = '\0';
    memcpy(res, url->s, len(url));
    return res;
}





/* external linkage */

Err lexbor_read_doc_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(htmldoc->lxbdoc)) {
        return "Lex failed to init html document";
    }

    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if (LXB_STATUS_OK != lxb_html_document_parse_chunk(
                htmldoc->lxbdoc,
                read_from_file_buffer,
                bytes_read
            )
        ) {
            return "Failed to parse HTML chunk";
        }
    }
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(htmldoc->lxbdoc)) {
        return "Lbx failed to parse html";
    }
    return 0x0;
}




int htmldoc_init(HtmlDoc d[static 1], const Str url[static 1]) {
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
            lxb_html_document_destroy(d->lxbdoc);
            return -1;
        }
    }

    *d = (HtmlDoc){ .url=u, .lxbdoc=document, .textbuf=(TextBuf){.current_line=1} };
    return 0;
}

HtmlDoc* htmldoc_create(const char* url) {
    HtmlDoc* rv = std_malloc(sizeof(HtmlDoc));
    Str u;
    if (str_init(&u, url)) { return NULL; }
    if (htmldoc_init(rv, &u)) {
        destroy(rv);
        return NULL;
    }
    return rv;
}

void htmldoc_reset(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(&htmldoc->cache);
    lxb_html_document_clean(htmldoc->lxbdoc);
    textbuf_reset(&htmldoc->textbuf);
    destroy((char*)htmldoc->url);
}

void htmldoc_cleanup(HtmlDoc htmldoc[static 1]) {
    htmldoc_cache_cleanup(&htmldoc->cache);
    lxb_html_document_destroy(htmldoc->lxbdoc);
    textbuf_cleanup(&htmldoc->textbuf);
    destroy((char*)htmldoc->url);
}

inline void htmldoc_destroy(HtmlDoc* htmldoc) {
    htmldoc_cleanup(htmldoc);
    std_free(htmldoc);
}


inline void htmldoc_update_url(HtmlDoc ad[static 1], char* url) {
        destroy((char*)ad->url);
        ad->url = url;
}


bool htmldoc_is_valid(HtmlDoc htmldoc[static 1]) {
    return htmldoc->url && htmldoc->lxbdoc && htmldoc->lxbdoc->body;
}

Err htmldoc_read_from_file(HtmlDoc htmldoc[static 1]) {
    FILE* fp = fopen(htmldoc->url, "r");
    if  (!fp) { return strerror(errno); }

    Err err = NULL;

    TextBuf* textbuf = htmldoc_textbuf(htmldoc);
    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if ((err = textbuf_append_part(textbuf, (char*)read_from_file_buffer, READ_FROM_FILE_BUFFER_LEN))) {
            return err;
        }
    }
    //TODO: free mem?
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    return textbuf_append_null(textbuf);
}


