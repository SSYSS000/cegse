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

#define VSVAL_MAX 4194303u

#define PLAYER_NAME_MAX_LEN 104

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

typedef u32 ref_t;

#define REF_TYPE(ref_id)	((ref_id) >> 22u)
#define REF_VALUE(ref_id)	((ref_id) & 0x3FFFFFu)

/*
 * Give a Reference ID value a type:
 * 0 = Index into the Form ID array
 * 1 = Regular (reference to Skyrim.esm)
 * 2 = Created (plugin index of 0xFF)
 */
#define REF_INDEX(val)		(REF_VALUE(val))
#define REF_REGULAR(val)	(REF_VALUE(val) | (1u << 22u))
#define REF_CREATED(val)	(REF_VALUE(val) | (2u << 22u))
#define REF_UNKNOWN(val)	(REF_VALUE(val) | (3u << 22u))

#define REF_IS_INDEX(ref_id)	(REF_TYPE(ref_id) == 0u)
#define REF_IS_REGULAR(ref_id)	(REF_TYPE(ref_id) == 1u)
#define REF_IS_CREATED(ref_id)	(REF_TYPE(ref_id) == 2u)
#define REF_IS_UNKNOWN(ref_id)	(REF_TYPE(ref_id) == 3u)

#endif // CEGSE_TYPES_H
