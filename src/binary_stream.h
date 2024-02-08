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
#ifndef CEGSE_BINARY_STREAM_H
#define CEGSE_BINARY_STREAM_H

#include <stdio.h>
#include <stdint.h>

struct cursor {
    unsigned char *pos;

    /* 
     * When n > 0, n denotes the distance to the end from the current
     * position pos.
     * 
     * When n == 0, the position points to the end.
     *
     * When n < 0, the cursor is out of bounds and n denotes how many bytes
     * would have been read/written past the end but as a negative number.
     * Position may not point at the end. This state will be reached when trying
     * to read/write more bytes than available at some position. This value
     * is useful in error checking and tells for example by how
     * much to grow a buffer.
     */
    long long n;
};

/* Pointer to buffer API */
void store_u8(unsigned char *dest, uint8_t value);
void store_le16(unsigned char *dest, uint16_t value);
void store_be24(unsigned char *dest, uint32_t value);
void store_le32(unsigned char *dest, uint32_t value);
void store_le64(unsigned char *dest, uint64_t value);
void store_lef32(unsigned char *dest, float value);

uint8_t load_u8(const unsigned char *src);
uint16_t load_le16(const unsigned char *src);
uint32_t load_be24(const unsigned char *src);
uint32_t load_le32(const unsigned char *src);
uint64_t load_le64(const unsigned char *src);
float load_lef32(const unsigned char *src);

/* Cursor API */
/*
 * Advance this cursor by n bytes. Equivalent to c_advance2(c, c, n).
 */
static inline void c_advance(struct cursor *c, long long n)
{
    c->pos += n;
    c->n -= n;
}

/*
 * Advance 'from' cursor by n bytes, storing the result to the 'to' cursor.
 */
void c_advance2(struct cursor *to, const struct cursor *from, long long n);

/*
 * These functions store a value at the current position and advance the
 * position. The operation fails if cursor goes out of bounds.
 */
void c_store_bytes(struct cursor *cursor, const void *bytes, size_t n);
void c_store_u8(struct cursor *c, uint8_t value);
void c_store_le16(struct cursor *c, uint16_t value);
void c_store_be24(struct cursor *c, uint32_t value);
void c_store_le32(struct cursor *c, uint32_t value);
void c_store_le64(struct cursor *c, uint64_t value);
void c_store_lef32(struct cursor *c, float value);

/* 
 * These functions load a value from the current position to dest.
 * On success, return nonzero value.
 */
int c_load_bytes(struct cursor *cursor, void *dest, size_t n);
int c_load_u8(struct cursor *cursor, uint8_t *dest);
int c_load_le16(struct cursor *cursor, uint16_t *dest);
int c_load_be24(struct cursor *cursor, uint32_t *dest);
int c_load_le32(struct cursor *cursor, uint32_t *dest);
int c_load_le64(struct cursor *cursor, uint64_t *dest);
int c_load_lef32(struct cursor *cursor, float *dest);

/*
 * These functions load a value from the current position and return it.
 * On out of bounds error, return 0.
 */
uint8_t c_load_u8_or0(struct cursor *cursor);
uint16_t c_load_le16_or0(struct cursor *cursor);
uint32_t c_load_be24_or0(struct cursor *cursor);
uint32_t c_load_le32_or0(struct cursor *cursor);
uint64_t c_load_le64_or0(struct cursor *cursor);
float c_load_lef32_or0(struct cursor *cursor);

static inline size_t
write_bytes(FILE *restrict stream, const void *restrict bytes, size_t n)
{
    return fwrite(bytes, 1, n, stream);
}

static inline size_t
read_bytes(FILE *restrict stream, void *restrict bytes, size_t n)
{
    return fread(bytes, 1, n, stream);
}

size_t
write_le16_array(FILE *restrict stream, uint16_t *restrict array, size_t n);
size_t
write_le32_array(FILE *restrict stream, uint32_t *restrict array, size_t n);
size_t
write_le64_array(FILE *restrict stream, uint64_t *restrict array, size_t n);
size_t put_u8(FILE *stream, uint8_t value);
size_t put_le16(FILE *stream, uint16_t value);
size_t put_le32(FILE *stream, uint32_t value);
size_t put_le64(FILE *stream, uint64_t value);
size_t put_be24(FILE *stream, uint32_t value);
size_t put_lef32(FILE *stream, float value);

size_t
read_le16_array(FILE *restrict stream, uint16_t *restrict out, size_t n);
size_t
read_le32_array(FILE *restrict stream, uint32_t *restrict out, size_t n);
size_t
read_le64_array(FILE *restrict stream, uint64_t *restrict out, size_t n);

int get_u8(FILE *restrict stream, uint8_t *restrict out);
int get_le16(FILE *restrict stream, uint16_t *restrict out);
int get_be24(FILE *restrict stream, uint32_t *restrict out);
int get_le32(FILE *restrict stream, uint32_t *restrict out);
int get_le64(FILE *restrict stream, uint64_t *restrict out);
int get_lef32(FILE *stream, float *out);

uint8_t get_u8_or0(FILE *stream);
uint16_t get_le16_or0(FILE *stream);
uint32_t get_be24_or0(FILE *stream);
uint32_t get_le32_or0(FILE *stream);
uint64_t get_le64_or0(FILE *stream);
float get_lef32_or0(FILE *stream);

size_t write_lef32_array(FILE *stream, float *array, size_t n);
size_t read_lef32_array(FILE *stream, float *array, size_t n);

#endif /* CEGSE_BINARY_STREAM_H */
