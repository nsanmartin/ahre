#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include <wrappers.h>
#include <user-interface.h>
#include <ahdoc.h>

void print_help(char* program) { printf("usage: %s <url>\n", program); }


int loop_lexbor(char* url) {
    AhCtx* ctx = AhCtxCreate(url, ah_process_line);
    if (!ctx) {
        /* TODO: do not ext, ask for another url instead */
        return EXIT_FAILURE;
    }

    while (!ctx->quit) {
        ah_read_line_from_user(ctx);
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

