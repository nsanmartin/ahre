#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline int
array_list_compar_int(void* item, void* elem) {
    return strncmp(item, elem, sizeof(int));
}

static inline void
array_list_copy_int(void* item, void* elem) {
    //memmove(a->items + a->len++ * a->item_sz, ptr, a->item_sz);
    memcpy(item, elem, sizeof(int));
}

#define T int
#define TCmp array_list_compar_int
#define TCpy array_list_copy_int
#include <arl.h>

int test_0(void) {
    ArlOf(int) a = {0};
    int value = 3;
    ArlFn(int, append)(&a, &value);
    int* x = ArlFn(int, at)(&a, 0);
    if (!x || *x != value) { return 1; }

    value = 7;
    ArlFn(int, append)(&a, &value);
    x = ArlFn(int, at)(&a, 1);
    if (!x || *x != value) { return 1; }

    free(a.items);
    return 0;
}

int main(void) {
    int errors = test_0();
    if (errors) {
        fprintf(stderr, "%d errors\n", errors);
    } else {
        puts("Tests Passed");
    }
}
