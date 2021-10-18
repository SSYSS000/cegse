#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "serialisation.h"
#include "snapshot.h"
#include "defines.h"

#include "types.h"
#include "defines.h"

#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#define serialise_i8(v, s)	serialise_u8((u8)(v), s)
#define serialise_i16(v, s)	serialise_u16((u16)(v), s)
#define serialise_i32(v, s)	serialise_u32((u32)(v), s)
#define serialise_i64(v, s) 	serialise_u64((u64)(v), s)

#define parse_i8(v, p)		parse_u8((u8 *)(v), p)
#define parse_i16(v, p)		parse_u16((u16 *)(v), p)
#define parse_i32(v, p)		parse_u32((u32 *)(v), p)
#define parse_i64(v, p)		parse_u64((u64 *)(v), p)

#define PARSE_SIZE_VALUE(sz_addr, par) (_Generic((sz_addr)			\
	u8 *: parse_u8(sz_addr,   par),						\
	u16 *: parse_u16(sz_addr, par),						\
	u32 *: parse_u32(sz_addr, par),						\
	u64 *: parse_u64(sz_addr, par)						\
))

#define NEXT_OBJECT(obj_sz, obj_addr, par) do {					\
	if (PARSE_SIZE_VALUE(&(obj_sz), par) != -1 &&				\
	    (obj_sz) > (par)->buf_sz)						\
		obj_addr = (par)->buf;						\
	else									\
		obj_addr = NULL;						\
} while (0)


struct header {
	/* Creation Engine version */
	u32 	engine;
	u32 	save_num;
	char 	ply_name[PLAYER_NAME_MAX_LEN + 1];
	u32 	level;
	char 	location[64];
	/* Playtime or in-game date */
	char 	game_time[48];
	char 	race_id[48];
	u16 	sex;
	f32 	current_xp;
	f32 	target_xp;
	FILETIME filetime;
	u32 	snapshot_width;
	u32 	snapshot_height;
	u16 	compressor;
};

struct offset_table {
	u32 off_form_ids_count;
	u32 off_unknown_table;
	u32 off_globals1;
	u32 off_globals2;
	u32 off_globals3;
	u32 off_change_forms;
	u32 num_globals1;
	u32 num_globals2;
	u32 num_globals3;
	u32 num_change_form;
};

struct serialise_format {
	enum game game;
	u32 engine;
	u8 revision;
};

struct serialiser {
	/*
	 * buf points to one byte past the end of the last serialised item.
	 * Set to NULL to find the required minimum buf size of
	 * serialisation. Refer to n_written for the size.
	 */
	void *buf;
	size_t n_written;
	struct serialise_format format;
	struct offset_table offsets;
};

struct parser {
	const void *buf;	/* Data being parsed */
	size_t buf_sz;		/* Size of buf */
	struct serialise_format format;
	struct offset_table offsets;

	int snapshot_width, snapshot_height;

	/*
	 * End of data condition.
	 * Set when attempting to parse more bytes than are available.
	 */
	unsigned eod;
};

static const char *get_magic_bytes(const struct serialise_format *f)
{
	switch (f->game) {
		case SKYRIM:
			return "SKYRIM";
		case FALLOUT4:
			return "FO4_SAVEGAME";
	}
	return NULL;
}

/*
 * Check if edition is Skyrim Special Edition.
 */
static inline bool is_skse(const struct serialise_format *f)
{
	return f->game == SKYRIM && f->engine == 12u;
}

/*
 * Check if edition is Skyrim Legendary Edition.
 */
static inline bool is_skle(const struct serialise_format *f)
{
	return f->game == SKYRIM && 7u <= f->engine && f->engine <= 9u;
}

/*
 * Check if a save file is expected to be compressed.
 */
static inline bool can_use_compressor(const struct serialise_format *f)
{
	return is_skse(f);
}

static inline bool has_light_plugins(const struct serialise_format *f)
{
	return is_skse(f) && f->revision >= 78u;
}

/*
 * Find out what type of snapshot pixel format the engine uses
 * and return it.
 */
static enum pixel_format which_snapshot_format(const struct serialise_format *f)
{
	if (f->engine >= 11u)
		return PXFMT_RGBA;

	return PXFMT_RGB;
}

/*
 * Convert a FILETIME to a time_t. Note that FILETIME is more accurate
 * than time_t.
 */
static inline time_t filetime_to_time(FILETIME filetime)
{
	return (time_t)(filetime / 10000000) - 11644473600;
}

/*
 * Convert a time_t to a FILETIME.
 */
static inline FILETIME time_to_filetime(time_t t)
{
	return (FILETIME)(t + 11644473600) * 10000000;
}

static inline void serialiser_add(size_t n, struct serialiser *s)
{
	if (s->buf)
		s->buf = (char *)s->buf + n;
	s->n_written += n;
}

static void serialise_u8(u8 value, struct serialiser *s)
{
	if (s->buf)
		*(u8 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

static void serialise_u16(u16 value, struct serialiser *s)
{
	value = htole16(value);
	if (s->buf)
		*(u16 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

static void serialise_u32(u32 value, struct serialiser *s)
{
	value = htole32(value);
	if (s->buf)
		*(u32 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

static void serialise_u64(u64 value, struct serialiser *s)
{
	value = htole64(value);
	if (s->buf)
		*(u64 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

static void serialise_f32(f32 value, struct serialiser *s)
{
	if (s->buf)
		*(f32 *)s->buf = value;
	serialiser_add(sizeof(value), s);
}

static inline void serialise_filetime(FILETIME value, struct serialiser *s)
{
	return serialise_u64(value, s);
}

/*
 * Serialise a variable-size value.
 * NOTE: Values greater than VSVAL_MAX are wrapped.
 */
static void serialise_vsval(u32 value, struct serialiser *s)
{
	unsigned i, hibytes;

	DWARN_IF(value > VSVAL_MAX, "%u becomes %u\n",
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

static void serialise_bstr(const char *str, struct serialiser *s)
{
	u16 len = strlen(str);
	serialise_u16(len, s->buf);
	if (s->buf)
		memcpy(s->buf, str, len);
	serialiser_add(len, s);
}

static inline size_t start_variable_length_block(struct serialiser *s)
{
	serialiser_add(sizeof(u32), s);
	return s->n_written;
}

static void end_variable_length_block(size_t start_pos, struct serialiser *s)
{
	size_t old_n_written;
	void *old_buf;
	u32 block_len;

	if (!s->buf)
		return;

	block_len = s->n_written - start_pos;

	old_n_written = s->n_written;
	old_buf = s->buf;
	s->buf = (char*)s->buf - block_len - sizeof(block_len);
	serialise_u32(block_len, s);
	s->buf = old_buf;
	s->n_written = old_n_written;
}

static void serialise_header(const struct header *header, struct serialiser *s)
{
	serialise_u32(header->engine, s);
	serialise_u32(header->save_num, s);
	serialise_bstr(header->ply_name, s);
	serialise_u32(header->level, s);
	serialise_bstr(header->location, s);
	serialise_bstr(header->game_time, s);
	serialise_bstr(header->race_id, s);
	serialise_u16(header->sex, s);
	serialise_f32(header->current_xp, s);
	serialise_f32(header->target_xp, s);
	serialise_filetime(header->filetime, s);
	serialise_u32(header->snapshot_width, s);
	serialise_u32(header->snapshot_height, s);
	if (can_use_compressor(&s->format))
		serialise_u16(header->compressor, s);
}

static void serialise_offset_table(struct offset_table *table, struct serialiser *s)
{
	serialise_u32(table->off_form_ids_count, s);
	serialise_u32(table->off_unknown_table, s);
	serialise_u32(table->off_globals1, s);
	serialise_u32(table->off_globals2, s);
	serialise_u32(table->off_change_forms, s);
	serialise_u32(table->off_globals3, s);
	serialise_u32(table->num_globals1, s);
	serialise_u32(table->num_globals2, s);
	serialise_u32(table->num_globals3, s);
	serialise_u32(table->num_change_form, s);
	if (s->buf)
		memset(s->buf, 0, sizeof(u32[15]));
	serialiser_add(sizeof(u32[15]), s);
}

static void serialise_plugins(char **plugins, u8 count, struct serialiser *s)
{
	size_t plugs_start;
	u8 i;

	plugs_start = start_variable_length_block(s);

	serialise_u8(count, s);
	for (i = 0u; i < count; ++i)
		serialise_bstr(plugins[i], s);

	end_variable_length_block(plugs_start, s);
}

static void serialise_light_plugins(char **plugins, u16 count, struct serialiser *s)
{
	u16 i;

	serialise_u16(count, s);
	for (i = 0u; i < count; ++i)
		serialise_bstr(plugins[i], s);
}

static void serialise_snapshot(const struct snapshot *snapshot, struct serialiser *s)
{
	int shot_sz;

	shot_sz = snapshot_size(snapshot);
	if (s->buf)
		memcpy(s->buf, snapshot->data, shot_sz);
	serialiser_add(shot_sz, s);
}

#define RETURN_EOD_IF_SHORT(size, p) do {					\
	if ((p)->buf_sz < (size)) {						\
		(p)->eod = 1;							\
		return -1;							\
	}									\
} while (0)

static inline void parser_remove(size_t n, struct parser *p)
{
	n = MIN(n, p->buf_sz);
	p->buf = (const char *)p->buf + n;
	p->buf_sz -= n;
}

static int parse_u8(u8 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(*value), p);
	*value = *(const u8 *)p->buf;
	parser_remove(sizeof(*value), p);
	return 0;
}

static int parse_u16(u16 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(*value), p);
	*value = le16toh(*(u16 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}

static int parse_u32(u32 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(*value), p);
	*value = le32toh(*(u32 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}

static int parse_u64(u64 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(*value), p);
	*value = le64toh(*(u64 *)p->buf);
	parser_remove(sizeof(*value), p);
	return 0;
}

static int parse_f32(f32 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(*value), p);
	*value = *(f32 *)p->buf;
	parser_remove(sizeof(*value), p);
	return 0;
}

static inline int parse_filetime(FILETIME *value, struct parser *p)
{
	return parse_u64(value, p);
}

static int parse_vsval(u32 *value, struct parser *p)
{
	unsigned i;

	/* Read 1 to 3 bytes into value. */
	*value = 0u;
	for (i = 0u; i <= (*value & 0x3u); ++i) {
		if (i == p->buf_sz) {
			p->eod = 1;
			return -1;
		}
		*value |= ((u8 *)p->buf)[i] << i * 8u;
	}

	parser_remove(i, p);
	return 0;
}

static int parse_bstr(char *dest, size_t n, struct parser *p)
{
	u16 bs_len, copy_len;

	if (parse_u16(&bs_len, p) == -1)
		return -1;

	RETURN_EOD_IF_SHORT(bs_len, p);

	copy_len = (n > bs_len) ? bs_len : (n - 1);
	memcpy(dest, p->buf, copy_len);
	dest[copy_len] = '\0';
	parser_remove(bs_len, p);

	DWARN_IF(copy_len != bs_len,
	       "truncated bstring due to insufficient buffer size\n");

	return copy_len;
}

static int parse_bstr_m(char **string, struct parser *p)
{
	u16 len;

	if (parse_u16(&len, p) == -1)
		return -1;

	RETURN_EOD_IF_SHORT(len, p);

	*string = malloc(len + 1u);
	if (!*string)
		return -1;

	memcpy(*string, p->buf, len);
	(*string)[len] = '\0';
	parser_remove(len, p);
	return len;
}

static static int parse_header(struct header *header, struct parser *p)
{
	parse_u32(&header->engine, p);
	parse_u32(&header->save_num, p);
	parse_bstr(header->ply_name, sizeof(header->ply_name), p);
	parse_u32(&header->level, p);
	parse_bstr(header->location, sizeof(header->location), p);
	parse_bstr(header->game_time, sizeof(header->game_time), p);
	parse_bstr(header->race_id, sizeof(header->race_id), p);
	parse_u16(&header->sex, p);
	parse_f32(&header->current_xp, p);
	parse_f32(&header->target_xp, p);
	parse_filetime(&header->filetime, p);
	parse_u32(&header->snapshot_width, p);
	parse_u32(&header->snapshot_height, p);
	if (p->eod)
		return -1;
	p->format->engine = header->engine;
	if (can_use_compressor(&p->format))
		parse_u16(&header->compressor, p);
	return p->eod ? -1 : 0;
}

static int parse_offset_table(struct offset_table *table, struct parser *p)
{
	parse_u32(&table->off_form_ids_count, p);
	parse_u32(&table->off_unknown_table, p);
	parse_u32(&table->off_globals1, p);
	parse_u32(&table->off_globals2, p);
	parse_u32(&table->off_change_forms, p);
	parse_u32(&table->off_globals3, p);
	parse_u32(&table->num_globals1, p);
	parse_u32(&table->num_globals2, p);
	parse_u32(&table->num_globals3, p);
	parse_u32(&table->num_change_form, p);

	/* Skip unused. */
	parser_remove(sizeof(u32[15]), p);

	return p->eod ? -1 : 0;
}

static int parse_bstr_array(char **array, unsigned array_len, struct parser *p)
{
	unsigned i;

	for (i = 0u; i < array_len; ++i) {
		if (parse_bstr_m(array + i, p) == -1) {
			while (i)
				free(array[--i]);
			return -1;
		}
	}
	return 0;
}

static int parse_plugins(char ***array, u8 *array_len, struct parser *p)
{
	char **plugins;
	u32 block_len;
	u8 count;

	parse_u32(&block_len, p);
	parse_u8(&count, p);

	if (p->eod)
		return -1;

	plugins = malloc(count * sizeof(*plugins));
	if (!plugins)
		return -1;

	if (parse_bstr_array(plugins, count, p) == -1) {
		free(plugins);
		return -1;
	}

	*array = plugins;
	*array_len = count;
	return 0;
}

static int parse_light_plugins(char ***array, u8 *array_len, struct parser *p)
{
	char **plugins;
	u16 count;

	if (parse_u16(&count, p) == -1)
		return -1;

	plugins = malloc(count * sizeof(*plugins));
	if (!plugins)
		return -1;

	if (parse_bstr_array(plugins, count, p) == -1) {
		free(plugins);
		return -1;
	}

	*array = plugins;
	*array_len = count;
	return 0;
}

static int parse_snapshot(struct snapshot **snapshot, struct parser *p)
{
	enum pixel_format px_format;
	int shot_sz;

	px_format = which_snapshot_format(&p->format);
	*snapshot = snapshot_new(px_format, p->snapshot_width, p->snapshot_height);
	if (!*snapshot)
		return -1;

	shot_sz = snapshot_size(*snapshot);
	RETURN_EOD_IF_SHORT(shot_sz, p);
	memcpy((*snapshot)->data, p->buf, shot_sz);
	parser_remove(shot_sz, p);
	return 0;
}
