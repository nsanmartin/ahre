#include <string.h>

#include "src/cmd-ed.h"
#include "src/constants.h"
#include "src/escape_codes.h"
#include "src/range_parse.h"
#include "src/re.h"
#include "src/str.h"
#include "src/session.h"
#include "src/textbuf.h"
#include "src/user-out.h"

#define MsgLastLine EscCodePurple "%{- last line -}%" EscCodeReset

static Err validate_range_for_buffer(TextBuf textbuf[static 1], Range range[static 1]) {
    if (!range->beg  || range->beg > range->end) { return  "error: unexpected bad range"; }
    if (textbuf_line_count(textbuf) < range->end) return "error: unexpected invalid range end";
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }
    return Ok;
}

    


//TODO: reset color during number printing (and re-establish it after).
static inline Err ed_print_n(TextBuf textbuf[static 1], Range range[static 1]) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    WriteUserOutputCallback wcb = ui_write_callback_stdout; //TODO: parameterize
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        try( ui_write_unsigned(wcb, linum));
        try( uiw_lit__(wcb,"\t"));
        if (line.len) try( uiw_strview(wcb,&line));
        else try( uiw_lit__(wcb,"\n"));
    }

    if (textbuf_line_count(textbuf) == range->end) try( uiw_lit__(wcb,"\n"));

    try(ui_flush_stdout());
    return Ok;
}

/* */

StrView parse_pattern(const char* tk) {
    StrView res = {0};
    char delim = '/';
    if (!tk) { return res; }
    tk = cstr_skip_space(tk);
    if (*tk != delim) { return res; }
    ++tk;
    const char* end = strchr(tk, delim);

    if (!end) {
        res = (StrView){.items = tk, .len=strlen(tk)};
    } else {
        res = (StrView){.items = tk, .len=end-tk};
    }
    return res;
}


Err dbg_print_all_lines_nums(TextBuf textbuf[static 1]) {
    size_t lnum = 1;
    StrView line;
    WriteUserOutputCallback wcb = ui_write_callback_stdout; //TODO: parameterize
    for (;textbuf_get_line(textbuf, lnum, &line); ++lnum) {
        try( ui_write_unsigned(wcb, lnum));
        try( uiw_lit__(wcb,"\t`"));
        if (line.len > 1) {
            try( uiw_mem(wcb,line.items, line.len-1));
        } 
        try( uiw_lit__(wcb,"'\n"));
    }
    try(ui_flush_stdout());
    return Ok;
}

static const char* _mem_find_esc_code_(const char* s, size_t len) {
    const char* res = s;
    while((res = memchr(res, '\033', len))) {
        if (res + sizeof(EscCodeReset)-1 > s + len) return NULL;
        const char code_prefix[] = "\033[";
        size_t code_prefix_len = sizeof(code_prefix) - 1;
        if (memcmp(code_prefix, res, code_prefix_len)) {
            /* strings differ */
            ++res;
        } else break;
    }
    return res;
}

Err ed_write(const char* rest, TextBuf textbuf[static 1]) {
    if (!rest || !*rest) { return "cannot write without file arg"; }
    rest = cstr_trim_space((char*)rest);
    if (!*rest) return "invalid url name";
    FILE* fp = fopen(rest, "w");
    if (!fp) return err_fmt("%s: could not open file: %s", __func__, rest); 

    const char* items = textbuf_items(textbuf);
    const char* beg = items;
    size_t len = textbuf_len(textbuf);
    while (beg && beg < items + len) {
        const char* end = _mem_find_esc_code_(beg, items + len - beg);
        if (!end) end = items + len;
        size_t chunklen = end - beg;
        if (chunklen > 1 && fwrite(beg, 1, chunklen, fp) != chunklen) {
            fclose(fp);
            return err_fmt("%s: error writing to file: %s", __func__, rest);
        }
        if (end + 1 < items + len) beg = ((char*)memchr(end + 1, 'm', items+len-(end+1))) + 1;
        else break;
    }
    if (fclose(fp)) return err_fmt("error closing file '%s'", rest);
    return Ok;
}


Err ed_global(TextBuf textbuf[static 1],  const char* rest) {
    (void)textbuf;
    StrView pattern = parse_pattern(rest);
    if (!pattern.items || !pattern.len) { return "Could not read pattern"; }
    printf("pattern: %s\n", pattern.items);
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


Err ed_print(TextBuf textbuf[static 1], Range range[static 1]) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    WriteUserOutputCallback wcb = ui_write_callback_stdout; //TODO PARAMTERIZE
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (line.len) {
            try( uiw_strview(wcb,&line));
        } else try( uiw_lit__(wcb, "\n"));
    }
    try(ui_flush_stdout());
    return Ok;
}


Err textbuf_eval_cmd(TextBuf textbuf[static 1], const char* line, Range range[static 1]) {
    line = cstr_skip_space(line);

    *textbuf_last_range(textbuf) = *range;

    const char* rest = 0x0;

    if (!*line) { return ed_print(textbuf, range); } /* default :NUM */
    if ((rest = csubstr_match(line, "a", 1)) && !*rest) { return ed_print_all(textbuf); } //TODO: depre: use :%
    if ((rest = csubstr_match(line, "l", 1)) && !*rest) { return dbg_print_all_lines_nums(textbuf); } //TODO: deprecate
    if ((rest = csubstr_match(line, "g", 1)) && *rest) { return ed_global(textbuf, rest); }
    if ((rest = csubstr_match(line, "n", 1)) && !*rest) { return ed_print_n(textbuf, range); }
    if ((rest = csubstr_match(line, "print", 1)) && !*rest) { return ed_print(textbuf, range); }
    if ((rest = csubstr_match(line, "write", 1))) { return ed_write(rest, textbuf); }
    return "unknown command";
}

Err ed_eval(TextBuf textbuf[static 1], const char* line) {
    if (!line) { return Ok; }
    Range range = {0};
    RangeParseCtx ctx = range_parse_ctx_from_textbuf(textbuf);
    line = parse_range(line, &range, &ctx);
    if (range_parse_failure(line)) {
        return range_parse_failure_to_err(line);
    } else *textbuf_last_range(textbuf) = range;

    const char* rest = 0x0;
    if ((rest = csubstr_match(line, "e", 1)) && *rest) 
        return ed_edit(textbuf, cstr_trim_space((char*)rest));
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }

    if (range.end == 0) return "error: unexpected range with end == 0";
    if (range.end > textbuf_line_count(textbuf)) return "error: range end too large";
    try( textbuf_get_offset_of_line(textbuf, range.end,textbuf_current_offset(textbuf)));
    return textbuf_eval_cmd(textbuf, line, &range);
}

Err shorcut_zb(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if(textbuf_current_line(tb) == 1) return "No more lines at the beginning";

    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }

    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    size_t beg = textbuf_current_line(tb) <= *session_nrows(session) - 1
        ? 1
        : textbuf_current_line(tb) - *session_nrows(session) - 1
        ;
    Range r = (Range){
        .beg=beg,
        .end=textbuf_current_line(tb)
    };
    try( textbuf_eval_cmd(tb, cmd, &r));
    try( textbuf_get_offset_of_line(tb, r.beg,textbuf_current_offset(tb)));

    return Ok;
}

Err shorcut_zf(Session session[static 1], const char* rest) {
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    if(textbuf_current_line(tb) > textbuf_line_count(tb)) return "No more lines in buffer";
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";

    size_t range_beg = (textbuf_last_range(tb)->beg && textbuf_last_range(tb)->end) 
        ? textbuf_last_range(tb)->end
        : textbuf_current_line(tb)
        ;
    Range r = (Range){ .beg=range_beg, .end=range_beg + *session_nrows(session) - 2 };
    if (r.end < r.beg) r.end = r.beg;
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    try( textbuf_eval_cmd(tb,cmd, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}

Err shorcut_zz(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    size_t beg = textbuf_current_line(tb) <= (*session_nrows(session)-2) / 2
        ? 1
        : textbuf_current_line(tb) - (*session_nrows(session)-2) / 2
        ;

    Range r = (Range){
        .beg=beg, .end=beg + *session_nrows(session) - 2
    };
    if (r.end < r.beg) r.end = r.beg;
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    return textbuf_eval_cmd(tb, cmd, &r);
}

Err shorcut_gg(Session session[static 1], const char* rest) {
    if (*session_nrows(session) <= 3) return "too few rows";
    const char* cmd = "print";
    if (*rest == 'n') { cmd = "n"; ++rest; }
    TextBuf* tb;
    try( session_current_buf(session, &tb));
    *textbuf_current_offset(tb) = 0;
    if(textbuf_current_line(tb) > textbuf_line_count(tb)) return "No lines in buffer";
    if (*rest) {
        rest = cstr_skip_space(rest);
        size_t incr;
        if (!parse_ull(rest, &incr)) return "invalid line number";
        *session_nrows(session) = incr;
    } 
    if (!*session_nrows(session)) return "invalid n rows";
    Range r = (Range){
        .beg=textbuf_current_line(tb),
        .end=textbuf_current_line(tb) + *session_nrows(session) - 2
    };
    if (r.end > textbuf_line_count(tb)) r.end = textbuf_line_count(tb);
    if (r.end < r.beg) r.end = r.beg;
    try( textbuf_eval_cmd(tb,cmd, &r));
    if (textbuf_current_line(tb) == r.end)
        puts(MsgLastLine);
    try( textbuf_get_offset_of_line(tb, r.end,textbuf_current_offset(tb)));
    return Ok;
}
