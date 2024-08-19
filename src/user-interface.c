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

int ah_re_fetch(AhCtx ctx[static 1]) {
    if (ctx->ahdoc->url) {
        ErrStr err_str = AhDocFetch(ctx->ahcurl, ctx->ahdoc);
        if (err_str) { return ah_log_error(err_str, ErrCurl); }
        return Ok;
    }
    puts("Not url to fech");
    return -1;
}


int ah_re_cmd(AhCtx ctx[static 1], char* line) {
    //BufOf(char)* b = &(BufOf(char)){0};

    char* rest = 0x0;
    line = skip_space(line);

    if ((rest = substr_match(line, "url", 1))) { return ah_re_url(ctx, rest); }

    if (!ctx->ahdoc->url || !ctx->ahdoc->url || !ctx->ahcurl) {
        ah_log_info("no document"); return 0;
    }

    else if ((rest = substr_match(line, "ahre", 2))) { if (!*skip_space(rest)) { lexbor_href_write(ctx->ahdoc->doc, &ctx->ahdoc->cache.hrefs, &ctx->buf); } }
    else if ((rest = substr_match(line, "append", 2))) { ahre_cmd_w(rest, &ctx->buf); }
//        lexbor_cp_html(ctx->ahdoc->doc, &ctx->ahdoc->buf); }
    else if ((rest = substr_match(line, "attr", 2))) { puts("attr"); }
    else if ((rest = substr_match(line, "class", 1))) { puts("class"); }
    else if ((rest = substr_match(line, "fetch", 1))) { ah_re_fetch(ctx); }
    //else if ((rest = substr_match(line, "print", 1))) { ctx->buf = &ctx->ahdoc->buf; }
    else if ((rest = substr_match(line, "summary", 1))) { ahctx_buffer_summary(ctx, &ctx->buf); }
    //else if ((rest = substr_match(line, "summary", 1))) { ahctx_print_summary(ctx, stdout); }
    //else if ((rest = substr_match(line, "print", 1))) { lexbor_cp_html(ctx->ahdoc->doc, &b); }
    else if ((rest = substr_match(line, "tag", 2))) { puts("tag"); lexbor_print_tag(rest, ctx->ahdoc->doc); }
    else if ((rest = substr_match(line, "text", 2))) { lexbor_print_html_text(ctx->ahdoc->doc); }

    if (ctx->buf.len) {
        fwrite(ctx->buf.items, 1, ctx->buf.len, stdout);
        //free(b->items);
        //puts("ah re cmo buf");
    }

    return 0;
}

int ah_ed_cmd(AhCtx ctx[static 1], char* line) {
    char* rest = 0x0;
    if (!line) { return 0; }
    else if ((rest = substr_match(line, "quit", 1)) && !*rest) { ctx->quit = true; return 0;}
    else if ((rest = substr_match(line, "print", 1)) && !*rest) { return ah_ed_cmd_print(ctx); }
    puts("unknown command");
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
