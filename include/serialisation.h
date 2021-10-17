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

#include "types.h"

#define  serialise_i8(v, s) serialise_u8((u8)(v), s)
#define serialise_i16(v, s) serialise_u16((u16)(v), s)
#define serialise_i32(v, s) serialise_u32((u32)(v), s)
#define serialise_i64(v, s) serialise_u64((u64)(v), s)

#define  parse_i8(v, p) parse_u8((u8 *)(v), p)
#define parse_i16(v, p) parse_u16((u16 *)(v), p)
#define parse_i32(v, p) parse_u32((u32 *)(v), p)
#define parse_i64(v, p) parse_u64((u64 *)(v), p)

#define PARSE_SIZE_VALUE(sz_addr, par) (_Generic((sz_addr)			\
	u8 *: parse_u8(sz_addr,   par),						\
	u16 *: parse_u16(sz_addr, par),						\
	u32 *: parse_u32(sz_addr, par),						\
	u64 *: parse_u64(sz_addr, par)						\
))

#define NEXT_OBJECT(obj_sz, obj_addr, par) do {					\
	if (PARSE_SIZE_VALUE(&(obj_sz), par) != -1 &&				\
	    (obj_sz) > (par)->buf_sz)						\
		obj_addr = (par)->buf;						\
	else									\
		obj_addr = NULL;						\
} while (0)

struct serialiser {
	/*
	 * buf points to one byte past the end of the last serialised item.
	 * Set to NULL to find the required minimum buf size of
	 * serialisation. Refer to n_written for the size.
	 */
	void *buf;
	size_t n_written;
};

struct parser {
	const void *buf;	/* Data being parsed */
	size_t buf_sz;		/* Size of buf */
};

static inline void serialiser_add(size_t n, struct serialiser *s)
{
	if (s->buf)
		s->buf = (char *)s->buf + n;
	s->n_written += n;
}

static inline void parser_remove(size_t n, struct parser *p)
{
	p->buf = (const char *)p->buf + n;
	p->buf_sz -= n;
}

void serialise_u8(u8 value, struct serialiser *s);
void serialise_u16(u16 value, struct serialiser *s);
void serialise_u32(u32 value, struct serialiser *s);
void serialise_u64(u64 value, struct serialiser *s);
void serialise_f32(f32 value, struct serialiser *s);

/*
 * Write a variable-size value to stream.
 * NOTE: Values greater than VSVAL_MAX are wrapped.
 */
void serialise_vsval(u32 value, struct serialiser *s);


static inline void serialise_filetime(FILETIME value, struct serialiser *s)
{
	return serialise_u64(value, s);
}

void serialise_bstr(const char *str, struct serialiser *s);

int parse_u8(u8 *value, struct parser *p);
int parse_u16(u16 *value, struct parser *p);
int parse_u32(u32 *value, struct parser *p);
int parse_u64(u64 *value, struct parser *p);
int parse_f32(f32 *value, struct parser *p);

static inline int parse_filetime(FILETIME *value, struct parser *p)
{
	return parse_u64(value, p);
}

/*
 * Read a variable-size value from buf.
 */
int parse_vsval(u32 *value, struct parser *p);

int parse_bstr(char *dest, size_t n, struct parser *p);
int parse_bstr_m(char **string, struct parser *p);

#endif /* CEGSE_SERIALISATION_H */
