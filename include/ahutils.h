#ifndef __UTILS_AHRE_H__
#define __UTILS_AHRE_H__

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

#include <aherror.h>

#define BT char
#include <buf.h>

#define T size_t
#include <arl.h>


#define buf_append_lit(LitStr, Buf_) do{\
   if (buffn(char,append)(Buf_, (char*)LitStr, sizeof(LitStr))) {\
       return -1; }}while(0)

static inline int buf_append_hexp(void* p, BufOf(char)*buf) {
    char num_buff[256];
    int len = snprintf(num_buff, 256, "%p", p);
    if (len >= 256) {
        fprintf(stdout, "truncating pointer address!\n");
        return -1;
    }
    if (!buffn(char,append)(buf, num_buff, len)) { return -1; }
    return 0;
}

typedef struct {
	const char* s;
	size_t len;
} Str;

static inline void ah_log_info(const char* msg) { puts(msg); }
static inline Error ah_log_error(const char* msg, Error e) {
    perror(msg); return e;
}

static inline char* skip_space(char* s) { for (; *s && isspace(*s); ++s); return s; }
static inline char* next_space(char* l) {
    while (*l && !isspace(*l)) { ++l; }
    return l;
}
static inline char* trim_space(char* l) {
    l = skip_space(l);
    char* r = next_space(l);
    *r = '\0';
    return l;
}

#endif
