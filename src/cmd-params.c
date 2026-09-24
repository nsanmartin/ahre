#include "cmd-params.h"
#include "generic.h"

/* const char* cmd_params_match(CmdParams p[_1_], const char* cmd_name, size_t unmatch) { */
/*     const char* s = p->ln; */
/*     const char* name = cmd_name; */
/*     if (!*s || !isalpha(*s)) { return 0x0; } */
/* 	for (; *s && isalpha(*s); ++s, ++name, (unmatch?--unmatch:unmatch)) { */
/* 		if (*s != *name) { return 0x0; } */
/* 	} */
/*     if (unmatch) { */ 
/*         try(msg__(p,svl("..."))); */
/*         try(msg__(p, cmd_name)); */
/*         try(msg__(p, svl("?\n"))); */
/*         return 0x0; */
/*     } */
/* 	return cstr_skip_space(s); */
/* } */

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

