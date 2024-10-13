#include "src/error.h"
#include "src/generic.h"
#include "src/mem.h"
#include "src/str.h"


char* url_cpy(const char* url) {
    char* res = NULL;
    if (url && *url) {
        const char* end = cstr_next_space(url);
        size_t len = end - url;
        res = malloc(1 + len);
        if (!res) { return NULL; }
        memcpy(res, url, 1 + len);
        memset(res + len, '\0', 1);
    }
    return res;
}

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

inline int str_init_from_mem(Str s[static 1], const char* beg, const char* end) {
    *s = (Str){.s=beg, .len=beg<=end?end-beg:0};
    return beg > end ? -1 : 0;
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


const char* substr_match(const char* s, const char* cmd, size_t len) {
    if (!*s) { return 0x0; }
	for (; *s && !isspace(*s); ++s, ++cmd, (len?--len:len)) {
		if (*s != *cmd) { return 0x0; }
	}
    if (len) { 
        printf("...%s?\n", cmd);
        return 0x0;
    }
	return cstr_skip_space(s);
}

const char* cstr_trim_space(char* s) {
    if (!s) return NULL;
    s = (char*)cstr_skip_space(s);
    if (!*s) return s;
    char* end = s + strlen(s);
    if (s == end) return s;
    while (end > s && isspace(end[-1])) --end;
    *end = '\0';
    return s;
}
