#include <stdlib.h>
#include <string.h>
#include "serialisation.h"
#include "defines.h"

int serialise_vsval(u32 value, u8 *buf)
{
	unsigned i, hibytes;

	DWARNC(value > VSVAL_MAX, "%u becomes %u\n",
	       (unsigned)value, (unsigned)(value % (VSVAL_MAX + 1)));

	value <<= 2u;
	hibytes = (value >= 0x100u) << (value >= 0x10000u);
	value |= hibytes;

	if (buf) {
		for (i = 0u; i <= hibytes; ++i) {
			buf[i] = value >> i * 8u;
		}
	}

	return hibytes + 1;
}

const u8 *parse_vsval(u32 *value, const u8 *buf, size_t buf_sz)
{
	unsigned i;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		if (i == buf_sz)
			return NULL;
		*value |= buf[i] << i * 8u;
	}

	*value >>= 2u;
	return buf + i;
}

int serialise_bstr(const char *restrict str, u8 *buf)
{
	unsigned written;
	u16 len;

	len = strlen(str);
	written = serialise_u16(len, buf);
	if (buf)
		memcpy(buf, str, len);
	written += len;
	return written;
}

const u8 *parse_bstr(char *dest, size_t n, const u8 *buf, size_t buf_size)
{
	u16 bs_len, copy_len;
	const u8 *start;

	NEXT_OBJECT(&bs_len, start, buf, buf_size);
	if (!start)
		return NULL;

	copy_len = (n > bs_len) ? bs_len : (n - 1);
	memcpy(dest, start, copy_len);
	dest[copy_len] = '\0';

	DWARNC(copy_len != bs_len,
	       "truncated bstring due to insufficient buffer size\n");

	return start + bs_len;
}

const u8 *parse_bstr_m(char **out, const u8 *buf, size_t buf_sz)
{
	const u8 *start;
	char *string;
	u16 len;

	NEXT_OBJECT(&len, start, buf, buf_sz);
	if (!start)
		return NULL;

	string = malloc(len + 1);
	if (!string)
		return NULL;

	memcpy(string, start, len);
	string[len] = '\0';
	*out = string;
	return start + len;
}
