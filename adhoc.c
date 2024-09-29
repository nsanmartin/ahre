#include <stdbool.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
 
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
const char* textdoc_url(TextDoc textdoc[static 1]) { return textdoc->url; }

Err textdoc_init(TextDoc textdoc[static 1], const char* fname) {
    textbuf_init(textdoc_textbuf(textdoc));
    if (fname && *fname) {
        const char* end = cstr_next_space(fname);
        size_t len = end - fname;
        char* mut_buf = malloc(1 + len);
        if (!mut_buf) { return "mem error"; }
        memcpy(mut_buf, fname, 1 + len);
        memset(mut_buf + len, '\0', 1);
        destroy((char*)textdoc->url);
        textdoc->url = mut_buf;
    }
    return Ok;
}

void textdoc_cleanup(TextDoc textdoc[static 1]) {
    textbuf_cleanup(&textdoc->textbuf);
    destroy((char*)textdoc->url);
}

static inline bool textdoc_has_url(TextDoc textdoc[static 1]) { return textdoc->url; }

Err textdoc_fetch(TextDoc textdoc[static 1]) {
    return textbuf_read_from_file(&textdoc->textbuf, textdoc->url);
}

Err textdoc_set_url(TextDoc textdoc[static 1], const char* url) {
    if (textdoc_url(textdoc)) destroy((char*)textdoc_url(textdoc));
    textbuf_cleanup(&textdoc->textbuf);
    textdoc->url = url_cpy(url);
    if (textdoc_has_url(textdoc)) {
        Err err = textdoc_fetch(textdoc);
        if (err) {
            return err_fmt("\nerror reading file: '%s': %s", url, err);
        }
    }
    //    printf("set with url: %s\n", url);
    //} //else { printf("url: %s\n",  session->html_doc->url ? session->html_doc->url : "<no url>"); }
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


//Err adhoc_ed_eval(TextSession session[static 1], const char* line) {
//    if (!line) { return Ok; }
//    const char* rest = 0x0;
//    Range range = {0};
//
//    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}
//    ///if ((rest = substr_match(line, "edit", 1))) { return cmd_set_url(session, rest); }
//
//    TextBuf* textbuf = text_session_current_buf(session);
//    size_t current_line = textbuf->current_line;
//    size_t nlines       = textbuf_line_count(textbuf);
//    line = parse_range(line, &range, current_line, nlines);
//    if (!line) { return "invalid range"; }
//
//    if (textbuf_is_empty(textbuf)) { return "empty buffer"; }
//
//    textbuf->current_line = range.end;
//    return textbuf_eval_cmd(textbuf, line, &range);
//}


Err ahdoc_process_line(TextSession session[static 1], const char* line) {
    if (!line) { session->quit = true; return "no input received, exiting"; }
    line = cstr_skip_space(line);

    if (!*line) { return Ok; }
    const char* rest = NULL;
    if ((rest = substr_match(line, "quit", 1)) && !*rest) { session->quit = true; return Ok;}
    if ((rest = substr_match(line, "edit", 1))) { return textdoc_set_url(text_session_textdoc(session), rest); }
    return ed_eval(textdoc_textbuf(text_session_textdoc(session)), line);

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
    TextSession session = {0};
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

