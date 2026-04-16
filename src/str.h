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

#include "error.h"
#include "generic.h"
#include "utils.h"


#define Str BufOf(char)

typedef struct {
	const char* items;
	size_t      len;
} StrView;

typedef StrView (*StrViewProvider)(void);


/* mem fns */

Err         mem_fwrite(const char* mem, size_t len, FILE* stream);
bool        mem_eq_case(const char* m1, size_t l1, const char* m2, size_t l2);
bool        mem_is_all_space(const char* data, size_t len);
bool        mem_skip_space_inplace(const char* data[_1_], size_t len[_1_]);
char*       mem_is_whitespace(char* s, size_t len);
const char* mem_to_dup_str(const char* data, size_t len);
size_t      mem_count_ocurrencies(char* data, size_t len, char c);
int         memstr_cmp(const char* mem, size_t len, const char* cstr);
Err         mem_convert_to_utf8(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[_1_],
    size_t outlen[_1_]
);

#define lit_write__(Lit, Stream) mem_fwrite(Lit,lit_len__(Lit), Stream)


/* cstr fns */

StrView     cstr_split_word(const char* s[_1_]);
bool        cstr_mem_eq_case(const char* cstr, const char* mem, size_t len);
bool        cstr_starts_with(const char* s, const char* t);
const char* cstr_cat_dup(const char* s, const char* t);
const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen);
const char* cstr_next_space(const char* l);
const char* cstr_skip_space(const char* s);
const char* cstr_trim_space(char* s);
const char* csubword_match(const char* s, const char* cmd, size_t len);
void        cstr_replace_char_inplace(char* s, char from, char to);


/* strview fns */

Err         strview_join_lines_to_str(StrView view, Str out[_1_]);
StrView     strview_from_arl_of_char(ArlOf(char) a[_1_]);
StrView     strview_from_cstr(const char* s);
StrView     strview_from_mem(const char* s, size_t len);
StrView     strview_from_mem_trim(const char* s, size_t len);
StrView     strview_from_str(Str s);
StrView     strview_from_str_ptr(Str s[_1_]);
StrView     strview_from_strview_ptr(StrView s[_1_]);
StrView     strview_id(StrView v);
StrView     strview_split_line(StrView text[_1_]);
StrView     strview_split_word(StrView s[_1_]);
bool        strview_is_empty(const StrView s[_1_]);
bool        strview_skip_space_inplace(StrView s[_1_]);
const char* strview_beg(const StrView s[_1_]);
const char* strview_end(const StrView s[_1_]);
size_t      strview_len(const StrView s[_1_]);
void        strview_trim_space_in_place(StrView s[_1_]);
void        strview_trim_space_left(StrView s[_1_]);

#define svl(LitStr) _Generic((LitStr), char*: strview_from_mem(LitStr, lit_len__(LitStr)))

#define strview_from__(P) _Generic((P),\
    Str         : strview_from_str,\
    Str*        : strview_from_str_ptr,\
    StrView     : strview_id,\
    StrView*    : strview_from_strview_ptr,\
    ArlOf(char)*: strview_from_arl_of_char,\
    const char* : strview_from_cstr,\
    char*       : strview_from_cstr\
)(P)

#define sv(...) \
    GET_MACRO__(NULL,__VA_ARGS__,strview_from_mem,strview_from__,skip__)(__VA_ARGS__)


/* str fns */

Err    null_terminated_str_from_mem(const char* mem, size_t len, Str out[_1_]);
Err    str_append_datetime_now(Str [_1_]);
Err    str_append_timespec(Str out[_1_], struct timespec ts[_1_]);
Err    str_append_ui_as_base10(Str buf[_1_], uintmax_t ui);
Err    str_append_ui_as_base36(Str buf[_1_], uintmax_t ui);
Err    str_replace_char_inplace(Str s[_1_], char from, char to);
bool   str_contains(Str s[_1_], char c);
bool   str_startswith_mem(Str s[_1_], const char* mem, size_t len);
size_t str_append_flip(const char* mem, size_t size, size_t nmemb, Str out[_1_]) ;
void   str_trim_space(StrView* l);


#define str_clean buffn(char,clean)
#define str_reset buffn(char,reset)
#define str_at buffn(char,at)


bool str_append_strview_2_(Str s[_1_], StrView t, StrView u);

#define str_append(S,P) (\
    (str_append_strview_2_(S,sv(P),svl(""))\
    ? err_fmt("error: str_append failure ("__FILE__":%d)", __LINE__) : Ok))

#define str_append_z(S,P) (\
    (str_append_strview_2_(S,sv(P),svl("\0"))\
    ? err_fmt("error: str_append_z failure ("__FILE__":%d)", __LINE__) : Ok))

#define str_append_ln(S,P) (\
    (str_append_strview_2_(S,sv(P),svl("\n"))\
    ? err_fmt("error: str_append_z failure ("__FILE__":%d)", __LINE__) : Ok))


bool strview_strview_eq_case (StrView s, StrView t);
#define str_eq_case(S, T) strview_strview_eq_case(sv(S), sv(T))

#endif
