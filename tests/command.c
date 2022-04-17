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

static int token_test(void)
{
	int rc = TEST_SUCCESS;
	char **tokens;
	int n_tokens;

	for (unsigned i = 0u; i < ARRAY_SIZE(cases); ++i) {
		n_tokens = cmd_line_tokens(cases[i].line, &tokens);
		if (n_tokens == -1) {
			ptest_error("case %u: could not split into tokens\n", i);
			rc = TEST_FAILURE;
			continue;
		}

		if (n_tokens != cases[i].argc) {
			ptest_error(
			"case %u: invalid number of arguments. "
			"expected %d, got %d\n", i, cases[i].argc, n_tokens);
			rc = TEST_FAILURE;
		}

		for (int j = 0; j < MIN(n_tokens, cases[i].argc); ++j) {
			if (strcmp(tokens[j] ?: "", cases[i].argv[j] ?: "") != 0) {
				ptest_error(
				"case %u: token mismatch. "
				"expected \"%s\", got \"%s\"\n",
				i, cases[i].argv[j], tokens[j]);
				rc = TEST_FAILURE;
			}
		}


		free(tokens);
	}

	return rc;
}
REGISTER_TEST(token_test);
