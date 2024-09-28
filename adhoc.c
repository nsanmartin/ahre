#include <stdbool.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
 
// #include "src/html-doc.h"
#include "src/mem.h"
#include "src/ranges.h"
#include "src/str.h"
#include "src/user-interface.h"

/* * */

typedef struct {
    const char* url;
    TextBuf textbuf;
} TextDoc;


TextBuf* textdoc_textbuf(TextDoc textdoc[static 1]) { return &textdoc->textbuf; }

Err textdoc_init(TextDoc textdoc[static 1], const char* fname) {
    textbuf_init(textdoc_textbuf(textdoc));
    Str u;
    str_init(&u, fname);
    if (!str_is_empty(&u)){
        fname = str_ndup_cstr(&u, 5000);
        if (!fname) { return "mem error"; }
    }
    textdoc->url = u.s;
    return Ok;
}


typedef struct {
    TextDoc textdoc;
    bool quit;
    //Err (*user_line_callback)(Session* session, const char*);
} TextSession;

TextDoc* text_session_textdoc(TextSession s[static 1]) { return &s->textdoc; }

TextBuf* text_session_current_buf(TextSession session[static 1]) {
    return &text_session_textdoc(session)->textbuf;
}

Err text_session_init(TextSession s[static 1], const char* fname) {
    s->quit = 0;
    return textdoc_init(text_session_textdoc(s), fname);
}

void text_session_destroy(TextSession s[static 1]) {
    free((char*)text_session_textdoc(s)->url);
}


Err adhoc_ed_eval(TextSession session[static 1], const char* line) {
    if (!line) { return Ok; }
    const char* rest = 0x0;
    Range range = {0};

    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}

    TextBuf* textbuf = text_session_current_buf(session);
    size_t current_line = textbuf->current_line;
    size_t nlines       = textbuf_line_count(textbuf);
    line = parse_range(line, &range, current_line, nlines);
    if (!line) { return "invalid range"; }

    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }

    textbuf->current_line = range.end;
    return textbuf_eval_cmd(textbuf, line, &range);
}


Err ahdoc_process_line(TextSession session[static 1], const char* line) {
    if (!line) { session->quit = true; return "no input received, exiting"; }
    line = cstr_skip_space(line);

    if (!*line) { return Ok; }
    return adhoc_ed_eval(session, line);

    return "unexpected";
}


Err ahdoc_read_line_from_user(TextSession session[static 1]) {
    char* line = 0x0;
    line = readline("");
    Err err = ahdoc_process_line(session, line);
    if (!err) {
        add_history(line);
    }
    destroy(line);
    return err;
}
/* * */


void print_help(char* program) { printf("usage: %s <url>\n", program); }



int loop_lexbor(char* url) {
    TextSession session;
    if (text_session_init(&session, url)) {
        return EXIT_FAILURE;
    }

    while (!session.quit) {
        Err err = ahdoc_read_line_from_user(&session);
        if (err) {
            fprintf(stderr, "%s.\n", err);
        }
    }

    text_session_destroy(&session);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc > 2) { 
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
    }

    return loop_lexbor(argv[1]);
}

