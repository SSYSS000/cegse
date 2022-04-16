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

#include <errno.h>
#include <lz4.h>
#include <zlib.h>
#include <string.h>
#include "compression.h"
#include "defines.h"

int lz4_compress(const void *src, void *dest, int src_size, int dest_size)
{
	int comp_size = LZ4_compress_default(src, dest, src_size, dest_size);
	if (comp_size == 0) {
		eprintf("lz4_compress: compression failed\n");
		return -1;
	}

	return comp_size;
}

int zlib_compress(const void *src, void *dest, int src_size, int dest_size)
{
	eprintf("zlib_compress: %s\n", strerror(ENOSYS));
	return -1;
}

int lz4_decompress(const void *src, void *dest, int src_size, int dest_size)
{
	int rc = LZ4_decompress_safe(src, dest, src_size, dest_size);
	if (rc < 0) {
		eprintf("lz4_compress: data malformed\n");
		return -1;
	}
	return rc;
}

int zlib_decompress(const void *src, void *dest, int src_size, int dest_size)
{
	uLongf zdest_len = dest_size;
	int rc = uncompress(dest, &zdest_len, src, src_size);
	if (rc != Z_OK) {
		eprintf("zlib_decompress: decompression failed\n");
		return -1;
	}

	return zdest_len;
}
