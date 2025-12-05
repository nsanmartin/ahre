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
#define BUTTON_CLOSE_STR  "}"

#define FORM_OPEN_STR  "\u231C" //[form "
#define FORM_CLOSE_STR  "\u231F"

#define ELEM_ID_SEP "_"
//#define ELEM_ID_SEP "\u02F2"
#define ELEM_ID_SEP_BUTTON "\u25B8"


static inline StrView input_text_open_str(void) { return strview_from_lit__(INPUT_OPEN_STR); }
static inline StrView input_text_sep_str(void) { return strview_from_lit__(INPUT_SEP_STR); }
static inline StrView input_text_close_str(void) { return strview_from_lit__(INPUT_CLOSE_STR); }

static inline StrView input_select_sep_str(void) { return strview_from_lit__(SELECT_SEP_STR); }

static inline StrView input_submit_sep_str(void) { return strview_from_lit__("|"); }
static inline StrView input_submit_close_str(void) { return strview_from_lit__(INPUT_CLOSE_STR); }

static inline StrView anchor_open_str(void) { return strview_from_lit__(ANCHOR_OPEN_STR); }
static inline StrView anchor_sep_str(void) { return strview_from_lit__(ANCHOR_SEP_STR); }
static inline StrView anchor_close_str(void) { return strview_from_lit__(ANCHOR_CLOSE_STR); }

static inline StrView image_open_str(void) { return strview_from_lit__(IMAGE_OPEN_STR); }
static inline StrView image_close_str(void) { return strview_from_lit__(IMAGE_CLOSE_STR); }

static inline StrView button_open_str(void) { return strview_from_lit__(BUTTON_OPEN_STR); }
static inline StrView button_close_str(void) { return strview_from_lit__(BUTTON_CLOSE_STR); }

static inline StrView form_open_str(void) { return strview_from_lit__(FORM_OPEN_STR); }
static inline StrView form_close_str(void) { return strview_from_lit__(FORM_CLOSE_STR); }

static inline StrView elem_id_sep_default(void) { return strview_from_lit__(ELEM_ID_SEP); }
static inline StrView h_tag_open_str(void) { return strview_from_lit__("\n# "); }

static inline StrView newline_str(void) { return strview_from_lit__("\n"); }
static inline StrView space_str(void) { return strview_from_lit__(" "); }
#endif
