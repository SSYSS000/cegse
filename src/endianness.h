/*
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
#ifndef CEGSE_ENDIAN_H
#define CEGSE_ENDIAN_H

#include <string.h>

#ifdef BSD
#include <sys/endian.h>
#ifdef __OpenBSD__
#define le16toh letoh16
#define le32toh letoh32
#define le64toh letoh64
#define be16toh betoh16
#define be32toh betoh32
#define be64toh betoh64
#endif
#else
#include <endian.h>
#endif

_Static_assert(sizeof(float) == 4, "float data type is not 4 bytes long");

/* runtime endianness check will likely be left out at compile time. */
static inline int float_is_ieee754_little_endian(void)
{
    float two_point_zero = 2.0f;
    return ((char *)&two_point_zero)[0] == 0x00 &&
           ((char *)&two_point_zero)[1] == 0x00 &&
           ((char *)&two_point_zero)[2] == 0x00 &&
           ((char *)&two_point_zero)[3] == 0x40;
}

static inline int float_is_ieee754_big_endian(void)
{
    float two_point_zero = 2.0f;
    return ((char *)&two_point_zero)[0] == 0x40 &&
           ((char *)&two_point_zero)[1] == 0x00 &&
           ((char *)&two_point_zero)[2] == 0x00 &&
           ((char *)&two_point_zero)[3] == 0x00;
}

static inline int float_is_ieee754(void)
{
    return float_is_ieee754_little_endian() || float_is_ieee754_big_endian();
}

#endif /* CEGSE_ENDIAN_H */
