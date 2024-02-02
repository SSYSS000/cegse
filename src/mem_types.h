/*
Copyright (C) 2024  SSYSS000

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

#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include <stddef.h>

/*
 * A chunk of memory, prefixed by its size.
 */
struct chunk {
    size_t size;
    char data[];
};

/*
 * Struct that encapsulates a pointer to some non-const data and
 * the size thereof.
 */
struct region {
	void *data;
	size_t size;
};

/* Like the region struct but with const qualified data pointer. */
struct cregion {
    const void *data;
    size_t size;
};

/* 
 * Macros to help pass the data and the size of a region as arguments to
 * a function or a macro.
 */
#define REGION_AS_PARAMS(r) r.data, r.size
#define CREGION_AS_PARAMS(cr) REGION_AS_PARAMS(cr)

/* Make a new region object. */
static inline struct region make_region(void *data, size_t size)
{
    return (struct region) {
        .size = size,
        .data = data
    };
}

/* Make a new cregion object. */
static inline struct cregion make_cregion(const void *data, size_t size)
{
    return (struct cregion) {
        .size = size,
        .data = data
    };
}

/*
 * Allocate a new chunk and set the size variable.
 */
struct chunk *chunk_alloc(size_t size);

#endif /* MEM_TYPES_H */
