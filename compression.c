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
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <lz4.h>
#include "compression.h"
#include "types.h"
#include "defines.h"

static int compress(const char *src, char *dest, int src_size, int dest_size,
	enum compressor method)
{
	int comp_size;

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

static int decompress(const char *src, char *dest, int src_size, int dest_size,
	enum compressor method)
{
	switch (method) {
	case COMPRESS_NONE:
		break;
	case COMPRESS_ZLIB:
		errno = ENOSYS;
		perror("cannot zlib decompress");
		return -1;

	case COMPRESS_LZ4:
		if (LZ4_decompress_safe(src, dest, src_size, dest_size) < 0) {
			eprintf("compress data malformed");
			return -1;
		}
		break;
	}

	return 0;
}

