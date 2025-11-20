#ifndef AHRE_WRITER_H__
#define AHRE_WRITER_H__

#include "htmldoc.h"

typedef struct Writer Writer;
typedef struct Writer {
    Writer* self;
    Err (*write)(Writer* self, const char*, size_t);
    void* out;
} Writer;


static inline Err writer_write(Writer w[static 1], const char* src, size_t size) {
    return w->write(w, src, size);
}

#define writer_write_lit__(Wr, LitStr) _Generic((LitStr),\
        const char*: writer_write,\
        char*      : writer_write\
    )(Wr, LitStr, lit_len__(LitStr))

/* Writer To file */
static inline Err writer_write_to_file(Writer* self, const char* mem, size_t len) {
    size_t written = fwrite(mem, 1, len, (FILE*)self->out);
    if (written != len) return "error writing to file";
    return Ok;
}

static inline Err _writer_init_to_file_(Writer w[static 1], FILE* f) {
    if (!f) return "error: failed to initialide Writer with NULL FILE";
    *w = (Writer){ .self=w, .write=writer_write_to_file, .out=f };
    return Ok;
}
// Writerto file


/* Writer To session msg */
static inline Err writer_write_to_session_msg(Writer* self, const char* mem, size_t len) {
    UserOutput* uo = session_uout(self->out);
    return uo->write_msg(mem, len, self->out);
}

static inline Err _writer_init_to_session_msg_(Writer w[static 1], Session s[static 1]) {
    *w = (Writer){ .self=w, .write=writer_write_to_session_msg, .out=s };
    return Ok;
}
// Writer To session msg

#define writer_init__(Wr, Out) _Generic((Out),\
        FILE*   : _writer_init_to_file_,\
        Session*: _writer_init_to_session_msg_\
    )(Wr, Out)
Err htmldoc_scripts_write(HtmlDoc h[static 1], RangeParse rp[static 1], Writer w[static 1]);
#endif
