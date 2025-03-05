#include "src/utils.h"
#include "src/htmldoc.h"
#include "src/wrapper-lexbor.h"

//lxb_inline lxb_status_t append_to_buf_callback(const lxb_char_t *data, size_t len, void *bufptr) ;

/* internal linkage */

static StrView cstr_next_word_view(const char* s) {
    if (!s || !*s) { return (StrView){0}; }
    while (*s && isspace(*s)) { ++s; }
    const char* end = s;
    while (*end && !isspace(*end)) { ++end; }
    if (s == end) { return (StrView){0}; }
    return (StrView){.s=s, .len=end-s};
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
    StrView tags = cstr_next_word_view(tag);
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


// deprecated
//Err lexbor_foreach_href(
//    lxb_dom_collection_t collection[static 1],
//    Err (*callback)(lxb_dom_element_t* element, void* session),
//    void* session
//) {
//    Err err = Ok;
//    for (size_t i = 0; i < lxb_dom_collection_length(collection); i++) {
//        lxb_dom_element_t* element = lxb_dom_collection_element(collection, i);
//        if ((err=callback(element, session))) { break; };
//    }
//
//    return err;
//}


// deprecated
//Err ahre_append_href(lxb_dom_element_t* element, void* aeBuf) {
//    TextBuf* buf = aeBuf;
//    size_t value_len = 0;
//    const lxb_char_t * value = lxb_dom_element_get_attribute(
//        element, (const lxb_char_t*)"href", 4, &value_len
//    );
//    if (value_len && value)
//        return textbuf_append_line(buf, (char*)value, value_len);
//    return NULL;
//}


/// deprecated
///*
// * Writes all hrefs into textbuf
// */
//Err lexbor_href_write(
//    lxb_html_document_t document[static 1],
//    lxb_dom_collection_t** hrefs,
//    TextBuf* textbuf
//) {
//    if (!*hrefs) {
//        *hrefs = lxb_dom_collection_make(&document->dom_document, 128);
//        if (!*hrefs) { return "failed to create lexbor collection object"; }
//        if (LXB_STATUS_OK != lxb_dom_elements_by_tag_name(
//            lxb_dom_interface_element(document->body), *hrefs, (const lxb_char_t *) "a", 1
//        )) { return "failed to get elements by name"; }
//    }
//
//    Err err = lexbor_foreach_href(*hrefs, ahre_append_href, (void*)textbuf);
//    if (err) return err;
//    return textbuf_append_null(textbuf);
//}


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
    Err err = NULL;
    if ((err = textbuf_append_part(buf, (char*)text, len))) {
        return err;
    }
    return textbuf_append_null(buf);
}


/*
 * The signature of this fn must match the api of curl_easy_setopt CURLOPT_WRITEFUNCTION
 */
size_t lexbor_parse_chunk_callback(char *in, size_t size, size_t nmemb, void* outstream) {
    HtmlDoc* htmldoc = outstream;
    size_t r = size * nmemb;
    if (textbuf_append_part(htmldoc_sourcebuf(htmldoc), in, r)) {
        //TODO: log error
        return LXB_STATUS_ERROR;
    }
    lxb_html_document_t* document = htmldoc->lxbdoc;
    return  LXB_STATUS_OK == lxb_html_document_parse_chunk(document, (lxb_char_t*)in, r) ? r : 0;
}


Err lexbor_set_attr_value(lxb_dom_node_t* node, const char* value, size_t valuelen)  {
    lxb_dom_element_set_attribute(
        lxb_dom_interface_element(node),
        (const lxb_char_t*)"value",
        sizeof("value")-1,
        (const lxb_char_t*)value,
        valuelen
    );
    return Ok;
}

/*
 * This is innefficient. For each time an attr is queried, 
 * all of them are iterated, but: there are more priority tasks and
 * there are usually few attrs. 
 */
void lexbor_find_attr_value(
    lxb_dom_node_t* node, const char* attr_name, const lxb_char_t* out[static 1], size_t* len
) 
{
    lxb_dom_attr_t* attr;

    attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));
    while (attr) {
        size_t data_len;
        const lxb_char_t* data = lxb_dom_attr_qualified_name(attr, &data_len);
        if (lexbor_str_eq(attr_name, data, data_len))  {
            *out = lxb_dom_attr_value(attr, len);
            return;
        }
        attr = lxb_dom_element_next_attribute(attr);
    }

    *out = NULL; *len = 0;
}

Err lexbor_node_to_str(lxb_dom_node_t* node, BufOf(char)* buf) {
    lxb_dom_attr_t* attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));
    char* nullstr = "(null)";
    char* indentstr = "    ";
    for (;attr;  attr = lxb_dom_element_next_attribute(attr)) {
        size_t namelen;
        const lxb_char_t* name = lxb_dom_attr_qualified_name(attr, &namelen);
        if (!namelen || !name)
            return "error: expecting name for attr but lxb_dom_attr_qualified_name returned null";

        size_t valuelen;
        const lxb_char_t* value = lxb_dom_attr_value(attr, &valuelen);

        if (!buffn(char, append)(buf, indentstr, sizeof(indentstr)-1))
            return "error: mem failure (BufOf.append)";
        if (!buffn(char, append)(buf, (char*)name, namelen))
            return "error: mem failure (BufOf.append)";
        if (!buffn(char, append)(buf, "=", 1)) return "error: mem failure (BufOf.append)";
        if (valuelen && valuelen) {
            if (!buffn(char, append)(buf, (char*)value, valuelen))
                return "error: mem failure (BufOf.append)";
        } else {
            if (!buffn(char, append)(buf, nullstr, sizeof(nullstr)-1))
                return "error: mem failure (BufOf.append)";
        }
        if (!buffn(char, append)(buf, "\n", 1)) return "error: mem failure (BufOf.append)";
    }
    return Ok;
}

lxb_dom_node_t*
_find_parent_form(lxb_dom_node_t* node) {
    for (;node; node = node->parent) {
        if (node->local_name == LXB_TAG_FORM) break;
    }
    return node;
}

bool _lexbor_attr_has_value(
     lxb_dom_node_t node[static 1], const char* attr, const char* expected_value
) {
    const lxb_char_t* value;
    size_t valuelen;
    lexbor_find_attr_value(node, attr, &value, &valuelen);
    return value && valuelen && lexbor_str_eq(expected_value, value, valuelen);
}

Err lexbor_get_title_text_line(lxb_dom_node_t* title, BufOf(char)* out) {
    if (!title) return Ok;
    lxb_dom_node_t* node = title->first_child; 
    lxb_dom_text_t* text = lxb_dom_interface_text(node);
    if (!text) return Ok;
    StrView text_view = strview_from_mem((const char*)text->char_data.data.data, text->char_data.data.length);
    while (text_view.len && text_view.s) {
        StrView line = str_split_line(&text_view);
        if (line.len && !buffn(char, append)(out, (char*)line.s, line.len))
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
    while (text_view.len && text_view.s) {
        StrView line = str_split_line(&text_view);
        if (line.len) fwrite(line.s, 1, line.len, stdout);
    }
    fwrite("\n", 1, 1, stdout);
    return Ok;
}
