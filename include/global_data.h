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

#ifndef CEGSE_GLOBAL_DATA_H
#define CEGSE_GLOBAL_DATA_H

#include "types.h"

enum global_data_type {
	GLOBAL_MISC_STATS		= 0,
	GLOBAL_PLAYER_LOCATION		= 1,
	GLOBAL_GAME			= 2,
	GLOBAL_GLOBAL_VARIABLES		= 3,
	GLOBAL_CREATED_OBJECTS		= 4,
	GLOBAL_EFFECTS			= 5,
	GLOBAL_WEATHER			= 6,
	GLOBAL_AUDIO			= 7,
	GLOBAL_SKY_CELLS		= 8,
	GLOBAL_PROCESS_LISTS		= 100,
	GLOBAL_COMBAT			= 101,
	GLOBAL_INTERFACE		= 102,
	GLOBAL_ACTOR_CAUSES		= 103,
	GLOBAL_UNKNOWN_104		= 104,
	GLOBAL_DETECTION_MANAGER	= 105,
	GLOBAL_LOCATION_METADATA	= 106,
	GLOBAL_QUEST_STATIC_DATA	= 107,
	GLOBAL_STORYTELLER		= 108,
	GLOBAL_MAGIC_FAVORITES		= 109,
	GLOBAL_PLAYER_CONTROLS		= 110,
	GLOBAL_STORY_EVENT_MANAGER 	= 111,
	GLOBAL_INGREDIENT_SHARED	= 112,
	GLOBAL_MENU_CONTROLS		= 113,
	GLOBAL_MENU_TOPIC_MANAGER	= 114,
	GLOBAL_TEMP_EFFECTS		= 1000,
	GLOBAL_PAPYRUS			= 1001,
	GLOBAL_ANIM_OBJECTS		= 1002,
	GLOBAL_TIMER			= 1003,
	GLOBAL_SYNCHRONIZED_ANIMS 	= 1004,
	GLOBAL_MAIN			= 1005
};

enum misc_stat_category {
	MS_GENERAL	= 0,
	MS_QUEST	= 1,
	MS_COMBAT	= 2,
	MS_MAGIC	= 3,
	MS_CRAFTING	= 4,
	MS_CRIME	= 5,
	MS_DLC		= 6
};

struct misc_stat {
	enum misc_stat_category category;
	char *name;
	i32 value;
};

struct weather {
	ref_t climate;
	ref_t weather;
	ref_t prev_weather;
	ref_t unk_weather1;
	ref_t unk_weather2;
	ref_t regn_weather;
	f32 current_time;	/* Current in-game time in hours */
	f32 begin_time;		/* Time of current weather beginning */

	/*
	 * A value from 0.0 to 1.0 describing how far in
	 * the current weather has transitioned
	 */
	f32 weather_pct;

	u32 data1[6];
	f32 data2;
	u32 data3;
	u32 flags;

	u32 data4_sz;
	char *data4;	/* Only present if flags has bit 0 or 1 set. */
};

struct player_location {
	/*
	 * Number of next savegame specific object id, i.e. FFxxxxxx.
	 */
	u32 next_object_id;

	/*
	 * This form is usually 0x0 or a worldspace.
	 * coord_x and coord_y represent a cell in this worldspace.
	 */
	ref_t world_space1;

	i32 coord_x; 	/* x-coordinate (cell coordinates) in world_space1 */
	i32 coord_y; 	/* y-coordinate (cell coordinates) in world_space1 */

	/*
	 * This can be either a worldspace or an interior cell.
	 * If it's a worldspace, the player is located at the cell
	 * (coord_x, coord_y). pos_x/y/z is the player's position inside
	 * the cell.
	 */
	ref_t world_space2;

	f32 pos_x; 	/* x-coordinate in world_space2 */
	f32 pos_y; 	/* y-coordinate in world_space2 */
	f32 pos_z; 	/* z-coordinate in world_space2 */

	u32 unknown; /* vsval? It seems absent in 9th version */
};

struct global_var {
	ref_t form_id;
	f32 value;
};

struct raw_global {
	u32 data_sz;
	char *data;
};

union global_data_value {
	struct misc_stat misc_stat;
	struct weather weather;
	struct global_var global_var;
	struct player_location player_location;
	struct raw_global raw;
};

struct global_data {
	enum global_data_type type;
	union global_data_value value;
};

void raw_global_free(struct raw_global *raw);

void global_data_free(struct global_data *gdata);

#endif /* CEGSE_GLOBAL_DATA_H */
