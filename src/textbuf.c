#include "src/textbuf.h"
#include "src/generic.h"

/*  internal linkage */
#define READ_FROM_FILE_BUFFER_LEN 4096
_Thread_local static char read_from_file_buffer[READ_FROM_FILE_BUFFER_LEN] = {-1};

static Err textbuf_append_line_indexes(TextBuf ab[static 1], char* data, size_t len);

static Err textbuf_append_line_indexes(TextBuf ab[static 1], char* data, size_t len) {
    char* end = data + len;
    char* it = data;

    for(;it < end;) {
        it = memchr(it, '\n', end-it);
        if (!it || it >= end) { break; }
        size_t index = len(ab) + (it - data);
        if(NULL == arlfn(size_t, append)(&ab->eols, &index)) { return "arlfn failed to append"; }
        ++it;
    }

    return Ok;
}

/* external linkage  */

void textbuf_cleanup(TextBuf b[static 1]) {
    buffn(char, clean)(&b->buf);
    arlfn(size_t, clean)(&b->eols);
    *b = (TextBuf){.current_line=1};
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

inline size_t textbuf_line_count(TextBuf textbuf[static 1]) {
    return textbuf->eols.len;
}


Err textbuf_append(TextBuf textbuf[static 1], char* data, size_t len) {
    Err err = Ok;
    return (err=textbuf_append_line_indexes(textbuf, data, len))
        ? err :
        ( !buffn(char,append)(&textbuf->buf, (char*)data, len) //buffn_append returns NULL on error
        ?  "buffn failed to append"
        : Ok
        )
        ;
}


Err textbuf_read_from_file(TextBuf textbuf[static 1], const char* filename) {
    FILE* fp = fopen(filename, "r");
    if  (!fp) { return strerror(errno); }

    Err err = NULL;

    size_t bytes_read = 0;
    while ((bytes_read = fread(read_from_file_buffer, 1, READ_FROM_FILE_BUFFER_LEN, fp))) {
        if ((err = textbuf_append(textbuf, read_from_file_buffer, bytes_read))) {
            return err;
        }
    }
    //TODO: free mem?
    if (ferror(fp)) { fclose(fp); return strerror(errno); }
    fclose(fp);

    return NULL;
}
