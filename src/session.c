#include <sys/stat.h>
#include <fcntl.h>

#include "generic.h"
#include "session.h"
#include "user-interface.h"
#include "reditline.h"


Err session_current_doc(Session session[_1_], HtmlDoc* out[_1_]) {
    TabList* f = session_tablist(session);
    HtmlDoc* d;
    try( tablist_current_doc(f, &d));
    *out = d;
    return Ok;
}


Err session_current_buf(Session session[_1_], TextBuf* out[_1_]) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    *out = htmldoc_textbuf(d);
    return Ok;
}

Err session_current_src(Session session[_1_], TextBuf* out[_1_]) {
    HtmlDoc* d;
    try( session_current_doc(session, &d));
    *out = htmldoc_sourcebuf(d);
    return Ok;
}


Err session_init(Session s[_1_], SessionConf sconf[_1_]) {
    *s = (Session) {
        .conf         = *sconf
    };

    try( session_conf_set_paths(session_conf(s)));
    try( url_client_init(
            session_url_client(s),
            session_cookies_fname(s),
            strview__(USER_AGENT_USED_),
            0u
    ));
    try( str_append_datetime_now(session_dt_now_str(s)));

    return Ok;
}


void session_destroy(Session* session) {
    session_cleanup(session);
    std_free(session);
}

/* * */

static size_t
_next_text_end_(TextBufMods mods[_1_], ModAt it[_1_], size_t off, size_t line_end) {
    return (it < arlfn(ModAt,end)(mods)
               && off <= it->offset
               && it->offset < line_end)
        ? it->offset
        : line_end;
}


Err write_range_mod(
    CmdOut  cmd_out[_1_],
    bool    monochrome,
    TextBuf textbuf[_1_],
    Range   range[_1_]
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
            try( cmd_out_std_append_str(cmd_out, &line));
            continue;
        }

        for (size_t off = line_off_beg; off < line_off_end;) {
            size_t next = _next_text_end_(textbuf_mods(textbuf), it, off, line_off_end);

            if (off < next)
                try( cmd_out_std_append(cmd_out, textbuf_items(textbuf) + off, next - off));

            off = next;
            while (it < arlfn(ModAt,end)(textbuf_mods(textbuf)) && next == it->offset) {
                StrView code_str;
                try( esc_code_to_str(textmod_to_esc_code(it->tmod), &code_str));
                try( cmd_out_std_append_str(cmd_out, &code_str));
                ++it;
            }
        }
    }

    if (!monochrome)
        try( cmd_out_std_append(cmd_out, EscCodeReset, sizeof(EscCodeReset) - 1));
    if (line.len && line.items[line.len-1] != '\n') 
        try( cmd_out_std_append(cmd_out, "\n", 1));
    return Ok;
}


Err session_write_screen_range_mod(
    Session s[_1_], TextBuf textbuf[_1_], Range range[_1_], CmdOut cmd_out[_1_]
) {
    return write_range_mod(cmd_out, session_monochrome(s), textbuf, range);
}


Err session_write_fetch_history(Session s[_1_]) {
    int fd;
    FILE* fp;
    Str fetch_history_fname = (Str){0};
    /* try(get_fetch_history_filename(&fetch_history_fname)); */
    try(session_fetch_history_fname_append(s, &fetch_history_fname));

    if (-1 != (fd = open(fetch_history_fname.items, O_WRONLY | O_CREAT | O_EXCL, 0666))) {
        ssize_t nbytes = write(fd, FETCH_HISTORY_HEADER, lit_len__(FETCH_HISTORY_HEADER)) == -1;
        close(fd);
        if(nbytes == -1)
            return err_fmt("error: could not write fetch history header: %s", strerror(errno));
    } else if (errno == ENOENT) {
        return err_fmt("error: could not write fetch history header: %s", strerror(errno));
    }


    try(file_open(fetch_history_fname.items, "a", &fp));
    ArlOf(FetchHistoryEntry)* history = session_fetch_history(s);
    Err e = Ok;
    for (FetchHistoryEntry* it = arlfn(FetchHistoryEntry,begin)(history)
        ; it != arlfn(FetchHistoryEntry,end)(history)
        ; ++it
    ) {
        try_or_jump(e, Clean, fetch_history_write_to_file(it, fp));
    }
Clean:
    str_clean(&fetch_history_fname);
    file_close(fp);
    return Ok;
}


Err session_write_input_history(Session s[_1_]) {
    Str input_history_fname = (Str){0};
    FILE* fp;
    /* try(get_input_history_filename(&history_fname)); */
    try(session_input_history_fname_append(s, &input_history_fname));
    try(file_open(input_history_fname.items, "a", &fp));
    ArlOf(const_cstr)* history = session_input_history(s);
    Err e = Ok;
    Str* dt = session_dt_now_str(s);
    try_or_jump(e, Clean, file_write_or_close(items__(dt), len__(dt), fp));
    try_or_jump(e, Clean, file_write_or_close("\n", 1, fp));
    for (const_cstr* it = arlfn(const_cstr,begin)(history)
        ; it != arlfn(const_cstr,end)(history)
        ; ++it
    ) {
        try_or_jump(e, Clean, file_write(*it, strlen(*it), fp));
        try_or_jump(e, Clean, file_write("\n", 1, fp));
    }

Clean:
    file_close(fp);
    str_clean(&input_history_fname);
    return Ok;
}


Err session_close(Session s[_1_]) {
    session_write_input_history(s);
    session_write_fetch_history(s);
    return Ok;
}

void session_set_verbose(Session s[_1_], bool value) {
    url_client_set_verbose(session_url_client(s), value);
}


Err session_flush_cmd_out_msg(Session s[_1_], CmdOut cout[_1_]) {
    try(session_uout(s)->flush_msg(s, cout));
    return Ok;
}

Err session_flush_cmd_out_screen(Session s[_1_], CmdOut cout[_1_]) {
    try(session_uout(s)->flush_std(s, cout));
    return Ok;
}

Err session_flush_cmd_out(Session s[_1_], CmdOut cout[_1_]) {
    try( session_flush_cmd_out_msg(s, cout));
    try( session_flush_cmd_out_screen(s, cout));
    return Ok;
}
