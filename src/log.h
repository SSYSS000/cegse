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

#ifndef CEGSE_LOG_H
#define CEGSE_LOG_H

#include <stdio.h>

#include "defines.h"

#define eprintf(...) 	fprintf(stderr, __VA_ARGS__)

#if !defined(NDEBUG)

# define DEBUG_LOG(...) do {                                    \
	eprintf("debug: %s: ", __func__);                           \
	eprintf(__VA_ARGS__);                                       \
} while (0)

#else
# define DEBUG_LOG(...)
#endif /* !defined(NDEBUG) */

#endif /* CEGSE_LOG_H */
