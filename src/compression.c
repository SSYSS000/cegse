/*
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

#include <assert.h>
#include <string.h>
#include <errno.h>

#include <lz4.h>
#include <zlib.h>

#include "compression.h"
#include "defines.h"

ssize_t lz4_compress(struct cregion src, struct region dest)
{
    int result;

    /* Check limits. */
    if (src.size > INT_MAX) {
        src.size = INT_MAX;
    }

    if (dest.size > INT_MAX) {
        dest.size = INT_MAX;
    }

    result = LZ4_compress_default(src.data, dest.data, src.size, dest.size);

    if (result <= 0) {
        eprintf("lz4_compress: compression failed\n");
        return -1;
    }

    return result;
}

ssize_t zlib_compress(struct cregion src, struct region dest)
{
    (void)src;
    (void)dest;

    eprintf("zlib_compress: %s\n", strerror(ENOSYS));

    return -1;
}

ssize_t lz4_decompress(struct cregion src, struct region dest)
{
    int result;

    result = LZ4_decompress_safe(src.data, dest.data, src.size, dest.size);

    if (result < 0) {
        eprintf("lz4_decompress: data malformed\n");
        return -1;
    }

    return result;
}

ssize_t zlib_decompress(struct cregion src, struct region dest)
{
    uLongf zdest_len;

    zdest_len = dest.size;

    if (uncompress(dest.data, &zdest_len, src.data, src.size) != Z_OK) {
        eprintf("zlib_decompress: decompression failed\n");
        return -1;
    }

    return zdest_len;
}
