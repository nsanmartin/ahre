#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <ahutils.h>
#include <wrappers.h>
#include <user-interface.h>
#include <user-cmd.h>
#include <ae-ranges.h>


int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    line = readline("");
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

int ahcmd_set_url(AhCtx ctx[static 1], char* url) {
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
                puts("TODO:cleanup");
                fprintf(stderr, "error fetching url: %s\n", err);
                return -1;
            }
        }
        printf("set with url: %s\n", url);
    } else {
       printf("url: %s\n",  ctx->ahdoc->url ? ctx->ahdoc->url : "<no url>");
    }
    return 0;
}


int ahcmd(AhCtx ctx[static 1], char* line) {
    char* rest = 0x0;
    line = skip_space(line);

    if ((rest = substr_match(line, "url", 1))) { return ahcmd_set_url(ctx, rest); }

    if (!ctx->ahdoc->url || !ctx->ahdoc->url || !ctx->ahcurl) {
        ah_log_info("no document"); return 0;
    }

    if ((rest = substr_match(line, "ahref", 2)) && !*rest) { return ahcmd_ahre(ctx); }
    if ((rest = substr_match(line, "attr", 2))) { puts("TODO: attr"); }
    if ((rest = substr_match(line, "class", 3))) { puts("TODO: class"); }
    if ((rest = substr_match(line, "clear", 3))) { return ahcmd_clear(ctx); }
    if ((rest = substr_match(line, "fetch", 1))) { return ahcmd_fetch(ctx); }
    if ((rest = substr_match(line, "summary", 1))) { return ahctx_buffer_summary(ctx); }
    if ((rest = substr_match(line, "tag", 2))) { return ahcmd_tag(rest, ctx); }
    if ((rest = substr_match(line, "text", 2))) { return ahcmd_text(ctx); }

    return 0;
}

bool is_range_valid(AhCtx ctx[static 1], AeRange r[static 1]) {
    size_t blen = AhCtxCurrentBuf(ctx)->buf.len;
    return r->beg <= r->end && r->end <= blen;
}

int ah_ed_cmd(AhCtx ctx[static 1], char* line) {
    char* rest = 0x0;
    if (!line) { return 0; }
    if (AeBufIsEmpty(AhCtxCurrentBuf(ctx))) { 
        puts("empty buffer");
        return 0;
    }
    AeRange range;
    rest = ad_range_parse(line, ctx, &range);
    if (!rest || !is_range_valid(ctx, &range)) {
        puts("invalid range");
        return 0;
    }
    line = rest;
    printf("range: %lu, %lu\n", range.beg, range.end);
    AhCtxCurrentBuf(ctx)->current_line = range.end;

    if ((rest = substr_match(line, "a", 1)) && !*rest) { return aecmd_print_all(ctx); }
    if ((rest = substr_match(line, "n", 1)) && !*rest) { return aecmd_print_all_lines_nums(ctx); }
    if ((rest = substr_match(line, "quit", 1)) && !*rest) { ctx->quit = true; return 0;}
    if ((rest = substr_match(line, "print", 1)) && !*rest) { return aecmd_print(ctx, &range); }
    if ((rest = substr_match(line, "write", 1))) { return aecmd_write(rest, ctx); }
    puts("unknown command");
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], char* line) {
    if (!line) { ctx->quit = true; return 0; }
    line = skip_space(line);

    if (!*line) { return 0; }

    if (*line == '\\') {
        if (ahcmd(ctx, line + 1)) { return -1; }

    } else {
        return ah_ed_cmd(ctx, line);
    }

    return 0;
}
