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

#ifndef CEGSE_LOG_H
#define CEGSE_LOG_H

#include <stdio.h>

extern FILE *debug_log_file;

void logging(void);

#if !defined(DISABLE_DEBUG_LOG)
#define DEBUG_LOG(fmt, ...)                                                    \
    do {                                                                       \
        if (debug_log_file) {                                                  \
            fprintf(debug_log_file, "%s: " fmt, __func__, ##__VA_ARGS__);      \
        }                                                                      \
    } while (0)
#else
#define DEBUG_LOG(...) (void)0
#endif /* !defined(DISABLE_DEBUG_LOG) */

#endif /* CEGSE_LOG_H */
