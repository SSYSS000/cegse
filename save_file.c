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
#include <stdlib.h>
#include "file_io.h"
#include "save_file.h"

int serialize_file_header(struct sf_stream *restrict stream,
			  const struct file_header *restrict header)
{
	sf_put_u32(stream, header->version);
	sf_put_u32(stream, header->save_num);
	sf_put_s(stream, header->ply_name);
	sf_put_u32(stream, header->ply_level);
	sf_put_s(stream, header->ply_location);
	sf_put_s(stream, header->game_time);
	sf_put_s(stream, header->ply_race_id);
	sf_put_u16(stream, header->ply_sex);
	sf_put_f32(stream, header->ply_current_xp);
	sf_put_f32(stream, header->ply_target_xp);
	sf_put_filetime(stream, header->filetime);
	sf_put_u32(stream, header->snapshot_width);
	sf_put_u32(stream, header->snapshot_height);

	if (header->version >= 12u)
		sf_put_u16(stream, header->compression_type);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}

int deserialize_file_header(struct sf_stream *restrict stream,
			    struct file_header *restrict header)
{
	sf_get_u32(stream, &header->version);
	sf_get_u32(stream, &header->save_num);
	sf_get_ns(stream, header->ply_name, sizeof(header->ply_name));
	sf_get_u32(stream, &header->ply_level);
	sf_get_ns(stream, header->ply_location, sizeof(header->ply_location));
	sf_get_ns(stream, header->game_time, sizeof(header->game_time));
	sf_get_ns(stream, header->ply_race_id, sizeof(header->ply_race_id));
	sf_get_u16(stream, &header->ply_sex);
	sf_get_f32(stream, &header->ply_current_xp);
	sf_get_f32(stream, &header->ply_target_xp);
	sf_get_filetime(stream, &header->filetime);
	sf_get_u32(stream, &header->snapshot_width);
	sf_get_u32(stream, &header->snapshot_height);

	if (header->version >= 12u)
		sf_get_u16(stream, &header->compression_type);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}

int serialize_file_location_table(
	struct sf_stream *restrict stream,
	const struct file_location_table *restrict table)
{
	sf_put_u32(stream, table->form_id_array_count_offset);
	sf_put_u32(stream, table->unknown_table_3_offset);
	sf_put_u32(stream, table->global_data_table_1_offset);
	sf_put_u32(stream, table->global_data_table_2_offset);
	sf_put_u32(stream, table->change_forms_offset);
	sf_put_u32(stream, table->global_data_table_3_offset);
	sf_put_u32(stream, table->global_data_table_1_count);
	sf_put_u32(stream, table->global_data_table_2_count);
	sf_put_u32(stream, table->global_data_table_3_count);
	sf_put_u32(stream, table->change_form_count);

	// Write unused bytes.
	for (int i = 0; i < 15; ++i)
		sf_put_u32(stream, 0u);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}

int deserialize_file_location_table(struct sf_stream *restrict stream,
				    struct file_location_table *restrict table)
{
	sf_get_u32(stream, &table->form_id_array_count_offset);
	sf_get_u32(stream, &table->unknown_table_3_offset);
	sf_get_u32(stream, &table->global_data_table_1_offset);
	sf_get_u32(stream, &table->global_data_table_2_offset);
	sf_get_u32(stream, &table->change_forms_offset);
	sf_get_u32(stream, &table->global_data_table_3_offset);
	sf_get_u32(stream, &table->global_data_table_1_count);
	sf_get_u32(stream, &table->global_data_table_2_count);
	sf_get_u32(stream, &table->global_data_table_3_count);
	sf_get_u32(stream, &table->change_form_count);

	// Skip unused.
	fseek(stream->stream, sizeof(u32[15]), SEEK_CUR);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}
