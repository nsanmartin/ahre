#include "src/textbuf.h"
#include "src/generic.h"

/*  internal linkage */
#define READ_FROM_FILE_BUFFER_LEN 4096
_Thread_local static char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN + 1] = {0};

static bool _get_line_(TextBuf tb[static 1], size_t n, Str out[static 1]) {
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

static size_t _compute_required_newlines_in_line_(size_t linelen, size_t maxlen) {
        return (linelen / maxlen) + (bool) (linelen % maxlen != 0);
}

size_t _compute_required_newlines_(TextBuf tb[static 1], size_t maxlen) {
    size_t res = 0;
    size_t n = 0;
    Str line;
    while (_get_line_(tb, n++, &line)) {
        res += _compute_required_newlines_in_line_(line.len,  maxlen);
    }
    return res;
}

static size_t _compute_missing_newlines_(TextBuf tb[static 1], size_t maxlen) {
    return _compute_required_newlines_(tb, maxlen) - textbuf_eol_count(tb);
}

static Err _insert_missing_newlines_(TextBuf tb[static 1], size_t maxlen) {
    try( textbuf_append_line_indexes(tb));
    size_t missing_newlines = _compute_missing_newlines_(tb, maxlen);
    if (!missing_newlines) return Ok;
    if (!buffn(char, __ensure_extra_capacity)(textbuf_buf(tb), missing_newlines))
        return "error: bufof mem failure";
    size_t n = 0;
    Str line;
    BufOf(char)* buf = &(BufOf(char)){0};
    while (_get_line_(tb, n++, &line)) {
        if (line.len && line.len <= maxlen) {
            if (!buffn(char,append)(buf,(char*)line.s, line.len)) {
                buffn(char,clean)(buf);
                return "error: bufof mem failure";
            }
        } else {
            size_t i = 0;
            for (i = 0; i + maxlen <= line.len; i+=maxlen) {
                if (!buffn(char,append)(buf,(char*)line.s+i, maxlen)) {
                    buffn(char,clean)(buf);
                    return "error: bufof mem failure";
                }
                if (i + maxlen < line.len) {
                    if (!buffn(char,append)(buf,"\n", 1)) {
                        buffn(char,clean)(buf);
                        return "error: bufof mem failure";
                    }
                }
            }
            if (i < line.len) {
                if (!buffn(char,append)(buf,(char*)line.s+i, line.len-i)) {
                    buffn(char,clean)(buf);
                    return "error: bufof mem failure";
                }
            }
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

