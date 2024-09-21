#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include <ah/doc.h>
#include <ah/mem.h>
#include <ah/user-interface.h>
#include <ah/wrappers.h>

void print_help(char* program) { printf("usage: %s <url>\n", program); }


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

