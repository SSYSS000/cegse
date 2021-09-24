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
#include "save_stream.h"
#include "defines.h"
#include "tests.h"

static int float_rw(void)
{
	struct save_stream s = { .stream = tmpfile() };
	float expected = 50.3f;
	float got;

	if (!s.stream) {
		perror("tmpfile");
		return TEST_FAILURE;
	}

	sf_put_f32(&s, expected);
	rewind(s.stream);
	sf_get_f32(&s, &got);

	if (expected != got)
		return TEST_FAILURE;

	return TEST_SUCCESS;
}
REGISTER_TEST(float_rw);

static int vsval_w(void)
{
	struct save_stream s = { .stream = tmpfile() };
	unsigned put_until = VSVAL_MAX + 2;
	unsigned i;
	u32 got;

	if (!s.stream) {
		perror("tmpfile");
		goto out_fail;
	}

	for (i = 0u; i <= put_until; ++i)
		sf_put_vsval(&s, i);

	rewind(s.stream);

	for (i = 0u; i <= put_until; ++i) {
		if (sf_get_vsval(&s, &got) < 0) {
			eprintf("sf_get_vsval failed.\n");
			goto out_fail;
		}

		if (got != (i % (VSVAL_MAX + 1))) {
			eprintf("Unexpected vsval read. Expected: %d, got: %d\n",
				i, got);
			goto out_fail;
		}
	}

	fclose(s.stream);
	return TEST_SUCCESS;
out_fail:
	if (s.stream)
		fclose(s.stream);
	return TEST_FAILURE;
}
REGISTER_TEST(vsval_w);
