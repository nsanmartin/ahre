#include <ae-ranges.h>

/*
 * + . current line,
 * + $ last line in the buffer
 * + 1 first line in the buffer
 * + 0 line before the first line
 * + 't is the line in which mark t is placed
 * + '< and '>  the previous selection begins and ends
 * + /pat/ is the next line where pat matches
 * + \/ the next line in which the previous search pattern matches
 * + \& the next line in which the previous substitute pattern matches 
*/

int ad_range_parse_beg(char* tk, size_t* beg) {
    if (!tk || !*tk) { return -1; }
    if (*tk == '.') 
}

int ad_range_parse_end(char* tk, size_t* end) {
}

int ad_range_parse(char* tk, AhRange* range) {
}
