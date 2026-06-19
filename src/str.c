#include "mem.h"

#include <ctype.h>
#include <errno.h>
#include <iconv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "generic.h"
#include "str.h"
#include "sys.h"

/* mem fns */
char* mem_is_whitespace(char* s, size_t len) {
    while(len && *s && !isspace(*s)) { ++s; --len; } //TODO1: why !
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


Err
str_append_file(Str s[1], const char* filename)
{
    *s  = (Str){0};
    Err e = Ok;
    FilePtr fp;
    try(file_open(filename, "rb", &fp.ptr));

    if (fseek(fp.ptr, 0, SEEK_END) != 0) { e=err_from(errno); goto Close; }
    long long_size = ftell(fp.ptr);
    if (long_size < 0) { e=err_from(errno); goto Close; }
    size_t size;
    try(cast_from(&size, long_size));
    if (fseek(fp.ptr, 0, SEEK_SET) != 0) { e=err_from(errno); goto Close; }
    rewind (fp.ptr);

    s->capacity = size + 1;
    s->items = std_malloc(s->capacity);
    if (!s->items)  { e=err_from(errno); goto Close; }

    tryjmp(e,Close, file_read(&fp, s->items, size, &s->len));

    s->items[s->len] = '\0';

Close:
    file_close(fp.ptr);
    return e;
}
/* strview fns */

StrView strview_from_mem(const char* s, size_t len) {return (StrView){.items=s, .len=len};}
StrView strview_from_strview_ptr(const StrView s[_1_]) { return *s; }
StrView strview_id(const StrView v) { return v; }
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
    while(s->len && is_visible(*(items__(s)))) { ++word.len; ++s->items; --s->len; }
    return word;
}

StrView cstr_split_word(const char* s[_1_]) {
    StrView word = (StrView){.items=*s};
    while(**s && is_visible(**s)) { ++(*s); ++word.len; }
    while(**s && !is_visible(**s)) { ++(*s); }
    return word;
}


StrView
strview_split(StrView s[_1__], char c) {
    StrView rv = (StrView){0};
    if (len__(s) == 0) return rv;

    char* end = memchr(items__(s), c, len__(s));
    if (!end) {
        rv = *s;
        *s  = (StrView){0};
        return rv;
    }

    size_t len = end - items__(s);
    if (end == items__(s)) {
        ++s->items;
        --s->len;
        return rv;
    }

    rv       = (StrView){.items=items__(s), .len=len};
    s->len   -= rv.len + 1;
    s->items += rv.len + 1;
    return rv;
}


StrView
strview_rsplit(StrView s[_1__], char c) {
    StrView rv = (StrView){0};
    if (len__(s) == 0) return rv;
    size_t dot_ix = len__(s) - 1;
    while (dot_ix && items__(s)[dot_ix] != c)
        --dot_ix;
    
    if (!dot_ix && items__(s)[0] != c) {
        rv = *s;
        *s  = (StrView){0};
        return rv;
    }

    rv       = (StrView){.items=items__(s) + dot_ix + 1, .len=len__(s) - dot_ix - 1};
    s->len -= rv.len + 1;
    return rv;
}


StrView strview_from_mem_trim(const char* s, size_t len) {
    StrView rv = strview_from_mem(s, len);
    strview_trim_space_in_place(&rv);
    return rv;
}

bool strview_skip_space_inplace(StrView s[_1_]) {
    strview_trim_left_utf8_space(s);
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
strview_from_str_ptr(const Str s[_1_]) {return strview_from_mem(items__(s), len__(s));}

StrView
strview_from_str(const Str s) {return strview_from_mem(s.items, s.len);}

StrView
strview_from_arl_of_char(const ArlOf(char) a[_1_]) {
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
    if (!line.len) return line;
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

bool
str_append_strview__(Str b[_1_], const StrView v) {
    char*  data = (char*)v.items;
    size_t len  = v.len;
#define T char
    if (!data || !len) return true;
    T* rest = buffn(T, __ensure_extra_capacity)(b, len + 1);
    if (!rest) return true;
    memcpy((void*)rest, data, len * sizeof(T));
    b->len += len;
    b->items[b->len] = '\0';
    return false;
#undef T
}

bool str_append_strview_2_(Str s[_1_], const StrView t, const StrView u) {
    return str_append_strview__(s, t)
        || (u.len ? str_append_strview__(s,u) : false);
}

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
str_startswith_mem(StrView s, const char* mem, size_t len) {
    return s.len >= len && !strncmp(s.items, mem, len);
}


bool str_startswith(StrView s, StrView v) { return str_startswith_mem(s, v.items, v.len); }


size_t
str_append_flip(const char* mem, size_t size, size_t nmemb, Str out[_1_]) {
    size_t len = size * nmemb;

    if (str_append(out, sv(mem, len))) return 0;
    return len;
}

bool strview_strview_eq_case (StrView s, StrView t) { return mem_eq_case(s.items, s.len, t.items, t.len); }



size_t mem_count_utf8(const char* s, size_t len) {
    const char* end = s + len;
    size_t      res = 0;
    if (!s || !len) return res;
    while (s < end) {
        if ((*s & 0x80) == 0) ++s;         // ASCII    - 1 byte
        else if ((*s & 0xE0) == 0xC0) s += 2; // 110xxxxx - 2 bytes
        else if ((*s & 0xF0) == 0xE0) s += 3; // 1110xxxx - 3 bytes
        else if ((*s & 0xF8) == 0xF0) s += 4; // 11110xxx - 4 bytes
        else {
            ++s; // Invalid leading byte - skip it to avoid infinite loop
            continue;
        }
        res++;
    }
    return res;
}

/**
 * Trims leading Unicode whitespace from a Str.
 * Updates both the pointer (s) and length (len) in place.
 * 
 * Recognizes:
 * - All ASCII whitespace (isspace)
 * - U+00A0 (NBSP)
 * - U+2000-U+200B (en quad through zero width space)
 * - U+2028-U+2029 (line/paragraph separators)
 * - U+202F (narrow no-break space)
 * - U+205F (medium mathematical space)
 * - U+3000 (ideographic space)
 *///TODO1: the lines check should be improoved
size_t
strview_trim_left_utf8_space(StrView s[_1_]) {
    if (!s->items || s->len == 0) return 0;

    const char* p = s->items;
    const char* end = s->items + s->len;

    while (p < end) {
        unsigned char b1 = (unsigned char)*p;

        // ASCII
        if (b1 < 0x80) { 
            if (isspace(b1)) {
                p++;
                continue;
            }
            break;
        }

        // 2-byte sequence
        if ((b1 & 0xE0) == 0xC0 && p + 1 < end) {
            unsigned char b2 = (unsigned char)p[1];
            if ((b2 & 0xC0) != 0x80) break;

            if (b1 == 0xC2 && b2 == 0xA0) {
                p += 2;
                continue;
            }
            break;

        // 3-byte sequence
        } else if ((b1 & 0xF0) == 0xE0 && p + 2 < end) {
            unsigned char b2 = (unsigned char)p[1];
            unsigned char b3 = (unsigned char)p[2];
            if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) break;

            if (b1 == 0xE2 && b2 == 0x80 && b3 >= 0x80 && b3 <= 0x8B) {
                p += 3;
                continue;
            }
            if (b1 == 0xE2 && b2 == 0x80 && (b3 == 0xA8 || b3 == 0xA9)) {
                p += 3;
                continue;
            }
            if (b1 == 0xE2 && b2 == 0x80 && b3 == 0xAF) {
                p += 3;
                continue;
            }
            if (b1 == 0xE2 && b2 == 0x81 && b3 == 0x9F) {
                p += 3;
                continue;
            }
            if (b1 == 0xE3 && b2 == 0x80 && b3 == 0x80) {
                p += 3;
                continue;
            }
            break;

        } else if ((b1 & 0xF8) == 0xF0 && p + 3 < end) {
            break; // 4-byte sequence — no Unicode spaces in this range
        } else break; // Invalid/incomplete sequence
    }

    size_t skipped = p - s->items;
    s->items = p;
    s->len -= skipped;
    return skipped;
}


static inline bool is_utf8_continuation_byte(unsigned char c) { return (c & 0xC0) == 0x80; }

// Check if the UTF-8 sequence starting at 's' (with at most 'remaining' bytes)
// represents a Unicode whitespace character.
// Returns true and sets '*advance' to the number of bytes consumed if it is whitespace.
// Returns false otherwise (non-whitespace or invalid UTF-8).
static bool is_unicode_whitespace(const unsigned char* s, size_t remaining, size_t* advance) {
    if (remaining == 0) return false;

    // ASCII range (U+0000-U+007F)
    if (s[0] < 0x80) {
        if (isspace(s[0])) {
            *advance = 1;
            return true;
        }
        return false;
    }

    // 2-byte UTF-8 (U+0080-U+07FF)
    if ((s[0] & 0xE0) == 0xC0 && remaining >= 2 && is_utf8_continuation_byte(s[1])) {
        uint16_t cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        if (cp == 0x00A0) {        // no-break space
            *advance = 2;
            return true;
        }
        return false;
    }

    // 3-byte UTF-8 (U+0800-U+FFFF)
    if ((s[0] & 0xF0) == 0xE0 && remaining >= 3 &&
        is_utf8_continuation_byte(s[1]) && is_utf8_continuation_byte(s[2])) {
        uint32_t cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        // Common Unicode spaces: U+2000–U+200A, U+2028, U+2029, U+202F, U+205F, U+3000
        if ((cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 || cp == 0x2029 || cp == 0x202F
            || cp == 0x205F || cp == 0x3000) {
            *advance = 3;
            return true;
        }
        return false;
    }

    // 4-byte UTF-8 (U+10000-U+10FFFF) – no common whitespace here
    return false;
}

/**
 * @brief Returns the length of the string after trimming trailing whitespace.
 *
 * Scans backwards from the end of the string (first 'len' bytes) and skips any
 * ASCII or Unicode whitespace. The original string is not modified.
 *
 * returns removes chars (in bytes).
 */
size_t strview_trim_right_utf8_space(StrView s[_1_]) {
    if (!s->len) return 0;

    const unsigned char* ustr = (const unsigned char*)items__(s);
    size_t pos = len__(s);

    while (pos > 0) {
        // Move backwards to the start of the last UTF-8 character
        size_t char_start = pos - 1;
        while (char_start > 0 && is_utf8_continuation_byte(ustr[char_start])) {
            char_start--;
        }
        size_t char_len = pos - char_start;
        size_t advance = 0;
        if (is_unicode_whitespace(ustr + char_start, char_len, &advance) && advance == char_len) {
            pos = char_start;
        } else break;
    }
    size_t rv = len__(s) - pos;
    s->len = pos;
    return rv;
}

size_t strview_trim_utf8_space(StrView s[_1__]) {
    return strview_trim_left_utf8_space(s) + strview_trim_right_utf8_space(s);
}

/* testing */
#ifdef TESTING_FAILURES
bool unsigned APPENDS__ = 0;
Err str_append_mem_(Str* s, char* items, size_t len) {
    bool condition = (++APPENDS__) % 480 == 0;
    if (condition) {
        return err_fmt("TESTING: appends limit (APPENDS__: %d)", APPENDS__);
    }
    return str_append(s, sv(items,len));
}
#endif
