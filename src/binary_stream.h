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
#ifndef CEGSE_BINARY_STREAM_H
#define CEGSE_BINARY_STREAM_H

#include <stdio.h>
#include <stdint.h>

int put_u8(FILE *stream, uint8_t value);
int put_le16(FILE *stream, uint16_t value);
int put_le32(FILE *stream, uint32_t value);
int put_le64(FILE *stream, uint64_t value);
int put_be24(FILE *stream, uint32_t value);
int put_le32_ieee754(FILE *stream, float value);

int get_u8(FILE *restrict stream, uint8_t *restrict out);
int get_leu16(FILE *restrict stream, uint16_t *restrict out);
int get_leu32(FILE *restrict stream, uint32_t *restrict out);
int get_leu64(FILE *restrict stream, uint64_t *restrict out);
int get_lei16(FILE *restrict stream, int16_t *restrict out);
int get_lei32(FILE *restrict stream, int32_t *restrict out);
int get_lei64(FILE *restrict stream, int64_t *restrict out);
uint8_t get_u8_or_zero(FILE *stream);
uint16_t get_leu16_or_zero(FILE *stream);
uint32_t get_leu32_or_zero(FILE *stream);
uint64_t get_leu64_or_zero(FILE *stream);
int8_t get_i8_or_zero(FILE *stream);
int16_t get_lei16_or_zero(FILE *stream);
int32_t get_lei32_or_zero(FILE *stream);
int64_t get_lei64_or_zero(FILE *stream);
uint32_t get_beu24_or_zero(FILE *stream);
float get_le32_ieee754_or_zero(FILE *stream);

#endif /* CEGSE_BINARY_STREAM_H */
