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

#ifndef CEGSE_SAVE_STREAM_H
#define CEGSE_SAVE_STREAM_H

#include <stdio.h>
#include "types.h"

#include <sys/param.h>

#if defined(BSD)
#include <sys/endian.h>
#else
#include <endian.h>
#endif

/*
 * Write an unsigned 8-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_u8(FILE *stream, u8 value)
{
	return fputc(value, stream);
}

/*
 * Read an unsigned 8-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_u8(FILE *restrict stream, u8 *restrict value)
{
	int c = fgetc(stream);
	*value = (u8)c;
	return c;
}

/*
 * Write an unsigned 16-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_u16(FILE *stream, u16 value)
{
	value = htole16(value);
	return fwrite(&value, sizeof(value), 1u, stream) ? 1 : EOF;
}

/*
 * Read an unsigned 16-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_u16(FILE *restrict stream, u16 *restrict value)
{
	if (!fread(value, sizeof(*value), 1u, stream))
		return EOF;

	*value = le16toh(*value);
	return 1;
}

/*
 * Write an unsigned 32-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_u32(FILE *stream, u32 value)
{
	value = htole32(value);
	return fwrite(&value, sizeof(value), 1u, stream) ? 1 : EOF;
}

/*
 * Read an unsigned 32-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_u32(FILE *restrict stream, u32 *restrict value)
{
	if (!fread(value, sizeof(*value), 1u, stream))
		return EOF;

	*value = le32toh(*value);
	return 1;
}

/*
 * Write an unsigned 64-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_u64(FILE *stream, u64 value)
{
	value = htole64(value);
	return fwrite(&value, sizeof(value), 1u, stream) ? 1 : EOF;
}

/*
 * Read an unsigned 64-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_u64(FILE *restrict stream, u64 *restrict value)
{
	if (!fread(value, sizeof(*value), 1u, stream))
		return EOF;

	*value = le64toh(*value);
	return 1;
}

/*
 * Write a signed 8-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_i8(FILE *stream, i8 value)
{
	return sf_put_u8(stream, (u8)value);
}

/*
 * Read a signed 8-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_i8(FILE *restrict stream, i8 *restrict value)
{
	return sf_get_u8(stream, (u8 *)value);
}

/*
 * Write a signed 16-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_i16(FILE *stream, i16 value)
{
	return sf_put_u16(stream, (u16)value);
}

/*
 * Read a signed 16-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_i16(FILE *restrict stream, i16 *restrict value)
{
	return sf_get_u16(stream, (u16 *)value);
}

/*
 * Write a signed 32-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_i32(FILE *stream, i32 value)
{
	return sf_put_u32(stream, (u32)value);
}

/*
 * Read a signed 32-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_i32(FILE *restrict stream, i32 *restrict value)
{
	return sf_get_u32(stream, (u32 *)value);
}

/*
 * Write a signed 64-bit integer to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_i64(FILE *stream, i64 value)
{
	return sf_put_u64(stream, (u64)value);
}

/*
 * Read a signed 64-bit integer from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_i64(FILE *restrict stream, i64 *restrict value)
{
	return sf_get_u64(stream, (u64 *)value);
}

/*
 * Write a 32-bit floating point to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_f32(FILE *stream, f32 value)
{
	return fwrite(&value, sizeof(value), 1u, stream) ? 1 : EOF;
}

/*
 * Read a 32-bit floating point from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_f32(FILE *restrict stream, f32 *restrict value)
{
	return fread(value, sizeof(*value), 1u, stream) ? 1 : EOF;
}

/*
 * Write a FILETIME to stream.
 *
 * Return EOF on error.
 */
static inline int sf_put_filetime(FILE *restrict stream, FILETIME value)
{
	return sf_put_u64(stream, value);
}

/*
 * Read a FILETIME from stream.
 *
 * Return EOF on end of file or error.
 */
static inline int sf_get_filetime(FILE *restrict stream,
	FILETIME *restrict value)
{
	return sf_get_u64(stream, value);
}

/*
 * Write a variable-size value to stream.
 *
 * NOTE: Values greater than VSVAL_MAX are wrapped.
 *
 * Return the number of high bytes or EOF on error.
 */
int sf_put_vsval(FILE *stream, u32 value);

/*
 * Read a variable-size value from stream.
 *
 * Return the number of high bytes or EOF on end of file or error.
 */
int sf_get_vsval(FILE *restrict stream, u32 *restrict value);

/*
 * Write a string to stream without the terminating null-byte character,
 * prefixed by a 16-bit integer denoting its length.
 *
 * Return EOF on error.
 */
int sf_put_bstring(FILE *restrict stream, const char *restrict string);

/*
 * Read a string, prefixed by its 16-bit length, from stream to buf.
 *
 * At most buf_size - 1 characters are read to buf. buf is always
 * null-terminated. The characters that cannot fit into buf are skipped.
 *
 * Return the number of characters read or EOF on end of file or error.
 */
int sf_get_bstring(FILE *restrict stream, char *restrict buf, size_t buf_size);

/*
 * Read a string, prefixed by its 16-bit length, from stream. Memory for
 * the string is allocated using malloc(). The caller is responsible for
 * freeing the string.
 *
 * Callers must use ferror() and feof() to determine error.
 *
 * Return the allocated string or NULL on malloc failure,
 * end of file or file error.
 */
char *sf_malloc_bstring(FILE *restrict stream);

/*
 * Extract at most num bytes from stream and compare them with data.
 *
 * The size of data should not be less than num.
 *
 * If equal, return 0. If unequal or EOF is reached, return 1.
 * On file error, return -S_EFILE.
 */
int sf_compare(FILE *restrict stream, const void *restrict data, size_t num);

#endif /* CEGSE_SAVE_STREAM_H */
