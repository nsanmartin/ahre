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


static lxb_dom_collection_t*
lexbor_anchor_collection_from_doc(lxb_html_document_t document[static 1]) {
    lxb_dom_collection_t* hrefs = lxb_dom_collection_make(&document->dom_document, 128);
    if (!hrefs) { RETERR("Failed to create Collection object", NULL); }

    if (LXB_STATUS_OK != lxb_dom_elements_by_tag_name(
                            lxb_dom_interface_element(document->body),
                            hrefs,
                            (const lxb_char_t *) "a",
                            1
                        )
    ) { RETERR("Failed to get elements by name", NULL); }
    return hrefs;
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

int lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf) {
    Str tags = cstr_next_word_view(tag);
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
            append_to_buf_callback,
            buf
        );
        if (status != LXB_STATUS_OK) {
            FAILED("Failed to serialization HTML tree");
        }
    }

    lxb_dom_collection_destroy(collection, true);
    return 0;

}


int lexbor_foreach_href(
    lxb_dom_collection_t collection[static 1],
    int (*callback)(lxb_dom_element_t* element, void* session),
    void* session
) {
    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);

        if (callback(element, session)) { return -1; };
    }
    return 0;
}


int ahre_append_href(lxb_dom_element_t* element, void* aeBuf) {
    TextBuf* buf = aeBuf;
    size_t value_len = 0;
    const lxb_char_t * value = lxb_dom_element_get_attribute(
        element, (const lxb_char_t*)"href", 4, &value_len
    );
    if (textbuf_append_line(buf, (char*)value, value_len)) { return -1; }
    //if (buffn(char,append)(buf, (char*)value, value_len)) { return -1; }
    //if (buffn(char,append)(buf, (char*)"\n", 1)) { return -1; }
    return 0;
}

int lexbor_href_write(
    lxb_html_document_t document[static 1],
    lxb_dom_collection_t** hrefs,
    TextBuf* textbuf
) {
    if (!*hrefs) {
        if(!(*hrefs = lexbor_anchor_collection_from_doc(document))) {
            return -1;
        }
    }

    if(lexbor_foreach_href(*hrefs, ahre_append_href, (void*)textbuf)) {
        return -1;
    }
    return 0;
}



void print_html(lxb_html_document_t* document) {
        serialize(lxb_dom_interface_node(document));
}



int
lexbor_html_text_append(lxb_html_document_t* document, TextBuf* buf) {
    lxb_dom_node_t *node = lxb_dom_interface_node(document->body);
    if (!node) {
        puts("No lxb doc");
        return -1;
    }
    size_t len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    if (textbuf_append(buf, (char*)text, len)) { return -1; }
    return 0;
}


size_t lexbor_parse_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    lxb_html_document_t* document = outstream;
    size_t r = size * nmemb;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}

