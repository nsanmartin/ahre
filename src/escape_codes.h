#ifndef AHRE_ESCAPE_CODES_H_
#define AHRE_ESCAPE_CODES_H_

#include "error.h"
#include "str.h"

#define EscCodeRed   "\033[91m"
#define EscCodeLightGreen  "\033[92m"
#define EscCodeBlue  "\033[94m"

#define EscCodeGreen  "\033[32m"
#define EscCodeReset "\033[0m"
#define EscCodeYellow  "\033[33m"
#define EscCodePurple  "\033[35m"


#define EscCodeBold "\033[1m"
#define EscCodeItalic "\033[3m"
#define EscCodeUnderline "\033[4m"

#define EscCodeClsScr "\033[1;1H\033[2J" 

typedef enum {
    esc_code_blue,
    esc_code_green,
    esc_code_red,
    esc_code_yellow,
    esc_code_purple,

    esc_code_light_green,

    esc_code_bold,
    esc_code_italic,
    esc_code_underline,

    esc_code_reset
} EscCode;

#define _set_ptr_(Ptr, Code) { *Ptr = (StrView){.items=Code, .len=sizeof(Code)-1}; break; }

static inline Err esc_code_to_str(EscCode code, StrView out[static 1]) {
    switch(code) {
        case esc_code_blue: _set_ptr_(out, EscCodeBlue);
        case esc_code_green: _set_ptr_(out, EscCodeGreen);
        case esc_code_red: _set_ptr_(out, EscCodeRed);
        case esc_code_yellow: _set_ptr_(out, EscCodeYellow);
        case esc_code_purple: _set_ptr_(out, EscCodePurple);
        case esc_code_light_green: _set_ptr_(out, EscCodeLightGreen);
        case esc_code_bold: _set_ptr_(out, EscCodeBold);
        case esc_code_italic: _set_ptr_(out, EscCodeItalic);
        case esc_code_underline: _set_ptr_(out, EscCodeUnderline);
        case esc_code_reset: _set_ptr_(out, EscCodeReset);
        default: return "error: invalid escape code";
    }
    return Ok;
}

#endif
