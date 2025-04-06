#include <stdio.h>

#include <utests.h>
#include "../src/textbuf.c"
#include "../src/error.c"



int test_0(void) {
#define str0 "01\n02\n03"
    size_t len = lit_len__(str0);

    size_t count = mem_count_ocurrencies(str0, len, '\n');
    utest_assert(count == 2, fail);

    TextBuf buf;
    textbuf_init(&buf);
    utest_try( textbuf_append_part(&buf, str0, len), fail);
    utest_try( textbuf_append_line_indexes(&buf), fail);
    utest_assert(count == buf.eols.len, fail);

    textbuf_cleanup(&buf);
    return 0;
fail:
    textbuf_cleanup(&buf);
    return 1;
}

static int _check_fit_lines_(
    char* str, size_t len, size_t ncols, size_t count_before, size_t count_after
) {

    TextBuf tb;
    textbuf_init(&tb);
    utest_try( textbuf_append_part(&tb, str, len), fail);
    utest_assert(count_before == mem_count_ocurrencies(str, len, '\n'), fail);
    utest_try( textbuf_fit_lines(&tb, ncols), fail);

    Str* buf = textbuf_buf(&tb);
    utest_assert(buf, fail);
    utest_assert(
        count_after == mem_count_ocurrencies(items__(buf), len__(buf), '\n'),
        fail);

    textbuf_cleanup(&tb);
    return 0;
fail:
    textbuf_cleanup(&tb);
    return 1;
}

#define check_fit_lines__(LitStr, Ncols, CountBefore, CountAfter) \
    _check_fit_lines_(LitStr, lit_len__(LitStr), Ncols, CountBefore, CountAfter)

int test_1_fit_lines(void) {
#define t1str0 "0123456789\n"
#define t1str1 "012340 5678901234\n"
#define t1str2 "0123456789 0123456789\n"
#define t1str3 "[0.The First Link ]| [1.The Second One]| [2.Third one] Some text after the links\n"
    utest_try (check_fit_lines__(t1str0, 6, 1, 2), fail);
    utest_try (check_fit_lines__(t1str1, 10, 1, 2), fail);
    utest_try (check_fit_lines__(t1str2, 10, 1, 2), fail);
    utest_try (check_fit_lines__(t1str3, 10, 1, 9), fail);
    return 0;
fail: return 1;
}

int modat_arr_eq(ArlOf(ModAt)* x, ArlOf(ModAt)* y) {
    if (len__(x) != len__(y)) return false;
    for (size_t i = 0; i < len__(x); ++i) {
        ModAt* p = arlfn(ModAt,at)(x, i);
        ModAt* q = arlfn(ModAt,at)(y, i);
        if (!p || !q) return false;
        if (p->tmod != q->tmod) return false;
        if (p->offset != q->offset) return false;
    }
    return true;
}


int test_2_fit_lines_mods(void) {
#define t2str0 " [0.Historial web]| [1.Configuracion]| [2.Acceder]"
    size_t len = lit_len__(t2str0);
    ModAt modarr[]  = {
        {.offset = 1,  .tmod = text_mod_blue},
        {.offset = 18, .tmod = text_mod_reset},
        {.offset = 20, .tmod = text_mod_blue},
        {.offset = 37, .tmod = text_mod_reset},
        {.offset = 39, .tmod = text_mod_blue},
        {.offset = 50, .tmod = text_mod_reset}
    };

    ArlOf(ModAt) mods = (ArlOf(ModAt)) { .items=modarr, .len=sizeof(modarr)/sizeof(*modarr) };
    TextBuf tb;
    textbuf_init(&tb);
    utest_try( textbuf_append_part(&tb, t2str0, len), fail);
    tb.mods = mods;
    size_t ncols = 10;
    utest_try( textbuf_fit_lines(&tb, ncols), fail);

    ModAt expected[]  = {
        {.offset = 1,      .tmod = text_mod_blue},
        {.offset = 18 + 1, .tmod = text_mod_reset},
        {.offset = 20 + 1, .tmod = text_mod_blue},
        {.offset = 37 + 2, .tmod = text_mod_reset},
        {.offset = 39 + 2, .tmod = text_mod_blue},
        {.offset = 50 + 3, .tmod = text_mod_reset}
    };
    utest_assert(
        modat_arr_eq(
            &mods,
            &(ArlOf(ModAt)){.items=expected, len=sizeof(expected)/sizeof(*expected)}
        ),
        fail
    );
    tb.mods = (ArlOf(ModAt)){0};
    textbuf_cleanup(&tb);
    return 0;
fail:
    tb.mods = (ArlOf(ModAt)){0};
    textbuf_cleanup(&tb);
    return 1;
}

int main(void) {
    print_running_test(__FILE__);
    int errors
        = test_0()
        + test_1_fit_lines()
        + test_2_fit_lines_mods()
        ;
    print_test_result(errors);
}
