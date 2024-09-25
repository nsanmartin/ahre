#ifndef __GENERIC_AHRE_H__
#define __GENERIC_AHRE_H__

#include "src/textbuf.h"
#include "src/utils.h"
#include "src/str.h"


#define LENGTH(Ptr) (Ptr)->len

#define len(Ptr) _Generic((Ptr), \
        Str*: str_len, \
        const Str*: str_len, \
        TextBuf*: textbuf_len, \
        const TextBuf*: textbuf_len \
    )(Ptr)

#endif
