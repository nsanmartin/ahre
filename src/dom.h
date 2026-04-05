#ifndef __DOM_WRAPPER_AHRE_H__
#define __DOM_WRAPPER_AHRE_H__

#include "wrapper-lexbor.h"

typedef struct { lxb_html_document_t* ptr; } Dom;
typedef struct { lxb_dom_node_t* ptr; }      DomNode;
typedef struct { lxb_dom_element_t* ptr; }   DomElem;
typedef struct { lxb_dom_text_t* ptr; }      DomText;
typedef struct { lxb_dom_attr_t* ptr; }      DomAttr;

#define T DomNode
#include <arl.h>

/* DOM */
Dom     dom_from_ptr(DomPtr ptr);
DomElem dom_get_elem_by_id(Dom dom, StrView id);
DomNode dom_root(Dom dom);
Err     dom_get_title_node(Dom dom, DomNode node[_1_]);
Err     dom_get_title_text_line(Dom dom, Str* out);
Err     dom_init(Dom d[_1_]);
Err     dom_parse(Dom d, StrView html);
void    dom_cleanup(Dom d);
void    dom_reset(Dom d);


/* DOM NODE */

DomNode dom_node_prev(DomNode n);
DomNode dom_node_parent(DomNode n);
DomNode dom_node_find_parent_form(DomNode node);
Err     dom_node_append_null_terminated_attr_to_str(DomNode node, StrView attr, Str out[_1_]);
Err     dom_node_remove_attr(DomNode n, StrView attr);
Err     dom_node_set_attr(DomNode n, StrView key, StrView value);
Err     dom_node_to_str(DomNode node, Str buf[_1_]);
StrView dom_node_attr_value(DomNode n, StrView attr);
StrView dom_node_text_view(DomNode node);
bool    dom_node_attr_has_value(DomNode n, StrView attr, StrView value);
bool    dom_node_has_attr(DomNode n, StrView attr);
bool    dom_node_tag_is_valid(DomNode n);
bool    dom_node_type_is_valid(DomNode n);
void    dom_node_insert_child_node(DomNode node, DomNode child);

#define dom_node_insert_child(Dn, Dch) \
    dom_node_insert_child_node(\
        (DomNode){.ptr=lxb_dom_interface_node(Dn.ptr)},\
        (DomNode){.ptr=lxb_dom_interface_node(Dch.ptr)}\
    )


/* DOM ELEMENT */
DomAttr dom_elem_first_attr(DomElem e);
DomElem dom_elem_from_node(DomNode n);
DomElem dom_elem_from_ptr(DomElemPtr ptr);
Err     dom_elem_init(DomElem de[_1_], Dom dom, StrView strv);
Err     dom_elem_set_attr(DomElem de, StrView key, StrView value);
StrView dom_elem_name_view(DomElem elem);
StrView dom_elem_attr_value(DomElem elem, StrView attr);
void    dom_elem_cleanup(DomElem de);

/* DOM ATTRIBUTE */
bool dom_attr_has_owner(DomAttr attr);

/* DOM TEXT */
Err dom_text_init(DomText dt[_1_], Dom dom, StrView strv);
void dom_text_cleanup(DomText dt);



typedef enum {
    DOM_NODE_TYPE_ELEMENT                = 0,
    DOM_NODE_TYPE_ATTRIBUTE,
    DOM_NODE_TYPE_TEXT,
    DOM_NODE_TYPE_CDATA_SECTION,
    DOM_NODE_TYPE_PROCESSING_INSTRUCTION,
    DOM_NODE_TYPE_COMMENT,
    DOM_NODE_TYPE_DOCUMENT,
    DOM_NODE_TYPE_DOCUMENT_TYPE,
    DOM_NODE_TYPE_DOCUMENT_FRAGMENT,
    DOM_NODE_TYPE_CHARACTER_DATA,
    DOM_NODE_TYPE_SHADOW_ROOT,
    DOM_NODE_TYPE__COUNT_,
    DOM_NODE_TYPE__INVALID_              = DOM_NODE_TYPE__COUNT_
} DomNodeType;

typedef enum {
    HTML_TAG_A                  = 0,
    HTML_TAG_ABBR,
    HTML_TAG_ACRONYM,
    HTML_TAG_ADDRESS,
    HTML_TAG_ALTGLYPH,
    HTML_TAG_ALTGLYPHDEF,
    HTML_TAG_ALTGLYPHITEM,
    HTML_TAG_ANIMATECOLOR,
    HTML_TAG_ANIMATEMOTION,
    HTML_TAG_ANIMATETRANSFORM,
    HTML_TAG_ANNOTATION_XML,
    HTML_TAG_APPLET,
    HTML_TAG_AREA,
    HTML_TAG_ARTICLE,
    HTML_TAG_ASIDE,
    HTML_TAG_AUDIO,
    HTML_TAG_B,
    HTML_TAG_BASE,
    HTML_TAG_BASEFONT,
    HTML_TAG_BDI,
    HTML_TAG_BDO,
    HTML_TAG_BGSOUND,
    HTML_TAG_BIG,
    HTML_TAG_BLINK,
    HTML_TAG_BLOCKQUOTE,
    HTML_TAG_BODY,
    HTML_TAG_BR,
    HTML_TAG_BUTTON,
    HTML_TAG_CANVAS,
    HTML_TAG_CAPTION,
    HTML_TAG_CENTER,
    HTML_TAG_CITE,
    HTML_TAG_CLIPPATH,
    HTML_TAG_CODE,
    HTML_TAG_COL,
    HTML_TAG_COLGROUP,
    HTML_TAG_DATA,
    HTML_TAG_DATALIST,
    HTML_TAG_DD,
    HTML_TAG_DEL,
    HTML_TAG_DESC,
    HTML_TAG_DETAILS,
    HTML_TAG_DFN,
    HTML_TAG_DIALOG,
    HTML_TAG_DIR,
    HTML_TAG_DIV,
    HTML_TAG_DL,
    HTML_TAG_DT,
    HTML_TAG_EM,
    HTML_TAG_EMBED,
    HTML_TAG_FEBLEND,
    HTML_TAG_FECOLORMATRIX,
    HTML_TAG_FECOMPONENTTRANSFER,
    HTML_TAG_FECOMPOSITE,
    HTML_TAG_FECONVOLVEMATRIX,
    HTML_TAG_FEDIFFUSELIGHTING,
    HTML_TAG_FEDISPLACEMENTMAP,
    HTML_TAG_FEDISTANTLIGHT,
    HTML_TAG_FEDROPSHADOW,
    HTML_TAG_FEFLOOD,
    HTML_TAG_FEFUNCA,
    HTML_TAG_FEFUNCB,
    HTML_TAG_FEFUNCG,
    HTML_TAG_FEFUNCR,
    HTML_TAG_FEGAUSSIANBLUR,
    HTML_TAG_FEIMAGE,
    HTML_TAG_FEMERGE,
    HTML_TAG_FEMERGENODE,
    HTML_TAG_FEMORPHOLOGY,
    HTML_TAG_FEOFFSET,
    HTML_TAG_FEPOINTLIGHT,
    HTML_TAG_FESPECULARLIGHTING,
    HTML_TAG_FESPOTLIGHT,
    HTML_TAG_FETILE,
    HTML_TAG_FETURBULENCE,
    HTML_TAG_FIELDSET,
    HTML_TAG_FIGCAPTION,
    HTML_TAG_FIGURE,
    HTML_TAG_FONT,
    HTML_TAG_FOOTER,
    HTML_TAG_FOREIGNOBJECT,
    HTML_TAG_FORM,
    HTML_TAG_FRAME,
    HTML_TAG_FRAMESET,
    HTML_TAG_GLYPHREF,
    HTML_TAG_H1,
    HTML_TAG_H2,
    HTML_TAG_H3,
    HTML_TAG_H4,
    HTML_TAG_H5,
    HTML_TAG_H6,
    HTML_TAG_HEAD,
    HTML_TAG_HEADER,
    HTML_TAG_HGROUP,
    HTML_TAG_HR,
    HTML_TAG_HTML,
    HTML_TAG_I,
    HTML_TAG_IFRAME,
    HTML_TAG_IMAGE,
    HTML_TAG_IMG,
    HTML_TAG_INPUT,
    HTML_TAG_INS,
    HTML_TAG_ISINDEX,
    HTML_TAG_KBD,
    HTML_TAG_KEYGEN,
    HTML_TAG_LABEL,
    HTML_TAG_LEGEND,
    HTML_TAG_LI,
    HTML_TAG_LINEARGRADIENT,
    HTML_TAG_LINK,
    HTML_TAG_LISTING,
    HTML_TAG_MAIN,
    HTML_TAG_MALIGNMARK,
    HTML_TAG_MAP,
    HTML_TAG_MARK,
    HTML_TAG_MARQUEE,
    HTML_TAG_MATH,
    HTML_TAG_MENU,
    HTML_TAG_META,
    HTML_TAG_METER,
    HTML_TAG_MFENCED,
    HTML_TAG_MGLYPH,
    HTML_TAG_MI,
    HTML_TAG_MN,
    HTML_TAG_MO,
    HTML_TAG_MS,
    HTML_TAG_MTEXT,
    HTML_TAG_MULTICOL,
    HTML_TAG_NAV,
    HTML_TAG_NEXTID,
    HTML_TAG_NOBR,
    HTML_TAG_NOEMBED,
    HTML_TAG_NOFRAMES,
    HTML_TAG_NOSCRIPT,
    HTML_TAG_OBJECT,
    HTML_TAG_OL,
    HTML_TAG_OPTGROUP,
    HTML_TAG_OPTION,
    HTML_TAG_OUTPUT,
    HTML_TAG_P,
    HTML_TAG_PARAM,
    HTML_TAG_PATH,
    HTML_TAG_PICTURE,
    HTML_TAG_PLAINTEXT,
    HTML_TAG_PRE,
    HTML_TAG_PROGRESS,
    HTML_TAG_Q,
    HTML_TAG_RADIALGRADIENT,
    HTML_TAG_RB,
    HTML_TAG_RP,
    HTML_TAG_RT,
    HTML_TAG_RTC,
    HTML_TAG_RUBY,
    HTML_TAG_S,
    HTML_TAG_SAMP,
    HTML_TAG_SCRIPT,
    HTML_TAG_SEARCH,
    HTML_TAG_SECTION,
    HTML_TAG_SELECT,
    HTML_TAG_SELECTEDCONTENT,
    HTML_TAG_SLOT,
    HTML_TAG_SMALL,
    HTML_TAG_SOURCE,
    HTML_TAG_SPACER,
    HTML_TAG_SPAN,
    HTML_TAG_STRIKE,
    HTML_TAG_STRONG,
    HTML_TAG_STYLE,
    HTML_TAG_SUB,
    HTML_TAG_SUMMARY,
    HTML_TAG_SUP,
    HTML_TAG_SVG,
    HTML_TAG_TABLE,
    HTML_TAG_TBODY,
    HTML_TAG_TD,
    HTML_TAG_TEMPLATE,
    HTML_TAG_TEXTAREA,
    HTML_TAG_TEXTPATH,
    HTML_TAG_TFOOT,
    HTML_TAG_TH,
    HTML_TAG_THEAD,
    HTML_TAG_TIME,
    HTML_TAG_TITLE,
    HTML_TAG_TR,
    HTML_TAG_TRACK,
    HTML_TAG_TT,
    HTML_TAG_U,
    HTML_TAG_UL,
    HTML_TAG_VAR,
    HTML_TAG_VIDEO,
    HTML_TAG_WBR,
    HTML_TAG_XMP,
    HTML_TAG__COUNT_,
    HTML_TAG__INVALID_              = HTML_TAG__COUNT_
} HtmlTag;

HtmlTag dom_node_tag(DomNode n);
DomNodeType dom_node_type(DomNode n);



static inline DomNode dom_node_first_child(DomNode n) { return (DomNode){.ptr=n.ptr->first_child}; }
static inline DomNode dom_node_last_child(DomNode n)  { return (DomNode){.ptr=n.ptr->last_child}; }
static inline DomNode dom_node_next(DomNode n){ return (DomNode){.ptr=n.ptr->next}; } 

static inline bool dom_node_eq(DomNode n, DomNode m) { return n.ptr == m.ptr; }

bool dom_node_has_tag(DomNode n, HtmlTag tag);
static inline bool dom_node_has_tag_body(DomNode n) { return n.ptr->local_name == LXB_TAG_BODY; }
static inline bool dom_node_has_tag_form(DomNode n) { return n.ptr->local_name == LXB_TAG_FORM; }
static inline bool dom_node_has_tag_head(DomNode n) { return n.ptr->local_name == LXB_TAG_HEAD; }
static inline bool dom_node_has_tag_html(DomNode n) { return n.ptr->local_name == LXB_TAG_HTML; }
static inline bool dom_node_has_tag_input(DomNode n) { return n.ptr->local_name == LXB_TAG_INPUT; }
static inline bool dom_node_has_tag_option(DomNode n) { return n.ptr->local_name == LXB_TAG_OPTION; }
static inline bool dom_node_has_tag_select(DomNode n) { return n.ptr->local_name == LXB_TAG_SELECT; }

static inline bool dom_node_has_type_document(DomNode n) {
    return n.ptr->type == LXB_DOM_NODE_TYPE_DOCUMENT;
}

static inline bool dom_node_has_type_text(DomNode n){return n.ptr->type == LXB_DOM_NODE_TYPE_TEXT;}

DomAttr dom_node_first_attr(DomNode node);

DomAttr dom_attr_next(DomAttr attr);
StrView dom_attr_name_view(DomAttr attr);
StrView dom_attr_value_view(DomAttr attr);

#endif
