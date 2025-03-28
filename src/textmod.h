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
typedef struct { size_t offset; ArlOf(TextMod) mods; } ModsAt;

#define T ModsAt
static inline void mods_at_clean(ModsAt* ma) { arlfn(TextMod,clean)(&ma->mods); }
//static inline int mods_at_copy(ModsAt* dest, const ModsAt* src, size_t n) {
//    //memmove(dest, src, n); return 0;
//    *dest = (ModsAt){.offset=src->offset};
//
//}
#define TClean mods_at_clean
#include <arl.h>
typedef ArlOf(ModsAt) TextBufMods;

static inline ModsAt*
mods_at_find_greater_or_eq(TextBufMods* mods, ModsAt it[static 1], size_t offset) {
    //TODO: use binsearch
    for (; it != arlfn(ModsAt,end)(mods) ; ++it) {
        if (offset >= it->offset) return it;
    }
    return NULL;
}

static inline Err mod_append(TextBufMods* mods, size_t offset, TextMod m) {
    ModsAt* back = arlfn(ModsAt, back)(mods);
    if (back && back->offset == offset) {
        if (!arlfn(TextMod, append)(&back->mods, &m)) return "error: arl failure";
    } else {
        ModsAt* mat = arlfn(ModsAt, append)(mods, &(ModsAt){.offset=offset});
        if (!mat) return "error: arl failure";
        if (!arlfn(TextMod, append)(&mat->mods, &m)) return "error: arl failure";
    }
    return Ok;
}

static inline TextMod esc_code_to_text_mod(EscCode code) { return (TextMod)code; }

#endif

