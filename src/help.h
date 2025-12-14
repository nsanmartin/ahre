#ifndef AHRE_HELP_H__
#define AHRE_HELP_H__
const char* _options_ = 
"options:\n"
"\t-b | --bookmark <fname>  use <fname> as bookmark file\n"
"\t-c | --cmd <str>         run <str> cmd prior to opennings urls\n"
"\t-d | --data <data> <url> visit <url> with post method with <data>. If an error occur program\n"
"\t                         will exit.\n"
"\t-h | --help              print help and exit\n"
"\t-i | --input <iface>     pick input interce, it should be one of: fgets, isocline, visual\n"
"\t-j | --js                enable js engine by default when opening documents\n"
"\t-m | --monochrome        disable colours\n"
"\t-v | --version           print help and exit\n"
"\n\n"
"Use '?' from within ahre to get the documentation of ahre commands\n"
;
void print_help(const char* prog) {
    printf(
        "usage: %s [options] [url ...]\n\n"
        "       visit url(s) (using get method). If an error occur, program will exit.\n"
        "       If you dont want to end the program in case some failure takes place fetching the\n"
        "       url, use -c 'get <url> ...' "
        "\n\n",
        prog
    );
    printf("%s", _options_);
}
#endif
