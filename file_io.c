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

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"

int file_compare(FILE *restrict stream, const void *restrict data, int num)
{
	int c;
	int i;

	for (i = 0; i < num; ++i) {
		c = fgetc(stream);
		if (c == EOF && ferror(stream))
			return -S_EFILE;

		if (c != (int)((u8 *)data)[i]) {
			fseek(stream, -(i + 1), SEEK_CUR);
			return 1;
		}

	}

	return 0;
}

int sf_write(struct save_file *restrict stream, const void *restrict src,
	     size_t size)
{
	size_t n_written;
	if (stream->status != S_OK)
		return -stream->status;

	n_written = fwrite(src, 1u, size, stream->stream);
	if (n_written < size) {
		stream->status = S_EFILE;
	}

	return -stream->status;
}

int sf_read(struct save_file *restrict stream, void *restrict dest, size_t size)
{
	size_t n_read;
	if (stream->status != S_OK)
		return -stream->status;

	n_read = fread(dest, 1u, size, stream->stream);
	if (n_read < size) {
		if (feof(stream->stream))
			stream->status = S_EOF;
		else
			stream->status = S_EFILE;
	}

	return -stream->status;
}


int sf_put_vsval(struct save_file *stream, u32 value)
{
	u32 i;
	if (value < 0x40u)
		return sf_put_u8(stream, value << 2u);

	if (value < 0x4000u)
		return sf_put_u16(stream, value << 2u | 1u);

	value = value << 2u | 2u;
	for (i = 0u; i < 3u; ++i)
		sf_put_u8(stream, value >> i * 8u);

	return -stream->status;
}

int sf_get_vsval(struct save_file *restrict stream, u32 *restrict value)
{
	u8 byte;
	u32 i;

	/* Read 1 to 3 bytes into value. */
	i = 0u;
	*value = 0u;
	do {
		if (sf_get_u8(stream, &byte) < 0)
			return -stream->status;

		*value |= byte << i * 8u;
		i++;
	} while (i < (*value & 0x3u) + 1u);

	*value >>= 2u;
	return 0;
}

int sf_put_s(struct save_file *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);
	sf_put_u16(stream, len);
	sf_write(stream, string, len);
	return -stream->status;
}

int sf_get_s(struct save_file *restrict stream, char **restrict dest)
{
	u16 len;
	char *string;

	sf_get_u16(stream, &len);
	string = malloc(len + 1);
	if (!string) {
		stream->status = S_EMEM;
		return -S_EMEM;
	}

	sf_read(stream, string, len);
	if (stream->status != S_OK) {
		free(string);
		return -stream->status;
	}

	string[len] = '\0';
	*dest = string;
	return len;
}

int sf_get_ns(struct save_file *restrict stream, char *restrict dest,
	      size_t dest_size)
{
	u16 len;

	sf_get_u16(stream, &len);

	if (dest_size < len + 1u) {
		stream->status = S_ESIZE;
		return -S_ESIZE;
	}

	if (sf_read(stream, dest, len) < 0) {
		return -stream->status;
	}

	dest[len] = '\0';
	return len;
}
