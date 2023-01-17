/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "alloc.h"
#include "binary_stream.h"
#include "defines.h"
#include "endian.h"
#include "log.h"

int put_u8(FILE *stream, uint8_t value)
{
    return fputc(value, stream) == EOF ? EOF : 0;
}

int get_u8(FILE *restrict stream, uint8_t *restrict out)
{
    return fread(out, sizeof(*out), 1, stream);
}

uint8_t get_u8_or_zero(FILE *stream)
{
    int c = fgetc(stream);
    return c == EOF ? 0 : c;
}

int8_t get_i8_or_zero(FILE *stream)
{
    uint8_t octet = get_u8_or_zero(stream);
    return *(int8_t*)&octet;
}

int put_be24(FILE *stream, uint32_t value)
{
    uint8_t octets[3] = {
        value >> 16 & 0xff,
        value >>  8 & 0xff,
        value       & 0xff
    };

    return fwrite(octets, sizeof(octets), 1, stream) * 3;
}

uint32_t get_beu24_or_zero(FILE *stream)
{
    return (uint32_t) get_u8_or_zero(stream) << 16 |
        (uint32_t) get_u8_or_zero(stream) << 8 |
        (uint32_t) get_u8_or_zero(stream);
}

int put_le32_ieee754(FILE* stream, float value)
{
    if (!float_is_ieee754_little_endian())
    {
        eprintf("Unsupported floating point format\n");
        abort();
    }

    return fwrite(&value, sizeof(value), 1, stream) * sizeof(float);
}

float get_le32_ieee754_or_zero(FILE* stream)
{
    float value;

    if (!float_is_ieee754_little_endian())
    {
        eprintf("Unsupported floating point format\n");
        abort();
    }

    if (fread(&value, sizeof(value), 1, stream) != 1) {
        return 0.0f;
    }

    return value;
}

#define DEFINE_PUT_LE(bits)                                                 \
int put_le##bits(FILE *stream, uint##bits##_t value)                        \
{                                                                           \
    value = htole##bits(value);                                             \
    return fwrite(&value, sizeof(value), 1, stream) * sizeof(value);        \
}

#define DEFINE_GET_LE(bits)                                                 \
int get_leu##bits(FILE *stream, uint##bits##_t *out)                        \
{                                                                           \
    if (fread(out, sizeof(*out), 1, stream) != 1) {                         \
        return 0;                                                           \
    }                                                                       \
    *out = le##bits##toh(*out);                                             \
    return sizeof(*out);                                                    \
}                                                                           \
                                                                            \
int get_lei##bits(FILE *stream, int##bits##_t *out)                         \
{                                                                           \
    return get_leu##bits(stream, (uint##bits##_t *)out);                    \
}                                                                           \
                                                                            \
uint##bits##_t get_leu##bits##_or_zero(FILE *stream)                        \
{                                                                           \
    uint##bits##_t value;                                                   \
    if (!get_leu##bits(stream, &value)) {                                   \
        return 0;                                                           \
    }                                                                       \
    return le##bits##toh(value);                                            \
}                                                                           \
                                                                            \
int##bits##_t get_lei##bits##_or_zero(FILE *stream)               \
{                                                                           \
    int##bits##_t s;                                                        \
    uint##bits##_t u = get_leu##bits##_or_zero(stream);                     \
    memcpy(&s, &u, sizeof(u));                                              \
    return s;                                                               \
}

DEFINE_PUT_LE(16)
DEFINE_PUT_LE(32)
DEFINE_PUT_LE(64)

DEFINE_GET_LE(16)
DEFINE_GET_LE(32)
DEFINE_GET_LE(64)

