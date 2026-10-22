// The table moves: bigger weapon, item, class and monster tables for mods.
#include <stdint.h>

enum { TABLE_FIELD_REL32, TABLE_FIELD_DISP32 };

// One rewrite of an instruction that builds a table address.
typedef struct {
    unsigned int rva;
    unsigned char fieldOffset, kind;
    unsigned char bytes[15];
} TableMoveSite;

// A rewrite whose new value isn't a distance: a bound or a size.
typedef struct {
    unsigned int rva;
    int size, fieldOffset;
    unsigned char shipped[8];
    int value;
    const char *name;
} TableValueSite;

// A rewrite of bytes that hold no table address.
typedef struct {
    unsigned int rva;
    int size;
    unsigned char shipped[15], patched[15];
} TablePatchSite;

// Weapons: mods' weapons take slots from 1200.
#define FIRST_MOD_WEAPON_SLOT     1200
#define MOD_WEAPONS_ADDED         2000
#define LAST_MOD_WEAPON_SLOT      (WEAPON_RECORDS_GAME - 2 + MOD_WEAPONS_ADDED)
#define WEAPON_RECORDS_MOVED      (LAST_MOD_WEAPON_SLOT + 2)

// Items: mods' items take slots from 300.
#define FIRST_MOD_ITEM_SLOT       300
#define MOD_ITEMS_ADDED           1000
#define LAST_MOD_ITEM_SLOT        (ITEM_TABLE_MAX - 2 + MOD_ITEMS_ADDED)
#define ITEM_RECORDS_MOVED        (LAST_MOD_ITEM_SLOT + 2)

// Classes: mods' classes take slots from 31; heroes, recruits and records grow together.
#define FIRST_MOD_CLASS_SLOT      31
#define MOD_CLASSES_ADDED         50
#define LAST_MOD_CLASS_SLOT       (CLASS_TABLE_MAX - 1 + MOD_CLASSES_ADDED)
#define CLASS_RECORDS_MOVED       (LAST_MOD_CLASS_SLOT + 1)
// The class picker's button ids: pictures at 100 + class, info at CLASS_PICKER_INFO_BASE + class.
#define CLASS_PICKER_PICK_BASE    100
#define CLASS_PICKER_INFO_BASE    1000
_Static_assert(CLASS_PICKER_PICK_BASE + LAST_MOD_CLASS_SLOT < CLASS_PICKER_INFO_BASE, "the picker's bands stay apart");

// Monsters: mods' monsters take slots from 2000; the per-monster arrays grow with the table.
#define FIRST_MOD_MONSTER_SLOT    2000
#define MOD_MONSTERS_ADDED        1000
#define LAST_MOD_MONSTER_SLOT     (MONSTER_TABLE_MAX - 2 + MOD_MONSTERS_ADDED)
#define MONSTER_RECORDS_MOVED     (LAST_MOD_MONSTER_SLOT + 2)

#ifndef __APPLE__
#include "weapon_table_sites.h"
#include "item_table_sites.h"
#include "class_heroes_table_sites.h"
#include "class_recruits_table_sites.h"
#include "class_table_sites.h"
#include "class_copy_table_sites.h"
#include "monster_table_sites.h"
#include "army_list_counts_table_sites.h"
#include "army_list_units_table_sites.h"
#include "army_list_least_a_table_sites.h"
#include "army_list_least_b_table_sites.h"
#include "monster_sprite_labels_table_sites.h"
#include "battle_summons_table_sites.h"
#include "monster_frame_sites.h"
#else
// A Mac site: an ADRP that builds a table address, as shipped.
typedef struct { unsigned int rva; uint32_t word; } TableMacSite;
// A Mac bound: a movz or cmp immediate sized by the table, as shipped and new.
typedef struct { unsigned int rva; uint32_t word; int value; } TableMacBound;
// The table's size in bytes, built by a movz and the movk after it, as shipped.
typedef struct { unsigned int rva; uint32_t word, next; } TableMacSize;
// A Mac word the move changes that holds no table address.
typedef struct { unsigned int rva; uint32_t shipped, word; } TableMacPatch;
#include "weapon_table_sites_mac.h"
#include "item_table_sites_mac.h"
#include "class_heroes_table_sites_mac.h"
#include "class_recruits_table_sites_mac.h"
#include "class_table_sites_mac.h"
#include "class_copy_table_sites_mac.h"
#include "monster_table_sites_mac.h"
#include "army_list_counts_table_sites_mac.h"
#include "army_list_units_table_sites_mac.h"
#include "army_list_least_a_table_sites_mac.h"
#include "army_list_least_b_table_sites_mac.h"
#include "monster_sprite_labels_table_sites_mac.h"
#include "battle_summons_table_sites_mac.h"
#include "monster_frame_sites_mac.h"
#endif

// One table: where the game keeps it, how big CoE5SE makes it, and what refuses an entry past it.
typedef struct TableMove {
    const char *name;
    unsigned int rva;
    int recordSize, gameRecords, movedRecords, firstModSlot, lastModSlot;
    // Tables that move together in one block: blockWithNext marks every one but the last.
    int blockWithNext;
    const TableMoveSite *sites;
    int siteCount;
    const TableValueSite *bounds, *values;
    int boundCount, valueCount;
    const TablePatchSite *patches;
    int patchCount;
    // Queues changes past the table's own sites; returns how many matched.
    int (*queueExtra)(const struct TableMove *t);
    int extraCount;
#ifdef __APPLE__
    const TableMacSite *macSites;
    const TableMacBound *macBounds;
    const TableMacSize *macSizes;
    const TableMacPatch *macPatches;
    int macSiteCount, macBoundCount, macSizeCount, macPatchCount;
#endif
    // Where an entry past the last slot is refused.
    unsigned int overflowCall, overflowTarget;
    void *overflowHook;
    unsigned int countRva;
    int countSlack;
    // The refusal's .c5m word, reason, pop-up words, and the word that names the entry.
    const char *word, *reason, *title, *opening, *header, *nameWord;
    unsigned char *moved;
    unsigned char *oldStart, *oldEnd;
} TableMove;

#ifndef __APPLE__
static void weaponOutOfSlotsHook(const char *fmt, const char *name);

static const TableValueSite WEAPON_VALUE_SITES[] = {
    {RVA_WEAPON_FREE_SLOT_CAP,    5, 1, {0x3d, 0x88, 0x13, 0x00, 0x00},             WEAPON_RECORDS_MOVED,     "free_slot_cap"},
    {RVA_SELECTWEAPON_BOUND,      5, 1, {0x3d, 0x88, 0x13, 0x00, 0x00},             WEAPON_RECORDS_MOVED,     "selectweapon"},
    {RVA_NEXTWEP_BOUND,           5, 1, {0x3d, 0x87, 0x13, 0x00, 0x00},             WEAPON_RECORDS_MOVED - 1, "nextwep"},
    {RVA_NEXTALWAYSWEP_BOUND,     5, 1, {0x3d, 0x87, 0x13, 0x00, 0x00},             WEAPON_RECORDS_MOVED - 1, "nextalwayswep"},
    {RVA_ITEMWEP_BOUND,           5, 1, {0x3d, 0x87, 0x13, 0x00, 0x00},             WEAPON_RECORDS_MOVED - 1, "itemwep"},
    {RVA_WEAPON_NAME_LINE_BOUND,  7, 3, {0x41, 0x81, 0xf8, 0x87, 0x13, 0x00, 0x00}, WEAPON_RECORDS_MOVED - 1, "name_line"},
    {RVA_WEAPON_CLEAR_SIZE,       6, 2, {0x41, 0xb8, 0x00, 0x35, 0x0c, 0x00},       WEAPON_RECORDS_MOVED * WEAPON_SIZE, "clear_size"},
    {RVA_WEAPON_BACKUP_SIZE,      5, 1, {0xb9, 0x00, 0x35, 0x0c, 0x00},             WEAPON_RECORDS_MOVED * WEAPON_SIZE, "backup_size"},
    {RVA_WEAPON_BACKUP_COPY_SIZE, 6, 2, {0x41, 0xb8, 0x00, 0x35, 0x0c, 0x00},       WEAPON_RECORDS_MOVED * WEAPON_SIZE, "backup_copy_size"},
    {RVA_WEAPON_RESTORE_SIZE,     6, 2, {0x41, 0xb8, 0x00, 0x35, 0x0c, 0x00},       WEAPON_RECORDS_MOVED * WEAPON_SIZE, "restore_size"},
};
#define WEAPON_VALUE_SITE_COUNT ((int)(sizeof WEAPON_VALUE_SITES / sizeof WEAPON_VALUE_SITES[0]))

static const TableValueSite ITEM_VALUE_SITES[] = {
    {RVA_GROUND_ITEMS_LOAD_CHECK,    5, 1, {0x3d, 0xe8, 0x03, 0x00, 0x00},       ITEM_RECORDS_MOVED, "ground_items_load"},
    {RVA_GROUND_ITEMS_LOAD_CHECK_2,   6, 2, {0x81, 0xf9, 0xe8, 0x03, 0x00, 0x00}, ITEM_RECORDS_MOVED, "ground_items_load_2"},
};
#define ITEM_VALUE_SITE_COUNT ((int)(sizeof ITEM_VALUE_SITES / sizeof ITEM_VALUE_SITES[0]))

#define CLASS_TABLE_BYTES_GAME   0x1892a0
#define CLASS_RECRUITS_BYTES_GAME 0x1443800
_Static_assert(CLASS_TABLE_BYTES_GAME == CLASS_TABLE_MAX * CLASS_SIZE && CLASS_RECRUITS_BYTES_GAME == CLASS_TABLE_MAX * CLASS_RECRUITS_SIZE,
               "the class sizes the game's code holds");
#define CLASS_SIZE_SITE_R8(rva, name) {rva, 6, 2, {0x41, 0xb8, 0xa0, 0x92, 0x18, 0x00}, CLASS_RECORDS_MOVED * CLASS_SIZE, name}
static const TableValueSite CLASS_VALUE_SITES[] = {
    CLASS_SIZE_SITE_R8(RVA_CLASS_COPY_SIZE_1, "copy_size_1"), CLASS_SIZE_SITE_R8(RVA_CLASS_COPY_SIZE_2, "copy_size_2"),
    CLASS_SIZE_SITE_R8(RVA_CLASS_COPY_SIZE_3, "copy_size_3"), CLASS_SIZE_SITE_R8(RVA_CLASS_COPY_SIZE_4, "copy_size_4"),
    CLASS_SIZE_SITE_R8(RVA_CLASS_COPY_SIZE_5, "copy_size_5"),
    {RVA_CLASS_BACKUP_SIZE, 5, 1, {0xb9, 0xa0, 0x92, 0x18, 0x00}, CLASS_RECORDS_MOVED * CLASS_SIZE, "backup_size"},
    CLASS_SIZE_SITE_R8(RVA_CLASS_BACKUP_SIZE + 10, "backup_copy_size"), CLASS_SIZE_SITE_R8(RVA_CLASS_RESTORE_SIZE, "restore_size"),
    {RVA_CLASS_RECRUITS_BACKUP_SIZE, 5, 1, {0xb9, 0x00, 0x38, 0x44, 0x01}, CLASS_RECORDS_MOVED * CLASS_RECRUITS_SIZE, "recruits_backup_size"},
    {RVA_CLASS_RECRUITS_BACKUP_SIZE + 10, 6, 2, {0x41, 0xb8, 0x00, 0x38, 0x44, 0x01}, CLASS_RECORDS_MOVED * CLASS_RECRUITS_SIZE, "recruits_backup_copy_size"},
    {RVA_CLASS_RECRUITS_RESTORE_SIZE, 6, 2, {0x41, 0xb8, 0x00, 0x38, 0x44, 0x01}, CLASS_RECORDS_MOVED * CLASS_RECRUITS_SIZE, "recruits_restore_size"},
    {RVA_CLASS_PICKER_INFO_ID, 6, 2, {0x8d, 0x8e, 0xc8, 0x00, 0x00, 0x00}, CLASS_PICKER_INFO_BASE, "picker_info_id"},
};
#define CLASS_VALUE_SITE_COUNT ((int)(sizeof CLASS_VALUE_SITES / sizeof CLASS_VALUE_SITES[0]))
#endif
static int queueClassPickerDecode(const struct TableMove *t);

#ifdef __APPLE__
static void weaponOutOfSlotsHook(const char *fmt, ...);
#endif
static TableMove tableMoves[] = {
#ifndef __APPLE__
    {.name = "weapon", .rva = RVA_WEAPON_TABLE, .recordSize = WEAPON_SIZE, .gameRecords = WEAPON_RECORDS_GAME,
     .movedRecords = WEAPON_RECORDS_MOVED, .firstModSlot = FIRST_MOD_WEAPON_SLOT, .lastModSlot = LAST_MOD_WEAPON_SLOT,
     .sites = WEAPON_TABLE_SITES, .siteCount = WEAPON_TABLE_SITE_COUNT, .values = WEAPON_VALUE_SITES, .valueCount = WEAPON_VALUE_SITE_COUNT,
     .overflowCall = RVA_WEAPON_OUT_OF_WEAPONS_CALL, .overflowTarget = RVA_TRACE_ERROR, .overflowHook = (void *)weaponOutOfSlotsHook,
     .word = "newweapon", .reason = "out_of_weapon_slots", .title = "CoE5SE: Weapons left out", .opening = "Weapons left out (no room):",
     .header = "Weapons left out"},
    {.name = "item", .rva = RVA_ITEM_TABLE, .recordSize = ITEM_SIZE, .gameRecords = ITEM_TABLE_MAX,
     .movedRecords = ITEM_RECORDS_MOVED, .firstModSlot = FIRST_MOD_ITEM_SLOT, .lastModSlot = LAST_MOD_ITEM_SLOT,
     .sites = ITEM_TABLE_SITES, .siteCount = ITEM_TABLE_SITE_COUNT, .bounds = ITEM_BOUND_SITES, .boundCount = ITEM_BOUND_SITE_COUNT,
     .values = ITEM_VALUE_SITES, .valueCount = ITEM_VALUE_SITE_COUNT, .countRva = RVA_ITEM_COUNT, .countSlack = 2,
     .word = "newitem", .reason = "out_of_item_slots", .title = "CoE5SE: Items left out", .opening = "Items left out (no room):",
     .header = "Items left out"},
    {.name = "class_heroes", .rva = RVA_CLASS_SIDE_B, .recordSize = CLASS_HEROES_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT, .blockWithNext = 1,
     .sites = CLASS_HEROES_TABLE_SITES, .siteCount = CLASS_HEROES_TABLE_SITE_COUNT},
    {.name = "class_recruits", .rva = RVA_CLASS_SIDE_A, .recordSize = CLASS_RECRUITS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT, .blockWithNext = 1,
     .sites = CLASS_RECRUITS_TABLE_SITES, .siteCount = CLASS_RECRUITS_TABLE_SITE_COUNT},
    {.name = "class", .rva = RVA_CLASS_TABLE, .recordSize = CLASS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT,
     .sites = CLASS_TABLE_SITES, .siteCount = CLASS_TABLE_SITE_COUNT, .bounds = CLASS_BOUND_SITES, .boundCount = CLASS_BOUND_SITE_COUNT,
     .values = CLASS_VALUE_SITES, .valueCount = CLASS_VALUE_SITE_COUNT, .queueExtra = queueClassPickerDecode, .extraCount = 1,
     .countRva = RVA_CLASS_COUNT, .countSlack = 1,
     .word = "newclass", .reason = "out_of_class_slots", .title = "CoE5SE: Classes left out", .opening = "Classes left out (no room):",
     .header = "Classes left out", .nameWord = "setclassname"},
    {.name = "class_copy", .rva = RVA_CLASS_TABLE_COPY, .recordSize = CLASS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT,
     .sites = CLASS_COPY_TABLE_SITES, .siteCount = CLASS_COPY_TABLE_SITE_COUNT},
    {.name = "monster", .rva = RVA_MONSTER_TABLE, .recordSize = MONSTER_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = MONSTER_TABLE_SITES, .siteCount = MONSTER_TABLE_SITE_COUNT, .bounds = MONSTER_BOUND_SITES, .boundCount = MONSTER_BOUND_SITE_COUNT,
     .patches = MONSTER_FRAME_SITES, .patchCount = MONSTER_FRAME_SITE_COUNT, .countRva = RVA_MONSTER_COUNT, .countSlack = 2,
     .word = "newmonster", .reason = "out_of_monster_slots", .title = "CoE5SE: Monsters left out", .opening = "Monsters left out (no room):",
     .header = "Monsters left out"},
    {.name = "army_list_counts", .rva = RVA_ARMY_LIST_COUNTS, .recordSize = ARMY_LIST_COUNT_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = ARMY_LIST_COUNTS_TABLE_SITES, .siteCount = ARMY_LIST_COUNTS_TABLE_SITE_COUNT},
    {.name = "army_list_units", .rva = RVA_ARMY_LIST_UNITS, .recordSize = ARMY_LIST_COUNT_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = ARMY_LIST_UNITS_TABLE_SITES, .siteCount = ARMY_LIST_UNITS_TABLE_SITE_COUNT},
    {.name = "army_list_least_a", .rva = RVA_ARMY_LIST_LEAST_A, .recordSize = ARMY_LIST_LEAST_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = ARMY_LIST_LEAST_A_TABLE_SITES, .siteCount = ARMY_LIST_LEAST_A_TABLE_SITE_COUNT},
    {.name = "army_list_least_b", .rva = RVA_ARMY_LIST_LEAST_B, .recordSize = ARMY_LIST_LEAST_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = ARMY_LIST_LEAST_B_TABLE_SITES, .siteCount = ARMY_LIST_LEAST_B_TABLE_SITE_COUNT},
    {.name = "monster_sprite_labels", .rva = RVA_MONSTER_SPRITE_LABELS, .recordSize = ARMY_LIST_COUNT_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .sites = MONSTER_SPRITE_LABELS_TABLE_SITES, .siteCount = MONSTER_SPRITE_LABELS_TABLE_SITE_COUNT},
    {.name = "battle_summons", .rva = RVA_BATTLE_SUMMONS, .recordSize = BATTLE_SUMMONS_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT,
     .sites = BATTLE_SUMMONS_TABLE_SITES, .siteCount = BATTLE_SUMMONS_TABLE_SITE_COUNT},
#else
    {.name = "weapon", .rva = RVA_WEAPON_TABLE, .recordSize = WEAPON_SIZE, .gameRecords = WEAPON_RECORDS_GAME,
     .movedRecords = WEAPON_RECORDS_MOVED, .firstModSlot = FIRST_MOD_WEAPON_SLOT, .lastModSlot = LAST_MOD_WEAPON_SLOT,
     .macSites = WEAPON_MAC_SITES, .macSiteCount = WEAPON_MAC_SITE_COUNT, .macBounds = WEAPON_MAC_BOUNDS, .macBoundCount = WEAPON_MAC_BOUND_COUNT,
     .macSizes = WEAPON_MAC_SIZES, .macSizeCount = WEAPON_MAC_SIZE_COUNT,
     .overflowCall = RVA_WEAPON_OUT_OF_WEAPONS_CALL, .overflowTarget = RVA_TRACE_ERROR, .overflowHook = (void *)weaponOutOfSlotsHook,
     .word = "newweapon", .reason = "out_of_weapon_slots", .title = "CoE5SE: Weapons left out", .opening = "Weapons left out (no room):",
     .header = "Weapons left out"},
    {.name = "item", .rva = RVA_ITEM_TABLE, .recordSize = ITEM_SIZE, .gameRecords = ITEM_TABLE_MAX,
     .movedRecords = ITEM_RECORDS_MOVED, .firstModSlot = FIRST_MOD_ITEM_SLOT, .lastModSlot = LAST_MOD_ITEM_SLOT,
     .macSites = ITEM_MAC_SITES, .macSiteCount = ITEM_MAC_SITE_COUNT, .macBounds = ITEM_MAC_BOUNDS, .macBoundCount = ITEM_MAC_BOUND_COUNT,
     .countRva = RVA_ITEM_COUNT, .countSlack = 2,
     .word = "newitem", .reason = "out_of_item_slots", .title = "CoE5SE: Items left out", .opening = "Items left out (no room):",
     .header = "Items left out"},
    {.name = "class_heroes", .rva = RVA_CLASS_SIDE_B, .recordSize = CLASS_HEROES_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT, .blockWithNext = 1,
     .macSites = CLASS_HEROES_MAC_SITES, .macSiteCount = CLASS_HEROES_MAC_SITE_COUNT},
    {.name = "class_recruits", .rva = RVA_CLASS_SIDE_A, .recordSize = CLASS_RECRUITS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT, .blockWithNext = 1,
     .macSites = CLASS_RECRUITS_MAC_SITES, .macSiteCount = CLASS_RECRUITS_MAC_SITE_COUNT,
     .macSizes = CLASS_RECRUITS_MAC_SIZES, .macSizeCount = CLASS_RECRUITS_MAC_SIZE_COUNT},
    {.name = "class", .rva = RVA_CLASS_TABLE, .recordSize = CLASS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT,
     .macSites = CLASS_MAC_SITES, .macSiteCount = CLASS_MAC_SITE_COUNT, .macBounds = CLASS_MAC_BOUNDS, .macBoundCount = CLASS_MAC_BOUND_COUNT,
     .macSizes = CLASS_MAC_SIZES, .macSizeCount = CLASS_MAC_SIZE_COUNT, .queueExtra = queueClassPickerDecode, .extraCount = 1,
     .countRva = RVA_CLASS_COUNT, .countSlack = 1,
     .word = "newclass", .reason = "out_of_class_slots", .title = "CoE5SE: Classes left out", .opening = "Classes left out (no room):",
     .header = "Classes left out", .nameWord = "setclassname"},
    {.name = "class_copy", .rva = RVA_CLASS_TABLE_COPY, .recordSize = CLASS_SIZE, .gameRecords = CLASS_TABLE_MAX,
     .movedRecords = CLASS_RECORDS_MOVED, .firstModSlot = FIRST_MOD_CLASS_SLOT, .lastModSlot = LAST_MOD_CLASS_SLOT,
     .macSites = CLASS_COPY_MAC_SITES, .macSiteCount = CLASS_COPY_MAC_SITE_COUNT},
    {.name = "monster", .rva = RVA_MONSTER_TABLE, .recordSize = MONSTER_SIZE, .gameRecords = MONSTER_TABLE_MAX,
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = 1,
     .macSites = MONSTER_MAC_SITES, .macSiteCount = MONSTER_MAC_SITE_COUNT, .macBounds = MONSTER_MAC_BOUNDS, .macBoundCount = MONSTER_MAC_BOUND_COUNT,
     .macSizes = MONSTER_MAC_SIZES, .macSizeCount = MONSTER_MAC_SIZE_COUNT,
     .macPatches = MONSTER_FRAME_MAC_SITES, .macPatchCount = MONSTER_FRAME_MAC_SITE_COUNT,
     .countRva = RVA_MONSTER_COUNT, .countSlack = 2,
     .word = "newmonster", .reason = "out_of_monster_slots", .title = "CoE5SE: Monsters left out", .opening = "Monsters left out (no room):",
     .header = "Monsters left out"},
#define MAC_MONSTER_ARRAY(n, r, size, sites, block) {.name = n, .rva = r, .recordSize = size, .gameRecords = MONSTER_TABLE_MAX, \
     .movedRecords = MONSTER_RECORDS_MOVED, .firstModSlot = FIRST_MOD_MONSTER_SLOT, .lastModSlot = LAST_MOD_MONSTER_SLOT, .blockWithNext = block, \
     .macSites = sites##_MAC_SITES, .macSiteCount = sites##_MAC_SITE_COUNT}
    MAC_MONSTER_ARRAY("army_list_counts", RVA_ARMY_LIST_COUNTS, ARMY_LIST_COUNT_SIZE, ARMY_LIST_COUNTS, 1),
    MAC_MONSTER_ARRAY("army_list_units", RVA_ARMY_LIST_UNITS, ARMY_LIST_COUNT_SIZE, ARMY_LIST_UNITS, 1),
    MAC_MONSTER_ARRAY("army_list_least_a", RVA_ARMY_LIST_LEAST_A, ARMY_LIST_LEAST_SIZE, ARMY_LIST_LEAST_A, 1),
    MAC_MONSTER_ARRAY("army_list_least_b", RVA_ARMY_LIST_LEAST_B, ARMY_LIST_LEAST_SIZE, ARMY_LIST_LEAST_B, 1),
    MAC_MONSTER_ARRAY("monster_sprite_labels", RVA_MONSTER_SPRITE_LABELS, ARMY_LIST_COUNT_SIZE, MONSTER_SPRITE_LABELS, 1),
    MAC_MONSTER_ARRAY("battle_summons", RVA_BATTLE_SUMMONS, BATTLE_SUMMONS_SIZE, BATTLE_SUMMONS, 0),
#endif
};
#define TABLE_MOVE_COUNT ((int)(sizeof tableMoves / sizeof tableMoves[0]))
enum { TABLE_WEAPON, TABLE_ITEM, TABLE_CLASS_HEROES, TABLE_CLASS_RECRUITS, TABLE_CLASS, TABLE_CLASS_COPY, TABLE_MONSTER };

// Where the game reads a table now, and how many records it holds.
static unsigned char *tableAddress(int table) {
    const TableMove *t = &tableMoves[table];
    return t->moved ? t->moved : gameBase + t->rva;
}
static int tableRecords(int table) { return tableMoves[table].moved ? tableMoves[table].movedRecords : tableMoves[table].gameRecords; }

// A monster's record in the table the game reads now, or NULL for a number the table doesn't hold.
static unsigned char *monsterRecord(int monster) {
    return monster >= 0 && monster < tableRecords(TABLE_MONSTER) ? tableAddress(TABLE_MONSTER) + (size_t)monster * MONSTER_SIZE : NULL;
}

// The table holding a game address, for records of `stride` bytes; -1 for none.
static int tableHolding(unsigned int rva, int stride) {
    for (int i = 0; i < TABLE_MOVE_COUNT; i++)
        if (tableMoves[i].rva && rva >= tableMoves[i].rva && rva < tableMoves[i].rva + (unsigned int)tableMoves[i].recordSize &&
            stride == tableMoves[i].recordSize)
            return i;
    return -1;
}

// ---- The refusal of an entry past a table's last slot ------------------------------------------
// A refused entry's lines are skipped up to the next line that starts or selects something.
static struct { int active, table, mod, line, skipped; char name[64]; } refusedTableEntry;
// The name on the latest line that starts a table's entry, for the refusal.
static char tableEntryLineName[64];

// The mods that had entries refused in the latest pass.
#define TABLE_LEFT_OUT_LINES 16
#define TABLE_LEFT_OUT_ROWS  10
static struct {
    char (*mods)[MOD_NAME_MAX];
    int count, capacity, level;
    unsigned int shownHash;
} tableLeftOut[TABLE_MOVE_COUNT];
static int tableLeftOutLastLevel = -1;

static void endRefusedTableEntry(void) {
    if (!refusedTableEntry.active) return;
    refusedTableEntry.active = 0;
    int known = refusedTableEntry.mod >= 0 && refusedTableEntry.mod <= G32(RVA_MOD_LAST_INDEX);
    writeLog("table_refused_lines_skipped", "table=%s mod=%s name=%.48s lines=%d", tableMoves[refusedTableEntry.table].name,
             known ? modName(refusedTableEntry.mod) : "-", refusedTableEntry.name, refusedTableEntry.skipped);
}

// Adds mod `mod` to table `table`'s list, once.
static void noteTableLeftOut(int table, const char *mod) {
    for (int i = 0; i < tableLeftOut[table].count; i++)
        if (!strcmp(tableLeftOut[table].mods[i], mod)) return;
    if (!growList((void **)&tableLeftOut[table].mods, &tableLeftOut[table].capacity, tableLeftOut[table].count + 1,
                  sizeof *tableLeftOut[table].mods))
        return;
    snprintf(tableLeftOut[table].mods[tableLeftOut[table].count++], MOD_NAME_MAX, "%s", mod);
}

// A table's pop-up text: its opening line, then one mod per line, by its display name.
static size_t tableLeftOutText(int table, char *text, size_t size) {
    size_t n = 0;
    char label[80];
    text[0] = '\0';
    appendText(text, size, &n, "%s\n", tableMoves[table].opening);
    for (int i = 0; i < tableLeftOut[table].count && i < TABLE_LEFT_OUT_LINES; i++) {
        modDisplayName(tableLeftOut[table].mods[i], label, sizeof label);
        appendText(text, size, &n, "\n%s", label);
    }
    if (tableLeftOut[table].count > TABLE_LEFT_OUT_LINES) appendText(text, size, &n, "\n...and others.");
    return n;
}

// Each table's pop-up, once for each new set of mods.
static void popupTablesLeftOut(void) {
    static char text[POPUP_TEXT_SIZE];
    endRefusedTableEntry();
    for (int t = 0; t < TABLE_MOVE_COUNT; t++) {
        if (!tableLeftOut[t].count) {
            tableLeftOut[t].shownHash = 0;
            continue;
        }
        tableLeftOutText(t, text, sizeof text);
        unsigned int hash = 2166136261u;
        for (const char *c = text; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
        if (hash == tableLeftOut[t].shownHash) continue;
        tableLeftOut[t].shownHash = hash;
        writeLog("table_left_out_popup", "table=%s mods=%d", tableMoves[t].name, tableLeftOut[t].count);
        okBox(tableMoves[t].title, text);
    }
}

// The CoE5SE page's box for each table whose entries were left out.
static void drawTablesLeftOut(void) {
    char label[80], tip[256];
    for (int t = 0; t < TABLE_MOVE_COUNT; t++) {
        if (!tableLeftOut[t].count) continue;
        sectionBegin();
        sectionHeader(tableMoves[t].header);
        for (int i = 0; i < tableLeftOut[t].count && i < TABLE_LEFT_OUT_ROWS; i++) {
            modDisplayName(tableLeftOut[t].mods[i], label, sizeof label);
            textButton(0, label);
            snprintf(tip, sizeof tip, "%.79s has more than fit, so its last ones were left out. coe5mod.log has each as c5m_refused.", label);
            tooltip(tip);
        }
        if (tableLeftOut[t].count > TABLE_LEFT_OUT_ROWS) textButton(0, "...and others. coe5mod.log has each as c5m_refused.");
        sectionEnd();
    }
}

// Refuses the entry the parser is starting, named `name`, for table `table`.
static void refuseTableEntry(int table, const char *name) {
    endRefusedTableEntry();
    const TableMove *t = &tableMoves[table];
    if (!name || !*name) name = tableEntryLineName;
    int known = modCommandMod >= 0 && modCommandMod <= G32(RVA_MOD_LAST_INDEX);
    if (known) noteTableLeftOut(table, modName(modCommandMod));
    tableLeftOut[table].level = modCommandLevel;
    refuseC5mLine(t->word, known ? modName(modCommandMod) : "-", t->reason, "line=%d name=%.48s last_slot=%d", G32(RVA_MOD_LINE),
                  name ? name : "", t->lastModSlot);
    refusedTableEntry.active = 1;
    refusedTableEntry.table = table;
    refusedTableEntry.mod = modCommandMod;
    refusedTableEntry.line = G32(RVA_MOD_LINE);
    refusedTableEntry.skipped = 0;
    snprintf(refusedTableEntry.name, sizeof refusedTableEntry.name, "%s", name ? name : "");
}

// Copies a line's argument after its word, its quotes taken off, into `out`.
static void quotedLineName(const char *args, char *out, size_t size) {
    const char *name = args + strspn(args, " \t\"");
    size_t n = strcspn(name, "\"\r\n");
    snprintf(out, size, "%.*s", (int)(n < size ? n : size - 1), name);
}

// Returns 1 for a line of a refused entry, which the game then never sees.
static int refusedTableLine(const char *line, size_t len, int mod, int level) {
    if (level != tableLeftOutLastLevel) {
        for (int t = 0; t < TABLE_MOVE_COUNT; t++)
            if (level == tableLeftOut[t].level) tableLeftOut[t].count = 0;
        tableLeftOutLastLevel = level;
    }
    int starter = -1;
    for (int t = 0; t < TABLE_MOVE_COUNT; t++) {
        if (!tableMoves[t].word || len != strlen(tableMoves[t].word) || strncmp(line, tableMoves[t].word, len)) continue;
        starter = t;
        quotedLineName(line + len, tableEntryLineName, sizeof tableEntryLineName);
    }
    if (refusedTableEntry.active) {
        int number = G32(RVA_MOD_LINE);
        int starts = (len >= 3 && !strncmp(line, "new", 3)) || (len >= 6 && !strncmp(line, "select", 6));
        if (mod != refusedTableEntry.mod || number < refusedTableEntry.line || starts) {
            endRefusedTableEntry();
        } else {
            const char *nameWord = tableMoves[refusedTableEntry.table].nameWord;
            if (nameWord && len == strlen(nameWord) && !strncmp(line, nameWord, len))
                quotedLineName(line + len, refusedTableEntry.name, sizeof refusedTableEntry.name);
            refusedTableEntry.line = number;
            refusedTableEntry.skipped++;
            return 1;
        }
    }
    if (starter >= 0) {
        const TableMove *t = &tableMoves[starter];
        if (t->moved && t->countRva && G32(t->countRva) + t->countSlack >= t->movedRecords) {
            refuseTableEntry(starter, NULL);
            return 1;
        }
    }
    return 0;
}

#ifndef __APPLE__
// ---- Windows: the patch ------------------------------------------------------------------------
// The weapon table's "Out of weapons '%s'" trace, where the free-slot search returns -1.
static void weaponOutOfSlotsHook(const char *fmt, const char *name) {
    (void)fmt;
    refuseTableEntry(TABLE_WEAPON, name);
}

// Logs a read or write of an old table before the game stops on it.
static LONG WINAPI logOldTableAccess(EXCEPTION_POINTERS *e) {
    const EXCEPTION_RECORD *r = e->ExceptionRecord;
    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2) return EXCEPTION_CONTINUE_SEARCH;
    const unsigned char *target = (const unsigned char *)r->ExceptionInformation[1];
    for (int i = 0; i < TABLE_MOVE_COUNT; i++) {
        const TableMove *t = &tableMoves[i];
        if (!t->oldStart || target < t->oldStart || target >= t->oldEnd) continue;
        const unsigned char *at = r->ExceptionAddress;
        ptrdiff_t fromTable = target - (gameBase + t->rva);
        writeLog("table_old_access", "table=%s instruction=%p instruction_rva=0x%llx write=%d slot=%d field=0x%x", t->name, (const void *)at,
                 (unsigned long long)(at - gameBase), (int)r->ExceptionInformation[0], (int)(fromTable / t->recordSize),
                 (unsigned)(fromTable % t->recordSize));
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// The changes to the game's code for one block of tables.
#define TABLE_MOVE_MAX_WRITES 8192
static struct { unsigned char *at; int size; unsigned char bytes[15]; } tableMoveWrites[TABLE_MOVE_MAX_WRITES];
static int tableMoveWriteCount;

static int queueTableBytes(unsigned char *at, const void *bytes, int size) {
    if (tableMoveWriteCount >= TABLE_MOVE_MAX_WRITES || size > (int)sizeof tableMoveWrites[0].bytes) return 0;
    tableMoveWrites[tableMoveWriteCount].at = at;
    tableMoveWrites[tableMoveWriteCount].size = size;
    memcpy(tableMoveWrites[tableMoveWriteCount++].bytes, bytes, (size_t)size);
    return 1;
}
static int queueTableWrite(unsigned char *at, int value) { return queueTableBytes(at, &value, 4); }

// The class picker's click decode, so classes past 99 can be picked.
static unsigned char *classPickerStub;
static const unsigned char CLASS_PICKER_DECODE_SHIPPED[8] = {0x8d, 0x43, 0x9c, 0x83, 0xf8, 0x63, 0x76, 0x4a};
#define CLASS_PICKER_STUB_BYTES 0x60

// Appends a rel32 jump (`opcode`, one or two bytes) from `*at` to game RVA `target`; 0 when out of reach.
static int stubJump(unsigned char **at, const unsigned char *opcode, int opcodeSize, unsigned int target) {
    memcpy(*at, opcode, (size_t)opcodeSize);
    long long rel = (gameBase + target) - (*at + opcodeSize + 4);
    if (rel < INT32_MIN || rel > INT32_MAX) return 0;
    int out = (int)rel;
    memcpy(*at + opcodeSize, &out, 4);
    *at += opcodeSize + 4;
    return 1;
}

static int queueClassPickerDecode(const struct TableMove *t) {
    unsigned char *decode = gameBase + RVA_CLASS_PICKER_DECODE, *p = classPickerStub;
    if (!p || memcmp(decode, CLASS_PICKER_DECODE_SHIPPED, sizeof CLASS_PICKER_DECODE_SHIPPED)) {
        writeLog("table_site_mismatch", "table=%s group=extra name=picker_decode rva=0x%x stub=%d", t->name, RVA_CLASS_PICKER_DECODE, p != NULL);
        return 0;
    }
    static const unsigned char JBE[] = {0x0f, 0x86}, JNS[] = {0x0f, 0x89}, JMP[] = {0xe9};
    const int last = LAST_MOD_CLASS_SLOT;
    const int pickBack = -CLASS_PICKER_PICK_BASE, infoBack = -CLASS_PICKER_INFO_BASE;
    int ok = 1;
    *p++ = 0x8d; *p++ = 0x43; *p++ = (unsigned char)pickBack;
    *p++ = 0x3d; memcpy(p, &last, 4); p += 4;
    ok &= stubJump(&p, JBE, 2, RVA_CLASS_PICKER_RETURN);
    static const unsigned char ABS[] = {0xbe, 0x08, 0x00, 0x00, 0x00, 0x89, 0xd8, 0x99, 0x31, 0xd0, 0x29, 0xd0};
    memcpy(p, ABS, sizeof ABS); p += sizeof ABS;
    *p++ = 0x8d; *p++ = 0x88; memcpy(p, &infoBack, 4); p += 4;
    *p++ = 0x81; *p++ = 0xf9; memcpy(p, &last, 4); p += 4;
    ok &= stubJump(&p, JBE, 2, RVA_CLASS_PICKER_INFO_CALL);
    *p++ = 0x85; *p++ = 0xdb;
    ok &= stubJump(&p, JNS, 2, RVA_CLASS_PICKER_LOOP);
    *p++ = 0x8d; *p++ = 0x48; *p++ = (unsigned char)pickBack;
    *p++ = 0x81; *p++ = 0xf9; memcpy(p, &last, 4); p += 4;
    ok &= stubJump(&p, JBE, 2, RVA_CLASS_PICKER_INFO_CALL);
    ok &= stubJump(&p, JMP, 1, RVA_CLASS_PICKER_LOOP);
    _Static_assert(CLASS_PICKER_PICK_BASE <= 128, "the stub's lea holds -100 in a byte");
    long long toStub = classPickerStub - (decode + 5);
    if (!ok || p - classPickerStub > CLASS_PICKER_STUB_BYTES || toStub < INT32_MIN || toStub > INT32_MAX) {
        writeLog("table_site_mismatch", "table=%s group=extra name=picker_stub out_of_reach=1", t->name);
        return 0;
    }
    unsigned char jump[5] = {0xe9};
    int rel = (int)toStub;
    memcpy(jump + 1, &rel, 4);
    return queueTableBytes(decode, jump, 5);
}

// Checks every site of table `t` against the game's bytes and queues its new field.
static int queueTableMoveWrites(const TableMove *t, unsigned char *table) {
    long long delta = table - (gameBase + t->rva);
    int matched = 0;
    for (int i = 0; i < t->siteCount; i++) {
        const TableMoveSite *s = &t->sites[i];
        unsigned char *at = gameBase + s->rva;
        int field;
        memcpy(&field, at + s->fieldOffset, 4);
        long long moved = field + delta;
        if (memcmp(at, s->bytes, (size_t)s->fieldOffset + 4) || moved < INT32_MIN || moved > INT32_MAX || !queueTableWrite(at + s->fieldOffset, (int)moved)) {
            writeLog("table_site_mismatch", "table=%s group=table rva=0x%x out_of_reach=%d", t->name, s->rva, moved < INT32_MIN || moved > INT32_MAX);
            continue;
        }
        matched++;
    }
    for (int pass = 0; pass < 2; pass++) {
        const TableValueSite *list = pass ? t->values : t->bounds;
        for (int i = 0, n = pass ? t->valueCount : t->boundCount; i < n; i++) {
            const TableValueSite *s = &list[i];
            if (memcmp(gameBase + s->rva, s->shipped, (size_t)s->size) || !queueTableWrite(gameBase + s->rva + s->fieldOffset, s->value)) {
                writeLog("table_site_mismatch", "table=%s group=%s name=%s rva=0x%x", t->name, pass ? "value" : "bound", s->name, s->rva);
                continue;
            }
            matched++;
        }
    }
    for (int i = 0; i < t->patchCount; i++) {
        const TablePatchSite *s = &t->patches[i];
        if (memcmp(gameBase + s->rva, s->shipped, (size_t)s->size) || !queueTableBytes(gameBase + s->rva, s->patched, s->size)) {
            writeLog("table_site_mismatch", "table=%s group=patch rva=0x%x", t->name, s->rva);
            continue;
        }
        matched++;
    }
    if (t->queueExtra) matched += t->queueExtra(t);
    if (!t->overflowCall) return matched;
    const unsigned char *call = gameBase + t->overflowCall;
    int rel;
    memcpy(&rel, call + 1, 4);
    if (call[0] != 0xe8 || call + 5 + rel != gameBase + t->overflowTarget)
        writeLog("table_site_mismatch", "table=%s group=call rva=0x%x", t->name, t->overflowCall);
    else
        matched++;
    return matched;
}

// How many sites table `t`'s move checks.
static int tableMoveSiteCount(const TableMove *t) {
    return t->siteCount + t->boundCount + t->valueCount + t->patchCount + t->extraCount + (t->overflowCall != 0);
}

// Makes the old table's whole pages unreadable.
static void protectOldTable(TableMove *t) {
    unsigned char *old = gameBase + t->rva;
    t->oldStart = (unsigned char *)(((uintptr_t)old + 0xfff) & ~(uintptr_t)0xfff);
    t->oldEnd = (unsigned char *)(((uintptr_t)old + (size_t)t->gameRecords * (size_t)t->recordSize) & ~(uintptr_t)0xfff);
    size_t pages = t->oldEnd > t->oldStart ? (size_t)(t->oldEnd - t->oldStart) / 0x1000 : 0;
    DWORD previous;
    int guarded = !pages || VirtualProtect(t->oldStart, pages * 0x1000, PAGE_NOACCESS, &previous) != 0;
    if (!guarded || !pages) t->oldStart = t->oldEnd = NULL;
    writeLog("table_old_protected", "table=%s ok=%d error=%lu pages=%zu", t->name, guarded, guarded ? 0UL : GetLastError(), pages);
}

// Moves the `count` tables from `first`, one block in their order, or changes nothing.
#define TABLE_GROUP_MAX 8
#define TABLE_BLOCK_SLACK 64
static int installTableGroup(TableMove *first, int count, unsigned char *stubs) {
    size_t at[TABLE_GROUP_MAX], total = 0;
    int sites[TABLE_GROUP_MAX], matched[TABLE_GROUP_MAX], needStub = 0;
    if (count > TABLE_GROUP_MAX) return 0;
    for (int k = 0; k < count; k++) {
        at[k] = total;
        total += (size_t)first[k].movedRecords * (size_t)first[k].recordSize;
        sites[k] = tableMoveSiteCount(&first[k]);
        needStub |= first[k].overflowCall != 0;
    }
    total += TABLE_BLOCK_SLACK;
    unsigned char *block = allocNearImage(total);
    if (!block || (needStub && !stubs)) {
        for (int k = 0; k < count; k++)
            writeLog("table_not_moved", "table=%s reason=no_memory_near_game bytes=%zu sites=0 of %d", first[k].name, total, sites[k]);
        if (block) VirtualFree(block, 0, MEM_RELEASE);
        return 0;
    }
    int all = 1;
    tableMoveWriteCount = 0;
    for (int k = 0; k < count; k++) {
        matched[k] = queueTableMoveWrites(&first[k], block + at[k]);
        all &= matched[k] == sites[k];
    }
    DWORD textProtection = 0;
    unsigned char *text = gameBase + RVA_TEXT_SECTION;
    size_t textSize = (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION);
    const char *reason = !all ? "game_build_differs"
                         : !VirtualProtect(text, textSize, PAGE_EXECUTE_READWRITE, &textProtection) ? "code_unwritable" : NULL;
    if (reason) {
        for (int k = 0; k < count; k++)
            writeLog("table_not_moved", "table=%s reason=%s sites=%d of %d", first[k].name, reason, matched[k], sites[k]);
        VirtualFree(block, 0, MEM_RELEASE);
        return 0;
    }
    for (int k = 0; k < count; k++)
        memcpy(block + at[k], gameBase + first[k].rva, (size_t)first[k].gameRecords * (size_t)first[k].recordSize);
    for (int i = 0; i < tableMoveWriteCount; i++) {
        unsigned char *to = tableMoveWrites[i].at;
        size_t n = (size_t)tableMoveWrites[i].size;
        if (to >= text && to + n <= text + textSize) {
            memcpy(to, tableMoveWrites[i].bytes, n);
            continue;
        }
        DWORD previous;
        if (!VirtualProtect(to, n, PAGE_READWRITE, &previous)) {
            writeLog("table_patch_unwritable", "rva=0x%llx error=%lu", (unsigned long long)(to - gameBase), GetLastError());
            continue;
        }
        memcpy(to, tableMoveWrites[i].bytes, n);
        VirtualProtect(to, n, previous, &previous);
    }
    VirtualProtect(text, textSize, textProtection, &textProtection);
    FlushInstructionCache(GetCurrentProcess(), text, textSize);
    for (int k = 0; k < count; k++) {
        TableMove *t = &first[k];
        t->moved = block + at[k];
        int hooked = !t->overflowCall || redirectCall(t->overflowCall, t->overflowTarget, stubs + 0x20 * (t - tableMoves), t->overflowHook, t->word, NULL);
        writeLog("table_moved", "table=%s from=%p to=%p rva=0x%llx records=%d mod_slots=%d-%d refusal=%d sites=%d of %d", t->name,
                 (void *)(gameBase + t->rva), (void *)t->moved, (unsigned long long)(t->moved - gameBase), t->movedRecords, t->firstModSlot,
                 t->lastModSlot, hooked, matched[k], sites[k]);
        protectOldTable(t);
    }
    return count;
}
#else
// ---- Mac: the patch ----------------------------------------------------------------------------
// The weapon table's "Out of weapons '%s'" trace, where the free-slot search returns -1.
static void weaponOutOfSlotsHook(const char *fmt, ...) {
    (void)fmt;
    refuseTableEntry(TABLE_WEAPON, NULL);
}

// Logs a read or write of an old table before the game stops on it.
static struct sigaction previousTableSegv, previousTableBus;
static void logOldTableAccess(int signal, siginfo_t *info, void *context) {
    const unsigned char *target = info->si_addr;
    for (int i = 0; i < TABLE_MOVE_COUNT; i++) {
        const TableMove *t = &tableMoves[i];
        if (!t->oldStart || target < t->oldStart || target >= t->oldEnd) continue;
        const ucontext_t *uc = context;
        const unsigned char *at = (const unsigned char *)__darwin_arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
        ptrdiff_t fromTable = target - (gameBase + t->rva);
        writeLog("table_old_access", "table=%s instruction=%p instruction_va=0x%llx slot=%d field=0x%x", t->name, (const void *)at,
                 (unsigned long long)(at - gameBase) + 0x100000000ULL, (int)(fromTable / t->recordSize), (unsigned)(fromTable % t->recordSize));
    }
    sigaction(signal, signal == SIGBUS ? &previousTableBus : &previousTableSegv, NULL);
}

// The changes to the game's code for one block of tables.
#define TABLE_MOVE_MAX_WRITES 4096
static struct { unsigned char *at; uint32_t word, old; } tableMoveWrites[TABLE_MOVE_MAX_WRITES];
static int tableMoveWriteCount;

static int queueTableWord(unsigned int rva, uint32_t word) {
    if (tableMoveWriteCount >= TABLE_MOVE_MAX_WRITES) return 0;
    tableMoveWrites[tableMoveWriteCount].at = gameBase + rva;
    tableMoveWrites[tableMoveWriteCount].word = word;
    memcpy(&tableMoveWrites[tableMoveWriteCount++].old, gameBase + rva, 4);
    return 1;
}

static uint32_t tableGameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

// A bound's new word with its immediate set to `value`; 0 when it can't hold it.
static uint32_t tableBoundWord(uint32_t word, int value) {
    if ((word & 0x7f800000u) == 0x52800000u && value >= 0 && value <= 0xffff) return (word & ~(0xffffu << 5)) | (uint32_t)value << 5;
    if ((word & 0x7fc0001fu) == 0x7100001fu && value >= 0 && value <= 0xfff) return (word & ~(0xfffu << 10)) | (uint32_t)value << 10;
    return 0;
}

// The class picker's click decode, so classes past 99 can be picked.
#define CLASS_PICKER_INFO_PATH (RVA_CLASS_PICKER_INFO_CALL - 32)
static const uint32_t CLASS_PICKER_DECODE_SHIPPED[4] = {0x51019320u, 0x7101901fu, 0x54000123u, 0x1104af28u};
static const uint32_t CLASS_PICKER_INFO_PATH_SHIPPED[8] = {0x7100033fu, 0x5a995728u, 0x53023d09u, 0x52828f6au,
                                                           0x1b0a7d29u, 0x53117d29u, 0x52800c8au, 0x1b0aa128u};
#define CLASS_PICKER_INFO_ID_SHIPPED 0x11032320u
#define CLASS_PICKER_INFO_CALL_SHIPPED 0x12003d00u
static uint32_t a64SubImm(unsigned rd, unsigned rn, unsigned imm) { return 0x51000000u | imm << 10 | rn << 5 | rd; }
static uint32_t a64CmpImm(unsigned rn, unsigned imm) { return 0x7100001fu | imm << 10 | rn << 5; }
static uint32_t a64Branch(uint32_t op, unsigned int from, unsigned int to) {
    int words = (int)(to - from) / 4;
    return op == 0x14000000u ? op | ((uint32_t)words & 0x3ffffffu) : op | ((uint32_t)words & 0x7ffffu) << 5;
}

static int queueClassPickerDecode(const struct TableMove *t) {
    int shipped = tableGameWord(RVA_CLASS_PICKER_INFO_ID) == CLASS_PICKER_INFO_ID_SHIPPED &&
                  tableGameWord(RVA_CLASS_PICKER_INFO_CALL) == CLASS_PICKER_INFO_CALL_SHIPPED;
    for (int i = 0; i < 4; i++) shipped &= tableGameWord(RVA_CLASS_PICKER_DECODE + 4 * (unsigned)i) == CLASS_PICKER_DECODE_SHIPPED[i];
    for (int i = 0; i < 8; i++) shipped &= tableGameWord(CLASS_PICKER_INFO_PATH + 4 * (unsigned)i) == CLASS_PICKER_INFO_PATH_SHIPPED[i];
    if (!shipped) {
        writeLog("table_site_mismatch", "table=%s group=extra name=picker_decode va=0x%llx", t->name, RVA_CLASS_PICKER_DECODE + 0x100000000ULL);
        return 0;
    }
    const unsigned last = LAST_MOD_CLASS_SLOT, path = CLASS_PICKER_INFO_PATH;
    const unsigned toInfo = CLASS_PICKER_INFO_BASE - CLASS_PICKER_PICK_BASE;
    _Static_assert(CLASS_PICKER_INFO_BASE + LAST_MOD_CLASS_SLOT <= 0xfff, "the info id fits add's 12 bits");
    _Static_assert(CLASS_PICKER_INFO_BASE - CLASS_PICKER_PICK_BASE > LAST_MOD_CLASS_SLOT, "the bands stay apart");
    uint32_t info[8] = {
        0x7100033fu,
        0x5a995729u,
        a64SubImm(8, 9, CLASS_PICKER_PICK_BASE),
        a64SubImm(10, 8, toInfo),
        a64CmpImm(8, toInfo),
        0x1a882148u,
        a64CmpImm(8, last),
        a64Branch(0x54000008u, path + 28, RVA_CLASS_PICKER_LOOP),
    };
    int ok = 1;
    for (int i = 0; i < 8; i++) ok &= queueTableWord(path + 4 * (unsigned)i, info[i]);
    ok &= queueTableWord(RVA_CLASS_PICKER_DECODE + 4, a64CmpImm(0, last + 1));
    ok &= queueTableWord(RVA_CLASS_PICKER_DECODE + 12, a64Branch(0x14000000u, RVA_CLASS_PICKER_DECODE + 12, path));
    ok &= queueTableWord(RVA_CLASS_PICKER_INFO_ID, (CLASS_PICKER_INFO_ID_SHIPPED & ~(0xfffu << 10)) | (uint32_t)CLASS_PICKER_INFO_BASE << 10);
    return ok;
}

// Checks every site of table `t` against the game's words and queues its new word.
static int queueTableMoveWrites(const TableMove *t, unsigned char *table) {
    long long pages = ((long long)(uintptr_t)table >> ADRP_PAGE_SHIFT) - ((long long)(uintptr_t)(gameBase + t->rva) >> ADRP_PAGE_SHIFT);
    int matched = 0;
    for (int i = 0; i < t->macSiteCount; i++) {
        const TableMacSite *s = &t->macSites[i];
        uint32_t moved = 0;
        if (tableGameWord(s->rva) != s->word || !adrpMovedBy(s->word, pages, &moved) || !queueTableWord(s->rva, moved)) {
            writeLog("table_site_mismatch", "table=%s group=table va=0x%llx", t->name, s->rva + 0x100000000ULL);
            continue;
        }
        matched++;
    }
    for (int i = 0; i < t->macBoundCount; i++) {
        const TableMacBound *s = &t->macBounds[i];
        uint32_t word = tableBoundWord(s->word, s->value);
        if (tableGameWord(s->rva) != s->word || !word || !queueTableWord(s->rva, word)) {
            writeLog("table_site_mismatch", "table=%s group=bound va=0x%llx", t->name, s->rva + 0x100000000ULL);
            continue;
        }
        matched++;
    }
    unsigned int bytes = (unsigned int)t->movedRecords * (unsigned int)t->recordSize;
    for (int i = 0; i < t->macSizeCount; i++) {
        const TableMacSize *s = &t->macSizes[i];
        if (tableGameWord(s->rva) != s->word || tableGameWord(s->rva + 4) != s->next ||
            !queueTableWord(s->rva, (s->word & ~(0xffffu << 5)) | (bytes & 0xffff) << 5) ||
            !queueTableWord(s->rva + 4, (s->next & ~(0xffffu << 5)) | (bytes >> 16) << 5)) {
            writeLog("table_site_mismatch", "table=%s group=size va=0x%llx", t->name, s->rva + 0x100000000ULL);
            continue;
        }
        matched++;
    }
    for (int i = 0; i < t->macPatchCount; i++) {
        const TableMacPatch *s = &t->macPatches[i];
        if (tableGameWord(s->rva) != s->shipped || !queueTableWord(s->rva, s->word)) {
            writeLog("table_site_mismatch", "table=%s group=patch va=0x%llx", t->name, s->rva + 0x100000000ULL);
            continue;
        }
        matched++;
    }
    if (t->queueExtra) matched += t->queueExtra(t);
    if (!t->overflowCall) return matched;
    unsigned char *trampoline = blTarget(gameBase + t->overflowCall) == gameBase + t->overflowTarget ? newTrampoline(t->overflowHook) : NULL;
    if (!trampoline || !queueTableWord(t->overflowCall, blTo(gameBase + t->overflowCall, trampoline)))
        writeLog("table_site_mismatch", "table=%s group=call va=0x%llx trampoline=%d", t->name, t->overflowCall + 0x100000000ULL, trampoline != NULL);
    else
        matched++;
    return matched;
}

// How many sites table `t`'s move checks.
static int tableMoveSiteCount(const TableMove *t) {
    return t->macSiteCount + t->macBoundCount + t->macSizeCount + t->macPatchCount + t->extraCount + (t->overflowCall != 0);
}

// Makes the old table's whole 16 KB pages unreadable.
static void protectOldTable(TableMove *t) {
    unsigned char *old = gameBase + t->rva;
    t->oldStart = (unsigned char *)(((uintptr_t)old + MAC_PAGE_MASK) & ~(uintptr_t)MAC_PAGE_MASK);
    t->oldEnd = (unsigned char *)(((uintptr_t)old + (size_t)t->gameRecords * (size_t)t->recordSize) & ~(uintptr_t)MAC_PAGE_MASK);
    size_t pages = t->oldEnd > t->oldStart ? (size_t)(t->oldEnd - t->oldStart) / (MAC_PAGE_MASK + 1) : 0;
    kern_return_t error = pages ? mach_vm_protect(mach_task_self(), (mach_vm_address_t)t->oldStart, (mach_vm_size_t)(t->oldEnd - t->oldStart),
                                                  FALSE, VM_PROT_NONE) : KERN_SUCCESS;
    if (error || !pages) t->oldStart = t->oldEnd = NULL;
    writeLog("table_old_protected", "table=%s ok=%d error=%d pages=%zu", t->name, !error, error, pages);
}

// Moves the `count` tables from `first`, one block in their order, or changes nothing.
#define TABLE_GROUP_MAX 8
static int installTableGroup(TableMove *first, int count) {
    size_t at[TABLE_GROUP_MAX], total = 0;
    int sites[TABLE_GROUP_MAX], matched[TABLE_GROUP_MAX];
    if (count > TABLE_GROUP_MAX) return 0;
    for (int k = 0; k < count; k++) {
        at[k] = ((total + ADRP_PAGE_MASK) & ~(size_t)ADRP_PAGE_MASK) + ((uintptr_t)(gameBase + first[k].rva) & ADRP_PAGE_MASK);
        total = at[k] + (size_t)first[k].movedRecords * (size_t)first[k].recordSize;
        sites[k] = tableMoveSiteCount(&first[k]);
    }
    total = (total + 64 + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    unsigned char *block = allocAfterImage(total);
    if (!block) {
        for (int k = 0; k < count; k++)
            writeLog("table_not_moved", "table=%s reason=no_memory_near_game bytes=%zu sites=0 of %d", first[k].name, total, sites[k]);
        return 0;
    }
    int all = 1;
    tableMoveWriteCount = 0;
    for (int k = 0; k < count; k++) {
        matched[k] = queueTableMoveWrites(&first[k], block + at[k]);
        all &= matched[k] == sites[k];
    }
    if (!all) {
        for (int k = 0; k < count; k++)
            writeLog("table_not_moved", "table=%s reason=game_build_differs sites=%d of %d", first[k].name, matched[k], sites[k]);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)block, total);
        return 0;
    }
    for (int k = 0; k < count; k++)
        memcpy(block + at[k], gameBase + first[k].rva, (size_t)first[k].gameRecords * (size_t)first[k].recordSize);
    for (int i = 0; i < tableMoveWriteCount; i++) {
        if (writeGameCode(tableMoveWrites[i].at, &tableMoveWrites[i].word, 4)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(tableMoveWrites[j].at, &tableMoveWrites[j].old, 4);
        for (int k = 0; k < count; k++)
            writeLog("table_not_moved", "table=%s reason=code_unwritable va=0x%llx sites=%d of %d", first[k].name,
                     (unsigned long long)(tableMoveWrites[i].at - gameBase) + 0x100000000ULL, matched[k], sites[k]);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)block, total);
        return 0;
    }
    for (int k = 0; k < count; k++) {
        TableMove *t = &first[k];
        t->moved = block + at[k];
        writeLog("table_moved", "table=%s from=%p to=%p va=0x%llx records=%d mod_slots=%d-%d refusal=1 sites=%d of %d", t->name,
                 (void *)(gameBase + t->rva), (void *)t->moved, (unsigned long long)(t->moved - gameBase) + 0x100000000ULL, t->movedRecords,
                 t->firstModSlot, t->lastModSlot, matched[k], sites[k]);
        protectOldTable(t);
    }
    return count;
}
#endif

// Moves every table.
static void tableMovesInit(void) {
    int moved = 0;
#ifndef __APPLE__
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    _Static_assert(TABLE_MOVE_COUNT * 0x20 <= 0x800, "every table's stub fits the page's first half, the picker's stub the rest");
    classPickerStub = stubs ? stubs + 0x800 : NULL;
    for (int i = 0, n; i < TABLE_MOVE_COUNT; i += n) {
        for (n = 1; i + n < TABLE_MOVE_COUNT && tableMoves[i + n - 1].blockWithNext; n++) {}
        moved += installTableGroup(&tableMoves[i], n, stubs);
    }
    if (moved) AddVectoredExceptionHandler(1, logOldTableAccess);
#else
    for (int i = 0, n; i < TABLE_MOVE_COUNT; i += n) {
        for (n = 1; i + n < TABLE_MOVE_COUNT && tableMoves[i + n - 1].blockWithNext; n++) {}
        int mapped = 1;
        for (int k = i; k < i + n; k++) mapped &= tableMoves[k].macSiteCount > 0;
        if (!mapped) {
            for (int k = i; k < i + n; k++) writeLog("table_not_moved", "table=%s reason=mac_not_mapped sites=0 of 0", tableMoves[k].name);
            continue;
        }
        moved += installTableGroup(&tableMoves[i], n);
    }
    if (moved) {
        struct sigaction action;
        memset(&action, 0, sizeof action);
        action.sa_sigaction = logOldTableAccess;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(SIGSEGV, &action, &previousTableSegv);
        sigaction(SIGBUS, &action, &previousTableBus);
    }
#endif
    writeLog("table_moves_installed", "moved=%d of %d", moved, TABLE_MOVE_COUNT);
}
