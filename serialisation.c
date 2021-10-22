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
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "serialisation.h"
#include "compression.h"
#include "snapshot.h"
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

#define parse_i8(v, p)		parse_u8((u32 *)(v), p)
#define parse_i16(v, p)		parse_u16((u32 *)(v), p)
#define parse_i32(v, p)		parse_u32((u32 *)(v), p)
#define parse_i64(v, p)		parse_u64((u64 *)(v), p)

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

struct file_context {
	struct header header;
	struct offset_table offsets;
	enum game *game;	/* points to header.game */
	u32 *engine;		/* points to header.engine */
	u32 revision;
};

struct serialiser {
	struct file_context *ctx;
	/*
	 * buf points to one byte past the end of the last serialised item.
	 * Set to NULL to find the required minimum buf size of
	 * serialisation. Refer to n_written for the size.
	 */
	void *buf;		/* Pointer to the next item */
	size_t n_written;	/* Bytes written */
	size_t offset;		/* File offset */
};

struct parser {
	struct file_context *ctx;

	/*
	 * sp_data points to a decompressed data block that a sub parser
	 * accesses. It must be freed when the sub parser goes out of scope.
	 * For the top level parser this is NULL.
	 */
	void *sp_data;

	const void *buf;	/* Pointer to the next item for parsing */
	size_t buf_sz;		/* The number of bytes remaining in buf */
	size_t offset;		/* File offset */

	/*
	 * End of data condition.
	 * Set when attempting to parse more bytes than are available.
	 */
	unsigned eod;
};

static const char skyrim_signature[] =
{'T','E','S','V','_','S','A','V','E','G','A','M','E'};

static const char fallout4_signature[] =
{'F','O','4','_','S','A','V','E','G','A','M','E'};

/*
 * Check if edition is Skyrim Special Edition.
 */
static inline bool is_skse(const struct file_context *f)
{
	return *f->game == SKYRIM && *f->engine == 12u;
}

/*
 * Check if edition is Skyrim Legendary Edition.
 */
static inline bool is_skle(const struct file_context *f)
{
	return *f->game == SKYRIM && 7u <= *f->engine && *f->engine <= 9u;
}

/*
 * Check if a save file is expected to be compressed.
 */
static inline bool can_use_compressor(const struct file_context *f)
{
	return is_skse(f);
}

static inline bool has_light_plugins(const struct file_context *f)
{
	return *f->game == FALLOUT4 || (is_skse(f) && f->revision >= 78u);
}

/*
 * Find out what type of snapshot pixel format the engine uses
 * and return it.
 */
static enum pixel_format which_snapshot_format(const struct file_context *f)
{
	if (*f->engine >= 11u)
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

static void print_offset_table(const struct offset_table *t)
{
	eprintf("%08x: Globals 1 (%u)\n", t->off_globals1, t->num_globals1);
	eprintf("%08x: Globals 2 (%u)\n", t->off_globals2, t->num_globals2);
	eprintf("%08x: Change forms (%u)\n", t->off_change_forms, t->num_change_form);
	eprintf("%08x: Globals 3 (%u)\n", t->off_globals3, t->num_globals3);
	eprintf("%08x: Form IDs\n", t->off_form_ids_count);
	eprintf("%08x: Unknown table\n", t->off_unknown_table);
}

static void init_blank_file_ctx(struct file_context *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->game = &ctx->header.game;
	ctx->engine = &ctx->header.engine;
}

static void init_file_ctx(const struct game_save *save, struct file_context *ctx)
{
	init_blank_file_ctx(ctx);
	*ctx->game = save->game;
	*ctx->engine = save->engine;
	ctx->revision = save->file_format;
	ctx->header.save_num = save->save_num;
	ctx->header.level = save->level;
	ctx->header.sex = save->sex;
	ctx->header.current_xp = save->current_xp;
	ctx->header.target_xp = save->target_xp;
	ctx->header.filetime = time_to_filetime(save->time_saved);
	ctx->header.snapshot_width = save->snapshot->width;
	ctx->header.snapshot_height = save->snapshot->height;
	strcpy(ctx->header.ply_name, save->ply_name);
	strcpy(ctx->header.location, save->location);
	strcpy(ctx->header.game_time, save->game_time);
	strcpy(ctx->header.race_id, save->race_id);

	switch (save->game) {
	case SKYRIM:
		ctx->offsets.num_globals1 = 9;
		ctx->offsets.num_globals2 = 14;
		ctx->offsets.num_globals3 = 5;
		break;
	case FALLOUT4:
		ctx->offsets.num_globals1 = 12;
		ctx->offsets.num_globals2 = 14;
		ctx->offsets.num_globals3 = 8;
		break;
	}

	ctx->offsets.num_change_form = save->num_change_forms;

	if (can_use_compressor(ctx))
		ctx->header.compressor = COMPRESS_LZ4;
}

static void init_serialiser(void *data, size_t file_offset,
	struct file_context *ctx, struct serialiser *s)
{
	memset(s, 0, sizeof(*s));
	s->buf = data;
	s->offset = file_offset;
	s->ctx = ctx;
}

static inline void serialiser_add(ssize_t n, struct serialiser *s)
{
	if (s->buf)
		s->buf = (char *)s->buf + n;
	s->n_written += n;
	s->offset += n;
}

static void serialiser_copy(const void *data, size_t len, struct serialiser *s)
{
	if (s->buf)
		memcpy(s->buf, data, len);
	serialiser_add(len, s);
}

static void serialise_u8(u32 value, struct serialiser *s)
{
	if (s->buf)
		*(u8 *)s->buf = value;
	serialiser_add(sizeof(u8), s);
}

static void serialise_u16(u32 value, struct serialiser *s)
{
	u16 swapped = htole16(value);
	if (s->buf)
		*(u16 *)s->buf = swapped;
	serialiser_add(sizeof(swapped), s);
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

static void serialise_ref_id(u32 ref_id, struct serialiser *s)
{
	u8 bytes[3];
	bytes[0] = ref_id;
	bytes[1] = ref_id >> 8;
	bytes[2] = ref_id >> 16;
	serialiser_copy(bytes, sizeof(bytes), s);
}

static void serialise_bstr(const char *str, struct serialiser *s)
{
	u16 len = strlen(str);
	serialise_u16(len, s);
	if (s->buf)
		memcpy(s->buf, str, len);
	serialiser_add(len, s);
}

static inline size_t start_variable_length_block(struct serialiser *s)
{
	serialiser_add(sizeof(u32), s);
	return s->offset;
}

static void end_variable_length_block(size_t start_pos, struct serialiser *s)
{
	u32 block_len;

	if (!s->buf)
		return;

	block_len = s->offset - start_pos;
	serialiser_add(-(ssize_t)(block_len + sizeof(u32)), s);
	serialise_u32(block_len, s);
	serialiser_add(block_len, s);
}

static void serialise_header(struct serialiser *s)
{
	const struct header *header = &s->ctx->header;
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
	if (can_use_compressor(s->ctx))
		serialise_u16(header->compressor, s);
}

static void serialise_offset_table(struct serialiser *s)
{
	const struct offset_table *table = &s->ctx->offsets;
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

static void serialise_plugins(char ** const plugins, u32 count,
	struct serialiser *s)
{
	size_t bs;
	u32 i;

	bs = start_variable_length_block(s);
	serialise_u8(count, s);
	for (i = 0u; i < count; ++i)
		serialise_bstr(plugins[i], s);
	/*
	 * idk what is going on, but Bethesda likes to add 2 to the actual
	 * size of plugin info.
	 */
	serialiser_add(2, s);
	end_variable_length_block(bs, s);
	serialiser_add(-2, s);
}

static void serialise_light_plugins(char ** const plugins, u32 count,
	struct serialiser *s)
{
	u32 i;

	serialise_u16(count, s);
	for (i = 0u; i < count; ++i)
		serialise_bstr(plugins[i], s);
}

static void serialise_uint_array(const u32 *array, u32 len, struct serialiser *s)
{
	u32 i;

	serialise_u32(len, s);
	for (i = 0u; i < len; ++i)
		serialise_u32(array[i], s);
}

static void serialise_raw_global(const struct raw_global *g, struct serialiser *s)
{
	serialiser_copy(g->data, g->data_sz, s);
}

static void serialise_misc_stats(const struct misc_stats *stats,
	struct serialiser *s)
{
	u32 i;

	serialise_u32(stats->count, s);
	for (i = 0u; i < stats->count; ++i) {
		serialise_bstr(stats->stats[i].name, s);
		serialise_u8(stats->stats[i].category, s);
		serialise_i32(stats->stats[i].value, s);
	}
}

static void serialise_player_location(const struct player_location *pl,
	struct serialiser *s)
{
	serialise_u32(pl->next_object_id, s);
	serialise_ref_id(pl->world_space1, s);
	serialise_i32(pl->coord_x, s);
	serialise_i32(pl->coord_y, s);
	serialise_ref_id(pl->world_space2, s);
	serialise_f32(pl->pos_x, s);
	serialise_f32(pl->pos_y, s);
	serialise_f32(pl->pos_z, s);
	if (*s->ctx->game == SKYRIM)
		serialise_u8(pl->unknown, s);
}

static void serialise_weather(const struct weather *w, struct serialiser *s)
{
	u32 i;

	serialise_ref_id(w->climate, s);
	serialise_ref_id(w->weather, s);
	serialise_ref_id(w->prev_weather, s);
	serialise_ref_id(w->unk_weather1, s);
	serialise_ref_id(w->unk_weather2, s);
	serialise_ref_id(w->regn_weather, s);
	serialise_f32(w->current_time, s);
	serialise_f32(w->begin_time, s);
	serialise_f32(w->weather_pct, s);
	for (i = 0u; i < ARRAY_SIZE(w->data1); ++i)
		serialise_u32(w->data1[i], s);
	serialise_f32(w->data2, s);
	serialise_u32(w->data3, s);
	serialise_u8(w->flags, s);
	serialiser_copy(w->data4, w->data4_sz, s);
}

static void serialise_global_vars(const struct global_vars *v, struct serialiser *s)
{
	u32 i;

	serialise_vsval(v->count, s);
	for (i = 0u; i < v->count; ++i) {
		serialise_ref_id(v->vars[i].form_id, s);
		serialise_f32(v->vars[i].value, s);
	}
}

static void serialise_global_data(const struct global_data *g,
	enum global_data_type type, struct serialiser *s)
{
	size_t bs;

	serialise_u32(type, s);
	bs = start_variable_length_block(s);

	printf("%08x: s global type %u\n", (unsigned)s->offset, type);

	switch (type) {
	case GLOBAL_MISC_STATS:
		serialise_misc_stats(&g->stats, s);
		break;
	case GLOBAL_PLAYER_LOCATION:
		serialise_player_location(&g->player_location, s);
		break;
	case GLOBAL_GAME:
		serialise_raw_global(&g->game, s);
		break;
	case GLOBAL_GLOBAL_VARIABLES:
		serialise_global_vars(&g->global_vars, s);
		break;
	case GLOBAL_CREATED_OBJECTS:
		serialise_raw_global(&g->created_objs, s);
		break;
	case GLOBAL_EFFECTS:
		serialise_raw_global(&g->effects, s);
		break;
	case GLOBAL_WEATHER:
		serialise_weather(&g->weather, s);
		break;
	case GLOBAL_AUDIO:
		serialise_raw_global(&g->audio, s);
		break;
	case GLOBAL_SKY_CELLS:
		serialise_raw_global(&g->sky_cells, s);
		break;
	case GLOBAL_UNKNOWN_9:
		serialise_raw_global(&g->unknown_9, s);
		break;
	case GLOBAL_UNKNOWN_10:
		serialise_raw_global(&g->unknown_10, s);
		break;
	case GLOBAL_UNKNOWN_11:
		serialise_raw_global(&g->unknown_11, s);
		break;
	case GLOBAL_PROCESS_LISTS:
		serialise_raw_global(&g->process_lists, s);
		break;
	case GLOBAL_COMBAT:
		serialise_raw_global(&g->combat, s);
		break;
	case GLOBAL_INTERFACE:
		serialise_raw_global(&g->interface, s);
		break;
	case GLOBAL_ACTOR_CAUSES:
		serialise_raw_global(&g->actor_causes, s);
		break;
	case GLOBAL_UNKNOWN_104:
		serialise_raw_global(&g->unknown_104, s);
		break;
	case GLOBAL_DETECTION_MANAGER:
		serialise_raw_global(&g->detection_man, s);
		break;
	case GLOBAL_LOCATION_METADATA:
		serialise_raw_global(&g->location_meta, s);
		break;
	case GLOBAL_QUEST_STATIC_DATA:
		serialise_raw_global(&g->quest_static, s);
		break;
	case GLOBAL_STORYTELLER:
		serialise_raw_global(&g->story_teller, s);
		break;
	case GLOBAL_MAGIC_FAVORITES:
		serialise_raw_global(&g->magic_favs, s);
		break;
	case GLOBAL_PLAYER_CONTROLS:
		serialise_raw_global(&g->player_ctrls, s);
		break;
	case GLOBAL_STORY_EVENT_MANAGER:
		serialise_raw_global(&g->story_event_man, s);
		break;
	case GLOBAL_INGREDIENT_SHARED:
		serialise_raw_global(&g->ingredient_shared, s);
		break;
	case GLOBAL_MENU_CONTROLS:
		serialise_raw_global(&g->menu_ctrls, s);
		break;
	case GLOBAL_MENU_TOPIC_MANAGER:
		serialise_raw_global(&g->menu_topic_man, s);
		break;
	case GLOBAL_UNKNOWN_115:
		serialise_raw_global(&g->unknown_115, s);
		break;
	case GLOBAL_UNKNOWN_116:
		serialise_raw_global(&g->unknown_116, s);
		break;
	case GLOBAL_UNKNOWN_117:
		serialise_raw_global(&g->unknown_117, s);
		break;
	case GLOBAL_TEMP_EFFECTS:
		serialise_raw_global(&g->temp_effects, s);
		break;
	case GLOBAL_PAPYRUS:
		serialise_raw_global(&g->papyrus, s);
		break;
	case GLOBAL_ANIM_OBJECTS:
		serialise_raw_global(&g->anim_objs, s);
		break;
	case GLOBAL_TIMER:
		serialise_raw_global(&g->timer, s);
		break;
	case GLOBAL_SYNCHRONISED_ANIMS:
		serialise_raw_global(&g->synced_anims, s);
		break;
	case GLOBAL_MAIN:
		serialise_raw_global(&g->main, s);
		break;
	case GLOBAL_UNKNOWN_1006:
		serialise_raw_global(&g->unknown_1006, s);
		break;
	case GLOBAL_UNKNOWN_1007:
		serialise_raw_global(&g->unknown_1007, s);
		break;
	default:
		eprintf("serialiser: unexpected global data type (%u)\n", type);
	}

	end_variable_length_block(bs, s);
}

static void serialise_globals1(const struct global_data *g, struct serialiser *s)
{
	serialise_global_data(g, GLOBAL_MISC_STATS, s);
	serialise_global_data(g, GLOBAL_PLAYER_LOCATION, s);
	serialise_global_data(g, GLOBAL_GAME, s);
	serialise_global_data(g, GLOBAL_GLOBAL_VARIABLES, s);
	serialise_global_data(g, GLOBAL_CREATED_OBJECTS, s);
	serialise_global_data(g, GLOBAL_EFFECTS, s);
	serialise_global_data(g, GLOBAL_WEATHER, s);
	serialise_global_data(g, GLOBAL_AUDIO, s);
	serialise_global_data(g, GLOBAL_SKY_CELLS, s);

	if (*s->ctx->game == FALLOUT4) {
		serialise_global_data(g, GLOBAL_UNKNOWN_9, s);
		serialise_global_data(g, GLOBAL_UNKNOWN_10, s);
		serialise_global_data(g, GLOBAL_UNKNOWN_11, s);
	}
}

static void serialise_globals2(const struct global_data *g, struct serialiser *s)
{
	serialise_global_data(g, GLOBAL_PROCESS_LISTS, s);
	serialise_global_data(g, GLOBAL_COMBAT, s);
	serialise_global_data(g, GLOBAL_INTERFACE, s);
	serialise_global_data(g, GLOBAL_ACTOR_CAUSES, s);
	if (*s->ctx->game == SKYRIM)
		serialise_global_data(g, GLOBAL_UNKNOWN_104, s);
	serialise_global_data(g, GLOBAL_DETECTION_MANAGER, s);
	serialise_global_data(g, GLOBAL_LOCATION_METADATA, s);
	if (*s->ctx->game == SKYRIM) {
		serialise_global_data(g, GLOBAL_QUEST_STATIC_DATA, s);
		serialise_global_data(g, GLOBAL_STORYTELLER, s);
	}
	serialise_global_data(g, GLOBAL_MAGIC_FAVORITES, s);
	serialise_global_data(g, GLOBAL_PLAYER_CONTROLS, s);
	serialise_global_data(g, GLOBAL_STORY_EVENT_MANAGER, s);
	if (*s->ctx->game == SKYRIM)
		serialise_global_data(g, GLOBAL_INGREDIENT_SHARED, s);
	serialise_global_data(g, GLOBAL_MENU_CONTROLS, s);
	serialise_global_data(g, GLOBAL_MENU_TOPIC_MANAGER, s);

	if (*s->ctx->game == FALLOUT4) {
		serialise_global_data(g, GLOBAL_UNKNOWN_115, s);
		serialise_global_data(g, GLOBAL_UNKNOWN_116, s);
		serialise_global_data(g, GLOBAL_UNKNOWN_117, s);
	}
}

static void serialise_globals3(const struct global_data *g, struct serialiser *s)
{
	serialise_global_data(g, GLOBAL_TEMP_EFFECTS, s);
	serialise_global_data(g, GLOBAL_PAPYRUS, s);
	serialise_global_data(g, GLOBAL_ANIM_OBJECTS, s);
	serialise_global_data(g, GLOBAL_TIMER, s);
	serialise_global_data(g, GLOBAL_SYNCHRONISED_ANIMS, s);
	serialise_global_data(g, GLOBAL_MAIN, s);

	if (*s->ctx->game == FALLOUT4) {
		serialise_global_data(g, GLOBAL_UNKNOWN_1006, s);
		serialise_global_data(g, GLOBAL_UNKNOWN_1007, s);
	}
}

static void serialise_change_form(const struct change_form *cf,
	struct serialiser *s)
{
	serialise_ref_id(cf->form_id, s);
	serialise_u32(cf->flags, s);
	serialise_u8(cf->type, s);
	serialise_u8(cf->version, s);

	switch (cf->type >> 6) {
	case 0u:
		serialise_u8(cf->length1, s);
		serialise_u8(cf->length2, s);
		break;
	case 1u:
		serialise_u16(cf->length1, s);
		serialise_u16(cf->length2, s);
		break;
	case 2u:
		serialise_u32(cf->length1, s);
		serialise_u32(cf->length2, s);
		break;
	default:
		eprintf("serialiser: impossible change form length type\n");
	}

	serialiser_copy(cf->data, cf->length1, s);
}

static int serialise_body(const struct game_save *save, struct serialiser *s)
{
	size_t off_offset_table;
	u32 i;

	serialise_u8(s->ctx->revision, s);

	if (*s->ctx->game == FALLOUT4)
		serialise_bstr(save->game_version, s);

	serialise_plugins(save->plugins, save->num_plugins, s);
	if (has_light_plugins(s->ctx)) {
		serialise_light_plugins(save->light_plugins,
			save->num_light_plugins, s);
	}

	off_offset_table = s->offset;
	serialise_offset_table(s);

	s->ctx->offsets.off_globals1 = s->offset;
	serialise_globals1(&save->globals, s);
	s->ctx->offsets.off_globals2 = s->offset;
	serialise_globals2(&save->globals, s);

	s->ctx->offsets.off_change_forms = s->offset;
	for (i = 0u; i < save->num_change_forms; ++i) {
		serialise_change_form(save->change_forms + i, s);
	}

	s->ctx->offsets.off_globals3 = s->offset;
	serialise_globals3(&save->globals, s);

	s->ctx->offsets.off_form_ids_count = s->offset;
	serialise_uint_array(save->form_ids, save->num_form_ids, s);
	serialise_uint_array(save->world_spaces, save->num_world_spaces, s);

	s->ctx->offsets.off_unknown_table = s->offset;
	serialise_u32(save->unknown3_sz, s);
	serialiser_copy(save->unknown3, save->unknown3_sz, s);

	struct serialiser ots;
	init_serialiser(
		s->buf ? ((char*)s->buf - (s->offset - off_offset_table)) : NULL,
		off_offset_table, s->ctx, &ots);

	serialise_offset_table(&ots);

	return 0;
}

static void serialise_signature(struct serialiser *s)
{
	switch (*s->ctx->game) {
	case SKYRIM:
		serialiser_copy(skyrim_signature,
			sizeof(skyrim_signature), s);
		break;
	case FALLOUT4:
		serialiser_copy(fallout4_signature,
			sizeof(fallout4_signature), s);
		break;
	}
}

static int serialise_save_data(const struct game_save *save,
	struct serialiser *s)
{
	u32 bs;

	serialise_signature(s);

	bs = start_variable_length_block(s);
	serialise_header(s);
	end_variable_length_block(bs, s);

	serialiser_copy(save->snapshot->data, snapshot_size(save->snapshot), s);

	if (!can_use_compressor(s->ctx))
		return serialise_body(save, s);

	struct serialiser body_s;
	void *body = NULL;
	int com_len;

	init_serialiser(NULL, s->offset, s->ctx, &body_s);
	serialise_body(save, &body_s);

	if (!s->buf) {
		/* 2*sizeof(u32) = uncompressed_len + compressed_len */
		serialiser_add(2*sizeof(u32) + body_s.n_written, s);
		return 0;
	}
	if ((body = malloc(body_s.n_written)) == NULL) {
		eprintf("serialiser: no memory\n");
		return -1;
	}

	init_serialiser(body, s->offset, s->ctx, &body_s);
	if (serialise_body(save, &body_s) == -1) {
		free(body);
		return -1;
	}

	serialise_u32(body_s.n_written, s); /* uncompressed length */
	bs = start_variable_length_block(s);

	com_len = cegse_compress(body, s->buf, body_s.n_written,
		body_s.n_written, s->ctx->header.compressor);
	free(body);
	if (com_len == -1)
		return -1;
	serialiser_add(com_len, s);

	end_variable_length_block(bs, s); /* write compressed length */

	return 0;
}

static ssize_t serialise_to_buffer(void *data, const struct game_save *save)
{
	struct file_context ctx;
	struct serialiser s;
	init_file_ctx(save, &ctx);
	init_serialiser(data, 0u, &ctx, &s);

	if (serialise_save_data(save, &s) == -1)
		return -1;

	return s.n_written;
}

int serialise_to_disk(int fd, const struct game_save *save)
{
	void *buffer;
	size_t serialised_len;
	serialised_len = serialise_to_buffer(NULL, save);

	if (lseek(fd, serialised_len - 1, SEEK_SET) == -1) {
		perror("serialiser: cannot seek");
		return -1;
	}

	if (write(fd, "", 1u) == -1) {
		perror("serialiser: cannot write to file");
		return -1;
	}

	buffer = mmap(NULL, serialised_len, PROT_WRITE, MAP_SHARED, fd, 0);
	if (buffer == MAP_FAILED) {
		perror("serialiser: cannot mmap file");
		return -1;
	}

	if (serialise_to_buffer(buffer, save) == -1) {
		munmap(buffer, serialised_len);
		return -1;
	}

	munmap(buffer, serialised_len);
	return 0;
}

#define RETURN_EOD_IF_SHORT(size, p) do {					\
	if ((p)->buf_sz < (size)) {						\
		(p)->eod = 1;							\
		return -1;							\
	}									\
} while (0)

void init_parser(const void *buf, size_t buf_sz, struct file_context *ctx,
	struct parser *p)
{
	memset(p, 0, sizeof(*p));
	p->buf = buf;
	p->buf_sz = buf_sz;
	p->ctx = ctx;
}

static inline void parser_remove(size_t n, struct parser *p)
{
	n = MIN(n, p->buf_sz);
	p->buf = (const char *)p->buf + n;
	p->buf_sz -= n;
	p->offset += n;
}

static int parser_copy(void *dest, u32 n, struct parser *p)
{
	RETURN_EOD_IF_SHORT(n, p);
	memcpy(dest, p->buf, n);
	parser_remove(n, p);
	return 0;
}

static void parser_free(struct parser *p)
{
	free(p->sp_data);
}

static int new_subparser(struct parser *restrict new, u32 com_len, u32 uncom_len,
	enum compressor method, struct parser *restrict p)
{
	void *decompressed;
	int rc;

	RETURN_EOD_IF_SHORT(com_len, p);

	decompressed = malloc(uncom_len);
	if (!decompressed) {
		eprintf("parser: no memory\n");
		return -1;
	}

	rc = cegse_decompress(p->buf, decompressed, com_len, uncom_len, method);
	if (rc == -1) {
		free(decompressed);
		return -1;
	}

	memcpy(new, p, sizeof(*new));
	parser_remove(com_len, p);

	new->sp_data = decompressed;
	new->buf = decompressed;
	new->buf_sz = uncom_len;
	return 0;
}

static enum game parse_signature(struct parser *p)
{
	enum game game = (enum game)-1;
	/*
	 * 32 should be more than enough to cover all signatures.
	 * No file signature is this long, but a valid save file should
	 * be at least 32 bytes long...
	 */
	RETURN_EOD_IF_SHORT(32, p);

	if (!memcmp(p->buf, skyrim_signature, sizeof(skyrim_signature))) {
		game = SKYRIM;
		parser_remove(sizeof(skyrim_signature), p);
	}
	else if (!memcmp(p->buf, fallout4_signature, sizeof(fallout4_signature))) {
		game = FALLOUT4;
		parser_remove(sizeof(fallout4_signature), p);
	}
	*p->ctx->game = game;
	return game;
}

static int parse_u8(u32 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(u8), p);
	*value = *(const u8 *)p->buf;
	parser_remove(sizeof(u8), p);
	return 0;
}

static int parse_u16(u32 *value, struct parser *p)
{
	RETURN_EOD_IF_SHORT(sizeof(u16), p);
	*value = le16toh(*(u16 *)p->buf);
	parser_remove(sizeof(u16), p);
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
	*value >>= 2;
	return 0;
}

static int parse_bstr(char *dest, size_t n, struct parser *p)
{
	u32 bs_len, copy_len;

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
	u32 len;

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

static int parse_header(struct parser *p)
{
	struct header *header = &p->ctx->header;
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
	if (can_use_compressor(p->ctx))
		parse_u16(&header->compressor, p);
	return p->eod ? -1 : 0;
}

static int parse_offset_table(struct parser *p)
{
	struct offset_table *table = &p->ctx->offsets;
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

/*
 * Parse a bstring array, prefixed by its length.
 * The length is parsed using parse_count callback function.
 * The array memory is allocated using malloc().
 */
static int parse_bstr_array(char ***array, u32 *array_len,
	int (*parse_count)(u32 *, struct parser *), struct parser *p)
{
	char **bstrings;
	u32 count, i;

	if (parse_count(&count, p) == -1)
		return -1;

	bstrings = malloc(count * sizeof(*bstrings));
	if (!bstrings)
		return -1;

	for (i = 0u; i < count; ++i) {
		if (parse_bstr_m(bstrings + i, p) == -1) {
			while (i)
				free(bstrings[--i]);
			free(bstrings);
			return -1;
		}
	}
	*array = bstrings;
	*array_len = count;
	return 0;
}

static int parse_ref_id(u32 *ref, struct parser *p)
{
	u8 bytes[3];

	if (parser_copy(bytes, sizeof(bytes), p) == -1)
		return -1;

	*ref = bytes[2] << 16 | bytes[1] << 8 | bytes[0];
	return 0;
}

static int parse_snapshot(struct snapshot **snapshot, struct parser *p)
{
	enum pixel_format px_format;
	unsigned shot_sz;

	px_format = which_snapshot_format(p->ctx);
	*snapshot = snapshot_new(px_format,
		p->ctx->header.snapshot_width, p->ctx->header.snapshot_height);
	if (!*snapshot)
		return -1;

	shot_sz = snapshot_size(*snapshot);
	return parser_copy((*snapshot)->data, shot_sz, p);
}

static int parse_misc_stats(struct misc_stats *stats, struct parser *p)
{
	u32 count, i;

	if (parse_u32(&count, p) == -1)
		return -1;

	stats->stats = malloc(count * sizeof(*stats->stats));
	if (!stats->stats) {
		eprintf("parser: no memory\n");
		return -1;
	}
	stats->count = count;

	for (i = 0u; i < count; ++i) {
		parse_bstr(stats->stats[i].name, sizeof(stats->stats[i].name), p);
		parse_u8(&stats->stats[i].category, p);
		parse_i32(&stats->stats[i].value, p);
	}

	return p->eod ? -1 : 0;
}

static int parse_global_vars(struct global_vars *vars, struct parser *p)
{
	u32 count, i;

	if (parse_vsval(&count, p) == -1)
		return -1;

	vars->vars = malloc(count * sizeof(*vars->vars));
	if (!vars->vars) {
		eprintf("parser: no memory\n");
		return -1;
	}
	vars->count = count;

	for (i = 0u; i < count; ++i) {
		parse_ref_id(&vars->vars[i].form_id, p);
		parse_f32(&vars->vars[i].value, p);
	}

	return p->eod ? -1 : 0;
}

static int parse_player_location(struct player_location *pl, struct parser *p)
{
	parse_u32(&pl->next_object_id, p);
	parse_ref_id(&pl->world_space1, p);
	parse_i32(&pl->coord_x, p);
	parse_i32(&pl->coord_y, p);
	parse_ref_id(&pl->world_space2, p);
	parse_f32(&pl->pos_x, p);
	parse_f32(&pl->pos_y, p);
	parse_f32(&pl->pos_z, p);
	if (*p->ctx->game == SKYRIM)
		parse_u8(&pl->unknown, p);
	return p->eod ? -1 : 0;
}

static int parse_weather(struct weather *w, u32 len, struct parser *p)
{
	size_t start = p->offset;
	u32 i;

	parse_ref_id(&w->climate, p);
	parse_ref_id(&w->weather, p);
	parse_ref_id(&w->prev_weather, p);
	parse_ref_id(&w->unk_weather1, p);
	parse_ref_id(&w->unk_weather2, p);
	parse_ref_id(&w->regn_weather, p);
	parse_f32(&w->current_time, p);
	parse_f32(&w->begin_time, p);
	parse_f32(&w->weather_pct, p);

	for (i = 0u; i < ARRAY_SIZE(w->data1); ++i)
		parse_u32(&w->data1[i], p);

	parse_f32(&w->data2, p);
	parse_u32(&w->data3, p);
	parse_u8(&w->flags, p);
	if (p->eod)
		return -1;

	w->data4_sz = len - (p->offset - start);
	w->data4 = malloc(w->data4_sz);
	if (!w->data4) {
		eprintf("parser: no memory\n");
		return -1;
	}

	parser_copy(w->data4, w->data4_sz, p);

	return p->eod ? -1 : 0;
}

static int parse_raw_global(struct raw_global *raw, u32 len, struct parser *p)
{
	raw->data = malloc(len);
	if (!raw->data)
		return -1;
	raw->data_sz = len;
	return parser_copy(raw->data, len, p);
}

static int parse_global_data(struct global_data *out, struct parser *p)
{
	size_t start_pos;
	size_t n_removed;
	u32 type;
	u32 len;
	int rc;

	parse_u32(&type, p);
	parse_u32(&len, p);
	if (p->eod)
		return -1;

	RETURN_EOD_IF_SHORT(len, p);
	start_pos = p->offset;
	printf("%08x: global type %u\n", (unsigned)p->offset, type);
	switch (type) {
	case GLOBAL_MISC_STATS:
		rc = parse_misc_stats(&out->stats, p);
		break;
	case GLOBAL_PLAYER_LOCATION:
		rc = parse_player_location(&out->player_location, p);
		break;
	case GLOBAL_GAME:
		rc = parse_raw_global(&out->game, len, p);
		break;
	case GLOBAL_GLOBAL_VARIABLES:
		rc = parse_global_vars(&out->global_vars, p);
		break;
	case GLOBAL_CREATED_OBJECTS:
		rc = parse_raw_global(&out->created_objs, len, p);
		break;
	case GLOBAL_EFFECTS:
		rc = parse_raw_global(&out->effects, len, p);
		break;
	case GLOBAL_WEATHER:
		rc = parse_weather(&out->weather, len, p);
		break;
	case GLOBAL_AUDIO:
		rc = parse_raw_global(&out->audio, len, p);
		break;
	case GLOBAL_SKY_CELLS:
		rc = parse_raw_global(&out->sky_cells, len, p);
		break;
	case GLOBAL_UNKNOWN_9:
		rc = parse_raw_global(&out->unknown_9, len, p);
		break;
	case GLOBAL_UNKNOWN_10:
		rc = parse_raw_global(&out->unknown_10, len, p);
		break;
	case GLOBAL_UNKNOWN_11:
		rc = parse_raw_global(&out->unknown_11, len, p);
		break;
	case GLOBAL_PROCESS_LISTS:
		rc = parse_raw_global(&out->process_lists, len, p);
		break;
	case GLOBAL_COMBAT:
		rc = parse_raw_global(&out->combat, len, p);
		break;
	case GLOBAL_INTERFACE:
		rc = parse_raw_global(&out->interface, len, p);
		break;
	case GLOBAL_ACTOR_CAUSES:
		rc = parse_raw_global(&out->actor_causes, len, p);
		break;
	case GLOBAL_UNKNOWN_104:
		rc = parse_raw_global(&out->unknown_104, len, p);
		break;
	case GLOBAL_DETECTION_MANAGER:
		rc = parse_raw_global(&out->detection_man, len, p);
		break;
	case GLOBAL_LOCATION_METADATA:
		rc = parse_raw_global(&out->location_meta, len, p);
		break;
	case GLOBAL_QUEST_STATIC_DATA:
		rc = parse_raw_global(&out->quest_static, len, p);
		break;
	case GLOBAL_STORYTELLER:
		rc = parse_raw_global(&out->story_teller, len, p);
		break;
	case GLOBAL_MAGIC_FAVORITES:
		rc = parse_raw_global(&out->magic_favs, len, p);
		break;
	case GLOBAL_PLAYER_CONTROLS:
		rc = parse_raw_global(&out->player_ctrls, len, p);
		break;
	case GLOBAL_STORY_EVENT_MANAGER:
		rc = parse_raw_global(&out->story_event_man, len, p);
		break;
	case GLOBAL_INGREDIENT_SHARED:
		rc = parse_raw_global(&out->ingredient_shared, len, p);
		break;
	case GLOBAL_MENU_CONTROLS:
		rc = parse_raw_global(&out->menu_ctrls, len, p);
		break;
	case GLOBAL_MENU_TOPIC_MANAGER:
		rc = parse_raw_global(&out->menu_topic_man, len, p);
		break;
	case GLOBAL_UNKNOWN_115:
		rc = parse_raw_global(&out->unknown_115, len, p);
		break;
	case GLOBAL_UNKNOWN_116:
		rc = parse_raw_global(&out->unknown_116, len, p);
		break;
	case GLOBAL_UNKNOWN_117:
		rc = parse_raw_global(&out->unknown_117, len, p);
		break;
	case GLOBAL_TEMP_EFFECTS:
		rc = parse_raw_global(&out->temp_effects, len, p);
		break;
	case GLOBAL_PAPYRUS:
		rc = parse_raw_global(&out->papyrus, len, p);
		break;
	case GLOBAL_ANIM_OBJECTS:
		rc = parse_raw_global(&out->anim_objs, len, p);
		break;
	case GLOBAL_TIMER:
		rc = parse_raw_global(&out->timer, len, p);
		break;
	case GLOBAL_SYNCHRONISED_ANIMS:
		rc = parse_raw_global(&out->synced_anims, len, p);
		break;
	case GLOBAL_MAIN:
		rc = parse_raw_global(&out->main, len, p);
		break;
	case GLOBAL_UNKNOWN_1006:
		rc = parse_raw_global(&out->unknown_1006, len, p);
		break;
	case GLOBAL_UNKNOWN_1007:
		rc = parse_raw_global(&out->unknown_1007, len, p);
		break;
	default:
		eprintf("parser: unexpected global data type (%u)\n", type);
		return -1;
	}

	if (rc == -1)
		return -1;

	n_removed = p->offset - start_pos;

	if (n_removed > len) {
		eprintf("parser: global data: read %u more than should have\n",
			(unsigned)(n_removed - len));
		return -1;
	}
	else if (n_removed < len) {
		eprintf("parser: global data: read %u less than should have\n",
			(unsigned)(len - n_removed));
		return -1;
	}

	return 0;
}

static int parse_change_form(struct change_form *cf, struct parser *p)
{
	parse_ref_id(&cf->form_id, p);
	parse_u32(&cf->flags, p);
	parse_u8(&cf->type, p);
	parse_u8(&cf->version, p);
	if (p->eod)
		return -1;

	switch (cf->type >> 6) {
	case 0u:
		parse_u8(&cf->length1, p);
		parse_u8(&cf->length2, p);
		break;
	case 1u:
		parse_u16(&cf->length1, p);
		parse_u16(&cf->length2, p);
		break;
	case 2u:
		parse_u32(&cf->length1, p);
		parse_u32(&cf->length2, p);
		break;
	default:
		eprintf("parser: impossible change form length type\n");
		return -1;
	}
	if (p->eod)
		return -1;

	/* Don't remove length information yet. */
	/* cf->type &= 0x3fu; */

	RETURN_EOD_IF_SHORT(cf->length1, p);

	cf->data = malloc(cf->length1);
	if (!cf->data) {
		eprintf("parser: no memory\n");
		return -1;
	}

	parser_copy(cf->data, cf->length1, p);
	return 0;
}

static int parse_uint_array(u32 **array, u32 *len, struct parser *p)
{
	u32 *larray;
	u32 llen, i;

	if (parse_u32(&llen, p) == -1)
		return -1;

	larray = malloc(llen * sizeof(*larray));
	if (!larray) {
		eprintf("parser: no memory\n");
		return -1;
	}

	for (i = 0u; i < llen; ++i) {
		if (parse_u32(larray + i, p) == -1)
			return -1;
	}

	*len = llen;
	*array = larray;
	return 0;
}

static int parse_body(struct game_save *save, struct parser *p)
{
	u32 next_len, i;
	int rc;

	if (parse_u8(&p->ctx->revision, p) == -1)
		return -1;
	save->file_format = p->ctx->revision;

	if (*p->ctx->game == FALLOUT4)
		parse_bstr(save->game_version, sizeof(save->game_version), p);

	parse_u32(&next_len, p);
	parse_bstr_array(&save->plugins, &save->num_plugins, parse_u8, p);

	if (has_light_plugins(p->ctx)) {
		parse_bstr_array(&save->light_plugins, &save->num_light_plugins,
			parse_u16, p);
	}

	if (parse_offset_table(p) == -1)
		return -1;
	print_offset_table(&p->ctx->offsets);

	next_len = p->ctx->offsets.num_globals1 + p->ctx->offsets.num_globals2;
	for (i = 0u; i < next_len; ++i)
		if (parse_global_data(&save->globals, p) == -1)
			return -1;

	next_len = p->ctx->offsets.num_change_form;
	save->change_forms = malloc(next_len * sizeof(*save->change_forms));
	if (!save->change_forms) {
		eprintf("parser: no memory\n");
		return -1;
	}
	for (i = 0u; i < next_len; ++i) {
		rc = parse_change_form(
			save->change_forms + save->num_change_forms, p);
		if (rc == -1)
			return -1;
		save->num_change_forms++;
	}

	for (i = 0u; i < p->ctx->offsets.num_globals3; ++i)
		if (parse_global_data(&save->globals, p) == -1)
			return -1;

	if (parse_uint_array(&save->form_ids, &save->num_form_ids, p) == -1)
		return -1;
	if (parse_uint_array(&save->world_spaces, &save->num_world_spaces, p) == -1)
		return -1;
	if (parse_u32(&save->unknown3_sz, p) == -1)
		return -1;

	save->unknown3 = malloc(save->unknown3_sz * sizeof(*save->unknown3));
	if (!save->unknown3) {
		eprintf("parser: no memory\n");
		return -1;
	}
	parser_copy(save->unknown3, save->unknown3_sz, p);
	return p->eod ? -1 : 0;
}

static int parse_save_data(void *data, size_t data_sz, struct game_save *save)
{
	struct file_context ctx;
	struct parser p;
	u32 bs;

	init_blank_file_ctx(&ctx);
	init_parser(data, data_sz, &ctx, &p);

	if (parse_signature(&p) == (enum game)-1) {
		eprintf("parser: not a Creation Engine save file\n");
		return -1;
	}

	parse_u32(&bs, &p);
	if (parse_header(&p) == -1)
		goto out_error;
	save->game = *ctx.game;
	save->engine = *ctx.engine;
	save->save_num = ctx.header.save_num;
	save->level = ctx.header.level;
	save->sex = ctx.header.sex;
	save->current_xp = ctx.header.current_xp;
	save->target_xp = ctx.header.target_xp;
	save->time_saved = filetime_to_time(p.ctx->header.filetime);
	strcpy(save->ply_name, ctx.header.ply_name);
	strcpy(save->location, ctx.header.location);
	strcpy(save->game_time, ctx.header.game_time);
	strcpy(save->race_id, ctx.header.race_id);

	if (parse_snapshot(&save->snapshot, &p) == -1)
		goto out_error;

	if (can_use_compressor(p.ctx)) {
		struct parser subp;
		u32 uncom_len;
		u32 com_len;
		int rc;
		parse_u32(&uncom_len, &p);
		parse_u32(&com_len, &p);
		if (p.eod)
			goto out_error;
		rc = new_subparser(&subp, com_len, uncom_len,
			p.ctx->header.compressor, &p);
		if (rc == -1)
			goto out_error;
		p = subp;
	}

	if (parse_body(save, &p) == -1)
		goto out_error;
	parser_free(&p);
	return 0;
out_error:
	if (p.eod)
		eprintf("parser: save file too short\n");
	if (save)
		game_save_free(save);
	parser_free(&p);
	return -1;
}

int parse_file_header_only(int fd, struct header *header)
{
	struct file_context ctx;
	struct parser p;
	char buf[1024];
	u32 header_sz;
	int in_len;

	if ((in_len = read(fd, buf, sizeof(buf))) == -1) {
		perror("parser: cannot read file");
		return -1;
	}

	init_blank_file_ctx(&ctx);
	init_parser(buf, in_len, &ctx, &p);

	if (parse_signature(&p) == (enum game)-1) {
		eprintf("parser: not a Creation Engine save file\n");
		return -1;
	}

	parse_u32(&header_sz, &p);
	if (parse_header(&p) == -1) {
		eprintf("parser: save file too short\n");
		return -1;
	}

	*header = p.ctx->header;

	return 0;
}

static int parse_file_from_pipe(int fd, struct game_save *out)
{
	size_t block = 10u * 1024u * 1024u;
	int in_length, total_read = 0;
	size_t buf_sz = 0u;
	void *buf = NULL;
	void *temp = NULL;

	while (1) {
		buf_sz += block;
		temp = realloc(buf, buf_sz);
		if (!temp) {
			free(buf);
			eprintf("parser: cannot allocate memory\n");
			return -1;
		}
		buf = temp;

		if ((in_length = read(fd, buf + total_read, block)) <= 0)
			break;

		total_read += in_length;
	}

	if (in_length == -1) {
		perror("parser: cannot read from pipe");
		free(buf);
		return -1;
	}

	if (parse_save_data(buf, total_read, out) == -1) {
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}

static int parse_file_from_disk(int fd, off_t size, struct game_save *out)
{
	void *contents;
	int rc;

	contents = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (contents == MAP_FAILED) {
		perror("parser: cannot mmap file");
		return -1;
	}
	madvise(contents, size, MADV_SEQUENTIAL);
	rc = parse_save_data(contents, size, out);
	munmap(contents, size);
	return rc;
}

int parse_file(int fd, struct game_save *out)
{
	struct stat stat;
	int rc;

	if (fstat(fd, &stat) == -1) {
		perror("parser: cannot fstat file");
		return -1;
	}

	if (S_ISFIFO(stat.st_mode))
		rc = parse_file_from_pipe(fd, out);
	else
		rc = parse_file_from_disk(fd, stat.st_size, out);
	return rc;
}
