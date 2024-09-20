#ifndef __AHRE_STR_H__
#define __AHRE_STR_H__

#include <string.h>


typedef struct {
	const char* s;
	size_t len;
} Str;

#include <ah/utils.h>


static inline int str_init(Str s[static 1], const char* cs) {
    *s = (Str){.s=cs, .len=cs?strlen(cs):0};
    return cs && !s->len ? -1 : 0;
}

bool str_is_empty(const Str* s);

size_t mem_count_ocurrencies(char* data, size_t len, char c);

#endif
