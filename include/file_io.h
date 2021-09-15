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

struct save_file {
	FILE *stream;
	enum status_code status;
	u32 engine_version;		/* Creation Engine version */
	u8 format;
};

/*
 * Compare the next num bytes in stream with data.
 *
 * If equal, return 0. If unequal or EOF is reached,
 * seek the file back to the original position and return 1.
 *
 * The size of data should not be less than num.
 *
 * On file error, return -S_EFILE.
 */
int file_compare(FILE *restrict stream, const void *restrict data, int num);

/*
 * NOTE: All sf_* functions fail immediately if the stream has faced an error,
 * i.e. stream status != S_OK.
 */

/*
 * Write exactly size bytes from src to stream.
 *
 * On file error, set stream status to S_EFILE.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_write(struct save_file *restrict stream, const void *restrict src,
	     size_t size);

/*
 * Read exactly size bytes from stream to dest.
 *
 * On file error, set stream status to S_EFILE.
 * On EOF, set stream status to S_EOF.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_read(struct save_file *restrict stream, void *restrict dest, size_t size);

/*
 * Write an unsigned 8-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_u8(struct save_file *stream, u8 value);

/*
 * Read an unsigned 8-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_u8(struct save_file *restrict stream, u8 *restrict value);

/*
 * Write an unsigned 16-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_u16(struct save_file *stream, u16 value);

/*
 * Read an unsigned 16-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_u16(struct save_file *restrict stream, u16 *restrict value);

/*
 * Write an unsigned 32-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_u32(struct save_file *stream, u32 value);

/*
 * Read an unsigned 32-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_u32(struct save_file *restrict stream, u32 *restrict value);

/*
 * Write an unsigned 64-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_u64(struct save_file *stream, u64 value);

/*
 * Read an unsigned 64-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_u64(struct save_file *restrict stream, u64 *restrict value);

/*
 * Write a signed 8-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_i8(struct save_file *stream, i8 value);

/*
 * Read a signed 8-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_i8(struct save_file *restrict stream, i8 *restrict value);

/*
 * Write a signed 16-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_i16(struct save_file *stream, i16 value);

/*
 * Read a signed 16-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_i16(struct save_file *restrict stream, i16 *restrict value);

/*
 * Write a signed 32-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_i32(struct save_file *stream, i32 value);

/*
 * Read a signed 32-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_i32(struct save_file *restrict stream, i32 *restrict value);

/*
 * Write a signed 64-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_i64(struct save_file *stream, i64 value);

/*
 * Read a signed 64-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_i64(struct save_file *restrict stream, i64 *restrict value);

/*
 * Write a 32-bit floating point to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_f32(struct save_file *stream, f32 value);

/*
 * Read a 32-bit floating point from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_f32(struct save_file *restrict stream, f32 *restrict value);

/*
 * Write a FILETIME to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_filetime(struct save_file *stream, FILETIME value);

/*
 * Read a FILETIME from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_filetime(struct save_file *restrict stream, FILETIME *restrict value);

/*
 * Write a variable-size value to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_vsval(struct save_file *stream, u32 value);

/*
 * Read a variable-size value from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_vsval(struct save_file *restrict stream, u32 *restrict value);

/*
 * Write a string to stream without the terminating null-byte character,
 * prefixed by a 16-bit integer denoting its length.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_s(struct save_file *restrict stream, const char *restrict string);

/*
 * Read a string, prefixed by its 16-bit length, from stream. The caller is
 * responsible for freeing the string mallocated in dest.
 *
 * On memory allocation error, the contents of dest remain unchanged
 * and stream status is set to S_EMEM.
 *
 * On success, return the length of the string. Otherwise,
 * return the stream status negated.
 */
int sf_get_s(struct save_file *restrict stream, char **restrict dest);

/*
 * Read a string, prefixed by its 16-bit length, from stream to dest.
 *
 * If the string cannot fit to dest along with its
 * terminating null-byte (strlen+1 > dest_size), the function fails and
 * S_ESIZE is set. The contents of dest remain unchanged, too.
 *
 * On success, return the length of the string. Otherwise,
 * return the stream status negated.
 */
int sf_get_ns(struct save_file *restrict stream, char *restrict dest,
	      size_t dest_size);

#endif // CEGSE_FILE_IO_H
