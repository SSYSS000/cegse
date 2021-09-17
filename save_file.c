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
#include "save_file.h"
#include "snapshot.h"

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

int sf_write(struct save_file *restrict stream, const void *restrict src,
	     size_t size)
{
	size_t n_written;
	if (stream->status != S_OK)
		return -stream->status;

	n_written = fwrite(src, 1u, size, stream->stream);
	if (n_written < size) {
		stream->status = S_EFILE;
	}

	return -stream->status;
}

int sf_read(struct save_file *restrict stream, void *restrict dest, size_t size)
{
	size_t n_read;
	if (stream->status != S_OK)
		return -stream->status;

	n_read = fread(dest, 1u, size, stream->stream);
	if (n_read < size) {
		if (feof(stream->stream))
			stream->status = S_EOF;
		else
			stream->status = S_EFILE;
	}

	return -stream->status;
}


int sf_put_vsval(struct save_file *stream, u32 value)
{
	u32 i;
	if (value < 0x40u)
		return sf_put_u8(stream, value << 2u);

	if (value < 0x4000u)
		return sf_put_u16(stream, value << 2u | 1u);

	value = value << 2u | 2u;
	for (i = 0u; i < 3u; ++i)
		sf_put_u8(stream, value >> i * 8u);

	return -stream->status;
}

int sf_get_vsval(struct save_file *restrict stream, u32 *restrict value)
{
	u8 byte;
	u32 i;

	/* Read 1 to 3 bytes into value. */
	i = 0u;
	*value = 0u;
	do {
		if (sf_get_u8(stream, &byte) < 0)
			return -stream->status;

		*value |= byte << i * 8u;
		i++;
	} while (i < (*value & 0x3u) + 1u);

	*value >>= 2u;
	return 0;
}

int sf_put_s(struct save_file *restrict stream, const char *restrict string)
{
	u16 len = strlen(string);
	sf_put_u16(stream, len);
	sf_write(stream, string, len);
	return -stream->status;
}

int sf_get_s(struct save_file *restrict stream, char **restrict dest)
{
	char *string;
	u16 len;

	if (sf_get_u16(stream, &len) < 0)
		return -stream->status;

	string = malloc(len + 1);
	if (!string) {
		stream->status = S_EMEM;
		return -S_EMEM;
	}

	if (sf_read(stream, string, len) < 0) {
		free(string);
		return -stream->status;
	}

	string[len] = '\0';
	*dest = string;
	return len;
}

int sf_get_ns(struct save_file *restrict stream, char *restrict dest,
	      size_t dest_size)
{
	u16 len;

	if (sf_get_u16(stream, &len) < 0)
		return -stream->status;

	if (dest_size < len + 1u) {
		stream->status = S_ESIZE;
		return -S_ESIZE;
	}

	if (sf_read(stream, dest, len) < 0)
		return -stream->status;

	dest[len] = '\0';
	return len;
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

	return -stream->status;
}

int serialize_file_location_table(struct save_file *restrict stream,
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

	/* Write unused bytes. */
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
	/* Skip unused. */
	fseek(stream->stream, sizeof(u32[15]), SEEK_CUR);

	return -stream->status;
}

int snapshot_from_stream(struct save_file *restrict istream,
	struct snapshot *restrict shot, int width, int height)
{
	enum pixel_format px_format;
	int shot_sz;

	px_format = determine_snapshot_format(istream->engine_version);
	shot_sz = init_snapshot(shot, px_format, width, height);
	if (shot_sz < 0) {
		istream->status = S_EMEM;
		return -istream->status;
	}
	if (sf_read(istream, get_snapshot_data(shot), shot_sz) < 0)
		goto out_stream_error;

	return S_OK;

out_stream_error:
	destroy_snapshot(shot);
	return -istream->status;
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
