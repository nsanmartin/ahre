#ifndef __HASHI_UTESTS_H__
#define __HASHI_UTESTS_H__

#include <string.h>
#include <stdio.h>

#include "../src/error.h"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

enum { TestOk = 0, TestFail = 1 };

#define utest_assert(Expr, Tag) do{ \
    if(!(Expr)) { fprintf(stderr, "assertion in line %d failed. ", __LINE__); goto Tag;} }while(0)

#define utest_try(Expr, Tag) do{ \
    if((Expr)) { fprintf(stderr, "failure executing line %d. ", __LINE__); goto Tag;} }while(0)

#define clean_and_ret(Status, Tag, Cleanup) do{\
    Status = 0; \
Tag: \
    Cleanup; \
    return Status; \
} while (0)

#define print_running_test(TestFname) do{\
    printf("Running %s... ", TestFname); fflush(stdout); }while(0)

#define print_test_result(ErrorCount) do{\
    if (ErrorCount) fprintf(stderr, " %s%d errors%s\n",  RED, ErrorCount, RESET); \
    else printf("%stests passed%s\n", GREEN, RESET); \
}while(0)

#define utest_ignore_params_0()
#define utest_ignore_params_1(p) (void)p;
#define utest_ignore_params_2(p, ...) (void)p; utest_ignore_params_1(__VA_ARGS__)
#define utest_ignore_params_3(p, ...) (void)p; utest_ignore_params_2(__VA_ARGS__)
#define utest_ignore_params_4(p, ...) (void)p; utest_ignore_params_3(__VA_ARGS__)
#define utest_ignore_params(...) utest_ignore_params_(__VA_ARGS__, 4, 3, 2, 1, 0)(__VA_ARGS__)
#define utest_ignore_params_(_1, _2, _3, _4, N, ...) utest_ignore_params_##N


static inline size_t mock_fwrite_ok(const void* ptr, size_t size, size_t nmemb, FILE * stream) {
    utest_ignore_params(ptr, stream);
    return size * nmemb;
}

static inline FILE* mock_fopen(const char *restrict pathname, const char *restrict mode) {
    utest_ignore_params(pathname, mode);
    return (FILE*)-1;
}

static inline int mock_fclose(FILE *stream) { (void)stream; return 0; }

static inline Err mock_file_open(const char* filename, const char* mode, FILE* fpp[static 1]) {
    (void)filename; (void)mode; (void)fpp; return Ok;
}

static inline Err mock_file_write_(const char* mem, size_t len, FILE* fp) {
    (void)mem; (void) len; (void) fp;  return Ok;
} 
static inline Err mock_file_close(FILE* fp) { (void)fp; return Ok; }

#endif
