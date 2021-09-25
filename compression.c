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

#include <stdlib.h>
#include <lz4.h>
#include "compression.h"
#include "types.h"

int compress_lz4(FILE *restrict istream, FILE *restrict ostream,
	unsigned data_sz)
{
	int comp_bound = LZ4_compressBound(data_sz);
	char *output_buffer = malloc(comp_bound);
	char *input_buffer = malloc(data_sz);
	int comp_size;
	int ret;

	if (!output_buffer || !input_buffer) {
		ret = -S_EMEM;
		goto out_cleanup;
	}

	if (!fread(input_buffer, data_sz, 1u, istream)) {
		ret = feof(istream) ? -S_EOF : -S_EFILE;
		goto out_cleanup;
	}

	comp_size = LZ4_compress_default(input_buffer, output_buffer,
		data_sz, comp_bound);

	if (!fwrite(output_buffer, comp_size, 1u, ostream)) {
		ret = -S_EFILE;
		goto out_cleanup;
	}

	ret = comp_size;

out_cleanup:
	free(output_buffer);
	free(input_buffer);
	return ret;
}

int decompress_lz4(FILE *restrict istream, FILE *restrict ostream,
	unsigned csize, unsigned dsize)
{
	int err = 0;
	int lz4_ret;
	char *cbuf = malloc(csize);
	char *dbuf = malloc(dsize);
	if (!cbuf || !dbuf) {
		err = S_EMEM;
		goto out_cleanup;
	}

	if (!fread(cbuf, csize, 1u, istream)) {
		err = feof(istream) ? S_EOF : S_EFILE;
		goto out_cleanup;
	}

	lz4_ret = LZ4_decompress_safe(cbuf, dbuf, csize, dsize);

	if (lz4_ret < 0) {
		err = S_EMALFORMED;
		goto out_cleanup;
	}

	if (!fwrite(dbuf, dsize, 1u, ostream)) {
		err = S_EFILE;
		goto out_cleanup;
	}

out_cleanup:
	free(cbuf);
	free(dbuf);
	return -err;
}
