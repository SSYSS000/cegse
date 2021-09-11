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

#include <endian.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"

int sf_write(struct sf_stream *restrict stream, const void *restrict src,
	     size_t size)
{
	size_t n_written;
	if (stream->status != SF_OK)
		return -1;

	n_written = fwrite(src, 1u, size, stream->stream);
	if (n_written < size) {
		if (feof(stream->stream))
			stream->status = SF_EOF;
		else
			stream->status = SF_EFILE;

		return -1;
	}

	return 0;
}

int sf_read(struct sf_stream *restrict stream, void *restrict dest, size_t size)
{
	size_t n_read;
	if (stream->status != SF_OK)
		return -1;

	n_read = fread(dest, 1u, size, stream->stream);
	if (n_read < size) {
		if (feof(stream->stream))
			stream->status = SF_EOF;
		else
			stream->status = SF_EFILE;

		return -1;
	}

	return 0;
}

int sf_put_u8(struct sf_stream *stream, u8 value)
{
	return sf_write(stream, &value, sizeof(value));
}

int sf_get_u8(struct sf_stream *restrict stream, u8 *restrict value)
{
	return sf_read(stream, value, sizeof(*value));
}

int sf_put_u16(struct sf_stream *stream, u16 value)
{
	value = htole16(value);
	return sf_write(stream, &value, sizeof(value));
}

int sf_get_u16(struct sf_stream *restrict stream, u16 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le16toh(*value);
	return ret;
}

int sf_put_u32(struct sf_stream *stream, u32 value)
{
	value = htole32(value);
	return sf_write(stream, &value, sizeof(value));
}

int sf_get_u32(struct sf_stream *restrict stream, u32 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le32toh(*value);
	return 0;
}

int sf_put_u64(struct sf_stream *stream, u64 value)
{
	value = htole64(value);
	return sf_write(stream, &value, sizeof(value));
}

int sf_get_u64(struct sf_stream *restrict stream, u64 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le64toh(*value);
	return 0;
}

int sf_put_i8(struct sf_stream *stream, i8 value)
{
	return sf_put_u8(stream, (u8)value);
}

int sf_get_i8(struct sf_stream *restrict stream, i8 *restrict value)
{
	return sf_get_u8(stream, (u8 *)value);
}

int sf_put_i16(struct sf_stream *stream, i16 value)
{
	return sf_put_u16(stream, (u16)value);
}

int sf_get_i16(struct sf_stream *restrict stream, i16 *restrict value)
{
	return sf_get_u16(stream, (u16 *)value);
}

int sf_put_i32(struct sf_stream *stream, i32 value)
{
	return sf_put_u32(stream, (u32)value);
}

int sf_get_i32(struct sf_stream *restrict stream, i32 *restrict value)
{
	return sf_get_u32(stream, (u32 *)value);
}

int sf_put_i64(struct sf_stream *stream, i64 value)
{
	return sf_put_u64(stream, (u64)value);
}

int sf_get_i64(struct sf_stream *restrict stream, i64 *restrict value)
{
	return sf_get_u64(stream, (u64 *)value);
}

int sf_put_f32(struct sf_stream *stream, f32 value)
{
	return sf_write(stream, &value, sizeof(value));
}

int sf_get_f32(struct sf_stream *restrict stream, f32 *restrict value)
{
	return sf_read(stream, value, sizeof(*value));
}

int sf_put_filetime(struct sf_stream *stream, FILETIME value)
{
	return sf_put_u64(stream, value);
}

int sf_get_filetime(struct sf_stream *restrict stream, FILETIME *restrict value)
{
	return sf_get_u64(stream, value);
}

int sf_put_vsval(struct sf_stream *stream, u32 value)
{
	u32 i;
	if (value < 0x40u)
		return sf_put_u8(stream, value << 2u);

	if (value < 0x4000u)
		return sf_put_u16(stream, value << 2u | 1u);

	value = value << 2u | 2u;
	for (i = 0u; i < 3u; ++i) {
		if (sf_put_u8(stream, value >> i * 8u) == -1)
			return -1;
	}

	return 0;
}

int sf_get_vsval(struct sf_stream *restrict stream, u32 *restrict value)
{
	u8 byte;
	u32 i;

	/* Read 1 to 3 bytes into value. */
	i = 0u;
	*value = 0u;
	do {
		if (sf_get_u8(stream, &byte) == -1)
			return -1;

		*value |= byte << i * 8u;
		i++;
	} while (i < (*value & 0x3u) + 1u);

	*value >>= 2u;
	return 0;
}

int sf_put_s(struct sf_stream *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);

	if (sf_put_u16(stream, len) < 0)
		return -1;

	if (sf_write(stream, string, len) < 0)
		return -1;

	return 0;
}

int sf_get_s(struct sf_stream *restrict stream, char **restrict dest)
{
	u16 len;
	char *string;

	if (sf_get_u16(stream, &len) < 0)
		return -1;

	string = malloc(len + 1);
	if (!string) {
		stream->status = SF_EMEM;
		return -1;
	}

	if (sf_read(stream, string, len) < 0) {
		free(string);
		return -1;
	}

	string[len] = '\0';
	*dest = string;
	return len;
}
