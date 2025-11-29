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
#include <errno.h>

#include "error.h"
#include "generic.h"
#include "mem.h"
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
mem_skip_space_inplace(const char* data[static 1], size_t len[static 1]) {
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

static inline Err mem_to_cstr(char* mem[static 1], size_t len) {
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

static inline const char* strview_beg(const StrView s[static 1]) { return s->items; }
static inline const char* strview_end(const StrView s[static 1]) { return s->items + s->len; }
static inline size_t strview_len(const StrView s[static 1]) { return s->len; }
static inline bool strview_is_empty(const StrView s[static 1]) { return !s->items || s->len == 0; }
inline static StrView strview_from_mem(const char* s, size_t len) {
    return (StrView){.items=s, .len=len};
}

inline static StrView strview_from_strview(StrView s[static 1]) {
    return (StrView){.items=items__(s), .len=len__(s)};
}

inline static void strview_trim_space_left(StrView s[static 1]) {
    while(s->len && isspace(*(items__(s)))) { ++s->items; --s->len; }
}

inline static void strview_trim_space_in_place(StrView s[static 1]) {
    while(s->len && isspace(*(s->items))) { ++s->items; --s->len; }
    while(s->len > 1 && isspace(s->items[s->len-1])) { --s->len; }
}

inline static StrView strview_split_word(StrView s[static 1]) {
    StrView word = (StrView){.items=items__(s)};
    while(s->len && !isspace(*(items__(s)))) { ++word.len; ++s->items; --s->len; }
    return word;
}

inline static StrView cstr_split_word(const char* s[static 1]) {
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
#define str_append(StrPtr, Items, NItems) \
    (buffn(char,append)(StrPtr, Items, NItems) ? Ok : "error: str_append failure")
     //: err_fmt("error: str_append failure ("__FILE__":%d", __LINE__))

#define str_append_z(StrPtr, Items, NItems) \
    ((!buffn(char,append)(StrPtr, Items, NItems) || !buffn(char,append)(StrPtr, "\0", 1))\
     ? "error: str_append_z failure" : Ok)

#define str_append_ln(StrPtr, Items, NItems) \
    ((!buffn(char,append)(StrPtr, Items, NItems) || !buffn(char,append)(StrPtr, "\n", 1))\
     ? "error: str_append_ln failure" : Ok)

static inline size_t str_append_flip(
    const char* mem,
    size_t      size,
    size_t      nmemb,
    Str         out[static 1]
) 
{
    size_t len = size * nmemb;

    if (buffn(char,append)(out, (char*)mem, len))
        return len;
    return 0;
}

static inline char* str_beg(Str s[static 1]) { return items__(s); }
static inline char* str_end(Str s[static 1]) {
    return len__(s) ? items__(s) + len__(s) : items__(s);
}
static inline Err null_terminated_str_from_mem(
    const char* mem,
    size_t      len,
    Str         out[static 1]
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

#define strview_from_lit__(LitStr) strview_from_mem(LitStr, sizeof(LitStr)-1)
static inline StrView strview_from_str__(Str s) {return strview_from_mem(s.items, s.len); }
static inline StrView strview_from_strptr__(Str s[static 1]) {return strview_from_mem(s->items, s->len); }
#define strview_from_strptr__(S) strview_from_mem((S)->items, (S)->len)
#define strview__(...) GET_MACRO__(NULL,__VA_ARGS__,strview_from_mem,strview_from_lit__,skip__)(__VA_ARGS__)

const char* parse_l(const char* tk, long lptr[static 1]);

StrView str_split_line(StrView text[static 1]);

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

static inline void replace_char_inplace(char* s, char from, char to) {
    char* p;
    while ((p=strchr(s,from))) *p = to;
}

Err _convert_to_utf8_(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[static 1],
    size_t outlen[static 1]
);
#endif
