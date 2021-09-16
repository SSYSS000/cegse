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

#ifndef CEGSE_TYPES_H
#define CEGSE_TYPES_H

#include <stdint.h>

enum status_code {
	S_OK = 0,
	S_EOF,		/* End-of-file */
	S_EFILE,	/* File error */
	S_EMEM,		/* Memory allocation error */
	S_ESIZE, 	/* Size error; buffer too small or size mismatch */
	S_EMALFORMED	/* Data malformed */
};

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;

/*
 * See: https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/filetime
 */
typedef u64 FILETIME;

#endif // CEGSE_TYPES_H
