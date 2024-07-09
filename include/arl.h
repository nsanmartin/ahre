// functions

#ifndef __HASHI_G_Functions_H__
#define __HASHI_G_Functions_H__

#include <string.h>

#define CAT_LITERAL(A, B) A##_##B
#define CAT_IND(A, B) CAT_LITERAL(A,B)

enum { ArlDefaultInitialCapacity = 2 };
#define ArlOf(T) CAT_IND(arl, T)
#define ArlFn(T, Name) CAT_IND(CAT_IND(arl, T), Name)

// static inline int
// arl_compar_values(void* item, void* elem, size_t itsz) {
//     return strncmp(item, elem, itsz);
// }
// 
// static inline void
// arl_copy_values(void* item, void* elem, size_t itsz) {
//     //memmove(a->items + a->len++ * a->item_sz, ptr, a->item_sz);
//     memmove(item, elem, itsz);
// }
#endif

//
// Arl
//

#ifndef T
#error "Template type T undefined"
#endif

#ifndef TCmp
#error "Template fn TCmp undefined"
#endif

#ifndef TCpy
#error "Template fn TCpy undefined"
#endif


typedef struct {
    T* items;
    size_t len;
    size_t capacity;
} ArlOf(T);

static inline int
ArlFn(T, realloc)(ArlOf(T)* a) {
    a->capacity = a->capacity ? 2 * a->capacity : ArlDefaultInitialCapacity ;
    a->items = realloc(a->items, a->capacity * sizeof(T)); 
    return a->items == 0;
} 

static inline T*
ArlFn(T, at)(ArlOf(T)* a, size_t ix) {
    return ix < a->len ? &a->items[ix] : NULL;
}

static inline int
ArlFn(T, append)(ArlOf(T)* a, T* ptr) {
    if (a->len >= a->capacity) {
        if (ArlFn(T, realloc)(a)) { /*error in realloc*/ return  1; }
    }
    memmove(a->items + a->len++, ptr, sizeof(T));
    return 0;
}

static inline T*
ArlFn(T, find) (ArlOf(T)* a, T* x) {
    for (size_t i = 0; i < a->len; ++i) {
        T* addr = &a->items[i];
        if (TCmp(addr, x) == 0) { return addr; }
    }
    return NULL;
}

#undef T
#undef TCmp
#undef TCpy

