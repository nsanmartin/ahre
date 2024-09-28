#include <string.h>

#include "src/cmd-ed.h"
#include "src/str.h"

static inline Err ed_print(TextBuf textbuf[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) {
        fprintf(stderr, "! bad range\n");
        return  "bad range";
    }

    if (textbuf_is_empty(textbuf)) {
        fprintf(stderr, "? empty buffer\n");
        return "empty buffer";
    }

    size_t beg_off_p;
    if (line_num_to_left_offset(range->beg, textbuf, &beg_off_p)) {
        fprintf(stderr, "? bag range beg\n");
        return "bad range beg";
    }
    size_t end_off_p;
    if (line_num_to_right_offset(range->end, textbuf, &end_off_p)) {
        fprintf(stderr, "? bag range end\n");
        return "bad range end";
    }

    if (beg_off_p >= end_off_p || end_off_p > textbuf->buf.len) {
        fprintf(stderr, "!Error check offsets");
        return "error: check offsets";
    }
    fwrite(textbuf->buf.items + beg_off_p, 1, end_off_p-beg_off_p, stdout);
    fwrite("\n", 1, 1, stdout);
    return Ok;
}

/* */

Str parse_pattern(const char* tk) {
    Str res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = cstr_skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) {
        res = (Str){.s = tk, .len=strlen(tk)};
    } else {
        res = (Str){.s = tk, .len=end-tk};
    }
    return res;
}


Err dbg_print_all_lines_nums(TextBuf textbuf[static 1]) {
    size_t len = len(textbuf);
    char* items = textbuf_items(textbuf);
    char* end = items + len;
    

    for (size_t lnum = 1; /*items && len && lnum < 40*/ ; ++lnum) {
        char* next = memchr(items, '\n', len);
        if (next >= end) {
            fprintf(stderr, "Error: print all lines nums\n");
            return  "Error: print all lines nums.";
        }
        printf("%ld: ", lnum);

        if (next) {
            size_t line_len = 1+next-items;
            fwrite(items, 1, line_len, stdout);
            items += line_len;
            len -= line_len;
        } else {
            fwrite(items, 1, len, stdout);
            break;
        }
    }
    return Ok;
}

Err ed_write(const char* rest, TextBuf textbuf[static 1]) {
    if (!rest || !*rest) { return "cannot write without file arg"; }
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        return err_fmt("%s: could not open file: %s", __func__, rest); 
    }
    if (fwrite(textbuf_items(textbuf), 1, len(textbuf), fp) != len(textbuf)) {
        return err_fmt("%s: error writing to file: %s", __func__, rest);
    }
    return Ok;
}


Err ed_global(TextBuf textbuf[static 1],  const char* rest) {
    (void)textbuf;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.s);
    return Ok;
}


Err textbuf_eval_cmd(TextBuf textbuf[static 1], const char* line, Range range[static 1]) {
    const char* rest = 0x0;

    if ((rest = substr_match(line, "a", 1)) && !*rest) { return ed_print_all(textbuf); }
    if ((rest = substr_match(line, "l", 1)) && !*rest) { return dbg_print_all_lines_nums(textbuf); }
    if ((rest = substr_match(line, "g", 1)) && *rest) { return ed_global(textbuf, rest); }
    if ((rest = substr_match(line, "print", 1)) && !*rest) { return ed_print(textbuf, range); }
    if ((rest = substr_match(line, "write", 1))) { return ed_write(rest, textbuf); }
    return "unknown command";
}

