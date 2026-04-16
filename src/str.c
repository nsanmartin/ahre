#include "mem.h"

#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "generic.h"
#include "str.h"
#include "sys.h"

/* mem fns */
char* mem_is_whitespace(char* s, size_t len) {
    while(len && *s && !isspace(*s)) { ++s; --len; }
    return len ? s : NULL;
}


size_t mem_count_ocurrencies(char* data, size_t len, char c) {
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


const char* mem_to_dup_str(const char* data, size_t len) {
    char* res = std_malloc(len + 1);
    if (!res) return NULL;
    memcpy(res, data, len);
    res[len] = '\0';
    return res;
}


bool mem_skip_space_inplace(const char* data[_1_], size_t len[_1_]) {
    for(; *len && isspace(**data); --(*len), ++(*data))
        ;
    return *len != 0;
}


Err mem_fwrite(const char* mem, size_t len, FILE* stream) {
    if (len != fwrite(mem, 1, len, stream)) return "error: mem_fwrite failure";
    return Ok;
}


int memstr_cmp(const char* mem, size_t len, const char* cstr) {
    size_t cmplen = strlen(cstr);
    if (cmplen > len) cmplen = len;
    return strncmp(mem, cstr, cmplen);
}


bool cstr_starts_with(const char* s, const char* t) {
    return s && t && strncmp(s, t, strlen(t)) == 0;
}


bool mem_eq_case(const char* m1, size_t l1, const char* m2, size_t l2) { 
    return m1 && m2 && l1 == l2 && !strncasecmp(m1, m2, l1);
}


/* static Err mem_get_indexes_of(char* data, size_t len, size_t offset, char c, ArlOf(size_t)* indexes) { */
/*     char* end = data + len; */
/*     char* it = data; */

/*     for(; it < end;) { */
/*         it = memchr(it, c, end-it); */
/*         if (!it || it >= end) { break; } */
/*         size_t off = offset + (it - data); */
/*         if(NULL == arlfn(size_t, append)(indexes, &off)) { return "arlfn append failed"; } */
/*         ++it; */
/*     } */

/*     return Ok; */
/* } */


/* cstr fns */

const char*
cstr_skip_space(const char* s) {
    for (; *s && isspace(*s); ++s);
    return s;
}


const char*
cstr_next_space(const char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}


void
cstr_replace_char_inplace(char* s, char from, char to) {
    char* p;
    while ((p=strchr(s,from))) *p = to;
}


const char* csubword_match(const char* s, const char* cmd, size_t len) {
    if (!*s || !isalpha(*s)) { return 0x0; }
	for (; *cmd && *s && isalpha(*s); ++s, ++cmd, (len?--len:len)) {
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
 
bool mem_is_all_space(const char* data, size_t len) {
    for(; len && isspace(*data); --len, ++data)
        ;
    return !len;
}

const char* cstr_cat_dup(const char* s, const char* t) {
    if (!s || !t) return NULL;
    size_t slen = strlen(s);
    size_t tlen = strlen(t);
    size_t len = slen + tlen;
    char* buf = malloc(len + 1);
    if (!buf) return NULL;
    buf[len] = '\0';
    memcpy(buf, s, slen);
    memcpy(buf + slen, t, tlen);
    return buf;
}

const char* cstr_mem_cat_dup(const char* s, const char* t, size_t tlen) {
    if (!s || !t) return NULL;
    size_t slen = strlen(s);
    //size_t tlen = strlen(t);
    size_t len = slen + tlen;
    char* buf = malloc(len + 1);
    if (!buf) return NULL;
    buf[len] = '\0';
    memcpy(buf, s, slen);
    memcpy(buf + slen, t, tlen);
    return buf;
}


Err mem_convert_to_utf8(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[_1_],
    size_t outlen[_1_]
) {
    if (!charset) return "error: cannot convert to utf8 from charsett NULL";
    if (!inlen || !inbuf || !*inbuf) {
        *outlen = 0;
        *outbuf = NULL;
        return Ok;
    }
#define ICONV_TO_STR_ "UTF-8"
    iconv_t cd = iconv_open(ICONV_TO_STR_, charset);
    if ((size_t)cd == (size_t)-1) {
        if (errno == EINVAL) return err_fmt("warning: convertion to '%s' not available", charset);
        return err_fmt("error: iconv_open failure from "ICONV_TO_STR_" to %s", charset);
    }

    size_t allocated =  inlen + 1 + inlen / 8;
    size_t outleft = allocated;
    *outlen = allocated; //TODO
    *outbuf = std_malloc(allocated);
    if (!*outbuf) return "error: malloc failure";

    char* outptr = (char*)*outbuf;
    const char* inbeg = inbuf;
    char const * inend = inbuf + inlen;
    size_t inleft = inend - inbeg;

    for (; inbeg < inend;) {
        size_t nconv = iconv(cd, (char**)&inbeg, &inleft, (char**)&outptr, &outleft);
        size_t written_so_far = allocated - outleft;
        if (nconv == (size_t)-1) {
            switch(errno) {
            case E2BIG: 
                outleft += allocated;
                allocated *= 2;
                *outbuf = realloc((char*)*outbuf, allocated);
                if (!*outbuf) {
                    if (iconv_close(cd)) return "error: iconv_close failure after realloc failure";
                    return "error: realloc failure";
                }
                inbeg = inbuf + inlen - inleft;
                outptr = (char*)*outbuf + written_so_far;
                continue;
            case EINVAL: 
                if (iconv_close(cd)) return "error: iconv_close failure";
                std_free((void*)*outbuf);
                return "error? unexpected incomplete multibyte seq?";
            default: 
                if (iconv_close(cd)) return "error: iconv_close failure";
                std_free((void*)*outbuf);
                return err_fmt("error: iconv unexpected failure: %s", strerror(errno));
            }
        }
    }
    if (iconv_close(cd)) return "error: iconv_close failure";
    *outlen = allocated - outleft;
    return Ok;
}


Err str_append_timespec(Str out[_1_], struct timespec ts[_1_]) {
#define DT_BUFLEN 128u
    char buff[DT_BUFLEN];
    if (!strftime(buff, sizeof buff, "%F %T", gmtime(&ts->tv_sec)))
        return "error: strftime output did not fit the DT_BUFLEN";
    try( str_append(out, buff));
    const int len = snprintf(buff, DT_BUFLEN, ".%09ld UTC", ts->tv_nsec);
    if ((int)DT_BUFLEN <= len)
        return "error: snprintf output did not fit the DT_BUFLEN";
    return str_append(out, buff);
}

Err str_append_datetime_now(Str out[_1_]) {

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return str_append_timespec(out, &ts);
}


Err str_append_ui_as_base10(Str buf[_1_], uintmax_t ui) {
    char numbuf[3 * sizeof ui] = {0};
    int len = snprintf(numbuf, (3 * sizeof ui), "%lu", ui);
    if (len < 0 || (len > 3 * (int)(sizeof(ui))))
        return "error: could not convert ui to str";
    return str_append(buf, sv(numbuf, (size_t)len));
}


Err str_append_ui_as_base36(Str buf[_1_], uintmax_t ui) {
    char numbf[3 * sizeof ui] = {0};
    size_t len = 0;
    try( uint_to_base36_str(numbf, 3 * sizeof ui, ui, &len));
    return str_append(buf, sv(numbf, len));
}


/* strview fns */

StrView strview_from_mem(const char* s, size_t len) {return (StrView){.items=s, .len=len};}
StrView strview_from_strview_ptr(StrView s[_1_]) { return *s; }
StrView strview_id(StrView v) { return v; }
bool strview_is_empty(const StrView s[_1_]) { return !s->items || s->len == 0; }
const char* strview_beg(const StrView s[_1_]) { return s->items; }
const char* strview_end(const StrView s[_1_]) { return s->items + s->len; }
size_t strview_len(const StrView s[_1_]) { return s->len; }

void strview_trim_space_left(StrView s[_1_]) {
    while(s->len && isspace(*(items__(s)))) { ++s->items; --s->len; }
}


void strview_trim_space_in_place(StrView s[_1_]) {
    while(s->len && isspace(*(s->items))) { ++s->items; --s->len; }
    while(s->len > 1 && isspace(s->items[s->len-1])) { --s->len; }
}

StrView strview_split_word(StrView s[_1_]) {
    StrView word = (StrView){.items=items__(s)};
    while(s->len && !isspace(*(items__(s)))) { ++word.len; ++s->items; --s->len; }
    return word;
}

StrView cstr_split_word(const char* s[_1_]) {
    StrView word = (StrView){.items=*s};
    while(**s && !isspace(**s)) { ++(*s); ++word.len; }
    while(**s && isspace(**s)) { ++(*s); }
    return word;
}

StrView strview_from_mem_trim(const char* s, size_t len) {
    StrView rv = strview_from_mem(s, len);
    strview_trim_space_in_place(&rv);
    return rv;
}

bool strview_skip_space_inplace(StrView s[_1_]) {
    for(; s->len && isspace(*s->items); --(s->len), ++(s->items))
        ;
    return s->len != 0;
}


Err strview_join_lines_to_str(StrView view, Str out[_1_]) {
    while (view.len && view.items) {
        StrView line = strview_split_line(&view);
        if (line.len) {
            Err err = str_append(out, line);
            if (err) { str_clean(out); return err; }
        }
    }
    return Ok;
}

StrView
strview_from_str_ptr(Str s[_1_]) {return strview_from_mem(items__(s), len__(s));}

StrView
strview_from_str(Str s) {return strview_from_mem(s.items, s.len);}

StrView
strview_from_arl_of_char(ArlOf(char) a[_1_]) {
    return strview_from_mem(items__(a), len__(a));
}


StrView
strview_from_cstr(const char* s) {
    if (s) return strview_from_mem(s, strlen(s));
    return (StrView){0};
}


StrView
strview_split_line(StrView text[_1_]) {
    StrView line = *text;
    const char* nl = memchr(line.items, '\n', line.len);
    if (nl) {
        line.len = nl - line.items;
        text->items = nl + 1;
        text->len -= (line.len + 1);
    } else  {
        *text = (StrView){0};
    }
    return line;
}


/* str fns*/

#define append_mem_(B,S,T) (!buffn(char,append)(B, (char*)S, T))
#define append_(B,S) (append_mem_(B, items__(S), len__(S)))
#define append_if_(B,S) (len__(S)?append_mem_(B, items__(S), len__(S)):false)
bool str_append_strview_2_(Str s[_1_], StrView t, StrView u) {
    return append_(s, &t) || append_if_(s,&u);
}
#undef append_mem_
#undef append_
#undef append_cstr_

bool
str_contains(Str s[_1_], char c) { return len__(s) && memchr(items__(s), c, len__(s)); }

void
str_trim_space(StrView* l) {
    l->items = cstr_skip_space(items__(l));
    l->len = cstr_next_space(items__(l)) - items__(l);
}


Err
str_replace_char_inplace(Str s[_1_], char from, char to) {
    char* rest = s->items;
    while ((rest = memchr(rest, from, s->len - cast__(size_t)(rest - s->items))))
        *rest = to;
    return Ok;
}


bool
str_startswith_mem(Str s[_1_], const char* mem, size_t len) {
    return len__(s) >= len && !strncmp(items__(s), mem, len);
}


size_t
str_append_flip(const char* mem, size_t size, size_t nmemb, Str out[_1_]) {
    size_t len = size * nmemb;

    if (buffn(char,append)(out, (char*)mem, len))
        return len;
    return 0;
}

bool strview_strview_eq_case (StrView s, StrView t) { return mem_eq_case(s.items, s.len, t.items, t.len); }

/* testing */
#ifdef TESTING_FAILURES
bool unsigned APPENDS__ = 0;
Err str_append_mem_(Str* s, char* items, size_t len) {
    bool condition = (++APPENDS__) % 480 == 0;
    if (condition) {
        return err_fmt("TESTING: appends limit (APPENDS__: %d)", APPENDS__);
    }
    return (buffn(char,append)(s, items, len) ? false  : true);
}
#endif
