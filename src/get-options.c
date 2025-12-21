#include "get-options.h"
#include "user-out.h"
#include "config.h"


static Err _read_bm_opt_(CliParams cparams[_1_], const char* optopt) {
    if (!optopt || !*optopt) return "invalid data option";
    try(resolve_bookmarks_file(optopt, &cparams->sconf.bookmarks_fname));
    return Ok;
}


static Err _read_cmd_opt_(CliParams cparams[_1_], const char* optopt) {
    if (!optopt || !*optopt) return "invalid data option";
    cparams->cmd = optopt;
    return Ok;
}


static Err _read_data_opt_(CliParams cparams[_1_], const char* data, const char* url) {
    if (!data || !*data) return "invalid -d data optopt";
    if (!url || !*url) return "invalid -d url optopt";
    Err err = Ok;
    Str urlstr = (Str){0};
    Request r = (Request){0};

    try(str_append(&urlstr, (char*)url, strlen(url) + 1));
    try_or_jump(err, Fail_Clean_Post_Fields, request_init_move_urlstr(&r, http_post, &urlstr, NULL));
    try_or_jump(err, Fail_Clean_Url_Str, str_append(&r.postfields, (char*)data, strlen(data)));
    if (!arlfn(Request,append)(cparams_requests(cparams), &r))
        return "error: arl append failure";

    return Ok;
Fail_Clean_Post_Fields:
    str_clean(&r.postfields);
Fail_Clean_Url_Str:
    str_clean(&urlstr);
    return err;
}


static Err _read_input_opt_(SessionConf sconf[_1_], const char* optopt) {
    if (!optopt || !*optopt) return "invalid input option";
    if (!strcmp("fgets", optopt)) sconf->ui = ui_fgets();
    else if (!strcmp("isocline", optopt)) sconf->ui = ui_isocline();
    else if (!strcmp("visual", optopt)) sconf->ui = ui_vi_mode();
    else return "invalid input iterface: must be fgets or isocline";
    return Ok;
}

Err _session_conf_from_options_(int argc, char* argv[], CliParams cparams[_1_]) {

    SessionConf* sconf = cparams_sconf(cparams);
    size_t nrows, ncols;
    try( ui_get_win_size(&nrows, &ncols));

    *sconf = (SessionConf) {
        .ui             = ui_vi_mode(), //ui_isocline(),
        .ncols          = 88 > ncols ? ncols : 88,
        .nrows          = nrows
    };

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        // read positional parameter
        if (*arg != '-') {
            Str urlstr = (Str){0};
            try(str_append(&urlstr, (char*)arg, strlen(arg) + 1));
            Request r;
            Err err = request_init_move_urlstr(&r, http_get, &urlstr, NULL);
            if (err) { str_clean(&urlstr); return err; }
            if (!arlfn(Request,append)(cparams_requests(cparams), &r))
                return "error: arl append failure";
            continue;
        }


        
        // read short options
        if (arg[1] != '-') {
            const char* short_opt = arg + 1;
            const char* optopt;
            const char* optopt2;
            switch (*short_opt) {
                case 'b':
                    optopt = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_bm_opt_(cparams, optopt));
                    break;
                case 'c':
                    optopt = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_cmd_opt_(cparams, optopt));
                    break;
                case 'd':
                    optopt  = short_opt[1] ? &short_opt[1] : argv[++i];
                    optopt2 = argv[++i];
                    try(_read_data_opt_(cparams, optopt, optopt2));
                    break;
                case 'h': *cparams_help(cparams) = true; break;
                case 'i': // input interface
                    optopt = short_opt[1] ? &short_opt[1] : argv[++i];
                    try(_read_input_opt_(sconf, optopt));
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
            const char* optopt = argv[++i];
            try(_read_bm_opt_(cparams, optopt));
        }
        else if (!strcmp("cmd", long_opt)) {
            const char* optopt = argv[++i];
            try(_read_cmd_opt_(cparams, optopt));
        }
        else if (!strcmp("data", long_opt)) {
            const char* data = argv[++i];
            const char* url  = argv[++i];
            try(_read_data_opt_(cparams, data, url));
        }
        else if (!strcmp("help", long_opt)) *cparams_help(cparams) = true;
        else if (!strcmp("input", long_opt)) {
            const char* optopt = argv[++i];
            try(_read_input_opt_(sconf, optopt));
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
