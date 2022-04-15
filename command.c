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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "command.h"

/*
 * Split a command line into tokens. This function modifies line argument.
 * Command line format: [reference.]function_name [arg...]
 *
 * ref_str is a pointer to the referred object or NULL if the
 * command does not have one.
 *
 * argv is a pointer to a dynamically allocated (malloc) array of pointers to
 * command line arguments in line argument and must be freed with free().
 *
 * Return the number of arguments in argv.
 */
static int cmd_line_tokens(char *restrict line, char **restrict ref_str,
	char ***restrict argv)
{
	char **reargv;
	char *save_ptr = NULL;
	char *function = NULL;
	size_t argv_cap = 6u;
	int argc = 0;
	char *token;

	token = strtok_r(line, " ", &save_ptr);
	if (!token) {
		*argv = NULL;
		return 0;
	}

	if ((*argv = malloc(sizeof(*argv) * argv_cap)) == NULL)
		return -1;

	function = strchr(token, '.');
	if (function) {
		*function = '\0';
		function++;
		*ref_str = token;
	}
	else {
		function = token;
		*ref_str = NULL;
	}

	(*argv)[argc++] = function;

	while ((token = strtok_r(NULL, " ", &save_ptr)) != NULL) {
		if (argv_cap < (argc + 1)) {
			/* Double argv's capacity. */
			argv_cap *= 2u;
			reargv = realloc(*argv, sizeof(*argv) * argv_cap);
			if (!reargv) {
				free(*argv);
				return -1;
			}
			*argv = reargv;
		}

		(*argv)[argc++] = token;
	}

	*argv = realloc(*argv, sizeof(*argv) * argc);
	return argc;
}
