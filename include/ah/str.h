#ifndef __AHRE_STR_H__
#define __AHRE_STR_H__

#include <string.h>


typedef struct {
	const char* s;
	size_t len;
} Str;

#include <ah/utils.h>


static inline int StrInit(Str s[static 1], const char* cs) {
    *s = (Str){.s=cs, .len=cs?strlen(cs):0};
    return cs && !s->len ? -1 : 0;
}

bool StrIsEmpty(const Str* s);

/* str_ prefix is for char* + size_t fns */
//int str_get_eols(char* data, size_t base_off, size_t len, ArlOf(size_t)* ptrs);
size_t str_count_ocurrencies(char* data, size_t len, char c);

#endif
