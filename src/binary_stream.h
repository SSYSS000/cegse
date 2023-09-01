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
#ifndef CEGSE_BINARY_STREAM_H
#define CEGSE_BINARY_STREAM_H

#include <stdio.h>
#include <stdint.h>

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
size_t put_le32_ieee754(FILE *stream, float value);

size_t
read_le16_array(FILE *restrict stream, uint16_t *restrict out, size_t n);
size_t
read_le32_array(FILE *restrict stream, uint32_t *restrict out, size_t n);
size_t
read_le64_array(FILE *restrict stream, uint64_t *restrict out, size_t n);
size_t get_u8(FILE *restrict stream, uint8_t *restrict out);
size_t get_le16(FILE *restrict stream, uint16_t *restrict out);
size_t get_le32(FILE *restrict stream, uint32_t *restrict out);
size_t get_le64(FILE *restrict stream, uint64_t *restrict out);
uint8_t get_u8_or_zero(FILE *stream);
uint16_t get_le16_or_zero(FILE *stream);
uint32_t get_le32_or_zero(FILE *stream);
uint64_t get_le64_or_zero(FILE *stream);
uint32_t get_beu24_or_zero(FILE *stream);

size_t write_le32_ieee754_array(FILE *stream, float *array, size_t n);
size_t read_le32_ieee754_array(FILE *stream, float *array, size_t n);
size_t get_le32_ieee754(FILE *stream, float *out);
float get_le32_ieee754_or_zero(FILE *stream);

#endif /* CEGSE_BINARY_STREAM_H */
