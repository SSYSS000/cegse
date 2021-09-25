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
#include <stdbool.h>
#include "defines.h"
#include "save_file.h"
#include "snapshot.h"

enum compression_method {
	COMPRESS_NONE,
	COMPRESS_ZLIB,
	COMPRESS_LZ4,
};

struct file_header {
	u32 	engine_version;		/* Creation Engine version */
	u32 	save_num;
	char 	ply_name[64];		/* Player name */
	u32 	ply_level;
	char 	ply_location[128];
	char 	game_time[48];		/* Playtime or in-game date */
	char 	ply_race_id[48];
	u16 	ply_sex;
	f32 	ply_current_xp;
	f32 	ply_target_xp;
	FILETIME filetime;
	u32 	snapshot_width;
	u32 	snapshot_height;
	u16 	compression_method;
};

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

struct save_load {
	FILE 			*stream;
	jmp_buf			*loader_env;
	FILE 			*compress; /* tmpfile for (de)compression */
	struct game_save   	*save;
	struct file_location_table locations;
	u8		   	format;
	u32			engine;
	enum game_title 	title;
	enum status_code	status;
	enum compression_method	compression_method;

};

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
 * Check if Skyrim Special Edition uses _engine_ version.
 */
static inline bool is_skse(enum game_title title, unsigned engine)
{
	return title == SKYRIM && engine == 12u;
}

/*
 * Check if Skyrim Legendary Edition uses _engine_ version.
 */
static inline bool is_skle(enum game_title title, unsigned engine)
{
	return title == SKYRIM && 7u <= engine && engine <= 9u;
}

/*
 * Check if a save file is expected to be compressed.
 */
static inline bool uses_compression(enum game_title title, unsigned engine)
{
	return is_skse(title, engine);
}

static inline
bool has_light_plugins(enum game_title title, unsigned engine, unsigned format)
{
	return is_skse(title, engine) && format >= 78u;
}

/*
 * Find out what type of snapshot pixel format the engine uses
 * and return it.
 */
enum pixel_format determine_snapshot_format(unsigned engine)
{
	if (engine >= 11u)
		return PXFMT_RGBA;

	return PXFMT_RGB;
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
 * Fail save load, returning to the loader.
 */
noreturn static inline
void save_load_fail(struct save_load *ctx, enum status_code error)
{
	ctx->status = error;
	longjmp(*ctx->loader_env, 1);
}

/*
 * Check the stream, failing save load if the stream had failed.
 */
static inline void save_load_check_stream(struct save_load *ctx)
{
	if (ferror(ctx->stream))
		save_load_fail(ctx, S_EFILE);

	if (feof(ctx->stream))
		save_load_fail(ctx, S_EOF);
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
 * read_* functions do NOT free allocated resources on failure.
 * The state of game_save shall be left in such a valid state that
 * destroy_game_save() can be called on it.
 */

/*
 * Serialize a file header to stream. Set stream status on error.
 *
 * Return nonnegative integer on success or EOF on error.
 */
static int write_file_header(FILE *restrict stream,
	const struct file_header *restrict header, enum game_title title)
{
	sf_put_u32(stream, header->engine_version);
	sf_put_u32(stream, header->save_num);
	sf_put_bstring(stream, header->ply_name);
	sf_put_u32(stream, header->ply_level);
	sf_put_bstring(stream, header->ply_location);
	sf_put_bstring(stream, header->game_time);
	sf_put_bstring(stream, header->ply_race_id);
	sf_put_u16(stream, header->ply_sex);
	sf_put_f32(stream, header->ply_current_xp);
	sf_put_f32(stream, header->ply_target_xp);
	sf_put_filetime(stream, header->filetime);
	sf_put_u32(stream, header->snapshot_width);
	sf_put_u32(stream, header->snapshot_height);

	if (uses_compression(title, header->engine_version))
		sf_put_u16(stream, header->compression_method);

	return ferror(stream) ? EOF : 0;
}

/*
 * Deserialize a file header from stream.
 *
 * Return nonnegative integer on success or EOF on end of file or error.
 */
static int read_file_header(FILE *restrict stream,
	struct file_header *header, enum game_title title)
{
	DPRINT("reading file header at 0x%lx\n", ftell(stream->stream));
	sf_get_u32(stream, &header->engine_version);
	sf_get_u32(stream, &header->save_num);
	sf_get_bstring(stream, header->ply_name, sizeof(header->ply_name));
	sf_get_u32(stream, &header->ply_level);
	sf_get_bstring(stream, header->ply_location, sizeof(header->ply_location));
	sf_get_bstring(stream, header->game_time, sizeof(header->game_time));
	sf_get_bstring(stream, header->ply_race_id, sizeof(header->ply_race_id));
	sf_get_u16(stream, &header->ply_sex);
	sf_get_f32(stream, &header->ply_current_xp);
	sf_get_f32(stream, &header->ply_target_xp);
	sf_get_filetime(stream, &header->filetime);
	sf_get_u32(stream, &header->snapshot_width);
	sf_get_u32(stream, &header->snapshot_height);

	if (ferror(stream) || feof(stream))
		return EOF;

	if (uses_compression(title, header->engine_version))
		sf_get_u16(stream, &header->compression_method);

	return (ferror(stream) || feof(stream)) ? EOF : 0;
}

/*
 * Deserialize a file location table from stream.
 */
static void read_file_location_table(struct save_load *restrict ctx)
{
	unsigned i;
	DPRINT("reading file location table at 0x%lx\n", ftell(ctx->stream));
	sf_get_u32(ctx->stream, &ctx->locations.form_id_array_count_offset);
	sf_get_u32(ctx->stream, &ctx->locations.unknown_table_3_offset);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_1_offset);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_2_offset);
	sf_get_u32(ctx->stream, &ctx->locations.change_forms_offset);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_3_offset);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_1_count);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_2_count);
	sf_get_u32(ctx->stream, &ctx->locations.global_data_table_3_count);
	sf_get_u32(ctx->stream, &ctx->locations.change_form_count);

	/* Skip unused. */
	for (i = 0u; i < sizeof(u32[15]); ++i)
		fgetc(ctx->stream);

	save_load_check_stream(ctx);
}

static void write_plugins(struct save_load *restrict ctx)
{
	u32 plugins_size;
	long start_pos;
	long end_pos;
	u8 i;

	start_pos = ftell(ctx->stream);
	sf_put_u32(ctx->stream, 0u);
	sf_put_u8(ctx->stream, ctx->save->num_plugins);
	for (i = 0u; i < ctx->save->num_plugins; ++i)
		sf_put_bstring(ctx->stream, ctx->save->plugins[i]);

	end_pos = ftell(ctx->stream);
	plugins_size = end_pos - start_pos - sizeof(plugins_size);

	fseek(ctx->stream, start_pos, SEEK_SET);
	sf_put_u32(ctx->stream, plugins_size);
	fseek(ctx->stream, end_pos, SEEK_SET);
}

static void read_plugins(struct save_load *restrict ctx)
{
	u32 plugins_size;
	u8 num_plugins;
	unsigned i;

	DPRINT("reading plugin info at 0x%lx\n", ftell(ctx->stream));
	sf_get_u32(ctx->stream, &plugins_size);
	sf_get_u8(ctx->stream, &num_plugins);

	save_load_check_stream(ctx);

	ctx->save->plugins = malloc(num_plugins * sizeof(char*));
	if (!ctx->save->plugins)
		save_load_fail(ctx, S_EMEM);

	for (i = 0u; i < num_plugins; ++i) {
		ctx->save->plugins[i] = sf_malloc_bstring(ctx->stream);
		if (!ctx->save->plugins[i]) {
			save_load_check_stream(ctx);
			save_load_fail(ctx, S_EMEM);
		}

		++ctx->save->num_plugins;
	}
}

static void write_light_plugins(struct save_load *restrict ctx)
{
	unsigned i;

	sf_put_u16(ctx->stream, ctx->save->num_light_plugins);
	for (i = 0u; i < ctx->save->num_light_plugins; ++i)
		sf_put_bstring(ctx->stream, ctx->save->light_plugins[i]);
}

static void read_light_plugins(struct save_load *restrict ctx)
{
	u16 num_plugins;
	unsigned i;

	DPRINT("reading light plugin info at 0x%lx\n", ftell(ctx->stream));
	sf_get_u16(ctx->stream, &num_plugins);
	save_load_check_stream(ctx);

	ctx->save->light_plugins = malloc(num_plugins * sizeof(char*));
	if (!ctx->save->plugins)
		save_load_fail(ctx, S_EMEM);

	for (i = 0u; i < num_plugins; ++i) {
		ctx->save->light_plugins[i] = sf_malloc_bstring(ctx->stream);
		if (!ctx->save->light_plugins[i]) {
			save_load_check_stream(ctx);
			save_load_fail(ctx, S_EMEM);
		}

		++ctx->save->num_light_plugins;
	}
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

static void read_save_data(struct save_load *restrict ctx)
{
	sf_get_u8(ctx->stream, &ctx->format);
	ctx->save->file_format = ctx->format;

	if (ctx->title == FALLOUT4)
		sf_get_bstring(ctx->stream, ctx->save->game_version,
			sizeof(ctx->save->game_version));

	read_plugins(ctx);

	if (has_light_plugins(ctx->title, ctx->engine, ctx->format))
		read_light_plugins(ctx);

	read_file_location_table(ctx);

	/* TODO: Read the rest. */

}

static void read_file_body(struct save_load *restrict ctx)
{
	u32 decompressed_size;
	u32 compressed_size;
	int rc;

	if (ctx->compression_method) {
		sf_get_u32(ctx->stream, &decompressed_size);
		sf_get_u32(ctx->stream, &compressed_size);
		save_load_check_stream(ctx);

		ctx->compress = tmpfile();
		if (!ctx->compress) {
			perror("tmpfile");
			save_load_fail(ctx, S_EFILE);
		}
		rc = decompress_lz4(ctx->stream, ctx->compress,
			compressed_size, decompressed_size);
		if (rc < 0)
			save_load_fail(ctx, -rc);
		rewind(ctx->compress);
		ctx->stream = ctx->compress;
	}

	read_save_data(ctx);
}

static void read_snapshot(struct save_load *restrict ctx, int width, int height)
{
	enum pixel_format px_format;
	int shot_sz;

	px_format = determine_snapshot_format(ctx->engine);
	ctx->save->snapshot = create_snapshot(px_format, width, height);
	if (!ctx->save->snapshot)
		save_load_fail(ctx, S_EMEM);
	shot_sz = get_snapshot_size(ctx->save->snapshot);

	DPRINT("reading snapshot data at 0x%lx\n", ftell(ctx->stream));
	fread(ctx->save->snapshot->data, shot_sz, 1u, ctx->stream);
}

static void read_file(struct save_load *restrict ctx)
{
	struct file_header header;
	u32 header_sz;

	sf_get_u32(ctx->stream, &header_sz);
	read_file_header(ctx->stream, &header, ctx->title);
	save_load_check_stream(ctx);

	ctx->engine = header.engine_version;
	ctx->compression_method = header.compression_method;
	ctx->save->time_saved = filetime_to_time(header.filetime);

	read_snapshot(ctx, header.snapshot_width, header.snapshot_height);
	read_file_body(ctx);
}

int load_game_save(struct game_save *restrict save, FILE *restrict stream)
{
	jmp_buf env;
	struct save_load sl = {
		.stream = stream,
		.loader_env = &env,
		.save = save
	};

	if (!setjmp(env))
		read_file(&sl);

	if (sl.compress)
		fclose(sl.compress);

	return -sl.status;
}
