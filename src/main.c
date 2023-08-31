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

#include <stdio.h>
#include <stdlib.h>
#include "savefile.h"
#include "defines.h"
#include "log.h"

int main(int argc, char **argv)
{
	struct savegame *save;
	int rc;

	logging();

	if (argc < 2) {
		eprintf("usage: %s path/to/savefile\n", argv[0]);
		return EXIT_FAILURE;
	}

	save = cengine_savefile_read(argv[1]);
	if (!save) {
		eprintf("fail\n");
		return EXIT_FAILURE;
	}



	rc = cengine_savefile_write("written_savefile", save);
	if (rc == -1) {
		eprintf("failed to write file\n");
		savegame_free(save);
		return EXIT_FAILURE;
	}

	savegame_free(save);

	return EXIT_SUCCESS;
}
