#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include "src/htmldoc.h"
#include "src/mem.h"
#include "src/user-interface.h"
#include "src/user-input.h"


void print_help(char* program) { printf("usage: %s <url>\n", program); }


int loop_lexbor(char* url) {
    Err err;
    Session session;
    if ((err=session_init(&session, url))) {
        fprintf(stderr, "%s", err);
        return EXIT_FAILURE;
    }

    init_user_input_history();
    while (!session_quit(&session)) {
        if ((err = read_line_from_user(&session))) {
            fprintf(stderr, "%s\n", err);
        }
    }

    session_cleanup(&session);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc > 2) { 
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
    }

    return loop_lexbor(argv[1]);
}
