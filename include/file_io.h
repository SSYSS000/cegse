/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2021  SSYSS000

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

#ifndef CEGSE_FILE_IO_H
#define CEGSE_FILE_IO_H

#include <stdio.h>
#include "types.h"


/*
 * Write an unsigned 8-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_u8(FILE *stream, u8 value);

/*
 * Read an unsigned 8-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_u8(FILE *restrict stream, u8 *restrict value);

/*
 * Write an unsigned 16-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_u16(FILE *stream, u16 value);

/*
 * Read an unsigned 16-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_u16(FILE *restrict stream, u16 *restrict value);

/*
 * Write an unsigned 32-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_u32(FILE *stream, u32 value);

/*
 * Read an unsigned 32-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_u32(FILE *restrict stream, u32 *restrict value);

/*
 * Write an unsigned 64-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_u64(FILE *stream, u64 value);

/*
 * Read an unsigned 64-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_u64(FILE *restrict stream, u64 *restrict value);

/*
 * Write a signed 8-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_i8(FILE *stream, i8 value);

/*
 * Read a signed 8-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_i8(FILE *restrict stream, i8 *restrict value);

/*
 * Write a signed 16-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_i16(FILE *stream, i16 value);

/*
 * Read a signed 16-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_i16(FILE *restrict stream, i16 *restrict value);

/*
 * Write a signed 32-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_i32(FILE *stream, i32 value);

/*
 * Read a signed 32-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_i32(FILE *restrict stream, i32 *restrict value);

/*
 * Write a signed 64-bit integer to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_i64(FILE *stream, i64 value);

/*
 * Read a signed 64-bit integer from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_i64(FILE *restrict stream, i64 *restrict value);

/*
 * Write a 32-bit floating point to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_f32(FILE *stream, f32 value);

/*
 * Read a 32-bit floating point from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_f32(FILE *restrict stream, f32 *restrict value);

/*
 * Write a FILETIME to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_filetime(FILE *stream, FILETIME value);

/*
 * Read a FILETIME from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_filetime(FILE *restrict stream, FILETIME *restrict value);

/*
 * Write a variable-size value to stream.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_vsval(FILE *stream, u32 value);

/*
 * Read a variable-size value from stream.
 *
 * On success, return nonnegative integer.
 * On file error or EOF, return -1.
 */
int sf_get_vsval(FILE *restrict stream, u32 *restrict value);

/*
 * Write a string to stream without the terminating null-byte character,
 * prefixed by a 16-bit integer denoting its length.
 *
 * On success, return nonnegative integer.
 * On file error, return -1.
 */
int sf_put_s(FILE *restrict stream, const char *restrict string);

/*
 * Read a string, prefixed by its 16-bit length, from stream. The caller is
 * responsible for freeing the string mallocated in dest.
 *
 * On success, return the length of the string.
 * On file error or EOF, return -1.
 * On memory allocation error, return -2.
 */
int sf_get_s(FILE *restrict stream, char **restrict dest);

#endif // CEGSE_FILE_IO_H
