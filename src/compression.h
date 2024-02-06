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

#ifndef CEGSE_COMPRESSION_H
#define CEGSE_COMPRESSION_H

#include <sys/types.h>
#include "mem_types.h"

typedef ssize_t (*compress_fn_t)(struct cregion src, struct region dest);
typedef ssize_t (*decompress_fn_t)(struct cregion src, struct region dest);

/*
 * Return compressed size on success or -1 on failure.
 */
ssize_t lz4_compress(struct cregion src, struct region dest);
ssize_t zlib_compress(struct cregion src, struct region dest);

/*
 * Return uncompressed size on success or -1 on failure.
 */
ssize_t lz4_decompress(struct cregion src, struct region dest);
ssize_t zlib_decompress(struct cregion src, struct region dest);

#endif /* CEGSE_COMPRESSION_H */
