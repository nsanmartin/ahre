#ifndef __AHRE_RE_H__
#define __AHRE_RE_H__

#ifndef AHRE_REGEX_DISABLED

Err regex_search_next(const char* pattern, const char* string, size_t* off) ;

Err regex_maybe_find_next(
    const char* pattern,
    const char* string,
    size_t* off[_1_]
);

#else
#define regex_search_next(P, S, M) "warn: regex not supported in this build"
#define regex_maybe_find_next(P, S, M) "warn: regex not supported in this build"
#endif /* AHRE_REGEX_DISABLED */

#endif /* __AHRE_RE_H__ */
