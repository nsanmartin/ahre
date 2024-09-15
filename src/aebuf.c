#include <aebuf.h>

inline size_t AeBufNLines(AeBuf ab[static 1]) {
    return ab->lines_offs.len;
}
