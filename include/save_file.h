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

#ifndef CEGSE_SAVE_FILE
#define CEGSE_SAVE_FILE

#include <time.h>
#include "file_io.h"
#include "types.h"

struct file_header {
	u32 engine_version;	/* Creation Engine version */
	u32 save_num;
	char ply_name[64];	/* Player name */
	u32 ply_level;
	char ply_location[128];
	char game_time[48];	/* Playtime or in-game date */
	char ply_race_id[48];
	u16 ply_sex;
	f32 ply_current_xp;
	f32 ply_target_xp;
	FILETIME filetime;
	u32 snapshot_width;
	u32 snapshot_height;
	u16 compression_type;
};

/*
 * Serialize a file header to stream.
 *
 * On success, return nonnegative integer.
 * On failure, return -1.
 */
int serialize_file_header(struct save_file *restrict stream,
			  const struct file_header *restrict header);

/*
 * Deserialize a file header from stream.
 *
 * On success, return nonnegative integer.
 * On failure, return -1.
 */
int deserialize_file_header(struct save_file *restrict stream,
			    struct file_header *restrict header);

struct file_location_table {
	u32 form_id_array_count_offset;
	u32 unknown_table_3_offset;
	u32 global_data_table_1_offset;
	u32 global_data_table_2_offset;
	u32 change_forms_offset;
	u32 global_data_table_3_offset;
	u32 global_data_table_1_count;
	u32 global_data_table_2_count;
	u32 global_data_table_3_count;
	u32 change_form_count;
};

/*
 * Serialize a file location table to stream.
 *
 * On success, return a nonnegative integer.
 * On failure, return a negative integer.
 */
int serialize_file_location_table(
	struct save_file *restrict stream,
	const struct file_location_table *restrict table);

/*
 * Deserialize a file location table from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, return a negative integer.
 */
int deserialize_file_location_table(struct save_file *restrict stream,
				    struct file_location_table *restrict table);

enum game_title {
	SKYRIM,
	FALLOUT4
};

enum pixel_format { PXFMT_RGB, PXFMT_RGBA };

struct snapshot {
	int width;
	int height;
	enum pixel_format pixel_format;
	unsigned char *pixels;
};

struct game_save {
	enum game_title game_title;
	int engine_version;
	int save_num;

	time_t time_saved;

	struct snapshot snapshot;

	/*
	 * The patch version of the creator of this save.
	 * For Fallout 4 this might be e.g. "1.10.162.0".
	 * Skyrim does not use this.
	*/
	char game_version[24];

	int num_plugins;
	char **plugins;

	int num_light_plugins;
	char **light_plugins;
};

/*
 * Create a game save.
 *
 * engine_version specifies the Creation Engine version
 * of the game specified by game_title.
 *
 * Return a pointer to an initialized game save or
 * return NULL if memory allocation fails.
 */
struct game_save* create_game_save(enum game_title game_title,
				   int engine_version);

/*
 * Destroy a game save, freeing its held resources.
 *
 * Further use of the game save results in undefined behavior.
 */
void destroy_game_save(struct game_save *save);

#endif // CEGSE_SAVE_FILE
