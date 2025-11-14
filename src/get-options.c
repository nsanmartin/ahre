#include "get-options.h"
#include "user-out.h"

static Err _read_input_opt_(SessionConf sconf[static 1], const char* optopt) {
    if (!optopt || !*optopt) return "invalid input option";
    if (!strcmp("fgets", optopt)) sconf->ui = ui_fgets();
    else if (!strcmp("isocline", optopt)) sconf->ui = ui_isocline();
    else if (!strcmp("visual", optopt)) sconf->ui = ui_vi_mode();
    else return "invalid input iterface: must be fgets or isocline";
    return Ok;
}

Err session_conf_from_options(int argc, char* argv[], CliParams cparams[static 1]) {

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
            if (!arlfn(cstr_view,append)(cparams_urls(cparams), &arg))
                return "error: arl append failure";
            continue;
        }


        
        // read short options
        if (arg[1] != '-') {
            const char* short_opt = arg + 1;
            const char* optopt;
            switch (*short_opt) {
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
        if (!strcmp("help", long_opt)) *cparams_help(cparams) = true;
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

