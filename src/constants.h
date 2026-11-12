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

#ifndef __CONSTANTS_AHRE_H__
#define __CONSTANTS_AHRE_H__

#include "str.h"

#define ANCHOR_OPEN_STR  "["
#define ANCHOR_SEP_STR  "."
#define ANCHOR_CLOSE_STR  "]"

#define INPUT_OPEN_STR  "{"
#define INPUT_SEP_STR  "|"
#define INPUT_CLOSE_STR  "}"

#define SELECT_SEP_STR  "-:"

#define IMAGE_OPEN_STR  "("
#define IMAGE_CLOSE_STR  ")"

#define BUTTON_OPEN_STR  "{"
#define BUTTON_SEP_STR  "^"
#define BUTTON_CLOSE_STR  "}"

#define FORM_OPEN_STR  "="
#define FORM_SEP_STR  ":" //"\u231C"
#define FORM_CLOSE_STR  "\u231F"

#define ELEM_ID_SEP "_"
//#define ELEM_ID_SEP "\u02F2"
#define ELEM_ID_SEP_BUTTON "\u25B8"


static inline StrView input_text_open_str(void) { return svl(INPUT_OPEN_STR); }
static inline StrView input_text_sep_str(void) { return svl(INPUT_SEP_STR); }
static inline StrView input_text_close_str(void) { return svl(INPUT_CLOSE_STR); }

static inline StrView input_select_sep_str(void) { return svl(SELECT_SEP_STR); }

static inline StrView input_submit_sep_str(void) { return svl("/"); }
static inline StrView input_submit_close_str(void) { return svl(INPUT_CLOSE_STR); }

static inline StrView anchor_open_str(void) { return svl(ANCHOR_OPEN_STR); }
static inline StrView anchor_sep_str(void) { return svl(ANCHOR_SEP_STR); }
static inline StrView anchor_close_str(void) { return svl(ANCHOR_CLOSE_STR); }

static inline StrView image_open_str(void) { return svl(IMAGE_OPEN_STR); }
static inline StrView image_close_str(void) { return svl(IMAGE_CLOSE_STR); }

static inline StrView button_open_str(void) { return svl(BUTTON_OPEN_STR); }
static inline StrView button_sep_str(void) { return svl(BUTTON_SEP_STR); }
static inline StrView button_close_str(void) { return svl(BUTTON_CLOSE_STR); }

static inline StrView form_open_str(void) { return svl(FORM_OPEN_STR); }
static inline StrView form_sep_str(void) { return svl(FORM_SEP_STR); }
static inline StrView form_close_str(void) { return svl(FORM_CLOSE_STR); }

static inline StrView elem_id_sep_default(void) { return svl(ELEM_ID_SEP); }
static inline StrView h_tag_open_str(void) { return svl("\n# "); }

static inline StrView newline_str(void) { return svl("\n"); }
static inline StrView space_str(void) { return svl(" "); }
#endif
