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

enum game_title {
	SKYRIM,
	FALLOUT4
};

struct game {
	enum game_title title;
	const unsigned char *magic_bytes;
	int magic_size;
};

struct game_save {
	enum game_title game_title;
	int engine_version;
	int save_num;
	u8 file_format;

	time_t time_saved;

	struct snapshot *snapshot;

	/*
	 * The patch version of the creator of this save.
	 * For Fallout 4 this might be e.g. "1.10.162.0".
	 * Skyrim does not use this.
	*/
	char game_version[24];

	u8 num_plugins;
	char **plugins;

	u16 num_light_plugins;
	char **light_plugins;
};

/*
 * Attempts to identify the game of a game save file by reading magic bytes.
 *
 * Note that on success, the file position indicator is at the end
 * of the magic bytes.
 * If the game cannot be identified or a file error occurred, return NULL.
 * Otherwise, return a pointer to the game.
 */
const struct game *identify_game_save_game(FILE *stream);

/*
 * Create a game save.
 *
 * engine_version specifies the Creation Engine version
 * of the game specified by game_title.
 *
 * Return a pointer to an initialized game save or
 * return NULL if memory allocation fails.
 */
struct game_save* game_save_new(enum game_title game_title, int engine_version);

/*
 * Destroy a game save, freeing its held resources.
 *
 * Further use of the game save results in undefined behavior.
 */
void game_save_free(struct game_save *save);

#endif /* CEGSE_GAME_SAVE_H */
