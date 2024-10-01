#include <stdbool.h>
#include <stdio.h>
 
#include "src/mem.h"
#include "src/ranges.h"
#include "src/str.h"
#include "src/cmd-ed.h"

/* * */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


int getline(char **lineptr, size_t *n, FILE *stream) {
    static char line[256];
    char *ptr;
    unsigned int len;

   if (lineptr == NULL || n == NULL) {
      errno = EINVAL;
      return -1;
   }

   if (ferror (stream))
      return -1;

   if (feof(stream))
      return -1;
     
   fgets(line,256,stream);

   ptr = strchr(line,'\n');   
   if (ptr)
      *ptr = '\0';

   len = strlen(line);
   
   if ((len+1) < 256) {
      ptr = realloc(*lineptr, 256);
      if (ptr == NULL)
         return(-1);
      *lineptr = ptr;
      *n = 256;
   }

   strcpy(*lineptr,line); 
   return(len);
}

char* adhoc_readline (const char *prompt) {
    char *line = NULL;
    size_t len = 0;
    int read;

    printf("%s", prompt);
    read = getline(&line, &len, stdin);
    if (read == -1) { free(line); line = 0x0; }
    return line;

}

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
        std_free((char*)textdoc->url);
        textdoc->url = mut_buf;
    }
    return Ok;
}

void textdoc_cleanup(TextDoc textdoc[static 1]) {
    textbuf_cleanup(&textdoc->textbuf);
    std_free((char*)textdoc->url);
}

static inline bool textdoc_has_url(TextDoc textdoc[static 1]) { return textdoc->url; }

Err textdoc_fetch(TextDoc textdoc[static 1]) {
    return textbuf_read_from_file(&textdoc->textbuf, textdoc->url);
}

Err textdoc_set_url(TextDoc textdoc[static 1], const char* url) {
    if (textdoc_url(textdoc)) std_free((char*)textdoc_url(textdoc));
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
    line = adhoc_readline("");
    Err err = ahdoc_process_line(session, line);
    ///if (!err) {
    ///    add_history(line);
    ///}
    std_free(line);
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

