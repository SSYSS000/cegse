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
#include <stdio.h>
#include <errno.h>

#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#include "stream.h"

#define DEF_WRITE_LE_FUNC(suffix, type)										\
size_t stream_write_le_##suffix(type value, FILE *stream)					\
{																			\
	return stream_write_le(&value, sizeof(type), stream);					\
}

#define DEF_READ_LE_FUNC(suffix, type)										\
size_t stream_read_le_##suffix(type *value, FILE *stream)					\
{																			\
	return stream_read_le(value, sizeof(type), stream);						\
}

/*
 * Write data in reverse order.
 */
static size_t stream_rwrite(const void *restrict data, size_t size, FILE *restrict stream)
{
	const unsigned char *bytes = data;
	size_t i = size;

	while (i-- > 0) {
		if (fputc(bytes[i], stream) == EOF) {
			return 0;
		}
	}

	return size;
}

/*
 * Read data in reverse order.
 */
static size_t stream_rread(void *restrict data, size_t size, FILE *restrict stream)
{
	unsigned char *bytes = data;
	size_t i = size;
	int c;

	while (i-- > 0) {
		if ((c = fgetc(stream)) == EOF) {
			return 0;
		}

		bytes[i] = c;
	}

	return size;
}

size_t stream_write_le(const void *restrict data, size_t size, FILE *restrict stream)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return fwrite(data, size, 1u, stream) * size;
#elif BYTE_ORDER == BIG_ENDIAN
	return stream_rwrite(data, size, stream);
#else
# error "Byte order not supported."
#endif
}

size_t stream_read_le(void *restrict data, size_t size, FILE *restrict stream)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	return fread(data, size, 1u, stream) * size;
#elif BYTE_ORDER == BIG_ENDIAN
	return stream_rread(data, size, stream);
#else
# error "Byte order not supported."
#endif
}

size_t stream_ignore(size_t num_bytes, FILE *stream)
{
	size_t num_ignored = 0;

	if (fseek(stream, num_bytes, SEEK_CUR) == 0) {
		num_ignored = num_bytes;
	}
	else if (errno == ESPIPE) {
		/* stream is not seekable, try extracting characters. */
		while (num_ignored < num_bytes && fgetc(stream) != EOF) {
			num_ignored++;
		}
	}
	else {
		/* EINVAL could indicate a bug. */
		perror("fseek");
	}

	return num_ignored;
}

size_t stream_write_f32(f32 value, FILE *stream)
{
	return fwrite(&value, sizeof value, 1u, stream) * sizeof(value);
}

size_t stream_read_f32(f32 *value, FILE *stream)
{
	return fread(value, sizeof *value, 1u, stream) * sizeof(*value);
}

DEF_WRITE_LE_FUNC(u16, u16)
DEF_READ_LE_FUNC(u16, u16)
DEF_WRITE_LE_FUNC(u32, u32)
DEF_READ_LE_FUNC(u32, u32)
DEF_WRITE_LE_FUNC(u64, u64)
DEF_READ_LE_FUNC(u64, u64)
