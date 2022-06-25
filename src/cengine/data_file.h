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

#ifndef CEGSE_CREATION_ENGINE_DATA_FILE_H
#define CEGSE_CREATION_ENGINE_DATA_FILE_H

#include "../stream.h"

/*
 * Creation Engine Data File.
 */
struct cengine_data_file {
	FILE *stream;
};

#define CENGINE_DATA_FILE(p)			((struct cengine_data_file *)(p))

size_t cedf_write_filetime(FILETIME value, struct cengine_data_file *file);

size_t cedf_read_filetime(FILETIME *value, struct cengine_data_file *file);

size_t cedf_write_wstring(const char *str, struct cengine_data_file *file);

size_t cedf_read_wstring(char *dest, size_t dest_size, struct cengine_data_file *file);

/*
 * Serialise a variable-size value.
 * NOTE: Values greater than VSVAL_MAX are wrapped.
 */
size_t cedf_write_vsval(u32 value, struct cengine_data_file *file);

size_t cedf_read_vsval(u32 *value, struct cengine_data_file *file);

size_t cedf_write_ref_id(u32 ref_id, struct cengine_data_file *file);

size_t cedf_read_ref_id(u32 *ref, struct cengine_data_file *file);

#endif /* CEGSE_CREATION_ENGINE_DATA_FILE_H */
