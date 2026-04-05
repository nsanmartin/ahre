#ifndef AHRE_SYS_H__
#define AHRE_SYS_H__

#include "error.h"
#include "utils.h"

Err resolve_path(const char *path, bool file_exists[_1_], Str out[_1_]);
#endif

