#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include <user-interface.h>

#define ahre_free(PTR) free(PTR)

int ahre_read_line_from_user(AhreCtx ctx[static 1]) {
    char* line = 0x0;
    //while ((line = readline("> "))) {
    //    if (line && *line) {
    //        ctx->user_line_callback(ctx, line);
    //        add_history(line);
    //    }
    //    ahre_free(line);
    //}
    line = readline("> ");
    if (line && *line) {
        ctx->user_line_callback(ctx, line);
        add_history(line);
    }
    ahre_free(line);
    return 0;
}

int ahre_process_line(AhreCtx ctx[static 1], const char* line) {
    printf("line: `%s'\n", line);
    if (strcmp("q", line) == 0) {
        puts("quitting");
        ctx->quit = true;
    }
    return 0;
}
