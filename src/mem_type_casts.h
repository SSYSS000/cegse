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
 *  The as_region and as_cregion macros should then correctly choose
 *  the appropriate handler with the help of _Generic.
 */
#ifndef MEM_TYPE_CASTS_H
#define MEM_TYPE_CASTS_H

#ifndef FOR_REGION_CASTS
#define FOR_REGION_CASTS(DO)
#endif

#define EXTEND_AS_REGION(type, func) , type: func

#define as_region(v) _Generic((v),              \
    struct chunk *: region_from_chunk,          \
    struct cregion: region_from_cregion         \
    FOR_REGION_CASTS(EXTEND_AS_REGION)          \
)(v)

#define as_cregion(v) cregion_from_region(as_region(v))

#endif /* MEM_TYPE_CASTS_H */
