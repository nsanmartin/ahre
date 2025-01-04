#ifndef __HASHI_UTESTS_H__
#define __HASHI_UTESTS_H__

#include <string.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

enum { TestOk = 0, TestFail = 1 };

#define utest_assert(Expr, Tag, Linenum) do{ \
    if(!(Expr)) { fprintf(stderr, "Assertion in line %d failed. ", Linenum); goto Tag;} }while(0)

#define clean_and_ret(Status, Tag, Cleanup) do{\
    Status = 0; \
Tag: \
    Cleanup; \
    return Status; \
} while (0)

#define print_running_test(TestFname) do{\
    printf("Running %s...", TestFname); }while(0)

#define print_test_result(ErrorCount) do{\
    if (ErrorCount) fprintf(stderr, " %s%d errors%s\n",  RED, ErrorCount, RESET); \
    else printf(" %stests passed%s\n", GREEN, RESET); \
}while(0)

#endif
