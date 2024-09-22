/**
 * Str struct is a const char* ptr and a size_t len.
 *
 * + Functions applied to a Str* are `str_.+`,
 * + functions applied to a [const] *   char* are `cstr_.+` (unless the
 *   `str[^_]+`such as strcmp, etc),
 * + functions applied to a [const] char* and a size_t len are `men_.+`
 *   (following examples such as memset, memchr, etc)
 *
 */

#ifndef __AHRE_STR_H__
#define __AHRE_STR_H__

#include <string.h>


typedef struct {
	const char* s;
	size_t len;
} Str;

#include <ah/utils.h>


static inline int str_init(Str s[static 1], const char* cs) {
    *s = (Str){.s=cs, .len=cs?strlen(cs):0};
    return cs && !s->len ? -1 : 0;
}

bool str_is_empty(const Str s[static 1]);

size_t mem_count_ocurrencies(char* data, size_t len, char c);

static inline const char* cstr_skip_space(const char* s) {
    for (; *s && isspace(*s); ++s);
    return s;
}

static inline const char* cstr_next_space(const char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}

static inline void str_trim_space(Str* l) {
    l->s = cstr_skip_space(l->s);
    l->len = cstr_next_space(l->s) - l->s;
}

#endif
