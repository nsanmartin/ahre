/* Copyright (C) 2026 Nicolas San Martin

This file is part of ahre.

ahre is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

ahre is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ahre; see the file LICENSE or see
<https://www.gnu.org/licenses/>.  */

#ifndef AHRE_HELP_H__
#define AHRE_HELP_H__
const char* _options_ = 
"options:\n"
"\t-b | --bookmark <fname>  use <fname> as bookmark file\n"
"\t-c | --cmd <str>         run <str> cmd after openning urls\n"
"\t-d | --data <data> <url> visit <url> with post method with <data>. If an error occur program\n"
"\t                         will exit.\n"
"\t-h | --help              print help and exit\n"
"\t-i | --input <iface>     pick input interce, it should be one of: fgets, isocline, visual\n"
"\t-j | --js                enable js engine by default when opening documents\n"
"\t-m | --monochrome        disable colours\n"
"\t-v | --version           print version and exit\n"
"\n\n"
"Use '?' from within ahre to get the documentation of ahre commands\n"
;
static inline void print_help(const char* prog) {
    printf(
        "usage: %s [options] [url ...]\n\n"
        "       visit url(s) (using get or post method). If an error occur, program will exit.\n"
        "       If you don't want to end the program in case some failure takes place fetching the\n"
        "       url, use -c 'get <url> ...' "
        "\n\n",
        prog
    );
    printf("%s", _options_);
}
#endif
