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
#include "file_io.h"
#include "tests.h"

int float_rw(void)
{
	struct sf_stream stream = {};
	FILE *tmp = tmpfile();
	if (!tmp) {
		perror("tmpfile");
		return TEST_FAILURE;
	}

	stream.stream = tmp;
	float f;
	sf_put_f32(&stream, 50.3f);
	rewind(tmp);
	sf_get_f32(&stream, &f);

	if (f != 50.3f)
		return TEST_FAILURE;

	return TEST_SUCCESS;
}
REGISTER_TEST(float_rw);

