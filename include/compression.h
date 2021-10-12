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

enum compression_method {
	COMPRESS_NONE,
	COMPRESS_ZLIB,
	COMPRESS_LZ4,
};

/*
 * Compress size bytes from file referenced by infd to file
 * referenced by outfd using LZ4 compression.
 *
 * Return the number of bytes written, or -1 on error.
 */
int compress_sf(int infd, int outfd, int size, enum compression_method method);

/*
 * Decompress csize bytes into dsize bytes from file referenced by infd to
 * file referenced by outfd using LZ4 compression.
 *
 * Return 0 on success, -1 on error.
 */
int decompress_sf(int infd, int outfd, int csize, int dsize,
	enum compression_method method);

#endif /* CEGSE_COMPRESSION_H */
