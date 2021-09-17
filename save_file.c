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
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include "defines.h"
#include "file_io.h"
#include "save_file.h"

struct game {
	enum game_title title;
	const unsigned char *magic_bytes;
	int magic_size;
};

static const unsigned char tesv_magic[] = {
	'T','E','S','V','_','S','A','V','E','G','A','M','E'};

static const unsigned char fo4_magic[] = {
	'F','O','4','_','S','A','V','E','G','A','M','E'};


#define INIT_GAME(title, magic) \
	{ title, magic, sizeof(magic) }

static const struct game supported_games[] = {
	INIT_GAME(FALLOUT4, fo4_magic),
	INIT_GAME(SKYRIM, tesv_magic),
};

#undef INIT_GAME

/*
 * Convert a FILETIME to a time_t. Note that FILETIME is more accurate
 * than time_t.
 */
static inline time_t filetime_to_time(FILETIME filetime)
{
	return (time_t)(filetime / 10000000) - 11644473600;
}

/*
 * Convert a time_t to a FILETIME.
 */
static inline FILETIME time_to_filetime(time_t t)
{
	return (FILETIME)(t + 11644473600) * 10000000;
}

/*
 * Attempts to identify the game of a game save file by reading magic bytes.
 *
 * Note that on success, the file position indicator is at the end
 * of the magic bytes.
 * If the game cannot be identified or a file error occurred, return NULL.
 * Otherwise, return a pointer to the game.
 */
static const struct game *identify_game_save_game(FILE *stream)
{
	int rc;
	const struct game *i;

	for (i = supported_games; i != ARRAY_END(supported_games); ++i) {
		rc = file_compare(stream, i->magic_bytes, i->magic_size);
		if (rc == -S_EFILE)
			return NULL;
		else if (rc == 0)
			return i;
	}

	return NULL;
}

struct game_save* create_game_save(enum game_title game_title,
				   int engine_version)
{
	struct game_save *save = calloc(1, sizeof(*save));
	if (!save)
		return NULL;

	save->game_title = game_title;
	save->engine_version = engine_version;
	return save;
}

/*
 * Builds a serializable file header of a game save.
 */
static void compose_file_header(struct file_header *restrict header,
				const struct game_save *restrict save)
{
	memset(header, 0, sizeof(*header));
	header->engine_version = save->engine_version;
	header->save_num = save->save_num;
	/* TODO: Copy player info here */
	header->filetime = time_to_filetime(save->time_saved);
	header->snapshot_width = save->snapshot.width;
	header->snapshot_height = save->snapshot.height;
}


void destroy_game_save(struct game_save *save)
{
	free(save->snapshot.pixels);

	for (int i = 0; i < save->num_plugins; ++i)
		free(save->plugins[i]);
	free(save->plugins);

	for (int i = 0; i < save->num_light_plugins; ++i)
		free(save->light_plugins[i]);
	free(save->light_plugins);

	free(save);
}
