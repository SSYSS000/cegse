/*
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

#define ARRAY_LEN(a)        (sizeof((a)) / sizeof(*(a)))

#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

#define SWAP(a, b) do {                                                     \
    typeof(a) tmp = a;                                                      \
    a = b;                                                                  \
    b = tmp;                                                                \
} while (0)

/* Like printf but prints to stderr. */
#define eprintf(...)        fprintf(stderr, __VA_ARGS__)

#if !defined(NDEBUG)
# define BUG(message) do {                                                     \
                                                                               \
} while (0)
#else
# define BUG(...) (void) 0
#endif /* !defined(NDEBUG) */

#endif /* CEGSE_DEFINES_H */
