/*
Copyright (C) 2024  SSYSS000

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef TEST_SUITE
#warning "TEST_SUITE not defined"
#define TEST_SUITE(x)
#endif

#define UNIT_TEST(identifier)   \
    __attribute__((used)) static void identifier(void)

#define DECL_UNIT_TEST(identifier)  \
    UNIT_TEST(identifier);

#define COUNT_TESTS(...) + 1
#define NUM_TESTS (0 TEST_SUITE(COUNT_TESTS))

#ifndef eprintf
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

typedef void (*unit_test_t)(void);

static void ut_vprintf(const char *color, const char *fmt, va_list va)
{
    static int add_colors = -1;

    if (add_colors == -1) {
        //add_colors = isatty(STDERR_FILENO);
        add_colors = 1;
    }

    if (add_colors) {
        fputs(color, stderr);
    }

    vfprintf(stderr, fmt, va);

    if (add_colors) {
        fputs("\e[0m", stderr);
    }
}

static void ut_info(const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    ut_vprintf("\e[0;93m", fmt, va);
    va_end(va);
}

static void ut_error(const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    ut_vprintf("\e[1;31m", fmt, va);
    va_end(va);
}

#define GET_PRI_SPECIFIER(x) _Generic((x),              \
    void *: "%p",                                       \
    const void *: "%p",                                 \
    char *: "%s",                                       \
    const char *: "%s",                                 \
    char: "%c",                                         \
    signed char: "%hhd",                                \
    unsigned char: "%hhu",                              \
    short: "%hd",                                       \
    unsigned short: "%hu",                              \
    int: "%d",                                          \
    unsigned: "%u",                                     \
    float: "%f",                                        \
    double: "%f",                                       \
    long: "%ld",                                        \
    unsigned long: "%lu",                               \
    long long: "%lld",                                  \
    unsigned long long: "%llu"                          \
)

/* Assert functions. */
#define PRINT_ASSERT_FAIL_HEADER_INNER(file, line, func, assertion, ...) do { \
    ut_error("Assertion failed\n"                       \
            "        File: %s\n"                        \
            "        Line: %d\n"                        \
            "        Test: %s\n"                        \
            "   Assertion: ",                           \
            file, line, func);                          \
    ut_error(assertion, ##__VA_ARGS__);                 \
    fputc('\n', stderr);                                \
} while (0)

#define PRINT_ASSERT_FAIL_HEADER(assertion, ...)        \
    PRINT_ASSERT_FAIL_HEADER_INNER(__FILE__, __LINE__,  \
            __func__, assertion, ##__VA_ARGS__)

#define BASIC_ASSERT(assertion, a, b) do {               \
    if (!(assertion)) {                                  \
        PRINT_ASSERT_FAIL_HEADER(#assertion);            \
        ut_error("        Left: ");                      \
        ut_error(GET_PRI_SPECIFIER(a), a);               \
        ut_error("\n");                                  \
        ut_error("       Right: ");                      \
        ut_error(GET_PRI_SPECIFIER(b), b);               \
        ut_error("\n");                                  \
        ut_error("\n");                                  \
        ut_error("\n");                                  \
        exit(1);                                         \
    }                                                    \
} while (0)

#define ASSERT_EQ(a, b) BASIC_ASSERT(a == b, a, b)
#define ASSERT_NE(a, b) BASIC_ASSERT(a != b, a, b)
#define ASSERT_LT(a, b) BASIC_ASSERT(a < b, a, b)
#define ASSERT_LE(a, b) BASIC_ASSERT(a <= b, a, b)
#define ASSERT_GT(a, b) BASIC_ASSERT(a > b, a, b)
#define ASSERT_GE(a, b) BASIC_ASSERT(a >= b, a, b)
#define ASSERT_TRUE(a) BASIC_ASSERT((bool) a == true, a, true)
#define ASSERT_FALSE(a) BASIC_ASSERT((bool) a == false, a, false)
#define ASSERT_NOT_NULL(a) BASIC_ASSERT(a != NULL, (void *)a, NULL)

#define ASSERT_EQ_ARR(a, b, n) do {                                       \
    int check_equal_element_sizes[sizeof(*(a)) != sizeof(*(b)) ? -1 : 1]; \
    (void) check_equal_element_sizes;                                     \
                                                                          \
    for (size_t i = 0; i < n; ++i) {                                      \
        if ((a)[i] != (b)[i]) {                                           \
            PRINT_ASSERT_FAIL_HEADER("Equal array elements over 0..%zu", (size_t)(n)); \
            ut_error("Discrepancy at index %zu:\n", i);                   \
            ut_error("      array1 = "#a "\n");                           \
            ut_error("      array2 = "#b "\n");                           \
            ut_error("      array1[%zu] = ", i);                          \
            ut_error(GET_PRI_SPECIFIER(*(a)), (a)[i]);                    \
            ut_error("\n");                                               \
            ut_error("      array2[%zu] = ", i);                          \
            ut_error(GET_PRI_SPECIFIER(*(b)), (b)[i]);                    \
            ut_error("\n");                                               \
            ut_error("\n");                                               \
            ut_error("\n");                                               \
            exit(1);                                                      \
        }                                                                 \
    }                                                                     \
} while (0)

__attribute__((used))
static void assert_eq_mem_impl(
        const char *file, int line, const char *func,
        const char *aname, const char *bname,
        const void *a, size_t asize,
        const void *b, size_t bsize)
{
    size_t smallest = asize > bsize ? bsize : asize;
    const unsigned char *ca = a;
    const unsigned char *cb = b;
    size_t diff_index;

    /* Find index where memories differ. */
    for (diff_index = 0; diff_index < smallest; ++diff_index) {
        if (ca[diff_index] != cb[diff_index]) {
            break;
        }
    }

    if (diff_index == smallest && asize == bsize) {
        /* No difference. */
        return;
    }

    PRINT_ASSERT_FAIL_HEADER_INNER(file, line, func,
            "Equal memory (%s, %s)", aname, bname);

    if (asize != bsize) {
        ut_error("Memory areas are not the same size:\n"
                "%s: %zu bytes\n"
                "%s: %zu bytes\n",
                aname, asize, bname, bsize);
    }

    enum {
        COLS_PER_LINE = 16,
        COLS_PER_ITEM = COLS_PER_LINE / 2,
        MAX_BYTES_TO_DUMP = 256
    };

    ut_error("Partial hex dump of memory areas at difference:\n");
    ut_info("          %-26s %s\n", aname, bname);

    for (size_t i = diff_index; i < smallest; i += COLS_PER_ITEM) {
        ut_info("%08zx:", i);

        for (size_t j = i; j < (i + COLS_PER_ITEM); ++j) {
            int equal_byte = j < asize && j < bsize && ca[j] == cb[j];
            if (!equal_byte) {
                eprintf("\e[0;33m");
            }

            if (j < asize) {
                eprintf(" %02x", (unsigned)ca[j]);
            }
            else {
                fputs("     ", stderr);
            }

            eprintf("\e[0m");
        }

        fputs("   ", stderr);

        for (size_t j = i; j < (i + COLS_PER_ITEM); ++j) {
            int equal_byte = j < asize && j < bsize && ca[j] == cb[j];
            if (!equal_byte) {
                eprintf("\e[0;33m");
            }

            if (j < bsize) {
                eprintf(" %02x", (unsigned)cb[j]);
            }
            else {
                fputs("     ", stderr);
            }

            eprintf("\e[0m");
        }

        fputs("\n", stderr);

        if (i - diff_index > MAX_BYTES_TO_DUMP) {
            break;
        }
    }

    exit(1);
}

#define ASSERT_EQ_MEM(a, asize, b, bsize)\
    assert_eq_mem_impl(__FILE__, __LINE__, __func__, #a, #b, a, asize, b, bsize)

#if defined(COMPILE_WITH_UNIT_TESTS)

TEST_SUITE(DECL_UNIT_TEST)

static void start_unit_test(const char *test_name, unit_test_t test)
{
    ut_info("====================[%s]====================\n", test_name);
    test();
    fflush(NULL);
}

#define START_UNIT_TEST(test)  start_unit_test(#test, test);

int main()
{
    TEST_SUITE(START_UNIT_TEST)
    return 0;
}

#endif /* defined(COMPILE_WITH_UNIT_TESTS) */

#endif /* UNIT_TESTS_H */
