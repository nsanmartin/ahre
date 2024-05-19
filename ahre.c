#include <stdbool.h>
#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
 
#include <wrappers.h>
#include <user-interface.h>
#include <ahdoc.h>

void print_help(const char* program) { printf("usage: %s [-b (lxb|tidy)] <url>\n", program); }

typedef enum { LxbBe, TidyBe } BeLib;


int loop_lexbor(const char* url) {
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
    bool bad_input = false;
    if (argc != 2 && argc != 4) { goto exit_error; }

    BeLib be_lib = LxbBe; /* default is lexbor */

    size_t param = 1;
    if  (argc == 4) {
        if (strcmp("-b", argv[param]) == 0) {
            if (strcmp("lxb", argv[param+1]) == 0) {
                be_lib = LxbBe;
                goto opt_read;
            } else if (strcmp("tidy", argv[param+1]) == 0) {
                be_lib = TidyBe;
                goto opt_read;
            } 
        } 
exit_error:
        print_help(argv[0]);
        exit(EXIT_FAILURE); 
opt_read:
        param += 2;
    }

    if (!bad_input) {
        switch (be_lib) {
            case LxbBe: {
                return loop_lexbor(argv[param]);
            }
            case TidyBe: { return ah_tidy(argv[param]); }
            default: { goto exit_error; }
        }
    }

}

