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
#include "compression.h"
#include "defines.h"

int cegse_compress(const void *src, void *dest, int src_size, int dest_size,
	enum compressor method)
{
	int comp_size = 0;

	switch (method) {
	case COMPRESS_NONE:
		break;
	case COMPRESS_ZLIB:
		errno = ENOSYS;
		perror("cannot zlib compress");
		return -1;
	case COMPRESS_LZ4:
		comp_size = LZ4_compress_default(src, dest, src_size,
			dest_size);
		if (comp_size == 0) {
			eprintf("compression failed");
			return -1;
		}
		break;
	}

	return comp_size;
}

int cegse_decompress(const void *src, void *dest, int src_size, int dest_size,
	enum compressor method)
{
	uLongf zdest_len = dest_size;
	int rc;

	switch (method) {
	case COMPRESS_NONE:
		eprintf("COMPRESS_NONE passed to decompress()\n");
		return -1;

	case COMPRESS_ZLIB:
		rc = uncompress(dest, &zdest_len, src, src_size);
		if (rc != Z_OK) {
			eprintf("zlib: failed to decompress\n");
			return -1;
		}
		break;

	case COMPRESS_LZ4:
		rc = LZ4_decompress_safe(src, dest, src_size, dest_size);
		if (rc < 0) {
			eprintf("lz4: compress data malformed\n");
			return -1;
		}
		break;
	}

	return 0;
}

