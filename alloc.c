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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "alloc.h"

static void die(const char *msg)
{
	write(STDERR_FILENO, msg, strlen(msg));
	abort();
}

void *xmalloc(size_t size)
{
	void *mem = malloc(size);
	if (!mem) {
		die("Out of memory\n");
	}
	return mem;
}

void *xcalloc(size_t nmemb, size_t size)
{
	void *mem = calloc(nmemb, size);
	if (!mem) {
		die("Out of memory\n");
	}
	return mem;
}

void *xrealloc(void *ptr, size_t size)
{
	void *mem = realloc(ptr, size);
	if (!mem) {
		die("Out of memory\n");
	}
	return mem;
}
