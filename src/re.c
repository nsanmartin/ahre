#include <stdio.h>

#include "error.h"
#ifndef AHRE_REGEX_DISABLED
#include <regex.h>

Err regex_search_next_(const char* pattern, const char* string, const char** matchp) {

    regex_t regex;
    if (regcomp(&regex, pattern, REG_NEWLINE))
        return err_fmt("regex error compiling patter: %s", pattern);

    regmatch_t  pmatch[1];
    int status = regexec(&regex, string, 1, pmatch, 0);
    if (status == REG_NOMATCH) {
        *matchp = NULL;
    } else if (status) {
        return err_fmt("Error executing regex, status: %d\n", status);
    }

    regoff_t off = pmatch[0].rm_so; // + (s - str);
    ///regoff_t len = pmatch[0].rm_eo - pmatch[0].rm_so;
    *matchp = string + off;

    regfree(&regex);

    return Ok;
}

Err regex_search_next(const char* pattern, const char* string, size_t* off) {

    regex_t regex;
    if (regcomp(&regex, pattern, REG_NEWLINE))
        return err_fmt("error compiling regex pattern: %s", pattern);

    regmatch_t  pmatch[1];
    int status = regexec(&regex, string, 1, pmatch, 0);
    if (status == REG_NOMATCH) {
        return "pattern not found";
    } else if (status) {
        return err_fmt("error executing regex, status: %d\n", status);
    }

    *off = pmatch[0].rm_so;

    regfree(&regex);

    return Ok;
}

Err regex_maybe_find_next(const char* pattern, const char* string, size_t* off[1]) {

    regex_t regex;
    if (regcomp(&regex, pattern, REG_NEWLINE))
        return err_fmt("error compiling regex pattern: %s", pattern);

    regmatch_t  pmatch[1];
    int status = regexec(&regex, string, 1, pmatch, 0);
    if (status == REG_NOMATCH) {
        *off = NULL;
        regfree(&regex);
        return Ok;
    } else if (status) {
        regfree(&regex);
        *off = NULL;
        return err_fmt("error executing regex, status: %d\n", status);
    }

    **off = pmatch[0].rm_eo;

    regfree(&regex);

    return Ok;
}

#else /* regex disabled: */
typedef int _regex_disabled_;
#endif /* AHRE_REGEX_DISABLED */
