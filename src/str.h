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

#ifndef AHRE_STR_H__
#define AHRE_STR_H__

#include <ctype.h>
#include <string.h>

#include "src/error.h"
#include "src/mem.h"
#include "src/utils.h"

//typedef struct {
//	const char* s;
//	size_t len;
//} Str;

///* getters */
//size_t str_len(const Str s[static 1]);
//static inline const char* str_beg(const Str s[static 1]) { return s->s; }
//bool str_is_empty(const Str s[static 1]);
//static inline const char* str_end(const Str s[static 1]) { return s->s + s->len; }
//
///* ctor */
//int str_init(Str s[static 1], const char* cs);
//
///**/
//void str_trim_space(Str* l);
//char* str_ndup_cstr(const Str* url, size_t n);
//char* url_cpy(const char* url) ;
//

/* mem fns */
size_t mem_count_ocurrencies(char* data, size_t len, char c);
bool mem_is_all_space(const char* data, size_t len);
static inline const char* mem_to_dup_str(const char* data, size_t len) {
    char* res = ah_malloc(len + 1);
    if (!res) return NULL;
    memcpy(res, data, len);
    res[len] = '\0';
    return res;
}

static inline bool
mem_skip_space_inplace(const char* data[static 1], size_t len[static 1]) {
    for(; *len && isspace(**data); --(*len), ++(*data))
        ;
    return *len != 0;
}

/* cstr fns */
const char* cstr_skip_space(const char* s);
const char* cstr_next_space(const char* l);
const char* csubstr_match(const char* s, const char* cmd, size_t len);
const char* csubword_match(const char* s, const char* cmd, size_t len);
const char* cstr_trim_space(char* s);

static inline bool cstr_starts_with(const char* s, const char* t) {
    return s && t && strncmp(s, t, strlen(t)) == 0;
}

const char* cstr_cat_dup(const char* s, const char* t);
const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen);


/* strview fns */
typedef struct {
	const char* s;
	size_t len;
} StrView;

static inline const char* strview_beg(const StrView s[static 1]) { return s->s; }
static inline const char* strview_end(const StrView s[static 1]) { return s->s + s->len; }
static inline size_t strview_len(const StrView s[static 1]) { return s->len; }
static inline bool strview_is_empty(const StrView s[static 1]) { return !s->s || s->len == 0; }
inline static StrView strview_from_mem(const char* s, size_t len) {
    return (StrView){.s=s, .len=len};
}

inline static StrView strview_from_strview(StrView s[static 1]) {
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

inline static StrView strview_from_mem_trim(const char* s, size_t len) {
    StrView rv = strview_from_mem(s, len);
    strview_trim_space_in_place(&rv);
    return rv;
}

#define Str2 BufOf(char)
#define str2_clean buffn(char,clean)
#define str2_append(Str2Ptr, Items, NItems) \
    (buffn(char,append)(Str2Ptr, Items, NItems) ? Ok : "error: str2_append failure")

#define strview_from_lit__(LitStr) strview_from_mem(LitStr, sizeof(LitStr)-1)
static inline StrView strview_from_str__(Str2 s) {return strview_from_mem(s.items, s.len); }
static inline StrView strview_from_strptr__(Str2 s[static 1]) {return strview_from_mem(s->items, s->len); }
#define strview_from_strptr__(S) strview_from_mem((S)->items, (S)->len)
#define strview__(...) GET_MACRO__(NULL,__VA_ARGS__,strview_from_mem,strview_from_lit__,skip__)(__VA_ARGS__)


const char* parse_l(const char* tk, long lptr[static 1]);

StrView str_split_line(StrView text[static 1]);

typedef Err (*SerializeCallback)(StrView s, void* ctx);

static inline Err
bufofchar_append_ui_as_str(BufOf(char) buf[static 1], uintmax_t ui) {
    char numbuf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbuf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        //TODO: provide more info about the error
        return "error: could not convert ui to str";
    }
    if (buffn(char, append)(buf, numbuf, len)) return Ok;
    return "error: could not append str_ui to bufof char";
}

static inline Err
serialize_unsigned(SerializeCallback cb, uintmax_t ui, void* ctx) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    if ((len = snprintf(numbf, (3 * sizeof ui), "%lu", ui)) > (3 * sizeof ui)) {
        return "error: snprintf failure";
    }
    return cb(strview_from_mem(numbf, len), ctx);
}

#endif
