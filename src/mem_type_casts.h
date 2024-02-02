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

/*
 * This header file lets the includer define custom conversion handlers
 * for the as_region macro. The handler must return a region struct
 * and accept the type to convert from as the sole parameter. The conversion
 * is expected to always succeed and return a region with valid member values.
 *
 * Example handler definition:
 *
 * struct custom {
 *    int *integers;
 *    int len;
 *    ...
 * };
 *
 * static struct region custom_as_region(struct custom *c)
 * {
 *     return make_region(c->integers, c->len * sizeof(int));
 * }
 *
 * To register a conversion handler, define the FOR_REGION_CASTS before
 * including this file. In the definition, call the DO macro argument
 * for each type. Below is an example how to do it for struct custom and
 * something_else_t.
 * 
 * #define FOR_REGION_CASTS(DO)                       \
 *     DO(struct custom *, custom_as_region)          \
 *     DO(something_else_t, something_else_as_region)
 *
 *  The as_region macro should then correctly choose the appropriate handler
 *  with the help of _Generic. Handlers for as_cregion are registered similarly
 *  by defining FOR_CREGION_CASTS.
 */
#ifndef MEM_TYPE_CASTS_H
#define MEM_TYPE_CASTS_H

#include "mem_types.h"

/*
 * Make a cregion from a region.
 * This adds the const qualifier to the data pointer.
 */
static inline struct cregion cregion_from_region(struct region r)
{
    return make_cregion(r.data, r.size);
}

/*
 * Make a region from a cregion.
 * This discards the const qualifier of the data pointer.
 */
static inline struct region region_from_cregion(struct cregion r)
{
    return make_region((void *)r.data, r.size);
}

/*
 * Make a region from a nonnull chunk.
 */
static inline struct region region_from_chunk(struct chunk *c)
{
    return make_region(c->data, c->size);
}

/*
 * Make a cregion from a nonnull const chunk.
 */
static inline struct cregion cregion_from_const_chunk(const struct chunk *c)
{
    return make_cregion(c->data, c->size);
}

#ifndef FOR_REGION_CASTS
#define FOR_REGION_CASTS(DO)
#endif

#ifndef FOR_CREGION_CASTS
#define FOR_CREGION_CASTS(DO)
#endif

#define FOR_REGION_CASTS_WITH_PREDEFINED(DO)                \
    DO(struct cregion, region_from_cregion)                 \
    DO(struct chunk *, region_from_chunk)                   \
    FOR_REGION_CASTS(DO)

#define FOR_CREGION_CASTS_WITH_PREDEFINED(DO)               \
    DO(struct region, cregion_from_region)                  \
    DO(struct chunk *, cregion_from_const_chunk)            \
    DO(const struct chunk *, cregion_from_const_chunk)      \
    FOR_CREGION_CASTS(DO)

#define EXTEND_AS_REGION(type, func) , type: func

#define as_region(v) _Generic((v)                           \
    FOR_REGION_CASTS_WITH_PREDEFINED(EXTEND_AS_REGION)      \
)(v)

#define as_cregion(v) _Generic((v)                          \
    FOR_CREGION_CASTS_WITH_PREDEFINED(EXTEND_AS_REGION)     \
)(v)

#endif /* MEM_TYPE_CASTS_H */
