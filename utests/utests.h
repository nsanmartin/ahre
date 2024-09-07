#ifndef __HASHI_UTESTS_H__
#define __HASHI_UTESTS_H__

#include <string.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

enum { TestOk = 0, TestFail = 1 };

#define utest_assert(Expr, Tag) do{ if(!(Expr)) { goto Tag;} }while(0)

#define clean_and_ret(Status, Tag, Cleanup) do{\
    Status = 0; \
Tag: \
    Cleanup; \
    return Status; \
} while (0)

//#define utest_assert_clean(Expr)  
//    if (!(Expr)) { fprintf(stderr, RED "Test failed: %s" RESET "n", __func__); goto fail_cleanup; }
//
//#define utest_assert_str_eq(Expected, String) 
//    utest_assert(strcmp(Expected, String) == 0)
//
//#define utest_assert_str_eq_clean(Expected, String) 
//    utest_assert_clean(strcmp(Expected, String) == 0)
//
//#define utest_finally_and_return(Clean) do { 
//	Clean; return TestOk; 
//fail_cleanup: 
//	Clean; return TestFail; 
//} while(0)

#endif
