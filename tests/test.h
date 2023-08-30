/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2023  SSYSS000

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

#ifndef CEGSE_TEST_H
#define CEGSE_TEST_H

#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ASSERT assert

#ifndef eprintf
#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#endif /* CEGSE_TEST_H */
