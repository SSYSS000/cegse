/*
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
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "binary_stream.h"
#include "compression.h"
#include "defines.h"
#include "mem_types.h"
#include "savefile.h"
#include "log.h"

#define FOR_REGION_CASTS(DO)                            \
    DO(struct block *, block_as_region)

#define FOR_CREGION_CASTS(DO)                           \
    DO(struct block *, const_block_as_cregion)

#include "mem_type_casts.h"

#define perror(str)              eprintf("%s:%d: %s: %s\n", __FILE__, __LINE__, str, strerror(errno))

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

#define LOCATION_TABLE_SIZE       100u

#define VSVAL_MAX                 4194303u

typedef enum cg_err {
    CG_OK = 0,
    CG_UNSUPPORTED,
    CG_EOF,
    CG_NO_MEM,
    CG_CORRUPT,
    CG_INVAL,
    CG_COMPRESS,
    CG_NOT_PRESENT
} cg_err_t;

enum object_type {
    OBJECT_FILE_HEADER,
    OBJECT_PLUGIN_INFO,

    /* Table 1 (0 - 11) */
#define FIRST_OBJECT_GLDA              OBJECT_GLDA_MISC_STATS
#define FIRST_TABLE1_OBJECT_GLDA       OBJECT_GLDA_MISC_STATS
    OBJECT_GLDA_MISC_STATS,
    OBJECT_GLDA_PLAYER_LOCATION,
    OBJECT_GLDA_GAME,
    OBJECT_GLDA_GLOBAL_VARIABLES,
    OBJECT_GLDA_CREATED_OBJECTS,
    OBJECT_GLDA_EFFECTS,
    OBJECT_GLDA_WEATHER,
    OBJECT_GLDA_AUDIO,
    OBJECT_GLDA_SKY_CELLS,
    OBJECT_GLDA_9,
    OBJECT_GLDA_10,
    OBJECT_GLDA_11,
    /* Table 2 (100 - 117) */
#define FIRST_TABLE2_OBJECT_GLDA       OBJECT_GLDA_PROCESS_LISTS
    OBJECT_GLDA_PROCESS_LISTS,
    OBJECT_GLDA_COMBAT,
    OBJECT_GLDA_INTERFACE,
    OBJECT_GLDA_ACTOR_CAUSES,
    OBJECT_GLDA_104,
    OBJECT_GLDA_DETECTION_MANAGER,
    OBJECT_GLDA_LOCATION_METADATA,
    OBJECT_GLDA_QUEST_STATIC_DATA,
    OBJECT_GLDA_STORYTELLER,
    OBJECT_GLDA_MAGIC_FAVORITES,
    OBJECT_GLDA_PLAYER_CONTROLS,
    OBJECT_GLDA_STORY_EVENT_MANAGER,
    OBJECT_GLDA_INGREDIENT_SHARED,
    OBJECT_GLDA_MENU_CONTROLS,
    OBJECT_GLDA_MENU_TOPIC_MANAGER,
    OBJECT_GLDA_115,
    OBJECT_GLDA_116,
    OBJECT_GLDA_117,
    /* Table 3 (1000 - 1007) */
#define FIRST_TABLE3_OBJECT_GLDA        OBJECT_GLDA_TEMP_EFFECTS
    OBJECT_GLDA_TEMP_EFFECTS,
    OBJECT_GLDA_PAPYRUS,
    OBJECT_GLDA_ANIM_OBJECTS,
    OBJECT_GLDA_TIMER,
    OBJECT_GLDA_SYNCHRONISED_ANIMS,
    OBJECT_GLDA_MAIN,
    OBJECT_GLDA_1006,
    OBJECT_GLDA_1007,
#define LAST_OBJECT_GLDA             OBJECT_GLDA_1007
#define OBJECT_GLDA_TYPE_COUNT       (LAST_OBJECT_GLDA - FIRST_OBJECT_GLDA + 1)

    OBJECT_TYPE_COUNT
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

struct psavegame {
    /*
     * Skyrim LE: 7,8,9
     * Skyrim SE: 12
     * Fallout 4: 11, 15
     */
    uint32_t file_version;
    uint8_t  form_version;

    /* The original savefile compression algorithm. */
    enum compressor compressor;

    unsigned n_change_forms;

    struct chunk *globals[OBJECT_GLDA_TYPE_COUNT];
    struct change_form *change_forms;
    struct chunk *unknown3; /* Data at the end of the savefile. */
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

enum block_type {
    /* Assembler adds block size (32-bit LE) only. */
    BLOCK_SIMPLE,

    /* Assembler adds global data type number. */
    BLOCK_GLOBAL_DATA,

    /* Assembler adds change form meta data. */
    BLOCK_CHANGE_FORM,
};

/*
 * Blocks always points to a location in a buffer where
 * an object is serialized to or deserialized from. NOT to the block header.
 *
 * When neither body nor block is not compressed, buffer points to file.
 * block ---->   file
 *       write   write header
 *
 * When only body is compressed, buffer points to body buffer.
 * block ----> buffer A    ->          file
 *       write           compress      write header
 *
 * When only block is compressed, buffer points to allocated buffer for block
 * and compress buffer points to file.
 *
 * block ----> buffer A    ->         file
 *        write           compress    write header
 *
 * When both are compressed:
 *
 * block ---->   buffer B   ->         buffer A        ->           file
 *        write           compress     write header  compress
 *
 */
struct block {
    /* Points to data, compressed or not.  */
    unsigned char *buffer;

    /* Buffer capacity. */
    unsigned buffer_size;

    /* The size of the data that buffer points to. */
    unsigned size;

    /* 
     * Only if >0, buffer points to compressed data and this denotes
     * the uncompressed size of the data.
     */
    uint32_t uncompressed_size;

    enum block_type block_type;
};

struct block_global_data {
    struct block base;
    uint32_t type_num;
};

struct block_change_form {
    struct block base;
    ref_t form_id;
    uint32_t flags;
    uint32_t type_num;
    uint32_t version;
};

static struct savegame *savegame_alloc(void);

static inline struct region block_as_region(struct block *block)
{
    return make_region(block->buffer, block->size);
}

static inline struct cregion const_block_as_cregion(const struct block *block)
{
    return make_cregion(block->buffer, block->size);
}

static cg_err_t file_writer(unsigned char *file, size_t *file_size_ptr, const struct savegame *save);
static cg_err_t file_reader(const unsigned char *file, size_t file_size, struct savegame *save);

static cg_err_t serializer(struct block *, const struct savegame *, enum object_type);
static cg_err_t deserializer(struct block *, struct savegame *, enum object_type);

/*
 * Compresses block buffer data to a new location _buffer_ and updates the
 * pointer and the size values accordingly. Block buffer will be set to point
 * to the new location.
 */
static cg_err_t compressor(struct block *, void *buffer, size_t buffer_size, enum compressor);
static cg_err_t decompressor(struct block *, void *buffer, size_t buffer_size, enum compressor);

/*
 * Final step that prepends the block header just before the location pointed
 * to by the block buffer. This means that there must be enough space there
 * for the header. Check with block_header_size().
 */
static void assembler(const struct block *, struct cursor *);
static cg_err_t disassembler(struct block *, struct cursor *);

static void print_locations_table(const struct location_table *t)
{
    DEBUG_LOG("\n"
       "+==============================================+\n"
       "|                Location table                |\n"
       "+==============================================+\n"
       "|   %8s    %6s       %-14s    |\n"
       "| 0x%08x    %6u       %-15s   |\n"
       "| 0x%08x    %6u       %-15s   |\n"
       "| 0x%08x    %6u       %-15s   |\n"
       "| 0x%08x    %6u       %-15s   |\n"
       "| 0x%08x    %6s       %-15s   |\n"
       "| 0x%08x    %6s       %-15s   |\n"
       "+==============================================+\n",
       "offset", "count", "item",
       t->off_globals1, t->num_globals1, "Globals 1",
       t->off_globals2, t->num_globals2, "Globals 2",
       t->off_change_forms, t->num_change_forms, "Change forms",
       t->off_globals3, t->num_globals3, "Globals 3",
       t->off_form_ids_count, "", "Form IDs",
       t->off_unknown_table, "", "Unknown table"
    );
}

static bool supports_save_file_compression(const struct savegame *save)
{
    return save->game == SKYRIM && save->priv->file_version >= 12;
}

static bool supports_light_plugins(const struct savegame *save)
{
    switch (save->game) {
    case SKYRIM:
        return save->priv->file_version >= 12 && save->priv->form_version >= 78;
    case FALLOUT4:
        return save->priv->file_version >= 12;
    }
    return false;
}

static unsigned snapshot_pixel_width(const struct savegame *save)
{
    return save->priv->file_version >= 11 ? 4 : 3;
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

#define GLDA_TYPE_NUMBER_LIST                           \
    X(0, OBJECT_GLDA_MISC_STATS)                        \
    X(1, OBJECT_GLDA_PLAYER_LOCATION)                   \
    X(2, OBJECT_GLDA_GAME)                              \
    X(3, OBJECT_GLDA_GLOBAL_VARIABLES)                  \
    X(4, OBJECT_GLDA_CREATED_OBJECTS)                   \
    X(5, OBJECT_GLDA_EFFECTS)                           \
    X(6, OBJECT_GLDA_WEATHER)                           \
    X(7, OBJECT_GLDA_AUDIO)                             \
    X(8, OBJECT_GLDA_SKY_CELLS)                         \
    X(9, OBJECT_GLDA_9)                                 \
    X(10, OBJECT_GLDA_10)                               \
    X(11, OBJECT_GLDA_11)                               \
    X(100, OBJECT_GLDA_PROCESS_LISTS)                   \
    X(101, OBJECT_GLDA_COMBAT)                          \
    X(102, OBJECT_GLDA_INTERFACE)                       \
    X(103, OBJECT_GLDA_ACTOR_CAUSES)                    \
    X(104, OBJECT_GLDA_104)                             \
    X(105, OBJECT_GLDA_DETECTION_MANAGER)               \
    X(106, OBJECT_GLDA_LOCATION_METADATA)               \
    X(107, OBJECT_GLDA_QUEST_STATIC_DATA)               \
    X(108, OBJECT_GLDA_STORYTELLER)                     \
    X(109, OBJECT_GLDA_MAGIC_FAVORITES)                 \
    X(110, OBJECT_GLDA_PLAYER_CONTROLS)                 \
    X(111, OBJECT_GLDA_STORY_EVENT_MANAGER)             \
    X(112, OBJECT_GLDA_INGREDIENT_SHARED)               \
    X(113, OBJECT_GLDA_MENU_CONTROLS)                   \
    X(114, OBJECT_GLDA_MENU_TOPIC_MANAGER)              \
    X(115, OBJECT_GLDA_115)                             \
    X(116, OBJECT_GLDA_116)                             \
    X(117, OBJECT_GLDA_117)                             \
    X(1000, OBJECT_GLDA_TEMP_EFFECTS)                   \
    X(1001, OBJECT_GLDA_PAPYRUS)                        \
    X(1002, OBJECT_GLDA_ANIM_OBJECTS)                   \
    X(1003, OBJECT_GLDA_TIMER)                          \
    X(1004, OBJECT_GLDA_SYNCHRONISED_ANIMS)             \
    X(1005, OBJECT_GLDA_MAIN)                           \
    X(1006, OBJECT_GLDA_1006)                           \
    X(1007, OBJECT_GLDA_1007)

static unsigned glda_type_number(enum object_type type)
{
#define X(number, object_type)  case object_type: return number;
    switch (type) {
        GLDA_TYPE_NUMBER_LIST
        default:
            assert(0); /* Invalid object type argument. */
            return 0;
    }
#undef X
}

static int object_type_from_glda_type_number(int type_number)
{
#define X(number, object_type)  case number: return object_type;
    switch (type_number) {
        GLDA_TYPE_NUMBER_LIST
        default:
            /* No object type for type_number. */
            return -1;
    }
#undef X
}

static cg_err_t encode_vsval(struct cursor *cursor, uint32_t value)
{
    int num_octets;

    if (value > VSVAL_MAX) {
        assert(0);
        return CG_INVAL;
    }

    /*
     * 2 least significant bits determine vsval size
     *
     *    2 LSB    | vsval size
     * ------------+------------
     *     00       |   1 octet
     *     01       |   2 octets
     *     10       |   3 octets
     */
    value <<= 2;               /* vsval size information takes 2 bits. */
    value &= 0xffffffu;        /* vsval max width is 24 bits. */
    num_octets = ((value >= 0x100u) << (value >= 0x10000u)) + 1;
    value |= num_octets - 1;   /* add size information */
    value = htole32(value);    /* vsval is little endian */

    c_store_bytes(cursor, &value, num_octets);
    return CG_OK;
}

static cg_err_t decode_vsval(struct cursor *cursor, uint32_t *out)
{
    uint8_t b[3] = {0};
    int i;

    /*
     * Loop at least once. Break when all octets have been read
     * or octet count is invalid.
     */
    for (i = 0; i < ((b[0] + 1) & 0x3); ++i) {
        if (!c_load_u8(cursor, &b[i])) {
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

static cg_err_t c_load_le16_str(struct cursor *cursor, char **string_out)
{
    uint16_t string_length;
    char *string;

    if (!c_load_le16(cursor, &string_length)) {
        return CG_EOF;
    }

    if (string_length >= 512) {
        DEBUG_LOG("Reading a very long string (%u chars).\n", string_length);
    }

    string = malloc((size_t)string_length + 1);
    if (!string) {
        return CG_NO_MEM;
    }

    if (!c_load_bytes(cursor, string, string_length)) {
        free(string);
        return CG_EOF;
    }

    string[string_length] = '\0';

    *string_out = string;
    return CG_OK;
}

static void c_store_le16_str(struct cursor *restrict cursor, const char *restrict string)
{
    size_t length = strlen(string);

    assert(length <= UINT16_MAX);

    c_store_le16(cursor, length);
    c_store_bytes(cursor, string, length);
}

static cg_err_t read_le16_str_array(struct cursor *cursor, char **array, int length)
{
    cg_err_t err = CG_OK;
    int i;

    for (i = 0; i < length; ++i) {
        err = c_load_le16_str(cursor, &array[i]);
        if (err) {
            break;
        }
    }

    if (err) {
        while (i--) {
            free(array[i]);
            array[i] = NULL;
        }
    }

    return err;
}

static cg_err_t alloc_and_read_chunk(struct cursor *cursor, struct chunk **chunk_out, size_t length)
{
    struct chunk *chunk;

    chunk = chunk_alloc(length);

    if (!chunk) {
        return CG_NO_MEM;
    }

    if (!c_load_bytes(cursor, chunk->data, length)) {
        free(chunk);
        return CG_EOF;
    }

    *chunk_out = chunk;

    return CG_OK;
}

static void dump_to_file(const char *filename, const void *data, size_t size, long offset)
{
    FILE *fp;

    if ((fp = fopen(filename, "w")) == NULL) {
        perror("fopen");
        return;
    }

    if (offset > 0) {
        fseek(fp, offset, SEEK_SET);
    }

    write_bytes(fp, data, size);
    fclose(fp);
}

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

struct savegame *cengine_savefile_read(const char *filename)
{
    struct savegame *save;
    size_t file_size = 0;
    unsigned char *file;
    cg_err_t err;

    file = mmap_entire_file_r(filename, &file_size);
    if (!file) {
        perror("mmap");
        return NULL;
    }

    if ((save = savegame_alloc()) == NULL) {
        munmap(file, file_size);
        return NULL;
    }

    DEBUG_LOG("Reading save file %s\n", filename);

    err = file_reader(file, file_size, save);
    switch (err) {
    case CG_UNSUPPORTED:
        eprintf("File cannot be read because its format is unsupported.\n");
        break;
    case CG_EOF:
        eprintf("File ended too soon. Is the save corrupt?\n");
        break;
    case CG_CORRUPT:
        eprintf("File may be corrupt.\n");
        break;
    case CG_NO_MEM:
        eprintf("Failed to allocate memory.\n");
        break;
    case CG_COMPRESS:
        eprintf("Compression error.\n");
        break;
    case CG_INVAL:
        eprintf("Invalid argument.\n");
        break;
    case CG_NOT_PRESENT:
        /* bug */
        break;
    case CG_OK:
        /* No error. */
        break;
    }

    munmap(file, file_size);

    if (err) {
        DEBUG_LOG("Error %d occurred while reading save file\n", err);
        savegame_free(save);
        return NULL;
    }

    return save;
}

static cg_err_t
file_reader(const unsigned char *file, size_t file_size, struct savegame *save)
{
    struct location_table locations;
    struct chunk *buffers[1] = {0};
    struct cursor file_cursor;
    struct cursor body_cursor;
    struct cursor *cursor;
    intptr_t offset_var = (intptr_t)file;
    cg_err_t err = CG_OK;

    union {
        struct block simple;
        struct block_change_form chfo;
        struct block_global_data glda;
    } block_buf = {0};
    struct block *block = &block_buf.simple;

    if (file_size < 300) {
        return CG_CORRUPT;
    }

    file_cursor.pos = (unsigned char *) file;
    file_cursor.n = file_size;
    cursor = &file_cursor;

    /* Calculates current file offset at cursor position. */
    #define OFFSET() ((unsigned long)((intptr_t)cursor->pos - offset_var))

    /*
     * Check the file signature.
     */
    {
        if (!memcmp(file, TESV_SIGNATURE, strlen(TESV_SIGNATURE))) {
            DEBUG_LOG("TESV file signature detected\n");
            save->game = SKYRIM;
            c_advance(cursor, strlen(TESV_SIGNATURE));
        }
        else if(!memcmp(file, FO4_SIGNATURE, strlen(FO4_SIGNATURE))) {
            DEBUG_LOG("Fallout 4 file signature detected\n");
            save->game = FALLOUT4;
            c_advance(cursor, strlen(FO4_SIGNATURE));
            //return CG_UNSUPPORTED;
        }
        else {
            DEBUG_LOG("File not recognized\n");
            err = CG_UNSUPPORTED;
            goto out_error;
        }
    }

    /*
     * Read the file header.
     */
    DEBUG_LOG("0x%08lx: Reading file header\n", OFFSET());
    block->block_type = BLOCK_SIMPLE;
    err = disassembler(block, cursor);
    if (err) {
        goto out_error;
    }
    err = deserializer(block, save, OBJECT_FILE_HEADER);
    if (err) {
        goto out_error;
    }

    DEBUG_LOG("File version: %u\n", save->priv->file_version);
    if (save->priv->file_version > 15) {
        err = CG_UNSUPPORTED;
        goto out_error;
    }

    /*
     * Read the snapshot.
     */
    save->snapshot_bytes_per_pixel = snapshot_pixel_width(save);
    save->snapshot_size = save->snapshot_width * save->snapshot_height *
                          save->snapshot_bytes_per_pixel;
    save->snapshot_data = malloc(save->snapshot_size);
    if (!save->snapshot_data) {
        err = CG_NO_MEM;
        goto out_error;
    }

    DEBUG_LOG("0x%08lx: Reading %u bytes of snapshot data\n", OFFSET(), save->snapshot_size);

    c_load_bytes(cursor, save->snapshot_data, save->snapshot_size);

    /* Initialize body cursor. */
    if (supports_save_file_compression(save)) {
        ssize_t uncompress_size;
        ssize_t compress_size;

        uncompress_size = c_load_le32_or0(cursor);
        compress_size = c_load_le32_or0(cursor);
        if (cursor->n < 0) {
            err = CG_EOF;
            goto out_error;
        }

        DEBUG_LOG("Save data uncompressed length: %zd\n", uncompress_size);
        DEBUG_LOG("Save data compressed length:   %zd\n", compress_size);

        if (save->priv->compressor != NO_COMPRESSION) {
            decompress_fn_t decompress = NULL;
            ssize_t decompress_size = 0;

            /* Allocate space for decompression. */
            buffers[0] = chunk_alloc(uncompress_size);
            if (!buffers[0]) {
                err = CG_NO_MEM;
                goto out_error;
            }

            /* Select decompress method. */
            switch (save->priv->compressor) {
            case LZ4:
                decompress = lz4_decompress;
                break;
            case ZLIB:
                decompress = zlib_decompress;
                break;
            case NO_COMPRESSION:
                abort();
                break;
            }

            /* Decompress. */
            struct cregion src = make_cregion(cursor->pos, compress_size);
            struct region dest = make_region(buffers[0]->data, buffers[0]->size);
            DEBUG_LOG("Decompressing save data\n");
            decompress_size = decompress(src, dest);
            if (decompress_size == -1) {
                err = CG_COMPRESS;
                goto out_error;
            }

            DEBUG_LOG("Dumping decompressed save data\n");
            dump_to_file("decompressed_save_data", dest.data, dest.size, OFFSET());

            body_cursor.pos = dest.data;
            body_cursor.n = decompress_size;
            offset_var = (intptr_t)body_cursor.pos - (file_cursor.pos - file);
        }
        else {
            /* 
             * No compression required. We will directly write to file, but
             * need to leave 8 bytes of space to store size information.
             */
            c_advance2(&body_cursor, &file_cursor, 8);
            offset_var += 8;
        }
    }
    else {
        /*
         * Game engine doesn't support compression for the save file.
         * We will continue reading where we left off.
         */
        c_advance2(&body_cursor, &file_cursor, 0);
    }

    cursor = &body_cursor;

    DEBUG_LOG("0x%08lx: Save data begins\n", OFFSET());

    if (!c_load_u8(cursor, &save->priv->form_version)) {
        err = CG_EOF;
        goto out_error;
    }

    DEBUG_LOG("Save data form version: %u\n", save->priv->form_version);

    if (save->game == FALLOUT4) {
        err = c_load_le16_str(cursor, &save->game_version);
        if (err) {
            goto out_error;
        }
    }

    /*
     * Read plugin information.
     */
    block->block_type = BLOCK_SIMPLE;
    err = disassembler(block, cursor);
    if (err) {
        goto out_error;
    }
    err = deserializer(block, save, OBJECT_PLUGIN_INFO);
    if (err) {
        goto out_error;
    }

    /*
     * Read location table.
     */
    DEBUG_LOG("0x%08lx: Reading locations table\n", OFFSET());
    locations.off_form_ids_count = c_load_le32_or0(cursor);
    locations.off_unknown_table = c_load_le32_or0(cursor);
    locations.off_globals1 = c_load_le32_or0(cursor);
    locations.off_globals2 = c_load_le32_or0(cursor);
    locations.off_change_forms = c_load_le32_or0(cursor);
    locations.off_globals3 = c_load_le32_or0(cursor);
    locations.num_globals1 = c_load_le32_or0(cursor);
    locations.num_globals2 = c_load_le32_or0(cursor);
    locations.num_globals3 = c_load_le32_or0(cursor);
    locations.num_change_forms = c_load_le32_or0(cursor);
    c_advance(cursor, 60); /* Skip junk. */

    locations.num_globals3 += 1; /* Count is bugged and short by 1. */
    print_locations_table(&locations);

    save->priv->n_change_forms = locations.num_change_forms;

    /*
     * Read global data table 1 and 2.
     */
    DEBUG_LOG("0x%08lx: Reading global data table 1 and 2\n", OFFSET());

    block->block_type = BLOCK_GLOBAL_DATA;
    for(unsigned i = 0; i < locations.num_globals1+locations.num_globals2; ++i) {
        struct block_global_data *glda = (struct block_global_data *) block;
        int object_type;

        err = disassembler(block, cursor);
        if (err) {
            goto out_error;
        }

        object_type = object_type_from_glda_type_number(glda->type_num);
        if (object_type == -1) {
            /* Invalid type number. */
            err = CG_CORRUPT;
            goto out_error;
        }

        err = deserializer(block, save, object_type);
        if (err) {
            goto out_error;
        }
    }

    /*
     * Read change forms.
     */
    save->priv->change_forms = calloc(locations.num_change_forms, sizeof(*save->priv->change_forms));
    if (!save->priv->change_forms) {
        err = CG_NO_MEM;
        goto out_error;
    }

    DEBUG_LOG("0x%08lx: Reading %u change forms\n", OFFSET(), locations.num_change_forms);

    block->block_type = BLOCK_CHANGE_FORM;
    for (unsigned i = 0; i < locations.num_change_forms; ++i) {
        struct change_form *cf = &save->priv->change_forms[i];

        err = disassembler(block, cursor);
        if (err) {
            goto out_error;
        }

        cf->flags = block_buf.chfo.flags;
        cf->form_id = block_buf.chfo.form_id;
        cf->version = block_buf.chfo.version;
        cf->type = block_buf.chfo.type_num;
        cf->length1 = block->size;
        cf->length2 = block->uncompressed_size;
        cf->data = malloc(block->size);
        if (!cf->data) {
            err = CG_NO_MEM;
            goto out_error;
        }

        memcpy(cf->data, block->buffer, block->size);
    }

    /*
     * Read global data table 3.
     */
    DEBUG_LOG("0x%08lx: Reading global data table 3\n", OFFSET());

    block->block_type = BLOCK_GLOBAL_DATA;
    for(unsigned i = 0; i < locations.num_globals3; ++i) {
        struct block_global_data *glda = (struct block_global_data *) block;
        int object_type;

        err = disassembler(block, cursor);
        if (err) {
            goto out_error;
        }

        object_type = object_type_from_glda_type_number(glda->type_num);
        if (object_type == -1) {
            /* Invalid type number. */
            err = CG_CORRUPT;
            goto out_error;
        }

        err = deserializer(block, save, object_type);
        if (err) {
            goto out_error;
        }
    }

    /*
     * Read form IDs.
     */
    DEBUG_LOG("0x%08lx: Reading %u form IDs\n", OFFSET(), save->num_form_ids);
    if (!c_load_le32(cursor, &save->num_form_ids)) {
        err = CG_EOF;
        goto out_error;
    }

    save->form_ids = calloc(save->num_form_ids, sizeof(*save->form_ids));
    if (!save->form_ids) {
        err = CG_NO_MEM;
        goto out_error;
    }

    for (uint32_t i = 0; i < save->num_form_ids; ++i)
        c_load_le32(cursor, &save->form_ids[i]);

    /*
     * Read world spaces.
     */
    DEBUG_LOG("0x%08lx: Reading %u world spaces\n", OFFSET(), save->num_world_spaces);
    if (!c_load_le32(cursor, &save->num_world_spaces)) {
        return CG_EOF;
    }

    save->world_spaces = calloc(save->num_world_spaces, sizeof(*save->world_spaces));
    if (!save->world_spaces) {
        err = CG_NO_MEM;
        goto out_error;
    }

    for (uint32_t i = 0; i < save->num_world_spaces; ++i)
        c_load_le32(cursor, &save->world_spaces[i]);

    /*
     * Read the unknown chunk at the end of the savefile.
     */
    {
        uint32_t size;

        DEBUG_LOG("0x%08lx: Reading unknown table\n", OFFSET());

        if (!c_load_le32(cursor, &size)) {
            err = CG_EOF;
            goto out_error;
        }

        err = alloc_and_read_chunk(cursor, &save->priv->unknown3, size);
        if (err) {
            goto out_error;
        }
    }

out_error:
    for (size_t i = 0; i < ARRAY_LEN(buffers); ++i) {
        free(buffers[i]);
    }

    return err;
#undef OFFSET
}

static unsigned block_header_size(const struct block *block)
{
    struct block_change_form *cf;

    switch (block->block_type) {
    case BLOCK_SIMPLE:
        return 4;
    case BLOCK_GLOBAL_DATA:
        return 8;
    case BLOCK_CHANGE_FORM:
        cf = (struct block_change_form *) block;
        return 9 + (2 << ((cf->type_num >> 6) & 0x3));
    }

    return 0;
}

static cg_err_t disassembler(struct block *block, struct cursor *cursor)
{
    struct block_change_form *cf;

    switch (block->block_type) {
    case BLOCK_GLOBAL_DATA:
        ((struct block_global_data *)block)->type_num = c_load_le32_or0(cursor);
        /* fallthrough */
    case BLOCK_SIMPLE:
        block->size = c_load_le32_or0(cursor);
        break;

    case BLOCK_CHANGE_FORM:
        cf = (struct block_change_form *) block;
        cf->form_id = c_load_be24_or0(cursor);
        cf->flags = c_load_le32_or0(cursor);
        cf->type_num = c_load_u8_or0(cursor);
        cf->version = c_load_u8_or0(cursor);
        if (cursor->n < 0) {
            break;
        }

        /* 
         * Two upper bits of type determine the sizes of size information.
         */
        switch ((cf->type_num >> 6) & 0x3) {
        case 0:
            block->size = c_load_u8_or0(cursor);
            block->uncompressed_size = c_load_u8_or0(cursor);
            break;
        case 1:
            block->size = c_load_le16_or0(cursor);
            block->uncompressed_size = c_load_le16_or0(cursor);
            break;
        case 2:
            block->size = c_load_le32_or0(cursor);
            block->uncompressed_size = c_load_le32_or0(cursor);
            break;
        default:
            return CG_CORRUPT;
        }
        break;
    }
    
    /* Unless readed header successfully and block is completely in buffer: */
    if (cursor->n < (long)block->size) {
        /* Bug or corrupt. */
        return CG_EOF;
    }

    block->buffer = cursor->pos;
    block->buffer_size = cursor->n;
    c_advance(cursor, block->size);

    return CG_OK;
}

__attribute__((unused))
static cg_err_t decompressor(struct block *block, void *buffer,
        size_t buffer_size, enum compressor compressor_type)
{
    decompress_fn_t decompress;
    ssize_t decompress_size;

    switch (compressor_type) {
    case LZ4:
        decompress = lz4_decompress;
        break;
    case ZLIB:
        decompress = zlib_decompress;
        break;
    default:
        return CG_INVAL;
    }

    decompress_size = decompress(as_cregion(block), make_region(buffer, buffer_size));
    if (decompress_size == -1) {
        /* Buffer too small? */
        return CG_COMPRESS;
    }

    /* Update values. */
    block->uncompressed_size = 0;
    block->size = decompress_size;
    block->buffer_size = buffer_size;
    block->buffer = buffer;

    return CG_OK;
}

#if defined(COMPILE_WITH_UNIT_TESTS)
static struct chunk *unit_test_file_objects[OBJECT_TYPE_COUNT];
#endif

static cg_err_t
deserializer(struct block *block, struct savegame *save, enum object_type object_type)
{
    struct cursor *cursor = (struct cursor[]) {{
        block->buffer,
        block->size
    }};
    cg_err_t err = CG_OK;

#if defined(COMPILE_WITH_UNIT_TESTS)
    if (unit_test_file_objects[object_type]) {
        free(unit_test_file_objects[object_type]);
    }

    /* Copy this block for unit testing. */
    unit_test_file_objects[object_type] = chunk_alloc(block->size);
    if (!unit_test_file_objects[object_type]) {
        perror("chunk_alloc");
        exit(1);
    }

    memcpy(unit_test_file_objects[object_type]->data, block->buffer, block->size);
#endif

    switch (object_type) {
    case OBJECT_FILE_HEADER:
        if (!c_load_le32(cursor, &save->priv->file_version)) {
            err = CG_EOF;
            break;
        }

        save->save_num = c_load_le32_or0(cursor);
        err = c_load_le16_str(cursor, &save->player_name);
        if (err) break;
        save->level = c_load_le32_or0(cursor);
        err = c_load_le16_str(cursor, &save->player_location_name);
        if (err) break;
        err = c_load_le16_str(cursor, &save->game_time);
        if (err) break;
        err = c_load_le16_str(cursor, &save->race_id);
        if (err) break;
        save->sex = c_load_le16_or0(cursor);
        save->current_xp = c_load_lef32_or0(cursor);
        save->target_xp = c_load_lef32_or0(cursor);
        save->filetime = c_load_le64_or0(cursor);
        save->snapshot_width = c_load_le32_or0(cursor);
        save->snapshot_height = c_load_le32_or0(cursor);

        if (supports_save_file_compression(save)) {
            /*
             * Read compression type and validate it.
             */
            uint16_t compression_type;

            if (!c_load_le16(cursor, &compression_type)) {
                err = CG_EOF;
                break;
            }

            if (!(compression_type <= 2)) {
                DEBUG_LOG("Read an invalid compression type: %u\n", compression_type);
                err = CG_CORRUPT;
                break;
            }

            save->priv->compressor = compression_type;

            DEBUG_LOG("Save file compression algorithm = %s.\n",
                      compression_type == LZ4 ? "lz4" :
                      compression_type == ZLIB ? "zlib" : "none");
        }
        else {
            DEBUG_LOG("No save file compression support\n");
        }
        break;

    case OBJECT_PLUGIN_INFO:
        /* Regular plugins. */
        if (!c_load_u8(cursor, &save->num_plugins)) {
            err = CG_EOF;
            break;
        }

        save->plugins = calloc(save->num_plugins, sizeof(*save->plugins));
        if (!save->plugins) {
            err = CG_NO_MEM;
            break;
        }

        err = read_le16_str_array(cursor, save->plugins, save->num_plugins);
        if (err) {
            break;
        }

        /* Light plugins. */
        if (supports_light_plugins(save)) {
            if (!c_load_le16(cursor, &save->num_light_plugins)) {
                err = CG_EOF;
                break;
            }

            save->light_plugins = calloc(save->num_light_plugins, sizeof(*save->light_plugins));
            if (!save->light_plugins) {
                err = CG_NO_MEM;
                break;
            }

            err = read_le16_str_array(cursor, save->light_plugins, save->num_light_plugins);
        }
        break;

    case OBJECT_GLDA_MISC_STATS:
        save->num_misc_stats = c_load_le32_or0(cursor);
        if (save->num_misc_stats == 0) {
            break;
        }

        save->misc_stats = calloc(save->num_misc_stats, sizeof(*save->misc_stats));
        if (!save->misc_stats) {
            err = CG_NO_MEM;
            break;
        }

        for (uint32_t i = 0u; i < save->num_misc_stats; ++i) {
            err = c_load_le16_str(cursor, &save->misc_stats[i].name);
            if (err) {
                break;
            }

            save->misc_stats[i].category = c_load_u8_or0(cursor);
            save->misc_stats[i].value    = c_load_le32_or0(cursor);
        }
        break;

    case OBJECT_GLDA_PLAYER_LOCATION:
        save->player_location.next_object_id = c_load_le32_or0(cursor);
        save->player_location.world_space1   = c_load_be24_or0(cursor);
        save->player_location.coord_x        = (int32_t)c_load_le32_or0(cursor);
        save->player_location.coord_y        = (int32_t)c_load_le32_or0(cursor);
        save->player_location.world_space2   = c_load_be24_or0(cursor);
        save->player_location.pos_x          = c_load_lef32_or0(cursor);
        save->player_location.pos_y          = c_load_lef32_or0(cursor);
        save->player_location.pos_z          = c_load_lef32_or0(cursor);
        if (save->game == SKYRIM) {
            save->player_location.unknown    = c_load_u8_or0(cursor); /* Skyrim only */
        }
        break;

    case OBJECT_GLDA_GLOBAL_VARIABLES:
        err = decode_vsval(cursor, &save->num_global_vars);
        if (err || save->num_global_vars == 0) {
            break;
        }

        save->global_vars = calloc(save->num_global_vars, sizeof(*save->global_vars));
        if (!save->global_vars) {
            err = CG_NO_MEM;
            break;
        }

        for (uint32_t i = 0u; i < save->num_global_vars; ++i) {
            save->global_vars[i].form_id = c_load_be24_or0(cursor);
            save->global_vars[i].value   = c_load_lef32_or0(cursor);
        }
        break;

    case OBJECT_GLDA_WEATHER:
        save->weather.climate      = c_load_be24_or0(cursor);
        save->weather.weather      = c_load_be24_or0(cursor);
        save->weather.prev_weather = c_load_be24_or0(cursor);
        save->weather.unk_weather1 = c_load_be24_or0(cursor);
        save->weather.unk_weather2 = c_load_be24_or0(cursor);
        save->weather.regn_weather = c_load_be24_or0(cursor);
        save->weather.current_time = c_load_lef32_or0(cursor);
        save->weather.begin_time   = c_load_lef32_or0(cursor);
        save->weather.weather_pct  = c_load_lef32_or0(cursor);
        for (size_t i = 0u; i < 6; ++i)
            save->weather.data1[i] = c_load_le32_or0(cursor);
        save->weather.data2        = c_load_lef32_or0(cursor);
        save->weather.data3        = c_load_le32_or0(cursor);
        save->weather.flags        = c_load_u8_or0(cursor);

        /* Read the remaining bytes. Don't know what they are. */
        if (cursor->n > 0) {
            save->weather.data4 = chunk_alloc(cursor->n);
            if (!save->weather.data4) {
                err = CG_NO_MEM;
                break;
            }
            c_load_bytes(cursor, save->weather.data4->data, save->weather.data4->size);
        }
        break;

    case OBJECT_GLDA_MAGIC_FAVORITES:
        err = decode_vsval(cursor, &save->num_favourites);
        if (err) {
            break;
        }

        save->favourites = calloc(save->num_favourites, sizeof(*save->favourites));
        if (!save->favourites) {
            err = CG_NO_MEM;
            break;
        }

        for (uint32_t i = 0; i < save->num_favourites; ++i) {
            save->favourites[i] = c_load_be24_or0(cursor);
        }

        err = decode_vsval(cursor, &save->num_hotkeys);
        if (err) {
            break;
        }

        save->hotkeys = calloc(save->num_hotkeys, sizeof(*save->hotkeys));
        if (!save->hotkeys) {
            err = CG_NO_MEM;
            break;
        }

        for (uint32_t i = 0; i < save->num_hotkeys; ++i) {
            save->hotkeys[i] = c_load_be24_or0(cursor);
        }
        break;

    case OBJECT_GLDA_GAME:
    case OBJECT_GLDA_CREATED_OBJECTS:
    case OBJECT_GLDA_EFFECTS:
    case OBJECT_GLDA_AUDIO:
    case OBJECT_GLDA_SKY_CELLS:
    case OBJECT_GLDA_9:
    case OBJECT_GLDA_10:
    case OBJECT_GLDA_11:
    case OBJECT_GLDA_PROCESS_LISTS:
    case OBJECT_GLDA_COMBAT:
    case OBJECT_GLDA_INTERFACE:
    case OBJECT_GLDA_ACTOR_CAUSES:
    case OBJECT_GLDA_104:
    case OBJECT_GLDA_DETECTION_MANAGER:
    case OBJECT_GLDA_LOCATION_METADATA:
    case OBJECT_GLDA_QUEST_STATIC_DATA:
    case OBJECT_GLDA_STORYTELLER:
    case OBJECT_GLDA_PLAYER_CONTROLS:
    case OBJECT_GLDA_STORY_EVENT_MANAGER:
    case OBJECT_GLDA_INGREDIENT_SHARED:
    case OBJECT_GLDA_MENU_CONTROLS:
    case OBJECT_GLDA_MENU_TOPIC_MANAGER:
    case OBJECT_GLDA_115:
    case OBJECT_GLDA_116:
    case OBJECT_GLDA_117:
    case OBJECT_GLDA_TEMP_EFFECTS:
    case OBJECT_GLDA_PAPYRUS:
    case OBJECT_GLDA_ANIM_OBJECTS:
    case OBJECT_GLDA_TIMER:
    case OBJECT_GLDA_SYNCHRONISED_ANIMS:
    case OBJECT_GLDA_MAIN:
    case OBJECT_GLDA_1006:
    case OBJECT_GLDA_1007:
        {
            /*
             * Read an unknown global data structure.
             */
            struct chunk **slot;

            slot = &save->priv->globals[object_type - FIRST_OBJECT_GLDA];

            if (*slot != NULL) {
                eprintf("encountered multiple global data of type %u\n", object_type);
                err = CG_CORRUPT;
                break;
            }

            err = alloc_and_read_chunk(cursor, slot, cursor->n);
            break;
        }

    default:
        break;
    }

    if (cursor->n < 0) {
        /* Bug or corrupt. */
        DEBUG_LOG("CG_EOF while deserializing object type %d. "
                "cursor->n = %lld\n",
                (int)object_type, cursor->n);
        return CG_EOF;
    }

    return err;
}

static void prepare_block(struct block *block, struct cursor *cursor)
{
    unsigned header_size = block_header_size(block);
    block->buffer = cursor->pos + header_size;
    block->buffer_size = cursor->n - header_size;
}

static cg_err_t
serializer(struct block *block, const struct savegame *save, enum object_type object_type)
{
    struct cursor *cursor = (struct cursor[]) {{
        block->buffer,
        block->buffer_size
    }};
    cg_err_t err = CG_OK;

    switch (object_type) {
    case OBJECT_FILE_HEADER:
        c_store_le32(cursor, save->priv->file_version);
        c_store_le32(cursor, save->save_num);
        c_store_le16_str(cursor, save->player_name);
        c_store_le32(cursor, save->level);
        c_store_le16_str(cursor, save->player_location_name);
        c_store_le16_str(cursor, save->game_time);
        c_store_le16_str(cursor, save->race_id);
        c_store_le16(cursor, save->sex);
        c_store_lef32(cursor, save->current_xp);
        c_store_lef32(cursor, save->target_xp);
        c_store_le64(cursor, save->filetime);
        c_store_le32(cursor, save->snapshot_width);
        c_store_le32(cursor, save->snapshot_height);
        if (supports_save_file_compression(save)) {
            c_store_le16(cursor, save->priv->compressor);
        }
        break;

    case OBJECT_PLUGIN_INFO:
        c_store_u8(cursor, save->num_plugins);
        for (uint32_t i = 0; i < save->num_plugins; ++i) {
            c_store_le16_str(cursor, save->plugins[i]);
        }

        if (supports_light_plugins(save)) {
            c_store_le16(cursor, save->num_light_plugins);
            for (uint32_t i = 0; i < save->num_light_plugins; ++i) {
                c_store_le16_str(cursor, save->light_plugins[i]);
            }
        }
        break;

    case OBJECT_GLDA_MISC_STATS:
        c_store_le32(cursor, save->num_misc_stats);
        for (uint32_t i = 0u; i < save->num_misc_stats; ++i) {
            c_store_le16_str(cursor, save->misc_stats[i].name);
            c_store_u8(cursor,       save->misc_stats[i].category);
            c_store_le32(cursor,     save->misc_stats[i].value);
        }
        break;

    case OBJECT_GLDA_PLAYER_LOCATION:
        c_store_le32(cursor, save->player_location.next_object_id);
        c_store_be24(cursor, save->player_location.world_space1);
        c_store_le32(cursor, save->player_location.coord_x);
        c_store_le32(cursor, save->player_location.coord_y);
        c_store_be24(cursor, save->player_location.world_space2);
        c_store_lef32(cursor, save->player_location.pos_x);
        c_store_lef32(cursor, save->player_location.pos_y);
        c_store_lef32(cursor, save->player_location.pos_z);
        c_store_u8(cursor, save->player_location.unknown); /* Present in Skyrim savefiles. */
        break;

    case OBJECT_GLDA_GLOBAL_VARIABLES:
        encode_vsval(cursor, save->num_global_vars);
        for (uint32_t i = 0u; i < save->num_global_vars; ++i) {
            c_store_be24(cursor, save->global_vars[i].form_id);
            c_store_lef32(cursor, save->global_vars[i].value);
        }
        break;

    case OBJECT_GLDA_WEATHER:
        c_store_be24(cursor, save->weather.climate);
        c_store_be24(cursor, save->weather.weather);
        c_store_be24(cursor, save->weather.prev_weather);
        c_store_be24(cursor, save->weather.unk_weather1);
        c_store_be24(cursor, save->weather.unk_weather2);
        c_store_be24(cursor, save->weather.regn_weather);
        c_store_lef32(cursor, save->weather.current_time);
        c_store_lef32(cursor, save->weather.begin_time);
        c_store_lef32(cursor, save->weather.weather_pct);
        for (int i = 0; i < 6; ++i)
            c_store_le32(cursor, save->weather.data1[i]);
        c_store_lef32(cursor, save->weather.data2);
        c_store_le32(cursor, save->weather.data3);
        c_store_u8(cursor, save->weather.flags);
        if (save->weather.data4)
            c_store_bytes(cursor, save->weather.data4->data, save->weather.data4->size);
        break;

    case OBJECT_GLDA_MAGIC_FAVORITES:
        encode_vsval(cursor, save->num_favourites);
        for (uint32_t i = 0u; i < save->num_favourites; ++i) {
            c_store_be24(cursor, save->favourites[i]);
        }
        encode_vsval(cursor, save->num_hotkeys);
        for (uint32_t i = 0u; i < save->num_hotkeys; ++i) {
            c_store_be24(cursor, save->hotkeys[i]);
        }
        break;

    case OBJECT_GLDA_GAME:
    case OBJECT_GLDA_CREATED_OBJECTS:
    case OBJECT_GLDA_EFFECTS:
    case OBJECT_GLDA_AUDIO:
    case OBJECT_GLDA_SKY_CELLS:
    case OBJECT_GLDA_9:
    case OBJECT_GLDA_10:
    case OBJECT_GLDA_11:
    case OBJECT_GLDA_PROCESS_LISTS:
    case OBJECT_GLDA_COMBAT:
    case OBJECT_GLDA_INTERFACE:
    case OBJECT_GLDA_ACTOR_CAUSES:
    case OBJECT_GLDA_104:
    case OBJECT_GLDA_DETECTION_MANAGER:
    case OBJECT_GLDA_LOCATION_METADATA:
    case OBJECT_GLDA_QUEST_STATIC_DATA:
    case OBJECT_GLDA_STORYTELLER:
    case OBJECT_GLDA_PLAYER_CONTROLS:
    case OBJECT_GLDA_STORY_EVENT_MANAGER:
    case OBJECT_GLDA_INGREDIENT_SHARED:
    case OBJECT_GLDA_MENU_CONTROLS:
    case OBJECT_GLDA_MENU_TOPIC_MANAGER:
    case OBJECT_GLDA_115:
    case OBJECT_GLDA_116:
    case OBJECT_GLDA_117:
    case OBJECT_GLDA_TEMP_EFFECTS:
    case OBJECT_GLDA_PAPYRUS:
    case OBJECT_GLDA_ANIM_OBJECTS:
    case OBJECT_GLDA_TIMER:
    case OBJECT_GLDA_SYNCHRONISED_ANIMS:
    case OBJECT_GLDA_MAIN:
    case OBJECT_GLDA_1006:
    case OBJECT_GLDA_1007:
        {
            /*
            * Unknown global data structure.
            */
            struct chunk *chunk;

            chunk = save->priv->globals[object_type - FIRST_OBJECT_GLDA];

            if (chunk == NULL) {
                err = CG_NOT_PRESENT;
                break;
            }

            c_store_bytes(cursor, chunk->data, chunk->size);
            break;
        }
    default:
        err = CG_NOT_PRESENT;
        break;
    }

    if (cursor->n < 0) {
        /* Buffer needs to grow by -cursor->n bytes. */
        err = CG_EOF;
    }

    if (err) {
        DEBUG_LOG("error %d at object type %d\n", err, (int)object_type);
        return err;
    }

    block->size = block->buffer_size - cursor->n;
    block->uncompressed_size = 0;
    return CG_OK;
}

__attribute__((unused)) static cg_err_t
compressor(struct block *block, void *buffer, size_t buffer_size, enum compressor compressor_type)
{
    compress_fn_t compress;
    ssize_t compress_size;

    switch (compressor_type) {
    case LZ4:
        compress = lz4_compress;
        break;
    case ZLIB:
        compress = zlib_compress;
        break;
    case NO_COMPRESSION:
        return CG_INVAL;
    }

    compress_size = compress(as_cregion(block), make_region(buffer, buffer_size));
    if (compress_size == -1) {
        /* Buffer too small? */
        return CG_COMPRESS;
    }

    /* Update values. */
    block->uncompressed_size = block->size;
    block->size = compress_size;
    block->buffer_size = buffer_size;
    block->buffer = buffer;

    return CG_OK;
}

static void assembler(const struct block *block, struct cursor *cursor)
{
    struct block_change_form *cf;

    /* Cursor must be at the block header. */
    assert(block->buffer - cursor->pos == block_header_size(block));

    switch (block->block_type) {
    case BLOCK_GLOBAL_DATA:
        c_store_le32(cursor, ((struct block_global_data *)block)->type_num);
        /* fallthrough */
    case BLOCK_SIMPLE:
        c_store_le32(cursor, block->size);
        break;

    case BLOCK_CHANGE_FORM:
        cf = (struct block_change_form *) block;
        c_store_be24(cursor, cf->form_id);
        c_store_le32(cursor, cf->flags);
        c_store_u8(cursor, cf->type_num);
        c_store_u8(cursor, cf->version);

        /* Two upper bits of type determine the sizes of length1 and length2. */
        switch ((cf->type_num >> 6) & 0x3) {
        case 0:
            c_store_u8(cursor, block->size);
            c_store_u8(cursor, block->uncompressed_size);
            break;
        case 1:
            c_store_le16(cursor, block->size);
            c_store_le16(cursor, block->uncompressed_size);
            break;
        case 2:
            c_store_le32(cursor, block->size);
            c_store_le32(cursor, block->uncompressed_size);
            break;
        default:
            BUG("Valid length types include uint8, uint16 and uint32.\n");
        }
        break;
    }
    
    assert(cursor->pos == block->buffer);

    c_advance(cursor, block->size);
}

static cg_err_t
file_writer(unsigned char *file, size_t *file_size_ptr, const struct savegame *save)
{
    struct location_table locations = {0};
    struct chunk *buffers[1] = {0}; /* Buffers for compression. */
    struct cursor file_cursor;  /* Cursor for additions to file. */
    struct cursor body_cursor;  /* Cursor for additions to body. */
    struct cursor *cursor;      /* The cursor being used. */
    intptr_t offset_var = (intptr_t)file; /* Variable for file offset calculation. */
    size_t max_file_size;       /* Size of 'file' arg for bounds checking. */
    unsigned char *ptr_to_locations; /* Points where to write location table. */
    cg_err_t err = CG_OK;

    max_file_size = *file_size_ptr;
    file_cursor.pos = file;
    file_cursor.n = max_file_size;
    cursor = &file_cursor;

    /* Calculates current file offset at cursor position. */
    #define OFFSET() ((uint32_t)((intptr_t)cursor->pos - offset_var))

    union {
        struct block simple;
        struct block_change_form chfo;
        struct block_global_data glda;
    } block_buf = {0};
    struct block *block = &block_buf.simple;

    /*
     * Write signature.
     */
    {
        const char *signature = NULL;

        switch (save->game) {
        case SKYRIM:
            signature = TESV_SIGNATURE;
            break;
        case FALLOUT4:
            signature = FO4_SIGNATURE;
            break;
        }

        c_store_bytes(cursor, signature, strlen(signature));
    }

    /*
     * Write the file header.
     */
    block->block_type = BLOCK_SIMPLE;
    prepare_block(block, cursor);
    err = serializer(block, save, OBJECT_FILE_HEADER);
    if (err) {
        goto out_error;
    }
    assembler(block, cursor);

    /*
     * Write snapshot.
     */
    c_store_bytes(cursor, save->snapshot_data, save->snapshot_size);

    /* Initialize body cursor. */
    if (supports_save_file_compression(save)) {
        if (save->priv->compressor != NO_COMPRESSION) {
            /*
             * Compression required => need a buffer to write the body in.
             * Allocate a buffer for the body and set the body cursor at
             * the beginning of the buffer.
             */
            buffers[0] = chunk_alloc(128 * 1024 * 1024);
            if (!buffers[0]) {
                err = CG_NO_MEM;
                goto out_error;
            }

            body_cursor.pos = buffers[0]->data;
            body_cursor.n = buffers[0]->size;
            offset_var = (intptr_t)body_cursor.pos - (file_cursor.pos - file);
        }
        else {
            /* 
             * No compression required. We will directly write to file, but
             * need to leave 8 bytes of space to store size information.
             */
            c_advance2(&body_cursor, &file_cursor, 8);
            offset_var += 8;
        }
    }
    else {
        /*
         * Game engine doesn't support compression for the save file.
         * We will continue writing where we left off.
         */
        c_advance2(&body_cursor, &file_cursor, 0);
    }

    cursor = &body_cursor;

    c_store_u8(cursor, save->priv->form_version);
    
    /*
     * Write game version string.
     */
    if (save->game == FALLOUT4) {
        c_store_le16_str(cursor, save->game_version);
    }

    /*
     * Write plugin info.
     */
    block->block_type = BLOCK_SIMPLE;
    prepare_block(block, cursor);
    err = serializer(block, save, OBJECT_PLUGIN_INFO);
    if (err) {
        goto out_error;
    }
    assembler(block, cursor);

    /*
     * Save pointer to file location table.
     */
    if (cursor->n < LOCATION_TABLE_SIZE) {
        err = CG_EOF;
        goto out_error;
    }
    ptr_to_locations = cursor->pos;
    memset(ptr_to_locations, 0, LOCATION_TABLE_SIZE);
    c_advance(cursor, LOCATION_TABLE_SIZE);

    /*
     * Write global data table 1.
     */
    locations.off_globals1 = OFFSET();
    block->block_type = BLOCK_GLOBAL_DATA;
    for (enum object_type i = FIRST_TABLE1_OBJECT_GLDA; i < FIRST_TABLE2_OBJECT_GLDA; ++i) {
        block_buf.glda.type_num = glda_type_number(i);
        prepare_block(block, cursor);
        err = serializer(block, save, i);
        if (err && err != CG_NOT_PRESENT) {
            goto out_error;
        }
        if (err != CG_NOT_PRESENT) {
            assembler(block, cursor);
            locations.num_globals1++;
        }

    }
    err = CG_OK;

    /*
     * Write global data table 2.
     */
    locations.off_globals2 = OFFSET();
    for (enum object_type i = FIRST_TABLE2_OBJECT_GLDA; i < FIRST_TABLE3_OBJECT_GLDA; ++i) {
        block_buf.glda.type_num = glda_type_number(i);
        prepare_block(block, cursor);
        err = serializer(block, save, i);
        if (err && err != CG_NOT_PRESENT) {
            goto out_error;
        }
        if (err != CG_NOT_PRESENT) {
            assembler(block, cursor);
            locations.num_globals2++;
        }
    }
    err = CG_OK;

    /*
     * Write change forms.
     */
    locations.off_change_forms = OFFSET();
    block->block_type = BLOCK_CHANGE_FORM;
    for (unsigned i = 0; i < save->priv->n_change_forms; ++i) {
        struct change_form *ptr = &save->priv->change_forms[i];
        /* Already serialized and compressed. */
        block_buf.chfo.flags = ptr->flags;
        block_buf.chfo.form_id = ptr->form_id;
        block_buf.chfo.version = ptr->version;
        block_buf.chfo.type_num = ptr->type;
        block->size = ptr->length1;
        block->uncompressed_size = ptr->length2;

        prepare_block(block, cursor);
        memcpy(block->buffer, ptr->data, block->size);
        assembler(block, cursor);

        locations.num_change_forms++;
    }

    /*
     * Write global data table 3.
     */
    locations.off_globals3 = OFFSET();
    block->block_type = BLOCK_GLOBAL_DATA;
    for (enum object_type i = FIRST_TABLE3_OBJECT_GLDA; i <= LAST_OBJECT_GLDA; ++i) {
        block_buf.glda.type_num = glda_type_number(i);
        prepare_block(block, cursor);
        err = serializer(block, save, i);
        if (err && err != CG_NOT_PRESENT) {
            goto out_error;
        }
        if (err != CG_NOT_PRESENT) {
            assembler(block, cursor);
            locations.num_globals3++;
        }
    }
    err = CG_OK;

    /*
     * Write form IDs.
     */
    locations.off_form_ids_count = OFFSET();
    c_store_le32(cursor, save->num_form_ids);
    for (uint32_t i = 0; i < save->num_form_ids; ++i) {
        c_store_le32(cursor, save->form_ids[i]);
    }

    /*
     * Write world spaces.
     */
    c_store_le32(cursor, save->num_world_spaces);
    for (uint32_t i = 0; i < save->num_world_spaces; ++i) {
        c_store_le32(cursor, save->world_spaces[i]);
    }

    /*
     * Write unknown table.
     */
    locations.off_unknown_table = OFFSET();
    c_store_le32(cursor, save->priv->unknown3->size);
    c_store_bytes(cursor, save->priv->unknown3->data, save->priv->unknown3->size);
    
    if (cursor->n < 0) {
        err = CG_EOF;
        goto out_error;
    }

    /*
     * Write locations table.
     */
    store_le32(&ptr_to_locations[0], locations.off_form_ids_count);
    store_le32(&ptr_to_locations[4], locations.off_unknown_table);
    store_le32(&ptr_to_locations[8], locations.off_globals1);
    store_le32(&ptr_to_locations[12], locations.off_globals2);
    store_le32(&ptr_to_locations[16], locations.off_change_forms);
    store_le32(&ptr_to_locations[20], locations.off_globals3);
    store_le32(&ptr_to_locations[24], locations.num_globals1);
    store_le32(&ptr_to_locations[28], locations.num_globals2);
    /* Subtract by 1: Skyrim doesn't acknowledge the last global data. */
    store_le32(&ptr_to_locations[32], locations.num_globals3 - 1);
    store_le32(&ptr_to_locations[36], locations.num_change_forms);
    print_locations_table(&locations);

    /*
     * Compress body into the file, if necessary.
     */
    cursor = &file_cursor;

    if (supports_save_file_compression(save)) {
        ssize_t uncompress_size = buffers[0]->size - body_cursor.n;
        ssize_t compress_size = 0;

        struct cregion src = make_cregion(buffers[0]->data, uncompress_size);
        struct region dest = make_region(cursor->pos + 8, cursor->n - 8);

        switch (save->priv->compressor) {
        case LZ4:
            compress_size = lz4_compress(src, dest);
            break;
        case ZLIB:
            compress_size = zlib_compress(src, dest);
            break;
        case NO_COMPRESSION:
            /* Not sure about this, but needed to correctly advance cursor. */
            compress_size = uncompress_size;
            break;
        }

        if (compress_size == -1) {
            err = CG_COMPRESS;
            goto out_error;
        }

        c_store_le32(cursor, uncompress_size);
        c_store_le32(cursor, compress_size);
        c_advance(cursor, compress_size);
    }
    else {
        c_advance2(&file_cursor, &body_cursor, 0);
    }

    /* File is now written, give the size to the caller. */
    *file_size_ptr = max_file_size - file_cursor.n;

out_error:
    for (size_t i = 0; i < ARRAY_LEN(buffers); ++i) {
        free(buffers[i]);
    }
    return err;
#undef OFFSET
}

int cengine_savefile_write(const char *filename, const struct savegame *savegame)
{
    size_t max_file_size;
    size_t file_size;
    cg_err_t err;
    void *file;
    int fd;

    /* savegame should have been initialized correctly. */
    assert(savegame->priv != NULL);

    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    max_file_size = 128 * 1024 * 1024; /* 128 MiB limit. */
    if (ftruncate(fd, max_file_size) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    file = mmap(NULL, max_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    if (close(fd) == -1) {
        perror("close");
    }

    file_size = max_file_size;
    err = file_writer(file, &file_size, savegame);

    if (munmap(file, max_file_size) == -1) {
        perror("munmap");
    }

    if (truncate(filename, file_size) == -1) {
        perror("truncate");
    }

    return err == CG_OK ? 0 : -1;
}

static struct savegame *savegame_alloc(void)
{
    struct savegame *save;
    struct psavegame *priv;

    save = calloc(1, sizeof(*save));
    priv = calloc(1, sizeof(*priv));

    if (!save || !priv) {
        free(priv);
        free(save);
        return NULL;
    }

    save->priv = priv;

    return save;
}

void savegame_free(struct savegame *save)
{
    struct psavegame *private = save->priv;
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

    for (i = 0; i < ARRAY_LEN(private->globals); ++i) {
        free(private->globals[i]);
    }

    if (private->change_forms) {
        for(i = 0; i < private->n_change_forms; ++i) {
            free(private->change_forms[i].data);
        }

        free(private->change_forms);
    }

    free(private->unknown3);
    free(private);
    free(save);
}

#if defined(COMPILE_WITH_UNIT_TESTS)

#define TEST_SUITE(TEST_CASE)                                   \
    TEST_CASE(serialize_deserialized_objects_test)              \
    TEST_CASE(read_and_write_sample_files_back_identically)     \

#include <dirent.h>
#include "unit_tests.h"

static void for_each_sample_file(void (*callback)(const char *filename))
{
    struct dirent *dirent;
    DIR *samples;

    samples = opendir("../samples");
    ASSERT_NOT_NULL(samples);

    while ((dirent = readdir(samples)) != NULL) {
        char sample_filename[512];

        if (dirent->d_type != DT_REG) {
			continue;
		}

        sprintf(sample_filename, "../samples/%s", dirent->d_name);

        eprintf("sample file: %s\n", sample_filename);
        callback(sample_filename);
    }

    closedir(samples);
}

static void serialize_deserialized_objects(const char *sample_filename)
{
    unsigned char *sample_file;
    size_t sample_file_size;
    struct savegame *save;
    cg_err_t err;

    enum {BUFFER_SIZE = 8 * 1024 * 1024};
    static unsigned char buffer[BUFFER_SIZE];

    struct block block = {
        .buffer = buffer,
        .buffer_size = BUFFER_SIZE
    };

    save = savegame_alloc();
    sample_file = mmap_entire_file_r(sample_filename, &sample_file_size);
    ASSERT_NE_PTR(sample_file, MAP_FAILED);
    ASSERT_NOT_NULL(save);

    /* Read objects into unit_test_file_objects. */
    err = file_reader(sample_file, sample_file_size, save);
    munmap(sample_file, sample_file_size);
    ASSERT_EQ(err, CG_OK);

    for (int i = 0; i < OBJECT_TYPE_COUNT; ++i) {
        ut_info("Serializing object %d\n", i);

        err = serializer(&block, save, i);
        if (err == CG_NOT_PRESENT) {
            continue;
        }
        ASSERT_EQ(err, CG_OK);
        ASSERT_EQ_MEM(block.buffer, block.size,
                unit_test_file_objects[i]->data,
                unit_test_file_objects[i]->size);
    }

    for (int i = 0; i < OBJECT_TYPE_COUNT; ++i) {
        free(unit_test_file_objects[i]);
        unit_test_file_objects[i] = NULL;
    }

    savegame_free(save);
}

UNIT_TEST(serialize_deserialized_objects_test)
{
    debug_log_file = stderr;
    for_each_sample_file(serialize_deserialized_objects);
}

static void check_writer_produces_identical_file(const char *sample_filename)
{
    unsigned char *rewritten_file;
    unsigned char *sample_file;
    size_t rewritten_file_size;
    size_t sample_file_size;
    struct savegame *save;
    cg_err_t err;

    sample_file = mmap_entire_file_r(sample_filename, &sample_file_size);
    save = savegame_alloc();
    ASSERT_NE_PTR(sample_file, MAP_FAILED);
    ASSERT_NOT_NULL(save);

    err = file_reader(sample_file, sample_file_size, save);
    ASSERT_EQ(err, CG_OK);

    rewritten_file = malloc(sample_file_size);
    ASSERT_NOT_NULL(rewritten_file);

    rewritten_file_size = sample_file_size;
    err = file_writer(rewritten_file, &rewritten_file_size, save);
    ASSERT_EQ(err, CG_OK);

    if (rewritten_file_size != sample_file_size) {
        char dump_filename[512] = "./dump_rewritten_file";

        eprintf("Unequal file lengths.\n"
                "Dump file: %s\n"
                "Original file: %s\n",
                dump_filename, sample_filename);

        dump_to_file(dump_filename, rewritten_file, rewritten_file_size, 0);
    }

    ASSERT_EQ_MEM(sample_file, sample_file_size,
            rewritten_file, rewritten_file_size);

    munmap(sample_file, sample_file_size);
    free(rewritten_file);
    savegame_free(save);
}

UNIT_TEST(read_and_write_sample_files_back_identically)
{
    debug_log_file = stderr;
    for_each_sample_file(check_writer_produces_identical_file);
}

#endif /* defined(COMPILE_WITH_UNIT_TESTS) */
