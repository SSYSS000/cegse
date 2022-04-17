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
#include <errno.h>
#include <limits.h>
#include "types.h"
#include "defines.h"
#include "command.h"

static char *cut_tail(char *str, int delimiter)
{
	char *boundary = strrchr(str, delimiter);
	if (!boundary)
		return NULL;

	*boundary = '\0';
	return boundary + 1;
}

/*
 * Split a command line into tokens. This function modifies line argument.
 * Command line format: [reference.]function_name [arg...]
 *
 * The first element of tokens is the function name. The second element is
 * the the reference or NULL if it's not present.
 * The rest is function arguments.
 *
 * tokens is a pointer to a dynamically allocated (malloc) array of pointers to
 * command line arguments in line argument and must be freed with free().
 *
 * Return the number of elements in tokens.
 */
static int cmd_line_tokens(char *restrict line, char ***restrict tokens)
{
	char *save_ptr = NULL;
	char *function;
	char **_tokens;
	char *token;
	size_t cap = 8u;
	int argc;

	token = strtok_r(line, " ", &save_ptr);
	if (!token) {
		*tokens = NULL;
		return 0;
	}

	if ((_tokens = malloc(sizeof(*tokens) * cap)) == NULL)
		return -1;

	function = cut_tail(token, '.');
	if (function) {
		_tokens[0] = function;
		_tokens[1] = token;
	}
	else {
		_tokens[0] = token;
		_tokens[1] = NULL;
	}

	argc = 2;

	while ((token = strtok_r(NULL, " ", &save_ptr)) != NULL) {
		if (cap < (argc + 1)) {
			/* Double argv's capacity. */
			cap *= 2u;
			*tokens = realloc(_tokens, sizeof(_tokens) * cap);
			if (!*tokens) {
				free(_tokens);
				return -1;
			}
			_tokens = *tokens;
		}

		_tokens[argc++] = token;
	}

	*tokens = realloc(_tokens, sizeof(_tokens) * argc);
	return argc;
}

static int evaluate_ref_str(const char *str, u32 *value)
{
	unsigned long converted;
	char *end;

	errno = 0;
	converted = strtoul(str, &end, 16);

	if (converted == ULONG_MAX && errno == ERANGE) {
		eprintf("evaluation error: %s is out of range\n", str);
		return -1;
	}

	if (*end != '\0') {
		eprintf("evaluation error: %s is not a hexadecimal integer\n", str);
		return -1;
	}

	*value = converted;
	return 0;
}
