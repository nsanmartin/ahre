#include <string.h>

#include "src/cmd-ed.h"
#include "src/re.h"
#include "src/str.h"
#include "src/session.h"

static Err validate_range_for_buffer(TextBuf textbuf[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) { return  "error: bad range"; }
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }
    return Ok;
}

static Err
textbuf_get_line_offset_range(
    TextBuf textbuf[static 1], size_t lline, size_t rline, Range out[static 1]
) {

    if (lline > rline) return "error: inverted range";
    if (line_num_to_left_offset(lline, textbuf, &out->beg)) {
        return "error: bad range beg";
    }

    if (line_num_to_right_offset(rline, textbuf, &out->end)) {
        return "error: bad range end";
    }

    if (out->beg >= out->end || out->end > textbuf->buf.len) {
        return "error: bad offsets";
    }
    return Ok;
}

//TODO: reset color during number printing (and re-establish it after).
static inline Err ed_print_n(TextBuf textbuf[static 1], Range range[static 1]) {

    Err err = validate_range_for_buffer(textbuf, range);
    if (err) return err;

    for (size_t line = range->beg; line <= range->end; ++line) {
        Range r = {0};
        if ((err=textbuf_get_line_offset_range(textbuf, line, line, &r)))
            return err;

        if (serialize_unsigned(write_to_file, line, stdout, -1))
            return "error serializing unsigned";
        fwrite("\t", 1, 1, stdout);
        fwrite(textbuf->buf.items + r.beg, 1, r.end - r.beg, stdout);
    }
    fwrite(EscCodeReset, 1, sizeof EscCodeReset, stdout);
    if (range->end == textbuf_line_count(textbuf)) 
        fwrite("\n", 1, 1, stdout);
    return Ok;
}

static inline Err ed_print(TextBuf textbuf[static 1], Range range[static 1]) {
    Err err = validate_range_for_buffer(textbuf, range);
    if (err) return err;

    for (size_t line = range->beg; line <= range->end; ++line) {
        Range r = {0};
        if ((err=textbuf_get_line_offset_range(textbuf, line, line, &r)))
            return err;

        fwrite(textbuf->buf.items + r.beg, 1, r.end - r.beg, stdout);
    }

    fwrite(EscCodeReset, 1, sizeof EscCodeReset, stdout);

    if (range->end == textbuf_line_count(textbuf)) 
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
    rest = cstr_trim_space((char*)rest);
    if (!*rest) return "invalid url name";
    FILE* fp = fopen(rest, "w");
    if (!fp) {
        return err_fmt("%s: could not open file: %s", __func__, rest); 
    }
    if (fwrite(textbuf_items(textbuf), 1, len(textbuf), fp) != len(textbuf)) {
        return err_fmt("%s: error writing to file: %s", __func__, rest);
    }
    if (fclose(fp)) return err_fmt("error closing file '%s'", rest);
    return Ok;
}


Err ed_global(TextBuf textbuf[static 1],  const char* rest) {
    (void)textbuf;
    Str pattern = parse_pattern(rest);
    if (!pattern.s || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.s);
    return Ok;
}

Err ed_go(HtmlDoc htmldoc[static 1],  const char* rest, Range range[static 1]) {
    TextBuf* textbuf = htmldoc_textbuf(htmldoc);
    for (size_t i = range->beg; i <= range->end; ++i) {
        printf("%lu\n", i);
        size_t begoff;
        if (line_num_to_left_offset(i, textbuf, &begoff)) {
            fprintf(stderr, "? bag range beg\n");
            return "bad range beg";
        }
        size_t endoff;
        if (line_num_to_right_offset(i, textbuf, &endoff)) {
            fprintf(stderr, "? bag range end\n");
            return "bad range end";
        }

        if (begoff >= endoff || endoff > textbuf->buf.len) {
            fprintf(stderr, "!Error check offsets");
            return "error: check offsets";
        }
        fwrite(textbuf->buf.items + begoff, 1, endoff-begoff, stdout);
    }
    (void)htmldoc;
    (void)rest;
    return Ok;
}

Err textbuf_eval_cmd(Session session[static 1], const char* line, Range range[static 1]) {
    HtmlDoc* htmldoc = session_current_doc(session);
    TextBuf* textbuf = session_current_buf(session);

    const char* rest = 0x0;

    if ((rest = substr_match(line, "a", 1)) && !*rest) { return ed_print_all(textbuf); }
    if ((rest = substr_match(line, "l", 1)) && !*rest) { return dbg_print_all_lines_nums(textbuf); }
    if ((rest = substr_match(line, "go", 2)) && !*rest) { return ed_go(htmldoc, rest, range); }
    if ((rest = substr_match(line, "g", 1)) && *rest) { return ed_global(textbuf, rest); }
    if ((rest = substr_match(line, "n", 1)) && !*rest) { return ed_print_n(textbuf, range); }
    if ((rest = substr_match(line, "print", 1)) && !*rest) { return ed_print(textbuf, range); }
    if ((rest = substr_match(line, "write", 1))) { return ed_write(rest, textbuf); }
    return "unknown command";
}


Err ed_eval(Session session[static 1], const char* line) {
    if (!line) { return Ok; }
    const char* rest = 0x0;
    TextBuf* textbuf = session_current_buf(session);
    Range range = {0};
    line = parse_range(line, &range, textbuf);
    if (!line) { return "invalid range"; }

    if ((rest = substr_match(line, "e", 1)) && *rest) 
        return ed_edit(textbuf, cstr_trim_space((char*)rest));
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }

    textbuf->current_line = range.end;
    return textbuf_eval_cmd(session, line, &range);
}

