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

#include "tests.h"
#include "../command.c"

static int token_test(void)
{
	int i, rc = TEST_SUCCESS;

	struct {
		char line[100];
		int argc;
		const char *argv[10];
	} cases[] = {
		{"player.additem f 1000",  4, {"additem", "player", "f", "1000"}},
		{"    player.additem   F   1000", 4, {"additem", "player", "F", "1000"}},
		{"player. dditem f 1000",  5, {"", "player", "dditem", "f", "1000"}},
		{"player .additem f 1000", 5, {"player", NULL, ".additem", "f", "1000"}},
		{".additem f 1000", 4, {"additem", "", "f", "1000"}},
		{"additem f 1000",  4, {"additem", NULL, "f", "1000"}},
		{"tcl", 2, {"tcl", NULL}},
		{"", 0, {}}
	};

	char **argv;
	int argc;

	for (i = 0; i < ARRAY_SIZE(cases); ++i) {
		argc = cmd_line_tokens(cases[i].line, &argv);
		if (argc == -1) {
			ptest_error("case %d: could not split into tokens\n", i);
			rc = TEST_FAILURE;
			continue;
		}

		if (argc != cases[i].argc) {
			ptest_error(
			"case %d: invalid number of arguments. "
			"expected %d, got %d\n", i, cases[i].argc, argc);
			rc = TEST_FAILURE;
		}

		for (int j = 0; j < MIN(argc, cases[i].argc); ++j) {
			if (strcmp(argv[j] ?: "", cases[i].argv[j] ?: "") != 0) {
				ptest_error(
				"case %d: token mismatch. "
				"expected \"%s\", got \"%s\"\n",
				i, cases[i].argv[j], argv[j]);
				rc = TEST_FAILURE;
			}
		}


		free(argv);
	}

	return rc;
}
REGISTER_TEST(token_test);
