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

#ifndef TEST_SUITE
#warning "TEST_SUITE not defined"
#define TEST_SUITE(x)
#endif

#define UNIT_TEST(identifier) static void identifier(void)
#define DECL_UNIT_TEST(identifier)  UNIT_TEST(identifier);
#define COUNT_TESTS(...) + 1
#define NUM_TESTS (0 TEST_SUITE(COUNT_TESTS))

#ifndef eprintf
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

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
#define PRINT_ASSERT_REPORT(assertion) do {             \
    ut_error("Assertion failed\n"                       \
            "        File: %s\n"                        \
            "        Line: %d\n"                        \
            "        Test: %s\n"                        \
            "   Assertion: %s\n",                       \
            __FILE__, __LINE__, __func__, assertion);   \
} while (0)                                 

#define BASIC_ASSERT(assertion, a, b) do {               \
    if (!(assertion)) {                                  \
        PRINT_ASSERT_REPORT(#assertion);                 \
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
                                                                          \
    for (size_t i = 0; i < n; ++i) {                                      \
        if ((a)[i] != (b)[i]) {                                           \
            PRINT_ASSERT_REPORT("Equal array elements over 0.."#n);       \
            ut_error("              where "#n" = %zu\n", (size_t)(n));    \
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

#if defined(COMPILE_WITH_UNIT_TESTS)

TEST_SUITE(DECL_UNIT_TEST)

#define RUN_TEST(identifier)                            \
        ut_info("Unit test: " #identifier "\n");        \
        identifier();

int main()
{
    TEST_SUITE(RUN_TEST)
    return 0;
}

#endif /* defined(COMPILE_WITH_UNIT_TESTS) */

#endif /* UNIT_TESTS_H */
