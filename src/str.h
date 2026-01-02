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

#include "mem.h"

#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "error.h"
#include "generic.h"
#include "utils.h"


/* mem fns */
static inline char* mem_is_whitespace(char* s, size_t len) {
    while(len && *s && !isspace(*s)) { ++s; --len; }
    return len ? s : NULL;
}
size_t mem_count_ocurrencies(char* data, size_t len, char c);
bool mem_is_all_space(const char* data, size_t len);
static inline const char* mem_to_dup_str(const char* data, size_t len) {
    char* res = std_malloc(len + 1);
    if (!res) return NULL;
    memcpy(res, data, len);
    res[len] = '\0';
    return res;
}

static inline bool
mem_skip_space_inplace(const char* data[_1_], size_t len[_1_]) {
    for(; *len && isspace(**data); --(*len), ++(*data))
        ;
    return *len != 0;
}

static inline Err mem_fwrite(const char* mem, size_t len, FILE* stream) {
    if (len != fwrite(mem, 1, len, stream)) return "error: mem_fwrite failure";
    return Ok;
}

static inline int memstr_cmp(const char* mem, size_t len, const char* cstr) {
    size_t cmplen = strlen(cstr);
    if (cmplen > len) cmplen = len;
    return strncmp(mem, cstr, cmplen);
}

static inline Err mem_to_cstr(char* mem[_1_], size_t len) {
    *mem = std_realloc(*mem, len + 1);
    if (!*mem) {
        return err_fmt("realloc failure: %s", strerror(errno));
    }
    *mem[len] = '\0';
    return Ok;
}

#define lit_write__(Lit, Stream) mem_fwrite(Lit,lit_len__(Lit), Stream)

/* cstr fns */
const char* cstr_skip_space(const char* s);
const char* cstr_next_space(const char* l);
const char* csubstr_match(const char* s, const char* cmd, size_t len);
const char* csubword_match(const char* s, const char* cmd, size_t len);
const char* cstr_trim_space(char* s);
bool substr_match_all(const char* s, size_t len, const char* cmd);

static inline bool cstr_starts_with(const char* s, const char* t) {
    return s && t && strncmp(s, t, strlen(t)) == 0;
}

const char* cstr_cat_dup(const char* s, const char* t);
const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen);

/* strview fns */
typedef struct {
	const char* items;
	size_t len;
} StrView;

typedef StrView (*StrViewProvider)(void);

static inline const char* strview_beg(const StrView s[_1_]) { return s->items; }
static inline const char* strview_end(const StrView s[_1_]) { return s->items + s->len; }
static inline size_t strview_len(const StrView s[_1_]) { return s->len; }
static inline bool strview_is_empty(const StrView s[_1_]) { return !s->items || s->len == 0; }
inline static StrView strview_from_mem(const char* s, size_t len) {
    return (StrView){.items=s, .len=len};
}

inline static StrView strview_from_strview(StrView s[_1_]) {
    return (StrView){.items=items__(s), .len=len__(s)};
}

inline static void strview_trim_space_left(StrView s[_1_]) {
    while(s->len && isspace(*(items__(s)))) { ++s->items; --s->len; }
}

inline static void strview_trim_space_in_place(StrView s[_1_]) {
    while(s->len && isspace(*(s->items))) { ++s->items; --s->len; }
    while(s->len > 1 && isspace(s->items[s->len-1])) { --s->len; }
}

inline static StrView strview_split_word(StrView s[_1_]) {
    StrView word = (StrView){.items=items__(s)};
    while(s->len && !isspace(*(items__(s)))) { ++word.len; ++s->items; --s->len; }
    return word;
}

inline static StrView cstr_split_word(const char* s[_1_]) {
    StrView word = (StrView){.items=*s};
    while(**s && !isspace(**s)) { ++(*s); ++word.len; }
    while(**s && isspace(**s)) { ++(*s); }
    return word;
}

inline static StrView strview_from_mem_trim(const char* s, size_t len) {
    StrView rv = strview_from_mem(s, len);
    strview_trim_space_in_place(&rv);
    return rv;
}

#define Str BufOf(char)
#define str_clean buffn(char,clean)
#define str_reset buffn(char,reset)
#define str_at buffn(char,at)
#define _str_ensure_extra_capacity(StrPtr, Len) \
    (buffn(char,__ensure_extra_capacity)(StrPtr, Len) ? Ok : "error: ensure extra capacity failure")
#define str_append_lit__(StrPtr, Lit) str_append(StrPtr, Lit, lit_len__(Lit))
/* #define str_append(StrPtr, Items, NItems) \ */
/*     (buffn(char,append)(StrPtr, Items, NItems) ? Ok : "error: str_append failure") */
#define str_append(StrPtr, Items, NItems) \
    (buffn(char,append)(StrPtr, Items, NItems) ? Ok \
     : err_fmt("error: str_append failure ("__FILE__":%d", __LINE__))

#define str_append_z(StrPtr, Items, NItems) \
    ((!buffn(char,append)(StrPtr, Items, NItems) || !buffn(char,append)(StrPtr, "\0", 1))\
     ? "error: str_append_z failure" : Ok)

#define str_append_ln(StrPtr, Items, NItems) \
    ((!buffn(char,append)(StrPtr, Items, NItems) || !buffn(char,append)(StrPtr, "\n", 1))\
     ? "error: str_append_ln failure" : Ok)

static inline Err str_append_str(Str s[_1_], Str t[_1_]) {
    return str_append(s, items__(t), len__(t));
}

static inline bool str_startswith_mem(Str s[_1_], const char* mem, size_t len) {
    return len__(s) >= len && !strncmp(items__(s), mem, len);
}

#define str_startswith_lit(S, Lit) str_startswith_mem(S, Lit, lit_len__(Lit))

static inline size_t str_append_flip(
    const char* mem,
    size_t      size,
    size_t      nmemb,
    Str         out[_1_]
) 
{
    size_t len = size * nmemb;

    if (buffn(char,append)(out, (char*)mem, len))
        return len;
    return 0;
}

static inline char* str_beg(Str s[_1_]) { return items__(s); }
static inline char* str_end(Str s[_1_]) {
    return len__(s) ? items__(s) + len__(s) : items__(s);
}
static inline Err null_terminated_str_from_mem(
    const char* mem,
    size_t      len,
    Str         out[_1_]
)
{
    str_reset(out); 
    Err e = _str_ensure_extra_capacity(out, len + 1);
    ok_then(e, str_append(out, (char*)mem, len));
    if (e) {
        str_clean(out);
        return e;
    }
    items__(out)[len] = '\0';
    return Ok;
}

#define strview_from_lit__(LitStr) strview_from_mem(LitStr, lit_len__(LitStr))
static inline StrView strview_from_str(Str s[_1_]) {return strview_from_mem(items__(s), len__(s));}
static inline StrView strview_from_cstr(const char* s) {
    if (s) return strview_from_mem(s, strlen(s));
    return (StrView){0};
}
#define _strview_from_onep(P) _Generic((P),\
    Str*    : strview_from_str,\
    StrView*: strview_from_strview,\
    const char*: strview_from_cstr,\
    char*: strview_from_cstr,\
    void*: strview_from_cstr\
)(P)

#define strview__(...) \
    GET_MACRO__(NULL,__VA_ARGS__,strview_from_mem,_strview_from_onep,skip__)(__VA_ARGS__)

const char* parse_l(const char* tk, long lptr[_1_]);

StrView str_split_line(StrView text[_1_]);

Err str_append_ui_as_base36(Str buf[_1_], uintmax_t ui);
Err str_append_ui_as_base10(Str buf[_1_], uintmax_t ui);

static inline void replace_char_inplace(char* s, char from, char to) {
    char* p;
    while ((p=strchr(s,from))) *p = to;
}

Err _convert_to_utf8_(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[_1_],
    size_t outlen[_1_]
);
Err str_append_timespec(Str out[_1_], struct timespec ts[_1_]);
Err str_append_datetime_now(Str [_1_]);
Err resolve_path(const char *path, bool file_exists[_1_], Str out[_1_]);
#endif
