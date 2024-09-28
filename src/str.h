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

#ifndef __AHRE_STR_H__
#define __AHRE_STR_H__

#include <string.h>


typedef struct {
	const char* s;
	size_t len;
} Str;

#include "src/generic.h"

size_t str_len(const Str s[static 1]);
bool str_is_empty(const Str s[static 1]);

int str_init(Str s[static 1], const char* cs);
bool str_is_empty(const Str s[static 1]);
size_t mem_count_ocurrencies(char* data, size_t len, char c);
const char* cstr_skip_space(const char* s);
const char* cstr_next_space(const char* l);
void str_trim_space(Str* l);
char* str_ndup_cstr(const Str* url, size_t n);

#endif
