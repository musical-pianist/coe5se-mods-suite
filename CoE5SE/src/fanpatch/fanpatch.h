// What the fan patch's fix files expect from their header, given by CoE5SE.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
typedef unsigned char BYTE;
typedef uintptr_t ULONG_PTR;
typedef long LONG;
#define InterlockedIncrement(p) __atomic_add_fetch((p), 1, __ATOMIC_SEQ_CST)
#endif
#ifndef _MSC_VER   /* clang on both builds; the fan patch was built with MSVC */
#define _ReturnAddress() __builtin_return_address(0)
#endif

#include "game_addresses.h"
#include "fanpatch_bridge.h"

/* The game's addresses: offsets from the game's base, as CoE5SE keeps them. */
#define GAME_PTR(type, va)      ((type)(fanpatchGameBase() + (va)))
#define GAME_VERSION_TEXT       "5.41"

#ifdef __APPLE__
#define VA_FIND_ALL_MODS        RVA_RUN_ENABLED_MODS   /* Mac inlines find_all_mods into SDL_main; see fix_item_reset.c */
#else
#define VA_FIND_ALL_MODS        RVA_FIND_ALL_MODS
#define VA_REPLACED_SPR         RVA_REPLACED_SPRITE    /* Mac inlines it; see fix_token_sprites.c */
#endif
#define VA_RESTORE_TABLES       RVA_RESET_MODS
/* The item table moves with CoE5SE's table moves (table_move.h): its address and size are read at run time. */
#define VA_ITEMS                ((ULONG_PTR)(fanpatchItemTable() - fanpatchGameBase()))
#define VA_ITEM_COUNT           RVA_ITEM_COUNT
#define ITEM_MAX                fanpatchItemRecords()
#define ITEM_STRIDE             ITEM_SIZE
#define VA_ADD_AFFL             RVA_ADD_AFFLICTION
#define VA_PICK_SPELL           RVA_RANDOM_MEMORIZED_SPELL
#define VA_REBUILD_SPELLS       RVA_CHOOSE_MEMORIZED
#define VA_UNIT_CARD            RVA_UNIT_CARD
#define VA_UNITS                RVA_UNITS
#define VA_UNIT_MAX             RVA_UNIT_COUNT
#define UNIT_STRIDE             UNIT_SIZE
#define VA_DRAW_SPRITE          RVA_SPRITE_DRAW_SCALED
#define VA_LOAD_SPRFILE         RVA_LOAD_SPRITE_FILE
#define VA_SPRFILE_TABLE        RVA_SPRITE_FILES
#define VA_SPR_WIDTH            RVA_SPRITE_WIDTHS
#define VA_SPR_HEIGHT           RVA_SPRITE_HEIGHTS
#ifdef __APPLE__
#define VA_TOKEN_ICON_RET       (RVA_TOKEN_ITEM_ICON_CALL + 4)   /* after the bl */
#else
#define VA_TOKEN_ICON_RET       (RVA_TOKEN_ITEM_ICON_CALL + 5)   /* after the call rel32 */
#endif

/* The fan patch's helpers. fp_config_int: CoE5SE has no fanpatch.ini, so every fix takes its default. */
#define fp_log                  fanpatchLog
#define fp_config_int(section, key, def) (def)
/* CoE5SE: the fixes' byte signatures became one check of CoE5SE's game code hash. */
#define fp_game_code_matches    fanpatchGameCodeMatches
