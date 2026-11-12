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

#include "cmd-params.h"
#include "generic.h"


bool
cmd_match_prefix(StrView input, const char* cmd_name, size_t unmatch, CmdParams p[_1_]) {
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

