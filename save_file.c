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
static time_t filetime_to_time(FILETIME filetime)
{
	return (time_t)(filetime / 10000000) - 11644473600;
}

/*
 * Convert a time_t to a FILETIME.
 */
static FILETIME time_to_filetime(time_t t)
{
	return (FILETIME)(t + 11644473600) * 10000000;
}

/*
 * Compare the next num bytes in stream with data.
 *
 * If equal, return 0. If unequal or EOF is reached,
 * seek the file back to the original position and return 1.
 *
 * The size of data should not be less than num.
 *
 * On file error, return -1.
 */
static int file_compare(FILE *restrict stream,
			const void *restrict data, int num)
{
	int c;
	int i;

	for (i = 0; i < num; ++i) {
		c = fgetc(stream);
		if (c == EOF && ferror(stream))
			return -1;

		if (c != (int)((u8 *)data)[i]) {
			fseek(stream, -(i + 1), SEEK_CUR);
			return 1;
		}

	}

	return 0;
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
		if (rc == -1)
			return NULL;
		else if (rc == 0)
			return i;
	}

	return NULL;
}

int serialize_file_header(struct sf_stream *restrict stream,
			  const struct file_header *restrict header)
{
	assert(header->engine_version == stream->engine_version);
	sf_put_u32(stream, header->engine_version);
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

	if (stream->engine_version >= 12u)
		sf_put_u16(stream, header->compression_type);

	if (stream->status != SF_OK)
		return -1;

	return 0;
}

int deserialize_file_header(struct sf_stream *restrict stream,
			    struct file_header *restrict header)
{
	sf_get_u32(stream, &header->engine_version);
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

	if (header->engine_version >= 12u)
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

/*
 * Find out what type of snapshot pixel format the engine uses
 * and return it.
 */
static enum pixel_format determine_snapshot_format(int engine_version)
{
	if (engine_version >= 11)
		return PXFMT_RGBA;

	return PXFMT_RGB;
}

static int get_snapshot_size(const struct snapshot *shot)
{
	int n_pixels = shot->width * shot->height;
	switch (shot->pixel_format) {
	case PXFMT_RGB: 	return 3 * n_pixels;
	case PXFMT_RGBA: 	return 4 * n_pixels;
	default:
		assert(!"snapshot contains unknown/unhandled pixel format.");
	}
}

/*
 * Initialize a snapshot. The caller is responsible for destroying it when
 * no longer needed.
 */
static int init_snapshot(struct snapshot *shot, enum pixel_format px_format,
			 int width, int height)
{
	int shot_sz;
	shot->pixel_format = px_format;
	shot->width = width;
	shot->height = height;
	shot_sz = get_snapshot_size(shot);
	shot->pixels = malloc(shot_sz);
	if (!shot->pixels)
		return -1;

	return shot_sz;
}

/*
 * Get a pointer to the raw pixel data of the snapshot.
 */
static unsigned char *get_snapshot_data(struct snapshot *shot)
{
	return shot->pixels;
}

/*
 * Destroy a snapshot, freeing its held resources. No further actions
 * should be performed on a destroyed snapshot.
 */
static void destroy_snapshot(struct snapshot *shot)
{
	free(shot->pixels);
	shot->pixels = NULL;
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
