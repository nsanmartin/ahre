#include <errno.h>

#include "textbuf.h"
#include "generic.h"

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
        try( bufofchar_append(buf, beg, len)) ;
        try( bufofchar_append_lit__(buf, "\n")) ;
        off += len;
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
        if (line.len && line.len <= maxlen) {
            /* line includes the '\n' */
            try( bufofchar_append(buf, (char*)line.items, line.len));
        } else {
            try( _insert_line_splitting_(&line, buf, maxlen, insertions));
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
    arlfn(ModsAt, clean)(textbuf_mods(b));
    *b = (TextBuf){0};
}

void textbuf_reset(TextBuf b[static 1]) {
    buffn(char, reset)(&b->buf);
    //TODO: use reset once avalable
    arlfn(size_t, clean)(&b->eols);
    arlfn(ModsAt, clean)(textbuf_mods(b));
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


static ModsAt* _skip_mods_at_the_left_(ModsAt it[static 1], ModsAt end[static 1], size_t offset) {
    for ( ; it < end && it->offset < offset ; ++it)
        ;
    return it;
}

static ModsAt* _move_mods_(ModsAt it[static 1], ModsAt end[static 1], size_t n, size_t max_off) {
    for ( ;; ++it) {
        if  (it == end || it->offset >= max_off) break;
        it->offset += n;
    }
    return it;
}

Err textbuf_fit_lines(TextBuf tb[static 1], size_t maxlen) {
    if (!textbuf_len(tb)) return Ok;
    ArlOf(size_t) insertions = (ArlOf(size_t)){0};
    try( _insert_missing_newlines_(tb, maxlen, &insertions));
    ModsAt* mod = arlfn(ModsAt,begin)(textbuf_mods(tb));
    ModsAt* mend = arlfn(ModsAt,end)(textbuf_mods(tb));
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
            mod = _skip_mods_at_the_left_(mod, mend, *from);
            mod = _move_mods_(mod, mend, (from-ibegin), *to);
        }

        for ( ; mod < mend ; ++mod) mod->offset += (from-ibegin);
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
