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
#include <strings.h>
#include <stdint.h>

#include "error.h"

#define T char
#include <buf.h>

typedef BufOf(char) Str;
typedef Str* StrPtr;

static inline int str_cmp(const void* xp, const void* yp) {
    const Str* x = xp;
    const Str* y = yp;
    size_t max = (x->len > y->len) ? x->len : y->len;
    return strncmp(x->items, y->items, max);
}
#define T Str
#define TClean buffn(char,clean)
#define TCmp str_cmp
#include <arl.h>


#define T char
#include <arl.h>


#define T ArlOf(Str)
#define TClean arlfn(Str,clean)
#include <arl.h>


typedef struct {
	const char* items;
	size_t      len;
} StrView;

#define T StrView
#include <arl.h>

typedef StrView (*StrViewProvider)(void);


#define _1__ 1

/* char fn */
static inline bool is_visible(char c) { return !isspace(c); }


/* mem fns */

Err         mem_fwrite(const char* mem, size_t len, FILE* stream);
bool        mem_eq_case(const char* m1, size_t l1, const char* m2, size_t l2);
bool        mem_is_all_space(const char* data, size_t len);
bool        mem_skip_space_inplace(const char* data[_1__], size_t len[_1__]);
char*       mem_is_whitespace(char* s, size_t len);
const char* mem_to_dup_str(const char* data, size_t len);
size_t      mem_count_ocurrencies(char* data, size_t len, char c);
int         memstr_cmp(const char* mem, size_t len, const char* cstr);
Err         mem_convert_to_utf8(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[_1__],
    size_t outlen[_1__]
);
size_t mem_count_utf8(const char* s, size_t len);

#define lit_write__(Lit, Stream) mem_fwrite(Lit,lit_len__(Lit), Stream)


/* cstr fns */

StrView     cstr_split_word(const char* s[_1__]);
bool        cstr_mem_eq_case(const char* cstr, const char* mem, size_t len);
bool        cstr_starts_with(const char* s, const char* t);
bool        str_startswith(StrView s, StrView v);
const char* cstr_cat_dup(const char* s, const char* t);
const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen);
//TODO1: replace all space functions by uspace (unicode) space functions
const char* cstr_next_space(const char* l);
const char* cstr_skip_space(const char* s);
const char* cstr_trim_space(char* s);
const char* csubword_match(const char* s, const char* cmd, size_t len);
void        cstr_replace_char_inplace(char* s, char from, char to);


/* strview fns */

Err         strview_join_lines_to_str(StrView view, Str out[_1__]);
StrView     strview_from_arl_of_char(const ArlOf(char) a[_1__]);
StrView     strview_from_cstr(const char* s);
StrView     strview_from_mem(const char* s, size_t len);
StrView     strview_from_mem_trim(const char* s, size_t len);
StrView     strview_from_str(const Str s);
StrView     strview_from_str_ptr(const Str s[_1__]);
StrView     strview_from_strview_ptr(const StrView s[_1__]);
StrView     strview_id(const StrView v);
StrView     strview_split_line(StrView text[_1__]);
StrView     strview_split_word(StrView s[_1__]);
StrView     strview_rsplit(StrView s[_1__], char c);
StrView     strview_split(StrView s[_1__], char c);
bool        strview_is_empty(const StrView s[_1__]);
bool        strview_skip_space_inplace(StrView s[_1__]);
const char* strview_beg(const StrView s[_1__]);
const char* strview_end(const StrView s[_1__]);
size_t      strview_len(const StrView s[_1__]);
void        strview_trim_space_in_place(StrView s[_1__]);
void        strview_trim_space_left(StrView s[_1__]);
size_t      strview_trim_left_utf8_space(StrView s[_1__]);
static inline
size_t      strview_count_utf8(StrView s) { return mem_count_utf8(s.items, s.len); }

#define svl(LitStr) _Generic((LitStr), char*: strview_from_mem(LitStr, (sizeof(LitStr)-1)))

#define strview_from__(P) _Generic((P),\
    Str                : strview_from_str,\
    Str*               : strview_from_str_ptr,\
    StrView            : strview_id,\
    StrView*           : strview_from_strview_ptr,\
    ArlOf(char)*       : strview_from_arl_of_char,\
    char*              : strview_from_cstr,\
    const Str          : strview_from_str,\
    const Str*         : strview_from_str_ptr,\
    const StrView      : strview_id,\
    const StrView*     : strview_from_strview_ptr,\
    const ArlOf(char)* : strview_from_arl_of_char,\
    const char*        : strview_from_cstr\
)(P)

#define sv(...) \
    GET_MACRO__(NULL,__VA_ARGS__,strview_from_mem,strview_from__,skip__)(__VA_ARGS__)


/* str fns */

Err    null_terminated_str_from_mem(const char* mem, size_t len, Str out[_1__]);
Err    str_append_datetime_now(Str [_1__]);
Err    str_append_timespec(Str out[_1__], struct timespec ts[_1__]);
Err    str_append_ui_as_base10(Str buf[_1__], uintmax_t ui);
Err    str_append_ui_as_base36(Str buf[_1__], uintmax_t ui);
Err    str_replace_char_inplace(Str s[_1__], char from, char to);
bool   str_contains(Str s[_1__], char c);
bool   str_startswith_mem(StrView s, const char* mem, size_t len);
size_t str_append_flip(const char* mem, size_t size, size_t nmemb, Str out[_1__]) ;
void   str_trim_space(StrView* l);


#define str_clean buffn(char,clean)
#define str_reset buffn(char,reset)
#define str_at buffn(char,at)


/* returns true on error */
bool str_append_strview__(Str b[_1__], const StrView v);
bool str_append_strview_2_(Str s[_1__], const StrView t, const StrView u);

#define str_append(S,P) (\
    (str_append_strview__(S,sv(P))\
    ? err_fmt("error: str_append failure ("__FILE__":%d)", __LINE__) : Ok))

#define str_append_z str_append

#define str_append_ln(S,P) (\
    (str_append_strview_2_(S,sv(P),svl("\n"))\
    ? err_fmt("error: str_append_z failure ("__FILE__":%d)", __LINE__) : Ok))

#define str_append_2(S,P,Q) (\
    (str_append_strview_2_(S,sv(P),sv(Q))\
    ? err_fmt("error: str_append_z failure ("__FILE__":%d)", __LINE__) : Ok))

bool strview_strview_eq_case (StrView s, StrView t);
#define str_eq_case(S, T) strview_strview_eq_case(sv(S), sv(T))

#endif
