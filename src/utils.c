#include <ah/utils.h>

bool str_is_empty(const Str* s) { return !s->s || !s->len; }

