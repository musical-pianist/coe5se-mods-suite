// The Ritual Expansion: a ritual table of RITUAL_SLOT_CAP slots instead of the game's 3000.
#include <stdint.h>
#ifndef __APPLE__
#include "ritual_expansion_sites.h"
#endif

#define EXPAND_RITUAL_TABLE_WORD  "expand_ritual_table"
// Mods get MOD_RITUAL_TARGET rituals, slots 750 to LAST_MOD_RITUAL_SLOT.
#define FIRST_MOD_RITUAL_SLOT     750
#define MOD_RITUAL_TARGET         10000
#define LAST_MOD_RITUAL_SLOT      (FIRST_MOD_RITUAL_SLOT + MOD_RITUAL_TARGET - 1)
#define RITUAL_TABLE_SLOTS        (LAST_MOD_RITUAL_SLOT + 2)
_Static_assert(RITUAL_TABLE_SLOTS <= 0xffff, "the Mac's bounds hold the table's slots in a movz");
#define RITUAL_COUNTED_SLOTS      (LAST_MOD_RITUAL_SLOT + 1)
#define RITUAL_SLOT_CAP           RITUAL_TABLE_SLOTS
#define VANILLA_RITUAL_SLOTS      3000
#define EXTRA_RITUAL_SLOTS        (RITUAL_COUNTED_SLOTS - VANILLA_RITUAL_SLOTS)
#define RITUAL_RITPOW_OFFSET      0x30
#define RITUAL_LEVEL_OFFSET       0x34
// The ritual screen's row ids.
#define RITUAL_ROW_CASTABLE_BASE  1000
#define RITUAL_ROW_GREYED_BASE    0x3000
_Static_assert(RITUAL_ROW_GREYED_BASE == RITUAL_ROW_GREYED && RITUAL_ROW_CASTABLE_BASE == RITUAL_ROW_CASTABLE,
               "jazzy_api.h names the row ids the moved table uses");
_Static_assert(RITUAL_ROW_GREYED_BASE - RITUAL_ROW_CASTABLE_BASE > LAST_MOD_RITUAL_SLOT &&
               RITUAL_ROW_GREYED_BASE - 0x1000 <= LAST_MOD_RITUAL_SLOT + RITUAL_ROW_CASTABLE_BASE,
               "greyed row ids start at the first multiple of 0x1000 past the castable ones");
// CoE5SE's copy of the player blocks keeps every player's cast counts.
_Static_assert(RITUAL_COUNTED_SLOTS * 2 + 0x38 <= PLAYER_BLOCK_SIZE, "each player's cast counts must fit in one player block's stride");
// The player blocks start with the player's name, and the cast counts follow.
#define PLAYER_BLOCKS_RVA         RVA_SLOT_NAME
#define CAST_COUNTS_OFFSET        0x38
_Static_assert(RVA_CAST_COUNTS - PLAYER_BLOCKS_RVA == CAST_COUNTS_OFFSET, "the cast counts sit at + 0x38 in each player block");
#define PLAYER_BLOCKS_SIZE        (PLAYER_SLOTS_MOVED * PLAYER_BLOCK_SIZE)
#define CAST_COUNTS_BYTES         PLAYER_BLOCKS_SIZE

// The block of extra cast counts after a save or turn file's last marker.
#define EXTRA_CASTS_MAGIC         0x31435852
#define EXTRA_CASTS_MAX_SLOTS     EXTRA_RITUAL_SLOTS
#define SAVE_END_MARKER           0x69
#define TURN_END_MARKER           0x6f

// The Windows bounds: each is a 32-bit field the expansion sets to `value`.
#ifdef __APPLE__
#define WINDOWS_RVA(rva) 0u
#else
#define WINDOWS_RVA(rva) (rva)
#endif
typedef struct { unsigned int rva; int value; const char *name; } RitualWindowsBound;
static const RitualWindowsBound RITUAL_WINDOWS_BOUNDS[] __attribute__((unused)) = {
    {WINDOWS_RVA(RVA_ALLOC_RITUAL_BOUND),          RITUAL_TABLE_SLOTS, "alloc_ritual"},
    {WINDOWS_RVA(RVA_RITUAL_ADD_FX_BOUND),         LAST_MOD_RITUAL_SLOT, "add_fx"},
    {WINDOWS_RVA(RVA_RITUAL_ADD_STRING_BOUND),     LAST_MOD_RITUAL_SLOT, "add_string"},
    {WINDOWS_RVA(RVA_EXECUTE_RITUAL_BOUND),        RITUAL_TABLE_SLOTS, "execute_ritual"},
    {WINDOWS_RVA(RVA_RITUAL_PR_BOUND),             RITUAL_TABLE_SLOTS, "pr"},
    {WINDOWS_RVA(RVA_RITUAL_CPR_BOUND),            RITUAL_TABLE_SLOTS, "cpr"},
    {WINDOWS_RVA(RVA_UNIT_RITUAL_LIST_BOUND),      RITUAL_TABLE_SLOTS, "unit_ritual_list"},
    {WINDOWS_RVA(RVA_RITUAL_SCREEN_LOOP_1),        RITUAL_TABLE_SLOTS, "screen_loop_1"},
    {WINDOWS_RVA(RVA_RITUAL_SCREEN_LOOP_2),        RITUAL_TABLE_SLOTS, "screen_loop_2"},
    {WINDOWS_RVA(RVA_RITUAL_BACKUP_SIZE),          RITUAL_TABLE_SLOTS * RITUAL_SIZE, "backup_size"},
    {WINDOWS_RVA(RVA_RITUAL_BACKUP_COPY_SIZE),     RITUAL_TABLE_SLOTS * RITUAL_SIZE, "backup_copy_size"},
    {WINDOWS_RVA(RVA_RITUAL_RESTORE_SIZE),         RITUAL_TABLE_SLOTS * RITUAL_SIZE, "restore_size"},
    {WINDOWS_RVA(RVA_RITUAL_ROW_ID_GREYED),        RITUAL_ROW_GREYED_BASE, "row_id_greyed"},
    {WINDOWS_RVA(RVA_RITUAL_CLICK_CASTABLE_BOUND), LAST_MOD_RITUAL_SLOT, "click_castable"},
    {WINDOWS_RVA(RVA_RITUAL_RIGHT_CLICK_BAND),     RITUAL_ROW_GREYED_BASE - RITUAL_ROW_CASTABLE_BASE, "right_click_band"},
    {WINDOWS_RVA(RVA_RITUAL_RIGHT_CLICK_GREYED),   -RITUAL_ROW_GREYED_BASE, "right_click_greyed"},
};
#define RITUAL_WINDOWS_BOUND_COUNT ((int)(sizeof RITUAL_WINDOWS_BOUNDS / sizeof RITUAL_WINDOWS_BOUNDS[0]))

static int ritualExpansionActive;
static unsigned char *ritualTable;

// The table's RVA and slot count: the game's own, or the moved table's.
static unsigned int ritualTableRva(void) {
    return ritualExpansionActive ? (unsigned int)(ritualTable - gameBase) : RVA_RITUAL_TABLE;
}
static int ritualSlotCount(void) { return ritualExpansionActive ? RITUAL_SLOT_CAP : VANILLA_RITUAL_SLOTS; }

// The .c5m line expand_ritual_table, kept so mods that carry it still load.
static void expandRitualTableLine(const char *args, int mod, int level) {
    (void)level;
    if (!args[strspn(args, " \t\r\n")]) return;
    int known = mod >= 0 && mod <= G32(RVA_MOD_LAST_INDEX);
    refuseC5mLine(EXPAND_RITUAL_TABLE_WORD, known ? modName(mod) : "-", "unexpected_value", "line=%d value=%.60s", G32(RVA_MOD_LINE), args);
}

#ifndef __APPLE__
// ---- Windows: the patch ------------------------------------------------------------------------
typedef int (*ReadIntFn)(void *file);
typedef int (*WriteIntFn)(int value, void *file);
typedef int (*ReadArrayFn)(void *data, int elementSize, int count, void *file);
typedef unsigned long long (*WriteArrayFn)(void *data, int elementSize, int count, void *file);
typedef void *(*MemsetFn)(void *dst, int value, size_t size);
typedef void *(*MemmoveFn)(void *dst, const void *src, size_t size);
typedef size_t (*FreadFn)(void *data, size_t size, size_t count, void *file);
typedef void (*SavePlayerBlockFn)(int player, void *file);
#define READ_INT     GAME_FN(ReadIntFn, RVA_FILE_READ_INT)
#define WRITE_INT    GAME_FN(WriteIntFn, RVA_FILE_WRITE_INT)
#define READ_ARRAY   GAME_FN(ReadArrayFn, RVA_FILE_READ_ARRAY)
#define WRITE_ARRAY  GAME_FN(WriteArrayFn, RVA_FILE_WRITE_ARRAY)
#define STUB_BYTES   16

static unsigned char *castCounts;
static unsigned char *ritualStubs;
static unsigned char *oldTableStart, *oldTableEnd;
static int turnFilePlayer = -1;
#else
// ---- Mac: the patch ----------------------------------------------------------------------------
// The Mac side, with arm64 instructions.
#include "ritual_expansion_sites_mac.h"
#include <signal.h>
#include <sys/ucontext.h>

// The game's file helpers are inlined on Mac.
static void countWritten(size_t bytes) { *(long long *)(gameBase + RVA_FILE_BYTES_WRITTEN) += (long long)bytes; }
static int macReadInt(void *file) {
    int value = 0;
    return fread(&value, 1, 4, file) == 4 ? value : 0;
}
static int macWriteInt(int value, void *file) {
    size_t written = fwrite(&value, 1, 4, file);
    countWritten(4);
    return (int)written;
}
static int macReadArray(void *data, int elementSize, int count, void *file) {
    return (int)fread(data, 1, (size_t)elementSize * (size_t)count, file);
}
static unsigned long long macWriteArray(void *data, int elementSize, int count, void *file) {
    size_t written = fwrite(data, 1, (size_t)elementSize * (size_t)count, file);
    countWritten((size_t)elementSize * (size_t)count);
    return written;
}
#define READ_INT     macReadInt
#define WRITE_INT    macWriteInt
#define READ_ARRAY   macReadArray
#define WRITE_ARRAY  macWriteArray

static unsigned char *castCounts;
static unsigned char *oldTableStart, *oldTableEnd;
static int turnFilePlayer = -1;
#endif

// Returns the moved cast counts for a pointer to a player's first count in the game's own.
static void *movedCastCounts(void *counts, int *player) {
    ptrdiff_t offset = (unsigned char *)counts - (gameBase + RVA_CAST_COUNTS);
    if (offset < 0 || offset >= (ptrdiff_t)PLAYER_SLOTS * PLAYER_BLOCK_SIZE) offset = (unsigned char *)counts - playerTableAddress(RVA_CAST_COUNTS);
    if (offset < 0 || offset % PLAYER_BLOCK_SIZE || offset / PLAYER_BLOCK_SIZE >= PLAYER_SLOTS_MOVED) {
        writeLog("ritual_cast_counts_unexpected", "pointer=%p", counts);
        return counts;
    }
    *player = (int)(offset / PLAYER_BLOCK_SIZE);
    return castCounts + offset;
}

// Player 0's first cast count, where the game counts now.
static unsigned char *castCountsBase(void) { return ritualExpansionActive && castCounts ? castCounts : gameBase + RVA_CAST_COUNTS; }

static short *extraCastCounts(int player) {
    return (short *)(castCounts + (size_t)player * PLAYER_BLOCK_SIZE) + VANILLA_RITUAL_SLOTS;
}

static int hasExtraCastCounts(int player) {
    for (int s = 0; s < EXTRA_RITUAL_SLOTS; s++)
        if (extraCastCounts(player)[s]) return 1;
    return 0;
}

#ifndef __APPLE__
// The game's reads of one player's 3000 counts, read into the moved counts.
static size_t readCastCounts(void *counts, size_t size, size_t count, void *file) {
    int player = -1;
    void *moved = movedCastCounts(counts, &player);
    turnFilePlayer = player;
    return GAME_FN(FreadFn, RVA_FREAD)(moved, size, count, file);
}

// The turn file writer's save_player_block(player, file).
static void saveTurnPlayerBlock(int player, void *file) {
    turnFilePlayer = player;
    if (player >= 0 && player < PLAYER_SLOTS)
        memcpy(playerTableAddress(RVA_CAST_COUNTS) + (size_t)player * PLAYER_BLOCK_SIZE, castCounts + (size_t)player * PLAYER_BLOCK_SIZE,
               VANILLA_RITUAL_SLOTS * sizeof(short));
    GAME_FN(SavePlayerBlockFn, RVA_SAVE_PLAYER_BLOCK)(player, file);
}
#endif

// Writes the extra counts block.
static void writeExtraCastCounts(void *file, int onlyPlayer, const char *kind) {
    int inBlock[PLAYER_SLOTS], players = 0;
    for (int p = 0; p < PLAYER_SLOTS; p++) {
        inBlock[p] = ritualExpansionActive && (onlyPlayer >= 0 ? p == onlyPlayer : hasExtraCastCounts(p));
        players += inBlock[p];
    }
    WRITE_INT(EXTRA_CASTS_MAGIC, file);
    WRITE_INT(VANILLA_RITUAL_SLOTS, file);
    WRITE_INT(EXTRA_RITUAL_SLOTS, file);
    WRITE_INT(players, file);
    for (int p = 0; p < PLAYER_SLOTS; p++) {
        if (!inBlock[p]) continue;
        WRITE_INT(p, file);
        WRITE_ARRAY(extraCastCounts(p), 2, EXTRA_RITUAL_SLOTS, file);
    }
    WRITE_INT(EXTRA_CASTS_MAGIC, file);
    writeLog("ritual_cast_counts_written", "file=%s players=%d slots=%d", kind, players, EXTRA_RITUAL_SLOTS);
}

// Reads the extra counts block after the file's last marker, if the file has one.
static int readExtraCastCounts(void *file, int onlyPlayer, const char *kind) {
    for (int p = 0; ritualExpansionActive && p < PLAYER_SLOTS; p++)
        if (onlyPlayer < 0 || p == onlyPlayer) memset(extraCastCounts(p), 0, EXTRA_RITUAL_SLOTS * sizeof(short));
    if (READ_INT(file) != EXTRA_CASTS_MAGIC) {
        writeLog("ritual_cast_counts_read", "file=%s players=0 reason=no_block", kind);
        return 0;
    }
    int first = READ_INT(file), slots = READ_INT(file), players = READ_INT(file);
    if (first != VANILLA_RITUAL_SLOTS || slots < 0 || slots > EXTRA_CASTS_MAX_SLOTS || players < 0 || players > PLAYER_SLOTS) {
        writeLog("ritual_cast_counts_bad_block", "file=%s first=%d slots=%d players=%d", kind, first, slots, players);
        return -1;
    }
    size_t perPlayer = (size_t)slots * sizeof(short);
    unsigned char *counts = calloc((size_t)players * perPlayer + 1, 1);
    int *who = calloc((size_t)players + 1, sizeof(int));
    int ok = counts && who;
    for (int i = 0; ok && i < players; i++) {
        who[i] = READ_INT(file);
        ok = who[i] >= 0 && who[i] < PLAYER_SLOTS && (!slots || READ_ARRAY(counts + perPlayer * i, 2, slots, file) == (int)perPlayer);
    }
    ok = ok && READ_INT(file) == EXTRA_CASTS_MAGIC;
    for (int i = 0; ok && ritualExpansionActive && i < players; i++)
        if (onlyPlayer < 0 || who[i] == onlyPlayer) memcpy(extraCastCounts(who[i]), counts + perPlayer * i, perPlayer);
    if (ok && (ritualExpansionActive || !players)) writeLog("ritual_cast_counts_read", "file=%s players=%d slots=%d", kind, players, slots);
    else if (ok) writeLog("ritual_cast_counts_read", "file=%s players=%d slots=%d kept=0 reason=expansion_off", kind, players, slots);
    else writeLog("ritual_cast_counts_bad_block", "file=%s players=%d slots=%d reason=%s", kind, players, slots,
                  counts && who ? "short_block_or_bad_player" : "out_of_memory");
    free(counts);
    free(who);
    return ok ? 1 : -1;
}

// The game's clears and moves of the player blocks.
static void mirrorPlayerBlocks(const unsigned char *dst, const unsigned char *src, size_t size, int value) {
    const unsigned char *blocks = playerTableAddress(PLAYER_BLOCKS_RVA);
    size_t span = (size_t)playerSlotCount() * PLAYER_BLOCK_SIZE;
    if (dst < blocks || dst + size > blocks + span) return;
    if (src && (src < blocks || src + size > blocks + span)) {
        writeLog("ritual_cast_counts_unexpected", "copy=move offset=%zu size=%zu", (size_t)(dst - blocks), size);
        return;
    }
    if (!src) memset(castCounts + (dst - blocks), value, size);
    else memmove(castCounts + (dst - blocks), castCounts + (src - blocks), size);
}

#ifndef __APPLE__
static void *clearPlayerBlocks(void *dst, int value, size_t size) {
    void *result = GAME_FN(MemsetFn, RVA_MEMSET)(dst, value, size);
    mirrorPlayerBlocks(dst, NULL, size, value);
    return result;
}

static void *movePlayerBlock(void *dst, const void *src, size_t size) {
    void *result = GAME_FN(MemmoveFn, RVA_MEMMOVE)(dst, src, size);
    mirrorPlayerBlocks(dst, src, size, 0);
    return result;
}
#endif

// random_ritual(ritpow, level): a random slot whose ritpow and level match, or 0.
static int randomRitual(int ritpow, int level) {
    static int matches[RITUAL_SLOT_CAP];
    int count = 0;
    for (int slot = 1; slot < RITUAL_SLOT_CAP; slot++) {
        const unsigned char *r = ritualTable + (size_t)slot * RITUAL_SIZE;
        int p = *(const int *)(r + RITUAL_RITPOW_OFFSET), l = *(const int *)(r + RITUAL_LEVEL_OFFSET);
        if ((p == ritpow || ritpow == -1) && (l == level || level == -1)) matches[count++] = slot;
    }
    if (count < 1) return 0;
    return matches[GAME_FN(int (*)(int), RVA_RANDOM)(count)];
}

// newritual's "Out of rituals: %s" when alloc_ritual has no slot left.
static struct { int active, mod, line, skipped; char name[RITUAL_NAME_SIZE]; } refusedRitual;

static void endRefusedRitual(void) {
    if (!refusedRitual.active) return;
    refusedRitual.active = 0;
    int known = refusedRitual.mod >= 0 && refusedRitual.mod <= G32(RVA_MOD_LAST_INDEX);
    writeLog("ritual_refused_lines_skipped", "mod=%s name=%.48s lines=%d", known ? modName(refusedRitual.mod) : "-", refusedRitual.name,
             refusedRitual.skipped);
}

// The mods that had rituals refused in the latest pass.
#define RITUALS_LEFT_OUT_LINES 16
#define RITUALS_LEFT_OUT_ROWS  10
static struct {
    char (*mods)[MOD_NAME_MAX];
    int count, capacity;
    int level, lastLevel;
} ritualsLeftOut = {NULL, 0, 0, -1, -1};

// Adds mod `mod` to the list, once.
static void noteRitualsLeftOut(const char *mod) {
    for (int i = 0; i < ritualsLeftOut.count; i++)
        if (!strcmp(ritualsLeftOut.mods[i], mod)) return;
    if (!growList((void **)&ritualsLeftOut.mods, &ritualsLeftOut.capacity, ritualsLeftOut.count + 1, sizeof *ritualsLeftOut.mods))
        return;
    snprintf(ritualsLeftOut.mods[ritualsLeftOut.count++], MOD_NAME_MAX, "%s", mod);
}

// The pop-up's text: its opening line, then one mod per line, by its display name.
static size_t ritualsLeftOutText(char *text, size_t size) {
    size_t n = 0;
    char label[80];
    text[0] = '\0';
    appendText(text, size, &n, "Rituals left out (no room):\n");
    for (int i = 0; i < ritualsLeftOut.count && i < RITUALS_LEFT_OUT_LINES; i++) {
        modDisplayName(ritualsLeftOut.mods[i], label, sizeof label);
        appendText(text, size, &n, "\n%s", label);
    }
    if (ritualsLeftOut.count > RITUALS_LEFT_OUT_LINES) appendText(text, size, &n, "\n...and others.");
    return n;
}

// The pop-up, once for each new set of mods.
static void popupRitualsLeftOut(void) {
    static unsigned int shownHash;
    static char text[POPUP_TEXT_SIZE];
    if (!ritualsLeftOut.count) {
        shownHash = 0;
        return;
    }
    ritualsLeftOutText(text, sizeof text);
    unsigned int hash = 2166136261u;
    for (const char *c = text; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
    if (hash == shownHash) return;
    shownHash = hash;
    writeLog("rituals_left_out_popup", "mods=%d", ritualsLeftOut.count);
    okBox("CoE5SE: Rituals left out", text);
}

// The CoE5SE page's "Rituals left out" box, drawn only when a mod's rituals were left out.
static void drawRitualsLeftOut(void) {
    if (!ritualsLeftOut.count) return;
    sectionBegin();
    sectionHeader("Rituals left out");
    char label[80], tip[256];
    for (int i = 0; i < ritualsLeftOut.count && i < RITUALS_LEFT_OUT_ROWS; i++) {
        modDisplayName(ritualsLeftOut.mods[i], label, sizeof label);
        textButton(0, label);
        snprintf(tip, sizeof tip, "%.79s has more rituals than fit, so its last rituals were left out. coe5mod.log has each "
                 "as c5m_refused.", label);
        tooltip(tip);
    }
    if (ritualsLeftOut.count > RITUALS_LEFT_OUT_ROWS) textButton(0, "...and others. coe5mod.log has each as c5m_refused.");
    sectionEnd();
}

static void refuseOutOfRituals(const char *name) {
    endRefusedRitual();
    int known = modCommandMod >= 0 && modCommandMod <= G32(RVA_MOD_LAST_INDEX);
    if (known) noteRitualsLeftOut(modName(modCommandMod));
    ritualsLeftOut.level = modCommandLevel;
    refuseC5mLine("newritual", known ? modName(modCommandMod) : "-", "out_of_ritual_slots", "line=%d name=%.48s last_slot=%d count=%d",
                  G32(RVA_MOD_LINE), name ? name : "", LAST_MOD_RITUAL_SLOT, G32(RVA_RITUAL_COUNT));
    refusedRitual.active = 1;
    refusedRitual.mod = modCommandMod;
    refusedRitual.line = G32(RVA_MOD_LINE);
    refusedRitual.skipped = 0;
    snprintf(refusedRitual.name, sizeof refusedRitual.name, "%s", name ? name : "");
}

// Returns 1 for a line of a refused ritual, which the game then never sees.
static int refusedRitualLine(const char *line, size_t len, int mod, int level) {
    if (level != ritualsLeftOut.lastLevel) {
        if (level == ritualsLeftOut.level) ritualsLeftOut.count = 0;
        ritualsLeftOut.lastLevel = level;
    }
    if (!refusedRitual.active) return 0;
    int number = G32(RVA_MOD_LINE);
    int starts = (len >= 3 && !strncmp(line, "new", 3)) || (len >= 6 && !strncmp(line, "select", 6));
    if (mod != refusedRitual.mod || number < refusedRitual.line || starts) {
        endRefusedRitual();
        return 0;
    }
    refusedRitual.line = number;
    refusedRitual.skipped++;
    return 1;
}
#ifndef __APPLE__
static void outOfRitualsHook(const char *fmt, const char *name) {
    (void)fmt;
    refuseOutOfRituals(name);
}
#else
// On Mac the word after the game's call starts selectritual's block.
__attribute__((used)) void *ritualOutOfRitualsExit;
__attribute__((used)) void ritualOutOfRitualsRefuse(const char *name);
__attribute__((used)) void ritualOutOfRitualsRefuse(const char *name) { refuseOutOfRituals(name); }
__asm__(
    ".text\n.p2align 2\n"
    "_ritualOutOfRitualsStub:\n"
    "  ldr x0, [sp]\n  bl _ritualOutOfRitualsRefuse\n"
    "  adrp x16, _ritualOutOfRitualsExit@PAGE\n  ldr x16, [x16, _ritualOutOfRitualsExit@PAGEOFF]\n  br x16\n");
extern const unsigned char ritualOutOfRitualsStub[] __asm__("_ritualOutOfRitualsStub");
#endif

#ifndef __APPLE__
// Logs a read or write of the old table before the game stops on it.
static LONG WINAPI logOldRitualTableAccess(EXCEPTION_POINTERS *e) {
    const EXCEPTION_RECORD *r = e->ExceptionRecord;
    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2) return EXCEPTION_CONTINUE_SEARCH;
    const unsigned char *target = (const unsigned char *)r->ExceptionInformation[1];
    if (target < oldTableStart || target >= oldTableEnd) return EXCEPTION_CONTINUE_SEARCH;
    const unsigned char *at = r->ExceptionAddress;
    ptrdiff_t fromTable = target - (gameBase + RVA_RITUAL_TABLE);
    writeLog("ritual_old_table_access", "instruction=%p instruction_rva=0x%llx write=%d slot=%d field=0x%x", (const void *)at,
             (unsigned long long)(at - gameBase), (int)r->ExceptionInformation[0], (int)(fromTable / RITUAL_SIZE),
             (unsigned)(fromTable % RITUAL_SIZE));
    return EXCEPTION_CONTINUE_SEARCH;
}

// The changes to the game's code, queued while checking and written once all are checked.
#define MAX_RITUAL_WRITES (RITUAL_TABLE_SITE_COUNT + RITUAL_CAST_SITE_COUNT + RITUAL_NAMED_SITE_COUNT + 64)
static struct { unsigned char *at; int size; unsigned char bytes[16], old[16]; } ritualWrites[MAX_RITUAL_WRITES];
static int ritualWriteCount, ritualWritesOverflowed;

static void queueWrite(unsigned char *at, const void *bytes, int size) {
    if (ritualWriteCount >= MAX_RITUAL_WRITES) {
        ritualWritesOverflowed = 1;
        return;
    }
    ritualWrites[ritualWriteCount].at = at;
    ritualWrites[ritualWriteCount].size = size;
    memcpy(ritualWrites[ritualWriteCount].old, at, (size_t)size);
    memcpy(ritualWrites[ritualWriteCount++].bytes, bytes, (size_t)size);
}

// Queues a generated site's field's new value.
static int queueSite(const RitualPatchSite *s, const char *group, long long delta, int value) {
    unsigned char *at = gameBase + s->rva;
    int field;
    memcpy(&field, at + s->fieldOffset, 4);
    long long moved = s->kind == RITUAL_FIELD_IMM32 ? value : field + delta;
    if (moved < INT32_MIN || moved > INT32_MAX) {
        writeLog("ritual_expansion_site_out_of_reach", "group=%s rva=0x%x", group, s->rva);
        return 0;
    }
    int out = (int)moved;
    queueWrite(at + s->fieldOffset, &out, 4);
    return 1;
}

// Checks that the instruction at `callRva` calls `targetRva`, and queues its redirect.
static int queueCall(int stub, unsigned int callRva, unsigned int targetRva, void *hook, const char *name) {
    unsigned char *call = gameBase + callRva, *at = ritualStubs + STUB_BYTES * stub;
    int rel;
    memcpy(&rel, call + 1, 4);
    if (call[0] != 0xe8 || call + 5 + rel != gameBase + targetRva) {
        writeLog("ritual_expansion_site_mismatch", "group=call name=%s rva=0x%x", name, callRva);
        return 0;
    }
    at[0] = 0xff; at[1] = 0x25; memset(at + 2, 0, 4);
    memcpy(at + 6, &hook, 8);
    long long toStub = at - (call + 5);
    if (toStub < INT32_MIN || toStub > INT32_MAX) {
        writeLog("ritual_expansion_site_out_of_reach", "group=call name=%s rva=0x%x", name, callRva);
        return 0;
    }
    int out = (int)toStub;
    queueWrite(call + 1, &out, 4);
    return 1;
}

// Finds and allocates a free block of `size` bytes within 2 GB above the image base.
static unsigned char *allocNearImage(size_t size) {
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(gameBase + ((const IMAGE_DOS_HEADER *)gameBase)->e_lfanew);
    uintptr_t granularity = 0x10000;
    uintptr_t at = ((uintptr_t)gameBase + nt->OptionalHeader.SizeOfImage + granularity - 1) & ~(granularity - 1);
    uintptr_t last = (uintptr_t)gameBase + 0x7fff0000 - size;
    while (at <= last) {
        MEMORY_BASIC_INFORMATION region;
        if (!VirtualQuery((void *)at, &region, sizeof region)) break;
        uintptr_t regionEnd = (uintptr_t)region.BaseAddress + region.RegionSize;
        if (region.State == MEM_FREE && regionEnd - at >= size) {
            void *p = VirtualAlloc((void *)at, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (p) return p;
        }
        at = (regionEnd + granularity - 1) & ~(granularity - 1);
    }
    return NULL;
}

// A redirected call: the call instruction, the function it calls, and CoE5SE's hook.
typedef struct { unsigned int callRva, targetRva; void *hook; const char *name; } RitualCall;

#define WIN_PAGE_MASK 0xfff

// Checks every instruction the expansion changes, and queues its new bytes.
static int queueRitualWrites(unsigned char *memory, size_t castsAt, int *callCount) {
    unsigned char *oldTable = gameBase + RVA_RITUAL_TABLE;
    size_t tableSize = (size_t)RITUAL_SLOT_CAP * RITUAL_SIZE;
    long long tableDelta = memory - oldTable;
    long long endDelta = (memory + tableSize) - (oldTable + (size_t)VANILLA_RITUAL_SLOTS * RITUAL_SIZE);
    long long castDelta = (memory + castsAt) - (gameBase + RVA_CAST_COUNTS);
    int ok = 1;
    ritualWriteCount = 0;
    for (int i = 0; i < RITUAL_TABLE_SITE_COUNT; i++)
        ok &= queueSite(&RITUAL_TABLE_SITES[i], "table", RITUAL_TABLE_SITES[i].kind == RITUAL_FIELD_END_REL32 ? endDelta : tableDelta, 0);
    for (int i = 0; i < RITUAL_CAST_SITE_COUNT; i++)
        ok &= queueSite(&RITUAL_CAST_SITES[i], "cast_counts", castDelta, 0);
    _Static_assert(sizeof RITUAL_WINDOWS_BOUNDS / sizeof RITUAL_WINDOWS_BOUNDS[0] == RITUAL_NAMED_SITE_COUNT, "a new value for every named site");
    for (int i = 0; i < RITUAL_WINDOWS_BOUND_COUNT; i++) {
        int j = 0;
        while (j < RITUAL_NAMED_SITE_COUNT && RITUAL_NAMED_SITES[j].rva != RITUAL_WINDOWS_BOUNDS[i].rva) j++;
        if (j == RITUAL_NAMED_SITE_COUNT) {
            writeLog("ritual_expansion_site_missing", "rva=0x%x", RITUAL_WINDOWS_BOUNDS[i].rva);
            ok = 0;
            continue;
        }
        ok &= queueSite(&RITUAL_NAMED_SITES[j], "bound", 0, RITUAL_WINDOWS_BOUNDS[i].value);
    }
    const RitualCall CALLS[] = {
        {RVA_LOAD_2H_CAST_COUNTS_CALL,   RVA_FREAD,             (void *)readCastCounts,      "load_2h_cast_counts"},
        {RVA_SAVE_PLAYER_BLOCK_CALL,     RVA_SAVE_PLAYER_BLOCK, (void *)saveTurnPlayerBlock, "save_2h_cast_counts"},
        {RVA_LOAD_GAME_CAST_COUNTS_CALL, RVA_FREAD,             (void *)readCastCounts,      "load_game_cast_counts"},
        {RVA_RESET_PLAYERS_CLEAR,        RVA_MEMSET,            (void *)clearPlayerBlocks,   "reset_players_clear"},
        {RVA_KEY_CHECK_PLAYERS_CLEAR,    RVA_MEMSET,            (void *)clearPlayerBlocks,   "key_check_players_clear"},
        {RVA_COMPACT_PLAYERS_MOVE,       RVA_MEMMOVE,           (void *)movePlayerBlock,     "compact_players_move"},
        {RVA_OUT_OF_RITUALS_CALL,        RVA_FATAL_FORMAT,      (void *)outOfRitualsHook,    "out_of_rituals"},
    };
    *callCount = (int)(sizeof CALLS / sizeof CALLS[0]);
    _Static_assert(sizeof CALLS / sizeof CALLS[0] * STUB_BYTES <= 0x1000, "every stub fits the one page allocNear gives");
    for (int i = 0; i < *callCount; i++)
        ok &= queueCall(i, CALLS[i].callRva, CALLS[i].targetRva, CALLS[i].hook, CALLS[i].name);
    unsigned char *entry = gameBase + RVA_RANDOM_RITUAL;
    if (!gameCodeMatches) {
        writeLog("ritual_expansion_site_mismatch", "group=entry name=random_ritual rva=0x%x reason=game_code_differs", RVA_RANDOM_RITUAL);
        ok = 0;
    } else {
        unsigned char jump[RITUAL_RANDOM_ENTRY_SIZE] = {0xff, 0x25, 0, 0, 0, 0};
        void *hook = (void *)randomRitual;
        memcpy(jump + 6, &hook, 8);
        queueWrite(entry, jump, RITUAL_RANDOM_ENTRY_SIZE);
    }
    if (ritualWritesOverflowed) {
        writeLog("ritual_expansion_too_many_writes", "max=%d", MAX_RITUAL_WRITES);
        ok = 0;
    }
    return ok;
}

// Makes the old table's whole pages unreadable.
static void protectOldRitualTable(unsigned char *oldTable) {
    oldTableStart = (unsigned char *)(((uintptr_t)oldTable + WIN_PAGE_MASK) & ~(uintptr_t)WIN_PAGE_MASK);
    oldTableEnd = (unsigned char *)(((uintptr_t)oldTable + (size_t)VANILLA_RITUAL_SLOTS * RITUAL_SIZE) & ~(uintptr_t)WIN_PAGE_MASK);
    DWORD old;
    int guarded = VirtualProtect(oldTableStart, (size_t)(oldTableEnd - oldTableStart), PAGE_NOACCESS, &old) != 0;
    if (guarded) AddVectoredExceptionHandler(1, logOldRitualTableAccess);
    writeLog("ritual_old_table_protected", "ok=%d from=%p to=%p error=%lu", guarded, (void *)oldTableStart, (void *)oldTableEnd,
             guarded ? 0UL : GetLastError());
}

// Moves the table and turns every part of the expansion on, or changes nothing.
static void adjustSaveCastCountRead(void);
static int installRitualExpansion(void) {
    if (!gameCodeMatches) {
        writeLog("ritual_expansion_off", "reason=game_code_differs queued=0");
        return 0;
    }
    size_t tableSize = (size_t)RITUAL_SLOT_CAP * RITUAL_SIZE, castsAt = (tableSize + WIN_PAGE_MASK) & ~(size_t)WIN_PAGE_MASK;
    unsigned char *memory = allocNearImage(castsAt + CAST_COUNTS_BYTES);
    if (!memory || !ritualStubs) {
        writeLog("ritual_expansion_off", "reason=no_memory_near_game table=%p stubs=%p bytes=%zu", (void *)memory, (void *)ritualStubs,
                 castsAt + CAST_COUNTS_BYTES);
        if (memory) VirtualFree(memory, 0, MEM_RELEASE);
        return 0;
    }
    writeLog("ritual_table_allocated", "address=%p rva=0x%llx slots=%d bytes=%zu", (void *)memory,
             (unsigned long long)(memory - gameBase), RITUAL_SLOT_CAP, castsAt + CAST_COUNTS_BYTES);
    unsigned char *oldTable = gameBase + RVA_RITUAL_TABLE;
    int callCount = 0;
    int ok = queueRitualWrites(memory, castsAt, &callCount);
    if (!ok) {
        writeLog("ritual_expansion_off", "reason=game_build_differs queued=%d", ritualWriteCount);
        VirtualFree(memory, 0, MEM_RELEASE);
        return 0;
    }
    writeLog("ritual_expansion_checked", "table_sites=%d cast_sites=%d bounds=%d calls=%d writes=%d", RITUAL_TABLE_SITE_COUNT,
             RITUAL_CAST_SITE_COUNT, RITUAL_NAMED_SITE_COUNT, callCount, ritualWriteCount);

    makePagesReadExecute(ritualStubs, 0x1000);
    if (!openGameCode(gameBase + RVA_TEXT_SECTION, (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION))) {
        writeLog("ritual_expansion_code_unwritable", "error=%lu", GetLastError());
        writeLog("ritual_expansion_off", "reason=code_unwritable queued=%d", ritualWriteCount);
        VirtualFree(memory, 0, MEM_RELEASE);
        return 0;
    }
    ritualTable = memory;
    castCounts = memory + castsAt;
    memcpy(ritualTable, oldTable, (size_t)VANILLA_RITUAL_SLOTS * RITUAL_SIZE);
    for (int p = 0; p < PLAYER_SLOTS; p++)
        memcpy(castCounts + (size_t)p * PLAYER_BLOCK_SIZE, gameBase + RVA_CAST_COUNTS + (size_t)p * PLAYER_BLOCK_SIZE,
               VANILLA_RITUAL_SLOTS * sizeof(short));
    for (int i = 0; i < ritualWriteCount; i++) {
        if (writeGameCode(ritualWrites[i].at, ritualWrites[i].bytes, (size_t)ritualWrites[i].size)) continue;
        writeLog("ritual_expansion_code_unwritable", "error=%lu", GetLastError());
        for (int j = i; j >= 0; j--) writeGameCode(ritualWrites[j].at, ritualWrites[j].old, (size_t)ritualWrites[j].size);
        closeGameCode();
        writeLog("ritual_expansion_off", "reason=code_unwritable queued=%d", ritualWriteCount);
        ritualTable = castCounts = NULL;
        VirtualFree(memory, 0, MEM_RELEASE);
        return 0;
    }
    closeGameCode();
    ritualExpansionActive = 1;
    adjustSaveCastCountRead();
    writeLog("ritual_table_moved", "from=%p to=%p slots=%d mod_slots=%d-%d cast_counts=%p", (void *)oldTable,
             (void *)ritualTable, RITUAL_SLOT_CAP, FIRST_MOD_RITUAL_SLOT, LAST_MOD_RITUAL_SLOT, (void *)castCounts);
    protectOldRitualTable(oldTable);
    return 1;
}
#else
// The hooks on the game's libc calls.
static size_t readCastCounts(void *counts, size_t size, size_t count, FILE *file) {
    int player = -1;
    void *moved = movedCastCounts(counts, &player);
    turnFilePlayer = player;
    return fread(moved, size, count, file);
}

// The game clears all player blocks with bzero, and moves one down with memcpy.
static void clearPlayerBlocks(void *dst, size_t size) {
    bzero(dst, size);
    mirrorPlayerBlocks(dst, NULL, size, 0);
}

static void *movePlayerBlock(void *dst, const void *src, size_t size) {
    void *result = memmove(dst, src, size);
    mirrorPlayerBlocks(dst, src, size, 0);
    return result;
}

// random_ritual's inlined copies branch to these stubs (RVA_RANDOM_RITUAL_*_ENTRY).
__attribute__((used)) void *ritualRandomResume[5];
__attribute__((used)) int *ritualRandomIndex;
__attribute__((used)) int ritualRandomDraw(int ritpow, int level);
__attribute__((used)) int ritualRandomDraw(int ritpow, int level) { return randomRitual(ritpow, level); }
__asm__(
    ".text\n.p2align 2\n"
    ".macro RX_SAVE\n"
    "  stp x29, x30, [sp, #-0x90]!\n  mov x29, sp\n"
    "  stp x0, x1, [sp, #0x10]\n  stp x2, x3, [sp, #0x20]\n  stp x4, x5, [sp, #0x30]\n  stp x6, x7, [sp, #0x40]\n"
    "  stp x8, x9, [sp, #0x50]\n  stp x10, x11, [sp, #0x60]\n  stp x12, x13, [sp, #0x70]\n  stp x14, x15, [sp, #0x80]\n"
    ".endm\n"
    ".macro RX_RESTORE\n"
    "  ldp x0, x1, [sp, #0x10]\n  ldp x2, x3, [sp, #0x20]\n  ldp x4, x5, [sp, #0x30]\n  ldp x6, x7, [sp, #0x40]\n"
    "  ldp x8, x9, [sp, #0x50]\n  ldp x10, x11, [sp, #0x60]\n  ldp x12, x13, [sp, #0x70]\n  ldp x14, x15, [sp, #0x80]\n"
    "  ldp x29, x30, [sp], #0x90\n"
    ".endm\n"
    ".macro RX_RESUME reg, index\n"
    "  adrp \\reg, _ritualRandomResume@PAGE\n  add \\reg, \\reg, _ritualRandomResume@PAGEOFF\n  ldr \\reg, [\\reg, #(\\index * 8)]\n"
    ".endm\n"
    ".globl _ritualRandomStubs\n_ritualRandomStubs:\n"
    "_ritualRandomStub1a:\n  RX_SAVE\n  mov w0, w21\n  mov w1, w19\n  bl _ritualRandomDraw\n  mov w17, w0\n  RX_RESTORE\n"
    "  mov w22, w17\n  RX_RESUME x16, 0\n  br x16\n"
    "_ritualRandomStub1b:\n  RX_SAVE\n  mov w0, #-1\n  mov w1, w19\n  bl _ritualRandomDraw\n  mov w17, w0\n  RX_RESTORE\n"
    "  mov w22, w17\n  RX_RESUME x16, 1\n  br x16\n"
    "_ritualRandomStub2a:\n  RX_SAVE\n  mov w0, w1\n  mov w1, #-1\n  bl _ritualRandomDraw\n  mov w17, w0\n  RX_RESTORE\n"
    "  mov w0, w17\n  RX_RESUME x16, 2\n  br x16\n"
    "_ritualRandomStub2b:\n  RX_SAVE\n  adrp x16, _ritualRandomIndex@PAGE\n  ldr x16, [x16, _ritualRandomIndex@PAGEOFF]\n  str w8, [x16]\n"
    "  mov w0, #-1\n  mov w1, #-1\n  bl _ritualRandomDraw\n  mov w17, w0\n  RX_RESTORE\n"
    "  adrp x16, _ritualRandomIndex@PAGE\n  ldr x16, [x16, _ritualRandomIndex@PAGEOFF]\n  ldr w8, [x16]\n"
    "  mov w0, w17\n  mov x7, #0\n  RX_RESUME x16, 3\n  br x16\n"
    "_ritualRandomStub3:\n  RX_SAVE\n  mov w0, w21\n  mov w1, w19\n  bl _ritualRandomDraw\n  mov w17, w0\n  RX_RESTORE\n"
    "  sxtw x28, w17\n  mov w16, #10000\n  RX_RESUME x17, 4\n  br x17\n");
extern const unsigned char ritualRandomStubs[];
extern const unsigned char ritualRandomStub1a[] __asm__("_ritualRandomStub1a");
extern const unsigned char ritualRandomStub1b[] __asm__("_ritualRandomStub1b");
extern const unsigned char ritualRandomStub2a[] __asm__("_ritualRandomStub2a");
extern const unsigned char ritualRandomStub2b[] __asm__("_ritualRandomStub2b");
extern const unsigned char ritualRandomStub3[] __asm__("_ritualRandomStub3");

// Logs a read or write of the old table before the game stops on it.
static struct sigaction previousSegv, previousBus;
static void logOldRitualTableAccess(int signal, siginfo_t *info, void *context) {
    const unsigned char *target = info->si_addr;
    if (target >= oldTableStart && target < oldTableEnd) {
        const ucontext_t *uc = context;
        const unsigned char *at = (const unsigned char *)__darwin_arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
        ptrdiff_t fromTable = target - (gameBase + RVA_RITUAL_TABLE);
        writeLog("ritual_old_table_access", "instruction=%p instruction_va=0x%llx slot=%d field=0x%x", (const void *)at,
                 (unsigned long long)(at - gameBase) + 0x100000000ULL, (int)(fromTable / RITUAL_SIZE), (unsigned)(fromTable % RITUAL_SIZE));
    }
    sigaction(signal, signal == SIGBUS ? &previousBus : &previousSegv, NULL);
}

// The changes to the game's code, queued while checking and written only once all are checked.
#define MAX_RITUAL_WRITES (RITUAL_MAC_TABLE_SITE_COUNT + RITUAL_MAC_CAST_SITE_COUNT + 2 * RITUAL_MAC_NAMED_SITE_COUNT + 32)
static struct { unsigned char *at; uint32_t word, old; } ritualWrites[MAX_RITUAL_WRITES];
static int ritualWriteCount, ritualWritesOverflowed;

static void queueWord(unsigned char *at, uint32_t word) {
    if (ritualWriteCount >= MAX_RITUAL_WRITES) {
        ritualWritesOverflowed = 1;
        return;
    }
    ritualWrites[ritualWriteCount].at = at;
    ritualWrites[ritualWriteCount].word = word;
    memcpy(&ritualWrites[ritualWriteCount++].old, at, 4);
}

static uint32_t gameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

// Queues the game's ADRP at a site moved by `pages` 4 KB pages.
static int queueAdrp(const RitualMacSite *s, long long pages, const char *group) {
    uint32_t word = gameWord(s->rva);
    long long imm = (long long)((word >> 5) & 0x7ffff) << 2 | (word >> 29 & 3);
    if (imm & 0x100000) imm -= 0x200000;
    imm += pages;
    if (imm < -0x100000 || imm > 0xfffff) {
        writeLog("ritual_expansion_site_out_of_reach", "group=%s va=0x%llx", group, s->rva + 0x100000000ULL);
        return 0;
    }
    queueWord(gameBase + s->rva, (word & 0x9f00001fu) | (uint32_t)(imm & 3) << 29 | (uint32_t)((imm >> 2) & 0x7ffff) << 5);
    return 1;
}

// The named site at `rva`, or NULL when ritual_expansion_sites_mac.h doesn't list it.
static const RitualMacNamedSite *namedSite(unsigned int rva) {
    for (int i = 0; i < RITUAL_MAC_NAMED_SITE_COUNT; i++)
        if (RITUAL_MAC_NAMED_SITES[i].rva == rva) return &RITUAL_MAC_NAMED_SITES[i];
    writeLog("ritual_expansion_site_missing", "va=0x%llx", rva + 0x100000000ULL);
    return NULL;
}

// A compare's 12-bit immediate set to `value`.
static int queueImm12(unsigned int rva, unsigned int value) {
    if (!namedSite(rva)) return 0;
    uint32_t shift = 0;
    if (value > 0xfff && !(value & 0xfff)) value >>= 12, shift = 1u << 22;
    if (value > 0xfff) {
        writeLog("ritual_expansion_bad_value", "va=0x%llx value=%u", rva + 0x100000000ULL, value);
        return 0;
    }
    queueWord(gameBase + rva, patchedWord(gameWord(rva), 0x1fffu << 10, value << 10 | shift));
    return 1;
}

// A movz or movn's 16-bit immediate set to `value`, and for a size pair its movk to `high`.
static int queueMovWide(unsigned int rva, unsigned int value, unsigned int high) {
    const RitualMacNamedSite *s = namedSite(rva);
    if (!s) return 0;
    queueWord(gameBase + rva, patchedWord(gameWord(rva), 0xffffu << 5, (value & 0xffff) << 5));
    if (s->pair) queueWord(gameBase + rva + 4, patchedWord(gameWord(rva + 4), 0xffffu << 5, (high & 0xffff) << 5));
    return 1;
}

// A `b` (op 0x14000000) or `bl` (0x94000000) at `at` to `to`.
static uint32_t branchWord(uint32_t op, const unsigned char *at, const unsigned char *to) {
    return op | ((uint32_t)((to - at) >> 2) & 0x3ffffffu);
}

// Writes `words` into the header padding and queues a `b` to it from the named site at `rva`.
static unsigned char *queueBranchToCode(unsigned int rva, uint32_t *words, int count, int back, const char *name) {
    if (!namedSite(rva)) return NULL;
    size_t size = ((size_t)count * 4 + 15) & ~(size_t)15;
    unsigned char *code = takePadding(size);
    if (!code) {
        writeLog("ritual_expansion_no_padding", "name=%s", name);
        return NULL;
    }
    if (back >= 0) words[back] = branchWord(0x14000000u, code + 4 * back, gameBase + rva + 4);
    if (!writeGameCode(code, words, (size_t)count * 4)) {
        writeLog("ritual_expansion_write_failed", "name=%s", name);
        return NULL;
    }
    queueWord(gameBase + rva, branchWord(0x14000000u, gameBase + rva, code));
    return code;
}

// The code a compare branches to when its value doesn't fit the 12-bit field.
static void ritualCompareStubWords(uint32_t compare, unsigned int value, unsigned int reg, uint32_t words[4]) {
    words[0] = (compare & 0x80000000u) | 0x52800000u | (value & 0xffffu) << 5 | reg;
    words[1] = (compare & 0xe00003ffu) | 0x0b000000u | reg << 16;
    words[2] = 0;
    words[3] = 0xd503201fu;
}

// The Mac bounds, each a compare with an immediate set to `value`.
typedef struct { unsigned int rva; unsigned int value; int reg; const char *name; } RitualMacBound;
static const RitualMacBound RITUAL_MAC_BOUNDS[] = {
    {RVA_INIT_RITUALS_BOUND,             RITUAL_TABLE_SLOTS,       12, "init_rituals"},
    {RVA_INIT_RITUALS_BOUND_RESET_MODS,  RITUAL_TABLE_SLOTS,       12, "init_rituals_reset_mods"},
    {RVA_INIT_RITUALS_BOUND_3,           RITUAL_TABLE_SLOTS,       12, "init_rituals_3"},
    {RVA_INIT_RITUALS_BOUND_RESET_STATE, RITUAL_TABLE_SLOTS,       12, "init_rituals_reset_state"},
    {RVA_ALLOC_RITUAL_BOUND,             LAST_MOD_RITUAL_SLOT - 1, 9,  "alloc_ritual"},
    {RVA_RITUAL_ADD_FX_BOUND,            LAST_MOD_RITUAL_SLOT,     9,  "add_fx"},
    {RVA_RITUAL_ADD_STRING_BOUND,        LAST_MOD_RITUAL_SLOT,     9,  "add_string"},
    {RVA_EXECUTE_RITUAL_BOUND,           RITUAL_TABLE_SLOTS,       9,  "execute_ritual"},
    {RVA_RITUAL_PR_BOUND,                RITUAL_TABLE_SLOTS,       9,  "pr"},
    {RVA_RITUAL_CPR_BOUND,               RITUAL_TABLE_SLOTS,       9,  "cpr"},
    {RVA_RITUAL_SCREEN_LOOP_1,           RITUAL_TABLE_SLOTS,       9,  "screen_loop_1"},
    {RVA_RITUAL_SCREEN_LOOP_2,           RITUAL_TABLE_SLOTS,       8,  "screen_loop_2"},
    {RVA_RITUAL_CLICK_CASTABLE_BOUND,    LAST_MOD_RITUAL_SLOT,     9,  "click_castable"},
    {RVA_RITUAL_RIGHT_CLICK_BAND,        RITUAL_ROW_GREYED_BASE,   -1, "right_click_band"},
};
#define RITUAL_MAC_BOUND_COUNT ((int)(sizeof RITUAL_MAC_BOUNDS / sizeof RITUAL_MAC_BOUNDS[0]))

// Whether a compare's 12-bit immediate holds `value`, shifted or not.
static int fitsCompareImmediate(unsigned int value) { return value <= 0xfff || (!(value & 0xfff) && value >> 12 <= 0xfff); }

// Queues one bound: in place, or a branch to its stub.
static int queueRitualMacBound(const RitualMacBound *b) {
    if (fitsCompareImmediate(b->value)) return queueImm12(b->rva, b->value);
    if (!namedSite(b->rva)) return 0;
    if (b->reg < 0 || b->value > 0xffff) {
        writeLog("ritual_expansion_bad_value", "va=0x%llx value=%u name=%s", b->rva + 0x100000000ULL, b->value, b->name);
        return 0;
    }
    uint32_t words[4];
    ritualCompareStubWords(gameWord(b->rva), b->value, (unsigned int)b->reg, words);
    return queueBranchToCode(b->rva, words, 4, 2, b->name) != NULL;
}

// Checks that the `bl` at `callRva` calls `targetRva`, and queues its redirect.
static int queueCall(unsigned int callRva, unsigned int targetRva, void *hook, const char *name) {
    if (blTarget(gameBase + callRva) != gameBase + targetRva) {
        writeLog("ritual_expansion_site_mismatch", "group=call name=%s va=0x%llx", name, callRva + 0x100000000ULL);
        return 0;
    }
    unsigned char *trampoline = newTrampoline(hook);
    if (!trampoline) {
        writeLog("ritual_expansion_no_padding", "name=%s", name);
        return 0;
    }
    queueWord(gameBase + callRva, blTo(gameBase + callRva, trampoline));
    return 1;
}

// The first address past the game's image: its segments' end, less __PAGEZERO.
static uintptr_t imageEnd(void) {
    const struct mach_header_64 *mh = (const struct mach_header_64 *)gameBase;
    const struct load_command *lc = (const struct load_command *)(mh + 1);
    uint64_t end = 0;
    for (uint32_t i = 0; i < mh->ncmds; i++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
        if (lc->cmd != LC_SEGMENT_64) continue;
        const struct segment_command_64 *seg = (const struct segment_command_64 *)lc;
        if (seg->vmaddr >= 0x100000000ULL && seg->vmaddr + seg->vmsize > end) end = seg->vmaddr + seg->vmsize;
    }
    return (uintptr_t)gameBase + (uintptr_t)(end - 0x100000000ULL);
}

// Allocates `size` bytes past the game's image, within reach of its ADRPs.
static unsigned char *allocAfterImage(size_t size) {
    uintptr_t at = (imageEnd() + 0xffffff) & ~(uintptr_t)0xffffff;
    for (; at + size <= (uintptr_t)gameBase + 0xf0000000; at += 0x1000000) {
        mach_vm_address_t address = at;
        if (mach_vm_allocate(mach_task_self(), &address, size, VM_FLAGS_FIXED) == KERN_SUCCESS) return (unsigned char *)address;
    }
    return NULL;
}

// The three stubs that add the distance to the expansion's cast counts, each with that distance as its literal at +16.
#define CAST_DELTA_CODES 3
static unsigned char *castDeltaCode[CAST_DELTA_CODES];

#define ADRP_PAGE_SHIFT 12
#define ADRP_PAGE_MASK  ((1u << ADRP_PAGE_SHIFT) - 1)

// Checks every instruction the expansion changes, and queues its new words.
static int queueRitualWrites(unsigned char *table, unsigned char *blocks) {
    unsigned char *oldTable = gameBase + RVA_RITUAL_TABLE, *oldBlocks = gameBase + PLAYER_BLOCKS_RVA;
    long long tablePages = (table - oldTable) >> ADRP_PAGE_SHIFT, blockPages = (blocks - oldBlocks) >> ADRP_PAGE_SHIFT;
    int ok = 1;
    ritualWriteCount = 0;
    for (int i = 0; i < RITUAL_MAC_TABLE_SITE_COUNT; i++) ok &= queueAdrp(&RITUAL_MAC_TABLE_SITES[i], tablePages, "table");
    for (int i = 0; i < RITUAL_MAC_CAST_SITE_COUNT; i++) ok &= queueAdrp(&RITUAL_MAC_CAST_SITES[i], blockPages, "cast_counts");
    for (int i = 0; i < RITUAL_MAC_BOUND_COUNT; i++) ok &= queueRitualMacBound(&RITUAL_MAC_BOUNDS[i]);
    unsigned int size = RITUAL_TABLE_SLOTS * RITUAL_SIZE;
    ok &= queueMovWide(RVA_RITUAL_BACKUP_SIZE, size & 0xffff, size >> 16);
    ok &= queueMovWide(RVA_RITUAL_BACKUP_COPY_SIZE, size & 0xffff, size >> 16);
    ok &= queueMovWide(RVA_RITUAL_RESTORE_SIZE, size & 0xffff, size >> 16);
    static const unsigned int GREYED[] = {RVA_RITUAL_ROW_ID_GREYED, RVA_RITUAL_ROW_ID_GREYED_2, RVA_RITUAL_ROW_ID_GREYED_3,
                                          RVA_RITUAL_ROW_ID_GREYED_4, RVA_RITUAL_ROW_ID_GREYED_5, RVA_RITUAL_ROW_ID_GREYED_6,
                                          RVA_RITUAL_ROW_ID_GREYED_7};
    for (size_t i = 0; i < sizeof GREYED / sizeof GREYED[0]; i++) ok &= queueMovWide(GREYED[i], RITUAL_ROW_GREYED_BASE, 0);
    ok &= queueMovWide(RVA_RITUAL_RIGHT_CLICK_GREYED, RITUAL_ROW_GREYED_BASE - 1, 0);

    static const struct { unsigned int entry, resume; const unsigned char *stub; } RANDOM[] = {
        {RVA_RANDOM_RITUAL_1A_ENTRY, RVA_RANDOM_RITUAL_1A_RESUME, ritualRandomStub1a},
        {RVA_RANDOM_RITUAL_1B_ENTRY, RVA_RANDOM_RITUAL_1B_RESUME, ritualRandomStub1b},
        {RVA_RANDOM_RITUAL_2A_ENTRY, RVA_RANDOM_RITUAL_2A_RESUME, ritualRandomStub2a},
        {RVA_RANDOM_RITUAL_2B_ENTRY, RVA_RANDOM_RITUAL_2B_RESUME, ritualRandomStub2b},
        {RVA_RANDOM_RITUAL_3_ENTRY,  RVA_RANDOM_RITUAL_3_RESUME,  ritualRandomStub3},
    };
    for (int i = 0; i < 5; i++) {
        unsigned char *trampoline = namedSite(RANDOM[i].entry) ? newTrampoline((void *)RANDOM[i].stub) : NULL;
        if (!trampoline) {
            writeLog("ritual_expansion_site_failed", "group=random_ritual copy=%d", i);
            ok = 0;
            continue;
        }
        ritualRandomResume[i] = gameBase + RANDOM[i].resume;
        queueWord(gameBase + RANDOM[i].entry, branchWord(0x14000000u, gameBase + RANDOM[i].entry, trampoline));
    }
    ritualRandomIndex = (int *)(gameBase + RVA_RANDOM_INDEX);

    long long castDelta = blocks - oldBlocks;
    uint32_t executeCode[6] = {0x5800008bu, 0x8b0b014au, 0, 0xd503201fu};
    long long executeDelta = castDelta + CAST_COUNTS_OFFSET;
    memcpy(&executeCode[4], &executeDelta, 8);
    castDeltaCode[0] = queueBranchToCode(RVA_EXECUTE_RITUAL_CAST_COUNTS, executeCode, 6, 2, "execute_ritual_cast_counts");
    uint32_t saveCode[6] = {0x58000088u, 0x8b0802e8u, 0x78747908u, 0};
    memcpy(&saveCode[4], &castDelta, 8);
    castDeltaCode[1] = queueBranchToCode(RVA_SAVE_GAME_CAST_COUNT_READ, saveCode, 6, 3, "save_game_cast_counts");
    uint32_t turnCode[6] = {0x58000088u, 0x8b0802c8u, 0x78797908u, 0};
    memcpy(&turnCode[4], &castDelta, 8);
    castDeltaCode[2] = queueBranchToCode(RVA_SAVE_2H_CAST_COUNT_READ, turnCode, 6, 3, "save_2h_cast_counts");
    for (int i = 0; i < CAST_DELTA_CODES; i++) ok &= castDeltaCode[i] != NULL;

    ok &= queueCall(RVA_LOAD_2H_CAST_COUNTS_CALL, RVA_FREAD, (void *)readCastCounts, "load_2h_cast_counts");
    ok &= queueCall(RVA_LOAD_GAME_CAST_COUNTS_CALL, RVA_FREAD, (void *)readCastCounts, "load_game_cast_counts");
    ok &= queueCall(RVA_RESET_PLAYERS_CLEAR, RVA_BZERO, (void *)clearPlayerBlocks, "reset_players_clear");
    ok &= queueCall(RVA_KEY_CHECK_PLAYERS_CLEAR, RVA_BZERO, (void *)clearPlayerBlocks, "key_check_players_clear");
    ok &= queueCall(RVA_COMPACT_PLAYERS_MOVE, RVA_MEMCPY, (void *)movePlayerBlock, "compact_players_move");
    ritualOutOfRitualsExit = gameBase + RVA_MOD_COMMAND_EXIT;
    ok &= queueCall(RVA_OUT_OF_RITUALS_CALL, RVA_FATAL_FORMAT, (void *)ritualOutOfRitualsStub, "out_of_rituals");
    if (ritualWritesOverflowed) {
        writeLog("ritual_expansion_too_many_writes", "max=%d", MAX_RITUAL_WRITES);
        ok = 0;
    }
    return ok;
}

// Makes the old table's whole 16 KB pages unreadable.
static void protectOldRitualTable(unsigned char *oldTable) {
    oldTableStart = (unsigned char *)(((uintptr_t)oldTable + MAC_PAGE_MASK) & ~(uintptr_t)MAC_PAGE_MASK);
    oldTableEnd = (unsigned char *)(((uintptr_t)oldTable + (size_t)VANILLA_RITUAL_SLOTS * RITUAL_SIZE) & ~(uintptr_t)MAC_PAGE_MASK);
    kern_return_t error = mach_vm_protect(mach_task_self(), (mach_vm_address_t)oldTableStart, (mach_vm_size_t)(oldTableEnd - oldTableStart),
                                          FALSE, VM_PROT_NONE);
    if (!error) {
        struct sigaction action;
        memset(&action, 0, sizeof action);
        action.sa_sigaction = logOldRitualTableAccess;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(SIGSEGV, &action, &previousSegv);
        sigaction(SIGBUS, &action, &previousBus);
    }
    writeLog("ritual_old_table_protected", "ok=%d from=%p to=%p error=%d", !error, (void *)oldTableStart, (void *)oldTableEnd, error);
}

// Moves the table and turns every part of the expansion on, or changes nothing.
static void adjustSaveCastCountRead(void);
static int installRitualExpansion(void) {
    if (!gameCodeMatches) {
        writeLog("ritual_expansion_off", "reason=game_code_differs queued=0");
        return 0;
    }
    unsigned char *oldTable = gameBase + RVA_RITUAL_TABLE, *oldBlocks = gameBase + PLAYER_BLOCKS_RVA;
    size_t tableSize = (size_t)RITUAL_SLOT_CAP * RITUAL_SIZE;
    size_t tableAt = (uintptr_t)oldTable & ADRP_PAGE_MASK;
    size_t blocksAt = ((tableAt + tableSize + ADRP_PAGE_MASK) & ~(size_t)ADRP_PAGE_MASK) + ((uintptr_t)oldBlocks & ADRP_PAGE_MASK);
    size_t total = (blocksAt + CAST_COUNTS_OFFSET + CAST_COUNTS_BYTES + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    unsigned char *memory = allocAfterImage(total);
    if (!memory) {
        writeLog("ritual_expansion_off", "reason=no_memory_near_game bytes=%zu", total);
        return 0;
    }
    writeLog("ritual_table_allocated", "address=%p va=0x%llx slots=%d bytes=%zu", (void *)memory,
             (unsigned long long)(memory - gameBase) + 0x100000000ULL, RITUAL_SLOT_CAP, total);
    unsigned char *table = memory + tableAt, *blocks = memory + blocksAt;
    if (!queueRitualWrites(table, blocks)) {
        writeLog("ritual_expansion_off", "reason=game_build_differs queued=%d", ritualWriteCount);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    writeLog("ritual_expansion_checked", "table_sites=%d cast_sites=%d named=%d writes=%d", RITUAL_MAC_TABLE_SITE_COUNT,
             RITUAL_MAC_CAST_SITE_COUNT, RITUAL_MAC_NAMED_SITE_COUNT, ritualWriteCount);

    ritualTable = table;
    castCounts = blocks + CAST_COUNTS_OFFSET;
    memcpy(ritualTable, oldTable, (size_t)VANILLA_RITUAL_SLOTS * RITUAL_SIZE);
    for (int p = 0; p < PLAYER_SLOTS; p++)
        memcpy(castCounts + (size_t)p * PLAYER_BLOCK_SIZE, gameBase + RVA_CAST_COUNTS + (size_t)p * PLAYER_BLOCK_SIZE,
               VANILLA_RITUAL_SLOTS * sizeof(short));
    for (int i = 0; i < ritualWriteCount; i++) {
        if (writeGameCode(ritualWrites[i].at, &ritualWrites[i].word, 4)) continue;
        writeLog("ritual_expansion_write_failed", "va=0x%llx", (unsigned long long)(ritualWrites[i].at - gameBase) + 0x100000000ULL);
        for (int j = i; j >= 0; j--) writeGameCode(ritualWrites[j].at, &ritualWrites[j].old, 4);
        writeLog("ritual_expansion_off", "reason=code_unwritable writes=%d", ritualWriteCount);
        ritualTable = NULL;
        castCounts = NULL;
        return 0;
    }
    ritualExpansionActive = 1;
    adjustSaveCastCountRead();
    writeLog("ritual_table_moved", "from=%p to=%p slots=%d mod_slots=%d-%d cast_counts=%p", (void *)oldTable,
             (void *)ritualTable, RITUAL_SLOT_CAP, FIRST_MOD_RITUAL_SLOT, LAST_MOD_RITUAL_SLOT, (void *)castCounts);
    protectOldRitualTable(oldTable);
    return 1;
}
#endif

// Registers the .c5m line and moves the table.
static void ritualExpansionInit(void) {
    int word = api->addModCommand("expand_ritual_table", expandRitualTableLine);
#ifndef __APPLE__
    ritualStubs = allocNear(gameBase + RVA_TEXT_SECTION);
#endif
    int moved = installRitualExpansion();
    writeLog("ritual_expansion_installed", "word=%d moved=%d", word, moved);
}
