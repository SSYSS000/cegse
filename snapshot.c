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
#include "snapshot.h"

int snapshot_size(const struct snapshot *shot)
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

struct snapshot *snapshot_new(enum pixel_format format, int width, int height)
{
	struct snapshot *shot;
	int shot_sz;
	shot = malloc(sizeof(*shot));
	if (!shot)
		return NULL;
	shot->pixel_format = format;
	shot->width = width;
	shot->height = height;
	shot_sz = snapshot_size(shot);
	shot->data = malloc(shot_sz);
	if (!shot->data) {
		free(shot);
		return NULL;
	}

	return shot;
}

void snapshot_free(struct snapshot *shot)
{
	free(shot->data);
	free(shot);
}
