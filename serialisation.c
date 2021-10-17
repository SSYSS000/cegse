#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "serialisation.h"
#include "defines.h"

#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif

void serialise_u8(u8 value, struct serialiser *s)
{
	if (s->buf)
		*(u8 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

void serialise_u16(u16 value, struct serialiser *s)
{
	value = htole16(value);
	if (s->buf)
		*(u16 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

void serialise_u32(u32 value, struct serialiser *s)
{
	value = htole32(value);
	if (s->buf)
		*(u32 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

void serialise_u64(u64 value, struct serialiser *s)
{
	value = htole64(value);
	if (s->buf)
		*(u64 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

void serialise_f32(f32 value, struct serialiser *s)
{
	if (s->buf)
		*(f32 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

void serialise_vsval(u32 value, struct serialiser *s)
{
	unsigned i, hibytes;

	DWARNC(value > VSVAL_MAX, "%u becomes %u\n",
	       (unsigned)value, (unsigned)(value % (VSVAL_MAX + 1)));

	value <<= 2u;
	hibytes = (value >= 0x100u) << (value >= 0x10000u);
	value |= hibytes;

	if (s->buf) {
		for (i = 0u; i <= hibytes; ++i) {
			((u8 *)s->buf)[i] = value >> i * 8u;
		}
	}

	serialiser_add(hibytes + 1, s);
}

void serialise_bstr(const char *str, struct serialiser *s)
{
	u16 len = strlen(str);
	serialise_u16(len, s->buf);
	if (s->buf)
		memcpy(s->buf, str, len);
	serialiser_add(len, s);
}

int parse_u8(u8 *value, struct parser *p)
{
	if (p->buf_sz < sizeof(*value))
		return -1;
	*value = *(const u8 *)p->buf;
	parser_remove(sizeof(*value), p);
	return 0;
}

int parse_u16(u16 *value, struct parser *p)
{
	if (p->buf_sz < sizeof(*value))
		return -1;
	*value = le16toh(*(u16 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}


int parse_u32(u32 *value, struct parser *p)
{
	if (p->buf_sz < sizeof(*value))
		return -1;
	*value = le32toh(*(u32 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}

int parse_u64(u64 *value, struct parser *p)
{
	if (p->buf_sz < sizeof(*value))
		return -1;
	*value = le64toh(*(u64 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}

int parse_f32(f32 *value, struct parser *p)
{
	if (p->buf_sz < sizeof(*value))
		return -1;
	*value = *(f32 *)p->buf;
	parser_remove(sizeof(*value), p);
	return 0;
}

int parse_vsval(u32 *value, struct parser *p)
{
	unsigned i;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		if (i == p->buf_sz)
			return -1;
		*value |= ((u8 *)p->buf)[i] << i * 8u;
	}

	parser_remove(i, p);
	return 0;
}

int parse_bstr(char *dest, size_t n, struct parser *p)
{
	u16 bs_len, copy_len;

	if (parse_u16(&bs_len, p) == -1 || bs_len > p->buf_sz)
		return -1;

	copy_len = (n > bs_len) ? bs_len : (n - 1);
	memcpy(dest, p->buf, copy_len);
	dest[copy_len] = '\0';
	parser_remove(bs_len, p);

	DWARNC(copy_len != bs_len,
	       "truncated bstring due to insufficient buffer size\n");

	return copy_len;
}

int parse_bstr_m(char **string, struct parser *p)
{
	u16 len;

	if (parse_u16(&len, p) == -1 || len > p->buf_sz)
		return -1;

	*string = malloc(len + 1u);
	if (!*string)
		return -1;

	memcpy(*string, p->buf, len);
	(*string)[len] = '\0';
	parser_remove(len, p);
	return len;
}
