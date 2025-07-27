#include "generic.h"
#include "session.h"
#include "user-interface.h"



Err session_current_doc(Session session[static 1], HtmlDoc* out[static 1]) {
    TabList* f = session_tablist(session);
    HtmlDoc* d;
    try( tablist_current_doc(f, &d));
    *out = d;
    return Ok;
}



Err session_current_buf(Session session[static 1], TextBuf* out[static 1]) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    *out = htmldoc_textbuf(d);
    return Ok;
}

Err session_current_src(Session session[static 1], TextBuf* out[static 1]) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    *out = htmldoc_sourcebuf(d);
    return Ok;
}


Err session_init(Session s[static 1], SessionConf sconf[static 1]) {
    *s = (Session) {
        .conf         = *sconf
    };
    try( url_client_init(session_url_client(s)));

    return Ok;
}


void session_destroy(Session* session) {
    session_cleanup(session);
    std_free(session);
}

/* * */

static size_t
_next_text_end_(TextBufMods mods[static 1], ModAt it[static 1], size_t off, size_t line_end) {
    return (it < arlfn(ModAt,end)(mods)
               && off <= it->offset
               && it->offset < line_end)
        ? it->offset
        : line_end;
}


Err session_write_range_mod(
    SessionMemWriter writer[static 1], TextBuf textbuf[static 1], Range range[static 1]
) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    ModAt* it = arlfn(ModAt,begin)(textbuf_mods(textbuf));
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (!line.len || !line.items || !*line.items) continue;

        size_t line_off_beg = line.items - textbuf_items(textbuf);
        size_t line_off_end = line_off_beg + line.len;
        it = mods_at_find_greater_or_eq(textbuf_mods(textbuf), it, line_off_beg);

        if (it >= arlfn(ModAt,end)(textbuf_mods(textbuf)) || line_off_end < it->offset) {
            try( writer->write(writer, (char*)line.items, line.len));
            continue;
        }

        for (size_t off = line_off_beg; off < line_off_end;) {
            size_t next = _next_text_end_(textbuf_mods(textbuf), it, off, line_off_end);

            if (off < next)
                try( writer->write(writer, textbuf_items(textbuf) + off, next - off));

            off = next;
            while (it < arlfn(ModAt,end)(textbuf_mods(textbuf)) && next == it->offset) {
                StrView code_str;
                try( esc_code_to_str(textmod_to_esc_code(it->tmod), &code_str));
                try( writer->write(writer, (char*)code_str.items, code_str.len));
                ++it;
            }
        }
    }

    if (!session_monochrome(writer->s))
        try( writer->write(writer, EscCodeReset, sizeof(EscCodeReset) - 1));
    if (line.len && line.items[line.len-1] != '\n') 
        try( writer->write(writer, "\n", 1));
    return Ok;
}
