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

#include <ctype.h>
#include <string.h>

#include "src/error.h"
#include "src/mem.h"

typedef struct {
	const char* s;
	size_t len;
} Str;

/* getters */
size_t str_len(const Str s[static 1]);
static inline const char* str_beg(const Str s[static 1]) { return s->s; }
bool str_is_empty(const Str s[static 1]);
static inline const char* str_end(const Str s[static 1]) { return s->s + s->len; }

/* ctor */
int str_init(Str s[static 1], const char* cs);

/**/
size_t mem_count_ocurrencies(char* data, size_t len, char c);
const char* cstr_skip_space(const char* s);
const char* cstr_next_space(const char* l);
void str_trim_space(Str* l);
char* str_ndup_cstr(const Str* url, size_t n);

const char* substr_match(const char* s, const char* cmd, size_t len);
const char* subword_match(const char* s, const char* cmd, size_t len);
char* url_cpy(const char* url) ;
const char* cstr_trim_space(char* s);
bool mem_is_all_space(const char* data, size_t len);

static inline bool cstr_starts_with(const char* s, const char* t) {
    return s && t && strncmp(s, t, strlen(t)) == 0;
}

typedef struct {
	const char* s;
	size_t len;
} StrView;

inline static StrView strview(const char* s, size_t len) {
    return (StrView){.s=s, .len=len};
}

inline static StrView strview_copy(StrView s[static 1]) {
    return (StrView){.s=s->s, .len=s->len};
}

inline static void strview_trim_space_left(StrView s[static 1]) {
    while(s->len && isspace(*(s->s))) { ++s->s; --s->len; }
}

inline static void strview_trim_space_in_place(StrView s[static 1]) {
    while(s->len && isspace(*(s->s))) { ++s->s; --s->len; }
    while(s->len > 1 && isspace(s->s[s->len-1])) { --s->len; }
}

inline static StrView strview_split_word(StrView s[static 1]) {
    StrView word = (StrView){.s=s->s};
    while(s->len && !isspace(*(s->s))) { ++word.len; ++s->s; --s->len; }
    return word;
}

inline static StrView cstr_split_word(const char* s[static 1]) {
    StrView word = (StrView){.s=*s};
    while(**s && !isspace(**s)) { ++(*s); ++word.len; }
    while(**s && isspace(**s)) { ++(*s); }
    return word;
}

inline static StrView strview_trim_from_mem(const char* s, size_t len) {
    StrView rv = strview(s, len);
    strview_trim_space_in_place(&rv);
    return rv;
}

Err str_prepend(Str s[static 1], const char* cs);

static inline const char* mem_to_dup_str(const char* data, size_t len) {
    char* res = ah_malloc(len + 1);
    if (!res) return NULL;
    memcpy(res, data, len);
    res[len] = '\0';
    return res;
}

const char* cstr_cat_dup(const char* s, const char* t);
const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen);
const char* parse_l(const char* tk, long lptr[static 1]);

/* returns whether there is more bytes beyond the space */
static inline bool
mem_skip_space_inplace(const char* data[static 1], size_t len[static 1]) {
    for(; *len && isspace(**data); --(*len), ++(*data))
        ;
    return *len != 0;
}

static inline Str mem_next_space_str(const char* data, size_t len) {
    while (len && !isspace(*data)) { --len; ++data; }
    return (Str){.s=data, .len=len};
}

static inline Str mem_get_word_str(const char* data, size_t len) {
    Str res = {.s=data};
    while (len && !isspace(*data)) { ++res.len; --len; ++data; }
    return res;
}

#endif
