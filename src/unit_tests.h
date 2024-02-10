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

#ifndef COMPILE_WITH_UNIT_TESTS
#error                                                                         \
    "unit_tests.h included but COMPILE_WITH_UNIT_TESTS not defined. Include guards recommended."
#endif

#ifdef UNIT_TESTS_H
#error "unit_tests.h included twice."
#endif

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

#define UNIT_TEST(identifier) __attribute__((used)) static void identifier(void)

#define DECL_UNIT_TEST(identifier) UNIT_TEST(identifier);

#define COUNT_TESTS(...) +1
#define NUM_TESTS        (0 TEST_SUITE(COUNT_TESTS))

#ifndef eprintf
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#ifndef XSTR
#define XSTR(x) STRINGIFY(x)
#endif

typedef void (*unit_test_t)(void);

static void ut_vprintf(const char *color, const char *fmt, va_list va)
{
    static int add_colors = -1;

    if (add_colors == -1) {
        // add_colors = isatty(STDERR_FILENO);
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

#define GET_PRI_SPECIFIER(x)                                                   \
    _Generic((x),              \
    _Bool: "%d",                                        \
    void *: "%p",                                       \
    const void *: "%p",                                 \
    char *: "\"%s\"",                                   \
    const char *: "\"%s\"",                             \
    char: "%c",                                         \
    signed char: "%hhd",                                \
    unsigned char: "0x%hhx",                            \
    short: "%hd",                                       \
    unsigned short: "%hu",                              \
    int: "%d",                                          \
    unsigned: "%u",                                     \
    float: "%f",             \
    double: "%f",\
    long: "%ld",                                        \
    unsigned long: "%lu",                               \
    long long: "%lld",                                  \
    unsigned long long: "%llu"                          \
)

/* Assert functions. */
static void print_assert_fail_header(const char *file, int line,
                                     const char *func)
{
    ut_error("ASSERTION FAILED\n");
    ut_info("File: %s\n"
            "Line: %d\n"
            "Function: %s\n"
            "\n"
            "Assertion:\n\t",
            file, line, func);
}
#define PRINT_ASSERT_FAIL_HEADER()                                             \
    print_assert_fail_header(__FILE__, __LINE__, __func__)

#define BASIC_ASSERT(aOp, bOp, test, aEval, bEval)                             \
    do {                                                                       \
        /* Evaluate only once */                                               \
        typeof(aEval) aa = aEval;                                              \
        typeof(bEval) bb = bEval;                                              \
        if (!(test(aa, bb))) {                                                 \
            PRINT_ASSERT_FAIL_HEADER();                                        \
            eprintf("%s\n", XSTR(test(aOp, bOp)));                             \
            ut_info("Left operand value:\n\t");                                \
            eprintf(GET_PRI_SPECIFIER(aa), aa);                                \
            ut_info("\nRight operand value:\n\t");                             \
            eprintf(GET_PRI_SPECIFIER(bb), bb);                                \
            eprintf("\n\n");                                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define TEST_EQUALS(a, b)             a == b
#define TEST_NOT_EQUALS(a, b)         a != b
#define TEST_LESS_THAN(a, b)          a < b
#define TEST_LESS_THAN_OR_EQ(a, b)    a <= b
#define TEST_GREATER_THAN(a, b)       a > b
#define TEST_GREATER_THAN_OR_EQ(a, b) a >= b

#define ASSERT_EQ(a, b) BASIC_ASSERT(a, b, TEST_EQUALS, a, b)
#define ASSERT_NE(a, b) BASIC_ASSERT(a, b, TEST_NOT_EQUALS, a, b)
#define ASSERT_LT(a, b) BASIC_ASSERT(a, b, TEST_LESS_THAN, a, b)
#define ASSERT_LE(a, b) BASIC_ASSERT(a, b, TEST_LESS_THAN_OR_EQ, a, b)
#define ASSERT_GT(a, b) BASIC_ASSERT(a, b, TEST_GREATER_THAN, a, b)
#define ASSERT_GE(a, b) BASIC_ASSERT(a, b, TEST_GREATER_THAN_OR_EQ, a, b)

#define ASSERT_TRUE(a)  BASIC_ASSERT(a, 0, TEST_NOT_EQUALS, (_Bool)(a), 0)
#define ASSERT_FALSE(a) BASIC_ASSERT(a, 0, TEST_EQUALS, (_Bool)(a), 0)

#define ASSERT_EQ_PTR(a, b)                                                    \
    BASIC_ASSERT(a, b, TEST_EQUALS, (void *)(a), (void *)(b))
#define ASSERT_NE_PTR(a, b)                                                    \
    BASIC_ASSERT(a, b, TEST_NOT_EQUALS, (void *)(a), (void *)(b))
#define ASSERT_NOT_NULL(a) ASSERT_NE_PTR(a, NULL)

#define ASSERT_EQ_ARR(a, b, n)                                                 \
    do {                                                                       \
        int check_equal_element_sizes[sizeof(*(a)) != sizeof(*(b)) ? -1 : 1];  \
        (void)check_equal_element_sizes;                                       \
                                                                               \
        for (size_t i = 0; i < n; ++i) {                                       \
            if ((a)[i] != (b)[i]) {                                            \
                PRINT_ASSERT_FAIL_HEADER();                                    \
                ut_info("Equal array elements over 0..%zu\n", (size_t)(n));    \
                ut_info("Discrepancy at index %zu:\n", i);                     \
                ut_info("      array1 = " #a "\n");                            \
                ut_info("      array2 = " #b "\n");                            \
                ut_info("      array1[%zu] = ", i);                            \
                ut_info(GET_PRI_SPECIFIER(*(a)), (a)[i]);                      \
                ut_info("\n");                                                 \
                ut_info("      array2[%zu] = ", i);                            \
                ut_info(GET_PRI_SPECIFIER(*(b)), (b)[i]);                      \
                ut_info("\n");                                                 \
                ut_info("\n");                                                 \
                ut_info("\n");                                                 \
                exit(1);                                                       \
            }                                                                  \
        }                                                                      \
    } while (0)

__attribute__((used)) static void assert_eq_mem_impl(
    const char *file, int line, const char *func, const char *aname,
    const char *bname, const void *a, size_t asize, const void *b, size_t bsize)
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

    print_assert_fail_header(file, line, func);
    eprintf("Matching memory areas\n");

    ut_info("Left operand:\n");
    eprintf("\t%s\n", aname);
    ut_info("Right operand:\n");
    eprintf("\t%s\n", bname);

    ut_info("Error:\n");
    if (diff_index != smallest) {
        eprintf("\t- Byte value mismatch at index %zu\n", diff_index);
    }
    if (asize != bsize) {
        eprintf("\t- Length mismatch: %zu and %zu bytes\n", asize, bsize);
    }

    if (diff_index != smallest) {
        ut_info("Hex dump starting at first nonmatching bytes:\n");
        eprintf("          %-26s %s\n", aname, bname);

        enum {
            COLS_PER_LINE = 16,
            COLS_PER_ITEM = COLS_PER_LINE / 2,
            MAX_BYTES_TO_DUMP = 256
        };

        for (size_t i = diff_index; i < smallest; i += COLS_PER_ITEM) {
            eprintf("%08zx:", i);

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
                eprintf("%9s stopping due to limit\n", "");
                break;
            }
        }
    }

    exit(1);
}

#define ASSERT_EQ_MEM(a, asize, b, bsize)                                      \
    assert_eq_mem_impl(__FILE__, __LINE__, __func__, #a, #b, a, asize, b, bsize)

TEST_SUITE(DECL_UNIT_TEST)

static void start_unit_test(const char *test_name, unit_test_t test)
{
    ut_info("====================[%s]====================\n", test_name);
    test();
    fflush(NULL);
}

#define START_UNIT_TEST(test) start_unit_test(#test, test);

int main()
{
    TEST_SUITE(START_UNIT_TEST)
    return 0;
}
