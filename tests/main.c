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
#include "tests.h"

extern struct test_function_info __start_tests;
extern struct test_function_info __stop_tests;

int main()
{
	struct test_function_info *i;
	int num_failed = 0;
	int result;

	for (i = &__start_tests; i != &__stop_tests; ++i) {
		fprintf(stderr, "%s: %s ...\n", i->file, i->name);

		result = i->test();

		if (result != TEST_SUCCESS) {
			num_failed++;
			fprintf(stderr, "%s: %s failed.\n", i->file, i->name);
		}
	}

	fprintf(stderr, "\n");

	if (num_failed == 0)
		fprintf(stderr, "All tests succeeded!\n");
	else
		fprintf(stderr, "%d tests failed.\n", num_failed);

	return 0;
}
