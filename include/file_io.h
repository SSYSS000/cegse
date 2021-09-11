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

enum sf_stream_status {
	SF_OK = 0,
	SF_EOF,		/* End-of-file */
	SF_EFILE,	/* File error */
	SF_EMEM,	/* Memory allocation error */
	SF_ESIZE 	/* Size error; buffer too small or size mismatch */
};

struct sf_stream {
	FILE *stream;
	enum sf_stream_status status;
	u32 version;
	u8 format;
};

/*
 * Write exactly size bytes from src to stream. If error status is set,
 * fail instead, but don't set error status.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_write(struct sf_stream *restrict stream, const void *restrict src,
	     size_t size);

/*
 * Read exactly size bytes from stream to dest. If error status is set,
 * fail instead, but don't set error status.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_read(struct sf_stream *restrict stream, void *restrict dest, size_t size);

/*
 * Write an unsigned 8-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_u8(struct sf_stream *stream, u8 value);

/*
 * Read an unsigned 8-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_u8(struct sf_stream *restrict stream, u8 *restrict value);

/*
 * Write an unsigned 16-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_u16(struct sf_stream *stream, u16 value);

/*
 * Read an unsigned 16-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_u16(struct sf_stream *restrict stream, u16 *restrict value);

/*
 * Write an unsigned 32-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_u32(struct sf_stream *stream, u32 value);

/*
 * Read an unsigned 32-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_u32(struct sf_stream *restrict stream, u32 *restrict value);

/*
 * Write an unsigned 64-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_u64(struct sf_stream *stream, u64 value);

/*
 * Read an unsigned 64-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_u64(struct sf_stream *restrict stream, u64 *restrict value);

/*
 * Write a signed 8-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_i8(struct sf_stream *stream, i8 value);

/*
 * Read a signed 8-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_i8(struct sf_stream *restrict stream, i8 *restrict value);

/*
 * Write a signed 16-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_i16(struct sf_stream *stream, i16 value);

/*
 * Read a signed 16-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_i16(struct sf_stream *restrict stream, i16 *restrict value);

/*
 * Write a signed 32-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_i32(struct sf_stream *stream, i32 value);

/*
 * Read a signed 32-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_i32(struct sf_stream *restrict stream, i32 *restrict value);

/*
 * Write a signed 64-bit integer to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_i64(struct sf_stream *stream, i64 value);

/*
 * Read a signed 64-bit integer from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_i64(struct sf_stream *restrict stream, i64 *restrict value);

/*
 * Write a 32-bit floating point to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_f32(struct sf_stream *stream, f32 value);

/*
 * Read a 32-bit floating point from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_f32(struct sf_stream *restrict stream, f32 *restrict value);

/*
 * Write a FILETIME to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_filetime(struct sf_stream *stream, FILETIME value);

/*
 * Read a FILETIME from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_filetime(struct sf_stream *restrict stream, FILETIME *restrict value);

/*
 * Write a variable-size value to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_vsval(struct sf_stream *stream, u32 value);

/*
 * Read a variable-size value from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_get_vsval(struct sf_stream *restrict stream, u32 *restrict value);

/*
 * Write a string to stream without the terminating null-byte character,
 * prefixed by a 16-bit integer denoting its length.
 *
 * On success, return a nonnegative integer.
 * On failure, set error status and return a negative integer.
 */
int sf_put_s(struct sf_stream *restrict stream, const char *restrict string);

/*
 * Read a string, prefixed by its 16-bit length, from stream. The caller is
 * responsible for freeing the string mallocated in dest.
 * The contents of dest remain unchanged on failure.
 *
 * On success, return the length of the string.
 * On failure, set error status and return a negative integer.
 */
int sf_get_s(struct sf_stream *restrict stream, char **restrict dest);

/*
 * Read a string, prefixed by its 16-bit length, from stream to dest.
 *
 * If the string cannot fit to dest along with its
 * terminating null-byte (strlen+1 > dest_size), the function fails and
 * SF_ESIZE is set. The contents of dest remain unchanged, too.
 *
 * On success, return the length of the string.
 * On failure, set error status and return a negative integer.
 */
int sf_get_ns(struct sf_stream *restrict stream, char *restrict dest,
	      size_t dest_size);

#endif // CEGSE_FILE_IO_H
