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

#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "save_stream.h"

int sf_put_vsval(FILE *stream, u32 value)
{
	unsigned i, hibytes;
	DWARNC(value > VSVAL_MAX, "%u becomes %u\n",
	       (unsigned)value, (unsigned)(value % (VSVAL_MAX + 1)));

	value <<= 2u;
	hibytes = (value >= 0x100u) << (value >= 0x10000u);
	value |= hibytes;

	for (i = 0u; i <= hibytes; ++i) {
		if (sf_put_u8(stream, value >> i * 8u) == EOF)
			return EOF;
	}

	return hibytes;
}

int sf_get_vsval(FILE *restrict stream, u32 *restrict value)
{
	u8 byte = 0u;
	unsigned i;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		if (sf_get_u8(stream, &byte) == EOF)
			return EOF;
		*value |= byte << i * 8u;
	}

	*value >>= 2u;
	return i - 1u;
}

int sf_put_bstring(FILE *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);

	if (sf_put_u16(stream, len) == EOF)
		return EOF;

	return fwrite(string, len, 1u, stream) ? len : EOF;
}

int sf_get_bstring(FILE *restrict stream, char *restrict buf, size_t buf_size)
{
	unsigned read_len;
	unsigned i;
	u16 bs_len;

	if (sf_get_u16(stream, &bs_len) == EOF)
		return EOF;

	read_len = buf_size > bs_len ? bs_len : (buf_size - 1);

	if (!fread(buf, read_len, 1u, stream) && read_len != 0)
		return EOF;

	buf[read_len] = '\0';

	/* Make sure we're past the bstring. */
	for (i = 0u; i < bs_len - read_len; ++i) {
		if (fgetc(stream) == EOF)
			return EOF;
	}

	DWARNC(read_len != bs_len,
	       "truncated bstring due to insufficient buffer size\n");

	return read_len;
}

char *sf_malloc_bstring(FILE *restrict stream)
{
	char *string;
	u16 len;

	if (sf_get_u16(stream, &len) == EOF)
		return NULL;

	string = malloc(len + 1);
	if (!string)
		return NULL;

	if (!fread(string, len, 1u, stream)) {
		free(string);
		return NULL;
	}

	string[len] = '\0';
	return string;
}

int sf_compare(FILE *restrict stream, const void *restrict data, size_t num)
{
	unsigned char buf[64];
	size_t read_sz;

	while (num) {
		read_sz = num < sizeof(buf) ? num : sizeof(buf);

		if (!fread(buf, read_sz, 1u, stream))
			return ferror(stream) ? EOF : 1;

		if (memcmp(data, buf, read_sz) != 0)
			return 1;

		data = (unsigned char*)data + read_sz;
		num -= read_sz;
	}

	return 0;
}
