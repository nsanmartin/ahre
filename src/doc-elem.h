#ifndef __AH_DOC_ELEM_H__
#define __AH_DOC_ELEM_H__

typedef enum {
    DOC_ELEM_TEXT,
    DOC_ELEM_HTML_P,
    DOC_ELEM_HTML_BR,
    DOC_ELEM_HTML_A,
    DOC_ELEM_HTML_H1,
    DOC_ELEM_HTML_H2,
    DOC_ELEM_HTML_H3,
    DOC_ELEM_HTML_H4,
    DOC_ELEM_HTML_H5,
    DOC_ELEM_HTML_H6
} DocElemTag;

typedef struct {
    DocElemTag tag;
    size_t offset;
    size_t len;
} DocElem;
#endif
