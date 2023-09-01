/*
Copyright (C) 2022  SSYSS000

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

size_t put_u8(FILE *stream, uint8_t value)
{
    return fputc(value, stream) == EOF ? 0 : 1;
}

size_t get_u8(FILE *restrict stream, uint8_t *restrict out)
{
    int c = fgetc(stream);

    if (c == EOF) {
        return 0;
    }

    *out = c;
    return 1;
}

uint8_t get_u8_or_zero(FILE *stream)
{
    int c = fgetc(stream);
    return c == EOF ? 0 : c;
}

size_t put_be24(FILE *stream, uint32_t value)
{
    uint8_t octets[3] = {
        value >> 16 & 0xff,
        value >>  8 & 0xff,
        value       & 0xff
    };

    return fwrite(octets, sizeof(octets), 1, stream);
}

uint32_t get_beu24_or_zero(FILE *stream)
{
    uint8_t o[3];

    if (fread(o, sizeof(o), 1, stream) != 1) {
        return 0;
    }

    return (uint32_t) o[0] << 16 | (uint32_t) o[1] << 8 | (uint32_t) o[2];
}

size_t write_le32_ieee754_array(FILE *stream, float *array, size_t n)
{
    if (!float_is_ieee754_little_endian()) {
        eprintf("Unsupported floating point format\n");
        abort();
    }

    return fwrite(array, sizeof(*array), n, stream);
}

size_t put_le32_ieee754(FILE* stream, float value)
{
    return write_le32_ieee754_array(stream, &value, 1);
}

size_t read_le32_ieee754_array(FILE *stream, float *array, size_t n)
{
    if (!float_is_ieee754_little_endian()) {
        eprintf("Unsupported floating point format\n");
        abort();
    }
    
    return fread(array, sizeof(*array), n, stream);
}

size_t get_le32_ieee754(FILE *stream, float *out)
{
    return read_le32_ieee754_array(stream, out, 1);
}

float get_le32_ieee754_or_zero(FILE* stream)
{
    float value;

    if (get_le32_ieee754(stream, &value) != 1) {
        return 0.0f;
    }

    return value;
}

#define DEFINE_PUT_LE(bits)                                                 \
size_t write_le##bits##_array(FILE *restrict stream,                        \
        uint##bits##_t *restrict array, size_t n)                           \
{                                                                           \
    size_t n_written;                                                       \
                                                                            \
    for (size_t i = 0; i < n; ++i) {                                        \
        array[i] = htole##bits(array[i]);                                   \
    }                                                                       \
                                                                            \
    n_written = fwrite(array, sizeof(*array), n, stream);                   \
                                                                            \
    for (size_t i = 0; i < n; ++i) {                                        \
        array[i] = le##bits##toh(array[i]);                                 \
    }                                                                       \
                                                                            \
    return n_written;                                                       \
}                                                                           \
                                                                            \
size_t put_le##bits(FILE *stream, uint##bits##_t value)                     \
{                                                                           \
    return write_le##bits##_array(stream, &value, 1);                       \
}

#define DEFINE_GET_LE(bits)                                                 \
size_t read_le##bits##_array(FILE *restrict stream,                         \
        uint##bits##_t *restrict array, size_t n)                           \
{                                                                           \
    n = fread(array, sizeof(*array), n, stream);                            \
                                                                            \
    for (size_t i = 0; i < n; ++i) {                                        \
        array[i] = le##bits##toh(array[i]);                                 \
    }                                                                       \
                                                                            \
    return n;                                                               \
}                                                                           \
                                                                            \
size_t get_le##bits(FILE *restrict stream, uint##bits##_t *restrict out)    \
{                                                                           \
    return read_le##bits##_array(stream, out, 1);                           \
}                                                                           \
                                                                            \
uint##bits##_t get_le##bits##_or_zero(FILE *stream)                         \
{                                                                           \
    uint##bits##_t value;                                                   \
    if (!get_le##bits(stream, &value)) {                                    \
        return 0;                                                           \
    }                                                                       \
    return value;                                                           \
}

DEFINE_PUT_LE(16)
DEFINE_PUT_LE(32)
DEFINE_PUT_LE(64)

DEFINE_GET_LE(16)
DEFINE_GET_LE(32)
DEFINE_GET_LE(64)
