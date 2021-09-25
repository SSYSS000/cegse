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

#ifndef CEGSE_COMPRESSION_H
#define CEGSE_COMPRESSION_H

#include <stdio.h>

enum compression_method {
	COMPRESS_NONE,
	COMPRESS_ZLIB,
	COMPRESS_LZ4,
};

/*
 * Compress data_sz bytes from istream to ostream using LZ4 compression.
 *
 * Return the number of compressed bytes written to ostream.
 * On file error, return -S_EFILE.
 * On memory allocation error, return -S_EMEM.
 * On unexpected EOF, return -S_EOF.
 */
int compress_lz4(FILE *restrict istream, FILE *restrict ostream,
	unsigned data_sz);

/*
 * Decompress csize bytes into dsize bytes from istream to ostream
 * using LZ4 compression.
 *
 * Return 0 on success.
 * On file error, return -S_EFILE.
 * On memory allocation error, return -S_EMEM.
 * On malformed data error, return -S_EMALFORMED.
 * On unexpected EOF, return -S_EOF.
 */
int decompress_lz4(FILE *restrict istream, FILE *restrict ostream,
	unsigned csize, unsigned dsize);

#endif /* CEGSE_COMPRESSION_H */
