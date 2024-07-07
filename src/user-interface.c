#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <ahutils.h>
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

char* str_match(char* s, const char* cmd) {
    if (!*s) { return 0x0; }
	for (; *s && !isspace(*s); ++s, ++cmd) {
		if (*s != *cmd) { return 0x0; }
	}
	return s;
}

size_t cmd_match(char* str, char* cmd) {
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

int ah_re_cmd(AhCtx ctx[static 1], char* line) {
    line = skip_space(line);
    char* rest = 0x0;
    if ((rest = str_match(line, "url"))) {
        puts("url");
        if (!*skip_space(line)) {
           printf("url: %s\n",  ctx->ahdoc->url ? ctx->ahdoc->url : "<no url>");
        } else {
            printf("set with url: %s\n", rest);
        }
        return 0;
    }

    if (!ctx->ahdoc->url) { ah_log_info("no document"); return 0;
    } else if ((rest = str_match(line, "fetch"))) {
        puts("fetch");
    } else if ((rest = str_match(line, "tag"))) {
        puts("tag");
        lexbor_print_tag(line, ctx->ahdoc->doc);
    } else if ((rest = str_match(line, "attr"))) {
        puts("attr");
    } else if ((rest = str_match(line, "class"))) {
        puts("class");
    } else if ((rest = str_match(line, "ahre"))) {
        puts("ahre");
            if (!*skip_space(rest)) {
                lexbor_print_a_href(ctx->ahdoc->doc);
            }
    }
    return 0;
}

int ah_ed_cmd(AhCtx ctx[static 1], char* line) {
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
                    ah_log_error("url error", ErrMem);
            }

    }
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], char* line) {
    if (!line) { ctx->quit = true; return 0; }
    line = skip_space(line);

    if (!*line) { return 0; }

    if (*line == '\\') { return ah_re_cmd(ctx, line + 1); }
    return ah_ed_cmd(ctx, line);


    return 0;
}
