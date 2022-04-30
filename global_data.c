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
#include <assert.h>
#include "global_data.h"

bool global_data_structure_is_known(enum global_data_type type)
{
	switch (type) {
	case GLOBAL_MISC_STATS:
	case GLOBAL_PLAYER_LOCATION:
	case GLOBAL_GLOBAL_VARIABLES:
	case GLOBAL_WEATHER:
	case GLOBAL_MAGIC_FAVORITES:
		return true;
	default:
		return false;
	}

	return false;
}

void global_data_free(struct global_data *g)
{
	switch (g->type) {
	case GLOBAL_MISC_STATS:
		free(g->object.stats.stats);
		break;

	case GLOBAL_GLOBAL_VARIABLES:
		free(g->object.global_vars.vars);
		break;

	case GLOBAL_WEATHER:
		free(g->object.weather.data4);
		break;

	case GLOBAL_MAGIC_FAVORITES:
		free(g->object.magic_favs.favourites);
		free(g->object.magic_favs.hotkeys);
		break;

	default:
		if (!global_data_structure_is_known(g->type)) {
			free(g->object.raw.data);
		}
	}
}
