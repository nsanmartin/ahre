#ifndef __MEM_AHRE_H__
#define __MEM_AHRE_H__

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#include <stdlib.h>
#include <limits.h>
#include <string.h>


#define std_strdup  strdup
#define std_free    free
#define std_malloc  malloc
#define std_calloc  calloc

#ifdef AHRE_SIMULATE_REALLOC_ERROR
void* realloc_simulate_error(void* ptr, size_t n);
#define std_realloc realloc_simulate_error
#else
#define std_realloc realloc
#endif

#endif
