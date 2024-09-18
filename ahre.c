#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include <ah/wrappers.h>
#include <ah/user-interface.h>
#include <ah/doc.h>

void print_help(char* program) { printf("usage: %s <url>\n", program); }


int loop_lexbor(char* url) {
    AhCtx* ctx = AhCtxCreate(url, ah_process_line);
    if (!ctx) {
        /* TODO: do not ext, ask for another url instead */
        return EXIT_FAILURE;
    }

    while (!ctx->quit) {
        int err = ah_read_line_from_user(ctx);
        if (err) {
            fprintf(stderr, "Ah re: error reading line, aborting.\n");
            return EXIT_FAILURE;
        }
    }

    AhCtxFree(ctx);
    return EXIT_SUCCESS;
}


int main(int argc, char **argv) {
    if (argc > 2) { 
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
    }

    return loop_lexbor(argv[1]);
}

