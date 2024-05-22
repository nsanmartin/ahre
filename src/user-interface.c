#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include <wrappers.h>
#include <user-interface.h>


void print_html(lxb_html_document_t* document);

int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    line = readline("> ");
    ctx->user_line_callback(ctx, line);
    add_history(line);
    ah_free(line);
    return 0;
}

int ah_re_cmd(AhCtx ctx[static 1], const char* line) {
    line = skip_space(line);
    if (strncmp("tag ", line, 4) == 0) {
        lexbor_print_tag(line + 4, ctx->ahdoc->doc);
    } else if (strncmp("ahre", line, 4) == 0) {
            line += 4;
            if (!*skip_space(line)) {
                lexbor_print_a_href(ctx->ahdoc->doc);
            }
    }
    return 0;
}

int ah_ed_cmd(AhCtx ctx[static 1], const char* line) {
	if (strcmp("q", line) == 0) { ctx->quit = true; }
    else if (strcmp("p", line) == 0) {
            print_html(ctx->ahdoc->doc);
        //lexbor_print_a_href(ctx->ahdoc->doc);
    } else if (strncmp("e ", line, 2) == 0) {
            line = skip_space(line + 2);
            const char* url = ah_urldup(line);

            if (url) {
                AhDocUpdateUrl(ctx->ahdoc, url);
                if (AhDocFetch(ctx->ahcurl, ctx->ahdoc)) {
                    puts("could not fetch");
                };
            } else {
                    perror("url error");
            }

    }
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], const char* line) {
    if (!line) { ctx->quit = true; return 0; }
    line = skip_space(line);

    if (!*line) { return 0; }

    if (*line == '\\') { return ah_re_cmd(ctx, line + 1); }
    return ah_ed_cmd(ctx, line);


    return 0;
}
