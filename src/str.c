#include <ah/str.h>


inline int
str_get_lines_offs(char* data, size_t base_off, size_t len, ArlOf(size_t)* ptrs) {
    char* end = data + len;
    char* it = data;

    for(; it < end;) {
        it = memchr(it, '\n', end-it);
        if (!it || it >= end) { break; }
        size_t off = base_off + (it - data);
        if(NULL == arlfn(size_t, append)(ptrs, &off)) { return -1; }
        ++it;
    }

    return 0;
}

