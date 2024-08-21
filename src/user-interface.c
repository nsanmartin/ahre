#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <ahutils.h>
#include <wrappers.h>
#include <user-interface.h>
#include <user-cmd.h>


void lexbor_cp_html(lxb_html_document_t* document, BufOf(char) out[static 1]);


int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    line = readline("> ");
    ctx->user_line_callback(ctx, line);
    add_history(line);
    ah_free(line);
    return 0;
}

char* substr_match(char* s, const char* cmd, size_t len) {
    if (!*s) { return 0x0; }
	for (; *s && !isspace(*s); ++s, ++cmd, (len?--len:len)) {
		if (*s != *cmd) { return 0x0; }
	}
    if (len) { 
        printf("...%s?\n", cmd);
        return 0x0;
    }
	return skip_space(s);
}

char* subword_match(char* s, const char* cmd, size_t len) {
    char* rest = substr_match(s, cmd, len);
    if (!rest) { return 0x0; }
    rest = skip_space(rest);
    return !*rest ? rest : 0x0;
}

bool substr_match_all(char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, cmd, len)) && !*skip_space(s);
}

int ah_re_url(AhCtx ctx[static 1], char* url) {
    puts("url");
    url = trim_space(url);
    if (*url && (!ctx->ahdoc->url || strcmp(url, ctx->ahdoc->url))) {
        AhDocCleanup(ctx->ahdoc);
        if (AhDocInit(ctx->ahdoc, url)) {
            puts("Doc init error");
            return -1;
        }

        if (url) {
            ErrStr err = AhDocFetch(ctx->ahcurl, ctx->ahdoc);
            if (err) {
                //ah_log_error(err, ErrCurl);
                puts("TODO:cleanup");
                puts("error fetching url");
                return -1;
            }
        }
        printf("set with url: %s\n", url);
    } else {
       printf("url: %s\n",  ctx->ahdoc->url ? ctx->ahdoc->url : "<no url>");
    }
    return 0;
}


int ah_re_cmd(AhCtx ctx[static 1], char* line, BufOf(char)* buf) {
    char* rest = 0x0;
    line = skip_space(line);

    if ((rest = substr_match(line, "url", 1))) { return ah_re_url(ctx, rest); }

    if (!ctx->ahdoc->url || !ctx->ahdoc->url || !ctx->ahcurl) {
        ah_log_info("no document"); return 0;
    }

    else if ((rest = subword_match(line, "ahref", 2))) { return ahre_cmd_ahre(ctx, buf); }
    //else if ((rest = substr_match(line, "append", 2))) { ahre_cmd_w(rest, buf); }
    else if ((rest = substr_match(line, "attr", 2))) { puts("TODO: attr"); }
    else if ((rest = substr_match(line, "class", 3))) { puts("TODO: class"); }
    else if ((rest = substr_match(line, "clear", 3))) { return ahre_cmd_clear(buf); }
    else if ((rest = substr_match(line, "fetch", 1))) { return ahre_fetch(ctx); }
    else if ((rest = substr_match(line, "summary", 1))) { return ahctx_buffer_summary(ctx, buf); }
    else if ((rest = substr_match(line, "tag", 2))) { return ahre_cmd_tag(rest, ctx, buf); }
    //else if ((rest = substr_match(line, "text", 2))) { lexbor_print_html_text(ctx->ahdoc->doc); }
    else if ((rest = substr_match(line, "text", 2))) { return ahre_cmd_text(ctx, buf); }


    return 0;
}

int ah_ed_cmd(AhCtx ctx[static 1], char* line) {
    char* rest = 0x0;
    if (!line) { return 0; }
    else if ((rest = substr_match(line, "quit", 1)) && !*rest) { ctx->quit = true; return 0;}
    else if ((rest = substr_match(line, "print", 1)) && !*rest) { return ahed_cmd_print(ctx); }
    puts("unknown command");
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], char* line) {
    if (!line) { ctx->quit = true; return 0; }
    line = skip_space(line);

    if (!*line) { return 0; }

    if (*line == '\\') {
        if (ah_re_cmd(ctx, line + 1, &ctx->buf)) { return -1; }
        if (ctx->buf.len) {
            fwrite(ctx->buf.items, 1, ctx->buf.len, stdout);
        }

    } else {
        return ah_ed_cmd(ctx, line);
    }

    return 0;
}
