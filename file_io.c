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

int sf_put_u8(FILE *stream, u8 value)
{
	if (fwrite(&value, sizeof(value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_get_u8(FILE *restrict stream, u8 *restrict value)
{
	if (fread(value, sizeof(*value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_put_u16(FILE *stream, u16 value)
{
	value = htole16(value);
	if (fwrite(&value, sizeof(value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_get_u16(FILE *restrict stream, u16 *restrict value)
{
	if (fread(value, sizeof(*value), 1, stream) < 1)
		return -1;

	*value = le16toh(*value);
	return 0;
}

int sf_put_u32(FILE *stream, u32 value)
{
	value = htole32(value);
	if (fwrite(&value, sizeof(value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_get_u32(FILE *restrict stream, u32 *restrict value)
{
	if (fread(value, sizeof(*value), 1, stream) < 1)
		return -1;

	*value = le32toh(*value);
	return 0;
}

int sf_put_u64(FILE *stream, u64 value)
{
	value = htole64(value);
	if (fwrite(&value, sizeof(value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_get_u64(FILE *restrict stream, u64 *restrict value)
{
	if (fread(value, sizeof(*value), 1, stream) < 1)
		return -1;

	*value = le64toh(*value);
	return 0;
}

int sf_put_i8(FILE *stream, i8 value)
{
	return sf_put_u8(stream, (u8)value);
}

int sf_get_i8(FILE *restrict stream, i8 *restrict value)
{
	return sf_get_u8(stream, (u8 *)value);
}

int sf_put_i16(FILE *stream, i16 value)
{
	return sf_put_u16(stream, (u16)value);
}

int sf_get_i16(FILE *restrict stream, i16 *restrict value)
{
	return sf_get_u16(stream, (u16 *)value);
}

int sf_put_i32(FILE *stream, i32 value)
{
	return sf_put_u32(stream, (u32)value);
}

int sf_get_i32(FILE *restrict stream, i32 *restrict value)
{
	return sf_get_u32(stream, (u32 *)value);
}

int sf_put_i64(FILE *stream, i64 value)
{
	return sf_put_u64(stream, (u64)value);
}

int sf_get_i64(FILE *restrict stream, i64 *restrict value)
{
	return sf_get_u64(stream, (u64 *)value);
}

int sf_put_f32(FILE *stream, f32 value)
{
	if (fwrite(&value, sizeof(value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_get_f32(FILE *restrict stream, f32 *restrict value)
{
	if (fread(value, sizeof(*value), 1, stream) < 1)
		return -1;

	return 0;
}

int sf_put_filetime(FILE *stream, FILETIME value)
{
	return sf_put_u64(stream, value);
}

int sf_get_filetime(FILE *restrict stream, FILETIME *restrict value)
{
	return sf_get_u64(stream, value);
}

int sf_put_vsval(FILE *stream, u32 value)
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

int sf_get_vsval(FILE *restrict stream, u32 *restrict value)
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

int sf_put_s(FILE *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);

	if (sf_put_u16(stream, len) == -1)
		return -1;

	if (fwrite(string, 1, len, stream) < len)
		return -1;

	return 0;
}

int sf_get_s(FILE *restrict stream, char **restrict dest)
{
	u16 len;
	char *string;

	if (sf_get_u16(stream, &len) == -1)
		return -1;

	string = malloc(len + 1);
	if (!string) {
		return -2;
	}

	if (fread(string, 1, len, stream) < len) {
		free(string);
		return -1;
	}

	string[len] = '\0';
	*dest = string;
	return len;
}
