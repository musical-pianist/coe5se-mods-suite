// The player table move: PLAYER_SLOTS_MOVED slots instead of the game's 32, for mods' factions.
#include <stdint.h>
#ifndef __APPLE__
#include "player_table_sites.h"
#else
#include "player_table_sites_mac.h"
#endif

_Static_assert(PLAYER_SLOTS_MOVED <= 0x7f, "the Windows bounds hold it in an imm8");

static int playerTableMoved;

#ifndef __APPLE__
// Each moved table: the game's address and one slot's entry size.
static const struct { unsigned int rva; int entry; } PLAYER_TABLES[PLAYER_GROUP_COUNT] = {
    [PLAYER_GROUP_BLOCKS] = {RVA_SLOT_NAME, PLAYER_BLOCK_SIZE}, [PLAYER_GROUP_CONTROLLER] = {RVA_SLOT_CONTROLLER, 2},
    [PLAYER_GROUP_CLASS] = {RVA_SLOT_CLASS, 2},                 [PLAYER_GROUP_HOME_X] = {RVA_PLAYER_HOME_X, 2},
    [PLAYER_GROUP_HOME_Y] = {RVA_PLAYER_HOME_Y, 2},             [PLAYER_GROUP_COLOURS] = {RVA_PLAYER_COLOURS, 12},
    [PLAYER_GROUP_PLANE_SEEN] = {RVA_PLAYER_PLANE_SEEN, 11},
};
#else
static const struct { unsigned int rva; int entry; } PLAYER_TABLES[PLAYER_GROUP_COUNT] = {
    [PLAYER_GROUP_BLOCKS] = {RVA_SLOT_NAME, PLAYER_BLOCK_SIZE}, [PLAYER_GROUP_HOMES] = {RVA_PLAYER_HOME_X, 2},
    [PLAYER_GROUP_CONTROLLER] = {RVA_SLOT_CONTROLLER, 2},       [PLAYER_GROUP_CLASS] = {RVA_SLOT_CLASS, 2},
    [PLAYER_GROUP_COLOURS] = {RVA_PLAYER_COLOURS, 12},          [PLAYER_GROUP_PLANE_SEEN] = {RVA_PLAYER_PLANE_SEEN, 11},
};
#define PLAYER_HOME_Y_MOVED_BY 0x40
#endif

static unsigned char *movedPlayerTables[PLAYER_GROUP_COUNT];

// The game's address of a table.
static unsigned char *playerTableOld(int group) { return gameBase + PLAYER_TABLES[group].rva; }

// The moved table holding a game address, or -1.
static int playerTableHolding(unsigned int rva) {
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++) {
        unsigned int end = PLAYER_TABLES[g].rva + (unsigned int)(PLAYER_SLOTS_GAME * PLAYER_TABLES[g].entry);
#ifdef __APPLE__
        if (g == PLAYER_GROUP_HOMES) end = RVA_PLAYER_HOME_Y + PLAYER_SLOTS_GAME * 2;
#endif
        if (rva >= PLAYER_TABLES[g].rva && rva < end) return g;
    }
    return -1;
}

static unsigned char *playerTableAddress(unsigned int rva) {
    int g = playerTableMoved ? playerTableHolding(rva) : -1;
    if (g < 0) return gameBase + rva;
    unsigned int offset = rva - PLAYER_TABLES[g].rva;
#ifdef __APPLE__
    if (g == PLAYER_GROUP_HOMES && rva >= RVA_PLAYER_HOME_Y) offset += PLAYER_HOME_Y_MOVED_BY;
#endif
    return movedPlayerTables[g] + offset;
}

static unsigned char *playerSlotAddress(unsigned int rva0, int player, int entry) {
    return playerTableAddress(rva0) + (size_t)player * (size_t)entry;
}

static int playerSlotCount(void) { return playerTableMoved ? PLAYER_SLOTS_MOVED : PLAYER_SLOTS_GAME; }

// The new value of a slot bound: 32 becomes 48, 31 becomes 47.
static int movedPlayerBound(int game) { return game + FACTION_SLOT_TARGET; }

// The entries in each int16 table and the plane-seen cache.
#define PLAYER_SLOTS_PADDED 64
_Static_assert(PLAYER_SLOTS_PADDED % PLAYER_SLOTS_GAME == 0 && PLAYER_SLOTS_PADDED >= PLAYER_SLOTS_MOVED, "whole steps of 32");
#ifdef __APPLE__
_Static_assert(PLAYER_SLOTS_PADDED * 2 <= (RVA_PLAYER_HOME_Y - RVA_PLAYER_HOME_X) + PLAYER_HOME_Y_MOVED_BY, "home_x's padded entries end before home_y");
#endif

// Copies the game's 32 entries of each table to its new place and fills the faction slots' entries.
static void fillMovedPlayerTables(unsigned char *const tables[PLAYER_GROUP_COUNT]) {
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++)
        memcpy(tables[g], playerTableOld(g), (size_t)PLAYER_SLOTS_GAME * (size_t)PLAYER_TABLES[g].entry);
    short removed = -1;
    for (int p = PLAYER_SLOTS_GAME; p < PLAYER_SLOTS_PADDED; p++) {
        memcpy(tables[PLAYER_GROUP_CONTROLLER] + 2 * p, &removed, 2);
#ifndef __APPLE__
        memcpy(tables[PLAYER_GROUP_HOME_X] + 2 * p, &removed, 2);
        memcpy(tables[PLAYER_GROUP_HOME_Y] + 2 * p, &removed, 2);
#endif
    }
    memset(tables[PLAYER_GROUP_PLANE_SEEN] + (size_t)PLAYER_SLOTS_GAME * 11, 0xff, (size_t)(PLAYER_SLOTS_PADDED - PLAYER_SLOTS_GAME) * 11);
}

#ifndef __APPLE__
// ---- Windows: the patch ------------------------------------------------------------------------
#define PLAYER_MOVE_SITE_COUNT (PLAYER_TABLE_SITE_COUNT + PLAYER_BOUND_SITE_COUNT + PLAYER_COUNT_SITE_COUNT + PLAYER_GUARD_SITE_COUNT)
#define PLAYER_MOVE_WRITE_COUNT (PLAYER_MOVE_SITE_COUNT + PLAYER_GUARD_SITE_COUNT)

static struct { unsigned char *at; int value, size, old; } playerMoveWrites[PLAYER_MOVE_WRITE_COUNT];
static int playerMoveWriteCount;

static void queuePlayerWrite(unsigned char *at, int value, int size) {
    playerMoveWrites[playerMoveWriteCount].at = at;
    playerMoveWrites[playerMoveWriteCount].value = value;
    playerMoveWrites[playerMoveWriteCount++].size = size;
}

// Queues every site's new field, every bound's new imm8, every count's new imm32 and every guard.
static int queuePlayerTableWrites(unsigned char *const tables[PLAYER_GROUP_COUNT]) {
    int matched = 0;
    playerMoveWriteCount = 0;
    for (int i = 0; i < PLAYER_TABLE_SITE_COUNT; i++) {
        const PlayerPatchSite *s = &PLAYER_TABLE_SITES[i];
        unsigned char *at = gameBase + s->rva;
        long long delta = tables[s->group] - playerTableOld(s->group);
        if (s->end) delta += (long long)FACTION_SLOT_TARGET * PLAYER_TABLES[s->group].entry;
        int field;
        memcpy(&field, at + s->fieldOffset, 4);
        long long moved = field + delta;
        if (moved < INT32_MIN || moved > INT32_MAX) {
            writeLog("player_table_site_mismatch", "group=%d rva=0x%x out_of_reach=1", s->group, s->rva);
            continue;
        }
        queuePlayerWrite(at + s->fieldOffset, (int)moved, 4);
        matched++;
    }
    for (int i = 0; i < PLAYER_BOUND_SITE_COUNT; i++) {
        const PlayerBoundSite *s = &PLAYER_BOUND_SITES[i];
        queuePlayerWrite(gameBase + s->rva + s->immOffset, movedPlayerBound(s->game), 1);
        matched++;
    }
    for (int i = 0; i < PLAYER_COUNT_SITE_COUNT; i++) {
        const PlayerCountSite *s = &PLAYER_COUNT_SITES[i];
        queuePlayerWrite(gameBase + s->rva + s->immOffset, PLAYER_SLOTS_MOVED * s->per, 4);
        matched++;
    }
    for (int i = 0; i < PLAYER_GUARD_SITE_COUNT; i++) {
        unsigned char *at = gameBase + PLAYER_GUARD_SITES[i].rva;
        if (PLAYER_GUARD_SITES[i].kind == 1) {
            unsigned char code[4] = {0x83, (unsigned char)(0xf8 | (at[2] & 7)), PLAYER_SLOTS_GAME - 1, 0x0f};
            int first;
            memcpy(&first, code, 4);
            queuePlayerWrite(at, first, 4);
            queuePlayerWrite(at + 4, 0x87, 1);
            matched++;
            continue;
        }
        int reg = at[1] & 7;
        unsigned char code[8] = {0x83, (unsigned char)(0xf8 | reg), PLAYER_SLOTS_GAME - 1, 0x77, 0x09, 0x8b, (unsigned char)(0xc0 | reg), 0};
        int first, rest;
        memcpy(&first, code, 4);
        memcpy(&rest, code + 4, 4);
        queuePlayerWrite(at, first, 4);
        queuePlayerWrite(at + 4, rest, 3);
        matched++;
    }
    return matched;
}

// Each table's place in the one block the move allocates (page-aligned for the blocks, 16 for the rest).
#define PLAYER_BLOCKS_BYTES    ((size_t)PLAYER_SLOTS_MOVED * PLAYER_BLOCK_SIZE)
#define PLAYER_SMALL_AT        ((PLAYER_BLOCKS_BYTES + 0xfff) & ~(size_t)0xfff)
static const unsigned short PLAYER_SMALL_OFFSET[PLAYER_GROUP_COUNT] = {
    [PLAYER_GROUP_CONTROLLER] = 0, [PLAYER_GROUP_CLASS] = 0x80, [PLAYER_GROUP_HOME_X] = 0x100, [PLAYER_GROUP_HOME_Y] = 0x180,
    [PLAYER_GROUP_COLOURS] = 0x200, [PLAYER_GROUP_PLANE_SEEN] = 0x480,
};
_Static_assert(PLAYER_SLOTS_PADDED * 2 <= 0x80 && PLAYER_SLOTS_MOVED * 12 <= 0x280 && PLAYER_SLOTS_PADDED * 11 <= 0x1000 - 0x480, "the tables don't overlap");

// The main save's read of a player's cast counts, relative to its loop's pointer into the player blocks.
static int saveCastReadAdjusted;
static void adjustSaveCastCountRead(void) {
    if (saveCastReadAdjusted || !playerTableMoved || !ritualExpansionActive) return;
    unsigned char *at = gameBase + RVA_SAVE_GAME_CAST_COUNT_READ + 3;
    int disp;
    memcpy(&disp, at, 4);
    long long moved = (long long)disp - (playerTableAddress(RVA_SLOT_NAME) - (gameBase + RVA_SLOT_NAME));
    int value = (int)moved;
    saveCastReadAdjusted = moved == value && writeGameCode(at, &value, 4);
    writeLog("save_cast_read_adjusted", "ok=%d", saveCastReadAdjusted);
}

// Moves the tables, or changes nothing.
static int installPlayerTableMove(void) {
    if (!gameCodeMatches) {
        writeLog("player_table_not_moved", "reason=game_code_differs sites=0 of %d", PLAYER_MOVE_SITE_COUNT);
        return 0;
    }
    size_t sink = ((size_t)PLAYER_BLOCK_SIZE + 0xfff) & ~(size_t)0xfff, size = sink + PLAYER_SMALL_AT + 0x1000;
    unsigned char *block = allocNearImage(size);
    if (!block) {
        writeLog("player_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", size, PLAYER_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *tables[PLAYER_GROUP_COUNT];
    tables[PLAYER_GROUP_BLOCKS] = block + sink;
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++)
        if (g != PLAYER_GROUP_BLOCKS) tables[g] = block + sink + PLAYER_SMALL_AT + PLAYER_SMALL_OFFSET[g];
    int matched = queuePlayerTableWrites(tables);
    const char *reason = matched != PLAYER_MOVE_SITE_COUNT ? "game_build_differs"
                         : !openGameCode(gameBase + RVA_TEXT_SECTION, (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION)) ? "code_unwritable" : NULL;
    if (reason) {
        writeLog("player_table_not_moved", "reason=%s sites=%d of %d", reason, matched, PLAYER_MOVE_SITE_COUNT);
        VirtualFree(block, 0, MEM_RELEASE);
        return 0;
    }
    fillMovedPlayerTables(tables);
    for (int i = 0; i < playerMoveWriteCount; i++) {
        size_t n = (size_t)playerMoveWrites[i].size;
        memcpy(&playerMoveWrites[i].old, playerMoveWrites[i].at, n);
        if (writeGameCode(playerMoveWrites[i].at, &playerMoveWrites[i].value, n)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(playerMoveWrites[j].at, &playerMoveWrites[j].old, (size_t)playerMoveWrites[j].size);
        closeGameCode();
        writeLog("player_table_not_moved", "reason=code_unwritable sites=%d of %d", matched, PLAYER_MOVE_SITE_COUNT);
        VirtualFree(block, 0, MEM_RELEASE);
        return 0;
    }
    closeGameCode();
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++) movedPlayerTables[g] = tables[g];
    playerTableMoved = 1;
    writeLog("player_table_moved", "from=%p to=%p slots=%d sites=%d of %d", (void *)playerTableOld(PLAYER_GROUP_BLOCKS), (void *)block,
             PLAYER_SLOTS_MOVED, matched, PLAYER_MOVE_SITE_COUNT);
    adjustSaveCastCountRead();
    return 1;
}
#else
// ---- Mac: the patch ----------------------------------------------------------------------------
// Mac's built-in sizes of the 32 slots, each with its 48-slot word.
#define PLAYER_IMM12 (0xfffu << 10)
#define PLAYER_IMM16 (0xffffu << 5)
typedef struct { unsigned int rva; uint32_t mask, value; } PlayerMacWord;
#define PLAYER_SIZE_LOW  ((PLAYER_SLOTS_MOVED * PLAYER_BLOCK_SIZE) & 0xffffu)
#define PLAYER_SIZE_HIGH ((PLAYER_SLOTS_MOVED * PLAYER_BLOCK_SIZE) >> 16)
static const PlayerMacWord PLAYER_MAC_WORDS[] = {
    {MAC(0x100585e14), PLAYER_IMM16, PLAYER_SIZE_LOW << 5}, {MAC(0x100585e18), PLAYER_IMM16, PLAYER_SIZE_HIGH << 5},
    {MAC(0x1003b56d0), PLAYER_IMM16, PLAYER_SIZE_LOW << 5}, {MAC(0x1003b56d4), PLAYER_IMM16, PLAYER_SIZE_HIGH << 5},
    {MAC(0x100586054), PLAYER_IMM12, (PLAYER_SLOTS_MOVED * 12u) << 10},
};
#define PLAYER_MAC_WORD_COUNT (int)(sizeof PLAYER_MAC_WORDS / sizeof PLAYER_MAC_WORDS[0])
#define PLAYER_MOVE_SITE_COUNT (PLAYER_MAC_SITE_COUNT + PLAYER_MAC_REBASE_COUNT + PLAYER_MAC_BOUND_COUNT + PLAYER_MAC_WORD_COUNT)

static struct { unsigned char *at; uint32_t word, old; } playerMoveWrites[PLAYER_MOVE_SITE_COUNT];
static int playerMoveWriteCount;

static uint32_t playerGameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

static void queuePlayerWord(unsigned int rva, uint32_t word) {
    playerMoveWrites[playerMoveWriteCount].at = gameBase + rva;
    playerMoveWrites[playerMoveWriteCount].word = word;
    memcpy(&playerMoveWrites[playerMoveWriteCount++].old, gameBase + rva, 4);
}

// Queues every adrp's new page, every home_y use's new offset and every bound's new immediate.
static int queuePlayerTableWrites(unsigned char *const tables[PLAYER_GROUP_COUNT]) {
    int matched = 0;
    playerMoveWriteCount = 0;
    for (int i = 0; i < PLAYER_MAC_SITE_COUNT; i++) {
        const PlayerMacSite *s = &PLAYER_MAC_SITES[i];
        long long pages = ((long long)(uintptr_t)tables[s->group] >> ADRP_PAGE_SHIFT) -
                          ((long long)(uintptr_t)playerTableOld(s->group) >> ADRP_PAGE_SHIFT);
        uint32_t moved = 0;
        if (!adrpMovedBy(playerGameWord(s->rva), pages, &moved)) {
            writeLog("player_table_site_mismatch", "group=%d va=0x%llx", s->group, s->rva + 0x100000000ULL);
            continue;
        }
        queuePlayerWord(s->rva, moved);
        matched++;
    }
    for (int i = 0; i < PLAYER_MAC_REBASE_COUNT; i++) {
        uint32_t word = playerGameWord(PLAYER_MAC_REBASES[i].rva);
        uint32_t imm = (word >> 10) & 0xfffu;
        if (imm + PLAYER_MAC_REBASES[i].step > 0xfffu) {
            writeLog("player_table_site_mismatch", "group=home_y va=0x%llx out_of_reach=1", PLAYER_MAC_REBASES[i].rva + 0x100000000ULL);
            continue;
        }
        queuePlayerWord(PLAYER_MAC_REBASES[i].rva, patchedWord(word, PLAYER_IMM12, (imm + PLAYER_MAC_REBASES[i].step) << 10));
        matched++;
    }
    for (int i = 0; i < PLAYER_MAC_BOUND_COUNT; i++) {
        const PlayerMacBound *s = &PLAYER_MAC_BOUNDS[i];
        uint32_t word = playerGameWord(s->rva);
        int movz = (word & 0x7f800000u) == 0x52800000u;
        queuePlayerWord(s->rva, patchedWord(word, movz ? PLAYER_IMM16 : PLAYER_IMM12, (uint32_t)movedPlayerBound(s->game) << (movz ? 5 : 10)));
        matched++;
    }
    for (int i = 0; i < PLAYER_MAC_WORD_COUNT; i++) {
        const PlayerMacWord *s = &PLAYER_MAC_WORDS[i];
        queuePlayerWord(s->rva, patchedWord(playerGameWord(s->rva), s->mask, s->value));
        matched++;
    }
    return matched;
}

// Writes the queued words, one protection change per run of words on the same page.
static int writePlayerWords(void) {
    int i = 0;
    while (i < playerMoveWriteCount) {
        uintptr_t page = (uintptr_t)playerMoveWrites[i].at & ~(uintptr_t)MAC_PAGE_MASK;
        int end = i;
        while (end < playerMoveWriteCount && ((uintptr_t)playerMoveWrites[end].at & ~(uintptr_t)MAC_PAGE_MASK) == page) end++;
        if (mach_vm_protect(mach_task_self(), page, MAC_PAGE_MASK + 1, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY)) return i;
        for (int k = i; k < end; k++) memcpy(playerMoveWrites[k].at, &playerMoveWrites[k].word, 4);
        if (mach_vm_protect(mach_task_self(), page, MAC_PAGE_MASK + 1, FALSE, VM_PROT_READ | VM_PROT_EXECUTE)) return i;
        sys_icache_invalidate((void *)page, MAC_PAGE_MASK + 1);
        i = end;
    }
    return i;
}

// Moves the tables, or changes nothing.
static int installPlayerTableMove(void) {
    if (!gameCodeMatches) {
        writeLog("player_table_not_moved", "reason=game_code_differs sites=0 of %d", PLAYER_MOVE_SITE_COUNT);
        return 0;
    }
    size_t at[PLAYER_GROUP_COUNT], total = ((size_t)PLAYER_BLOCK_SIZE + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++) {
        size_t bytes = ((uintptr_t)playerTableOld(g) & ADRP_PAGE_MASK) + (size_t)PLAYER_SLOTS_PADDED * (size_t)PLAYER_TABLES[g].entry;
        if (g == PLAYER_GROUP_HOMES) bytes += (RVA_PLAYER_HOME_Y - RVA_PLAYER_HOME_X) + PLAYER_HOME_Y_MOVED_BY;
        at[g] = total;
        total += (bytes + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    }
    unsigned char *memory = allocAfterImage(total);
    if (!memory) {
        writeLog("player_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", total, PLAYER_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *tables[PLAYER_GROUP_COUNT];
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++) tables[g] = memory + at[g] + ((uintptr_t)playerTableOld(g) & ADRP_PAGE_MASK);
    int matched = queuePlayerTableWrites(tables);
    if (matched != PLAYER_MOVE_SITE_COUNT) {
        writeLog("player_table_not_moved", "reason=game_build_differs sites=%d of %d", matched, PLAYER_MOVE_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    fillMovedPlayerTables(tables);
    unsigned char *homeY = tables[PLAYER_GROUP_HOMES] + (RVA_PLAYER_HOME_Y - RVA_PLAYER_HOME_X) + PLAYER_HOME_Y_MOVED_BY;
    memcpy(homeY, gameBase + RVA_PLAYER_HOME_Y, (size_t)PLAYER_SLOTS_GAME * 2);
    memset(tables[PLAYER_GROUP_HOMES] + 2 * PLAYER_SLOTS_GAME, 0xff, 2 * (PLAYER_SLOTS_PADDED - PLAYER_SLOTS_GAME));
    memset(homeY + 2 * PLAYER_SLOTS_GAME, 0xff, 2 * (PLAYER_SLOTS_PADDED - PLAYER_SLOTS_GAME));
    int written = writePlayerWords();
    if (written != playerMoveWriteCount) {
        for (int j = 0; j < playerMoveWriteCount; j++) writeGameCode(playerMoveWrites[j].at, &playerMoveWrites[j].old, 4);
        writeLog("player_table_not_moved", "reason=code_unwritable va=0x%llx sites=%d of %d",
                 (unsigned long long)(playerMoveWrites[written].at - gameBase) + 0x100000000ULL, matched, PLAYER_MOVE_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    for (int g = 0; g < PLAYER_GROUP_COUNT; g++) movedPlayerTables[g] = tables[g];
    playerTableMoved = 1;
    writeLog("player_table_moved", "from=%p to=%p slots=%d sites=%d of %d", (void *)playerTableOld(PLAYER_GROUP_BLOCKS),
             (void *)tables[PLAYER_GROUP_BLOCKS], PLAYER_SLOTS_MOVED, matched, PLAYER_MOVE_SITE_COUNT);
    adjustSaveCastCountRead();
    return 1;
}

// The Ritual Expansion's three cast count stubs, relative to their loops' pointers into the player blocks.
static int saveCastReadAdjusted;
static void adjustSaveCastCountRead(void) {
    if (saveCastReadAdjusted || !playerTableMoved || !ritualExpansionActive) return;
    long long moved = (long long)(playerTableAddress(RVA_SLOT_NAME) - (gameBase + RVA_SLOT_NAME));
    int ok = 1;
    for (int i = 0; i < CAST_DELTA_CODES; i++) {
        long long delta;
        memcpy(&delta, castDeltaCode[i] + 16, 8);
        delta -= moved;
        ok &= writeGameCode(castDeltaCode[i] + 16, &delta, 8);
    }
    saveCastReadAdjusted = ok;
    writeLog("save_cast_read_adjusted", "ok=%d", ok);
}
#endif

// Moves the tables once per launch, at the first faction a mod declares.
static int ensurePlayerTableMoved(void) {
    static int tried;
    if (playerTableMoved || tried) return playerTableMoved;
    tried = 1;
    return installPlayerTableMove();
}
