// Mods' factions: their slots past the game's 32 and what the game keeps per square for them.

#define FACTION_SQUARES (MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES)

// A square's record in the game's square table.
static unsigned char *squareRecord(int x, int y) {
    return GAME_PTR(RVA_SQUARE_RECORDS) + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
}

// A slot's controller, from the moved table once moved.
static short playerSlotController(int player) {
    short c;
    memcpy(&c, (playerTableMoved ? movedPlayerTables[PLAYER_GROUP_CONTROLLER] : GAME_PTR(RVA_SLOT_CONTROLLER)) + 2 * player, 2);
    return c;
}

// A faction slot's per-square arrays, allocated with the player table move.
static unsigned char *factionSquareSeen, *factionSquareSeenMark, *factionSquareTrueSeen;
static int *factionSquareAiStrength;

// 1 for a faction's slot.
static int factionSlot(int player) { return playerTableMoved && player >= PLAYER_SLOTS_GAME && player < PLAYER_SLOTS_MOVED; }

// A faction slot's entry in one of the side arrays, for square (x, y).
static size_t factionSquareIndex(int x, int y, int player) {
    return ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * FACTION_SLOT_TARGET + (size_t)(player - PLAYER_SLOTS_GAME);
}

// Allocates the side arrays (zeroed, and AI memory -1, as RVA_RESET_SQUARES leaves the game's); 1 when done.
static int allocFactionSquareArrays(void) {
    size_t entries = (size_t)FACTION_SQUARES * FACTION_SLOT_TARGET;
    factionSquareSeen = calloc(entries, 1);
    factionSquareSeenMark = calloc(entries, 1);
    factionSquareTrueSeen = calloc(entries, 1);
    factionSquareAiStrength = malloc(entries * sizeof(int));
    if (!factionSquareSeen || !factionSquareSeenMark || !factionSquareTrueSeen || !factionSquareAiStrength) {
        free(factionSquareSeen), free(factionSquareSeenMark), free(factionSquareTrueSeen), free(factionSquareAiStrength);
        factionSquareSeen = factionSquareSeenMark = factionSquareTrueSeen = NULL, factionSquareAiStrength = NULL;
        writeLog("faction_squares_not_allocated", "reason=no_memory bytes=%zu", entries * 7);
        return 0;
    }
    memset(factionSquareAiStrength, 0xff, entries * sizeof(int));
    memset(factionSquareSeenMark, 0xff, entries);
    return 1;
}

// ---- RVA_AI_REVEAL_PORTAL_EXITS ------------------------------------------------------------------
// The game's portal reveal, for a faction slot: each portal square in view reveals every square of its portal.
typedef int (*RevealPortalExits)(int player);

static int factionRevealPortalExits(int player) {
    short controller = playerSlotController(player);
    if (controller != CONTROLLER_AI && controller != CONTROLLER_INDEPENDENT) return 0;
    int w = G32(RVA_MAP_DEPTH), h = G32(RVA_MAP_H), revealed = 0;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            short portal;
            memcpy(&portal, squareRecord(x, y) + SQUARE_PORTAL, 2);
            if (factionSquareSeen[factionSquareIndex(x, y, player)] <= 1 || portal <= 0) continue;
            for (int x2 = 0; x2 < w; x2++)
                for (int y2 = 0; y2 < h; y2++) {
                    short other;
                    memcpy(&other, squareRecord(x2, y2) + SQUARE_PORTAL, 2);
                    unsigned char *seen = factionSquareSeen + factionSquareIndex(x2, y2, player);
                    if (abs(other) == portal && (signed char)*seen < 1) *seen = 1, revealed++;
                }
        }
    return revealed;
}

static RevealPortalExits gameRevealPortalExits;
static int revealPortalExitsHook(int player) {
    if (factionSlot(player)) return factionRevealPortalExits(player);
    return gameRevealPortalExits ? gameRevealPortalExits(player) : GAME_FN(RevealPortalExits, RVA_AI_REVEAL_PORTAL_EXITS)(player);
}

// ---- RVA_REBUILD_SQUARE_ARMY --------------------------------------------------------------------
// The game's square army recount, for a game with factions: the same steps with a count per slot up to 48.
int fanpatchHookCreate(void *target, void *detour, void **original);
#ifndef __APPLE__
static int installFactionSightHooks(void);
#define FACTION_SIGHT_HOOK_COUNT 8
static int installFactionStubs(void);
#define FACTION_STUB_COUNT 18
static int installFactionSlotHooks(void);
#define FACTION_SLOT_HOOK_COUNT 3
static int installFactionMovers(void);
#else
static int installFactionSightHooks(void);
#define FACTION_SIGHT_HOOK_COUNT 4
static int installFactionSlotHooks(void);
#define FACTION_SLOT_HOOK_COUNT 2
static int installMacFactionSites(void);
static int installFactionMovers(void);
static int macRedirectCalls(unsigned int rva, void *hook, const char *name);
#endif
int fanpatchHookEnable(void *target, int enable);
static unsigned char *monsterRecord(int monster);
typedef void (*RebuildSquareArmyFn)(int x, int y);
static RebuildSquareArmyFn gameRebuildSquareArmy;

// RVA_UNIT_HOLDS_SQUARE: whether a unit counts toward its square's army.
static int unitHoldsSquare(int unit) {
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return 0;
    const unsigned char *monster = monsterRecord((short)G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE));
    unsigned long long flags = 0;
    if (monster) memcpy(&flags, monster + (RVA_MONSTER_FLAGS - RVA_MONSTER_TABLE), sizeof flags);
    return !(flags & MONSTER_FLAG_NOT_ARMY) && (short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE) != PLAYER_SLOTS_GAME - 1;
}

// RVA_SAME_TEAM with the moved slot count: both slots in range, and the same slot or the same team above 0.
static int playerSameTeam(int a, int b) {
    if (a < 0 || a >= playerSlotCount() || b < 0 || b >= playerSlotCount()) return 0;
    if (a == b) return 1;
    short teamA, teamB;
    memcpy(&teamA, playerSlotAddress(RVA_SLOT_TEAM, a, PLAYER_BLOCK_SIZE), 2);
    memcpy(&teamB, playerSlotAddress(RVA_SLOT_TEAM, b, PLAYER_BLOCK_SIZE), 2);
    return teamA > 0 && teamA == teamB;
}

// Whether a game uses a faction slot now: one's controller isn't removed.
static int factionSlotsInUse(void) {
    if (!playerTableMoved) return 0;
    for (int p = PLAYER_SLOTS_GAME; p < PLAYER_SLOTS_MOVED; p++)
        if (playerSlotController(p) != CONTROLLER_REMOVED) return 1;
    return 0;
}

static void factionRebuildSquareArmy(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_W) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    unsigned char *record = GAME_PTR(RVA_SQUARE_RECORDS) + square * SQUARE_SIZE;
    short *holder = (short *)(record + SQUARE_HOLDER), *armyOwner = (short *)(record + SQUARE_ARMY_OWNER);
    short *army = (short *)GAME_PTR(RVA_SQUARE_ARMY) + square;
    *holder = -1;
    int counts[PLAYER_SLOTS_MOVED] = {0}, units = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        short owner = (short)G16(RVA_UNIT_PLAYER + at);
        if (G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y || (short)G16(RVA_UNIT_HP + at) <= 0 || owner == PLAYER_SLOTS_GAME - 1 || owner < 0) continue;
        if (unitHoldsSquare(u) && owner < PLAYER_SLOTS_MOVED) counts[owner]++;
        units++;
    }
    if (!units) {
        *army = 0, *armyOwner = -1;
        return;
    }
    *army = (short)units;
    int best = 0, bestSlot = -1, second = 0, secondSlot = -1;
    for (int p = 0; p < PLAYER_SLOTS_MOVED; p++) {
        if (counts[p] > best) second = best, secondSlot = bestSlot, best = counts[p], bestSlot = p;
        else if (counts[p] > second) second = counts[p], secondSlot = p;
    }
    if (*armyOwner < 0 || *armyOwner >= PLAYER_SLOTS_MOVED || !counts[*armyOwner]) *armyOwner = (short)bestSlot;
    if (secondSlot < 0) return;
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        short owner = (short)G16(RVA_UNIT_PLAYER + at);
        if (G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y || (short)G16(RVA_UNIT_HP + at) <= 0 || owner == PLAYER_SLOTS_GAME - 1 || !unitHoldsSquare(u)) continue;
        if (!playerSameTeam(owner, *armyOwner)) {
            *army = -1, *holder = owner;
            return;
        }
    }
}

// ---- RVA_SQUARE_STRONGEST_OWNER -----------------------------------------------------------------
// The owner with the most max hp on a square, for a game with factions: the same sums per slot up to 48.
typedef int (*SquareStrongestOwnerFn)(int x, int y);
static SquareStrongestOwnerFn gameSquareStrongestOwner;

static int factionSquareStrongestOwner(int x, int y) {
    int sums[PLAYER_SLOTS_MOVED] = {0}, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; x >= 0 && u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        short owner = (short)G16(RVA_UNIT_PLAYER + at);
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y || owner == PLAYER_SLOTS_GAME - 1 || owner < 0 ||
            owner >= PLAYER_SLOTS_MOVED)
            continue;
        const unsigned char *monster = monsterRecord((short)G16(RVA_UNIT_MONSTER + at));
        unsigned long long flags = 0;
        if (monster) memcpy(&flags, monster + (RVA_MONSTER_FLAGS - RVA_MONSTER_TABLE), sizeof flags);
        if (!(flags & MONSTER_FLAG_NOT_ARMY)) sums[owner] += GAME_FN(int (*)(int), RVA_UNIT_MAX_HP)(u);
    }
    int best = 0, bestSlot = -1;
    for (int p = 0; p < PLAYER_SLOTS_MOVED; p++)
        if (sums[p] > best) best = sums[p], bestSlot = p;
    return bestSlot;
}

__attribute__((unused)) static int squareStrongestOwnerHook(int x, int y) {
    return factionSlotsInUse() ? factionSquareStrongestOwner(x, y) : gameSquareStrongestOwner(x, y);
}

static void rebuildSquareArmyHook(int x, int y) {
    if (factionSlotsInUse()) factionRebuildSquareArmy(x, y);
    else if (gameRebuildSquareArmy) gameRebuildSquareArmy(x, y);
    else GAME_FN(RebuildSquareArmyFn, RVA_REBUILD_SQUARE_ARMY)(x, y);
}

// ---- Installing ----------------------------------------------------------------------------------
// Points the game's per-square slot reads at the faction paths; 1 when every one is in.
static int installFactionPaths(void) {
#ifndef __APPLE__
    if (!allocFactionSquareArrays()) return 0;
    void *reveal = gameBase + RVA_AI_REVEAL_PORTAL_EXITS;
    int ok = fanpatchHookCreate(reveal, (void *)revealPortalExitsHook, (void **)&gameRevealPortalExits) && fanpatchHookEnable(reveal, 1);
    void *rebuild = gameBase + RVA_REBUILD_SQUARE_ARMY;
    ok += fanpatchHookCreate(rebuild, (void *)rebuildSquareArmyHook, (void **)&gameRebuildSquareArmy) && fanpatchHookEnable(rebuild, 1);
    void *strongest = gameBase + RVA_SQUARE_STRONGEST_OWNER;
    ok += fanpatchHookCreate(strongest, (void *)squareStrongestOwnerHook, (void **)&gameSquareStrongestOwner) && fanpatchHookEnable(strongest, 1);
    ok += installFactionSightHooks();
    ok += installFactionSlotHooks();
    ok += installFactionStubs();
    ok += installFactionMovers();
    int paths = 4 + FACTION_SIGHT_HOOK_COUNT + FACTION_SLOT_HOOK_COUNT + FACTION_STUB_COUNT;
    writeLog("faction_paths_installed", "ok=%d of %d", ok, paths);
    return ok == paths;
#else
    if (!allocFactionSquareArrays()) return 0;
    gameRebuildSquareArmy = GAME_FN(RebuildSquareArmyFn, RVA_REBUILD_SQUARE_ARMY);
    gameSquareStrongestOwner = GAME_FN(SquareStrongestOwnerFn, RVA_SQUARE_STRONGEST_OWNER);
    int ok = (macRedirectCalls(RVA_AI_REVEAL_PORTAL_EXITS, (void *)revealPortalExitsHook, "ai_reveal_portal_exits") > 0) +
             (macRedirectCalls(RVA_REBUILD_SQUARE_ARMY, (void *)rebuildSquareArmyHook, "rebuild_square_army") > 0) +
             (macRedirectCalls(RVA_SQUARE_STRONGEST_OWNER, (void *)squareStrongestOwnerHook, "square_strongest_owner") > 0);
    ok += installFactionSightHooks();
    ok += installFactionSlotHooks();
    ok += installMacFactionSites();
    ok += installFactionMovers();
    int paths = 3 + FACTION_SIGHT_HOOK_COUNT + FACTION_SLOT_HOOK_COUNT + 2;
    writeLog("faction_paths_installed", "ok=%d of %d", ok, paths);
    return ok == paths;
#endif
}

// Gets the game ready for factions once per launch: the player table move, then the faction paths.
static int factionSlotsTried, factionSlotsReady;

static int ensureFactionSlots(void) {
    if (factionSlotsTried) return factionSlotsReady;
    factionSlotsTried = 1;
    factionSlotsReady = ensurePlayerTableMoved() && installFactionPaths();
    if (playerTableMoved) refreshPluginPlayerAddresses();
    return factionSlotsReady;
}
