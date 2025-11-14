#ifndef AHRE_HELP_H__
#define AHRE_HELP_H__
const char* _options_ = 
"options:\n"
"\t-h | --help             print help and exit\n"
"\t-i | --input <iface>    pick input interce, it should be one of: fgets, isocline, visual\n"
"\t-j | --js               enable js engine by default when opening documents\n"
"\t-m | --monochrome       disable colours\n"
"\t-v | --version          print help and exit\n"
"\n\n"
"Use '?' from within ahre to get the documentation of ahre commands\n"
;
void print_help(const char* prog) {
    printf("usage: %s [options] [url ...]    visit url(s)\n\n", prog);
    printf("%s", _options_);
}
#endif
