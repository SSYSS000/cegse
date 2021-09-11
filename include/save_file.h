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

#include "file_io.h"
#include "types.h"

struct file_header {
	u32 version;		/* Creation Engine version */
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
int serialize_file_header(struct sf_stream *restrict stream,
			  const struct file_header *restrict header);

/*
 * Deserialize a file header from stream.
 *
 * On success, return nonnegative integer.
 * On failure, return -1.
 */
int deserialize_file_header(struct sf_stream *restrict stream,
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
	struct sf_stream *restrict stream,
	const struct file_location_table *restrict table);

/*
 * Deserialize a file location table from stream.
 *
 * On success, return a nonnegative integer.
 * On failure, return a negative integer.
 */
int deserialize_file_location_table(struct sf_stream *restrict stream,
				    struct file_location_table *restrict table);

#endif // CEGSE_SAVE_FILE
