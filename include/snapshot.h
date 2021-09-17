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
	unsigned char *pixels;
};

/*
 * Find out what type of snapshot pixel format the engine uses
 * and return it.
 */
enum pixel_format determine_snapshot_format(int engine_version);

/*
 * Initialize a snapshot. The caller is responsible for destroying it when
 * no longer needed.
 *
 * On success, return the amount of memory allocated for pixel data in bytes.
 * Otherwise, return -S_EMEM.
 */
int init_snapshot(struct snapshot *shot, enum pixel_format px_format,
	int width, int height);

/*
 * Get a pointer to the raw pixel data of the snapshot.
 */
static inline unsigned char *get_snapshot_data(struct snapshot *shot)
{
	return shot->pixels;
}

/*
 * Destroy a snapshot, freeing its held resources. No further actions
 * should be performed on a destroyed snapshot.
 */
void destroy_snapshot(struct snapshot *shot);

#endif /* CEGSE_SNAPSHOT_H */
