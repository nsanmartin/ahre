#ifndef __DOM_WRAPPER_AHRE_H__
#define __DOM_WRAPPER_AHRE_H__

#include "wrapper-lexbor.h"

typedef struct { lxb_html_document_t* ptr; } Dom;
typedef struct { DomNodePtr ptr; }           DomNode;
typedef struct { DomElemPtr ptr; }           DomElem;
typedef struct { lxb_dom_text_t* ptr; }      DomText;
typedef struct { lxb_dom_attr_t* ptr; }      DomAttr;

#define T DomNode
#include <arl.h>

/* DOM */
Dom     dom_from_ptr(DomPtr ptr);
DomElem dom_get_elem_by_id(Dom dom, StrView id);
DomNode dom_root(Dom dom);
Err     dom_get_title_node(Dom dom, DomNode node[_1_]);
Err     dom_get_title_elem(Dom dom, DomElem node[_1_]);
Err     dom_get_title_text_line(Dom dom, Str* out);
Err     dom_init(Dom d[_1_]);
Err     dom_parse(Dom d, StrView html);
void    dom_cleanup(Dom d);
void    dom_reset(Dom d);
Err     dom_get_head_elem(Dom dom, DomElem head[_1_]);
Err     dom_get_body_elem(Dom dom, DomElem body[_1_]);


/* DOM NODE */

DomNode dom_node_prev(DomNode n);
DomNode dom_node_parent(DomNode n);
DomNode dom_node_find_parent_form(DomNode node);
DomNode dom_node_from_elem(DomElem node);
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
DomNode dom_skip_text(DomNode n);
DomNode dom_node_first_elem_child(DomNode n);

#define dom_node_insert_child(Dn, Dch) \
    dom_node_insert_child_node(\
        (DomNode){.ptr=lxb_dom_interface_node(Dn.ptr)},\
        (DomNode){.ptr=lxb_dom_interface_node(Dch.ptr)}\
    )


/* DOM ELEMENT */
DomAttr dom_elem_first_attr(DomElem e);
DomElem dom_elem_from_node(DomNode n);
DomElem dom_elem_from_nodeptr(DomNodePtr n);
DomElem dom_elem_from_ptr(DomElemPtr ptr);
Err     dom_elem_init(DomElem de[_1_], Dom dom, StrView strv);
Err     dom_elem_set_attr(DomElem de, StrView key, StrView value);
StrView dom_elem_name_view(DomElem elem);
StrView dom_elem_attr_value(DomElem elem, StrView attr);
void    dom_elem_cleanup(DomElem de);
Err     dom_elem_set_text_content(DomElem elem, StrView text);
Err     dom_elem_get_text_content(DomElem elem, Str *out);

/* DOM ATTRIBUTE */
bool dom_attr_has_owner(DomAttr attr);

/* DOM TEXT */
Err dom_text_init(DomText dt[_1_], Dom dom, StrView strv);
void dom_text_cleanup(DomText dt);



typedef enum {
    DOM_NODE_TYPE_ELEMENT                = 0x0,
    DOM_NODE_TYPE_ATTRIBUTE              = 0x1,
    DOM_NODE_TYPE_TEXT                   = 0x2,
    DOM_NODE_TYPE_CDATA_SECTION          = 0x3,
    DOM_NODE_TYPE_PROCESSING_INSTRUCTION = 0x4,
    DOM_NODE_TYPE_COMMENT                = 0x5,
    DOM_NODE_TYPE_DOCUMENT               = 0x6,
    DOM_NODE_TYPE_DOCUMENT_TYPE          = 0x7,
    DOM_NODE_TYPE_DOCUMENT_FRAGMENT      = 0x8,
    DOM_NODE_TYPE_CHARACTER_DATA         = 0x9,
    DOM_NODE_TYPE_SHADOW_ROOT            = 0xA,
    DOM_NODE_TYPE__COUNT_                = 0xB,
    DOM_NODE_TYPE__INVALID_              = DOM_NODE_TYPE__COUNT_
} DomNodeType;

typedef enum {
    HTML_TAG_A                   = 0x1,
    HTML_TAG_ABBR                = 0x2,
    HTML_TAG_ACRONYM             = 0x3,
    HTML_TAG_ADDRESS             = 0x4,
    HTML_TAG_ALTGLYPH            = 0x5,
    HTML_TAG_ALTGLYPHDEF         = 0x6,
    HTML_TAG_ALTGLYPHITEM        = 0x7,
    HTML_TAG_ANIMATECOLOR        = 0x8,
    HTML_TAG_ANIMATEMOTION       = 0x9,
    HTML_TAG_ANIMATETRANSFORM    = 0xA,
    HTML_TAG_ANNOTATION_XML      = 0xB,
    HTML_TAG_APPLET              = 0xC,
    HTML_TAG_AREA                = 0xD,
    HTML_TAG_ARTICLE             = 0xE,
    HTML_TAG_ASIDE               = 0xF,
    HTML_TAG_AUDIO               = 0x10,
    HTML_TAG_B                   = 0x11,
    HTML_TAG_BASE                = 0x12,
    HTML_TAG_BASEFONT            = 0x13,
    HTML_TAG_BDI                 = 0x14,
    HTML_TAG_BDO                 = 0x15,
    HTML_TAG_BGSOUND             = 0x16,
    HTML_TAG_BIG                 = 0x17,
    HTML_TAG_BLINK               = 0x18,
    HTML_TAG_BLOCKQUOTE          = 0x19,
    HTML_TAG_BODY                = 0x1A,
    HTML_TAG_BR                  = 0x1B,
    HTML_TAG_BUTTON              = 0x1C,
    HTML_TAG_CANVAS              = 0x1D,
    HTML_TAG_CAPTION             = 0x1E,
    HTML_TAG_CENTER              = 0x1F,
    HTML_TAG_CITE                = 0x20,
    HTML_TAG_CLIPPATH            = 0x21,
    HTML_TAG_CODE                = 0x22,
    HTML_TAG_COL                 = 0x23,
    HTML_TAG_COLGROUP            = 0x24,
    HTML_TAG_DATA                = 0x25,
    HTML_TAG_DATALIST            = 0x26,
    HTML_TAG_DD                  = 0x27,
    HTML_TAG_DEL                 = 0x28,
    HTML_TAG_DESC                = 0x29,
    HTML_TAG_DETAILS             = 0x2A,
    HTML_TAG_DFN                 = 0x2B,
    HTML_TAG_DIALOG              = 0x2C,
    HTML_TAG_DIR                 = 0x2D,
    HTML_TAG_DIV                 = 0x2E,
    HTML_TAG_DL                  = 0x2F,
    HTML_TAG_DT                  = 0x30,
    HTML_TAG_EM                  = 0x31,
    HTML_TAG_EMBED               = 0x32,
    HTML_TAG_FEBLEND             = 0x33,
    HTML_TAG_FECOLORMATRIX       = 0x34,
    HTML_TAG_FECOMPONENTTRANSFER = 0x35,
    HTML_TAG_FECOMPOSITE         = 0x36,
    HTML_TAG_FECONVOLVEMATRIX    = 0x37,
    HTML_TAG_FEDIFFUSELIGHTING   = 0x38,
    HTML_TAG_FEDISPLACEMENTMAP   = 0x39,
    HTML_TAG_FEDISTANTLIGHT      = 0x3A,
    HTML_TAG_FEDROPSHADOW        = 0x3B,
    HTML_TAG_FEFLOOD             = 0x3C,
    HTML_TAG_FEFUNCA             = 0x3D,
    HTML_TAG_FEFUNCB             = 0x3E,
    HTML_TAG_FEFUNCG             = 0x3F,
    HTML_TAG_FEFUNCR             = 0x40,
    HTML_TAG_FEGAUSSIANBLUR      = 0x41,
    HTML_TAG_FEIMAGE             = 0x42,
    HTML_TAG_FEMERGE             = 0x43,
    HTML_TAG_FEMERGENODE         = 0x44,
    HTML_TAG_FEMORPHOLOGY        = 0x45,
    HTML_TAG_FEOFFSET            = 0x46,
    HTML_TAG_FEPOINTLIGHT        = 0x47,
    HTML_TAG_FESPECULARLIGHTING  = 0x48,
    HTML_TAG_FESPOTLIGHT         = 0x49,
    HTML_TAG_FETILE              = 0x4A,
    HTML_TAG_FETURBULENCE        = 0x4B,
    HTML_TAG_FIELDSET            = 0x4C,
    HTML_TAG_FIGCAPTION          = 0x4D,
    HTML_TAG_FIGURE              = 0x4E,
    HTML_TAG_FONT                = 0x4F,
    HTML_TAG_FOOTER              = 0x50,
    HTML_TAG_FOREIGNOBJECT       = 0x51,
    HTML_TAG_FORM                = 0x52,
    HTML_TAG_FRAME               = 0x53,
    HTML_TAG_FRAMESET            = 0x54,
    HTML_TAG_GLYPHREF            = 0x55,
    HTML_TAG_H1                  = 0x56,
    HTML_TAG_H2                  = 0x57,
    HTML_TAG_H3                  = 0x58,
    HTML_TAG_H4                  = 0x59,
    HTML_TAG_H5                  = 0x5A,
    HTML_TAG_H6                  = 0x5B,
    HTML_TAG_HEAD                = 0x5C,
    HTML_TAG_HEADER              = 0x5D,
    HTML_TAG_HGROUP              = 0x5E,
    HTML_TAG_HR                  = 0x5F,
    HTML_TAG_HTML                = 0x60,
    HTML_TAG_I                   = 0x61,
    HTML_TAG_IFRAME              = 0x62,
    HTML_TAG_IMAGE               = 0x63,
    HTML_TAG_IMG                 = 0x64,
    HTML_TAG_INPUT               = 0x65,
    HTML_TAG_INS                 = 0x66,
    HTML_TAG_ISINDEX             = 0x67,
    HTML_TAG_KBD                 = 0x68,
    HTML_TAG_KEYGEN              = 0x69,
    HTML_TAG_LABEL               = 0x6A,
    HTML_TAG_LEGEND              = 0x6B,
    HTML_TAG_LI                  = 0x6C,
    HTML_TAG_LINEARGRADIENT      = 0x6D,
    HTML_TAG_LINK                = 0x6E,
    HTML_TAG_LISTING             = 0x6F,
    HTML_TAG_MAIN                = 0x70,
    HTML_TAG_MALIGNMARK          = 0x71,
    HTML_TAG_MAP                 = 0x72,
    HTML_TAG_MARK                = 0x73,
    HTML_TAG_MARQUEE             = 0x74,
    HTML_TAG_MATH                = 0x75,
    HTML_TAG_MENU                = 0x76,
    HTML_TAG_META                = 0x77,
    HTML_TAG_METER               = 0x78,
    HTML_TAG_MFENCED             = 0x79,
    HTML_TAG_MGLYPH              = 0x7A,
    HTML_TAG_MI                  = 0x7B,
    HTML_TAG_MN                  = 0x7C,
    HTML_TAG_MO                  = 0x7D,
    HTML_TAG_MS                  = 0x7E,
    HTML_TAG_MTEXT               = 0x7F,
    HTML_TAG_MULTICOL            = 0x80,
    HTML_TAG_NAV                 = 0x81,
    HTML_TAG_NEXTID              = 0x82,
    HTML_TAG_NOBR                = 0x83,
    HTML_TAG_NOEMBED             = 0x84,
    HTML_TAG_NOFRAMES            = 0x85,
    HTML_TAG_NOSCRIPT            = 0x86,
    HTML_TAG_OBJECT              = 0x87,
    HTML_TAG_OL                  = 0x88,
    HTML_TAG_OPTGROUP            = 0x89,
    HTML_TAG_OPTION              = 0x8A,
    HTML_TAG_OUTPUT              = 0x8B,
    HTML_TAG_P                   = 0x8C,
    HTML_TAG_PARAM               = 0x8D,
    HTML_TAG_PATH                = 0x8E,
    HTML_TAG_PICTURE             = 0x8F,
    HTML_TAG_PLAINTEXT           = 0x90,
    HTML_TAG_PRE                 = 0x91,
    HTML_TAG_PROGRESS            = 0x92,
    HTML_TAG_Q                   = 0x93,
    HTML_TAG_RADIALGRADIENT      = 0x94,
    HTML_TAG_RB                  = 0x95,
    HTML_TAG_RP                  = 0x96,
    HTML_TAG_RT                  = 0x97,
    HTML_TAG_RTC                 = 0x98,
    HTML_TAG_RUBY                = 0x99,
    HTML_TAG_S                   = 0x9A,
    HTML_TAG_SAMP                = 0x9B,
    HTML_TAG_SCRIPT              = 0x9C,
    HTML_TAG_SEARCH              = 0x9D,
    HTML_TAG_SECTION             = 0x9E,
    HTML_TAG_SELECT              = 0x9F,
    HTML_TAG_SELECTEDCONTENT     = 0xA0,
    HTML_TAG_SLOT                = 0xA1,
    HTML_TAG_SMALL               = 0xA2,
    HTML_TAG_SOURCE              = 0xA3,
    HTML_TAG_SPACER              = 0xA4,
    HTML_TAG_SPAN                = 0xA5,
    HTML_TAG_STRIKE              = 0xA6,
    HTML_TAG_STRONG              = 0xA7,
    HTML_TAG_STYLE               = 0xA8,
    HTML_TAG_SUB                 = 0xA9,
    HTML_TAG_SUMMARY             = 0xAA,
    HTML_TAG_SUP                 = 0xAB,
    HTML_TAG_SVG                 = 0xAC,
    HTML_TAG_TABLE               = 0xAD,
    HTML_TAG_TBODY               = 0xAE,
    HTML_TAG_TD                  = 0xAF,
    HTML_TAG_TEMPLATE            = 0xB0,
    HTML_TAG_TEXTAREA            = 0xB1,
    HTML_TAG_TEXTPATH            = 0xB2,
    HTML_TAG_TFOOT               = 0xB3,
    HTML_TAG_TH                  = 0xB4,
    HTML_TAG_THEAD               = 0xB5,
    HTML_TAG_TIME                = 0xB6,
    HTML_TAG_TITLE               = 0xB7,
    HTML_TAG_TR                  = 0xB8,
    HTML_TAG_TRACK               = 0xB9,
    HTML_TAG_TT                  = 0xBA,
    HTML_TAG_U                   = 0xBB,
    HTML_TAG_UL                  = 0xBC,
    HTML_TAG_VAR                 = 0xBD,
    HTML_TAG_VIDEO               = 0xBE,
    HTML_TAG_WBR                 = 0xBF,
    HTML_TAG_XMP                 = 0xC0,
    HTML_TAG__COUNT_             = 0xC1,
    HTML_TAG__INVALID_           = HTML_TAG__COUNT_
} HtmlTag;

HtmlTag dom_node_tag(DomNode n);
DomNodeType dom_node_type(DomNode n);



static inline DomNode dom_node_first_child(DomNode n) { return (DomNode){.ptr=n.ptr->first_child}; }
static inline DomNode dom_node_last_child(DomNode n)  { return (DomNode){.ptr=n.ptr->last_child}; }
static inline DomNode dom_node_next(DomNode n){ return (DomNode){.ptr=n.ptr->next}; } 
DomNode dom_node_next_elem(DomNode n);

static inline bool dom_node_eq(DomNode n, DomNode m) { return n.ptr == m.ptr; }

bool dom_node_has_tag(DomNode n, HtmlTag tag);
bool dom_node_has_type(DomNode n, DomNodeType type);
//TODO1: deprecate:
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

bool html_input_type_is_text_like(StrView type);
Err dom_get_tag_nodes_in_doc(Dom dom, StrView tag, ArlOf(DomNode) nodes[_1_]);
#endif
