#ifndef AHRE_WRITER_H__
#define AHRE_WRITER_H__

#include "cmd-out.h"


typedef struct Writer Writer;
typedef struct Writer {
    Writer* self;
    Err (*write)(Writer* self, const char*, size_t);
    void* out;
} Writer;


static inline Err _writer_write_(Writer w[_1_], const char* src, size_t size) {
    return w->write(w, src, size);
}

#define writer_write(Wr, Src,Sz) (\
    Sz ? _writer_write_(Wr, Src, Sz) \
       : err_fmt("error: cannot write empty buffer ("__FILE__":%d", __LINE__))

#define writer_write_lit__(Wr, LitStr) (\
   lit_len__(LitStr) \
    ? _Generic((LitStr),\
        const char*: _writer_write_,\
        char*      : _writer_write_\
    )(Wr, LitStr, lit_len__(LitStr)) \
    : err_fmt("error: cannot write empty buffer ("__FILE__":%d", __LINE__))

/* Writer To file */
static inline Err writer_write_to_file(Writer* self, const char* mem, size_t len) {
    size_t written = fwrite(mem, 1, len, (FILE*)self->out);
    if (written != len) return "error writing to file";
    return Ok;
}

static inline Err file_writer_init(Writer w[_1_], FILE* f) {
    if (!f) return "error: failed to initialide Writer with NULL FILE";
    *w = (Writer){ .self=w, .write=writer_write_to_file, .out=f };
    return Ok;
}
// Writer to file

/* Writer To Str */
static inline Err writer_write_to_str(Writer* self, const char* mem, size_t len) {
    return str_append((Str*)self->out, sv(mem, len));
}

static inline Err str_writer_init(Writer w[_1_], Str* s) {
    if (!s) return "error: failed to initialide Writer with NULL FILE";
    *w = (Writer){ .self=w, .write=writer_write_to_str, .out=s };
    return Ok;
}
// Writer to Str

/* Writer To Msg */
static inline Err writer_write_to_msg(Writer* self, const char* mem, size_t len) {
    return msg_append((Msg*)self->out, sv(mem, len));
}

static inline Err msg_writer_init(Writer w[_1_], Msg* s) {
    if (!s) return "error: failed to initialide Writer with NULL FILE";
    *w = (Writer){ .self=w, .write=writer_write_to_msg, .out=s };
    return Ok;
}
// Writer to Str

#endif
