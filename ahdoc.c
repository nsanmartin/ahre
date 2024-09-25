#include <stdbool.h>
#include <stdio.h>
 
#include <ah/doc.h>
#include <ah/mem.h>
#include <ah/user-interface.h>

void print_help(char* program) { printf("usage: %s <url>\n", program); }

Doc* ahdoc_create(const char* fname) {
    Doc* rv = std_malloc(sizeof(Doc));
    if (!rv) { perror("Mem error"); goto exit_fail; }
    Str u;
    if (str_init(&u, fname)) { goto free_rv; }
    if (!str_is_empty(&u)){
        fname = str_ndup_cstr(&u, 5000);
        if (!fname) { goto free_rv; }
    }
    *rv = (Doc){ .url=fname, .lxbdoc=NULL, .textbuf=(TextBuf){.current_line=1} };
    return rv;
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

Session* ahdoc_session_create(char* fname, UserLineCallback callback) {
    Session* rv = std_malloc(sizeof(Session));
    if (!rv) { perror("Mem error"); goto exit_fail; }
    *rv = (Session){0};

    Doc* doc = ahdoc_create(fname);
    if (!doc) { goto free_rv; }
    Err err = doc_read_from_file(doc);
    if (err) { perror(err); goto free_rv; }

    *rv = (Session) {
        .url_client=NULL,
        .user_line_callback=callback,
        .doc=doc,
        .quit=false
    };

    return rv;
free_rv:
    destroy(rv);
exit_fail:
    return 0x0;
}

int loop_lexbor(char* url) {
    Session* session = session_create(url, process_line);
    if (!session) {
        /* TODO: do not ext, ask for another url instead */
        return EXIT_FAILURE;
    }

    while (!session->quit) {
        Err err = read_line_from_user(session);
        if (err) {
            fprintf(stderr, "%s.\n", err);
        }
    }

    destroy(session);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc > 2) { 
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
    }

    return loop_lexbor(argv[1]);
}

