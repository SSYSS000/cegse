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

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include "defines.h"
#include "game_save.h"

struct game_save* game_save_new(void)
{
	return calloc(1, sizeof(struct game_save));
}

struct global_data *game_save_get_global_data(const struct game_save *save,
	enum global_data_type type)
{
	u32 i;

	for (i = 0u; i < save->num_globals; ++i) {
		if (save->globals[i].type == type)
			return save->globals + i;
	}

	return NULL;
}

void game_save_free(struct game_save *save)
{
	unsigned i;

	if (save->snapshot)
		snapshot_free(save->snapshot);

	for (i = 0u; i < save->num_plugins; ++i)
		free(save->plugins[i]);
	free(save->plugins);

	for (i = 0u; i < save->num_light_plugins; ++i)
		free(save->light_plugins[i]);
	free(save->light_plugins);

	for (i = 0u; i < save->num_globals; ++i)
		global_data_free(save->globals + i);
	free(save->globals);

	for (i = 0u; i < save->num_change_forms; ++i)
		free(save->change_forms[i].data);
	free(save->change_forms);

	free(save->form_ids);
	free(save->world_spaces);
	free(save->unknown3);

	free(save);
}
