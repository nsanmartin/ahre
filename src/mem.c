#include "mem.h"


#ifdef AHRE_SIMULATE_REALLOC_ERROR
void* realloc_simulate_error(void* ptr, size_t n) {
    static size_t counter = 0;
    if (counter++ % 5 == 0) return NULL;
    return realloc(ptr, n);
}
#endif
