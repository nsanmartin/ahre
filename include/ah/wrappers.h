#ifndef WRAPPERS_A_HREF_H_
#define WRAPPERS_A_HREF_H_

#include <stdio.h>
#include <lexbor/html/html.h>

#include <ah/url-client.h>

#define FAILED(msg) { perror(msg); exit(EXIT_FAILURE); }


int lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf);

int curl_set_all_options(CURL* curl, const char* url, char* errbuf);

size_t lexbor_parse_chunk_callback(char *en, size_t size, size_t nmemb, void* outstream);

int ahre_append_href(lxb_dom_element_t* element, void* buf) ;
int lexbor_foreach_href(
    lxb_dom_collection_t collection[static 1],
    int (*callback)(lxb_dom_element_t* element, void* textbuf),
    void* textbuf
);

lxb_inline lxb_status_t append_to_buf_callback(const lxb_char_t *data, size_t len, void *bufptr) {
    BufOf(char)* buf = bufptr;
    //if (textbuf_append(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    if (buffn(char,append)(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    return LXB_STATUS_OK;
}

#endif

