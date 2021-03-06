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

#ifndef CEGSE_CHANGE_FORMS_H
#define CEGSE_CHANGE_FORMS_H

#include "types.h"

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
	CHANGE_FORM_FLAGS				= 0x00000001,
	CHANGE_CLASS_TAG_SKILLS				= 0x00000002,
	CHANGE_FACTION_FLAGS				= 0x00000002,
	CHANGE_FACTION_REACTIONS			= 0x00000004,
	CHANGE_FACTION_CRIME_COUNTS			= 0x80000000,
	CHANGE_TALKING_ACTIVATOR_SPEAKER		= 0x00800000,
	CHANGE_BOOK_TEACHES				= 0x00000020,
	CHANGE_BOOK_READ				= 0x00000040,
	CHANGE_DOOR_EXTRA_TELEPORT			= 0x00020000,
	CHANGE_INGREDIENT_USE				= 0x80000000,
	CHANGE_ACTOR_BASE_DATA				= 0x00000002,
	CHANGE_ACTOR_BASE_ATTRIBUTES			= 0x00000004,
	CHANGE_ACTOR_BASE_AIDATA			= 0x00000008,
	CHANGE_ACTOR_BASE_SPELLLIST			= 0x00000010,
	CHANGE_ACTOR_BASE_FULLNAME			= 0x00000020,
	CHANGE_ACTOR_BASE_FACTIONS			= 0x00000040,
	CHANGE_NPC_SKILLS				= 0x00000200,
	CHANGE_NPC_CLASS				= 0x00000400,
	CHANGE_NPC_FACE					= 0x00000800,
	CHANGE_NPC_DEFAULT_OUTFIT			= 0x00001000,
	CHANGE_NPC_SLEEP_OUTFIT				= 0x00002000,
	CHANGE_NPC_GENDER				= 0x01000000,
	CHANGE_NPC_RACE					= 0x02000000,
	CHANGE_LEVELED_LIST_ADDED_OBJECT		= 0x80000000,
	CHANGE_NOTE_READ				= 0x80000000,
	CHANGE_CELL_FLAGS				= 0x00000002,
	CHANGE_CELL_FULLNAME				= 0x00000004,
	CHANGE_CELL_OWNERSHIP				= 0x00000008,
	CHANGE_CELL_EXTERIOR_SHORT			= 0x10000000,
	CHANGE_CELL_EXTERIOR_CHAR			= 0x20000000,
	CHANGE_CELL_DETACHTIME				= 0x40000000,
	CHANGE_CELL_SEENDATA				= 0x80000000,
	CHANGE_REFR_MOVE				= 0x00000002,
	CHANGE_REFR_HAVOK_MOVE				= 0x00000004,
	CHANGE_REFR_CELL_CHANGED			= 0x00000008,
	CHANGE_REFR_SCALE				= 0x00000010,
	CHANGE_REFR_INVENTORY				= 0x00000020,
	CHANGE_REFR_EXTRA_OWNERSHIP			= 0x00000040,
	CHANGE_REFR_BASEOBJECT				= 0x00000080,
	CHANGE_REFR_PROMOTED				= 0x02000000,
	CHANGE_REFR_EXTRA_ACTIVATING_CHILDREN		= 0x04000000,
	CHANGE_REFR_LEVELED_INVENTORY			= 0x08000000,
	CHANGE_REFR_ANIMATION				= 0x10000000,
	CHANGE_REFR_EXTRA_ENCOUNTER_ZONE		= 0x20000000,
	CHANGE_REFR_EXTRA_CREATED_ONLY			= 0x40000000,
	CHANGE_REFR_EXTRA_GAME_ONLY			= 0x80000000,
	CHANGE_ACTOR_LIFESTATE				= 0x00000400,
	CHANGE_ACTOR_EXTRA_PACKAGE_DATA			= 0x00000800,
	CHANGE_ACTOR_EXTRA_MERCHANT_CONTAINER		= 0x00001000,
	CHANGE_ACTOR_EXTRA_DISMEMBERED_LIMBS		= 0x00020000,
	CHANGE_ACTOR_LEVELED_ACTOR			= 0x00040000,
	CHANGE_ACTOR_DISPOSITION_MODIFIERS		= 0x00080000,
	CHANGE_ACTOR_TEMP_MODIFIERS			= 0x00100000,
	CHANGE_ACTOR_DAMAGE_MODIFIERS			= 0x00200000,
	CHANGE_ACTOR_OVERRIDE_MODIFIERS			= 0x00400000,
	CHANGE_ACTOR_PERMANENT_MODIFIERS		= 0x00800000,
	CHANGE_OBJECT_EXTRA_ITEM_DATA			= 0x00000400,
	CHANGE_OBJECT_EXTRA_AMMO			= 0x00000800,
	CHANGE_OBJECT_EXTRA_LOCK			= 0x00001000,
	CHANGE_OBJECT_EMPTY				= 0x00200000,
	CHANGE_OBJECT_OPEN_DEFAULT_STATE		= 0x00400000,
	CHANGE_OBJECT_OPEN_STATE			= 0x00800000,
	CHANGE_TOPIC_SAIDONCE				= 0x80000000,
	CHANGE_QUEST_FLAGS				= 0x00000002,
	CHANGE_QUEST_SCRIPT_DELAY			= 0x00000004,
	CHANGE_QUEST_ALREADY_RUN			= 0x04000000,
	CHANGE_QUEST_INSTANCES				= 0x08000000,
	CHANGE_QUEST_RUNDATA				= 0x10000000,
	CHANGE_QUEST_OBJECTIVES				= 0x20000000,
	CHANGE_QUEST_SCRIPT				= 0x40000000,
	CHANGE_QUEST_STAGES				= 0x80000000,
	CHANGE_PACKAGE_WAITING				= 0x40000000,
	CHANGE_PACKAGE_NEVER_RUN			= 0x80000000,
	CHANGE_FORM_LIST_ADDED_FORM			= 0x80000000,
	CHANGE_ENCOUNTER_ZONE_FLAGS			= 0x00000002,
	CHANGE_ENCOUNTER_ZONE_GAME_DATA			= 0x80000000,
	CHANGE_LOCATION_KEYWORDDATA			= 0x40000000,
	CHANGE_LOCATION_CLEARED				= 0x80000000,
	CHANGE_QUEST_NODE_TIME_RUN			= 0x80000000,
	CHANGE_RELATIONSHIP_DATA			= 0x00000002,
	CHANGE_SCENE_ACTIVE				= 0x80000000,
	CHANGE_BASE_OBJECT_VALUE			= 0x00000002,
	CHANGE_BASE_OBJECT_FULLNAME			= 0x00000004
};

struct change_form {
	ref_t form_id;
	u32 flags;
	u32 type;
	u32 version;
	u32 length1;	/* Length of data */
	u32 length2;	/* Non-zero value means data is compressed */
	unsigned char *data;
};

#endif /* CEGSE_CHANGE_FORMS_H */
