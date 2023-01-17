
/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2022  SSYSS000

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

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include "defines.h"
#include "savegame.h"

void savegame_free(struct savegame *save)
{
	unsigned i;

	void free_savefile_private(struct savegame_private *);

	free(save->player_name);
	free(save->player_location_name);
	free(save->game_time);
	free(save->race_id);
	free(save->snapshot_data);
	free(save->game_version);

	if (save->plugins) {
		for (i = 0u; i < save->num_plugins; ++i) {
			free(save->plugins[i]);
		}

		free(save->plugins);
	}

	if (save->light_plugins) {
		for (i = 0u; i < save->num_light_plugins; ++i) {
			free(save->light_plugins[i]);
		}

		free(save->light_plugins);
	}

	if (save->misc_stats) {
		for (i = 0; i < save->num_misc_stats; ++i) {
			free(save->misc_stats[i].name);
		}

		free(save->misc_stats);
	}

	free(save->global_vars);
	free(save->weather.data4);
	free(save->favourites);
	free(save->hotkeys);
	free(save->form_ids);
	free(save->world_spaces);
	free_savefile_private(save->_private);
	free(save);
}
