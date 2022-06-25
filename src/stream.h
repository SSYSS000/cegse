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
#ifndef CEGSE_STREAM_H
#define CEGSE_STREAM_H

#include <stdio.h>

#include "types.h"

#define stream_write_le_i16(v, s)	stream_write_le_u16((u16)(v), s)
#define stream_write_le_i32(v, s)	stream_write_le_u32((u32)(v), s)
#define stream_write_le_i64(v, s)	stream_write_le_u64((u64)(v), s)

#define stream_read_le_i16(v, s)	stream_read_le_u16((u16 *)(v), s)
#define stream_read_le_i32(v, p)	stream_read_le_u32((u32 *)(v), p)
#define stream_read_le_i64(v, p)	stream_read_le_u64((u64 *)(v), p)

size_t stream_write_le(const void *restrict data, size_t size, FILE *restrict stream);

size_t stream_read_le(void *restrict data, size_t size, FILE *restrict stream);

size_t stream_ignore(size_t num_bytes, FILE *stream);

size_t stream_write_le_u16(u16 value, FILE *stream);

size_t stream_read_le_u16(u16 *value, FILE *stream);

size_t stream_write_le_u32(u32 value, FILE *stream);

size_t stream_read_le_u32(u32 *value, FILE *stream);

size_t stream_write_le_u64(u64 value, FILE *stream);

size_t stream_read_le_u64(u64 *value, FILE *stream);

#endif /* CEGSE_STREAM_H */
