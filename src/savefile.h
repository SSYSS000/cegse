/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2023  SSYSS000

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
#ifndef CEGSE_CENGINE_SAVEFILE_H
#define CEGSE_CENGINE_SAVEFILE_H

#include <stdint.h>

typedef uint32_t ref_t;

enum game {
    SKYRIM,
    FALLOUT4
};

enum misc_stat_category {
    MS_GENERAL    = 0,
    MS_QUEST      = 1,
    MS_COMBAT     = 2,
    MS_MAGIC      = 3,
    MS_CRAFTING   = 4,
    MS_CRIME      = 5,
    MS_DLC        = 6
};

struct misc_stat {
    char *name;
    uint8_t category;
    int32_t value;
};

struct weather {
    ref_t climate;
    ref_t weather;
    ref_t prev_weather;
    ref_t unk_weather1;
    ref_t unk_weather2;
    ref_t regn_weather;
    float current_time;    /* Current in-game time in hours */
    float begin_time;      /* Time of current weather beginning */

    /*
     * A value from 0.0 to 1.0 describing how far in
     * the current weather has transitioned
     */
    float weather_pct;

    uint32_t data1[6];
    float data2;

     /*
      * data3 seems to affect sky colour.
      * Anything other than 2 or 3 makes the sky purple in
      * Sanctuary in Fallout.
      */
    uint32_t data3;
    uint8_t flags;

    uint32_t data4_sz;
    unsigned char *data4;    /* Only present if flags has bit 0 or 1 set. */

    /*
     * If flags & 0x1:
     * data4 {
     *     u16, (?)
     *     i8, (?)
     *     i8, (?)
     *
     *     seems to be incrementing rapidly in game.
     *     cannot be vsval or ref_id.
     *     u8[3],
     *     ...
     * }
     *
     */
};

struct player_location {
    /*
     * Number of next savegame specific object id, i.e. FFxxxxxx.
     */
    uint32_t next_object_id;

    /*
     * This form is usually 0x0 or a worldspace.
     * coord_x and coord_y represent a cell in this worldspace.
     */
    ref_t world_space1;

    int32_t coord_x;     /* x-coordinate (cell coordinates) in world_space1 */
    int32_t coord_y;     /* y-coordinate (cell coordinates) in world_space1 */

    /*
     * This can be either a worldspace or an interior cell.
     * If it's a worldspace, the player is located at the cell
     * (coord_x, coord_y). pos_x/y/z is the player's position inside
     * the cell.
     */
    ref_t world_space2;

    float pos_x;     /* x-coordinate in world_space2 */
    float pos_y;     /* y-coordinate in world_space2 */
    float pos_z;     /* z-coordinate in world_space2 */

    uint8_t unknown; /* vsval? It seems absent in 9th version */
};

struct global_variable {
    ref_t form_id;
    float value;
};

struct savegame_private;
struct savegame {
    enum game game;
    uint32_t save_num;
    char *player_name;
    uint32_t level;
    char *player_location_name;
    /* Playtime or in-game date */
    char *game_time;
    char *race_id;
    uint32_t sex;
    float current_xp;
    float target_xp;

    /* Windows API FILETIME. */
    uint64_t filetime;

    uint32_t snapshot_width;
    uint32_t snapshot_height;
    uint32_t snapshot_size;
    unsigned snapshot_bytes_per_pixel; /* 3 = RGB, 4 = RGBA. */
    unsigned char *snapshot_data;

    /*
     * The patch version of the creator of this save.
     * For Fallout 4 this might be e.g. "1.10.162.0".
     * Skyrim does not use this.
    */
    char *game_version;

    uint8_t num_plugins;
    char **plugins;

    uint16_t num_light_plugins;
    char **light_plugins;

    uint32_t num_misc_stats;
    struct misc_stat *misc_stats;
    struct player_location player_location;
    uint32_t num_global_vars;
    struct global_variable *global_vars;
    struct weather weather;

    uint32_t num_favourites;
    uint32_t num_hotkeys;
    ref_t *favourites;
    ref_t *hotkeys;

    uint32_t num_form_ids;
    uint32_t *form_ids;

    uint32_t num_world_spaces;
    uint32_t *world_spaces;

    /* Information needed to rewrite savefile properly. */
    struct savegame_private *_private;
};

/*
 * Deallocate save game memory.
 */
void savegame_free(struct savegame *save);

int cengine_savefile_write(const char *filename, const struct savegame *savegame);

struct savegame *cengine_savefile_read(const char *filename);

#endif /* CEGSE_CENGINE_SAVEFILE_H */
