#include <wrappers.h>


size_t chunk_callback(char *in, size_t size, size_t nmemb, void* outstream);


int lexbor_fetch_document(lxb_html_document_t* document, const char* url) {

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_begin(document)) { FAILED("Failed to parse HTML"); }

    CurlWithErrors cwe = curl_create();
    if (!cwe.curl) { FAILED("curl_easy_init failure\n"); }

    if (curl_set_all_options(cwe.curl, url, cwe.errbuf)
        || curl_set_callback_and_buffer(cwe.curl, chunk_callback, document)) {
        FAILED("error initializing cwe.curl");
    }

    if (curl_easy_perform(cwe.curl)) { FAILED(cwe.errbuf); }

    curl_easy_cleanup(cwe.curl);

    if (LXB_STATUS_OK != lxb_html_document_parse_chunk_end(document)) { FAILED("Failed to parse HTML"); }
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

        //serialize_node(lxb_dom_interface_node(element));
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


size_t chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    lxb_html_document_t* document = outstream;
    size_t r = size * nmemb;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}

 
//int ah_lexbor_doc(AhCtx ctx[static 1]) {
//    if (ctx->fetch) {
//        if (lexbor_fetch_document(ctx->doc, ctx->url)) { FAILED("Failed to create HTML Document"); }
//        //if (LXB_STATUS_OK != serialize(lxb_dom_interface_node(document))) { FAILED("Failed to serialize HTML"); }
//        lexbor_print_a_href(ctx->doc);
//        ctx->fetch = false;
//
//        //lxb_html_document_destroy(document);
//    }
//    return EXIT_SUCCESS;
//}

int ah_lexbor(const char* url) {
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) { FAILED("Failed to create HTML Document"); }

    if (lexbor_fetch_document(document, url)) { FAILED("Failed to create HTML Document"); }

    //if (LXB_STATUS_OK != serialize(lxb_dom_interface_node(document))) { FAILED("Failed to serialize HTML"); }
    lexbor_print_a_href(document);


    lxb_html_document_destroy(document);
    return EXIT_SUCCESS;
}

