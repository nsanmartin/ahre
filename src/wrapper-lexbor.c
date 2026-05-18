#include "utils.h"
#include "htmldoc.h"
#include "wrapper-lexbor.h"
#include <lexbor/dom/dom.h>


/* internal linkage */


/* external linkage */



/*
 * The signature of this fn must match the api of curl_easy_setopt CURLOPT_WRITEFUNCTION
 */
static size_t _read_curl_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    HtmlDoc* htmldoc = outstream;
    size_t r = size * nmemb;
    /* Your callback should return the number of bytes actually taken care of.
     * If that amount differs from the amount passed to your callback function,
     * it signals an error condition to the library. This causes the transfer
     * to get aborted and the libcurl function used returns CURLE_WRITE_ERROR.
     * */
    if (textbuf_append_part(htmldoc_sourcebuf(htmldoc), in, r)) return 0;
    return r;
}

size_t lexbor_parse_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    size_t r;
    if ((r=_read_curl_chunk_callback(in, size, nmemb, outstream)) != size * nmemb)
        return r;
    HtmlDoc* htmldoc = outstream;
    if (!htmldoc_content_is_html(htmldoc) || !htmldoc_http_charset_is_utf8(htmldoc))
        return r;
    lxb_html_document_t* document = htmldoc_dom(htmldoc).ptr;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}


Err
lexbor_count_tag_occurences_in_doc(DomPtr doc, StrView tag, size_t count[_1_]) {
    lxb_dom_collection_t* collection = lxb_dom_collection_make(&doc->dom_document, 16);
    if (collection == NULL)
        return err_internal("lexbor failed to create Collection object");

    DomElemPtr domdoc = lxb_dom_interface_element(doc);
    if (LXB_STATUS_OK !=
        lxb_dom_elements_by_tag_name(domdoc, collection, (const lxb_char_t *)tag.items, tag.len))
        return  err_internal("lexbor failed to get scripts");
    *count = lxb_dom_collection_length(collection);
    lxb_dom_collection_destroy(collection, true);
    return Ok;
}
