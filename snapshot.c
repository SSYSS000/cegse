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

#include <assert.h>
#include <stdlib.h>
#include "types.h"
#include "snapshot.h"

enum pixel_format determine_snapshot_format(int engine_version)
{
	if (engine_version >= 11)
		return PXFMT_RGBA;

	return PXFMT_RGB;
}

int get_snapshot_size(const struct snapshot *shot)
{
	int n_pixels = shot->width * shot->height;
	switch (shot->pixel_format) {
	case PXFMT_RGB:
		return 3 * n_pixels;
	case PXFMT_RGBA:
		return 4 * n_pixels;
	default:
		assert(!"snapshot contains unknown/unhandled pixel format.");
	}
}

int init_snapshot(struct snapshot *shot, enum pixel_format px_format,
	int width, int height)
{
	int shot_sz;
	shot->pixel_format = px_format;
	shot->width = width;
	shot->height = height;
	shot_sz = get_snapshot_size(shot);
	shot->pixels = malloc(shot_sz);
	if (!shot->pixels)
		return -S_EMEM;

	return shot_sz;
}

void destroy_snapshot(struct snapshot *shot)
{
	free(shot->pixels);
	shot->pixels = NULL;
}
