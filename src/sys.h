#ifndef AHRE_SYS_H__
#define AHRE_SYS_H__

#include "error.h"
#include "utils.h"

Err resolve_path(const char *path, bool* file_exists, Str out[_1_]);

#define file_write_or_close(Mem, Len, Fp) _file_write_or_close_(Mem, Len, Fp, __FILE__)
#define file_write(Mem, Len, Fp) _file_write_(Mem, Len, Fp, __FILE__)
#define file_write_lit_sep(Mem, Len, LitSep, Fp) \
    _file_write_sep_(Mem, Len, LitSep, lit_len__(LitSep), Fp, __FILE__)

#define file_write_int_lit_sep(Int, LitSep, Fp) \
    _file_write_int_sep_(Int, LitSep, lit_len__(LitSep), Fp, __FILE__)

Err _file_write_int_sep_(intmax_t i, const char* sep, size_t seplen, FILE* fp, const char* caller);

Err _file_write_or_close_(const char* mem, size_t len, FILE* fp, const char* caller);
Err file_open(const char* filename, const char* mode, FILE* fpp[_1_]);
Err _file_write_(const char* mem, size_t len, FILE* fp, const char* caller);
Err file_close(FILE* fp);
Err _file_write_sep_(
    const char* mem, size_t len, const char* sep, size_t seplen, FILE* fp, const char* caller
);
Err expand_path(const char *path, Str out[_1_]);
bool path_is_dir(const char* path);
#endif
