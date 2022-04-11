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
#include "global_data.h"

static inline void free_misc_stats(struct misc_stats *stats)
{
	free(stats->stats);
}

static inline void free_global_vars(struct global_vars *vars)
{
	free(vars->vars);
}

static inline void free_weather(struct weather *w)
{
	free(w->data4);
}

static inline void free_magic_favourites(struct magic_favourites *mf)
{
	free(mf->favourites);
	free(mf->hotkeys);
}

static inline void free_raw_global(struct raw_global *raw)
{
	free(raw->data);
}

void global_data_free(struct global_data *gdata)
{
	free_misc_stats(&gdata->stats);
	free_raw_global(&gdata->game);
	free_global_vars(&gdata->global_vars);
	free_raw_global(&gdata->created_objs);
	free_raw_global(&gdata->effects);
	free_weather(&gdata->weather);
	free_raw_global(&gdata->audio);
	free_raw_global(&gdata->sky_cells);
	free_raw_global(&gdata->unknown_9);
	free_raw_global(&gdata->unknown_10);
	free_raw_global(&gdata->unknown_11);
	free_raw_global(&gdata->process_lists);
	free_raw_global(&gdata->combat);
	free_raw_global(&gdata->interface);
	free_raw_global(&gdata->actor_causes);
	free_raw_global(&gdata->unknown_104);
	free_raw_global(&gdata->detection_man);
	free_raw_global(&gdata->location_meta);
	free_raw_global(&gdata->quest_static);
	free_raw_global(&gdata->story_teller);
	free_magic_favourites(&gdata->magic_favs);
	free_raw_global(&gdata->player_ctrls);
	free_raw_global(&gdata->story_event_man);
	free_raw_global(&gdata->ingredient_shared);
	free_raw_global(&gdata->menu_ctrls);
	free_raw_global(&gdata->menu_topic_man);
	free_raw_global(&gdata->unknown_115);
	free_raw_global(&gdata->unknown_116);
	free_raw_global(&gdata->unknown_117);
	free_raw_global(&gdata->temp_effects);
	free_raw_global(&gdata->papyrus);
	free_raw_global(&gdata->anim_objs);
	free_raw_global(&gdata->timer);
	free_raw_global(&gdata->synced_anims);
	free_raw_global(&gdata->main);
	free_raw_global(&gdata->unknown_1006);
	free_raw_global(&gdata->unknown_1007);
}
