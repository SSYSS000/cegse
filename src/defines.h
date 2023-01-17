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

#ifndef CEGSE_DEFINES_H
#define CEGSE_DEFINES_H

#include <stdio.h>
#include <signal.h>

#define ARRAY_LEN(a) 	(sizeof((a)) / sizeof(*(a)))

#define MIN(a, b)	    ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))

#if !defined(NDEBUG)

/* Break debugger. */
# define DEBUG_BREAK() raise(SIGTRAP)

#else
# define DEBUG_BREAK()
#endif /* !defined(NDEBUG) */

#endif /* CEGSE_DEFINES_H */
