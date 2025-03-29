#ifndef __AHRE_TEXTMOD_H__
#define __AHRE_TEXTMOD_H__

#include "escape_codes.h"

typedef enum {
    text_mod_blue,
    text_mod_green,
    text_mod_red,
    text_mod_yellow,
    text_mod_purple,
    text_mod_light_green,
    text_mod_bold,
    text_mod_italic,
    text_mod_underline,
    text_mod_reset,
    text_mod_last_entry
} TextMod;

#define T TextMod
#include <arl.h>
typedef struct { size_t offset; TextMod tmod; } ModsAt;

#define T ModsAt
//static inline void mods_at_clean(ModsAt* ma) { arlfn(TextMod,clean)(&ma->mods); }
//#define TClean mods_at_clean
#include <arl.h>
typedef ArlOf(ModsAt) TextBufMods;

static inline ModsAt*
mods_at_find_greater_or_eq(TextBufMods* mods, ModsAt it[static 1], size_t offset) {
    //TODO: use binsearch
    for (; it < arlfn(ModsAt,end)(mods) ; ++it) {
        if (offset <= it->offset) return it;
    }
    return arlfn(ModsAt,end)(mods);
}

static inline Err textmod_append(TextBufMods* mods, size_t offset, TextMod m) {
    if (!arlfn(ModsAt, append)(mods, &(ModsAt){.offset=offset,.tmod=m})) return "error: arl failure";
    return Ok;
}

static inline TextMod esc_code_to_text_mod(EscCode code) { return (TextMod)code; }
static inline EscCode textmod_to_esc_code(TextMod tm) { return (EscCode)tm; }

static inline Err
textmod_concatenate(TextBufMods base[static 1], size_t offset, TextBufMods consumed[static 1]) {
    for (ModsAt* it = arlfn(ModsAt, begin)(consumed)
        ; it != arlfn(ModsAt, end)(consumed)
        ; ++it
    ) {
        ModsAt displaced = (ModsAt){.offset=it->offset + offset, .tmod=it->tmod};
        ModsAt* appended = arlfn(ModsAt,append)(base, &displaced);
        if (!appended) return "error: arl failure"; //TODO: clean space on error

    }
    return Ok;
}

#endif
