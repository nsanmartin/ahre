#ifndef __MEM_AHRE_H__
#define __MEM_AHRE_H__

#include <stdlib.h>

#define std_malloc malloc
#define std_free free
#define ah_strndup strndup

#include "src/url-client.h"
#include "src/doc.h"
#include "src/session.h"
#include "src/textbuf.h"

#define destroy(Ptr) _Generic((Ptr), \
    TextBuf*:   textbuf_destroy, \
    UrlClient*: url_client_destroy, \
    Doc*:       doc_destroy, \
    Session*:   session_destroy, \
    char*:     std_free \
)(Ptr)

#endif
