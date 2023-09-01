/*
CEGSE allows the manipulation and the inspection of Creation Engine
game save files.

Copyright (C) 2023  SSYSS000

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
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "binary_stream.h"
#include "compression.h"
#include "defines.h"
#include "savefile.h"
#include "log.h"

#define perror(str)              eprintf("%s:%d: %s: %s\n", __FILE__, __LINE__, str, strerror(errno))

#define put_ref_id               put_be24
#define get_ref_id               get_beu24_or_zero

#define TESV_SIGNATURE           "TESV_SAVEGAME"
#define FO4_SIGNATURE            "FO4_SAVEGAME"

#define REF_TYPE(ref_id)         ((ref_id) >> 22u)
#define REF_VALUE(ref_id)        ((ref_id) & 0x3FFFFFu)

/*
 * These macros assign ref IDs a type:
 * 0 = Index into the Form ID array
 * 1 = Regular (reference to Skyrim.esm)
 * 2 = Created (plugin index of 0xFF)
 */
#define REF_INDEX(val)            (REF_VALUE(val))
#define REF_REGULAR(val)          (REF_VALUE(val) | (1u << 22u))
#define REF_CREATED(val)          (REF_VALUE(val) | (2u << 22u))

#define REF_IS_INDEX(ref_id)      ((ref_id) != 0u && REF_TYPE(ref_id) == 0u)
#define REF_IS_REGULAR(ref_id)    ((ref_id) == 0u || REF_TYPE(ref_id) == 1u)
#define REF_IS_CREATED(ref_id)    (REF_TYPE(ref_id) == 2u)
#define REF_IS_UNKNOWN(ref_id)    (REF_TYPE(ref_id) == 3u)

#define VSVAL_MAX                 4194303u

typedef enum cg_err {
    CG_OK = 0,
    CG_UNSUPPORTED,
    CG_EOF,
    CG_NO_MEM,
    CG_FSEEK,
    CG_FTELL,
    CG_TMPFILE,
    CG_CORRUPT
} cg_err_t;

struct chunk {
    size_t length;
    char data[];
};

enum change_form_type {
    CHANGE_REFR,
    CHANGE_ACHR,
    CHANGE_PMIS,
    CHANGE_PGRE,
    CHANGE_PBEA,
    CHANGE_PFLA,
    CHANGE_CELL,
    CHANGE_INFO,
    CHANGE_QUST,
    CHANGE_NPC_,
    CHANGE_ACTI,
    CHANGE_TACT,
    CHANGE_ARMO,
    CHANGE_BOOK,
    CHANGE_CONT,
    CHANGE_DOOR,
    CHANGE_INGR,
    CHANGE_LIGH,
    CHANGE_MISC,
    CHANGE_APPA,
    CHANGE_STAT,
    CHANGE_MSTT,
    CHANGE_FURN,
    CHANGE_WEAP,
    CHANGE_AMMO,
    CHANGE_KEYM,
    CHANGE_ALCH,
    CHANGE_IDLM,
    CHANGE_NOTE,
    CHANGE_ECZN,
    CHANGE_CLAS,
    CHANGE_FACT,
    CHANGE_PACK,
    CHANGE_NAVM,
    CHANGE_WOOP,
    CHANGE_MGEF,
    CHANGE_SMQN,
    CHANGE_SCEN,
    CHANGE_LCTN,
    CHANGE_RELA,
    CHANGE_PHZD,
    CHANGE_PBAR,
    CHANGE_PCON,
    CHANGE_FLST,
    CHANGE_LVLN,
    CHANGE_LVLI,
    CHANGE_LVSP,
    CHANGE_PARW,
    CHANGE_ENCH
};

enum change_flags {
    CHANGE_FORM_FLAGS                             = 0x00000001,
    CHANGE_CLASS_TAG_SKILLS                       = 0x00000002,
    CHANGE_FACTION_FLAGS                          = 0x00000002,
    CHANGE_FACTION_REACTIONS                      = 0x00000004,
    CHANGE_FACTION_CRIME_COUNTS                   = 0x80000000,
    CHANGE_TALKING_ACTIVATOR_SPEAKER              = 0x00800000,
    CHANGE_BOOK_TEACHES                           = 0x00000020,
    CHANGE_BOOK_READ                              = 0x00000040,
    CHANGE_DOOR_EXTRA_TELEPORT                    = 0x00020000,
    CHANGE_INGREDIENT_USE                         = 0x80000000,
    CHANGE_ACTOR_BASE_DATA                        = 0x00000002,
    CHANGE_ACTOR_BASE_ATTRIBUTES                  = 0x00000004,
    CHANGE_ACTOR_BASE_AIDATA                      = 0x00000008,
    CHANGE_ACTOR_BASE_SPELLLIST                   = 0x00000010,
    CHANGE_ACTOR_BASE_FULLNAME                    = 0x00000020,
    CHANGE_ACTOR_BASE_FACTIONS                    = 0x00000040,
    CHANGE_NPC_SKILLS                             = 0x00000200,
    CHANGE_NPC_CLASS                              = 0x00000400,
    CHANGE_NPC_FACE                               = 0x00000800,
    CHANGE_NPC_DEFAULT_OUTFIT                     = 0x00001000,
    CHANGE_NPC_SLEEP_OUTFIT                       = 0x00002000,
    CHANGE_NPC_GENDER                             = 0x01000000,
    CHANGE_NPC_RACE                               = 0x02000000,
    CHANGE_LEVELED_LIST_ADDED_OBJECT              = 0x80000000,
    CHANGE_NOTE_READ                              = 0x80000000,
    CHANGE_CELL_FLAGS                             = 0x00000002,
    CHANGE_CELL_FULLNAME                          = 0x00000004,
    CHANGE_CELL_OWNERSHIP                         = 0x00000008,
    CHANGE_CELL_EXTERIOR_SHORT                    = 0x10000000,
    CHANGE_CELL_EXTERIOR_CHAR                     = 0x20000000,
    CHANGE_CELL_DETACHTIME                        = 0x40000000,
    CHANGE_CELL_SEENDATA                          = 0x80000000,
    CHANGE_REFR_MOVE                              = 0x00000002,
    CHANGE_REFR_HAVOK_MOVE                        = 0x00000004,
    CHANGE_REFR_CELL_CHANGED                      = 0x00000008,
    CHANGE_REFR_SCALE                             = 0x00000010,
    CHANGE_REFR_INVENTORY                         = 0x00000020,
    CHANGE_REFR_EXTRA_OWNERSHIP                   = 0x00000040,
    CHANGE_REFR_BASEOBJECT                        = 0x00000080,
    CHANGE_REFR_PROMOTED                          = 0x02000000,
    CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN         = 0x04000000,
    CHANGE_REFR_LEVELED_INVENTORY                 = 0x08000000,
    CHANGE_REFR_ANIMATION                         = 0x10000000,
    CHANGE_REFR_EXTRA_ENCOUNTER_ZONE              = 0x20000000,
    CHANGE_REFR_EXTRA_CREATED_ONLY                = 0x40000000,
    CHANGE_REFR_EXTRA_GAME_ONLY                   = 0x80000000,
    CHANGE_ACTOR_LIFESTATE                        = 0x00000400,
    CHANGE_ACTOR_EXTRA_PACKAGE_DATA               = 0x00000800,
    CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER         = 0x00001000,
    CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS          = 0x00020000,
    CHANGE_ACTOR_LEVELED_ACTOR                    = 0x00040000,
    CHANGE_ACTOR_DISPOSITION_MODIFIERS            = 0x00080000,
    CHANGE_ACTOR_TEMP_MODIFIERS                   = 0x00100000,
    CHANGE_ACTOR_DAMAGE_MODIFIERS                 = 0x00200000,
    CHANGE_ACTOR_OVERRIDE_MODIFIERS               = 0x00400000,
    CHANGE_ACTOR_PERMANENT_MODIFIERS              = 0x00800000,
    CHANGE_OBJECT_EXTRA_ITEM_DATA                 = 0x00000400,
    CHANGE_OBJECT_EXTRA_AMMO                      = 0x00000800,
    CHANGE_OBJECT_EXTRA_LOCK                      = 0x00001000,
    CHANGE_OBJECT_EMPTY                           = 0x00200000,
    CHANGE_OBJECT_OPEN_DEFAULT_STATE              = 0x00400000,
    CHANGE_OBJECT_OPEN_STATE                      = 0x00800000,
    CHANGE_TOPIC_SAIDONCE                         = 0x80000000,
    CHANGE_QUEST_FLAGS                            = 0x00000002,
    CHANGE_QUEST_SCRIPT_DELAY                     = 0x00000004,
    CHANGE_QUEST_ALREADY_RUN                      = 0x04000000,
    CHANGE_QUEST_INSTANCES                        = 0x08000000,
    CHANGE_QUEST_RUNDATA                          = 0x10000000,
    CHANGE_QUEST_OBJECTIVES                       = 0x20000000,
    CHANGE_QUEST_SCRIPT                           = 0x40000000,
    CHANGE_QUEST_STAGES                           = 0x80000000,
    CHANGE_PACKAGE_WAITING                        = 0x40000000,
    CHANGE_PACKAGE_NEVER_RUN                      = 0x80000000,
    CHANGE_FORM_LIST_ADDED_FORM                   = 0x80000000,
    CHANGE_ENCOUNTER_ZONE_FLAGS                   = 0x00000002,
    CHANGE_ENCOUNTER_ZONE_GAME_DATA               = 0x80000000,
    CHANGE_LOCATION_KEYWORDDATA                   = 0x40000000,
    CHANGE_LOCATION_CLEARED                       = 0x80000000,
    CHANGE_QUEST_NODE_TIME_RUN                    = 0x80000000,
    CHANGE_RELATIONSHIP_DATA                      = 0x00000002,
    CHANGE_SCENE_ACTIVE                           = 0x80000000,
    CHANGE_BASE_OBJECT_VALUE                      = 0x00000002,
    CHANGE_BASE_OBJECT_FULLNAME                   = 0x00000004
};

struct change_form {
    ref_t form_id;
    uint32_t flags;
    uint32_t type;
    uint32_t version;
    uint32_t length1;    /* Length of data */
    uint32_t length2;    /* Non-zero value means data is compressed */
    unsigned char *data;
};

enum compressor {
    NO_COMPRESSION = 0,
    ZLIB           = 1,
    LZ4            = 2,
};

struct savegame_private {
    /*
     * Skyrim LE: 7,8,9
     * Skyrim SE: 12
     * Fallout 4: 11, 15
     */
    uint32_t file_version;
    uint8_t  form_version;

    /* the original savefile compression algorithm. */
    enum compressor compressor;

    /* Unknown global data structures. */
    struct chunk *global2;
    struct chunk *global4;
    struct chunk *global5;
    struct chunk *global7;
    struct chunk *global8;
    struct chunk *global100;
    struct chunk *global101;
    struct chunk *global102;
    struct chunk *global103;
    struct chunk *global104;
    struct chunk *global105;
    struct chunk *global106;
    struct chunk *global107;
    struct chunk *global108;
    struct chunk *global110;
    struct chunk *global111;
    struct chunk *global112;
    struct chunk *global113;
    struct chunk *global114;
    struct chunk *global1000;
    struct chunk *global1001;
    struct chunk *global1002;
    struct chunk *global1003;
    struct chunk *global1004;
    struct chunk *global1005;

    uint32_t num_change_forms;
    struct change_form *change_forms;

    /* Data at the end of the savefile. */
    struct chunk *unknown3;
};

struct location_table {
    uint32_t off_save_data;
    uint32_t off_form_ids_count;
    uint32_t off_unknown_table;
    uint32_t off_globals1;
    uint32_t off_globals2;
    uint32_t off_globals3;
    uint32_t off_change_forms;
    uint32_t num_globals1;
    uint32_t num_globals2;
    uint32_t num_globals3;
    uint32_t num_change_forms;
};

static void print_locations_table(const struct location_table *t)
{
    DEBUG_LOG("Location table\n=============================\n");
    DEBUG_LOG("File offset   Object\n");
    DEBUG_LOG("0x%08x:   Globals 1 (%u)\n", t->off_globals1, t->num_globals1);
    DEBUG_LOG("0x%08x:   Globals 2 (%u)\n", t->off_globals2, t->num_globals2);
    DEBUG_LOG("0x%08x:   Change forms (%u)\n", t->off_change_forms, t->num_change_forms);
    DEBUG_LOG("0x%08x:   Globals 3 (%u)\n", t->off_globals3, t->num_globals3);
    DEBUG_LOG("0x%08x:   Form IDs\n", t->off_form_ids_count);
    DEBUG_LOG("0x%08x:   Unknown table\n", t->off_unknown_table);
    DEBUG_LOG("=============================\n");
}

static bool supports_save_file_compression(const struct savegame *save)
{
    return save->game == SKYRIM && save->_private->file_version >= 12;
}

static bool supports_light_plugins(const struct savegame *save)
{
    switch (save->game) {
    case SKYRIM:
        return save->_private->file_version >= 12 && save->_private->form_version >= 78;

    case FALLOUT4:
        return save->_private->file_version >= 12;

    default:
        BUG("invalid game");
        abort();
    }
}

static unsigned snapshot_pixel_width(const struct savegame *save)
{
    return save->_private->file_version >= 11 ? 4 : 3;
}

#if 0
/**
 * @brief Convert a FILETIME to a time_t.
 *
 * @note FILETIME is more accurate than time_t.
 * @param filetime Filetime.
 * @return Filetime converted to time_t.
 */
static inline time_t filetime_to_time(FILETIME filetime)
{
    return (time_t)(filetime / 10000000) - 11644473600;
}

/**
 * @brief Convert a time_t to a FILETIME.
 *
 * @note FILETIME is more accurate than time_t.
 * @param t time_t value.
 * @return Filetime.
 */
static inline FILETIME time_to_filetime(time_t t)
{
    return (FILETIME)(t + 11644473600) * 10000000;
}
#endif

static int encode_vsval(FILE *stream, uint32_t value)
{
    int num_octets;

    assert(value <= VSVAL_MAX);

    /*
     * 2 least significant bits determine vsval size
     *
     *    2 LSB    | vsval size
     * ------------+------------
     *     00       |   1 octet
     *     01       |   2 octets
     *     10       |   3 octets
     */
    value <<= 2;            /* vsval size information takes 2 bits. */
    value &= 0xffffffu;        /* vsval max width is 24 bits. */
    num_octets = ((value >= 0x100u) << (value >= 0x10000u)) + 1;
    value |= num_octets - 1; /* add size information */
    value = htole32(value);  /* vsval is little endian */
    return fwrite(&value, 1, num_octets, stream);
}

static cg_err_t decode_vsval(FILE *stream, uint32_t *out)
{
    uint8_t b[3] = {0};
    int i;

    /*
     * Loop at least once. Break when all octets have been read
     * or octet count is invalid.
     */
    for (i = 0; i < ((b[0] + 1) & 0x3); ++i) {
        if (!get_u8(stream, &b[i])) {
            return CG_EOF;
        }
    }

    /* Check if octet count is invalid (> 3 octets). */
    if ((b[0] & 0x3) == 3) {
        /* Count not valid. Corrupt vsval. */
        return CG_CORRUPT;
    }

    *out = ((uint32_t)b[2] << 16 | (uint32_t)b[1] << 8 | b[0]) >> 2;

    return CG_OK;
}

static cg_err_t get_le16_str(FILE *stream, char **string_out)
{
    uint16_t string_length;
    char *string;

    if (!get_leu16(stream, &string_length)) {
        return CG_EOF;
    }

    string = malloc((size_t)string_length + 1);
    if (!string) {
        return CG_NO_MEM;
    }

    if (fread(string, 1, string_length, stream) != string_length) {
        free(string);
        return CG_EOF;
    }

    string[string_length] = '\0';

    *string_out = string;
    return CG_OK;
}

static int put_le16_str(FILE *restrict stream, const char *restrict string)
{
    size_t length = strlen(string);
    assert(length <= UINT16_MAX);

    if (!put_le16(stream, length)) {
        return 0;
    }

    if (fwrite(string, 1, length, stream) != length) {
        return 0;
    }

    return sizeof(uint16_t) + length;
}

static cg_err_t read_le32_array(FILE *stream, uint32_t **ptr, size_t n)
{
    uint32_t *array;

    if (n == 0) {
        *ptr = NULL;
        return CG_OK;
    }

    array = malloc(n * sizeof(*array));

    if (!array) {
        return CG_NO_MEM;
    }

    for (size_t i = 0; i < n; ++i) {
        array[i] = get_leu32_or_zero(stream);
    }

    if (feof(stream) || ferror(stream)) {
        free(array);
        return CG_EOF;
    }

    *ptr = array;

    return CG_OK;
}

static cg_err_t read_le16_str_array(FILE *stream, char ***ptr, size_t length)
{
    cg_err_t err = CG_OK;
    char **array;
    size_t i;

    if (length == 0) {
        *ptr = NULL;
        return CG_OK;
    }

    array = calloc(length, sizeof(*array));

    if (!array) {
        return CG_NO_MEM;
    }

    for (i = 0; i < length; ++i) {
        err = get_le16_str(stream, &array[i]);

        if (err) {
            break;
        }
    }

    if (err) {
        while (i--) {
            free(array[i]);
        }

        free(array);
        return err;
    }

    *ptr = array;

    return CG_OK;
}

static cg_err_t alloc_and_read_chunk(FILE *stream, struct chunk **chunk_out, size_t length)
{
    struct chunk *chunk = malloc(sizeof(*chunk) + length);

    if (!chunk) {
        return CG_NO_MEM;
    }

    chunk->length = length;
    if (fread(chunk->data, 1, length, stream) != length) {
        free(chunk);
        return CG_EOF;
    }

    *chunk_out = chunk;

    return CG_OK;
}

static void dump_to_file(const char *filename, void *data, size_t size, long offset)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        return;
    }

    if (offset > 0) {
        fseek(fp, offset, SEEK_SET);
    }

    fwrite(data, size, 1, fp);
    fclose(fp);
}

static cg_err_t read_global_data_table(FILE *stream, struct savegame *save, unsigned count)
{
    long start_of_data, file_pos;
    uint32_t type, length;
    cg_err_t err;

    for(unsigned i = 0; i < count; ++i) {
        err    = CG_OK;
        type   = get_leu32_or_zero(stream);
        length = get_leu32_or_zero(stream);

        if (feof(stream) || ferror(stream)) {
            return CG_EOF;
        }

        start_of_data = ftell(stream);
        if (start_of_data == -1) {
            return CG_FTELL;
        }

        DEBUG_LOG("0x%08lx: Global data, type=%u length=%u\n", ftell(stream), type, length);

        switch(type) {
        case 0: /* Miscellaneous statistics */
            if (!get_leu32(stream, &save->num_misc_stats)) {
                err = CG_EOF;
                break;
            }

            if (save->num_misc_stats == 0) {
                break;
            }

            save->misc_stats = calloc(save->num_misc_stats, sizeof(*save->misc_stats));
            if (!save->misc_stats) {
                err = CG_NO_MEM;
                break;
            }

            for (uint32_t i = 0u; i < save->num_misc_stats; ++i) {
                err = get_le16_str(stream, &save->misc_stats[i].name);
                if (err) {
                    break;
                }

                save->misc_stats[i].category = get_u8_or_zero(stream);
                save->misc_stats[i].value    = get_lei32_or_zero(stream);
            }
            break;

        case 1: /* Player location */
            save->player_location.next_object_id = get_leu32_or_zero(stream);
            save->player_location.world_space1   = get_ref_id(stream);
            save->player_location.coord_x        = get_lei32_or_zero(stream);
            save->player_location.coord_y        = get_lei32_or_zero(stream);
            save->player_location.world_space2   = get_ref_id(stream);
            save->player_location.pos_x          = get_le32_ieee754_or_zero(stream);
            save->player_location.pos_y          = get_le32_ieee754_or_zero(stream);
            save->player_location.pos_z          = get_le32_ieee754_or_zero(stream);
            if (save->game == SKYRIM) {
                save->player_location.unknown    = get_u8_or_zero(stream); /* Skyrim only */
            }
            break;

        case 2:
            err = alloc_and_read_chunk(stream, &save->_private->global2, length);
            break;

        case 3:
            err = decode_vsval(stream, &save->num_global_vars);

            if (err || save->num_global_vars == 0) {
                break;
            }

            save->global_vars = calloc(save->num_global_vars, sizeof(*save->global_vars));

            if (!save->global_vars) {
                err = CG_NO_MEM;
                break;
            }

            for (uint32_t i = 0u; i < save->num_global_vars; ++i) {
                save->global_vars[i].form_id = get_ref_id(stream);
                save->global_vars[i].value   = get_le32_ieee754_or_zero(stream);
            }
            break;

        case 4:
            err = alloc_and_read_chunk(stream, &save->_private->global4, length);
            break;

        case 5:
            err = alloc_and_read_chunk(stream, &save->_private->global5, length);
            break;

        case 6:
            save->weather.climate      = get_ref_id(stream);
            save->weather.weather      = get_ref_id(stream);
            save->weather.prev_weather = get_ref_id(stream);
            save->weather.unk_weather1 = get_ref_id(stream);
            save->weather.unk_weather2 = get_ref_id(stream);
            save->weather.regn_weather = get_ref_id(stream);
            save->weather.current_time = get_le32_ieee754_or_zero(stream);
            save->weather.begin_time   = get_le32_ieee754_or_zero(stream);
            save->weather.weather_pct  = get_le32_ieee754_or_zero(stream);
            for (size_t i = 0u; i < 6; ++i)
                save->weather.data1[i] = get_leu32_or_zero(stream);
            save->weather.data2        = get_le32_ieee754_or_zero(stream);
            save->weather.data3        = get_leu32_or_zero(stream);
            save->weather.flags        = get_u8_or_zero(stream);

            if (feof(stream) || ferror(stream)) {
                err = CG_EOF;
                break;
            }

            /* Read the remaining bytes. Don't know what they are. */
            file_pos = ftell(stream);

            if (file_pos == -1) {
                err = CG_FTELL;
                break;
            }

            save->weather.data4_sz = length - (file_pos - start_of_data);
            if (save->weather.data4_sz > 0) {
                save->weather.data4 = malloc(save->weather.data4_sz);
                if (!save->weather.data4) {
                    err = CG_NO_MEM;
                    break;
                }

                fread(save->weather.data4, save->weather.data4_sz, 1, stream);
            }
            break;

        case 7:
            err = alloc_and_read_chunk(stream, &save->_private->global7, length);
            break;

        case 8:
            err = alloc_and_read_chunk(stream, &save->_private->global8, length);
            break;

        case 100:
            err = alloc_and_read_chunk(stream, &save->_private->global100, length);
            break;

        case 101:
            err = alloc_and_read_chunk(stream, &save->_private->global101, length);
            break;

        case 102:
            err = alloc_and_read_chunk(stream, &save->_private->global102, length);
            break;

        case 103:
            err = alloc_and_read_chunk(stream, &save->_private->global103, length);
            break;

        case 104:
            err = alloc_and_read_chunk(stream, &save->_private->global104, length);
            break;

        case 105:
            err = alloc_and_read_chunk(stream, &save->_private->global105, length);
            break;

        case 106:
            err = alloc_and_read_chunk(stream, &save->_private->global106, length);
            break;

        case 107:
            err = alloc_and_read_chunk(stream, &save->_private->global107, length);
            break;

        case 108:
            err = alloc_and_read_chunk(stream, &save->_private->global108, length);
            break;

        case 109:
            err = decode_vsval(stream, &save->num_favourites);
            if (err) {
                break;
            }

            save->favourites = malloc(sizeof(*save->favourites) * save->num_favourites);

            if (!save->favourites) {
                err = CG_NO_MEM;
                break;
            }

            for (uint32_t i = 0; i < save->num_favourites; ++i) {
                save->favourites[i] = get_ref_id(stream);
            }

            err = decode_vsval(stream, &save->num_hotkeys);
            if (err) {
                break;
            }

            save->hotkeys = malloc(sizeof(*save->hotkeys) * save->num_hotkeys);

            if (!save->hotkeys) {
                err = CG_NO_MEM;
                break;
            }

            for (uint32_t i = 0; i < save->num_hotkeys; ++i) {
                save->hotkeys[i] = get_ref_id(stream);
            }
            break;

        case 110:
            err = alloc_and_read_chunk(stream, &save->_private->global110, length);
            break;

        case 111:
            err = alloc_and_read_chunk(stream, &save->_private->global111, length);
            break;

        case 112:
            err = alloc_and_read_chunk(stream, &save->_private->global112, length);
            break;

        case 113:
            err = alloc_and_read_chunk(stream, &save->_private->global113, length);
            break;

        case 114:
            err = alloc_and_read_chunk(stream, &save->_private->global114, length);
            break;

        case 1000:
            err = alloc_and_read_chunk(stream, &save->_private->global1000, length);
            break;

        case 1001:
            err = alloc_and_read_chunk(stream, &save->_private->global1001, length);
            break;

        case 1002:
            err = alloc_and_read_chunk(stream, &save->_private->global1002, length);
            break;

        case 1003:
            err = alloc_and_read_chunk(stream, &save->_private->global1003, length);
            break;

        case 1004:
            err = alloc_and_read_chunk(stream, &save->_private->global1004, length);
            break;

        case 1005:
            err = alloc_and_read_chunk(stream, &save->_private->global1005, length);
            break;

        default:
            break;
        }

        if (err) {
            return err;
        }
        else if (feof(stream) || ferror(stream)) {
            return CG_EOF;
        }

        file_pos = ftell(stream);

        if (file_pos == -1) {
            perror("ftell");
            return CG_FTELL;
        }

        long bytes_read = file_pos - start_of_data;
        long bytes_remaining = (long)length - bytes_read;

        if (bytes_remaining != 0) {
            DEBUG_LOG("bytes remaining=%ld, type=%u\n", bytes_remaining, type);
            return CG_EOF;
        }
    }

    return CG_OK;
}

static cg_err_t read_change_form(FILE *restrict stream, struct change_form *restrict cf)
{
    memset(cf, 0, sizeof(*cf));

    cf->form_id = get_ref_id(stream);
    cf->flags   = get_leu32_or_zero(stream);
    cf->type    = get_u8_or_zero(stream);
    cf->version = get_u8_or_zero(stream);

    if (feof(stream) || ferror(stream)) {
        return CG_EOF;
    }


    /* Two upper bits of type determine the sizes of length1 and length2. */
    switch ((cf->type >> 6) & 0x3) {
    case 0:
        cf->length1 = get_u8_or_zero(stream);
        cf->length2 = get_u8_or_zero(stream);
        break;
    case 1:
        cf->length1 = get_leu16_or_zero(stream);
        cf->length2 = get_leu16_or_zero(stream);
        break;
    case 2:
        cf->length1 = get_leu32_or_zero(stream);
        cf->length2 = get_leu32_or_zero(stream);
        break;
    default:
        /* 
         * The savefile may be corrupt, but more likely there's a bug that
         * led us here.
         */
        BUG("change form type length bits: 11");
        return CG_CORRUPT;
    }

    if (feof(stream) || ferror(stream)) {
        return CG_EOF;
    }

    DEBUG_LOG("0x%08lx: Change form, formid=%u flags=0x%x type=%u version=%u length1=%u length2=%u\n",
              ftell(stream),
              cf->form_id,
              cf->flags,
              cf->type & 0x3fu,
              cf->version,
              cf->length1,
              cf->length2);

    /* TODO: Support for change forms */
    /* cf->type &= 0x3fu; */

    cf->data = malloc(cf->length1);

    if (cf->data == NULL) {
        return CG_NO_MEM;
    }

    if (fread(cf->data, 1, cf->length1, stream) != cf->length1) {
        return CG_EOF;
    }

    return CG_OK;
}

static cg_err_t read_save_data(FILE *stream, struct savegame *save);
static cg_err_t read_savefile(FILE *stream, struct savegame *save);

struct savegame *cengine_savefile_read(const char *filename)
{
    struct savegame *save;
    FILE *stream;
    cg_err_t err;

    save = calloc(1, sizeof(*save));
    if (!save) {
        perror("calloc");
        return NULL;
    }

    DEBUG_LOG("Opening save file %s\n", filename);

    stream = fopen(filename, "rb");
    if (!stream) {
        perror("fopen");
        free(save);
        return NULL;
    }

    DEBUG_LOG("Reading save file %s\n", filename);

    err = read_savefile(stream, save);

    switch (err) {
    case CG_OK:
        /* No error. */
        break;
    case CG_UNSUPPORTED:
        eprintf("File cannot be read because its format is unsupported.\n");
        break;
    case CG_EOF:
        if (feof(stream)) {
            eprintf("File ended too soon. Is the save corrupt?\n");
        }
        else if(ferror(stream)) {
            eprintf("File error occurred.\n");
        }
        else {
            eprintf("bug: Neither EOF nor file error condition set, yet EOF was returned.\n");
        }
        break;
    case CG_NO_MEM:
        eprintf("Failed to allocate memory.\n");
        break;
    }

    (void) fclose(stream);

    if (err) {
        DEBUG_LOG("Error %d occurred while reading save file\n", err);
        savegame_free(save);
        return NULL;
    }

    return save;
}

static cg_err_t read_savefile(FILE *stream, struct savegame *save)
{
    uint32_t block_size;
    cg_err_t err;

    save->_private = calloc(1, sizeof(*save->_private));

    if (!save->_private) {
        return CG_NO_MEM;
    }

    /*
     * Check the file signature.
     */
    {
        char file_signature[48];

        DEBUG_LOG("0x%08lx: Reading file signature\n", ftell(stream));

        if (fread(file_signature, sizeof(file_signature), 1, stream) != 1) {
            return CG_EOF;
        }

        if (!memcmp(file_signature, TESV_SIGNATURE, strlen(TESV_SIGNATURE))) {
            DEBUG_LOG("TESV file signature detected\n");

            save->game = SKYRIM;
            block_size = strlen(TESV_SIGNATURE);
        }
        else if(!memcmp(file_signature, FO4_SIGNATURE, strlen(FO4_SIGNATURE))) {
            DEBUG_LOG("Fallout 4 file signature detected\n");

            save->game = FALLOUT4;
            block_size = strlen(FO4_SIGNATURE);
            //return CG_UNSUPPORTED;
        }
        else {
            return CG_UNSUPPORTED;
        }

        /* Return to the end of the file signature. */
        if (fseek(stream, block_size, SEEK_SET) == -1) {
            perror("fseek");
            return CG_FSEEK;
        }
    }

    /*
     * Read the file header.
     */
    if (!get_leu32(stream, &block_size)) {
        return CG_EOF;
    }


    DEBUG_LOG("0x%08lx: Reading file header\n", ftell(stream));

    if (!get_leu32(stream, &save->_private->file_version)) {
        return CG_EOF;
    }

    DEBUG_LOG("File version: %u\n", save->_private->file_version);

    if (save->_private->file_version > 15) {
        return CG_UNSUPPORTED;
    }

    save->save_num = get_leu32_or_zero(stream);
    err = get_le16_str(stream, &save->player_name);
    if (err) return err;
    save->level = get_leu32_or_zero(stream);
    err = get_le16_str(stream, &save->player_location_name);
    if (err) return err;
    err = get_le16_str(stream, &save->game_time);
    if (err) return err;
    err = get_le16_str(stream, &save->race_id);
    if (err) return err;
    save->sex             = get_leu16_or_zero(stream);
    save->current_xp      = get_le32_ieee754_or_zero(stream);
    save->target_xp       = get_le32_ieee754_or_zero(stream);
    save->filetime        = get_leu64_or_zero(stream);
    save->snapshot_width  = get_leu32_or_zero(stream);
    save->snapshot_height = get_leu32_or_zero(stream);

    if (supports_save_file_compression(save)) {
        /*
         * Read compression type and validate it.
         */
        uint16_t compression_type;

        if (!get_leu16(stream, &compression_type)) {
            return CG_EOF;
        }

        /* Validate. */
        if (!(compression_type <= 2)) {
            DEBUG_LOG("Read an invalid compression type: %u\n", compression_type);
            return CG_CORRUPT;
        }

        save->_private->compressor = compression_type;

        DEBUG_LOG("Save file compression algorithm = %s.\n",
            compression_type == LZ4 ? "lz4" :
            compression_type == ZLIB ? "zlib" : "none");
    }
    else {
        DEBUG_LOG("No save file compression support\n");
    }

    if (feof(stream) || ferror(stream)) {
        return CG_EOF;
    }

    /*
     * Read the snapshot.
     */
    save->snapshot_bytes_per_pixel = snapshot_pixel_width(save);
    save->snapshot_size = save->snapshot_width * save->snapshot_height;
    save->snapshot_size *= save->snapshot_bytes_per_pixel;
    save->snapshot_data = malloc(save->snapshot_size);
    if (!save->snapshot_data) {
        return CG_NO_MEM;
    }

    DEBUG_LOG("0x%08lx: Reading %u bytes of snapshot data\n", ftell(stream), save->snapshot_size);

    if (fread(save->snapshot_data, save->snapshot_size, 1, stream) != 1) {
        return CG_EOF;
    }

    /*
     * Depending on file version, the save data could be compressed.
     * If the save file does not contain compression information, just keep
     * reading from this stream. Otherwise, decompress the save data into a
     * temporary stream and continue reading from there.
     */
    if (!supports_save_file_compression(save)) {
        return read_save_data(stream, save);
    }

    /* Save file contains compression information. */
    struct {
        uint32_t size;
        char    *data;
    } uncompressed, compressed;
    long start_of_save_data;
    FILE *temp_stream;
    int decompress_rc;

    start_of_save_data = ftell(stream);
    if (start_of_save_data == -1) {
        perror("ftell");
        return CG_FTELL;
    }

    /* Read compression related sizes. */
    uncompressed.size = get_leu32_or_zero(stream);
    compressed.size   = get_leu32_or_zero(stream);

    if (feof(stream) || ferror(stream)) {
        return CG_EOF;
    }

    DEBUG_LOG("Save data uncompressed length: %u\n", uncompressed.size);
    DEBUG_LOG("Save data compressed length:   %u\n", compressed.size);

    /* Allocate buffers for decompression. */
    uncompressed.data = malloc(uncompressed.size);
    compressed.data   = malloc(compressed.size);

    if (!uncompressed.data || !compressed.data) {
        free(uncompressed.data);
        free(compressed.data);
        return CG_NO_MEM;
    }

    if (fread(compressed.data, compressed.size, 1, stream) != 1) {
        free(uncompressed.data);
        free(compressed.data);
        return CG_EOF;
    }

    /*
     * Decompress save data.
     */
    DEBUG_LOG("Decompressing save data\n");

    switch (save->_private->compressor) {
    case NO_COMPRESSION:
        eprintf("No handler for uncompressed save data\n");
        exit(1);

    case LZ4:
        decompress_rc = lz4_decompress(compressed.data, uncompressed.data, compressed.size, uncompressed.size);
        break;

    case ZLIB:
        decompress_rc = zlib_decompress(compressed.data, uncompressed.data, compressed.size, uncompressed.size);
        break;

    default:
        BUG("invalid compressor");
        abort();
    }

    free(compressed.data);

    if (decompress_rc == -1) {
        eprintf("Decompression failed\n");
        free(uncompressed.data);
        return CG_CORRUPT;
    }

    DEBUG_LOG("Dumping decompressed save data\n");
    dump_to_file("decompressed_save_data", uncompressed.data, uncompressed.size, start_of_save_data);

    temp_stream = tmpfile();
    if (!temp_stream) {
        free(uncompressed.data);
        return CG_TMPFILE;
    }

    if (fseek(temp_stream, start_of_save_data, SEEK_SET) == -1) {
        fclose(temp_stream);
        free(uncompressed.data);
        perror("fseek");
        return CG_FSEEK;
    }

    if (fwrite(uncompressed.data, uncompressed.size, 1, temp_stream) != 1) {
        fclose(temp_stream);
        free(uncompressed.data);
        return CG_EOF;
    }

    free(uncompressed.data);

    if (fseek(temp_stream, start_of_save_data, SEEK_SET) == -1) {
        fclose(temp_stream);
        perror("fseek");
        return CG_FSEEK;
    }

    err = read_save_data(temp_stream, save);

    (void) fclose(temp_stream);

    return err;
}

static cg_err_t read_save_data(FILE *stream, struct savegame *save)
{
    struct location_table loc;
    uint32_t block_size;
    cg_err_t err;

    DEBUG_LOG("0x%08lx: Save data begins\n", ftell(stream));

    if (!get_u8(stream, &save->_private->form_version)) {
        return CG_EOF;
    }

    DEBUG_LOG("Save data form version: %u\n", save->_private->form_version);

    if (save->game == FALLOUT4) {
        err = get_le16_str(stream, &save->game_version);

        if (err) {
            return err;
        }
    }

    /*
     * Read plugin information.
     */

    /*
     * Length of plugin information: size of plugin count, plugin strings,
     * light plugin count and light plugin string.
     */
    (void) get_leu32_or_zero(stream);

    if (!get_u8(stream, &save->num_plugins)) {
        return CG_EOF;
    }

    DEBUG_LOG("0x%08lx: Reading %u plugins\n", ftell(stream), save->num_plugins);

    err = read_le16_str_array(stream, &save->plugins, save->num_plugins);
    if (err) {
        return err;
    }

    /*
     * Read light plugins.
     */
    if (supports_light_plugins(save)) {
        if (!get_leu16(stream, &save->num_light_plugins)) {
            return CG_EOF;
        }

        DEBUG_LOG("0x%08lx: Reading %u light plugins\n", ftell(stream), save->num_light_plugins);

        err = read_le16_str_array(stream, &save->light_plugins, save->num_light_plugins);
        if (err) {
            return err;
        }
    }

    /*
     * Read the offsets and the sizes of savefile objects.
     */
    DEBUG_LOG("0x%08lx: Reading locations table\n", ftell(stream));

    loc.off_form_ids_count = get_leu32_or_zero(stream);
    loc.off_unknown_table  = get_leu32_or_zero(stream);
    loc.off_globals1       = get_leu32_or_zero(stream);
    loc.off_globals2       = get_leu32_or_zero(stream);
    loc.off_change_forms   = get_leu32_or_zero(stream);
    loc.off_globals3       = get_leu32_or_zero(stream);
    loc.num_globals1       = get_leu32_or_zero(stream);
    loc.num_globals2       = get_leu32_or_zero(stream);
    loc.num_globals3       = get_leu32_or_zero(stream);
    loc.num_change_forms   = get_leu32_or_zero(stream);

    save->_private->num_change_forms = loc.num_change_forms;

    /* Skip junk. */
    if (fseek(stream, 60, SEEK_CUR) == -1) {
        perror("fseek");
        return CG_FSEEK;
    }

    print_locations_table(&loc);

    /* Globals 3 table count is bugged and short by 1. */
    loc.num_globals3 += 1;

    /*
     * Read global data tables 1 and 2.
     */
    DEBUG_LOG("0x%08lx: Reading global data table 1\n", ftell(stream));

    err = read_global_data_table(stream, save, loc.num_globals1);

    if (err) {
        return err;
    }

    DEBUG_LOG("0x%08lx: Reading global data table 2\n", ftell(stream));

    err = read_global_data_table(stream, save, loc.num_globals2);

    if (err) {
        return err;
    }

    /*
     * Read change forms.
     */
    save->_private->change_forms = calloc(loc.num_change_forms, sizeof(*save->_private->change_forms));

    if (!save->_private->change_forms) {
        return CG_NO_MEM;
    }

    DEBUG_LOG("0x%08lx: Reading %u change forms\n", ftell(stream), loc.num_change_forms);

    for (unsigned i = 0; i < loc.num_change_forms; ++i) {
        err = read_change_form(stream, save->_private->change_forms + i);
        if (err) {
            return err;
        }
    }

    /*
     * Read global data table 3.
     */
    DEBUG_LOG("0x%08lx: Reading global data table 3\n", ftell(stream));

    err = read_global_data_table(stream, save, loc.num_globals3);

    if (err) {
        return err;
    }

    /*
     * Read form IDs.
     */
    if (!get_leu32(stream, &save->num_form_ids)) {
        return CG_EOF;
    }

    DEBUG_LOG("0x%08lx: Reading %u form IDs\n", ftell(stream), save->num_form_ids);

    err = read_le32_array(stream, &save->form_ids, save->num_form_ids);

    if (err) {
        return err;
    }

    /*
     * Read world spaces.
     */
    if (!get_leu32(stream, &save->num_world_spaces)) {
        return CG_EOF;
    }

    DEBUG_LOG("0x%08lx: Reading %u world spaces\n", ftell(stream), save->num_world_spaces);

    err = read_le32_array(stream, &save->world_spaces, save->num_world_spaces);

    if (err) {
        return err;
    }

    /*
     * Read the unknown chunk at the end of the savefile.
     */
    if (!get_leu32(stream, &block_size)) {
        return CG_EOF;
    }

    DEBUG_LOG("0x%08lx: Reading unknown table\n", ftell(stream));
    err = alloc_and_read_chunk(stream, &save->_private->unknown3, block_size);

    return err;
}

static void write_locations(FILE *restrict stream,
        const struct location_table *restrict table)
{
    put_le32(stream, table->off_form_ids_count);
    put_le32(stream, table->off_unknown_table);
    put_le32(stream, table->off_globals1);
    put_le32(stream, table->off_globals2);
    put_le32(stream, table->off_change_forms);
    put_le32(stream, table->off_globals3);
    put_le32(stream, table->num_globals1);
    put_le32(stream, table->num_globals2);
    /* Subtract by 1: Skyrim doesn't acknowledge the last global data. */
    put_le32(stream, table->num_globals3 - 1);
    put_le32(stream, table->num_change_forms);

    /* Add 60 zeros at the end. */
    fseek(stream, 59, SEEK_CUR);
    fputc(0, stream);
}

static void write_chunk(FILE *restrict stream, const struct chunk *chunk)
{
    fwrite(chunk->data, chunk->length, 1, stream);
}

#define BEGIN_VARIABLE_LENGTH_BLOCK(stream)                                    \
{                                                                              \
    put_le32(stream, 0);                                                       \
    long $block_start = ftell(stream);                                         \
    {

#define END_VARIABLE_LENGTH_BLOCK(stream)                                      \
    }                                                                          \
    long $block_end = ftell(stream);                                           \
    long $block_len = $block_end - $block_start;                               \
    fseek(stream, $block_start - sizeof(uint32_t), SEEK_SET);                  \
    put_le32(stream, $block_len);                                              \
    fseek(stream, $block_end, SEEK_SET);                                       \
}

#define WRITE_UNKNOWN_GLOBAL_DATA(stream, savegame, number) do {               \
    put_le32((stream), number);                                                \
    put_le32((stream), (savegame)->_private->global##number->length);          \
    DEBUG_LOG("writing global data type %u at 0x%08lx\n", number, ftell(stream));  \
    write_chunk((stream), (savegame)->_private->global##number);               \
} while (0)                                                                    \

/* Write types 0 to 8. */
static int write_global_data_table1(FILE *restrict stream, const struct savegame *save)
{
    int num_objects = 0;

    /*
     * Write miscellaneous statistics.
     */
    put_le32(stream, 0);
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    put_le32(stream, save->num_misc_stats);
    for (uint32_t i = 0u; i < save->num_misc_stats; ++i) {
        put_le16_str(stream, save->misc_stats[i].name);
        put_u8(stream,       save->misc_stats[i].category);
        put_le32(stream,     save->misc_stats[i].value);
    }
    END_VARIABLE_LENGTH_BLOCK(stream)
    num_objects++;

    /*
     * Write player location.
     */
    put_le32(stream, 1);
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    put_le32(stream, save->player_location.next_object_id);
    put_ref_id(stream, save->player_location.world_space1);
    put_le32(stream, save->player_location.coord_x);
    put_le32(stream, save->player_location.coord_y);
    put_ref_id(stream, save->player_location.world_space2);
    put_le32_ieee754(stream, save->player_location.pos_x);
    put_le32_ieee754(stream, save->player_location.pos_y);
    put_le32_ieee754(stream, save->player_location.pos_z);
    put_u8(stream, save->player_location.unknown); /* Present in Skyrim savefiles. */
    END_VARIABLE_LENGTH_BLOCK(stream)
    num_objects++;

    if (save->_private->global2) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 2);
        num_objects++;
    }

    /*
     * Write global variables.
     */
    put_le32(stream, 3);
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    encode_vsval(stream, save->num_global_vars);
    for (uint32_t i = 0u; i < save->num_global_vars; ++i) {
        put_ref_id(stream, save->global_vars[i].form_id);
        put_le32_ieee754(stream, save->global_vars[i].value);
    }
    END_VARIABLE_LENGTH_BLOCK(stream)
    num_objects++;

    if (save->_private->global4) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 4);
        num_objects++;
    }
    if (save->_private->global5) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 5);
        num_objects++;
    }

    /*
     * Write weather.
     */
    put_le32(stream, 6);
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    put_ref_id(stream, save->weather.climate);
    put_ref_id(stream, save->weather.weather);
    put_ref_id(stream, save->weather.prev_weather);
    put_ref_id(stream, save->weather.unk_weather1);
    put_ref_id(stream, save->weather.unk_weather2);
    put_ref_id(stream, save->weather.regn_weather);
    put_le32_ieee754(stream, save->weather.current_time);
    put_le32_ieee754(stream, save->weather.begin_time);
    put_le32_ieee754(stream, save->weather.weather_pct);
    for (size_t i = 0u; i < 6; ++i)
        put_le32(stream, save->weather.data1[i]);
    put_le32_ieee754(stream, save->weather.data2);
    put_le32(stream, save->weather.data3);
    put_u8(stream, save->weather.flags);
    fwrite(save->weather.data4, save->weather.data4_sz, 1, stream);
    END_VARIABLE_LENGTH_BLOCK(stream)
    num_objects++;

    if (save->_private->global7) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 7);
        num_objects++;
    }
    if (save->_private->global8) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 8);
        num_objects++;
    }

    return num_objects;
}

/* Write types 100 to 114. */
static int write_global_data_table2(FILE *restrict stream, const struct savegame *save)
{
    int num_objects = 0;

    if (save->_private->global100) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 100);
        num_objects++;
    }
    if (save->_private->global101) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 101);
        num_objects++;
    }
    if (save->_private->global102) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 102);
        num_objects++;
    }
    if (save->_private->global103) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 103);
        num_objects++;
    }
    if (save->_private->global104) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 104);
        num_objects++;
    }
    if (save->_private->global105) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 105);
        num_objects++;
    }
    if (save->_private->global106) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 106);
        num_objects++;
    }
    if (save->_private->global107) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 107);
        num_objects++;
    }
    if (save->_private->global108) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 108);
        num_objects++;
    }

    /*
     * Write magic favourites.
     */
    put_le32(stream, 109);
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    encode_vsval(stream, save->num_favourites);

    for (uint32_t i = 0u; i < save->num_favourites; ++i) {
        put_ref_id(stream, save->favourites[i]);
    }

    encode_vsval(stream, save->num_hotkeys);

    for (uint32_t i = 0u; i < save->num_hotkeys; ++i) {
        put_ref_id(stream, save->hotkeys[i]);
    }
    END_VARIABLE_LENGTH_BLOCK(stream)
    num_objects++;

    if (save->_private->global110) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 110);
        num_objects++;
    }
    if (save->_private->global111) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 111);
        num_objects++;
    }
    if (save->_private->global112) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 112);
        num_objects++;
    }
    if (save->_private->global113) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 113);
        num_objects++;
    }
    if (save->_private->global114) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 114);
        num_objects++;
    }

    return num_objects;
}

/* Write types 1000 to 1005. */
static int write_global_data_table3(FILE *restrict stream, const struct savegame *save)
{
    int num_objects = 0;

    if (save->_private->global1000) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1000);
        num_objects++;
    }
    if (save->_private->global1001) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1001);
        num_objects++;
    }
    if (save->_private->global1002) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1002);
        num_objects++;
    }
    if (save->_private->global1003) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1003);
        num_objects++;
    }
    if (save->_private->global1004) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1004);
        num_objects++;
    }
    if (save->_private->global1005) {
        WRITE_UNKNOWN_GLOBAL_DATA(stream, save, 1005);
        num_objects++;
    }

    return num_objects;
}

static void write_change_form(FILE *restrict stream,
        const struct change_form *restrict cf)
{
    put_ref_id(stream, cf->form_id);
    put_le32(stream, cf->flags);
    put_u8(stream, cf->type);
    put_u8(stream, cf->version);

    /* Two upper bits of type determine the sizes of length1 and length2. */
    switch ((cf->type >> 6) & 0x3) {
    case 0:
        put_u8(stream, cf->length1);
        put_u8(stream, cf->length2);
        break;
    case 1:
        put_le16(stream, cf->length1);
        put_le16(stream, cf->length2);
        break;
    case 2:
        put_le32(stream, cf->length1);
        put_le32(stream, cf->length2);
        break;
    default:
        /* Valid length types include uint8, uint16 and uint32. */
        assert(0);
    }

    fwrite(cf->data, cf->length1, 1, stream);
}

static int write_save_data(FILE *restrict stream, const struct savegame *save)
{
    struct location_table loc = {0};
    long location_table_offset;
    long end_of_save_data;

    DEBUG_LOG("save data begins at 0x%08lx\n", ftell(stream));

    put_u8(stream, save->_private->form_version);

    if (save->game == FALLOUT4) {
        put_le16_str(stream, save->game_version);
    }

    /*
     * Write plugin information.
     */
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    put_u8(stream, save->num_plugins);
    for (uint32_t i = 0; i < save->num_plugins; ++i) {
        put_le16_str(stream, save->plugins[i]);
    }

    if (supports_light_plugins(save)) {
        put_le16(stream, save->num_light_plugins);
        for (uint32_t i = 0; i < save->num_light_plugins; ++i) {
            put_le16_str(stream, save->light_plugins[i]);
        }
    }
    END_VARIABLE_LENGTH_BLOCK(stream)

    location_table_offset = ftell(stream);

    /* Locations table placeholder. */
    write_locations(stream, &loc);

    /*
     * Write global data table 1.
     */
    loc.off_globals1 = ftell(stream);
    loc.num_globals1 = write_global_data_table1(stream, save);

    /*
     * Write global data table 2.
     */
    loc.off_globals2 = ftell(stream);
    loc.num_globals2 = write_global_data_table2(stream, save);

    /*
     * Write change forms.
     */
    loc.off_change_forms = ftell(stream);
    loc.num_change_forms = save->_private->num_change_forms;
    for (uint32_t i = 0; i < save->_private->num_change_forms; ++i) {
        write_change_form(stream, &save->_private->change_forms[i]);
    }

    /*
     * Write global data table 3.
     */
    loc.off_globals3 = ftell(stream);
    loc.num_globals3 = write_global_data_table3(stream, save);

    /*
     * Write form IDs.
     */
    loc.off_form_ids_count = ftell(stream);
    put_le32(stream, save->num_form_ids);
    for (uint32_t i = 0; i < save->num_form_ids; ++i) {
        put_le32(stream, save->form_ids[i]);
    }

    /*
     * Write world spaces.
     */
    put_le32(stream, save->num_world_spaces);
    for (uint32_t i = 0; i < save->num_world_spaces; ++i) {
        put_le32(stream, save->world_spaces[i]);
    }

    /*
     * Write unknown table.
     */
    loc.off_unknown_table = ftell(stream);
    put_le32(stream, save->_private->unknown3->length);
    write_chunk(stream, save->_private->unknown3);

    end_of_save_data = ftell(stream);
    if (end_of_save_data == -1) {
        return CG_FTELL;
    }

    /*
     * Go back and fill in the location table placeholder with real values.
     */
    if (fseek(stream, location_table_offset, SEEK_SET) == -1) {
        return CG_FSEEK;
    }

    write_locations(stream, &loc);

    if (fseek(stream, end_of_save_data, SEEK_SET) == -1) {
        return CG_FSEEK;
    }

    if (ferror(stream)) {
        return CG_EOF;
    }

    return CG_OK;
}

static int write_save_file(FILE *restrict stream, const struct savegame *save)
{
    cg_err_t err;

    /*
     * Write signature.
     */
    switch (save->game) {
    case SKYRIM:
        fwrite(TESV_SIGNATURE, strlen(TESV_SIGNATURE), 1, stream);
        break;

    case FALLOUT4:
        fwrite(FO4_SIGNATURE, strlen(FO4_SIGNATURE), 1, stream);
        break;
   }

    /*
     * Write the file header.
     */
    BEGIN_VARIABLE_LENGTH_BLOCK(stream)
    put_le32(stream, save->_private->file_version);
    put_le32(stream, save->save_num);
    put_le16_str(stream, save->player_name);
    put_le32(stream, save->level);
    put_le16_str(stream, save->player_location_name);
    put_le16_str(stream, save->game_time);
    put_le16_str(stream, save->race_id);
    put_le16(stream, save->sex);
    put_le32_ieee754(stream, save->current_xp);
    put_le32_ieee754(stream, save->target_xp);
    put_le64(stream, save->filetime);
    put_le32(stream, save->snapshot_width);
    put_le32(stream, save->snapshot_height);
    if (supports_save_file_compression(save)) {
        put_le16(stream, save->_private->compressor);
    }
    END_VARIABLE_LENGTH_BLOCK(stream)

    /*
     * Write snapshot.
     */
    fwrite(save->snapshot_data, save->snapshot_size, 1, stream);

    /*
     * Write the body of the save file. If there is no support for save file
     * compression, continue writing to this stream directly. Otherwise, we'll
     * make a temporary stream and write the uncompressed data there. Then we'll
     * compress to the original stream.
     */
    if (!supports_save_file_compression(save)) {
        return write_save_data(stream, save);
    }

    struct {
        uint32_t size;
        char    *data;
    } uncompressed, compressed;
    long start_of_save_data, end_of_save_data;
    long save_data_size;
    FILE *temp_stream;
    int compress_rc;

    /* Get the starting point for data size calculation. */
    start_of_save_data = ftell(stream);

    if (start_of_save_data == -1) {
        perror("ftell");
        return CG_FTELL;
    }

    /* Open a stream to write the uncompressed save data. */
    temp_stream = tmpfile();

    if(!temp_stream) {
        perror("tmpfile");
        return CG_TMPFILE;
    }

    /*
     * To preserve file offsets, go to the same position as the
     * original stream is at.
     */
    if (fseek(temp_stream, start_of_save_data, SEEK_SET) == -1) {
        perror("fseek");
        fclose(temp_stream);
        return CG_FSEEK;
    }

    /* Write uncompressed save data. */
    err = write_save_data(temp_stream, save);

    if (err) {
        fclose(temp_stream);
        return err;
    }

    /* Get end position for data size calculation. */
    end_of_save_data = ftell(temp_stream);

    if (end_of_save_data == -1) {
        perror("ftell");
        fclose(temp_stream);
        return CG_FTELL;
    }

    save_data_size = end_of_save_data - start_of_save_data;

    /* Prepare to read the save data we just wrote. */
    if (fseek(temp_stream, start_of_save_data, SEEK_SET) == -1) {
        perror("fseek");
        fclose(temp_stream);
        return CG_FSEEK;
    }

    /*
     * Allocate buffers for compression.
     */
    uncompressed.size = save_data_size;
    uncompressed.data = malloc(uncompressed.size);
    compressed.size   = save_data_size;
    compressed.data   = malloc(compressed.size);

    if (!uncompressed.data || !compressed.data) {
        fclose(temp_stream);
        free(uncompressed.data);
        free(compressed.data);
        return CG_NO_MEM;
    }

    /* Read the save data. */
    if (fread(uncompressed.data, uncompressed.size, 1, temp_stream) != 1) {
        fclose(temp_stream);
        free(uncompressed.data);
        free(compressed.data);
        return CG_EOF;
    }

    dump_to_file("uncompressed_save_data_w", uncompressed.data, uncompressed.size, start_of_save_data);

    (void) fclose(temp_stream);

    /*
     * Compress.
     */
    switch (save->_private->compressor) {
    case NO_COMPRESSION:
        /* Swap compressed.data and uncompressed.data */
        char *temp_ptr    = compressed.data;
        compressed.data   = uncompressed.data;
        uncompressed.data = temp_ptr;
        compress_rc       = uncompressed.size;
        break;

    case LZ4:
        compress_rc = lz4_compress(uncompressed.data, compressed.data,
                                   uncompressed.size, compressed.size);
        break;

    case ZLIB:
        compress_rc = zlib_compress(uncompressed.data, compressed.data,
                                    uncompressed.size, compressed.size);
        break;

    default:
        BUG("invalid compressor");
        abort();
    }

    free(uncompressed.data);

    if (compress_rc == -1) {
        eprintf("compress error\n");
        free(compressed.data);
        return CG_CORRUPT;
    }

    compressed.size = compress_rc;

    DEBUG_LOG("save data length information begins at 0x%08lx\n", ftell(stream));

    put_le32(stream, save_data_size); /* uncompressed size */
    put_le32(stream, compressed.size);

    if (fwrite(compressed.data, compressed.size, 1, stream) != 1) {
        err = CG_EOF;
    }

    free(compressed.data);

    return err;
}

int cengine_savefile_write(const char *filename, const struct savegame *savegame)
{
    FILE *stream;
    cg_err_t err;

    if (!savegame->_private) {
        eprintf("It is an error to try to write a savegame that was not read before.\n");
        return -1;
    }

    stream = fopen(filename, "wb");

    if (!stream) {
        perror("fopen");
        return -1;
    }

    err = write_save_file(stream, savegame);

    if (fclose(stream) == EOF) {
        perror("fclose");
        return -1;
    }

    return err == CG_OK ? 0 : -1;
}

#if 0
static off_t get_file_size(int fd)
{
    struct stat statbuf;

    if (fstat(fd, &statbuf) == 0) {
        return statbuf.st_size;
    }
    else {
        return (off_t)-1;
    }
}

static void *mmap_entire_file_r(const char *filename, size_t *pfsize)
{
    void *fcontents;
    int save_errno;
    off_t fsize;
    int fd;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return MAP_FAILED;
    }

    fsize = get_file_size(fd);
    if (fsize == -1) {
        save_errno = errno;
        close(fd);
        errno = save_errno;
        return MAP_FAILED;
    }

    fcontents = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fcontents == MAP_FAILED) {
        save_errno = errno;
        close(fd);
        errno = save_errno;
        return MAP_FAILED;
    }

    close(fd);

    *pfsize = fsize;
    return fcontents;
}
#endif

static void free_savefile_private(struct savegame_private *private)
{
    if (!private) {
        return;
    }

    free(private->global2);
    free(private->global4);
    free(private->global5);
    free(private->global7);
    free(private->global8);
    free(private->global100);
    free(private->global101);
    free(private->global102);
    free(private->global103);
    free(private->global104);
    free(private->global105);
    free(private->global106);
    free(private->global107);
    free(private->global108);
    free(private->global110);
    free(private->global111);
    free(private->global112);
    free(private->global113);
    free(private->global114);
    free(private->global1000);
    free(private->global1001);
    free(private->global1002);
    free(private->global1003);
    free(private->global1004);
    free(private->global1005);

    if (private->change_forms) {
        for(unsigned i = 0; i < private->num_change_forms; ++i) {
            free(private->change_forms[i].data);
        }

        free(private->change_forms);
    }

    free(private->unknown3);
    free(private);
}

void savegame_free(struct savegame *save)
{
    unsigned i;

    free(save->player_name);
    free(save->player_location_name);
    free(save->game_time);
    free(save->race_id);
    free(save->snapshot_data);
    free(save->game_version);

    if (save->plugins) {
        for (i = 0u; i < save->num_plugins; ++i) {
            free(save->plugins[i]);
        }

        free(save->plugins);
    }

    if (save->light_plugins) {
        for (i = 0u; i < save->num_light_plugins; ++i) {
            free(save->light_plugins[i]);
        }

        free(save->light_plugins);
    }

    if (save->misc_stats) {
        for (i = 0; i < save->num_misc_stats; ++i) {
            free(save->misc_stats[i].name);
        }

        free(save->misc_stats);
    }

    free(save->global_vars);
    free(save->weather.data4);
    free(save->favourites);
    free(save->hotkeys);
    free(save->form_ids);
    free(save->world_spaces);
    free_savefile_private(save->_private);
    free(save);
}
