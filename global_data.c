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
#include <stdbool.h>
#include "global_data.h"

void raw_global_free(struct raw_global *raw)
{
	free(raw->data);
}

static bool is_valid_type(enum global_data_type type)
{
	return 0 <= type && type <= 1005;
}

void global_data_free(struct global_data *gdata)
{
	if (!is_valid_type(gdata->type))
		return;
	switch (gdata->type) {
	default:
		raw_global_free(&gdata->value.raw);
	}
}
