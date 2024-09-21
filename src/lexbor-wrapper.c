#include <ah/utils.h>
#include <ah/doc.h>
#include <ah/wrappers.h>


/* internal linkage */

static Str cstr_next_word_view(const char* s) {
    if (!s || !*s) { return (Str){0}; }
    while (*s && isspace(*s)) { ++s; }
    const char* end = s;
    while (*end && !isspace(*end)) { ++end; }
    if (s == end) { return (Str){0}; }
    return (Str){.s=s, .len=end-s};
}


lxb_inline lxb_status_t
serializer_callback(const lxb_char_t *data, size_t len, void *session) {
    (void)session;
    return len == fwrite(data, 1, len, stdout) ? LXB_STATUS_OK : LXB_STATUS_ERROR;
}


lxb_inline lxb_status_t serialize(lxb_dom_node_t *node) {
    return lxb_html_serialize_pretty_tree_cb(
        node, LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL
    );
}

/* external linkage */

Err lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf) {
    Str tags = cstr_next_word_view(tag);
    if (tags.len == 0) { return "invalid tag"; }

    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 128);
    if (collection == NULL) {
        return "failed to create lexbor collection object";
    }

    if (lxb_dom_elements_by_tag_name(
            lxb_dom_interface_element(document->body),
            collection,
            (const lxb_char_t *) tags.s,
            tags.len 
        ) != LXB_STATUS_OK
    ) { return "failed to get elements by name"; }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);

        lxb_status_t status = lxb_html_serialize_pretty_cb(
            lxb_dom_interface_node(element),
            LXB_HTML_SERIALIZE_OPT_UNDEF,
            0,
            append_to_buf_callback,
            buf
        );
        if (status != LXB_STATUS_OK) {
            return "failed to serialization HTML tree";
        }
    }

    lxb_dom_collection_destroy(collection, true);
    return Ok;

}


Err lexbor_foreach_href(
    lxb_dom_collection_t collection[static 1],
    Err (*callback)(lxb_dom_element_t* element, void* session),
    void* session
) {
    Err err = Ok;
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);
        if ((err=callback(element, session))) { break; };
    }

    return err;
}


Err ahre_append_href(lxb_dom_element_t* element, void* aeBuf) {
    TextBuf* buf = aeBuf;
    size_t value_len = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)"href", 4, &value_len
    );
    return textbuf_append_line(buf, (char*)value, value_len);
}


Err lexbor_href_write(
    lxb_html_document_t document[static 1],
    lxb_dom_collection_t** hrefs,
    TextBuf* textbuf
) {
    if (!*hrefs) {
        *hrefs = lxb_dom_collection_make(&document->dom_document, 128);
        if (!*hrefs) { return "failed to create lexbor collection object"; }
        if (LXB_STATUS_OK != lxb_dom_elements_by_tag_name(
            lxb_dom_interface_element(document->body), *hrefs, (const lxb_char_t *) "a", 1
        )) { return "failed to get elements by name"; }
    }

    return lexbor_foreach_href(*hrefs, ahre_append_href, (void*)textbuf);
}


void print_html(lxb_html_document_t* document) {
        serialize(lxb_dom_interface_node(document));
}


Err
lexbor_html_text_append(lxb_html_document_t* document, TextBuf* buf) {
    lxb_dom_node_t *node = lxb_dom_interface_node(document->body);
    if (!node) {
        return "could not get lexbor document body as node";
    }
    size_t len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    return textbuf_append(buf, (char*)text, len);
}


size_t lexbor_parse_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    lxb_html_document_t* document = outstream;
    size_t r = size * nmemb;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}

