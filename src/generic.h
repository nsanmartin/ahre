#ifndef __GENERIC_AHRE_H__
#define __GENERIC_AHRE_H__

#include "src/htmldoc.h"
#include "src/session.h"
#include "src/str.h"
#include "src/textbuf.h"
#include "src/url-client.h"
#include "src/utils.h"


#define len__(Ptr) (Ptr)->len


#define len(Ptr) _Generic((Ptr), \
        Str*: str_len, \
        const Str*: str_len, \
        TextBuf*: textbuf_len, \
        const TextBuf*: textbuf_len \
    )(Ptr)


#define destroy(Ptr) _Generic((Ptr), \
    TextBuf*  : textbuf_destroy, \
    UrlClient*: url_client_destroy, \
    HtmlDoc*  : htmldoc_destroy, \
    Session*  : session_destroy, \
    char*     : std_free \
)(Ptr)

#endif
