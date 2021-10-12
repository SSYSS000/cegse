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
	enum compression_method method)
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
	enum compression_method method)
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

int compress_sf(int infd, int outfd, int size, enum compression_method method)
{
	int comp_bound = LZ4_compressBound(size);
	char *dest = NULL, *src = NULL;
	off_t in_offset, out_offset;
	int comp_size = -1;

	if ((in_offset = lseek(infd, 0, SEEK_CUR)) == -1)
		goto out_errno;
	if ((out_offset = lseek(outfd, 0, SEEK_CUR)) == -1)
		goto out_errno;
	if (ftruncate(outfd, comp_bound + out_offset) != 0)
		goto out_errno;
	
	src = mmap(NULL, size, PROT_READ, MAP_PRIVATE, infd, in_offset);
	if (src == MAP_FAILED)
		goto out_errno;

	dest = mmap(NULL, comp_bound, PROT_WRITE, MAP_SHARED, outfd, out_offset);
	if (dest == MAP_FAILED)
		goto out_errno;
	if ((comp_size = compress(src, dest, size, comp_bound, method)) == -1)
		goto out_cleanup;
	if (msync(dest, comp_size, MS_SYNC) != 0)
		goto out_errno;

	ftruncate(outfd, comp_size + out_offset);

out_cleanup:
	if (src > (char*)NULL)
		munmap(src, size);
	if (dest > (char*)NULL)
		munmap(dest, comp_bound);
	return comp_size;
out_errno:
	perror("failed to compress savedata");
	comp_size = -1;
	goto out_cleanup;
}

int decompress_sf(int infd, int outfd, int csize, int dsize,
	enum compression_method method)
{
	char *cpa = NULL, *dpa = NULL;
	off_t in_offset, out_offset;
	int err = 0;

	if ((in_offset = lseek(infd, 0, SEEK_CUR)) == -1)
		goto out_errno;
	if ((out_offset = lseek(outfd, 0, SEEK_CUR)) == -1)
		goto out_errno;
	if (ftruncate(outfd, dsize + out_offset) != 0)
		goto out_errno;
	
	cpa = mmap(NULL, csize, PROT_READ, MAP_PRIVATE, infd, in_offset);
	if (cpa == MAP_FAILED)
		goto out_errno;

	dpa = mmap(NULL, dsize, PROT_WRITE, MAP_SHARED, outfd, out_offset);
	if (dpa == MAP_FAILED)
		goto out_errno;
	if ((err = decompress(cpa, dpa, csize, dsize, method)) != 0)
		goto out_cleanup;
	if (msync(dpa, dsize, MS_SYNC) != 0)
		goto out_errno;

out_cleanup:
	if (cpa > (char*)NULL)
		munmap(cpa, csize);
	if (dpa > (char*)NULL)
		munmap(dpa, dsize);
	return err ? -1 : 0;
out_errno:
	perror("failed to decompress savedata");
	err = errno;
	goto out_cleanup;
}
