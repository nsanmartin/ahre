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

char* substr_match(char* s, size_t len, const char* cmd) {
    if (!*s) { return 0x0; }
	for (; *s && !isspace(*s); ++s, ++cmd, (len?--len:len)) {
		if (*s != *cmd) { return 0x0; }
	}
	return len ? 0x0 : skip_space(s);
}

bool substr_match_all(char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, len, cmd)) && !*skip_space(s);
}

int ah_re_url(AhCtx ctx[static 1], char* url) {
    puts("url");
    url = skip_space(url);
    if (*url) {
        url = ah_urldup(url);
        if (!url) { return ah_log_error("urldup error", ErrMem); }
        AhDocUpdateUrl(ctx->ahdoc, url);
        printf("set with url: %s\n", url);
    } else {
       printf("url: %s\n",  ctx->ahdoc->url ? ctx->ahdoc->url : "<no url>");
    }
    return 0;
}

int ah_re_fetch(AhCtx ctx[static 1], char* url) {
    url = skip_space(url);
    if (*url) { ah_re_url(ctx, url); } 
    ErrStr err_str =  AhDocFetch(ctx->ahcurl, ctx->ahdoc);
    if (err_str) { return ah_log_error(err_str, ErrCurl); }
    return Ok;
}

int ah_re_cmd(AhCtx ctx[static 1], char* line) {
    line = skip_space(line);
    char* rest = 0x0;

    if ((rest = substr_match(line, 1, "url"))) { return ah_re_url(ctx, rest); }

    if (!ctx->ahdoc->url) { ah_log_info("no document"); return 0; }

    else if ((rest = substr_match(line, 2, "ahre"))) { puts("ahre"); if (!*skip_space(rest)) { lexbor_print_a_href(ctx->ahdoc->doc); } }
    else if ((rest = substr_match(line, 2, "attr"))) { puts("attr"); }
    else if ((rest = substr_match(line, 1, "class"))) { puts("class"); }
    else if ((rest = substr_match(line, 1, "fetch"))) { ah_re_fetch(ctx, rest); }
    else if ((rest = substr_match(line, 1, "tag"))) { puts("tag"); lexbor_print_tag(rest, ctx->ahdoc->doc); }
    return 0;
}

int ah_ed_cmd(AhCtx ctx[static 1], char* line) {
    char* rest = 0x0;
    if (!line) { return 0; }
    else if ((rest = substr_match(line, 1, "quit")) && !*rest) { ctx->quit = true; }
    else if ((rest = substr_match(line, 1, "print"))) { print_html(ctx->ahdoc->doc); }
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
