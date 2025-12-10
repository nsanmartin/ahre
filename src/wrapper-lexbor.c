#include "utils.h"
#include "htmldoc.h"
#include "wrapper-lexbor.h"

/* internal linkage */

static StrView cstr_next_word_view(const char* s) {
    if (!s || !*s) { return (StrView){0}; }
    while (*s && isspace(*s)) { ++s; }
    const char* end = s;
    while (*end && !isspace(*end)) { ++end; }
    if (s == end) { return (StrView){0}; }
    return (StrView){.items=s, .len=end-s};
}

/* external linkage */

Err lexbor_cp_tag(const char* tag, lxb_html_document_t* document, BufOf(char)* buf) {
    StrView tags = cstr_next_word_view(tag);
    if (tags.len == 0) { return "invalid tag"; }

    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 128);
    if (collection == NULL) {
        return "failed to create lexbor collection object";
    }

    if (lxb_dom_elements_by_tag_name(
            lxb_dom_interface_element(document->body),
            collection,
            (const lxb_char_t *) tags.items,
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


Err lexbor_html_text_append(lxb_html_document_t* document, TextBuf* buf) {
    lxb_dom_node_t *node = lxb_dom_interface_node(document->body);
    if (!node) {
        return "could not get lexbor document body as node";
    }
    size_t len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &len);
    Err err = NULL;
    if ((err = textbuf_append_part(buf, (char*)text, len))) {
        return err;
    }
    return textbuf_append_null(buf);
}


/*
 * The signature of this fn must match the api of curl_easy_setopt CURLOPT_WRITEFUNCTION
 */
size_t _read_curl_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
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
    if (!htmldoc_http_content_type_text_or_undef(htmldoc) || !htmldoc_http_charset_is_utf8(htmldoc))
        return r;
    lxb_html_document_t* document = htmldoc_lxbdoc(htmldoc);
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}


static inline Err _lexbor_node_to_str_(lxb_dom_node_t* node, Str buf[_1_], size_t level) {
    if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) return Ok;
    size_t len;
    char* tag = (char*)lxb_dom_element_qualified_name(lxb_dom_interface_element(node), &len);
    if (len && tag) {
        for (size_t i = 0; i < level; ++i) try( str_append_lit__(buf, "    "));
        try( str_append_lit__(buf, "<"));
        try( str_append(buf, tag, len));
        try( str_append_lit__(buf, ">\n"));
    }

    lxb_dom_attr_t* attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));
    for (;attr;  attr = lxb_dom_element_next_attribute(attr)) {
        size_t namelen;
        const lxb_char_t* name = lxb_dom_attr_qualified_name(attr, &namelen);
        if (!namelen || !name)
            return "error: expecting name for attr but lxb_dom_attr_qualified_name returned null";

        size_t valuelen;
        const lxb_char_t* value = lxb_dom_attr_value(attr, &valuelen);

        for (size_t i = 0; i <= level; ++i) try( str_append_lit__(buf, "    "));
        try( str_append(buf, (char*)name, namelen));
        try( str_append_lit__(buf, "="));
        if (valuelen && valuelen) try( str_append(buf, (char*)value, valuelen));
        else try( str_append_lit__(buf, "(null)"));
        try( str_append_lit__(buf, "\n"));
    }

    for (lxb_dom_node_t* it = node->first_child; it; it = it->next) {
        _lexbor_node_to_str_(it, buf, level + 1);
        if (it == node->last_child) break;
    }
    if (len && tag) {
        for (size_t i = 0; i < level; ++i) try( str_append_lit__(buf, "    "));
        try( str_append_lit__(buf, "<"));
        try( str_append(buf, tag, len));
        try( str_append_lit__(buf, "/>\n"));
    }
    return Ok;
}


Err lexbor_node_to_str(lxb_dom_node_t* node, Str buf[_1_]) {
    return _lexbor_node_to_str_(node, buf, 0);
}


lxb_dom_node_t* _find_parent_form(lxb_dom_node_t* node) {
    for (;node; node = node->parent) {
        if (node->local_name == LXB_TAG_FORM) break;
    }
    return node;
}


Err lexbor_get_title_text(lxb_dom_node_t* title, Str out[_1_]) {
    if (!title) return Ok;
    lxb_dom_node_t* node = title->first_child; 
    lxb_dom_text_t* text = lxb_dom_interface_text(node);
    if (!text) return Ok;
    StrView view = strview_from_mem(
        (const char*)text->char_data.data.data, text->char_data.data.length
    );
    if (view.len && str_append(out, (char*)view.items, view.len))
            return "error: buf append failure";
    return Ok;
}

Err lexbor_get_title_text_line(lxb_dom_node_t* title, BufOf(char)* out) {
    if (!title) return Ok;
    lxb_dom_node_t* node = title->first_child; 
    lxb_dom_text_t* text = lxb_dom_interface_text(node);
    if (!text) return Ok;
    StrView text_view = strview_from_mem(
        (const char*)text->char_data.data.data, text->char_data.data.length
    );
    while (text_view.len && text_view.items) {
        StrView line = str_split_line(&text_view);
        if (line.len && !buffn(char, append)(out, (char*)line.items, line.len))
            return "error: buf append failure";
    }
    return Ok;
}

Err dbg_print_title(lxb_dom_node_t* title) {
    if (!title) return "error: no title";
    lxb_dom_node_t* node = title->first_child; 
    lxb_dom_text_t* text = lxb_dom_interface_text(node);

    if (!text) {
        fprintf(stderr, "invalid title in dom\n");
        return Ok;
    }
    
    StrView text_view = strview_from_mem((const char*)text->char_data.data.data, text->char_data.data.length);
    while (text_view.len && text_view.items) {
        StrView line = str_split_line(&text_view);
        if (line.len) fwrite(line.items, 1, line.len, stdout);
    }
    fwrite("\n", 1, 1, stdout);
    return Ok;
}

