#include "cmd-params.h"
#include "generic.h"


bool cmd_match_substring(StrView input, const char* cmd_name, size_t unmatch, CmdParams p[_1_]) {
    const char* s = input.items;
    const char* name = cmd_name;
    if (!*s || !isalpha(*s)) { return false; }
	for (; *s && isalpha(*s); ++s, ++name, (unmatch?--unmatch:unmatch)) {
		if (*s != *name) { return false; }
	}
    if (unmatch) { 
        try(msg__(p,svl("...")));
        try(msg__(p, cmd_name));
        try(msg__(p, svl("?\n")));
        return false;
    }
	return true;
}

