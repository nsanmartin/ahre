#ifndef __AHRE_RE_H__
#define __AHRE_RE_H__

#ifndef NO_REGEX

Err regex_search_next(const char* pattern, const char* string, size_t* off) ;

Err regex_maybe_find_next(
    const char* pattern,
    const char* string,
    size_t* off[static 1]
);

#else
#define regex_search_next(P, S, M) "warn: regex not supported in this build"
#define regex_maybe_find_next(P, S, M) "warn: regex not supported in this build"
#endif /* NO_REGEX */

#endif
