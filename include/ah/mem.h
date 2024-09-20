#ifndef __MEM_AHRE_H__
#define __MEM_AHRE_H__

#include <stdlib.h>

#define std_malloc malloc
#define std_free free
#define ah_strndup strndup

#include <ah/url-client.h>
#include <ah/doc.h>
#include <ah/session.h>
#include <ah/textbuf.h>

#define destroy(Ptr) _Generic((Ptr), \
    TextBuf:   textbuf_destroy, \
    UrlClient: url_client_destroy, \
    Doc:       doc_destroy, \
    Session:   session_destroy, \
    default:   std_free \
)(Ptr)

#endif
