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

#endif /* CEGSE_GLOBAL_DATA_H */
