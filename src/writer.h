#ifndef AHRE_WRITER_H__
#define AHRE_WRITER_H__


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

static inline Err file_writer_init(Writer w[static 1], FILE* f) {
    if (!f) return "error: failed to initialide Writer with NULL FILE";
    *w = (Writer){ .self=w, .write=writer_write_to_file, .out=f };
    return Ok;
}
// Writer to file


#endif
