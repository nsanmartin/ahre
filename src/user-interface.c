#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <wrappers.h>
#include <user-interface.h>


void print_html(lxb_html_document_t* document);
static void log_info(const char* msg) { puts(msg); }

int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    line = readline("> ");
    ctx->user_line_callback(ctx, line);
    add_history(line);
    ah_free(line);
    return 0;
}

size_t cmd_match(const char* str, const char* cmd) {
	size_t len = strlen(cmd);
	size_t space = 0;
	for (; space < len && str[space] && !isspace(str[space]); ++space) {
		if (str[space] != cmd[space]) { return false; }
	}
	bool res = space > 0  && (isspace(str[space]) || (str[space] && isspace(str[space+1])));
	if (res) {
		printf("'%s' matched\n", cmd);
		return space + 1;
	}
	return 0;
}

int ah_re_cmd(AhCtx ctx[static 1], const char* line) {
    line = skip_space(line);
    if (!ctx->ahdoc->url) { log_info("no document"); return 0; }
    size_t offset = 0;
    if ((offset = cmd_match(line, "tag"))) {
        lexbor_print_tag(line + offset, ctx->ahdoc->doc);
    } else if ((offset = cmd_match(line, "attr"))) {
    } else if ((offset = cmd_match(line, "class"))) {
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
    } else if (strncmp("e ", line, 2) == 0) {
            line = skip_space(line + 2);
            const char* url = ah_urldup(line);
            if (url) {
                AhDocUpdateUrl(ctx->ahdoc, url);
                ErrStr err_str =  AhDocFetch(ctx->ahcurl, ctx->ahdoc);
                if (err_str) { fprintf(stderr, "Error fetching document: %s\n", err_str); }
            } else {
                    ah_log_err("url error");
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
