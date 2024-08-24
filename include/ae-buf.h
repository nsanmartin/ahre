#ifndef __AE_BUF_H__
#define __AE_BUF_H__

typedef struct {
    BufOf(char) buf;
    size_t current_line;
} AeBuf;
#endif
