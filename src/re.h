#ifndef __AHRE_RE_H__
#define __AHRE_RE_H__


//Err regex_search_next(const char* pattern, const char* string, const char** matchp);
Err regex_search_next(const char* pattern, const char* string, size_t* off) ;

Err regex_maybe_find_next(
    const char* pattern,
    const char* string,
    size_t* off[static 1]
);
#endif
