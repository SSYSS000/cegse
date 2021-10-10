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
#include "defines.h"
#include "tests.h"
#include "compression.h"

#define UNCOMPRESSED_SZ 15000000

int compress_decompress_test(void)
{
	FILE *original = NULL, *comp = NULL, *decomp = NULL;
	int result = TEST_FAILURE;
	int comp_size, rc, i;

	original = tmpfile();
	comp = tmpfile();
	decomp = tmpfile();
	if (!original || !comp || !decomp) {
		eprintf("tmpfile failed\n");
		goto out_cleanup;
	}

	for (i = 0; i < UNCOMPRESSED_SZ; ++i)
		fputc(i % 256, original);
	if (ferror(original)) {
		eprintf("file error\n");
		goto out_cleanup;	
	}

	rewind(original);
	comp_size = compress_sf(fileno(original), fileno(comp),
		UNCOMPRESSED_SZ, COMPRESS_LZ4);
	if (comp_size == -1)
		goto out_cleanup;

	rc = decompress_sf(fileno(comp), fileno(decomp), comp_size,
		UNCOMPRESSED_SZ, COMPRESS_LZ4);
	if (rc == -1)
		goto out_cleanup;
	
	for (i = 0; i < UNCOMPRESSED_SZ; ++i) {
		if (fgetc(decomp) != (i % 256)) {
			eprintf("invalid data detected.");
			goto out_cleanup;
		}
	}

	result = TEST_SUCCESS;
out_cleanup:
	if (original)
		fclose(original);
	if (comp)
		fclose(comp);
	if (decomp)
		fclose(decomp);
	return result;
}
REGISTER_TEST(compress_decompress_test);

