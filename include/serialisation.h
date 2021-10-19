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

#ifndef CEGSE_SERIALISATION_H
#define CEGSE_SERIALISATION_H

#include "game_save.h"
#include "types.h"

struct header {
	enum game game;
	/* Creation Engine version */
	u32 	engine;
	u32 	save_num;
	char 	ply_name[PLAYER_NAME_MAX_LEN + 1];
	u32 	level;
	char 	location[64];
	/* Playtime or in-game date */
	char 	game_time[48];
	char 	race_id[48];
	u32 	sex;
	f32 	current_xp;
	f32 	target_xp;
	FILETIME filetime;
	u32 	snapshot_width;
	u32 	snapshot_height;
	u32 	compressor;
};

int parse_file_header_only(int fd, struct header *header);
int parse_file(int fd, struct game_save **out);

#endif /* CEGSE_SERIALISATION_H */
