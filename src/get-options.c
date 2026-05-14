#include "get-options.h"
#include "user-out.h"
#include "config.h"
#include "generic.h"


static Err _read_bm_opt_(CliParams cparams[_1_], const char* optparam) {
    if (!optparam || !*optparam) return "bookmark file name must be provided to -b option";
    try(resolve_bookmarks_file(optparam, &cparams->sconf.bookmarks_fname));
    return Ok;
}


static Err _read_cmd_opt_(CliParams cparams[_1_], const char* optparam) {
    if (!optparam || !*optparam) return "cmd string must be provided to -c option";
    cparams->cmd = optparam;
    return Ok;
}


static Err _read_data_opt_(CliParams cparams[_1_], const char* data, const char* url) {
    if (!data || !*data) return "<data> and <url> parameters must be provided to -d option";
    if (!url || !*url) return "<url> must be provided to -d option";
    Request* r;
    try(arl_append_zero(Request,cparams_requests(cparams),r));
    try(request_from_cli_params(r, http_post, sv(url), sv(data)));
    return Ok;
}


static Err _read_input_opt_(SessionConf sconf[_1_], const char* optparam) {
    if (!optparam || !*optparam) return "invalid input option";
    if (!strcmp("fgets", optparam)) sconf->ui = ui_fgets();
    else if (!strcmp("isocline", optparam)) sconf->ui = ui_isocline();
    else if (!strcmp("visual", optparam)) sconf->ui = ui_vi_mode();
    else return "invalid input iterface: must be fgets or isocline";
    return Ok;
}

static Err _session_conf_from_options_(int argc, char* argv[], CliParams cparams[_1_]) {

    SessionConf* sconf = cparams_sconf(cparams);
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));

    *sconf = (SessionConf) {
        .ui             = ui_vi_mode(), //ui_isocline(),
        .ncols          = ncols,
        .nrows          = nrows
    };

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        // read positional parameter
        if (*arg != '-') {
            Request* r;
            try(arl_append_zero(Request,cparams_requests(cparams),r));
            try(request_init(r, http_get, sv(arg), NULL));
            continue;
        }


        
        // read short options
        if (arg[1] != '-') {
            const char* short_opt = arg + 1;
            const char* optparam;
            const char* optparam2;
            switch (*short_opt) {
                case 'b':
                    optparam = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_bm_opt_(cparams, optparam));
                    break;
                case 'c':
                    optparam = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_cmd_opt_(cparams, optparam));
                    break;
                case 'd':
                    optparam  = short_opt[1] ? &short_opt[1] : argv[++i];
                    optparam2 = argv[++i];
                    try(_read_data_opt_(cparams, optparam, optparam2));
                    break;
                case 'h': *cparams_help(cparams) = true; break;
                case 'i': // input interface
                    optparam = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_input_opt_(sconf, optparam));
                    break;
                case 'j': session_conf_js_set(sconf, true); break;
                case 'm': // monochrome
                    if (short_opt[1]) return err_fmt("unrecognized short option opt in '%s'", arg);
                    session_conf_monochrome_set(sconf, true);
                    break;
                case 'v': *cparams_version(cparams) = true; break;
                default:
                    return err_fmt("unrecognized short option '%s'", short_opt);
            }
            continue;
        }

        const char* long_opt = arg + 2;
        if (!strcmp("bookmark", long_opt)) {
            const char* optparam = argv[++i];
            try(_read_bm_opt_(cparams, optparam));
        }
        else if (!strcmp("cmd", long_opt)) {
            const char* optparam = argv[++i];
            try(_read_cmd_opt_(cparams, optparam));
        }
        else if (!strcmp("data", long_opt)) {
            const char* data = argv[++i];
            const char* url  = argv[++i];
            try(_read_data_opt_(cparams, data, url));
        }
        else if (!strcmp("help", long_opt)) *cparams_help(cparams) = true;
        else if (!strcmp("input", long_opt)) {
            const char* optparam = argv[++i];
            try(_read_input_opt_(sconf, optparam));
        }
        else if (!strcmp("js", long_opt)) session_conf_js_set(sconf, true);
        else if (!strcmp("monochrome", long_opt)) session_conf_monochrome_set(sconf, true);
        else if (!strcmp("version", long_opt)) *cparams_version(cparams) = true;
        else return err_fmt("unrecognized long option '%s'", long_opt);
    }
    return Ok;
}

Err session_conf_from_options(int argc, char* argv[], CliParams cparams[_1_]) {
    Err e = _session_conf_from_options_(argc, argv, cparams);
    if (e) cparams_clean(cparams);
    return e;
}
