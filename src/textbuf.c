#include <errno.h>

#include "textbuf.h"
#include "generic.h"
#include "range-parse.h"
#include "re.h"
#include "session.h"

/*
 * TextBuf lines indexes are 1-based so be careful 
 */

/*  internal linkage */

static size_t _compute_required_newlines_in_line_(size_t linelen, size_t maxlen) {
        return (linelen / maxlen) + (bool) (linelen % maxlen != 0);
}

size_t _compute_required_newlines_(TextBuf tb[static 1], size_t maxlen) {
    size_t res = 0;
    size_t n = 1;
    StrView line;
    while (textbuf_get_line(tb, n++, &line)) {
        res += _compute_required_newlines_in_line_(line.len,  maxlen);
    }
    return res;
}

static size_t _estimate_missing_newlines_(TextBuf tb[static 1], size_t maxlen) {
    size_t res = _compute_required_newlines_(tb, maxlen) - textbuf_eol_count(tb);
    return res + res / 12;
}

static bool _char_is_point_of_break_(char c) {
    return isspace(c) || c == '\033';
}

//static Err _insert_line_splitting_with_esc_codes_(StrView line[static 1], BufOf(char) buf[static 1], size_t maxlen) {
//    size_t off = 0;
//    size_t len = 0;
//    for (off = 0; off < strview_len(line); ) {
//        char* beg = (char*)strview_beg(line) + off;
//        if (beg + maxlen >= strview_end(line)) {
//            len = strview_len(line) - off;
//        } else {
//            len = maxlen;
//            size_t esc_codes_len = _mem_count_escape_codes_(beg, len);
//            size_t esc_codes_mem = esc_codes_len * 4;
//            if (beg + len + esc_codes_mem < strview_end(line))
//                len += esc_codes_mem;
//
//            if (!_char_is_point_of_break_(line->items[off + len])) {
//                while (len && !_char_is_point_of_break_(line->items[off + len])) --len;
//                if (!len) len = maxlen;
//            }
//        }
//        try( bufofchar_append(buf, beg, len)) ;
//        try( bufofchar_append_lit__(buf, "\n")) ;
//        off += len;
//        if (beg + len < strview_end(line) && isspace(beg[len]))
//            ++off;
//    }
//    return Ok;
//}

static Err _insert_line_splitting_(
    StrView line[static 1],
    BufOf(char) buf[static 1],
    size_t maxlen,
    ArlOf(size_t) insertions[static 1]
) {
    size_t off = 0;
    size_t len = 0;
    for (off = 0; off < strview_len(line); ) {
        char* beg = (char*)strview_beg(line) + off;
        if (beg + maxlen >= strview_end(line)) {
            len = strview_len(line) - off;
        } else {
            len = maxlen;

            if (!_char_is_point_of_break_(line->items[off + len])) {
                while (len && !_char_is_point_of_break_(line->items[off + len])) --len;
                if (!len) len = maxlen;
            }
        }
        off += len;
        try( bufofchar_append(buf, beg, len)) ;
        if (off >= strview_len(line)) break;
        try( bufofchar_append_lit__(buf, "\n")) ;
        if (beg + len < strview_end(line) && isspace(beg[len]))
            ++off;
        else if (!arlfn(size_t,append)(insertions,&len__(buf))) return "error: arl failure";
    }
    return Ok;
}

static Err
_insert_missing_newlines_(TextBuf tb[static 1], size_t maxlen, ArlOf(size_t) insertions[static 1]) {
    size_t missing_newlines = _estimate_missing_newlines_(tb, maxlen);
    if (!missing_newlines) return Ok;
    if (!buffn(char, __ensure_extra_capacity)(textbuf_buf(tb), missing_newlines))
        return err_fmt("error: %s: bufof mem failure", __func__);
    size_t n = 1;
    StrView line;
    BufOf(char)* buf = &(BufOf(char)){0};
    while (textbuf_get_line(tb, n++, &line)) {
        Err err;
        if (line.len && line.len <= maxlen) {
            /* line includes the '\n' */
            err = bufofchar_append(buf, (char*)line.items, line.len);
        } else {
            err = _insert_line_splitting_(&line, buf, maxlen, insertions);
        }
        if (err) {
            buffn(char,clean)(buf);
            return err;
        }
    }
    buffn(char,clean)(textbuf_buf(tb));
    tb->buf = *buf;
    return  textbuf_append_line_indexes(tb);
}


/* external linkage  */

void textbuf_cleanup(TextBuf b[static 1]) {
    buffn(char, clean)(&b->buf);
    arlfn(size_t, clean)(&b->eols);
    arlfn(ModAt, clean)(textbuf_mods(b));
    *b = (TextBuf){0};
}

void textbuf_reset(TextBuf b[static 1]) {
    buffn(char, reset)(&b->buf);
    //TODO: use reset once avalable
    arlfn(size_t, clean)(&b->eols);
    arlfn(ModAt, clean)(textbuf_mods(b));
    b->current_offset = 0;
}

inline void textbuf_destroy(TextBuf* b) {
    textbuf_cleanup(b);
    std_free(b);
}


size_t* textbuf_eol_at(TextBuf tb[static 1], size_t i) {
    return arlfn(size_t, at)(&tb->eols, i);
}


Err textbuf_append_part(TextBuf textbuf[static 1], char* data, size_t len) {
    return !buffn(char,append)(&textbuf->buf, (char*)data, len)
        ?  "buffn failed to append"
        : Ok
        ;
}


Err textbuf_get_line_of_offset(TextBuf tb[static 1], size_t off, size_t* out) {
    size_t tb_len = textbuf_len(tb);
    if (off > tb_len) return "error: offset out of range";
    ArlOf(size_t)* eols = textbuf_eols(tb);
    if (!len__(eols)) { *out = 1; return Ok; }
    for ( size_t* it = arlfn(size_t,begin)(eols)
        ; it != arlfn(size_t,end)(eols)
        ; ++it
    ) {
        if (off <= *it) { *out = 1 + it - arlfn(size_t,begin)(eols); return Ok; }
    }
    *out = len__(eols);
    return Ok;
}

Err textbuf_get_line_of(TextBuf tb[static 1], const char* ch, size_t* out) {
    char* bufbeg = textbuf_items(tb);
    if (ch < bufbeg) return "error: invalid char offset in textbuf";
    size_t off = ch - bufbeg;
    if (off >= textbuf_len(tb)) { return "error: offset out of textbuf"; }
    ArlOf(size_t)* eols = textbuf_eols(tb);
    const size_t* beg = arlfn(size_t, begin)(eols);
    const size_t* it = beg;
    const size_t* end = arlfn(size_t, end)(eols);
    //TODO: use binsearch
    while (it < end && *it < off) 
        ++it;
    if (textbuf_eol_count(tb)) *out = 2 + (it-beg);
    else *out = 1;
    return  Ok;
}


static ModAt*
_skip_mods_at_the_left_(ModAt it[static 1], ModAt end[static 1], size_t offset, size_t increase) {
    for ( ; it < end && it->offset + increase < offset ; ++it)
        ;
    return it;
}

static ModAt* _move_mods_(ModAt it[static 1], ModAt end[static 1], size_t max_off, size_t increase) {
    for (;;) {
        if  (it == end || it->offset + increase >= max_off) break;
        it->offset += increase;
        ++it;
    }
    return it;
}

Err textbuf_fit_lines(TextBuf tb[static 1], size_t maxlen) {
    if (!textbuf_len(tb)) return Ok;
    ArlOf(size_t) insertions = (ArlOf(size_t)){0};
    try( _insert_missing_newlines_(tb, maxlen, &insertions));
    ModAt* mod = arlfn(ModAt,begin)(textbuf_mods(tb));
    ModAt* mend = arlfn(ModAt,end)(textbuf_mods(tb));
    const size_t* ibegin = arlfn(size_t,begin)(&insertions);

    if (insertions.len == 1 && len__(textbuf_mods(tb))) {
        size_t* it = (size_t*)ibegin;
        for (mod = mods_at_find_greater_or_eq(textbuf_mods(tb), mod, *it)
            ; mod < mend
            ; ++mod)
        {
            ++mod->offset;
        }

    } else if (insertions.len > 1 && len__(textbuf_mods(tb))) {
        size_t* from = (size_t*)ibegin;
        size_t* to = from + 1;

        for ( ; to < arlfn(size_t,end)(&insertions) ; ++from, ++to) {
            size_t increase = (1 + from - ibegin);
            mod = _skip_mods_at_the_left_(mod, mend, *from, increase);
            mod = _move_mods_(mod, mend, *to, increase);
        }

        for ( ; mod < mend ; ++mod) mod->offset += (1 + from - ibegin);
    }
    arlfn(size_t, clean)(&insertions);
    return Ok;
}

Err textbuf_append_line_indexes(TextBuf tb[static 1]) {
    char* it = textbuf_items(tb);
    char* end = it + textbuf_len(tb);
    arlfn(size_t, clean)(&tb->eols);

    for(;it < end;) {
        it = memchr(it, '\n', end-it);
        if (!it || it >= end) { break; }
        size_t index =  (it - textbuf_items(tb));
        if(NULL == arlfn(size_t, append)(&tb->eols, &index)) { return "arlfn failed to append"; }
        ++it;
    }
    return Ok;
}

/* The retrieved line includes newline (if present: the last one may not have it) */
bool textbuf_get_line(TextBuf tb[static 1], size_t n, StrView out[static 1]) {
    if (n == 0) return false; /* lines indexes are 1-based! */
    size_t nlines = textbuf_line_count(tb);
    if (nlines == 0 || nlines < n) return false;

    ArlOf(size_t)* eols = textbuf_eols(tb);
    if (n == 1) {
        /* first line: check its end */
        size_t* linelenptr = arlfn(size_t,at)(eols, 0);
        if (linelenptr) *out = (StrView){.items=textbuf_items(tb), .len=1+*linelenptr};
        else *out = (StrView){.items=textbuf_items(tb), .len=textbuf_len(tb)};
        return true;
    } else if (n <= len__(eols)) {
        size_t* begoffp = arlfn(size_t,at)(eols, n-2);
        if (!begoffp) return false; //"error: expecting len(eols) >= n-1";
        size_t* eoloffp = arlfn(size_t,at)(eols, n-1);
        if (!eoloffp) return false; //"error: expecting len(eols) >= n";
        *out = (StrView){.items=textbuf_items(tb) + *begoffp + 1, .len=*eoloffp-*begoffp};
        return true;
    } else if (n == len__(eols) + 1) {
        size_t* begoffp = arlfn(size_t,back)(eols);
        if (!begoffp) return false; //"error: expecting len(eols) > 0
        size_t eoloff = textbuf_len(tb);
        *out = (StrView){.items=textbuf_items(tb) + *begoffp + 1, .len=eoloff - *begoffp -1};
        return true;
    }
    /* n can't be grater than len(eols). If eols == 0 and line_count == 1, 0 will be
     * the first line, there no be line == 1 (second) etc.
    }
    */
    return false;
}

Err _regex_search_pattern_in_buf_(
    StrView pattern[static 1], const char* buf, size_t match_offset[static 1]
) {
    const char* lastptr = items__(pattern) + len__(pattern) - 1;
    char last           = *lastptr;
    *(char*)lastptr     = '\0';
    size_t match        = 0;
    size_t* pmatch      = &match;
    Err err             = regex_maybe_find_next(items__(pattern), buf, &pmatch);
    *(char*)lastptr     = last;
    if (err) return err;
    if (!pmatch) return "pattern not found";
    *match_offset =  match;
    return Ok;
}

Err _textbuf_regex_search_linenum(
    TextBuf tb[static 1],
    StrView pattern[static 1],
    size_t  outln[static 1],
    size_t  match_offset[static 1]
) {
    size_t current_offset = *textbuf_current_offset(tb);
    const char* buf = textbuf_items(tb) + current_offset;
    try( _regex_search_pattern_in_buf_(pattern, buf, match_offset));
    return textbuf_get_line_of_offset(tb, *match_offset, outln);
}

Err _textbuf_range_parse_to_range_(
    TextBuf          tb[static 1],
    RangeParse parse[static 1],
    Range            range_out[static 1],
    size_t           match_offset[static 1]
) {
    *range_out             = (Range){0};
    size_t current_offset  = *textbuf_current_offset(tb);
    const char* buf = textbuf_items(tb);
    switch (parse->beg.tag) {
        case range_addr_beg_tag: range_out->beg = 1 + parse->beg.delta;
            break;
        case range_addr_curr_tag: range_out->beg = textbuf_current_line(tb) + parse->beg.delta;
            break;
        case range_addr_end_tag: range_out->beg = textbuf_line_count(tb) + parse->beg.delta;
            break;
        case range_addr_none_tag: /*?*/
             if (parse->end.tag == range_addr_none_tag) {
                range_out->end = range_out->beg = textbuf_current_line(tb);
                return Ok;
             }
             return "unexpected none beg range";
        case range_addr_num_tag: range_out->beg = parse->beg.n + parse->beg.delta;
            break;
        case range_addr_search_tag:
            try( _regex_search_pattern_in_buf_(&parse->beg.s, buf + current_offset, match_offset));
            *match_offset += current_offset;
            current_offset = *match_offset;
            try( textbuf_get_line_of_offset(tb, *match_offset, &range_out->beg));
            break;
        case range_addr_prev_tag: 
            if (parse->end.tag != range_addr_prev_tag) return "error: invalid range parsed";
            *range_out = *textbuf_last_range(tb);
            return Ok;
        default: return "error: invalid RangeParse beg tag";
    }
    switch (parse->end.tag) {
        case range_addr_beg_tag: range_out->end = 1 + parse->beg.delta;
            break;
        case range_addr_curr_tag: range_out->end = textbuf_current_line(tb) + parse->end.delta;
            break;
        case range_addr_end_tag: range_out->end = textbuf_line_count(tb) + parse->end.delta;
            break;
        case range_addr_none_tag: range_out->end = range_out->beg;
            break;
        case range_addr_num_tag: range_out->end = parse->end.n + parse->end.delta;
            break;
        case range_addr_search_tag:
            try( _regex_search_pattern_in_buf_(&parse->end.s, buf + current_offset, match_offset));
            *match_offset += current_offset;
            current_offset = *match_offset;
            try( textbuf_get_line_of_offset(tb, *match_offset, &range_out->end));
            break;
        case range_addr_prev_tag: 
            return "error: range_addr_prev_tag should be in beg and end";
        default: return "error: invalid RangeParse end tag";
    }
    return Ok;
}

static inline Err _textbuf_range_validate_(TextBuf tb[static 1], Range r[static 1]) {
    if (r->end < r->beg) return "Backward r given";
    if (r->end == 0) return "error: unexpected r with end == 0";
    if (r->end > textbuf_line_count(tb)) return "r end too large";
    return Ok;
}


Err textbuf_range_from_parsed_range(
    TextBuf          textbuf[static 1],
    RangeParse rres[static 1],
    Range            range[static 1]
) {
    size_t match_offset = textbuf_len(textbuf); /* we use this value to indicate None value */
    try(_textbuf_range_parse_to_range_(textbuf, rres, range, &match_offset));
    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }
    try(_textbuf_range_validate_(textbuf, range));
    if (match_offset >= textbuf_len(textbuf))
        try( textbuf_get_offset_of_line(textbuf, range->end, textbuf_current_offset(textbuf)));
    else
        *textbuf_current_offset(textbuf) = match_offset;
    return Ok;
}

Err textbuf_to_file(TextBuf textbuf[static 1], const char* filename, const char* mode) {
    if (!filename || !*filename) { return "cannot write without file arg"; }
    filename = cstr_trim_space((char*)filename);
    if (!*filename) return "invalid url name";
    FILE* fp;
    try(file_open(filename, mode, &fp));

    const char* items = textbuf_items(textbuf);
    const char* beg = items;
    size_t len = textbuf_len(textbuf);
    if (len && !items[len-1]) --len;
    try(file_write_or_close(beg, len, fp));
    return file_close(fp);
}
