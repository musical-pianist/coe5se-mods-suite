/*
 * fix_item_reset.c - #4: items are reset like everything else when the game
 * re-reads mods (resetmods, new game, changing the mod list).
 *
 * The bug
 *   At startup the game copies the built-in monster, class, recruitment,
 *   weapon, ritual and terrain tables aside. Before mods
 *   are re-read, VA_RESTORE_TABLES copies them back, so every mod starts from
 *   clean data. The item table (1000 items of 0x1d0 bytes) is in neither
 *   list, in v5.40 or v5.41. So
 *   item changes made by a mod stay after the mod is turned off, and items
 *   added with newitem pile up: each reload adds them again after the old
 *   copies (item 300, then 301, ...).
 *
 * The fix
 *   Before the game's own restore, the item table and the item count are
 *   copied back from a copy the fan patch takes at startup.
 *   In v5.40 that copy was taken right after the game's backup function. In
 *   v5.41 the backup is built into the game's main function (the separate
 *   copy of it is never called), so the copy is taken when the game next
 *   calls findallmods, which comes straight after the backup and before any
 *   mod is read.
 *
 *   Why it matters for sprites: each time mods are read, the game numbers mod
 *   item images from the highest sprite number of any item still in the
 *   table, and forgets the previous images. Without the reset, the old copies
 *   of modded items keep sprite numbers that no longer have an image, so an
 *   old copy (for example one handed out as a random item) shows a missing
 *   sprite.
 *
 * CoE5SE: this file is Alex the Beetrayer's fan patch fix, built into CoE5SE; each change is marked
 *   "CoE5SE:". The item table is CoE5SE's moved one (2000 items on Windows): VA_ITEMS and ITEM_MAX
 *   in fanpatch.h read it at run time. The Mac build inlines findallmods into the game's main
 *   function, so there the copy is taken at the first run of the enabled mods instead (the startup
 *   pass, before any item is read). In a network game the fix steps aside (CoE5SE's ADR 0004).
 */

#include "fanpatch.h"
#include "minhook/include/MinHook.h"
#include <stdlib.h>
#include <string.h>

#define ITEM_TABLE_BYTES   ((size_t)ITEM_MAX * ITEM_STRIDE)

#ifdef __APPLE__
/* CoE5SE: the Mac build's first bytes of run_enabled_mods and resetmods. */
static const BYTE SIG_FIND_ALL_MODS[] = { 0xFF,0x43,0x01,0xD1,0xF6,0x57,0x02,0xA9,0xF4,0x4F,0x03,0xA9,0xFD,0x7B,0x04,0xA9 };
static const BYTE SIG_RESTORE[] = { 0xFC,0x6F,0xBA,0xA9,0xFA,0x67,0x01,0xA9,0xF8,0x5F,0x02,0xA9,0xF6,0x57,0x03,0xA9 };
#else
static const BYTE SIG_FIND_ALL_MODS[] = { 0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x6C,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48 };
static const BYTE SIG_FIND_CALL[]     = { 0xE8 };
static const BYTE SIG_RESTORE[] = { 0x40,0x57,0x41,0x54,0x41,0x57,0x48,0x81,0xEC,0x10,0x0A,0x00,0x00 };
#endif

typedef void (*VoidFn_t)(void);
#ifdef __APPLE__
typedef void (*LevelFn_t)(int level);   /* CoE5SE: run_enabled_mods takes the reading level */
static LevelFn_t o_find_all_mods;
static VoidFn_t o_restore;
#else
static VoidFn_t o_find_all_mods, o_restore;
#endif

static BYTE *s_items_saved;
static int   s_count_saved;
static long  s_restores;

static int last_item_number(void)
{
    const BYTE *items = GAME_PTR(const BYTE *, VA_ITEMS);
    int i;

    /* the table ends with an entry whose sprite number is -999 */
    for (i = 0; i < ITEM_MAX; i++)
        if (*(const short *)(items + (size_t)i * ITEM_STRIDE + 0x30) == -999)
            return i - 1;
    return ITEM_MAX - 1;
}

static void save_items(void)
{
    if (s_items_saved)
        return;                 /* only the first time: built-in items */
    s_items_saved = (BYTE *)malloc(ITEM_TABLE_BYTES);
    if (!s_items_saved) {
        fp_log("fix item_reset: out of memory; items will not be reset");
        return;
    }
    memcpy(s_items_saved, GAME_PTR(void *, VA_ITEMS), ITEM_TABLE_BYTES);
    s_count_saved = *GAME_PTR(int *, VA_ITEM_COUNT);
    fp_log("fix item_reset: built-in items saved (%d items, last number %d)",
           last_item_number() + 1, s_count_saved);
}

#ifdef __APPLE__
static void hk_find_all_mods(int level)
{
    save_items();
    o_find_all_mods(level);
}
#else
static void hk_find_all_mods(void)
{
    save_items();
    o_find_all_mods();
}
#endif

static void hk_restore(void)
{
    if (s_items_saved && !fanpatchNetworkGame()) {   /* CoE5SE: the game's own in a network game */
        int before = last_item_number();
        memcpy(GAME_PTR(void *, VA_ITEMS), s_items_saved, ITEM_TABLE_BYTES);
        *GAME_PTR(int *, VA_ITEM_COUNT) = s_count_saved;
        s_restores++;
        fp_log("fix item_reset: items reset to built-in (last item was %d, now %d)",
               before, last_item_number());
    }
    o_restore();
}

static int hook(ULONG_PTR va, void *detour, void **orig, const char *what)
{
    void *target = GAME_PTR(void *, va);
    MH_STATUS st = MH_CreateHook(target, detour, orig);

    if (st != MH_OK) {
        fp_log("fix item_reset: MH_CreateHook(%s) failed (%s)", what, MH_StatusToString(st));
        return 0;
    }
    st = MH_EnableHook(target);
    if (st != MH_OK) {
        fp_log("fix item_reset: MH_EnableHook(%s) failed (%s)", what, MH_StatusToString(st));
        MH_RemoveHook(target);
        return 0;
    }
    return 1;
}

int fix_item_reset_install(void)
{
    if (!fp_config_int("fixes", "item_reset", 1)) {
        fp_log("fix item_reset: off in fanpatch.ini");
        return 0;
    }
    if (
#ifndef __APPLE__   /* CoE5SE: Mac has no call to findallmods to check */
        !fp_check_bytes(VA_FIND_ALL_MODS_CALL, SIG_FIND_CALL, sizeof SIG_FIND_CALL, "findallmods call") ||
        *GAME_PTR(int *, VA_FIND_ALL_MODS_CALL + 1) != (int)(VA_FIND_ALL_MODS - (VA_FIND_ALL_MODS_CALL + 5)) ||
#endif
        !fp_check_bytes(VA_FIND_ALL_MODS, SIG_FIND_ALL_MODS, sizeof SIG_FIND_ALL_MODS, "findallmods") ||
        !fp_check_bytes(VA_RESTORE_TABLES, SIG_RESTORE, sizeof SIG_RESTORE, "table restore")) {
        fp_log("fix item_reset: NOT installed (code does not match " GAME_VERSION_TEXT ")");
        return 0;
    }
    if (!hook(VA_FIND_ALL_MODS, (void *)hk_find_all_mods, (void **)&o_find_all_mods, "findallmods"))
        return 0;
    if (!hook(VA_RESTORE_TABLES, (void *)hk_restore, (void **)&o_restore, "restore")) {
        MH_DisableHook(GAME_PTR(void *, VA_FIND_ALL_MODS));
        MH_RemoveHook(GAME_PTR(void *, VA_FIND_ALL_MODS));
        return 0;
    }
    fp_log("fix item_reset: installed");
    return 1;
}

void fix_item_reset_report(void)
{
    fp_log("fix item_reset: items reset %ld time(s)%s", s_restores,
           s_items_saved ? "" : " (built-in items were never saved)");
}
