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

int sf_write(struct save_stream *restrict stream, const void *restrict src,
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

int sf_read(struct save_stream *restrict stream, void *restrict dest, size_t size)
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

int sf_put_vsval(struct save_stream *stream, u32 value)
{
	unsigned i, hibytes;
	DWARNC(value > VSVAL_MAX, "%u becomes %u\n",
	       (unsigned)value, (unsigned)(value % (VSVAL_MAX + 1)));

	value <<= 2u;
	hibytes = (value >= 0x100u) << (value >= 0x10000u);
	value |= hibytes;

	for (i = 0u; i <= hibytes; ++i)
		sf_put_u8(stream, value >> i * 8u);

	return -stream->status;
}

int sf_get_vsval(struct save_stream *restrict stream, u32 *restrict value)
{
	u8 byte = 0u;
	unsigned i;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		sf_get_u8(stream, &byte);
		*value |= byte << i * 8u;
	}

	*value >>= 2u;
	return -stream->status;
}

int sf_put_s(struct save_stream *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);
	sf_put_u16(stream, len);
	sf_write(stream, string, len);
	return -stream->status;
}

int sf_get_s(struct save_stream *restrict stream, char **restrict dest)
{
	char *string;
	u16 len;

	if (sf_get_u16(stream, &len) < 0)
		return -stream->status;

	string = malloc(len + 1);
	if (!string) {
		stream->status = S_EMEM;
		return -S_EMEM;
	}

	if (sf_read(stream, string, len) < 0) {
		free(string);
		return -stream->status;
	}

	string[len] = '\0';
	*dest = string;
	return len;
}

int sf_get_s_arr(struct save_stream *restrict stream, char **restrict array,
	int len)
{
	int array_len = 0;
	for (; array_len < len; ++array_len) {
		if (sf_get_s(stream, array + array_len) < 0)
			goto out_fail;
	}

	return 0;

out_fail:
	while (array_len--)
		free(array[array_len]);
	return -stream->status;
}

int sf_get_ns(struct save_stream *restrict stream, char *restrict dest,
	      size_t dest_size)
{
	u16 len;

	if (sf_get_u16(stream, &len) < 0)
		return -stream->status;

	if (dest_size < len + 1u) {
		stream->status = S_ESIZE;
		return -S_ESIZE;
	}

	if (sf_read(stream, dest, len) < 0)
		return -stream->status;

	dest[len] = '\0';
	return len;
}
