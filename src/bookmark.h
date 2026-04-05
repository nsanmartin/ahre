#ifndef AHRE_BOOKMARK_H__
#define AHRE_BOOKMARK_H__

#include <errno.h>

#include "dom.h"
#include "utils.h"
#include "htmldoc.h"
#include "error.h"
#include "fetch-history.h"
#include "dom.h"

Err cmd_bookmarks(CmdParams p[_1_]);

#define AHRE_BOOKMARK_HEAD "<html><head><title>Bookmarks</title></head>\n<body>\n"
#define AHRE_BOOKMARK_TAIL "</body>\n</html>\n"
#define EMPTY_BOOKMARK AHRE_BOOKMARK_HEAD "<h1>Bookmarks</h1>\n" AHRE_BOOKMARK_TAIL

Err bookmark_sections_body(HtmlDoc bookmark[_1_], DomNode out[_1_]);

static inline Err bookmark_sections(DomNode body, ArlOf(Str)* out) {
    for (DomNode it = dom_node_first_child(body)
        ; !dom_node_eq(it,dom_node_last_child(body))
        ; it = dom_node_next(it)
    ) {
        if (dom_node_has_tag(it, HTML_TAG_H2)) {
            if (!dom_node_eq(dom_node_first_child(it), dom_node_last_child(it)))
                return "invalid bookmark file";

            Str* buf = arlfn(Str,append)(out, &(Str){0});
            if (!buf) return "error: arl append failure";

            StrView data = dom_node_text_view(dom_node_first_child(it));
            if (data.len) try( str_append(buf, &data));
        }
    }

    return Ok;
}

static inline void bookmark_sections_sort(ArlOf(BufOf(char))* list) {
    qsort(list->items, list->len, sizeof(BufOf(char)), buf_of_char_cmp);
}

static inline Err bookmark_section_insert(Dom dom, DomNode body, const char* q, DomElem bm_entry) {
    DomElem section;
    try(dom_elem_init(&section, dom, svl("h2")));
    DomText text;
    Err err =  dom_text_init(&text, dom, sv(q));
    if (err) goto Clean_Section;

    dom_node_insert_child(section, text);
    dom_node_insert_child(body, section);

    DomElem ul;
    err = dom_elem_init(&ul, dom, svl("ul"));
    if (err) goto Clean_Text;
    dom_node_insert_child(ul, bm_entry);
    dom_node_insert_child(body, ul);

    return Ok;

Clean_Text:
    dom_text_cleanup(text);
Clean_Section:
    dom_elem_cleanup(section);

    return err;
}


static inline Err
bookmark_section_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix) {
    DomNode res = (DomNode){0};
    for (DomNode it = dom_node_first_child(body)
        ; !dom_node_eq(it, dom_node_last_child(body))
        ; it = dom_node_next(it)
    ) {
        if (dom_node_has_tag(it, HTML_TAG_H2)) {
            if (!dom_node_eq(dom_node_first_child(it),dom_node_last_child(it)))
                return "invalid bookmark file";

            StrView data = dom_node_text_view(dom_node_first_child(it));
            if (data.len) {
                size_t qlen = strlen(q);
                size_t len = match_prefix ? qlen : data.len;
                if (qlen <= len && strncmp(q, data.items, len) == 0) {
                    if (!isnull(res)) return "unequivocal reference to bookmark section";
                    res = it;
                }
            }
        }
    }
    *out = res;
    return Ok;
}

static inline Err
bookmark_section_ul_get(DomNode body, const char* q, DomNode out[_1_], bool match_prefix) {
    *out = (DomNode){0};
    DomNode section;
    try( bookmark_section_get(body, q, &section, match_prefix));
    if (!isnull(section)) {
        if (isnull(dom_node_next(section))) return "error: expecting section's next (TEXT)";
        DomNode ul = dom_node_next(dom_node_next(section));//section->next->next;
        if (isnull(ul)) return "error: expecting section's next next";
        if (!dom_node_has_tag(ul, HTML_TAG_UL))
            return "error: expecting section's next next to be ul";
        *out = ul;
    }
    return Ok;
}

static inline Err bookmark_mk_anchor (Dom dom, char* href, Str* text, DomElem out[_1_]) {
    try(dom_elem_init(out, dom, svl("a")));
    try(dom_elem_set_attr(*out, svl("href"), sv(href, strlen(href))));
    DomText dom_text;
    try(dom_text_init(&dom_text, dom, sv(text)));
    if (isnull(dom_text)) return "error: lxb text create failure";//TODO: clean *out

    dom_node_insert_child((*out), dom_text);
    return Ok;
}

static inline Err
bookmark_mk_entry(Dom document, char* href, Str* text, DomElem out[_1_]) {
    try( dom_elem_init(out, document, svl("li")));

    DomElem a;
    Err err = bookmark_mk_anchor(document, href, text, &a);
    if (err) {
        dom_elem_cleanup(*out);
        return err;
    }

    dom_node_insert_child((*out), a);
    return Ok;
}



static inline Err _bm_to_source_rec_(DomNode node, Str out[_1_]) ;

static inline Err _bm_to_source_rec_childs_(DomNode node, Str out[_1_]) {
    for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it)) {
        try( _bm_to_source_rec_(it, out));
        if (dom_node_eq(it, dom_node_last_child(node))) break;
    }
    return Ok;
}


static inline Err _bm_to_source_rec_childs_no_text_(DomNode node, Str out[_1_]) { for(DomNode it = dom_node_first_child(node); !isnull(it) ; it = dom_node_next(it)) {
        if (dom_node_has_type_text(it)) continue; 
        try( _bm_to_source_rec_(it, out));
        if (dom_node_eq(it, dom_node_last_child(node))) break;
    }
    return Ok;
}


static inline Err _bm_to_source_rec_tag_a_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<a")));
    DomAttr attr = dom_node_first_attr(node);
    while (!isnull(attr)) {
        StrView name = dom_attr_name_view(attr);
        if (name.len)  {
            try( str_append(out, svl(" ")));
            try( str_append(out, &name));
        }
        StrView value = dom_attr_value_view(attr);
        if (value.len)  {
            try( str_append(out, svl("=")));
            try( str_append(out, &value));
        }
        attr = dom_attr_next(attr);
    }
    try( str_append(out, svl(">")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</a>")));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_ul_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<ul>\n")));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    char* end = "<!--End of section (do not delete this comment)-->\n</ul>\n";
    try( str_append(out, end));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_li_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<li>")));
    try( _bm_to_source_rec_childs_no_text_(node, out));
    try( str_append(out, svl("\n")));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_h1_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<h1>")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</h1>\n")));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_h2_(DomNode node, Str out[_1_]) {
    try( str_append(out, svl("<h2>")));
    try( _bm_to_source_rec_childs_(node, out));
    try( str_append(out, svl("</h2>\n")));
    return Ok;
}

static inline Err _bm_to_source_rec_tag_(DomNode node, Str out[_1_]) {
    switch(dom_node_tag(node)) {
        case HTML_TAG_A: return _bm_to_source_rec_tag_a_(node, out);
        case HTML_TAG_H1: return _bm_to_source_rec_tag_h1_(node, out);
        case HTML_TAG_H2: return _bm_to_source_rec_tag_h2_(node, out);
        case HTML_TAG_UL: return _bm_to_source_rec_tag_ul_(node, out);
        case HTML_TAG_LI: return _bm_to_source_rec_tag_li_(node, out);
        default: return _bm_to_source_rec_childs_no_text_(node, out);
    }
    return Ok;
}

static inline Err _bm_to_source_append_text_(DomNode node, Str out[_1_]) {
    StrView data = dom_node_text_view(node);
    return str_append(out, &data);
}

static inline Err _bm_to_source_rec_(DomNode node, Str out[_1_]) {
    if (!isnull(node)) {
        switch(dom_node_type(node)) {
            case DOM_NODE_TYPE_ELEMENT: return _bm_to_source_rec_tag_(node, out);
            case DOM_NODE_TYPE_TEXT: return _bm_to_source_append_text_(node, out);
            case DOM_NODE_TYPE_COMMENT: return Ok;
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

static inline Err bookmark_to_source(HtmlDoc bm[_1_], Str out[_1_]) {
    DomNode body;
    try( bookmark_sections_body(bm, &body));

    Err err = str_append(out, svl(AHRE_BOOKMARK_HEAD));
    if (err) {
        str_clean(out);
        return err;
    }
    try( _bm_to_source_rec_childs_no_text_(body, out));
    if (!err) {
        err = str_append(out, svl(AHRE_BOOKMARK_TAIL));
    }
    return err;
}

static inline Err
bookmarks_save_to_disc(HtmlDoc bm[_1_], StrView bm_path) {
    Str source = (Str){0};
    try( bookmark_to_source(bm, &source));

    FILE* fp;
    try(file_open(bm_path.items, "w", &fp));

    Err err = Ok;
    size_t written = fwrite(source.items, 1, source.len, fp);
    if (written != source.len) err = "error: could not write to bookmarks file";
    try(file_close(fp));
    str_clean(&source);
    return err;
}


Err get_bookmarks_doc(
    UrlClient   url_client[_1_],
    StrView     bm_path,
    CmdOut      cmd_out[_1_],
    HtmlDoc     htmldoc_out[_1_]
);
#endif
