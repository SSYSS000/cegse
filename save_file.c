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
#include <string.h>
#include <assert.h>
#include <lz4.h>
#include <stdnoreturn.h>
#include "save_file.h"
#include "snapshot.h"

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

int file_compare(FILE *restrict stream, const void *restrict data, int num)
{
	int c;
	int i;

	for (i = 0; i < num; ++i) {
		c = fgetc(stream);
		if (c == EOF && ferror(stream))
			return -S_EFILE;

		if (c != (int)((u8 *)data)[i]) {
			fseek(stream, -(i + 1), SEEK_CUR);
			return 1;
		}

	}

	return 0;
}

/*
 * Fail save load, returning to whomever owns it.
 */
noreturn static inline
void save_load_fail(struct save_load *ctx, enum status_code error)
{
	longjmp(ctx->jmpbuf, error);
}

/*
 * Check the stream, failing save load if the stream had failed.
 */
static inline void save_load_check_stream(struct save_load *ctx)
{
	if (ctx->stream.status != S_OK)
		save_load_fail(ctx, ctx->stream.status);
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
	header->snapshot_width = save->snapshot->width;
	header->snapshot_height = save->snapshot->height;
}

/*
 * deserialize_* functions do NOT free allocated resources on failure.
 * The state of game_save shall be left in such a valid state that
 * destroy_game_save() can be called on it.
 */

/*
 * Serialize a file header to stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
 */
static void serialize_file_header(struct save_load *restrict ctx)
{
	sf_put_u32(&ctx->stream, ctx->header.engine_version);
	sf_put_u32(&ctx->stream, ctx->header.save_num);
	sf_put_s  (&ctx->stream, ctx->header.ply_name);
	sf_put_u32(&ctx->stream, ctx->header.ply_level);
	sf_put_s  (&ctx->stream, ctx->header.ply_location);
	sf_put_s  (&ctx->stream, ctx->header.game_time);
	sf_put_s  (&ctx->stream, ctx->header.ply_race_id);
	sf_put_u16(&ctx->stream, ctx->header.ply_sex);
	sf_put_f32(&ctx->stream, ctx->header.ply_current_xp);
	sf_put_f32(&ctx->stream, ctx->header.ply_target_xp);
	sf_put_filetime(&ctx->stream, ctx->header.filetime);
	sf_put_u32(&ctx->stream, ctx->header.snapshot_width);
	sf_put_u32(&ctx->stream, ctx->header.snapshot_height);

	if (ctx->header.engine_version >= 12u)
		sf_put_u16(&ctx->stream, ctx->header.compression_type);
}

/*
 * Deserialize a file header from stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
 */
static void deserialize_file_header(struct save_load *restrict ctx)
{
	struct save_stream *restrict stream = &ctx->stream;
	struct file_header *restrict header = &ctx->header;
	sf_get_u32(stream, &header->engine_version);
	sf_get_u32(stream, &header->save_num);
	sf_get_ns (stream, header->ply_name, sizeof(header->ply_name));
	sf_get_u32(stream, &header->ply_level);
	sf_get_ns (stream, header->ply_location, sizeof(header->ply_location));
	sf_get_ns (stream, header->game_time, sizeof(header->game_time));
	sf_get_ns (stream, header->ply_race_id, sizeof(header->ply_race_id));
	sf_get_u16(stream, &header->ply_sex);
	sf_get_f32(stream, &header->ply_current_xp);
	sf_get_f32(stream, &header->ply_target_xp);
	sf_get_filetime(stream, &header->filetime);
	sf_get_u32(stream, &header->snapshot_width);
	sf_get_u32(stream, &header->snapshot_height);

	if (stream->status == S_OK && header->engine_version >= 12u)
		sf_get_u16(stream, &header->compression_type);

	save_load_check_stream(ctx);
}

/*
 * Deserialize a file location table from stream. Set stream status on error.
 */
static void deserialize_file_location_table(struct save_load *restrict ctx)
{
	sf_get_u32(&ctx->stream, &ctx->locations.form_id_array_count_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.unknown_table_3_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_1_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_2_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.change_forms_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_3_offset);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_1_count);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_2_count);
	sf_get_u32(&ctx->stream, &ctx->locations.global_data_table_3_count);
	sf_get_u32(&ctx->stream, &ctx->locations.change_form_count);
	/* Skip unused. */
	fseek(ctx->stream.stream, sizeof(u32[15]), SEEK_CUR);

	save_load_check_stream(ctx);
}

static void serialize_plugins(struct save_load *restrict ctx)
{
	u32 plugins_size;
	long start_pos;
	long end_pos;
	u8 i;

	start_pos = ftell(ctx->stream.stream);
	sf_put_u32(&ctx->stream, 0u);
	sf_put_u8(&ctx->stream, ctx->save->num_plugins);
	for (i = 0u; i < ctx->save->num_plugins; ++i)
		sf_put_s(&ctx->stream, ctx->save->plugins[i]);

	end_pos = ftell(ctx->stream.stream);
	plugins_size = end_pos - start_pos - sizeof(plugins_size);

	fseek(ctx->stream.stream, start_pos, SEEK_SET);
	sf_put_u32(&ctx->stream, plugins_size);
	fseek(ctx->stream.stream, end_pos, SEEK_SET);
}

static void deserialize_plugins(struct save_load *restrict ctx)
{
	u32 plugins_size;
	u8 num_plugins;

	sf_get_u32(&ctx->stream, &plugins_size);
	sf_get_u8(&ctx->stream, &num_plugins);

	save_load_check_stream(ctx);

	ctx->save->plugins = malloc(num_plugins * sizeof(char*));
	if (!ctx->save->plugins)
		save_load_fail(ctx, S_EMEM);

	sf_get_s_arr(&ctx->stream, ctx->save->plugins, num_plugins);

	save_load_check_stream(ctx);

	ctx->save->num_plugins = num_plugins;
}

static void serialize_light_plugins(struct save_load *restrict ctx)
{
	u8 i;

	sf_put_u16(&ctx->stream, ctx->save->num_light_plugins);
	for (i = 0u; i < ctx->save->num_light_plugins; ++i)
		sf_put_s(&ctx->stream, ctx->save->light_plugins[i]);
}

static void deserialize_light_plugins(struct save_load *restrict ctx)
{
	u16 num_plugins;

	sf_get_u16(&ctx->stream, &num_plugins);
	save_load_check_stream(ctx);

	ctx->save->light_plugins = malloc(num_plugins * sizeof(char*));
	if (!ctx->save->plugins)
		save_load_fail(ctx, S_EMEM);

	sf_get_s_arr(&ctx->stream, ctx->save->light_plugins, num_plugins);

	save_load_check_stream(ctx);

	ctx->save->num_light_plugins = num_plugins;
}

static void deserialize_snapshot(struct save_load *restrict ctx)
{
	enum pixel_format px_format;
	struct snapshot *shot;
	int shot_sz;

	px_format = determine_snapshot_format(ctx->header.engine_version);
	shot = create_snapshot(px_format, ctx->header.snapshot_width,
		ctx->header.snapshot_height);
	if (!shot)
		save_load_fail(ctx, S_EMEM);
	ctx->save->snapshot = shot;

	shot_sz = get_snapshot_size(shot);
	sf_read(&ctx->stream, get_snapshot_data(shot), shot_sz);
	save_load_check_stream(ctx);
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

static void deserialize_save_data(struct save_load *restrict ctx)
{
	sf_get_u8(&ctx->stream, &ctx->format);
	ctx->save->file_format = ctx->format;

	if (ctx->header.engine_version == 11u)
		sf_get_ns(&ctx->stream, ctx->save->game_version,
			sizeof(ctx->save->game_version));

	deserialize_plugins(ctx);

	if (ctx->header.engine_version == 12u && ctx->format >= 78)
		deserialize_light_plugins(ctx);

	deserialize_file_location_table(ctx);

	/* TODO: Read the rest. */

}

static void deserialize_file_body(struct save_load *restrict ctx)
{
	u32 decompressed_size;
	u32 compressed_size;
	int rc;

	deserialize_snapshot(ctx);

	if (ctx->header.engine_version >= 12u) {
		sf_get_u32(&ctx->stream, &decompressed_size);
		sf_get_u32(&ctx->stream, &compressed_size);
		save_load_check_stream(ctx);

		ctx->compress = tmpfile();
		if (!ctx->compress) {
			perror("tmpfile");
			save_load_fail(ctx, S_EFILE);
		}
		rc = decompress_lz4(ctx->stream.stream, ctx->compress,
			compressed_size, decompressed_size);
		if (rc < 0)
			save_load_fail(ctx, -rc);
		rewind(ctx->compress);
		ctx->stream.stream = ctx->compress;
	}

	deserialize_save_data(ctx);
}

static void deserialize_file(struct save_load *restrict ctx)
{
	u32 header_sz;

	sf_get_u32(&ctx->stream, &header_sz);
	deserialize_file_header(ctx);

	ctx->save->engine_version = ctx->header.engine_version;
	ctx->save->time_saved = filetime_to_time(ctx->header.filetime);

	deserialize_file_body(ctx);
}

int load_game_save(struct game_save *restrict save, FILE *restrict stream)
{
	struct save_load sl = {0};
	int ret;

	sl.stream.stream = stream;
	sl.save = save;
	ret = setjmp(sl.jmpbuf);
	if (ret == 0)
		deserialize_file(&sl);

	if (sl.compress)
		fclose(sl.compress);

	return -ret;
}
