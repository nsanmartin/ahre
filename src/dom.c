#include <lexbor/html/html.h>
#include <lexbor/dom/dom.h>

#include "dom.h"


static Err str_append_wrapped(Str out[_1_], StrView pre, StrView s, StrView post);
static Err _str_append_indent_(Str out[_1_], StrView indentantion, size_t level);


static DomNodeType _lexbor_to_dom_node_type_[] = {
    [LXB_DOM_NODE_TYPE_ELEMENT]                       =       DOM_NODE_TYPE_ELEMENT,
    [LXB_DOM_NODE_TYPE_ATTRIBUTE]                     =       DOM_NODE_TYPE_ATTRIBUTE,
    [LXB_DOM_NODE_TYPE_TEXT]                          =       DOM_NODE_TYPE_TEXT,
    [LXB_DOM_NODE_TYPE_CDATA_SECTION]                 =       DOM_NODE_TYPE_CDATA_SECTION,
    [LXB_DOM_NODE_TYPE_PROCESSING_INSTRUCTION]        =       DOM_NODE_TYPE_PROCESSING_INSTRUCTION,
    [LXB_DOM_NODE_TYPE_COMMENT]                       =       DOM_NODE_TYPE_COMMENT,
    [LXB_DOM_NODE_TYPE_DOCUMENT]                      =       DOM_NODE_TYPE_DOCUMENT,
    [LXB_DOM_NODE_TYPE_DOCUMENT_TYPE]                 =       DOM_NODE_TYPE_DOCUMENT_TYPE,
    [LXB_DOM_NODE_TYPE_DOCUMENT_FRAGMENT]             =       DOM_NODE_TYPE_DOCUMENT_FRAGMENT,
    [LXB_DOM_NODE_TYPE_CHARACTER_DATA]                =       DOM_NODE_TYPE_CHARACTER_DATA,
    [LXB_DOM_NODE_TYPE_SHADOW_ROOT]                   =       DOM_NODE_TYPE_SHADOW_ROOT,
    [LXB_DOM_NODE_TYPE_LAST_ENTRY]                    =       DOM_NODE_TYPE__COUNT_
};

static HtmlTag _lexbor_to_html_tag_[] = {
    [LXB_TAG_A]                     =    HTML_TAG_A,
    [LXB_TAG_ABBR]                  =    HTML_TAG_ABBR,
    [LXB_TAG_ACRONYM]               =    HTML_TAG_ACRONYM,
    [LXB_TAG_ADDRESS]               =    HTML_TAG_ADDRESS,
    [LXB_TAG_ALTGLYPH]              =    HTML_TAG_ALTGLYPH,
    [LXB_TAG_ALTGLYPHDEF]           =    HTML_TAG_ALTGLYPHDEF,
    [LXB_TAG_ALTGLYPHITEM]          =    HTML_TAG_ALTGLYPHITEM,
    [LXB_TAG_ANIMATECOLOR]          =    HTML_TAG_ANIMATECOLOR,
    [LXB_TAG_ANIMATEMOTION]         =    HTML_TAG_ANIMATEMOTION,
    [LXB_TAG_ANIMATETRANSFORM]      =    HTML_TAG_ANIMATETRANSFORM,
    [LXB_TAG_ANNOTATION_XML]        =    HTML_TAG_ANNOTATION_XML,
    [LXB_TAG_APPLET]                =    HTML_TAG_APPLET,
    [LXB_TAG_AREA]                  =    HTML_TAG_AREA,
    [LXB_TAG_ARTICLE]               =    HTML_TAG_ARTICLE,
    [LXB_TAG_ASIDE]                 =    HTML_TAG_ASIDE,
    [LXB_TAG_AUDIO]                 =    HTML_TAG_AUDIO,
    [LXB_TAG_B]                     =    HTML_TAG_B,
    [LXB_TAG_BASE]                  =    HTML_TAG_BASE,
    [LXB_TAG_BASEFONT]              =    HTML_TAG_BASEFONT,
    [LXB_TAG_BDI]                   =    HTML_TAG_BDI,
    [LXB_TAG_BDO]                   =    HTML_TAG_BDO,
    [LXB_TAG_BGSOUND]               =    HTML_TAG_BGSOUND,
    [LXB_TAG_BIG]                   =    HTML_TAG_BIG,
    [LXB_TAG_BLINK]                 =    HTML_TAG_BLINK,
    [LXB_TAG_BLOCKQUOTE]            =    HTML_TAG_BLOCKQUOTE,
    [LXB_TAG_BODY]                  =    HTML_TAG_BODY,
    [LXB_TAG_BR]                    =    HTML_TAG_BR,
    [LXB_TAG_BUTTON]                =    HTML_TAG_BUTTON,
    [LXB_TAG_CANVAS]                =    HTML_TAG_CANVAS,
    [LXB_TAG_CAPTION]               =    HTML_TAG_CAPTION,
    [LXB_TAG_CENTER]                =    HTML_TAG_CENTER,
    [LXB_TAG_CITE]                  =    HTML_TAG_CITE,
    [LXB_TAG_CLIPPATH]              =    HTML_TAG_CLIPPATH,
    [LXB_TAG_CODE]                  =    HTML_TAG_CODE,
    [LXB_TAG_COL]                   =    HTML_TAG_COL,
    [LXB_TAG_COLGROUP]              =    HTML_TAG_COLGROUP,
    [LXB_TAG_DATA]                  =    HTML_TAG_DATA,
    [LXB_TAG_DATALIST]              =    HTML_TAG_DATALIST,
    [LXB_TAG_DD]                    =    HTML_TAG_DD,
    [LXB_TAG_DEL]                   =    HTML_TAG_DEL,
    [LXB_TAG_DESC]                  =    HTML_TAG_DESC,
    [LXB_TAG_DETAILS]               =    HTML_TAG_DETAILS,
    [LXB_TAG_DFN]                   =    HTML_TAG_DFN,
    [LXB_TAG_DIALOG]                =    HTML_TAG_DIALOG,
    [LXB_TAG_DIR]                   =    HTML_TAG_DIR,
    [LXB_TAG_DIV]                   =    HTML_TAG_DIV,
    [LXB_TAG_DL]                    =    HTML_TAG_DL,
    [LXB_TAG_DT]                    =    HTML_TAG_DT,
    [LXB_TAG_EM]                    =    HTML_TAG_EM,
    [LXB_TAG_EMBED]                 =    HTML_TAG_EMBED,
    [LXB_TAG_FEBLEND]               =    HTML_TAG_FEBLEND,
    [LXB_TAG_FECOLORMATRIX]         =    HTML_TAG_FECOLORMATRIX,
    [LXB_TAG_FECOMPONENTTRANSFER]   =    HTML_TAG_FECOMPONENTTRANSFER,
    [LXB_TAG_FECOMPOSITE]           =    HTML_TAG_FECOMPOSITE,
    [LXB_TAG_FECONVOLVEMATRIX]      =    HTML_TAG_FECONVOLVEMATRIX,
    [LXB_TAG_FEDIFFUSELIGHTING]     =    HTML_TAG_FEDIFFUSELIGHTING,
    [LXB_TAG_FEDISPLACEMENTMAP]     =    HTML_TAG_FEDISPLACEMENTMAP,
    [LXB_TAG_FEDISTANTLIGHT]        =    HTML_TAG_FEDISTANTLIGHT,
    [LXB_TAG_FEDROPSHADOW]          =    HTML_TAG_FEDROPSHADOW,
    [LXB_TAG_FEFLOOD]               =    HTML_TAG_FEFLOOD,
    [LXB_TAG_FEFUNCA]               =    HTML_TAG_FEFUNCA,
    [LXB_TAG_FEFUNCB]               =    HTML_TAG_FEFUNCB,
    [LXB_TAG_FEFUNCG]               =    HTML_TAG_FEFUNCG,
    [LXB_TAG_FEFUNCR]               =    HTML_TAG_FEFUNCR,
    [LXB_TAG_FEGAUSSIANBLUR]        =    HTML_TAG_FEGAUSSIANBLUR,
    [LXB_TAG_FEIMAGE]               =    HTML_TAG_FEIMAGE,
    [LXB_TAG_FEMERGE]               =    HTML_TAG_FEMERGE,
    [LXB_TAG_FEMERGENODE]           =    HTML_TAG_FEMERGENODE,
    [LXB_TAG_FEMORPHOLOGY]          =    HTML_TAG_FEMORPHOLOGY,
    [LXB_TAG_FEOFFSET]              =    HTML_TAG_FEOFFSET,
    [LXB_TAG_FEPOINTLIGHT]          =    HTML_TAG_FEPOINTLIGHT,
    [LXB_TAG_FESPECULARLIGHTING]    =    HTML_TAG_FESPECULARLIGHTING,
    [LXB_TAG_FESPOTLIGHT]           =    HTML_TAG_FESPOTLIGHT,
    [LXB_TAG_FETILE]                =    HTML_TAG_FETILE,
    [LXB_TAG_FETURBULENCE]          =    HTML_TAG_FETURBULENCE,
    [LXB_TAG_FIELDSET]              =    HTML_TAG_FIELDSET,
    [LXB_TAG_FIGCAPTION]            =    HTML_TAG_FIGCAPTION,
    [LXB_TAG_FIGURE]                =    HTML_TAG_FIGURE,
    [LXB_TAG_FONT]                  =    HTML_TAG_FONT,
    [LXB_TAG_FOOTER]                =    HTML_TAG_FOOTER,
    [LXB_TAG_FOREIGNOBJECT]         =    HTML_TAG_FOREIGNOBJECT,
    [LXB_TAG_FORM]                  =    HTML_TAG_FORM,
    [LXB_TAG_FRAME]                 =    HTML_TAG_FRAME,
    [LXB_TAG_FRAMESET]              =    HTML_TAG_FRAMESET,
    [LXB_TAG_GLYPHREF]              =    HTML_TAG_GLYPHREF,
    [LXB_TAG_H1]                    =    HTML_TAG_H1,
    [LXB_TAG_H2]                    =    HTML_TAG_H2,
    [LXB_TAG_H3]                    =    HTML_TAG_H3,
    [LXB_TAG_H4]                    =    HTML_TAG_H4,
    [LXB_TAG_H5]                    =    HTML_TAG_H5,
    [LXB_TAG_H6]                    =    HTML_TAG_H6,
    [LXB_TAG_HEAD]                  =    HTML_TAG_HEAD,
    [LXB_TAG_HEADER]                =    HTML_TAG_HEADER,
    [LXB_TAG_HGROUP]                =    HTML_TAG_HGROUP,
    [LXB_TAG_HR]                    =    HTML_TAG_HR,
    [LXB_TAG_HTML]                  =    HTML_TAG_HTML,
    [LXB_TAG_I]                     =    HTML_TAG_I,
    [LXB_TAG_IFRAME]                =    HTML_TAG_IFRAME,
    [LXB_TAG_IMAGE]                 =    HTML_TAG_IMAGE,
    [LXB_TAG_IMG]                   =    HTML_TAG_IMG,
    [LXB_TAG_INPUT]                 =    HTML_TAG_INPUT,
    [LXB_TAG_INS]                   =    HTML_TAG_INS,
    [LXB_TAG_ISINDEX]               =    HTML_TAG_ISINDEX,
    [LXB_TAG_KBD]                   =    HTML_TAG_KBD,
    [LXB_TAG_KEYGEN]                =    HTML_TAG_KEYGEN,
    [LXB_TAG_LABEL]                 =    HTML_TAG_LABEL,
    [LXB_TAG_LEGEND]                =    HTML_TAG_LEGEND,
    [LXB_TAG_LI]                    =    HTML_TAG_LI,
    [LXB_TAG_LINEARGRADIENT]        =    HTML_TAG_LINEARGRADIENT,
    [LXB_TAG_LINK]                  =    HTML_TAG_LINK,
    [LXB_TAG_LISTING]               =    HTML_TAG_LISTING,
    [LXB_TAG_MAIN]                  =    HTML_TAG_MAIN,
    [LXB_TAG_MALIGNMARK]            =    HTML_TAG_MALIGNMARK,
    [LXB_TAG_MAP]                   =    HTML_TAG_MAP,
    [LXB_TAG_MARK]                  =    HTML_TAG_MARK,
    [LXB_TAG_MARQUEE]               =    HTML_TAG_MARQUEE,
    [LXB_TAG_MATH]                  =    HTML_TAG_MATH,
    [LXB_TAG_MENU]                  =    HTML_TAG_MENU,
    [LXB_TAG_META]                  =    HTML_TAG_META,
    [LXB_TAG_METER]                 =    HTML_TAG_METER,
    [LXB_TAG_MFENCED]               =    HTML_TAG_MFENCED,
    [LXB_TAG_MGLYPH]                =    HTML_TAG_MGLYPH,
    [LXB_TAG_MI]                    =    HTML_TAG_MI,
    [LXB_TAG_MN]                    =    HTML_TAG_MN,
    [LXB_TAG_MO]                    =    HTML_TAG_MO,
    [LXB_TAG_MS]                    =    HTML_TAG_MS,
    [LXB_TAG_MTEXT]                 =    HTML_TAG_MTEXT,
    [LXB_TAG_MULTICOL]              =    HTML_TAG_MULTICOL,
    [LXB_TAG_NAV]                   =    HTML_TAG_NAV,
    [LXB_TAG_NEXTID]                =    HTML_TAG_NEXTID,
    [LXB_TAG_NOBR]                  =    HTML_TAG_NOBR,
    [LXB_TAG_NOEMBED]               =    HTML_TAG_NOEMBED,
    [LXB_TAG_NOFRAMES]              =    HTML_TAG_NOFRAMES,
    [LXB_TAG_NOSCRIPT]              =    HTML_TAG_NOSCRIPT,
    [LXB_TAG_OBJECT]                =    HTML_TAG_OBJECT,
    [LXB_TAG_OL]                    =    HTML_TAG_OL,
    [LXB_TAG_OPTGROUP]              =    HTML_TAG_OPTGROUP,
    [LXB_TAG_OPTION]                =    HTML_TAG_OPTION,
    [LXB_TAG_OUTPUT]                =    HTML_TAG_OUTPUT,
    [LXB_TAG_P]                     =    HTML_TAG_P,
    [LXB_TAG_PARAM]                 =    HTML_TAG_PARAM,
    [LXB_TAG_PATH]                  =    HTML_TAG_PATH,
    [LXB_TAG_PICTURE]               =    HTML_TAG_PICTURE,
    [LXB_TAG_PLAINTEXT]             =    HTML_TAG_PLAINTEXT,
    [LXB_TAG_PRE]                   =    HTML_TAG_PRE,
    [LXB_TAG_PROGRESS]              =    HTML_TAG_PROGRESS,
    [LXB_TAG_Q]                     =    HTML_TAG_Q,
    [LXB_TAG_RADIALGRADIENT]        =    HTML_TAG_RADIALGRADIENT,
    [LXB_TAG_RB]                    =    HTML_TAG_RB,
    [LXB_TAG_RP]                    =    HTML_TAG_RP,
    [LXB_TAG_RT]                    =    HTML_TAG_RT,
    [LXB_TAG_RTC]                   =    HTML_TAG_RTC,
    [LXB_TAG_RUBY]                  =    HTML_TAG_RUBY,
    [LXB_TAG_S]                     =    HTML_TAG_S,
    [LXB_TAG_SAMP]                  =    HTML_TAG_SAMP,
    [LXB_TAG_SCRIPT]                =    HTML_TAG_SCRIPT,
    [LXB_TAG_SEARCH]                =    HTML_TAG_SEARCH,
    [LXB_TAG_SECTION]               =    HTML_TAG_SECTION,
    [LXB_TAG_SELECT]                =    HTML_TAG_SELECT,
    [LXB_TAG_SELECTEDCONTENT]       =    HTML_TAG_SELECTEDCONTENT,
    [LXB_TAG_SLOT]                  =    HTML_TAG_SLOT,
    [LXB_TAG_SMALL]                 =    HTML_TAG_SMALL,
    [LXB_TAG_SOURCE]                =    HTML_TAG_SOURCE,
    [LXB_TAG_SPACER]                =    HTML_TAG_SPACER,
    [LXB_TAG_SPAN]                  =    HTML_TAG_SPAN,
    [LXB_TAG_STRIKE]                =    HTML_TAG_STRIKE,
    [LXB_TAG_STRONG]                =    HTML_TAG_STRONG,
    [LXB_TAG_STYLE]                 =    HTML_TAG_STYLE,
    [LXB_TAG_SUB]                   =    HTML_TAG_SUB,
    [LXB_TAG_SUMMARY]               =    HTML_TAG_SUMMARY,
    [LXB_TAG_SUP]                   =    HTML_TAG_SUP,
    [LXB_TAG_SVG]                   =    HTML_TAG_SVG,
    [LXB_TAG_TABLE]                 =    HTML_TAG_TABLE,
    [LXB_TAG_TBODY]                 =    HTML_TAG_TBODY,
    [LXB_TAG_TD]                    =    HTML_TAG_TD,
    [LXB_TAG_TEMPLATE]              =    HTML_TAG_TEMPLATE,
    [LXB_TAG_TEXTAREA]              =    HTML_TAG_TEXTAREA,
    [LXB_TAG_TEXTPATH]              =    HTML_TAG_TEXTPATH,
    [LXB_TAG_TFOOT]                 =    HTML_TAG_TFOOT,
    [LXB_TAG_TH]                    =    HTML_TAG_TH,
    [LXB_TAG_THEAD]                 =    HTML_TAG_THEAD,
    [LXB_TAG_TIME]                  =    HTML_TAG_TIME,
    [LXB_TAG_TITLE]                 =    HTML_TAG_TITLE,
    [LXB_TAG_TR]                    =    HTML_TAG_TR,
    [LXB_TAG_TRACK]                 =    HTML_TAG_TRACK,
    [LXB_TAG_TT]                    =    HTML_TAG_TT,
    [LXB_TAG_U]                     =    HTML_TAG_U,
    [LXB_TAG_UL]                    =    HTML_TAG_UL,
    [LXB_TAG_VAR]                   =    HTML_TAG_VAR,
    [LXB_TAG_VIDEO]                 =    HTML_TAG_VIDEO,
    [LXB_TAG_WBR]                   =    HTML_TAG_WBR,
    [LXB_TAG_XMP]                   =    HTML_TAG_XMP,
    [LXB_TAG__LAST_ENTRY]           =    HTML_TAG__COUNT_
};



/*
 * DOM
 *
 */


DomNode
dom_root(Dom dom) { return (DomNode){.ptr=lxb_dom_interface_node(dom.ptr)}; }

Err
dom_init(Dom d[_1_]) {
    d->ptr = lxb_html_document_create();
    if (!d->ptr) return "error: lxb failed to create html document";
    else return Ok;
}

void dom_reset(Dom d) {
    lxb_html_document_clean(d.ptr);
}

void dom_cleanup(Dom d) {
    lxb_html_document_destroy(d.ptr);
}

Err dom_parse(Dom d, StrView html) {
    if (LXB_STATUS_OK !=
        lxb_html_document_parse(d.ptr, (const lxb_char_t*)html.items, html.len))
        return "error: lexbor reparse failed";
    return Ok;
}

/*
 * DOM NODE
 *
 */

Dom dom_from_ptr(DomPtr ptr) { return (Dom){.ptr=ptr}; }


DomNode
dom_node_prev(DomNode n) { return (DomNode){.ptr=n.ptr->prev}; }

DomNode
dom_node_parent(DomNode n) { return (DomNode){.ptr=n.ptr->parent}; }

DomElem
dom_get_elem_by_id(Dom dom, StrView id) {
    return (DomElem){
        .ptr=lexbor_doc_get_element_by_id(lxb_html_interface_document(dom.ptr), id.items, id.len)
    };
}

bool
dom_node_has_tag(DomNode n, HtmlTag tag) { return _lexbor_to_html_tag_[n.ptr->local_name] == tag; }


bool
dom_node_has_attr(DomNode n, StrView attr) {
    return lxb_dom_element_has_attribute(
        lxb_dom_interface_element(n.ptr),
        (const lxb_char_t*)attr.items,
        attr.len
    );

}


DomAttr
dom_node_first_attr(DomNode node) {
    return (DomAttr){.ptr=lxb_dom_element_first_attribute(lxb_dom_interface_element(node.ptr))};
}


bool
dom_node_type_is_valid(DomNode n) {
    return !(n.ptr->type >= LXB_DOM_NODE_TYPE_LAST_ENTRY);
}


bool
dom_node_tag_is_valid(DomNode n) {
    return LXB_TAG__BEGIN <= n.ptr->local_name
        && LXB_TAG__LAST_ENTRY > n.ptr->local_name
        ;
}

StrView
dom_node_text_view(DomNode node) {
    lxb_dom_text_t* text = lxb_dom_interface_text(node.ptr);
    if(!text) return (StrView){0};
    return (StrView) {
        .items= (const char*)text->char_data.data.data,
        .len = text->char_data.data.length
    };
}


StrView
dom_node_attr_value(DomNode n, StrView attr) {
    return dom_elem_attr_value(dom_elem_from_node(n), attr);
}


bool
dom_node_attr_has_value(DomNode n, StrView attr, StrView value) {
    StrView actual_value = dom_node_attr_value(n, attr);
    return strview_eq_case(value, actual_value);
}


Err dom_node_append_null_terminated_attr_to_str(DomNode node, StrView attr, Str out[_1_]) {
    StrView value = dom_node_attr_value(node, attr);
    if (!value.items || !value.len) return "dom node does not have attr";
    return str_append_z(out, &value);
}

/*
 * DOM ELEMENT
 *
 */

DomElem
dom_elem_from_ptr(DomElemPtr ptr) { return (DomElem){.ptr=ptr}; }

Err dom_elem_init(DomElem de[_1_], Dom dom, StrView strv) {
    de->ptr = lxb_dom_document_create_element(
        &dom.ptr->dom_document, (const lxb_char_t*)strv.items, strv.len, NULL
    );
    if (!de->ptr) return "error: lexbor elem create failure";
    return Ok;
}

void dom_elem_cleanup(DomElem de) { lxb_dom_document_destroy_element(de.ptr); }


void dom_node_insert_child_node(DomNode node, DomNode child) {
    lxb_dom_node_insert_child(node.ptr, child.ptr);
}

HtmlTag dom_node_tag(DomNode n) {
    size_t ix = n.ptr->local_name;
    if (sizeof(_lexbor_to_html_tag_) <= ix) return HTML_TAG__INVALID_;
    return _lexbor_to_html_tag_[ix];
}
DomNodeType dom_node_type(DomNode n) {
    size_t ix = n.ptr->type;
    if (sizeof(_lexbor_to_dom_node_type_) <= ix) return DOM_NODE_TYPE__INVALID_;
    return _lexbor_to_dom_node_type_[ix];
}


StrView dom_elem_name_view(DomElem elem) {
    size_t len;
    char* tag = (char*)lxb_dom_element_qualified_name(lxb_dom_interface_element(elem.ptr), &len);
    return sv(tag, len);
}

StrView dom_elem_attr_value(DomElem elem, StrView attr) {
    StrView res = (StrView){0};
    if (attr.items && attr.len) {
        res.items = (const char*)lxb_dom_element_get_attribute(
            elem.ptr, (const lxb_char_t*)attr.items, attr.len, &res.len
        );
    }
    return res;
}




static Err str_append_wrapped(Str out[_1_], StrView pre, StrView s, StrView post) {
    try( str_append(out, &pre));
    try( str_append(out, &s));
    try( str_append(out, &post));
    return Ok;
}

static Err _str_append_indent_(Str out[_1_], StrView indentantion, size_t level) {
    for (size_t i = 0; i <= level; ++i) try( str_append(out, &indentantion));
    return Ok;
}

DomElem dom_elem_from_node(DomNode n) { return (DomElem){.ptr=lxb_dom_interface_element(n.ptr)}; }
#define DOM_NODE_TO_STR_INDENTATION svl("    ")
static Err _dom_node_to_str_impl_(DomNode node, Str buf[_1_], size_t level) {
    if (dom_node_type(node) != DOM_NODE_TYPE_ELEMENT) return Ok;

    DomElem elem = dom_elem_from_node(node);

    StrView tag = dom_elem_name_view(elem);
    if (tag.items && tag.len) {
        try (_str_append_indent_(buf, DOM_NODE_TO_STR_INDENTATION, level));
        try( str_append_wrapped(buf, svl("<"), tag, svl(">\n")));
    }


    DomAttr attr = dom_elem_first_attr(elem);
    for (;!isnull(attr);  attr = dom_attr_next(attr)) {
        StrView name = dom_attr_name_view(attr);
        if (!name.len || !name.items) return "error: expecting name for attr but got null";
        StrView value = dom_attr_value_view(attr);

        try (_str_append_indent_(buf, DOM_NODE_TO_STR_INDENTATION, level));
        try( str_append(buf, &name));
        try( str_append(buf, svl("=")));
        if (value.len && value.items) try( str_append_ln(buf, &value));
        else try( str_append_ln(buf, svl("(null)")));
    }

    for (DomNode it = dom_node_first_child(node); !isnull(it); it = dom_node_next(it)) {
        _dom_node_to_str_impl_(it, buf, level + 1);
        if (dom_node_eq(it, dom_node_last_child(node))) break;
    }

    if (tag.items && tag.len) {
        try (_str_append_indent_(buf, DOM_NODE_TO_STR_INDENTATION, level));
        try( str_append_wrapped(buf, svl("<"), tag, svl("/>\n")));
    }
    return Ok;
}

Err dom_node_to_str(DomNode node, Str buf[_1_]) {
    return _dom_node_to_str_impl_(node, buf, 0);
}


Err dom_elem_set_attr(DomElem de, StrView key, StrView value) {
    if (!key.items || !key.len) return "error: attr is NULL or len is 0";
    lxb_dom_attr_t * after = lxb_dom_element_set_attribute(
        de.ptr,
        (const lxb_char_t*)key.items,
        key.len,
        (const lxb_char_t*)value.items,
        value.len
    );
    if (!after) return "error: value attr could not be set";
    return Ok;
}


Err dom_node_set_attr(DomNode n, StrView key, StrView value) {
    return dom_elem_set_attr(dom_elem_from_node(n), key, value);
}


Err dom_node_remove_attr(DomNode n, StrView attr) {
    lxb_status_t status = lxb_dom_element_remove_attribute(
        dom_elem_from_node(n).ptr,
        (lxb_char_t*)attr.items,
        attr.len
    );
    return LXB_STATUS_OK == status ? Ok : "error: could not set element's attribte";
}


static inline void
_search_title_rec_(lxb_dom_node_t* node, lxb_dom_node_t* title[_1_]) {
    if (!node) return;
    else if (node->local_name == LXB_TAG_TITLE) *title = node; 
    else {
        for(lxb_dom_node_t* it = node->first_child; it ; it = it->next) {
            _search_title_rec_(it, title);
            if (*title) break;
        }
    }
    return;
}


Err dom_get_title_node(Dom dom, DomNode title[_1_]) {
    lxb_dom_node_t* node = dom_root(dom).ptr;
    if (!node) return "error: no document";
    *title = (DomNode){0};
    _search_title_rec_(node, &title->ptr);
    return Ok;
}


Err dom_get_title_text_line(Dom dom, Str* out) {
    DomNode title;
    dom_get_title_node(dom, &title);
    if (isnull(title)) return Ok;
    DomNode node = dom_node_first_child(title); 
    try(strview_join_lines_to_str(dom_node_text_view(node), out));
    return Ok;
}

/*
 * DOM ATTRIBUTE
 *
 */

bool
dom_attr_has_owner(DomAttr attr) {
    return attr.ptr
        && attr.ptr->node.owner_document
        ;
}


DomAttr
dom_elem_first_attr(DomElem e) {
    return (DomAttr){.ptr=lxb_dom_element_first_attribute(e.ptr)};
}


DomAttr
dom_attr_next(DomAttr attr) {
    return (DomAttr){.ptr=lxb_dom_element_next_attribute(attr.ptr)};
}


StrView
dom_attr_name_view(DomAttr attr) {
    size_t namelen;
    const lxb_char_t* name = lxb_dom_attr_qualified_name(attr.ptr, &namelen);
    return (StrView){.items=(const char*)name, .len=namelen};
}


StrView
dom_attr_value_view(DomAttr attr) {
    size_t valuelen;
    const lxb_char_t* value = lxb_dom_attr_value(attr.ptr, &valuelen);
    return (StrView){.items=(const char*)value, .len=valuelen};
}

/* DOM TEXT */
Err
dom_text_init(DomText dt[_1_], Dom dom, StrView strv) {
    dt->ptr = lxb_dom_document_create_text_node(
        &dom.ptr->dom_document, (const lxb_char_t*)strv.items, strv.len
    );
    if (!dt->ptr) return "error: lexbor text create failure";
    return Ok;
}


void
dom_text_cleanup(DomText dt) {
    lxb_dom_node_destroy(lxb_dom_interface_node(dt.ptr));
}


DomNode dom_node_find_parent_form(DomNode n) {
    for (;!isnull(n); n = dom_node_parent(n)) {
        if (dom_node_has_tag(n, HTML_TAG_FORM)) break;
    }
    return n;
}
