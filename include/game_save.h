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

#ifndef CEGSE_GAME_SAVE_H
#define CEGSE_GAME_SAVE_H

#include <time.h>
#include "types.h"
#include "snapshot.h"
#include "global_data.h"
#include "change_forms.h"

enum game {
	SKYRIM,
	FALLOUT4
};

struct game_save {
	enum game game;
	u32 engine;
	u32 file_format;
	u32 save_num;
	struct snapshot *snapshot;

	/*
	 * The patch version of the creator of this save.
	 * For Fallout 4 this might be e.g. "1.10.162.0".
	 * Skyrim does not use this.
	*/
	char game_version[24];

	/* the following strings mustn't be edited. */
	char ply_name[PLAYER_NAME_MAX_LEN + 1];
	char location[64];
	char game_time[48];
	char race_id[48];
	u32 level;
	u32 sex;
	f32 current_xp, target_xp;
	time_t time_saved;

	u32 num_plugins;
	char **plugins;

	u32 num_light_plugins;
	char **light_plugins;

	u32 num_globals;
	struct global_data *globals;

	u32 num_change_forms;
	struct change_form *change_forms;

	u32 num_form_ids;
	u32 *form_ids;

	u32 num_world_spaces;
	u32 *world_spaces;

	u32 unknown3_sz;
	void *unknown3;
};

struct global_data *game_save_get_global_data(const struct game_save *save,
	enum global_data_type type);

/*
 * Create a game save.
 *
 * Return a pointer to an initialized game save or
 * return NULL if memory allocation fails.
 */
struct game_save* game_save_new(void);

/*
 * Destroy a game save, freeing its held resources.
 *
 * Further use of the game save results in undefined behavior.
 */
void game_save_free(struct game_save *save);

#endif /* CEGSE_GAME_SAVE_H */
