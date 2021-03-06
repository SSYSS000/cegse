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

static unsigned calculate_size(enum pixel_format format, unsigned w, unsigned h)
{
	unsigned n_pixels = w * h;
	switch (format) {
	case PXFMT_RGB:
		return 3u * n_pixels;
	case PXFMT_RGBA:
		return 4u * n_pixels;
	}

	assert(!"snapshot contains unknown/unhandled pixel format.");
}

unsigned snapshot_size(const struct snapshot *shot)
{
	return calculate_size(shot->pixel_format, shot->width, shot->height);
}

struct snapshot *snapshot_new(enum pixel_format format, unsigned w, unsigned h)
{
	struct snapshot *shot;
	unsigned shot_sz;

	shot_sz = calculate_size(format, w, h);
	shot = malloc(sizeof(*shot) + shot_sz);
	if (!shot)
		return NULL;
	shot->pixel_format = format;
	shot->width = w;
	shot->height = h;
	return shot;
}

void snapshot_free(struct snapshot *shot)
{
	free(shot);
}
