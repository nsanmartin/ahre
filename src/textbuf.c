#include "src/textbuf.h"
#include "src/generic.h"

/*  internal linkage */
#define READ_FROM_FILE_BUFFER_LEN 4096u
/* _Thread_local */ static char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN + 1] = {0};

static size_t _compute_required_newlines_in_line_(size_t linelen, size_t maxlen) {
        return (linelen / maxlen) + (bool) (linelen % maxlen != 0);
}

size_t _compute_required_newlines_(TextBuf tb[static 1], size_t maxlen) {
    size_t res = 0;
    size_t n = 0;
    Str line;
    while (textbuf_get_line(tb, n++, &line)) {
        res += _compute_required_newlines_in_line_(line.len,  maxlen);
    }
    return res;
}

static size_t _estimate_missing_newlines_(TextBuf tb[static 1], size_t maxlen) {
    size_t res = _compute_required_newlines_(tb, maxlen) - textbuf_eol_count(tb);
    return res + res / 12;
}

static size_t _mem_count_escape_codes_(const char* buf, size_t len) {
    const char* it = buf;
    size_t count = 0;
    while (it && it < buf + len) {
        it = memchr(it, '\033', buf + len - it);
        if (it) {
            ++it;
            ++count;
        }
    }
    return count;
}

static bool _char_is_point_of_break_(char c) {
    return isspace(c) || c == '\033';
}

static Err _insert_line_splitting_(Str line[static 1], BufOf(char) buf[static 1], size_t maxlen) {
    size_t off = 0;
    size_t len = 0;
    for (off = 0; off < str_len(line); ) {
        char* beg = (char*)str_beg(line) + off;
        if (beg + maxlen >= str_end(line)) {
            len = str_len(line) - off;
        } else {
            len = maxlen;
            size_t esc_codes_len = _mem_count_escape_codes_(beg, len);
            size_t esc_codes_mem = esc_codes_len * 4;
            if (beg + len + esc_codes_mem < str_end(line))
                len += esc_codes_mem;

            if (!_char_is_point_of_break_(line->s[off + len])) {
                while (len && !_char_is_point_of_break_(line->s[off + len])) --len;
                if (!len) len = maxlen;
            }
        }
        try( bufofchar_append(buf, beg, len)) ;
        try( bufofchar_append_lit__(buf, "\n")) ;
        off += len;
        if (beg + len < str_end(line) && isspace(beg[len]))
            ++off;
    }
    return Ok;
}

static Err _insert_missing_newlines_(TextBuf tb[static 1], size_t maxlen) {
    try( textbuf_append_line_indexes(tb));
    size_t missing_newlines = _estimate_missing_newlines_(tb, maxlen);
    if (!missing_newlines) return Ok;
    if (!buffn(char, __ensure_extra_capacity)(textbuf_buf(tb), missing_newlines))
        return "error: bufof mem failure";
    size_t n = 0;
    Str line;
    BufOf(char)* buf = &(BufOf(char)){0};
    while (textbuf_get_line(tb, n++, &line)) {
        if (line.len && line.len <= maxlen) {
            try( bufofchar_append(buf, (char*)line.s, line.len));
        } else {
            try( _insert_line_splitting_(&line, buf, maxlen));
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
    *b = (TextBuf){.current_line=1};
}

void textbuf_reset(TextBuf b[static 1]) {
    buffn(char, reset)(&b->buf);
    //TODO: use reset once avalable
    arlfn(size_t, clean)(&b->eols);
    b->current_line = 1;
}

inline void textbuf_destroy(TextBuf* b) {
    textbuf_cleanup(b);
    std_free(b);
}


inline size_t textbuf_len(TextBuf textbuf[static 1]) { return textbuf->buf.len; }
inline char* textbuf_items(TextBuf textbuf[static 1]) { return textbuf->buf.items; }

size_t* textbuf_eol_at(TextBuf tb[static 1], size_t i) {
    return arlfn(size_t, at)(&tb->eols, i);
}

inline size_t textbuf_eol_count(TextBuf textbuf[static 1]) {
    return textbuf->eols.len;
}

size_t textbuf_line_count(TextBuf textbuf[static 1]) {
    size_t neols = textbuf_eol_count(textbuf);
    size_t len = textbuf_len(textbuf);
    size_t* last_eolp = arlfn(size_t, back)(&textbuf->eols);
    return neols && last_eolp
        ? neols + ( len == 1 + *last_eolp ? 0 : 1)
        : (len ? 1 : 0);
}


Err textbuf_append_part(TextBuf textbuf[static 1], char* data, size_t len) {
    return !buffn(char,append)(&textbuf->buf, (char*)data, len)
        ?  "buffn failed to append"
        : Ok
        ;
}


Err textbuf_read_from_file(TextBuf textbuf[static 1], const char* filename) {
    FILE* fp = fopen(filename, "r");
    if  (!fp) { return strerror(errno); }

    Err err = NULL;

    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if ((err = textbuf_append_part(textbuf, read_from_file_buffer, bytes_read))) {
            return err;
        }
    }
    //TODO: free me?
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);
    try( textbuf_append_null(textbuf));
    return textbuf_fit_lines(textbuf, 90);
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
    *out = 1 + (it-beg);
    return  Ok;
}

char* textbuf_line_offset(TextBuf tb[static 1], size_t line) {
    if (line == 0) { return NULL; }
    char* beg = textbuf_items(tb);
    if (line == 1) { return beg; }
    ArlOf(size_t)* eols = textbuf_eols(tb);
    size_t* it = arlfn(size_t, at)(eols, line-2);
    if (it) 
        return beg + *it;

    return NULL;
}

Err textbuf_fit_lines(TextBuf tb[static 1], size_t maxlen) {
    return _insert_missing_newlines_(tb, maxlen);
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

bool textbuf_get_line(TextBuf tb[static 1], size_t n, Str out[static 1]) {
    size_t nlines = textbuf_line_count(tb);
    if (nlines == 0 || nlines < n) return false;
    ArlOf(size_t)* eols = textbuf_eols(tb);
    if (n == 0) {
        size_t* linelenptr = arlfn(size_t,at)(eols, n);
        if (!linelenptr) return false; //"error: expecting len(eols) > 0";
        *out = (Str){.s=textbuf_items(tb), .len=*linelenptr};
        return true;
    } else if (n < len__(eols)) {
        size_t* begoffp = arlfn(size_t,at)(eols, n-1);
        if (!begoffp) return false; //"error: expecting len(eols) >= n-1";
        size_t* eoloffp = arlfn(size_t,at)(eols, n);
        if (!eoloffp) return false; //"error: expecting len(eols) >= n";
        *out = (Str){.s=textbuf_items(tb) + *begoffp + 1, .len=*eoloffp-*begoffp};
        return true;
    } else if (n == len__(eols)) {
        size_t* begoffp = arlfn(size_t,at)(eols, n-1);
        if (!begoffp) return false; //"error: expecting len(eols) >= n-1";
        size_t eoloff = textbuf_len(tb);
        *out = (Str){.s=textbuf_items(tb) + *begoffp, .len=eoloff - *begoffp};
        return true;
    }
    /* n can't be grater than len(eols). If eols == 0 and line_count == 1, 0 will be
     * the first line, there no be line == 1 (second) etc.
    }
    */
    return false;
}
