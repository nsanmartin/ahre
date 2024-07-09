#include <ahutils.h>
#include <ahdoc.h>
#include <wrappers.h>


lxb_inline lxb_status_t
print_tag_callback(const lxb_char_t *data, size_t len, void *ctx) {
    (void)ctx;
    fwrite(data, 1, len, stdout);
    return LXB_STATUS_OK;
}

Str next_word(const char* s) {
    if (!s || !*s) { return (Str){0}; }
    while (*s && isspace(*s)) { ++s; }
    const char* end = s;
    while (*end && !isspace(*end)) { ++end; }
    if (s == end) { return (Str){0}; }
    return (Str){.s=s, .len=end-s};
}

int lexbor_print_tag(const char* tag, lxb_html_document_t* document) {
    Str tags = next_word(tag);
    if (tags.len == 0) { puts("Ah, invalid tag"); return 0; }

    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 128);
    if (collection == NULL) {
        FAILED("Failed to create Collection object");
    }

    if (lxb_dom_elements_by_tag_name(
            lxb_dom_interface_element(document->body),
            collection,
            (const lxb_char_t *) tags.s,
            tags.len 
        ) != LXB_STATUS_OK
    ) { FAILED("Failed to get elements by name"); }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);

        lxb_status_t status = lxb_html_serialize_pretty_cb(
            lxb_dom_interface_node(element),
            LXB_HTML_SERIALIZE_OPT_UNDEF,
            0,
            print_tag_callback,
            NULL
        );
        if (status != LXB_STATUS_OK) {
            FAILED("Failed to serialization HTML tree");
        }

        ////TODO: store in a buffer
        //size_t value_len = 0;
        //const lxb_char_t * value = lxb_dom_element_get_attribute(element, (const lxb_char_t*)"href", 4, &value_len);
        //fwrite(value, 1, value_len, stdout);
        //fwrite("\n", 1, 1, stdout);
    }

    lxb_dom_collection_destroy(collection, true);
    return 0;

}

int lexbor_print_a_href(lxb_html_document_t* document) {
    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 128);
    if (collection == NULL) {
        FAILED("Failed to create Collection object");
    }

    if (LXB_STATUS_OK !=
        lxb_dom_elements_by_tag_name(lxb_dom_interface_element(document->body), collection, (const lxb_char_t *) "a", 1)
    ) { FAILED("Failed to get elements by name"); }

    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);

        //TODO: store in a buffer
        size_t value_len = 0;
        const lxb_char_t * value = lxb_dom_element_get_attribute(element, (const lxb_char_t*)"href", 4, &value_len);
        fwrite(value, 1, value_len, stdout);
        fwrite("\n", 1, 1, stdout);
    }

    lxb_dom_collection_destroy(collection, true);
    return 0;
}


lxb_inline lxb_status_t serializer_callback(const lxb_char_t *data, size_t len, void *ctx) {
    (void)ctx;
    return len == fwrite(data, 1, len, stdout) ? LXB_STATUS_OK : LXB_STATUS_ERROR;
}

lxb_inline lxb_status_t serializer_copy_callback(const lxb_char_t *data, size_t len, void *ctx) {
    AhBuf* buf = ctx;
    if (AhBufAppend(buf, (char*)data, len)) { return LXB_STATUS_ERROR; }
    return LXB_STATUS_OK;
}


lxb_inline void
serialize_node(lxb_dom_node_t *node)
{
    lxb_status_t status;

    status = lxb_html_serialize_pretty_cb(node, LXB_HTML_SERIALIZE_OPT_UNDEF,
                                          0, serializer_callback, NULL);
    if (status != LXB_STATUS_OK) {
        FAILED("Failed to serialization HTML tree");
    }
}



lxb_inline lxb_status_t serialize(lxb_dom_node_t *node) {
    return lxb_html_serialize_pretty_tree_cb(
        node, LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL
    );
}

void print_html(lxb_html_document_t* document) {
        serialize(lxb_dom_interface_node(document));
}

void lexbor_cp_html(lxb_html_document_t* document, AhBuf out[static 1]) {
    //TODO: return err
    lxb_html_serialize_pretty_tree_cb(
        lxb_dom_interface_node(document), LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_copy_callback, out
    );
}

size_t chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    lxb_html_document_t* document = outstream;
    size_t r = size * nmemb;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}

void lexbor_print_html_text(lxb_html_document_t* document) {
    lxb_dom_node_t *node = lxb_dom_interface_node(document->body);
    size_t len = 0x0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    fwrite(text, 1, len, stdout);
    fwrite("\n", 1, 1, stdout);
    return;
}

int lexbor_append_html_text(lxb_html_document_t* document, AhBuf dest[static 1]) {
    lxb_dom_node_t *node = lxb_dom_interface_node(document->body);
    size_t len = 0x0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    size_t prev_end = dest->len;
    dest->len += len;
    dest->data = realloc(dest->data, dest->len);
    if (!dest->data) { perror("Mem Err"); return 1; }
    memcpy(dest->data + prev_end, text, len);
    return 0;
}
