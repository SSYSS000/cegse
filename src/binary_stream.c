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

#include <stdlib.h>

#include "binary_stream.h"
#include "defines.h"
#include "endianness.h"

#define FOR_PRIMITIVE_TYPES(ITEM)                       \
    ITEM(u8, uint8_t, 1)                                \
    ITEM(le16, uint16_t, 2)                             \
    ITEM(le32, uint32_t, 4)                             \
    ITEM(le64, uint64_t, 8)                             \
    ITEM(lef32, float,   4)

#define FOR_ALL_TYPES(ITEM)                             \
    FOR_PRIMITIVE_TYPES(ITEM)                           \
    ITEM(be24, uint32_t, 3)                             \

#define DEFINE_POINTER_TO_BUF_API(id, type, size)       \
void store_##id(unsigned char *dest, type value)        \
{                                                       \
    value = hto##id(value);                             \
    memcpy(dest, &value, size);                         \
}                                                       \
                                                        \
type load_##id(const unsigned char *src)                \
{                                                       \
    type value;                                         \
    memcpy(&value, src, size);                          \
    return id##toh(value);                              \
}

void store_be24(unsigned char *dest, uint32_t value)
{
    dest[0] = (value >> 16) & 0xff;
    dest[1] = (value >> 8) & 0xff;
    dest[2] = value & 0xff;
}

uint32_t load_be24(const unsigned char *src)
{
    return (uint32_t) src[0] << 16 |
           (uint32_t) src[1] << 8  |
           (uint32_t) src[2];
}

void c_store_bytes(struct cursor *cursor, const void *bytes, size_t n)
{
    cursor->n -= n;
    if (cursor->n >= 0) {
        memcpy(cursor->pos, bytes, n);
        cursor->pos += n;
    }
}

int c_load_bytes(struct cursor *cursor, void *dest, size_t n)
{
    cursor->n -= n;
    if (cursor->n >= 0) {
        memcpy(dest, cursor->pos, n);
        cursor->pos += n;
        return 1;
    }
    return 0;
}

void c_advance2(struct cursor *to, const struct cursor *from, long long n)
{
    to->pos = from->pos + n;
    to->n = from->n - n;
}

#define DEFINE_CURSOR_API(id, type, size)               \
void c_store_##id(struct cursor *c, type value)         \
{                                                       \
    c->n -= size;                                       \
    if (c->n >= 0) {                                    \
        store_##id(c->pos, value);                      \
        c->pos += size;                                 \
    }                                                   \
}                                                       \
                                                        \
int c_load_##id(struct cursor *c, type *dest)           \
{                                                       \
    c->n -= size;                                       \
    if (c->n >= 0) {                                    \
        *dest = load_##id(c->pos);                      \
        c->pos += size;                                 \
        return 1;                                       \
    }                                                   \
    return 0;                                           \
}                                                       \
                                                        \
type c_load_##id##_or0(struct cursor *c)                \
{                                                       \
    type value;                                         \
    return c_load_##id(c, &value) ? value : 0;          \
}

#define DEFINE_FILE_API(id, type, size)                 \
size_t put_##id(FILE *stream, type value)               \
{                                                       \
    unsigned char bytes[size];                          \
    store_##id(bytes, value);                           \
    return fwrite(bytes, size, 1, stream);              \
}                                                       \
                                                        \
int get_##id(FILE *restrict stream, type *restrict dest)\
{                                                       \
    unsigned char bytes[size];                          \
    if (fread(bytes, size, 1, stream)) {                \
        *dest = load_##id(bytes);                       \
        return 1;                                       \
    }                                                   \
    else {                                              \
        return 0;                                       \
    }                                                   \
}                                                       \
                                                        \
type get_##id##_or0(FILE *stream)                       \
{                                                       \
    type value;                                         \
    if (get_##id(stream, &value)) {                     \
        return value;                                   \
    }                                                   \
    else {                                              \
        return 0;                                       \
    }                                                   \
}

/* 
 * These defines are just by-product of reusing lists and function
 * definition templates for types that don't need byte swapping.
 */
#define htolef32(x) x
#define lef32toh(x) x
#define u8toh(x) x
#define htou8(x) x
FOR_PRIMITIVE_TYPES(DEFINE_POINTER_TO_BUF_API)
FOR_ALL_TYPES(DEFINE_CURSOR_API)
FOR_ALL_TYPES(DEFINE_FILE_API)

#define TEST_SUITE(TEST_CASE)                           \
    TEST_CASE(store_load_functions)                     \
    TEST_CASE(cursor_advance)

#include "unit_tests.h"

UNIT_TEST(store_load_functions)
{
    unsigned char buffer[1024];

    {
        const char correct_repr[] = {0x12, 0x34, 0x56};
        store_be24(buffer, 0x123456);
        ASSERT_EQ_ARR(buffer, correct_repr, 3);
        ASSERT_EQ(load_be24(buffer), 0x123456);
    }

    {
        const char correct_repr[] = {0x00, 0x00, 0x00, 0x40};
        store_lef32(buffer, 2.0f);
        ASSERT_EQ_ARR(buffer, correct_repr, 4);
        ASSERT_EQ(load_lef32(buffer), 2.0f);
    }
}

UNIT_TEST(cursor_advance)
{
    enum { BUFFER_SIZE = 1024 };
    unsigned char buffer[BUFFER_SIZE];

    struct cursor *cursor = (struct cursor[]) {{
        buffer, BUFFER_SIZE
    }};

    /* c_advance */
    c_advance(cursor, BUFFER_SIZE);
    ASSERT_EQ(cursor->n, 0);
    ASSERT_EQ_PTR(cursor->pos, buffer + BUFFER_SIZE);

    c_advance(cursor, -BUFFER_SIZE);
    ASSERT_EQ(cursor->n, BUFFER_SIZE);
    ASSERT_EQ_PTR(cursor->pos, buffer);

    c_advance(cursor, BUFFER_SIZE + 1);
    ASSERT_EQ(cursor->n, -1);

    c_advance(cursor, 4);
    ASSERT_EQ(cursor->n, -5);

    c_advance(cursor, -(BUFFER_SIZE + 5));

    /* c_advance2 */
    struct cursor cursor2_buf;
    c_advance2(&cursor2_buf, cursor, 4);
    ASSERT_EQ(cursor->n, BUFFER_SIZE);
    ASSERT_EQ_PTR(cursor->pos, buffer);
    ASSERT_EQ(cursor2_buf.n, BUFFER_SIZE - 4);
    ASSERT_EQ_PTR(cursor2_buf.pos, buffer + 4);
}
