#include <errno.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <ah/utils.h>
#include <ah/wrappers.h>
#include <ah/user-interface.h>
#include <ah/user-cmd.h>
#include <ah/ranges.h>


int ah_read_line_from_user(AhCtx ctx[static 1]) {
    char* line = 0x0;
    line = readline("");
    ctx->user_line_callback(ctx, line);
    add_history(line);
    ah_free(line);
    return 0;
}

const char* substr_match(const char* s, const char* cmd, size_t len) {
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

bool substr_match_all(const char* s, size_t len, const char* cmd) {
    return (s=substr_match(s, cmd, len)) && !*skip_space(s);
}


int ahcmd_set_url(AhCtx ctx[static 1], const char* url) {
    puts("url");
    //Str u = (Str){.s=url, .len=strlen(url)};
    Str u;
    if(StrInit(&u, url)) { return -1; }
    trim_space(&u);
    if (!StrIsEmpty(&u) && (!ctx->ahdoc->url || strncmp(u.s, ctx->ahdoc->url, u.len))) {
        AhDocCleanup(ctx->ahdoc);
        if (AhDocInit(ctx->ahdoc, &u)) {
            puts("Doc init error");
            return -1;
        }

        AhDoc* doc = AhCtxCurrentDoc(ctx);
        if (AhDocHasUrl(doc)) {
            ErrStr err = AhDocFetch(ctx->ahcurl, doc);
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


int ahcmd(AhCtx ctx[static 1], const char* line) {
    const char* rest = 0x0;
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

bool is_range_valid_or_no_range(AhCtx ctx[static 1], AeRange r[static 1]) {
    AhBuf* buf = AhCtxCurrentBuf(ctx);
    size_t nlines = AhBufNLines(buf);
    return (r->beg <= r->end && r->end <= nlines)
        || r->no_range;
}

int ah_ed_cmd(AhCtx ctx[static 1], const char* line) {
    const char* rest = 0x0;
    if (!line) { return 0; }
    AeRange range = {0};
    line = ad_range_parse(line, ctx, &range);
    if (!line || !is_range_valid_or_no_range(ctx, &range)) {
        puts("invalid range");
        return 0;
    }

    if ((rest = substr_match(line, "quit", 1)) && !*rest) { ctx->quit = true; return 0;}
    if (AhBufIsEmpty(AhCtxCurrentBuf(ctx))) { 
        puts("empty buffer");
        return 0;
    }

    printf("range: %lu, %lu\n", range.beg, range.end);
    AhCtxCurrentBuf(ctx)->current_line = range.end;

    if ((rest = substr_match(line, "a", 1)) && !*rest) { return aecmd_print_all(ctx); }
    if ((rest = substr_match(line, "l", 1)) && !*rest) { return aecmd_print_all_lines_nums(ctx); }
    if ((rest = substr_match(line, "g", 1)) && *rest) { return aecmd_global(ctx, rest); }
    if ((rest = substr_match(line, "print", 1)) && !*rest) { return aecmd_print(ctx, &range); }
    if ((rest = substr_match(line, "write", 1))) { return aecmd_write(rest, ctx); }
    puts("unknown command");
    return 0;
}

int ah_process_line(AhCtx ctx[static 1], const char* line) {
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
