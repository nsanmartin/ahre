#include <ah/str.h>

inline size_t
str_count_ocurrencies(char* data, size_t len, char c) {
    char* end = data + len;
    size_t count = 0;

    for(;;) {
        data = memchr(data, c, end-data);
        if (!data || data >= end) { break; }
        ++count;
        ++data;
    }

    return count;
}


int str_get_indexes(char* data, size_t len, size_t offset, char c, ArlOf(size_t)* ptrs) {
    char* end = data + len;
    char* it = data;

    for(; it < end;) {
        it = memchr(it, c, end-it);
        if (!it || it >= end) { break; }
        size_t off = offset + (it - data);
        if(NULL == arlfn(size_t, append)(ptrs, &off)) { return -1; }
        ++it;
    }

    return 0;
}


//inline int
//str_get_eols(char* data, size_t base_off, size_t len, ArlOf(size_t)* ptrs) {
//    char* end = data + len;
//    char* it = data;
//
//    for(; it < end;) {
//        it = memchr(it, '\n', end-it);
//        if (!it || it >= end) { break; }
//        size_t off = base_off + (it - data);
//        if(NULL == arlfn(size_t, append)(ptrs, &off)) { return -1; }
//        ++it;
//    }
//
//    return 0;
//}
//
