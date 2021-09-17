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

#ifndef CEGSE_FILE_IO_H
#define CEGSE_FILE_IO_H

#include <endian.h>
#include <stdio.h>
#include "types.h"
#include "snapshot.h"

struct save_file {
	FILE *stream;
	enum status_code status;
	u32 engine_version;		/* Creation Engine version */
	u8 format;
};

/*
 * Compare the next num bytes in stream with data.
 *
 * If equal, return 0. If unequal or EOF is reached,
 * seek the file back to the original position and return 1.
 *
 * The size of data should not be less than num.
 *
 * On file error, return -S_EFILE.
 */
int file_compare(FILE *restrict stream, const void *restrict data, int num);

/*
 * NOTE: All sf_* functions fail immediately if the stream has faced an error,
 * i.e. stream status != S_OK.
 */

/*
 * Write exactly size bytes from src to stream.
 *
 * On file error, set stream status to S_EFILE.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_write(struct save_file *restrict stream, const void *restrict src,
	     size_t size);

/*
 * Read exactly size bytes from stream to dest.
 *
 * On file error, set stream status to S_EFILE.
 * On EOF, set stream status to S_EOF.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_read(struct save_file *restrict stream, void *restrict dest, size_t size);

/*
 * Write an unsigned 8-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_u8(struct save_file *stream, u8 value)
{
	return sf_write(stream, &value, sizeof(value));
}

/*
 * Read an unsigned 8-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_u8(struct save_file *restrict stream,
			    u8 *restrict value)
{
	return sf_read(stream, value, sizeof(*value));
}

/*
 * Write an unsigned 16-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_u16(struct save_file *stream, u16 value)
{
	value = htole16(value);
	return sf_write(stream, &value, sizeof(value));
}

/*
 * Read an unsigned 16-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_u16(struct save_file *restrict stream,
			     u16 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le16toh(*value);
	return ret;
}

/*
 * Write an unsigned 32-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_u32(struct save_file *stream, u32 value)
{
	value = htole32(value);
	return sf_write(stream, &value, sizeof(value));
}

/*
 * Read an unsigned 32-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_u32(struct save_file *restrict stream,
			     u32 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le32toh(*value);
	return ret;
}

/*
 * Write an unsigned 64-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_u64(struct save_file *stream, u64 value)
{
	value = htole64(value);
	return sf_write(stream, &value, sizeof(value));
}

/*
 * Read an unsigned 64-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_u64(struct save_file *restrict stream,
			     u64 *restrict value)
{
	int ret = sf_read(stream, value, sizeof(*value));
	if (ret >= 0)
		*value = le64toh(*value);
	return ret;
}

/*
 * Write a signed 8-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_i8(struct save_file *stream, i8 value)
{
	return sf_put_u8(stream, (u8)value);
}

/*
 * Read a signed 8-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_i8(struct save_file *restrict stream,
			    i8 *restrict value)
{
	return sf_get_u8(stream, (u8 *)value);
}

/*
 * Write a signed 16-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_i16(struct save_file *stream, i16 value)
{
	return sf_put_u16(stream, (u16)value);
}

/*
 * Read a signed 16-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_i16(struct save_file *restrict stream,
			     i16 *restrict value)
{
	return sf_get_u16(stream, (u16 *)value);
}

/*
 * Write a signed 32-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_i32(struct save_file *stream, i32 value)
{
	return sf_put_u32(stream, (u32)value);
}

/*
 * Read a signed 32-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_i32(struct save_file *restrict stream,
			     i32 *restrict value)
{
	return sf_get_u32(stream, (u32 *)value);
}

/*
 * Write a signed 64-bit integer to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_i64(struct save_file *stream, i64 value)
{
	return sf_put_u64(stream, (u64)value);
}

/*
 * Read a signed 64-bit integer from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_i64(struct save_file *restrict stream,
			     i64 *restrict value)
{
	return sf_get_u64(stream, (u64 *)value);
}

/*
 * Write a 32-bit floating point to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_f32(struct save_file *stream, f32 value)
{
	return sf_write(stream, &value, sizeof(value));
}

/*
 * Read a 32-bit floating point from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_f32(struct save_file *restrict stream,
			     f32 *restrict value)
{
	return sf_read(stream, value, sizeof(*value));
}

/*
 * Write a FILETIME to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_put_filetime(struct save_file *stream, FILETIME value)
{
	return sf_put_u64(stream, value);
}

/*
 * Read a FILETIME from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
static inline int sf_get_filetime(struct save_file *restrict stream,
				  FILETIME *restrict value)
{
	return sf_get_u64(stream, value);
}

/*
 * Write a variable-size value to stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_vsval(struct save_file *stream, u32 value);

/*
 * Read a variable-size value from stream.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_get_vsval(struct save_file *restrict stream, u32 *restrict value);

/*
 * Write a string to stream without the terminating null-byte character,
 * prefixed by a 16-bit integer denoting its length.
 *
 * On success, return a nonnegative integer. Otherwise,
 * return the stream status negated.
 */
int sf_put_s(struct save_file *restrict stream, const char *restrict string);

/*
 * Read a string, prefixed by its 16-bit length, from stream. The caller is
 * responsible for freeing the string mallocated in dest.
 *
 * On memory allocation error, the contents of dest remain unchanged
 * and stream status is set to S_EMEM.
 *
 * On success, return the length of the string. Otherwise,
 * return the stream status negated.
 */
int sf_get_s(struct save_file *restrict stream, char **restrict dest);

/*
 * Read a string, prefixed by its 16-bit length, from stream to dest.
 *
 * If the string cannot fit to dest along with its
 * terminating null-byte (strlen+1 > dest_size), the function fails and
 * S_ESIZE is set. The contents of dest remain unchanged, too.
 *
 * On success, return the length of the string. Otherwise,
 * return the stream status negated.
 */
int sf_get_ns(struct save_file *restrict stream, char *restrict dest,
	      size_t dest_size);

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
	u16 	compression_type;
};

/*
 * Serialize a file header to stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
 */
int serialize_file_header(struct save_file *restrict stream,
	const struct file_header *restrict header);

/*
 * Deserialize a file header from stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
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
 * Serialize a file location table to stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
 */
int serialize_file_location_table(struct save_file *restrict stream,
	const struct file_location_table *restrict table);

/*
 * Deserialize a file location table from stream. Set stream status on error.
 *
 * On success, return nonnegative integer.
 */
int deserialize_file_location_table(struct save_file *restrict stream,
	struct file_location_table *restrict table);

/*
 *
 */
int snapshot_from_stream(struct save_file *restrict istream,
	struct snapshot *restrict shot, int width, int height);

#endif // CEGSE_FILE_IO_H
