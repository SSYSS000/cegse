/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2022  SSYSS000

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


#include <stdio.h>
#include <string.h>

#include "data_file.h"
#include "defines.h"

size_t cedf_write_filetime(FILETIME value, struct cengine_data_file *file)
{
	return stream_write_le_u64(value, file->stream);
}

size_t cedf_read_filetime(FILETIME *value, struct cengine_data_file *file)
{
	return stream_read_le_u64(value, file->stream);
}

size_t cedf_write_wstring(const char *str, struct cengine_data_file *file)
{
	u16 len = strlen(str);

	if (stream_write_le_u16(len, file->stream) != sizeof(len)) {
		return -1;
	}

	if (fwrite(str, 1u, len, file->stream) != len) {
		return -1;
	}

	return sizeof(len) + len;
}

size_t cedf_read_wstring(char *dest, size_t dest_size,
						struct cengine_data_file *file)
{
	u16 overflow_len = 0;
	u16 string_len;
	u16 copy_len;

	if (stream_read_le_u16(&string_len, file->stream) != sizeof(string_len)) {
		return -1;
	}

	if (dest_size > string_len) {
		copy_len = string_len;
	}
	else {
		copy_len = dest_size - 1;
		overflow_len = string_len - copy_len;
	}

	DWARN_IF(overflow_len > 0,
	       "truncated wstring due to insufficient destination size\n");

	if (fread(dest, 1u, copy_len, file->stream) != copy_len) {
		return -1;
	}

	dest[copy_len] = '\0';

	if (overflow_len > 0) {
		if (stream_ignore(overflow_len, file->stream) != overflow_len) {
			return -1;
		}
	}

	return sizeof(string_len) + string_len;
}

size_t cedf_write_vsval(u32 value, struct cengine_data_file *s)
{
	 /*
	  * type value  | vsval size
	  * ------------+------------
	  *     0	    |   1 byte
	  *     1	    |   2 bytes
	  *     2	    |   3 bytes
	  */
	unsigned type;
	unsigned i;
	unsigned char buffer[3];

	DWARN_IF(value > VSVAL_MAX, "%u becomes %u\n",
	       (unsigned)value, (unsigned)(value % (VSVAL_MAX + 1)));

	value <<= 2u;
	type = (value >= 0x100u) << (value >= 0x10000u);
	value |= type;

	for (i = 0u; i <= type; ++i) {
		buffer[i] = value >> i * 8u;
	}

	return fwrite(buffer, type + 1, 1u, s->stream) * (type + 1);
}

size_t cedf_read_vsval(u32 *value, struct cengine_data_file *file)
{
	 /*
	  * type value  | vsval size
	  * ------------+------------
	  *     0	    |   1 byte
	  *     1	    |   2 bytes
	  *     2	    |   3 bytes
	  */
	unsigned i;
	int byte;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		if ((byte = fgetc(file->stream) == EOF) {
			return 0;
		}

		*value |= byte << i * 8u;
	}

	*value >>= 2;
	return i;
}

size_t cedf_write_ref_id(u32 ref_id, struct cengine_data_file *file)
{
	u8 bytes[] = {
		ref_id >> 16,
		ref_id >> 8,
		ref_id
	};
	return fwrite(bytes, sizeof(bytes), 1u, file->stream) * sizeof(bytes);
}

size_t cedf_read_ref_id(u32 *ref, struct cengine_data_file *p)
{
	u8 bytes[3];

	if (!fread(bytes, sizeof(bytes), 1u, p)) {
		return 0;
	}

	*ref = bytes[0] << 16 | bytes[1] << 8 | bytes[2];
	return sizeof(bytes);
}

