// Spell paths 100 to 127, always on.
#include "spell_path_takeover.h"

#define SPELL_PATH_COUNT       128
#define GAME_SPELL_PATH_COUNT  100
#define EXPANDED_PATH_COUNT    (SPELL_PATH_COUNT - GAME_SPELL_PATH_COUNT)

static int spellPathsExpanded;
static const char **spellPathCopy;

// The plugin that took the paths over, once it has.
static struct {
    const char **names;
    int count;
    char mod[MOD_NAME_MAX];
    char dll[64];
    HMODULE module;
    int modIndex;
    int lastOn;
} pathTakeover = {.modIndex = -1, .lastOn = -1};

// Whether the plugin's mod is on in the game now.
static int pathTakeoverModOn(void) {
    int last = G32(RVA_MOD_LAST_INDEX);
    int m = pathTakeover.modIndex;
    if (m < 0 || m > last || _stricmp(modName(m), pathTakeover.mod)) {
        for (m = 0; m <= last && _stricmp(modName(m), pathTakeover.mod); m++) {}
        pathTakeover.modIndex = m <= last ? m : -1;
    }
    int on = pathTakeover.modIndex >= 0 && modEnabled(pathTakeover.modIndex);
    if (on != pathTakeover.lastOn) {
        writeLog("spell_path_count", "paths=%d mod=%s mod_on=%d", on ? pathTakeover.count : SPELL_PATH_COUNT, pathTakeover.mod, on);
        pathTakeover.lastOn = on;
    }
    return on;
}

// The entries of the name table the game reads.
static int spellPathTableSize(void) {
    return pathTakeover.names ? pathTakeover.count : spellPathsExpanded ? SPELL_PATH_COUNT : GAME_SPELL_PATH_COUNT;
}

// The paths mods may use now.
static int spellPathCount(void) {
    if (pathTakeover.names && !pathTakeoverModOn() && pathTakeover.count > SPELL_PATH_COUNT) return SPELL_PATH_COUNT;
    return spellPathTableSize();
}

// Fills `copy` with the game's names and makes it the table CoE5SE names paths in.
static void startSpellPathCopy(const char **copy) {
    memcpy(copy, gameBase + RVA_SPELL_PATH_NAMES, GAME_SPELL_PATH_COUNT * sizeof *copy);
    for (int p = GAME_SPELL_PATH_COUNT; p < SPELL_PATH_COUNT; p++) copy[p] = NULL;
    spellPathCopy = copy;
}

// ---- Transforms ----------------------------------------------------------------------------------
// The game's functions the transform hooks call.
static int (*unitSpellWeaponLevel)(int unit, int path);
static void (*learnPromotionSpells)(int unit, int path, int level);
static void (*chooseMemorized)(int unit);

// A unit's levels in paths 100 to 127 before its transform.
static struct {
    int unit;
    int levels[EXPANDED_PATH_COUNT];
} transformLevels = {.unit = -1};

// The transform's level snapshot of paths 0 to 99.
static int transformSnapshotHook(int unit, int path) {
    if (path == 0) {
        transformLevels.unit = unit;
        for (int i = 0; i < EXPANDED_PATH_COUNT; i++) transformLevels.levels[i] = unitSpellWeaponLevel(unit, GAME_SPELL_PATH_COUNT + i);
    }
    return unitSpellWeaponLevel(unit, path);
}

// The transform calls choose_memorized(unit) after teaching the spells gained in paths 0 to 99.
static void transformMemorizeHook(int unit) {
    if (transformLevels.unit == unit) {
        for (int i = 0; i < EXPANDED_PATH_COUNT; i++) {
            int path = GAME_SPELL_PATH_COUNT + i, before = transformLevels.levels[i], now = unitSpellWeaponLevel(unit, path);
            for (int level = before + 1; level <= now; level++) learnPromotionSpells(unit, path, level);
            if (now > before) writeLog("spell_path_promotions", "unit=%d path=%d from=%d to=%d", unit, path, before, now);
        }
    }
    transformLevels.unit = -1;
    chooseMemorized(unit);
}

#ifndef __APPLE__
// ---- Windows: the patch --------------------------------------------------------------------------
// A compare whose imm8 changes, and for a loop the conditional jump after it.
typedef struct {
    unsigned int rva;
    unsigned char immAt, imm, jumpAt, jump;
    const char *name;
} PathBytePatch;

static const PathBytePatch PATH_BYTE_PATCHES[] = {
    {RVA_PARSER_SPELLWEAPON50S_PATH,    2, 0x7e, 0, 0, "parser_spellweapon50s"},
    {RVA_PARSER_SPELLWEAPON_PATH,       2, 0x7e, 0, 0, "parser_spellweapon"},
    {RVA_PARSER_SPELLWEAPONBONUS_PATH,  2, 0x7e, 0, 0, "parser_spellweaponbonus"},
    {RVA_PARSER_SPELLWEAPON50_PATH,     2, 0x7e, 0, 0, "parser_spellweapon50"},
    {RVA_PARSER_SPELLWEAPONSINGLE_PATH, 2, 0x7e, 0, 0, "parser_spellweaponsingle"},
    {RVA_ITEM_PATH_BOOST_BOUND,         2, 0x7e, 0, 0, "item_path_boost"},
    {RVA_STATS_SPELL_ATTACK_PATH_BOUND, 2, 0x7f, 0, 0, "stats_path_level"},
    {RVA_STATS_KNOWN_SPELLS_PATH_LOOP,  3, 0x7f, 5, 0x8e, "stats_known_spells"},
    {RVA_LIBRARY_PATH_LOOP,             2, 0x7f, 4, 0x8e, "library"},
    {RVA_LEARN_START_SPELLS_PATH_LOOP,  3, 0x7f, 5, 0x8e, "learn_start_spells"},
    {RVA_CHOOSE_MEMORIZED_PATH_LOOP,    2, 0x7f, 4, 0x8f, "choose_memorized"},
    {RVA_UNIT_DEBUG_PATH_BOUND,         3, 0x7f, 4, 0x7f, "debug_unit_screen"},
};
#define PATH_BYTE_PATCH_COUNT ((int)(sizeof PATH_BYTE_PATCHES / sizeof PATH_BYTE_PATCHES[0]))
#define PATH_BUG_CHECK_SKIP_SIZE 6

// A reader of the name table.
typedef struct {
    unsigned int rva;
    int fieldOffset;
    const char *name;
} PathReaderSite;

static const PathReaderSite PATH_READER_SITES[] = {
    {RVA_PATH_NAMES_READ_STATS_LEVEL, 4, "stats_path_level"},
    {RVA_PATH_NAMES_READ_STATS_KNOWN, 4, "stats_known_spells"},
    {RVA_PATH_NAMES_READ_LIBRARY,     4, "library"},
    {RVA_PATH_NAMES_READ_LIBRARY_2,   4, "library_2"},
    {RVA_PATH_NAMES_READ_ITEM_BOOST,  4, "item_path_boost"},
    {RVA_PATH_NAMES_READ_DEBUG_UNIT,  4, "debug_unit_screen"},
};
#define PATH_READER_COUNT ((int)(sizeof PATH_READER_SITES / sizeof PATH_READER_SITES[0]))

// A redirected call: the call instruction, the function it calls, and CoE5SE's hook.
typedef struct { unsigned int callRva, targetRva; int snapshot; const char *name; } PathTransformCall;

static const PathTransformCall PATH_TRANSFORM_CALLS[] = {
    {RVA_MASTERY_SNAPSHOT_CALL,          RVA_UNIT_SPELL_WEAPON_LEVEL, 1, "mastery_snapshot"},
    {RVA_MASTERY_CHOOSE_MEMORIZED_CALL,  RVA_CHOOSE_MEMORIZED,        0, "mastery_memorize"},
    {RVA_RITUAL_TRANSFORM_SNAPSHOT_CALL, RVA_UNIT_SPELL_WEAPON_LEVEL, 1, "ritual_transform_snapshot"},
    {RVA_RITUAL_TRANSFORM_MEMORIZE_CALL, RVA_CHOOSE_MEMORIZED,        0, "ritual_transform_memorize"},
};
#define PATH_TRANSFORM_CALL_COUNT ((int)(sizeof PATH_TRANSFORM_CALLS / sizeof PATH_TRANSFORM_CALLS[0]))
#define PATH_SITE_COUNT (PATH_BYTE_PATCH_COUNT + 1 + PATH_READER_COUNT + PATH_TRANSFORM_CALL_COUNT)

// The changes, queued while checking and written only once every site matched.
static struct { unsigned char *at; int size; unsigned char bytes[8], old[8]; } pathWrites[PATH_SITE_COUNT];
static int pathWriteCount;
static unsigned char *pathStubs;

static void queuePathWrite(unsigned char *at, const void *bytes, int size) {
    pathWrites[pathWriteCount].at = at;
    pathWrites[pathWriteCount].size = size;
    memcpy(pathWrites[pathWriteCount].old, at, (size_t)size);
    memcpy(pathWrites[pathWriteCount++].bytes, bytes, (size_t)size);
}

// Queues every site's change, for the copy at `copy` and the hooks' two stubs at `stubs`.
static int queueSpellPathWrites(const char **copy, unsigned char *stubs) {
    int matched = 0;
    pathWriteCount = 0;
    for (int i = 0; i < PATH_BYTE_PATCH_COUNT; i++) {
        const PathBytePatch *s = &PATH_BYTE_PATCHES[i];
        unsigned char *at = gameBase + s->rva, bytes[8];
        int size = (s->jumpAt ? s->jumpAt : s->immAt) + 1;
        memcpy(bytes, at, (size_t)size);
        bytes[s->immAt] = s->imm;
        if (s->jumpAt) bytes[s->jumpAt] = s->jump;
        queuePathWrite(at, bytes, size);
        matched++;
    }
    unsigned char *skip = gameBase + RVA_SPELL_BUG_CHECK_SPELL_LOOP_SKIP, jump[PATH_BUG_CHECK_SKIP_SIZE] = {0xe9};
    int rel;
    memcpy(&rel, skip + 2, 4);
    rel += 1;
    memcpy(jump + 1, &rel, 4);
    jump[5] = 0x90;
    queuePathWrite(skip, jump, PATH_BUG_CHECK_SKIP_SIZE);
    matched++;
    long long delta = (const unsigned char *)copy - (gameBase + RVA_SPELL_PATH_NAMES);
    for (int i = 0; i < PATH_READER_COUNT; i++) {
        const PathReaderSite *s = &PATH_READER_SITES[i];
        unsigned char *at = gameBase + s->rva;
        int field;
        memcpy(&field, at + s->fieldOffset, 4);
        long long moved = field + delta;
        if (moved < INT32_MIN || moved > INT32_MAX) {
            writeLog("spell_paths_site_mismatch", "group=reader name=%s rva=0x%x out_of_reach=1", s->name, s->rva);
            continue;
        }
        int out = (int)moved;
        queuePathWrite(at + s->fieldOffset, &out, 4);
        matched++;
    }
    void *hooks[2] = {(void *)transformMemorizeHook, (void *)transformSnapshotHook};
    for (int h = 0; h < 2; h++) {
        unsigned char *stub = stubs + 16 * h;
        stub[0] = 0xff; stub[1] = 0x25; memset(stub + 2, 0, 4);
        memcpy(stub + 6, &hooks[h], 8);
    }
    for (int i = 0; i < PATH_TRANSFORM_CALL_COUNT; i++) {
        const PathTransformCall *c = &PATH_TRANSFORM_CALLS[i];
        unsigned char *call = gameBase + c->callRva, *stub = stubs + 16 * c->snapshot;
        int rel;
        memcpy(&rel, call + 1, 4);
        long long toStub = stub - (call + 5);
        if (call[0] != 0xe8 || call + 5 + rel != gameBase + c->targetRva || toStub < INT32_MIN || toStub > INT32_MAX) {
            writeLog("spell_paths_site_mismatch", "group=call name=%s rva=0x%x", c->name, c->callRva);
            continue;
        }
        int out = (int)toStub;
        queuePathWrite(call + 1, &out, 4);
        matched++;
    }
    return matched;
}

// Raises the game's paths to 128, or changes nothing.
static int installSpellPathExpansion(void) {
    if (!gameCodeMatches) {
        writeLog("spell_paths_not_expanded", "reason=game_code_differs sites=0 of %d", PATH_SITE_COUNT);
        return 0;
    }
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    const char **copy = (const char **)allocNearImage(SPELL_PATH_COUNT * sizeof(const char *));
    if (!copy || !stubs) {
        writeLog("spell_paths_not_expanded", "reason=no_memory_near_game names=%p stubs=%p sites=0 of %d", (void *)copy, (void *)stubs,
                 PATH_SITE_COUNT);
        if (copy) VirtualFree(copy, 0, MEM_RELEASE);
        if (stubs) VirtualFree(stubs, 0, MEM_RELEASE);
        return 0;
    }
    int matched = queueSpellPathWrites(copy, stubs);
    const char *reason = matched != PATH_SITE_COUNT ? "game_build_differs"
                         : !openGameCode(gameBase + RVA_TEXT_SECTION, (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION)) ? "code_unwritable" : NULL;
    if (reason) {
        writeLog("spell_paths_not_expanded", "reason=%s sites=%d of %d error=%lu", reason, matched, PATH_SITE_COUNT,
                 matched == PATH_SITE_COUNT ? GetLastError() : 0UL);
        VirtualFree(copy, 0, MEM_RELEASE);
        VirtualFree(stubs, 0, MEM_RELEASE);
        return 0;
    }
    makePagesReadExecute(stubs, 0x1000);
    unitSpellWeaponLevel = GAME_FN(int (*)(int, int), RVA_UNIT_SPELL_WEAPON_LEVEL);
    learnPromotionSpells = GAME_FN(void (*)(int, int, int), RVA_LEARN_PROMOTION_SPELLS);
    chooseMemorized = GAME_FN(void (*)(int), RVA_CHOOSE_MEMORIZED);
    startSpellPathCopy(copy);
    for (int i = 0; i < pathWriteCount; i++) {
        if (writeGameCode(pathWrites[i].at, pathWrites[i].bytes, (size_t)pathWrites[i].size)) continue;
        DWORD error = GetLastError();
        for (int j = i; j >= 0; j--) writeGameCode(pathWrites[j].at, pathWrites[j].old, (size_t)pathWrites[j].size);
        closeGameCode();
        writeLog("spell_paths_not_expanded", "reason=code_unwritable sites=%d of %d error=%lu", matched, PATH_SITE_COUNT, error);
        spellPathCopy = NULL;
        VirtualFree(copy, 0, MEM_RELEASE);
        VirtualFree(stubs, 0, MEM_RELEASE);
        return 0;
    }
    closeGameCode();
    pathStubs = stubs;
    spellPathsExpanded = 1;
    writeLog("spell_paths_expanded", "paths=%d sites=%d of %d names=%p", SPELL_PATH_COUNT, matched, PATH_SITE_COUNT, (void *)copy);
    return 1;
}

// Puts back every instruction installSpellPathExpansion changed, or none.
static int uninstallSpellPathExpansion(void) {
    for (int i = 0; i < pathWriteCount; i++)
        if (memcmp(pathWrites[i].at, pathWrites[i].bytes, (size_t)pathWrites[i].size)) {
            writeLog("spell_paths_not_uninstalled", "reason=site_changed rva=0x%x", (unsigned)(pathWrites[i].at - gameBase));
            return 0;
        }
    if (!openGameCode(gameBase + RVA_TEXT_SECTION, (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION))) {
        writeLog("spell_paths_not_uninstalled", "reason=code_unwritable error=%lu", GetLastError());
        return 0;
    }
    for (int i = 0; i < pathWriteCount; i++) {
        if (writeGameCode(pathWrites[i].at, pathWrites[i].old, (size_t)pathWrites[i].size)) continue;
        DWORD error = GetLastError();
        for (int j = i; j >= 0; j--) writeGameCode(pathWrites[j].at, pathWrites[j].bytes, (size_t)pathWrites[j].size);
        closeGameCode();
        writeLog("spell_paths_not_uninstalled", "reason=code_unwritable error=%lu", error);
        return 0;
    }
    closeGameCode();
    spellPathsExpanded = 0;
    VirtualFree((void *)spellPathCopy, 0, MEM_RELEASE);
    VirtualFree(pathStubs, 0, MEM_RELEASE);
    spellPathCopy = NULL;
    pathStubs = NULL;
    writeLog("spell_paths_uninstalled", "sites=%d", pathWriteCount);
    pathWriteCount = 0;
    return 1;
}
#else
// ---- Mac: the patch ------------------------------------------------------------------------------
// A compare (cmp, 32- or 64-bit) whose 12-bit immediate becomes `value`.
typedef struct { unsigned int rva, value; const char *name; } PathCompareSite;

static const PathCompareSite PATH_COMPARE_SITES[] = {
    {RVA_PARSER_SPELLWEAPON50S_PATH,    0x7e, "parser_spellweapon50s"},
    {RVA_PARSER_SPELLWEAPON_PATH,       0x7e, "parser_spellweapon"},
    {RVA_PARSER_SPELLWEAPONBONUS_PATH,  0x7e, "parser_spellweaponbonus"},
    {RVA_PARSER_SPELLWEAPON50_PATH,     0x7e, "parser_spellweapon50"},
    {RVA_PARSER_SPELLWEAPONSINGLE_PATH, 0x7e, "parser_spellweaponsingle"},
    {RVA_ITEM_PATH_BOOST_BOUND,         0x7e, "item_path_boost"},
    {RVA_ITEM_PATH_BOOST_BOUND_2,       0x7e, "item_path_boost_2"},
    {RVA_ITEM_PATH_BOOST_BOUND_3,       0x7e, "item_path_boost_3"},
    {RVA_ITEM_PATH_BOOST_BOUND_4,       0x7e, "item_path_boost_4"},
    {RVA_STATS_SPELL_ATTACK_PATH_BOUND, 0x7f, "stats_path_level"},
    {RVA_UNIT_DEBUG_PATH_BOUND,         0x7f, "debug_unit_screen"},
    {RVA_STATS_KNOWN_SPELLS_PATH_LOOP,  0x80, "stats_known_spells"},
    {RVA_LIBRARY_PATH_LOOP,             0x80, "library"},
    {RVA_LEARN_START_SPELLS_PATH_LOOP,  0x80, "learn_start_spells"},
    {RVA_CHOOSE_MEMORIZED_PATH_LOOP,    0x80, "choose_memorized"},
};
#define PATH_COMPARE_COUNT ((int)(sizeof PATH_COMPARE_SITES / sizeof PATH_COMPARE_SITES[0]))

// A reader of the name table: an ADRP of its page, then the ADD of its low 12 bits.
typedef struct { unsigned int rva; const char *name; } PathReaderPair;

static const PathReaderPair PATH_READER_PAIRS[] = {
    {RVA_PATH_NAMES_READ_LIBRARY,      "library"},
    {RVA_PATH_NAMES_READ_STATS_LEVEL,  "stats_path_level"},
    {RVA_PATH_NAMES_READ_STATS_KNOWN,  "stats_known_spells"},
    {RVA_PATH_NAMES_READ_ITEM_BOOST,   "item_path_boost"},
    {RVA_PATH_NAMES_READ_ITEM_BOOST_2, "item_path_boost_2"},
    {RVA_PATH_NAMES_READ_ITEM_BOOST_3, "item_path_boost_3"},
    {RVA_PATH_NAMES_READ_ITEM_BOOST_4, "item_path_boost_4"},
    {RVA_PATH_NAMES_READ_DEBUG_UNIT,   "debug_unit_screen"},
};
#define PATH_READER_COUNT ((int)(sizeof PATH_READER_PAIRS / sizeof PATH_READER_PAIRS[0]))

// A redirected call.
typedef struct { unsigned int callRva, targetRva; int snapshot; const char *name; } PathTransformCall;

static const PathTransformCall PATH_TRANSFORM_CALLS[] = {
    {RVA_MASTERY_SNAPSHOT_CALL,          RVA_UNIT_SPELL_WEAPON_LEVEL, 1, "mastery_snapshot"},
    {RVA_MASTERY_CHOOSE_MEMORIZED_CALL,  RVA_CHOOSE_MEMORIZED,        0, "mastery_memorize"},
    {RVA_RITUAL_TRANSFORM_SNAPSHOT_CALL, RVA_UNIT_SPELL_WEAPON_LEVEL, 1, "ritual_transform_snapshot"},
    {RVA_RITUAL_TRANSFORM_MEMORIZE_CALL, RVA_CHOOSE_MEMORIZED,        0, "ritual_transform_memorize"},
};
#define PATH_TRANSFORM_CALL_COUNT ((int)(sizeof PATH_TRANSFORM_CALLS / sizeof PATH_TRANSFORM_CALLS[0]))
#define PATH_SITE_COUNT (PATH_COMPARE_COUNT + 1 + PATH_READER_COUNT + PATH_TRANSFORM_CALL_COUNT)

// A cmp's word with its 12-bit immediate set to `value` (below 0x1000).
static uint32_t compareWithImmediate(uint32_t word, unsigned int value) { return (word & ~(0xfffu << 10)) | (value & 0xfffu) << 10; }

// An ADRP's word moved by `pages` 4 KB pages, into *out.
static int adrpMovedBy(uint32_t word, long long pages, uint32_t *out) {
    long long imm = (long long)((word >> 5) & 0x7ffff) << 2 | (word >> 29 & 3);
    if (imm & 0x100000) imm -= 0x200000;
    imm += pages;
    if (imm < -0x100000 || imm > 0xfffff) return 0;
    *out = (word & 0x9f00001fu) | (uint32_t)(imm & 3) << 29 | (uint32_t)((imm >> 2) & 0x7ffff) << 5;
    return 1;
}

// The changes, queued while checking and written only once every site matched.
static struct { unsigned char *at; uint32_t word, old; } pathWrites[PATH_SITE_COUNT];
static int pathWriteCount;
static unsigned char *pathMemory;
static size_t pathMemorySize;

static void queuePathWord(unsigned int rva, uint32_t word) {
    pathWrites[pathWriteCount].at = gameBase + rva;
    pathWrites[pathWriteCount].word = word;
    memcpy(&pathWrites[pathWriteCount++].old, gameBase + rva, 4);
}

static uint32_t pathGameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

// Queues every site's change, for the copy at `copy`.
static int queueSpellPathWrites(const char **copy) {
    int matched = 0;
    pathWriteCount = 0;
    for (int i = 0; i < PATH_COMPARE_COUNT; i++) {
        const PathCompareSite *s = &PATH_COMPARE_SITES[i];
        queuePathWord(s->rva, compareWithImmediate(pathGameWord(s->rva), s->value));
        matched++;
    }
    queuePathWord(RVA_SPELL_BUG_CHECK_SPELL_LOOP_SKIP,
                  0x14000000u | ((uint32_t)((RVA_SPELL_BUG_CHECK_SPELL_LOOP_END - RVA_SPELL_BUG_CHECK_SPELL_LOOP_SKIP) >> 2) & 0x3ffffffu));
    matched++;
    long long pages = ((long long)(uintptr_t)copy >> ADRP_PAGE_SHIFT) - ((long long)(uintptr_t)(gameBase + RVA_SPELL_PATH_NAMES) >> ADRP_PAGE_SHIFT);
    for (int i = 0; i < PATH_READER_COUNT; i++) {
        const PathReaderPair *s = &PATH_READER_PAIRS[i];
        uint32_t moved = 0;
        if (!adrpMovedBy(pathGameWord(s->rva), pages, &moved)) {
            writeLog("spell_paths_site_mismatch", "group=reader name=%s va=0x%llx", s->name, s->rva + 0x100000000ULL);
            continue;
        }
        queuePathWord(s->rva, moved);
        matched++;
    }
    unsigned char *trampolines[2] = {NULL, NULL};
    for (int i = 0; i < PATH_TRANSFORM_CALL_COUNT; i++) {
        const PathTransformCall *c = &PATH_TRANSFORM_CALLS[i];
        if (blTarget(gameBase + c->callRva) != gameBase + c->targetRva) {
            writeLog("spell_paths_site_mismatch", "group=call name=%s va=0x%llx", c->name, c->callRva + 0x100000000ULL);
            continue;
        }
        if (!trampolines[c->snapshot])
            trampolines[c->snapshot] = newTrampoline(c->snapshot ? (void *)transformSnapshotHook : (void *)transformMemorizeHook);
        if (!trampolines[c->snapshot]) {
            writeLog("spell_paths_site_mismatch", "group=call name=%s reason=no_padding", c->name);
            continue;
        }
        queuePathWord(c->callRva, blTo(gameBase + c->callRva, trampolines[c->snapshot]));
        matched++;
    }
    return matched;
}

// Raises the game's paths to 128, or changes nothing.
static int installSpellPathExpansion(void) {
    if (!gameCodeMatches) {
        writeLog("spell_paths_not_expanded", "reason=game_code_differs sites=0 of %d", PATH_SITE_COUNT);
        return 0;
    }
    size_t at = (uintptr_t)(gameBase + RVA_SPELL_PATH_NAMES) & ADRP_PAGE_MASK;
    size_t total = (at + SPELL_PATH_COUNT * sizeof(const char *) + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    unsigned char *memory = allocAfterImage(total);
    if (!memory) {
        writeLog("spell_paths_not_expanded", "reason=no_memory_near_game bytes=%zu sites=0 of %d", total, PATH_SITE_COUNT);
        return 0;
    }
    const char **copy = (const char **)(memory + at);
    int matched = queueSpellPathWrites(copy);
    if (matched != PATH_SITE_COUNT) {
        writeLog("spell_paths_not_expanded", "reason=game_build_differs sites=%d of %d", matched, PATH_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    unitSpellWeaponLevel = GAME_FN(int (*)(int, int), RVA_UNIT_SPELL_WEAPON_LEVEL);
    learnPromotionSpells = GAME_FN(void (*)(int, int, int), RVA_LEARN_PROMOTION_SPELLS);
    chooseMemorized = GAME_FN(void (*)(int), RVA_CHOOSE_MEMORIZED);
    startSpellPathCopy(copy);
    for (int i = 0; i < pathWriteCount; i++) {
        if (writeGameCode(pathWrites[i].at, &pathWrites[i].word, 4)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(pathWrites[j].at, &pathWrites[j].old, 4);
        writeLog("spell_paths_not_expanded", "reason=code_unwritable va=0x%llx sites=%d of %d",
                 (unsigned long long)(pathWrites[i].at - gameBase) + 0x100000000ULL, matched, PATH_SITE_COUNT);
        spellPathCopy = NULL;
        return 0;
    }
    pathMemory = memory;
    pathMemorySize = total;
    spellPathsExpanded = 1;
    writeLog("spell_paths_expanded", "paths=%d sites=%d of %d names=%p", SPELL_PATH_COUNT, matched, PATH_SITE_COUNT, (void *)copy);
    return 1;
}

// Puts back every word installSpellPathExpansion changed, or none.
static int uninstallSpellPathExpansion(void) {
    for (int i = 0; i < pathWriteCount; i++)
        if (pathGameWord((unsigned int)(pathWrites[i].at - gameBase)) != pathWrites[i].word) {
            writeLog("spell_paths_not_uninstalled", "reason=site_changed va=0x%llx", (unsigned long long)(pathWrites[i].at - gameBase) + 0x100000000ULL);
            return 0;
        }
    for (int i = 0; i < pathWriteCount; i++) {
        if (writeGameCode(pathWrites[i].at, &pathWrites[i].old, 4)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(pathWrites[j].at, &pathWrites[j].word, 4);
        writeLog("spell_paths_not_uninstalled", "reason=code_unwritable va=0x%llx", (unsigned long long)(pathWrites[i].at - gameBase) + 0x100000000ULL);
        return 0;
    }
    spellPathsExpanded = 0;
    spellPathCopy = NULL;
    mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)pathMemory, pathMemorySize);
    pathMemory = NULL;
    writeLog("spell_paths_uninstalled", "sites=%d", pathWriteCount);
    pathWriteCount = 0;
    return 1;
}
#endif

// Raises the paths.
static void spellPathExpansionInit(void) {
    installSpellPathExpansion();
}

// ---- Handing the paths to a plugin ---------------------------------------------------------------
// coe5seTakeOverSpellPaths, which a plugin calls from its coe5sePluginLoad.
static int spellPathsInUse(void);

static int takeOverSpellPaths(void *caller, SpellPathInstallFn install) {
    const char *call = SPELL_PATH_TAKEOVER_EXPORT;
    PluginRecord *r = loadingPlugin[0] ? pluginRecordFor(loadingPlugin) : NULL;
    if (!install) {
        refuseApiCall(caller, call, "no_callback", "paths=%d", spellPathTableSize());
        return spellPathTableSize();
    }
    if (!r || !r->entryCalled || r->reason) {
        refuseApiCall(caller, call, "not_in_plugin_entry", "paths=%d", spellPathTableSize());
        return spellPathTableSize();
    }
    if (pathTakeover.names) {
        refuseApiCall(caller, call, "already_taken", "by=%s paths=%d", pathTakeover.dll, pathTakeover.count);
        return pathTakeover.count;
    }
    if (spellPathsInUse()) {
        refuseApiCall(caller, call, "too_late", "paths=%d", spellPathTableSize());
        return spellPathTableSize();
    }
    int wasExpanded = spellPathsExpanded;
    if (wasExpanded && !uninstallSpellPathExpansion()) {
        refuseApiCall(caller, call, "coe5se_patch_changed", "paths=%d", spellPathTableSize());
        return spellPathTableSize();
    }
    int count = 0;
    const char **names = NULL;
    int installed = install(&count, &names);
    if (!installed || !names || count < GAME_SPELL_PATH_COUNT) {
        refuseApiCall(caller, call, installed ? "bad_result" : "not_installed", "count=%d names=%p", count, (void *)names);
        if (installed) return GAME_SPELL_PATH_COUNT;
        if (wasExpanded) installSpellPathExpansion();
        return spellPathTableSize();
    }
    pathTakeover.names = names;
    pathTakeover.count = count;
    pathTakeover.module = r->module;
    snprintf(pathTakeover.mod, sizeof pathTakeover.mod, "%s", r->mod);
    snprintf(pathTakeover.dll, sizeof pathTakeover.dll, "%.63s", r->file);
    writeLog("spell_paths_taken_over", "dll=%s mod=%s paths=%d names=%p", pathTakeover.dll, pathTakeover.mod, count, (void *)names);
    return count;
}

__declspec(dllexport) int coe5seTakeOverSpellPaths(SpellPathInstallFn install) {
    return takeOverSpellPaths(__builtin_return_address(0), install);
}

// The paths mods may use now, for DevTools and other readers outside CoE5SE.
__declspec(dllexport) int coe5seSpellPathCount(void) {
    return spellPathCount();
}

// Whether code at `address` runs in network games.
static int runsInNetworkGames(const void *address) {
    if (isCoe5seCode(address)) return 1;
    HMODULE m = pathTakeover.module ? moduleOf(address) : NULL;
    return m && m == pathTakeover.module;
}
