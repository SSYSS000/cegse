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
#include <lz4.h>
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
static inline time_t filetime_to_time(FILETIME filetime)
{
	return (time_t)(filetime / 10000000) - 11644473600;
}

/*
 * Convert a time_t to a FILETIME.
 */
static inline FILETIME time_to_filetime(time_t t)
{
	return (FILETIME)(t + 11644473600) * 10000000;
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
		if (rc == -S_EFILE)
			return NULL;
		else if (rc == 0)
			return i;
	}

	return NULL;
}

int serialize_file_header(struct save_file *restrict stream,
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

	return -stream->status;
}

int deserialize_file_header(struct save_file *restrict stream,
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

	return -stream->status;
}

int serialize_file_location_table(
	struct save_file *restrict stream,
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

	return -stream->status;
}

int deserialize_file_location_table(struct save_file *restrict stream,
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

	return -stream->status;
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
 *
 * On success, return the amount of memory allocated for pixel data in bytes.
 * Otherwise, return -S_EMEM.
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
		return -S_EMEM;

	return shot_sz;
}

/*
 * Get a pointer to the raw pixel data of the snapshot.
 */
static inline unsigned char *get_snapshot_data(struct snapshot *shot)
{
	return shot->pixels;
}

/*
 * Destroy a snapshot, freeing its held resources. No further actions
 * should be performed on a destroyed snapshot.
 */
static inline void destroy_snapshot(struct snapshot *shot)
{
	free(shot->pixels);
	shot->pixels = NULL;
}

int snapshot_from_stream(struct save_file *restrict istream,
			 struct snapshot *restrict shot, int width, int height)
{
	enum pixel_format px_format;
	int shot_sz;

	px_format = determine_snapshot_format(istream->engine_version);
	shot_sz = init_snapshot(shot, px_format, width, height);
	if (shot_sz < 0)
		return shot_sz;

	if (sf_read(istream, get_snapshot_data(shot), shot_sz) < 0)
		goto out_stream_error;

	return S_OK;

out_stream_error:
	destroy_snapshot(shot);
	return -istream->status;
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

/*
 * Compress input_size bytes from istream to ostream using LZ4.
 *
 * Return the number of compressed bytes written to ostream.
 * On file error, return -S_EFILE.
 * On memory allocation error, return -S_EMEM.
 * On unexpected EOF, return -S_EOF.
 */
static int compress_lz4(FILE *restrict istream, FILE *restrict ostream,
			int input_size)
{
	int ret;
	int output_size = LZ4_compressBound(input_size);
	int comp_size;
	char *output_buffer = malloc(output_size);
	char *input_buffer = malloc(input_size);
	if (!output_buffer || !input_buffer) {
		ret = -S_EMEM;
		goto out_cleanup;
	}


	if ((int)fread(input_buffer, 1, input_size, istream) < input_size) {
		ret = feof(istream) ? -S_EOF : -S_EFILE;
		goto out_cleanup;
	}

	comp_size = LZ4_compress_default(input_buffer, output_buffer,
					 input_size, output_size);

	if ((int)fwrite(output_buffer, 1, comp_size, ostream) < comp_size) {
		ret = -S_EFILE;
		goto out_cleanup;
	}
	ret = comp_size;
out_cleanup:
	free(output_buffer);
	free(input_buffer);
	return ret;
}

/*
 * Decompress comp_size bytes into decomp_size bytes from istream to ostream
 * using LZ4.
 *
 * Return the number of decompressed bytes written to ostream.
 * On file error, return -S_EFILE.
 * On memory allocation error, return -S_EMEM.
 * On unexpected EOF, return -S_EOF.
 * On malformed data detection, return -S_EMALFORMED.
 */
static int decompress_lz4(FILE *restrict istream, FILE *restrict ostream,
			  int comp_size, int decomp_size)
{
	int ret;
	int num_decomp;
	char *comp_buf = malloc(comp_size);
	char *decomp_buf = malloc(decomp_size);
	if (!comp_buf || !decomp_buf) {
		ret = -S_EMEM;
		goto out_cleanup;
	}
	if ((int)fread(comp_buf, 1, comp_size, istream) < comp_size) {
		ret = feof(istream) ? -S_EOF : -S_EFILE;
		goto out_cleanup;
	}
	num_decomp = LZ4_decompress_safe(comp_buf, decomp_buf, comp_size,
					 decomp_size);
	if (num_decomp < 0) {
		 /* Also possible that buffer capacity was insufficient... */
		ret = -S_EMALFORMED;
		goto out_cleanup;
	}
	if ((int)fwrite(decomp_buf, 1, decomp_size, ostream) < decomp_size) {
		ret = -S_EFILE;
		goto out_cleanup;
	}
	ret = num_decomp;
out_cleanup:
	free(comp_buf);
	free(decomp_buf);
	return ret;
}
