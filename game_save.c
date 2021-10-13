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

const struct game *identify_game_save_game(FILE *stream)
{
	int rc;
	const struct game *i;

	for (i = supported_games; i != ARRAY_END(supported_games); ++i) {
		rc = sf_compare(stream, i->magic_bytes, i->magic_size);
		if (rc == EOF)
			return NULL;
		if (rc == 0)
			return i;

		rewind(stream);
	}

	return NULL;
}

struct game_save* game_save_new(enum game_title game_title, int engine_version)
{
	struct game_save *save = calloc(1, sizeof(*save));
	if (!save)
		return NULL;

	save->game_title = game_title;
	save->engine_version = engine_version;
	return save;
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
	free(save);
}
