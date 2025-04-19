#include <iconv.h>
#include <errno.h>

#include "error.h"
#include "generic.h"
#include "mem.h"
#include "str.h"


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


inline const char* cstr_skip_space(const char* s) {
    for (; *s && isspace(*s); ++s);
    return s;
}

inline const char* cstr_next_space(const char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}

inline void str_trim_space(StrView* l) {
    l->items = cstr_skip_space(items__(l));
    l->len = cstr_next_space(items__(l)) - items__(l);
}


const char* csubstr_match(const char* s, const char* cmd, size_t len) {
    if (!*s || !isalpha(*s)) { return 0x0; }
	for (; *s && isalpha(*s); ++s, ++cmd, (len?--len:len)) {
		if (*s != *cmd) { return 0x0; }
	}
    if (len) { 
        //TODO: use msg
        printf("...%s?\n", cmd);
        return 0x0;
    }
	return cstr_skip_space(s);
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

StrView str_split_line(StrView text[static 1]) {
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

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=csubstr_match(s, cmd, len)) && !*cstr_skip_space(s);
}

Err _convert_to_utf8_(
    const char* inbuf,
    const size_t inlen,
    const char* charset,
    const char* outbuf[static 1],
    size_t outlen[static 1]
) {
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
