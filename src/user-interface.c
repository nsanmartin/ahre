#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include <wrappers.h>
#include <user-interface.h>

#define ah_free(PTR) free(PTR)

int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    //while ((line = readline("> "))) {
    //    if (line && *line) {
    //        ctx->user_line_callback(ctx, line);
    //        add_history(line);
    //    }
    //    ah_free(line);
    //}
    line = readline("> ");
    if (line && *line) {
        ctx->user_line_callback(ctx, line);
        add_history(line);
    }
    ah_free(line);
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], const char* line) {
    printf("line: `%s'\n", line);
    if (strcmp("q", line) == 0) {
        puts("quitting");
        ctx->quit = true;
    } else if (strcmp("p", line) == 0) {
        lexbor_print_a_href(ctx->doc);
    }    

    return 0;
}
