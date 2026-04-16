#include "cmd-params.h"

const char* cmd_params_match(CmdParams p[_1_], const char* cmd_name, size_t unmatch) {
    const char* s = p->ln;
    const char* name = cmd_name;
    if (!*s || !isalpha(*s)) { return 0x0; }
	for (; *s && isalpha(*s); ++s, ++name, (unmatch?--unmatch:unmatch)) {
		if (*s != *name) { return 0x0; }
	}
    if (unmatch) { 
        try(msg__(p,svl("...")));
        try(msg__(p, cmd_name));
        try(msg__(p, svl("?\n")));
        return 0x0;
    }
	return cstr_skip_space(s);
}

