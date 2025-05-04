#ifndef AHRE_BOOKMARK_H__
#define AHRE_BOOKMARK_H__

#include <errno.h>

#include "debug.h"
#include "utils.h"
#include "htmldoc.h"
#include "error.h"

static inline Err
bookmark_sections_body(HtmlDoc bookmark[static 1], lxb_dom_node_t* out[static 1]) {
    lxb_html_document_t* lxbdoc = htmldoc_lxbdoc(bookmark);
    lxb_dom_node_t* node = lxb_dom_interface_node(lxbdoc);
    if (!node) return "error: no document";
    if (node->type != LXB_DOM_NODE_TYPE_DOCUMENT) return "not a bookmark file";
    lxb_dom_node_t* html = node->first_child;
    if (!html) return "not a bookmark file: expecting an html tag";
    if (html != node->last_child) return "not a bookmark file";
    if (html->local_name != LXB_TAG_HTML) return "not a bookmark file";
    if (!html->first_child) return "not a bookmark: expecting head";
    if (html->first_child->local_name != LXB_TAG_HEAD) return "not a bookmark file";
    if (!html->last_child) return "not a bookmark: expecting body";
    if (html->last_child->local_name != LXB_TAG_BODY) return "not a bookmark file";
    *out = html->last_child;
    return Ok;
}

static inline Err bookmark_sections(lxb_dom_node_t* body, ArlOf(BufOf(char))* out) {
    for (lxb_dom_node_t* it = body->first_child; it != body->last_child; it = it->next) {
        if (it->local_name == LXB_TAG_H2) {
            if (it->first_child != it->last_child) return "invalid bookmark file";

            BufOf(char)* buf = arlfn(BufOf(char),append)(out, &(BufOf(char)){0});
            if (!buf) return "error: arl append failure";

            const char* data;
            size_t len;
            try( lexbor_node_get_text(it->first_child, &data, &len));
            if (len)
                if (!buffn(char, append)(buf, (char*)data, len)) return "error: buf append failure";
        }
    }

    return Ok;
}

static inline void bookmark_sections_sort(ArlOf(BufOf(char))* list) {
    qsort(list->items, list->len, sizeof(BufOf(char)), buf_of_char_cmp);
}

static inline Err bookmark_section_insert(
    lxb_dom_document_t *domdoc, lxb_dom_node_t* body, const char* q, lxb_dom_element_t* bm_entry
) {
    lxb_dom_element_t* section =
        lxb_dom_document_create_element(domdoc, (const lxb_char_t*)"h2", 2, NULL);
    if (!section) return "error: lxb elem create failure";
    lxb_dom_text_t* domtext =
        lxb_dom_document_create_text_node( domdoc, (const lxb_char_t*)q, strlen(q));
    if (!domtext) return "error: lxb text create failure";//TODO: clean *out

    lxb_dom_node_insert_child(lxb_dom_interface_node(section), lxb_dom_interface_node(domtext));
    lxb_dom_node_insert_child(lxb_dom_interface_node(body), lxb_dom_interface_node(section));

    lxb_dom_element_t* ul =
        lxb_dom_document_create_element(domdoc, (const lxb_char_t*)"ul", 2, NULL);
    if (!ul) return "error: lxb text create failure";//TODO: clean *out
    lxb_dom_node_insert_child(lxb_dom_interface_node(ul), lxb_dom_interface_node(bm_entry));
    lxb_dom_node_insert_child(lxb_dom_interface_node(body), lxb_dom_interface_node(ul));
    return Ok;
}


static inline Err
bookmark_section_get(lxb_dom_node_t* body, const char* q, lxb_dom_node_t* out[static 1]) {
    lxb_dom_node_t* res = NULL;
    for (lxb_dom_node_t* it = body->first_child; it != body->last_child; it = it->next) {
        if (it->local_name == LXB_TAG_H2) {
            if (it->first_child != it->last_child) return "invalid bookmark file";

            const char* data;
            size_t len;
            try( lexbor_node_get_text(it->first_child, &data, &len));
            if (len ) {
                size_t qlen = strlen(q);
                size_t min = qlen == len ? len : qlen;
                if (qlen <= min && strncmp(q, data, min) == 0) {
                    if (res) return "unequivocal reference to bookmark section";
                    res = it;
                }
            }
        }
    }
    *out = res;
    return Ok;
}

static inline Err
bookmark_section_ul_get(lxb_dom_node_t* body, const char* q, lxb_dom_node_t* out[static 1]) {
    *out = NULL;
    lxb_dom_node_t* section;
    try( bookmark_section_get(body, q, &section));
    if (section) {
        if (!section->next) return "error: expecting section's next (TEXT)";
        lxb_dom_node_t* ul = section->next->next;
        if (!ul) return "error: expecting section's next next";
        if (section->next->next->local_name != LXB_TAG_UL) 
            return "error: expecting section's next next to be ul";
        *out = ul;
    }
    return Ok;
}

static inline Err bookmark_mk_anchor (
    lxb_dom_document_t *domdoc, char* href, BufOf(char)* text, lxb_dom_element_t* out[static 1]
 ) {
    *out = lxb_dom_document_create_element(domdoc, (const lxb_char_t*)"a", 1, NULL);
    if (!*out) return "error: lxb elem create failure";
    lxb_dom_element_set_attribute(
        *out, (const lxb_char_t*)"href", sizeof("href")-1, (const lxb_char_t*)href, strlen(href)
    );
    lxb_dom_text_t* domtext = lxb_dom_document_create_text_node(
        domdoc, (const lxb_char_t*)text->items, text->len
    );
    if (!domtext) return "error: lxb text create failure";//TODO: clean *out

    lxb_dom_node_insert_child(
        lxb_dom_interface_node(*out), lxb_dom_interface_node(domtext)
    );
    return Ok;
}

static inline Err
bookmark_mk_entry(
    lxb_html_document_t *document,
    char* href,
    BufOf(char)* text,
    lxb_dom_element_t* out[static 1]
) {
    lxb_dom_document_t *domdoc = &document->dom_document;
    *out = lxb_dom_document_create_element(domdoc, (const lxb_char_t*)"li", 2, NULL);
    if (!*out) return "error: lxb elem create failure";

    lxb_dom_element_t* a;
    Err err = bookmark_mk_anchor(domdoc, href, text, &a);
    if (err) {
        lxb_dom_document_destroy_element(*out);
        return err;
    }

    lxb_dom_node_insert_child(lxb_dom_interface_node(*out), lxb_dom_interface_node(a));
    return Ok;
}

static inline Err
_get_bookmarks_doc_(UrlClient url_client[static 1], Str* bmfile, HtmlDoc out[static 1]) {
    try( get_bookmark_filename_if_it_exists(bmfile));
    try(htmldoc_init(out, mk_union_cstr(bmfile->items), http_get));
    Err err = htmldoc_fetch(out, url_client, output_dev_null__, NULL);
    if (err) {
        htmldoc_cleanup(out);
        return err;
    }
    return Ok;
}

static inline Err _bm_to_source_rec_(lxb_dom_node_t* node, Str out[static 1]) ;

static inline Err _bm_to_source_rec_childs_(lxb_dom_node_t* node, Str out[static 1]) {
    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
        try( _bm_to_source_rec_(it, out));
        if (it == node->last_child) break;
    }
    return Ok;
}


static inline Err _bm_to_source_rec_childs_no_text_(lxb_dom_node_t* node, Str out[static 1]) {
    for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
        if (it->type == LXB_DOM_NODE_TYPE_TEXT) continue; 
        try( _bm_to_source_rec_(it, out));
        if (it == node->last_child) break;
    }
    return Ok;
}


static inline Err _bm_to_source_rec_tag_a_(lxb_dom_node_t* node, Str out[static 1]) {
    try( str_append(out, "<a", 2));
    lxb_dom_attr_t* attr;
    attr = lxb_dom_element_first_attribute(lxb_dom_interface_element(node));
    while (attr) {
        size_t namelen;
        const lxb_char_t* name = lxb_dom_attr_qualified_name(attr, &namelen);
        if (namelen)  {
            try( str_append(out, " ", 1));
            try( str_append(out, (char*)name, namelen));
        }
        size_t valuelen;
        const lxb_char_t* value = lxb_dom_attr_value(attr, &valuelen);
        if (valuelen)  {
            try( str_append(out, "=", 1));
            try( str_append(out, (char*)value, valuelen));
        }
        attr = lxb_dom_element_next_attribute(attr);
    }
    try( str_append(out, ">", 1));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, "</a>", 4));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_ul_(lxb_dom_node_t* node, Str out[static 1]) {
    try( str_append(out, "<ul>\n", 5));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    char* end = "<!--End of section (do not delete this comment)-->\n</ul>\n";
    try( str_append(out, end, strlen(end)));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_li_(lxb_dom_node_t* node, Str out[static 1]) {
    try( str_append(out, "<li>", 4));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    try( str_append(out, "\n", 1));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_h1_(lxb_dom_node_t* node, Str out[static 1]) {
    try( str_append(out, "<h1>", 4));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, "</h1>\n", 6));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_h2_(lxb_dom_node_t* node, Str out[static 1]) {
    try( str_append(out, "<h2>", 4));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, "</h2>\n", 6));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_(lxb_dom_node_t* node, Str out[static 1]) {
    switch(node->local_name) {
        case LXB_TAG_A: return _bm_to_source_rec_tag_a_(node, out);
        case LXB_TAG_H1: return _bm_to_source_rec_tag_h1_(node, out);
        case LXB_TAG_H2: return _bm_to_source_rec_tag_h2_(node, out);
        case LXB_TAG_UL: return _bm_to_source_rec_tag_ul_(node, out);
        case LXB_TAG_LI: return _bm_to_source_rec_tag_li_(node, out);
        default: return _bm_to_source_rec_childs_no_text_(node, out);
    }
    return Ok;
}

static inline Err _bm_to_source_append_text_(lxb_dom_node_t* node, Str out[static 1]) {
    const char* data;
    size_t len;
    try( lexbor_node_get_text(node, &data, &len));
    return str_append(out, (char*)data, len);
}

static inline Err _bm_to_source_rec_(lxb_dom_node_t* node, Str out[static 1]) {
    if (node) {
        switch(node->type) {
            case LXB_DOM_NODE_TYPE_ELEMENT: return _bm_to_source_rec_tag_(node, out);
            case LXB_DOM_NODE_TYPE_TEXT: return _bm_to_source_append_text_(node, out);
            case LXB_DOM_NODE_TYPE_COMMENT: return Ok;
            default: {
                 //TODO: log this or do anythong else?
                //if (node->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY)
                //    log_warn__("lexbor node type greater than last entry: %lx\n", node->type);
                //else log_warn__("%s\n", "Ignored Node Type in Bookmar file");
                return Ok;
            }
        }
    }
    return Ok;
}

static inline Err bookmark_to_source(HtmlDoc bm[static 1], Str out[static 1]) {
    lxb_dom_node_t* body;
    try( bookmark_sections_body(bm, &body));

    char* bm_header = "<html><head><title>Bookmarks</title></head>\n<body>\n";
    Err err = str_append(out, bm_header, strlen(bm_header));
    if (err) {
        str_clean(out);
        return err;
    }
    try( _bm_to_source_rec_childs_no_text_(body, out));
    if (!err) {
        char* bm_header = "</body>\n</html>\n";
        err = str_append(out, bm_header, strlen(bm_header));
    }
    return err;
}

static inline Err
bookmarks_save_to_disc(HtmlDoc bm[static 1], Str bmfile[static 1]) {
    Str source = (Str){0};
    try( bookmark_to_source(bm, &source));

    FILE* fp = fopen(bmfile->items, "w");
    if (!fp) {
        str_clean(&source);
        return err_fmt("error saving bm file '%s': %s", bmfile->items, strerror(errno));
    }
    Err err = Ok;
    size_t written = fwrite(source.items, 1, source.len, fp);
    if (written != source.len) err = "error: could not write to bookmarks file";
    fclose(fp);
    str_clean(&source);
    return err;
}


#endif
