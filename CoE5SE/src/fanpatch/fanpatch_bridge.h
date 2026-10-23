// The calls between the fan patch's files and CoE5SE.
#pragma once

#include <stddef.h>

unsigned char *fanpatchGameBase(void);
void fanpatchLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int fanpatchGameCodeMatches(const char *fix);     /* 1 when the game's code is the build's (CoE5SE's code hash) */
unsigned char *fanpatchItemTable(void);           /* the item table where the game reads it now */
int fanpatchItemRecords(void);                    /* its records */
int fanpatchNetworkGame(void);                    /* 1 in a network game: the fixes step aside (ADR 0004) */
int fanpatchSpellAboveLevel(int unit);            /* 1 when the unit's battle spells hold one above the level it casts now */
int fanpatchReplacedSprite(unsigned file, unsigned spr);   /* the game's replaced_spr: Mac inlines it */

/* The entry hooks behind minhook/include/MinHook.h: 1 on success. */
int fanpatchHookCreate(void *target, void *detour, void **original);
int fanpatchHookEnable(void *target, int enable);
int fanpatchHookRemove(void *target);

/* The fixes, in fix_*.c. */
int fix_item_reset_install(void);
void fix_item_reset_report(void);
int fix_feeblemind_install(void);
void fix_feeblemind_report(void);
int fix_token_sprites_install(void);
void fix_token_sprites_report(void);
