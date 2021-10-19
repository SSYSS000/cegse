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

#ifndef CEGSE_SNAPSHOT_H
#define CEGSE_SNAPSHOT_H

enum pixel_format { PXFMT_RGB, PXFMT_RGBA };

struct snapshot {
	int width;
	int height;
	enum pixel_format pixel_format;
	unsigned char data[];
};

/*
 * Calculate the size of the snapshot pixels in bytes
 * and return it.
 */
unsigned snapshot_size(const struct snapshot *shot);

/*
 * Initialize a snapshot. The caller is responsible for destroying it when
 * no longer needed.
 *
 * On memory allocation error, return NULL. Otherwise, return a pointer
 * to the snapshot.
 */
struct snapshot *snapshot_new(enum pixel_format format, int width, int height);

/*
 * Destroy a snapshot, freeing its held resources. No further actions
 * should be performed on a destroyed snapshot.
 */
void snapshot_free(struct snapshot *shot);

#endif /* CEGSE_SNAPSHOT_H */
