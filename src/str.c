#include <ah/str.h>

size_t
mem_count_ocurrencies(char* data, size_t len, char c) {
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


Err mem_get_indexes(char* data, size_t len, size_t offset, char c, ArlOf(size_t)* indexes) {
    char* end = data + len;
    char* it = data;

    for(; it < end;) {
        it = memchr(it, c, end-it);
        if (!it || it >= end) { break; }
        size_t off = offset + (it - data);
        if(NULL == arlfn(size_t, append)(indexes, &off)) { return "arlfn append failed"; }
        ++it;
    }

    return Ok;
}

inline size_t str_len(const Str s[static 1]) { return s->len; }
inline bool str_is_empty(const Str s[static 1]) { return !s->s || len(s) == 0; }

inline int str_init(Str s[static 1], const char* cs) {
    *s = (Str){.s=cs, .len=cs?strlen(cs):0};
    return cs && !len(s) ? -1 : 0;
}


inline const char* cstr_skip_space(const char* s) {
    for (; *s && isspace(*s); ++s);
    return s;
}

inline const char* cstr_next_space(const char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}

inline void str_trim_space(Str* l) {
    l->s = cstr_skip_space(l->s);
    l->len = cstr_next_space(l->s) - l->s;
}
