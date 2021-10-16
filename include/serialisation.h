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

#ifndef CEGSE_SERIALISATION_H
#define CEGSE_SERIALISATION_H

#include <sys/stat.h>
#include "types.h"

#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#define  serialise_i8(v, buf) serialise_u8((u8)(v), buf)
#define serialise_i16(v, buf) serialise_u16((u16)(v), buf)
#define serialise_i32(v, buf) serialise_u32((u32)(v), buf)
#define serialise_i64(v, buf) serialise_u64((u64)(v), buf)

#define  parse_i8(v, buf, buf_sz) parse_u8((u8 *)(v), buf, buf_sz)
#define parse_i16(v, buf, buf_sz) parse_u16((u16 *)(v), buf, buf_sz)
#define parse_i32(v, buf, buf_sz) parse_u32((u32 *)(v), buf, buf_sz)
#define parse_i64(v, buf, buf_sz) parse_u64((u64 *)(v), buf, buf_sz)

#define PARSE_SIZE_VALUE(sz_addr, buf, buf_sz) (_Generic((sz_addr)		\
	u8 *: parse_u8(sz_addr, buf, buf_sz),					\
	u16 *: parse_u16(sz_addr, buf, buf_sz),					\
	u32 *: parse_u32(sz_addr, buf, buf_sz),					\
	u64 *: parse_u64(sz_addr, buf, buf_sz)					\
))

#define NEXT_OBJECT(size_addr, obj_addr, buf, buf_sz) do {			\
	(obj_addr) = PARSE_SIZE_VALUE(size_addr, buf, buf_sz);			\
	if ((obj_addr) && *(size_addr) > ((buf_sz) - ((obj_addr) - (buf))))	\
		obj_addr = NULL;						\
} while (0)

static inline int serialise_u8(u8 value, u8 *buf)
{
	if (buf)
		*buf = value;
	return sizeof(value);
}

static inline const u8 *parse_u8(u8 *value, const u8 *buf, size_t buf_sz)
{
	if (buf_sz < sizeof(*value))
		return NULL;

	*value = *buf;
	return buf + sizeof(*value);
}

static inline int serialise_u16(u16 value, u8 *buf)
{
	value = htole16(value);
	if (buf)
		*((u16 *)buf) = value;
	return sizeof(value);
}

static inline const u8 *parse_u16(u16 *value, const u8 *buf, size_t buf_sz)
{
	if (buf_sz < sizeof(*value))
		return NULL;

	*value = le16toh(*((u16 *)buf));
	return buf + sizeof(*value);
}

static inline int serialise_u32(u32 value, u8 *buf)
{
	value = htole32(value);
	if (buf)
		*((u32 *)buf) = value;
	return sizeof(value);
}

static inline const u8 *parse_u32(u32 *value, const u8 *buf, size_t buf_sz)
{
	if (buf_sz < sizeof(*value))
		return NULL;

	*value = le32toh(*((u32 *)buf));
	return buf + sizeof(*value);
}

static inline int serialise_u64(u64 value, u8 *buf)
{
	value = htole64(value);
	if (buf)
		*((u64 *)buf) = value;
	return sizeof(value);
}

static inline const u8 *parse_u64(u64 *value, const u8 *buf, size_t buf_sz)
{
	if (buf_sz < sizeof(*value))
		return NULL;

	*value = le64toh(*((u64 *)buf));
	return buf + sizeof(*value);
}

static inline int serialise_f32(f32 value, u8 *buf)
{
	if (buf)
		*((f32 *)buf) = value;
	return sizeof(value);
}

static inline const u8 *parse_f32(f32 *value, const u8 *buf, size_t buf_sz)
{
	if (buf_sz < sizeof(*value))
		return NULL;

	*value = *((f32 *)buf);
	return buf + sizeof(*value);
}

static inline int serialise_filetime(FILETIME value, u8 *buf)
{
	return serialise_u64(value, buf);
}

static inline const u8 *
parse_filetime(FILETIME *value, const u8 *buf, size_t buf_sz)
{
	return parse_u64(value, buf, buf_sz);
}

/*
 * Write a variable-size value to stream.
 * NOTE: Values greater than VSVAL_MAX are wrapped.
 */
int serialise_vsval(u32 value, u8 *buf);

/*
 * Read a variable-size value from buf.
 */
const u8 *parse_vsval(u32 *value, const u8 *buf, size_t buf_sz);

#endif /* CEGSE_SERIALISATION_H */
