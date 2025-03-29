#include "cmd-ed.h"
#include "ranges.h"
#include "session.h"
#include "textbuf.h"
#include "escape_codes.h"


static Err _vi_write_std_to_screen_( Session* s, const char* mem, size_t len) {
    if (!s) return "error: no session";
    HtmlDoc* doc;
    try( session_current_doc(s, &doc));
    Str* screen = htmldoc_screen(doc);
    return str_append(screen, (char*)mem, len);
}

#define _vi_write_std_to_screen_lit__(Ses, Lit) _vi_write_std_to_screen_(Ses, Lit, lit_len__(Lit))

static inline Err _vi_write_unsigned_std_screen_(Session s[static 1], uintmax_t ui) {
    return ui_write_unsigned(session_conf_uout(session_conf(s))->write_std, ui, s);
}

Err _vi_write_std_(const char* mem, size_t len, Session* s) {
    if (!s) return "error: no session";
    (void)mem;
    (void)len;
    return Ok;
}

static size_t
_next_text_end_(TextBufMods mods[static 1], ModsAt it[static 1], size_t off, size_t line_end) {
    return (it < arlfn(ModsAt,end)(mods)
               && off <= it->offset
               && it->offset < line_end)
        ? it->offset
        : line_end;
}

static Err _vi_print_range_std_mod_(TextBuf textbuf[static 1], Range range[static 1], Session* s) {
    try(validate_range_for_buffer(textbuf, range));
    StrView line;
    ModsAt* it = arlfn(ModsAt,begin)(textbuf_mods(textbuf));
    for (size_t linum = range->beg; linum <= range->end; ++linum) {
        if (!textbuf_get_line(textbuf, linum, &line)) return "error: invalid linum";
        if (!line.len || !line.items || !*line.items) continue;

        size_t line_off_beg = line.items - textbuf_items(textbuf);
        size_t line_off_end = line_off_beg + line.len;
        it = mods_at_find_greater_or_eq(textbuf_mods(textbuf), it, line_off_beg);

        if (it >= arlfn(ModsAt,end)(textbuf_mods(textbuf)) || line_off_end < it->offset) {
            try( _vi_write_std_to_screen_(s, (char*)line.items, line.len));
            continue;
        }

        for (size_t off = line_off_beg; off < line_off_end;) {
            size_t next = _next_text_end_(textbuf_mods(textbuf), it, off, line_off_end);

            if (off < next)
                try( _vi_write_std_to_screen_(s, textbuf_items(textbuf) + off, next - off));

            off = next;
            while (it < arlfn(ModsAt,end)(textbuf_mods(textbuf)) && next == it->offset) {
                StrView code_str;
                try( esc_code_to_str(textmod_to_esc_code(it->tmod), &code_str));
                try( _vi_write_std_to_screen_(s, (char*)code_str.items, code_str.len));
                ++it;
            }
        }
    }

    if (!session_monochrome(s))
        try( _vi_write_std_to_screen_lit__(s, EscCodeReset));
    if (line.len && line.items[line.len-1] != '\n') 
        try( _vi_write_std_to_screen_lit__(s, "\n"));
    return Ok;
}


static void _update_if_smaller_(size_t value[static 1], size_t new_value) {
    if (*value > new_value) *value = new_value;
}

#define EMPTY_SESSION_MSG_ "Session is empty\n"
#define EMPTY_BUFFER_MSG_ "Buffer is empty\n"
Err _vi_show_session_(Session* s) {
    if (!s) return "error: unexpected null session, this should really not happen";
    try( lit_write__(EscCodeClsScr, stdout));
    if (session_is_empty(s)) {
        try( str_append_lit__(session_msg(s), EMPTY_SESSION_MSG_));
        session_uout(s)->flush_std(s);
        return Ok;
    }


    TextBuf* tb;
    try( session_current_buf(s, &tb));
    if (textbuf_is_empty(tb)) {
        try( str_append_lit__(session_msg(s), EMPTY_BUFFER_MSG_));
        session_uout(s)->flush_std(s);
        return Ok;
    }

    session_uout(s)->flush_msg(s);
    size_t line = textbuf_current_line(tb);
    if (!line) return "error: expecting line number, not found";

    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));
    _update_if_smaller_(session_nrows(s), nrows - 2);
    _update_if_smaller_(session_ncols(s), ncols);

    if (nrows <= 3) return "too few rows";
    size_t end = line + *session_nrows(s);
    end = (end > textbuf_line_count(tb)) ? textbuf_line_count(tb) : end;
    Range r = (Range){ .beg=line, .end=end };

    try( _vi_print_range_std_mod_(tb, &r, s));
    session_uout(s)->flush_std(s);
    return Ok;
}

Err _vi_flush_std_(Session* s) {
    if (!s) return "error: no session";
    Str* msg = session_msg(s);
    if (len__(msg)) {
        try( mem_fwrite(items__(msg), len__(msg), stdout));
        if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
        str_reset(msg);
    }
    if (!session_is_empty(s)) {
        HtmlDoc* doc;
        try( session_current_doc(s, &doc));
        Str* screen = htmldoc_screen(doc);
        if (!len__(screen)) return Ok;
        try( mem_fwrite(items__(screen), len__(screen), stdout));
        if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
        str_reset(screen);
    }
    return Ok;
}


Err _vi_write_msg_(const char* mem, size_t len, Session* s) {
    if (!s) return "error: no session";
    return str_append(session_msg(s), (char*)mem, len);
}

#define MSG_PREFIX_ "{msg}:\n"
#define CONTINUE_MSG_ "{% type enter to continue %}"
Err _vi_flush_msg_(Session* s) {
    if (!s) return "error: no session";
    Str* msg = session_msg(s);
    if (!len__(msg)) return Ok;
    try( lit_write__(MSG_PREFIX_, stdout));
    try( mem_fwrite(items__(msg), len__(msg), stdout));
    try( lit_write__(CONTINUE_MSG_, stdout));
    if (fflush(stdout)) return err_fmt("error: fflush failure: %s", strerror(errno));
    getchar();
    str_reset(msg);
    return Ok;
}

Err _vi_show_err_(Session* s, char* err, size_t len) {
    (void)s;
    FILE* stream = stdout;
    if (err) {
        if (mem_fwrite(err, len, stream)
        || lit_write__(" {type enter}", stream))
            return "error: fprintf failure while attempting to show an error :/";

    }
    fflush(stream);
    getchar();
    return Ok;
}

