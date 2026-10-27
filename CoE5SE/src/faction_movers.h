// Mods' factions' turns: the vanilla independents' movers for a faction slot, by its preset.
#pragma once

// ---- The game's helpers the movers call ----------------------------------------------------------
// @platform-split mover_helpers: Mac inlines some of the movers' helpers, so CoE5SE's copies run there, and Mac's sitepop takes a flag
#ifndef __APPLE__
static int moverCountTerrainAround(int x, int y, int radius, int terrain) { return GAME_FN(int (*)(int, int, int, int), RVA_COUNT_TERRAIN_AROUND)(x, y, radius, terrain); }
static int moverFollowersNamed(int commander, const char *name) { return GAME_FN(int (*)(int, const char *), RVA_COMMANDER_FOLLOWERS_NAMED)(commander, name); }
static int moverReleaseFollowers(int commander, const char *name, int most) { return GAME_FN(int (*)(int, const char *, int), RVA_COMMANDER_RELEASE_FOLLOWERS)(commander, name, most); }
static const char *moverTraceName(int unit) { return GAME_FN(const char *(*)(int), RVA_UNIT_TRACE_NAME)(unit); }
static int moverPortalsLinkedTo(int plane, int kind) { return GAME_FN(int (*)(int, int), RVA_PORTALS_LINKED_TO)(plane, kind); }
static int moverUnitCommander(int unit) { return GAME_FN(int (*)(int), RVA_UNIT_COMMANDER_INDEX)(unit); }
static void moverSitepop(int slot, int x, int y, const char *group) { GAME_FN(void (*)(int, int, int, const char *), RVA_SITEPOP)(slot, x, y, group); }
#else
// RVA_COUNT_TERRAIN_AROUND: the squares of the (2 radius + 1) box holding that terrain number, off the map as -1.
static int moverCountTerrainAround(int x, int y, int radius, int terrain) {
    int n = 0, mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int ty = y - radius; ty <= y + radius; ty++)
        for (int tx = x - radius; tx <= x + radius; tx++) {
            int here = tx < 0 || ty < 0 || tx >= mapW || ty >= mapH ? -1 : sightTerrain(tx, ty);
            n += here == terrain;
        }
    return n;
}
// The monster record name of a unit, or NULL for a unit with none.
static const char *unitMonsterName(int unit) {
    short monster = (short)G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE);
    return monster >= 0 ? (const char *)monsterRecord(monster) : NULL;
}
// RVA_COMMANDER_FOLLOWERS_NAMED: the living units whose leader is the commander, of that monster name.
static int moverFollowersNamed(int commander, const char *name) {
    int n = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        const char *own = unitMonsterName(u);
        n += (short)G16(RVA_UNIT_HP + at) > 0 && (int)G32(RVA_UNIT_LEADER + at) == commander && own && !strcmp(own, name);
    }
    return n;
}
// RVA_COMMANDER_RELEASE_FOLLOWERS: up to `most` of them get no leader (-1), in unit order.
static int moverReleaseFollowers(int commander, const char *name, int most) {
    int n = 0, last = G32(RVA_UNIT_COUNT);
    if (most < 1) return 0;
    for (int u = 0; u <= last; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        const char *own = unitMonsterName(u);
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || (int)G32(RVA_UNIT_LEADER + at) != commander || !own || strcmp(own, name)) continue;
        *(int *)GAME_PTR(RVA_UNIT_LEADER + at) = -1;
        if (++n >= most) break;
    }
    return n;
}
// RVA_UNIT_TRACE_NAME's text for the carrier traces: the monster's name (the trace lines are the game's log only).
static const char *moverTraceName(int unit) {
    const char *own = unit >= 0 ? unitMonsterName(unit) : NULL;
    return own ? own : "?";
}
static short squarePortalAt(int x, int y) { return *(short *)(sightSquare(x, y) + SQUARE_PORTAL); }
// 0x242900: whether a square of plane `kind` (-1 any) holds portal number `portal` (either sign).
static int planeHasPortalNumber(int portal, int kind) {
    int x0 = 0, y0 = 0, x1 = G32(RVA_MAP_DEPTH) - 1, y1 = G32(RVA_MAP_H) - 1;
    if (kind >= 0) x0 = planeField(kind, PLANE_X), y0 = planeField(kind, PLANE_Y), x1 = x0 + planeField(kind, PLANE_W) - 1, y1 = y0 + planeField(kind, PLANE_H) - 1;
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            if (abs(squarePortalAt(x, y)) == portal) return 1;
    return 0;
}
// The squares of a plane with a portal linked to plane `kind`.
static int moverPortalsLinkedTo(int plane, int kind) {
    if (plane == -2) return moverPortalsLinkedTo(0, kind) + moverPortalsLinkedTo(3, kind) + moverPortalsLinkedTo(2, kind);
    int x0 = 0, y0 = 0, x1 = G32(RVA_MAP_DEPTH) - 1, y1 = G32(RVA_MAP_H) - 1, n = 0;
    if (plane >= 0) x0 = planeField(plane, PLANE_X), y0 = planeField(plane, PLANE_Y), x1 = x0 + planeField(plane, PLANE_W) - 1, y1 = y0 + planeField(plane, PLANE_H) - 1;
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) {
            short portal = squarePortalAt(x, y);
            if (portal > 0 && (kind == -1 || planeHasPortalNumber(portal, kind))) n++;
        }
    return n;
}
// RVA_UNIT_COMMANDER_INDEX: the commander whose unit it is, -1 none.
static int moverUnitCommander(int unit) {
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last; c++)
        if ((int)G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) == unit) return c;
    return -1;
}
static void moverSitepop(int slot, int x, int y, const char *group) { GAME_FN(void (*)(int, int, int, int, const char *), RVA_SITEPOP)(slot, x, y, 0, group); }
#endif

// The game's planeshift of a plane's monsters, for a faction.
static void moverPlaneShiftMonsters(int plane, int percent, int each, const char *name) {
#ifndef __APPLE__
    GAME_FN(void (*)(int, int, int, const char *), RVA_PLANE_SPAWN_CHANCE)(plane, percent, each, name);
#else
    int x0 = planeField(plane, PLANE_X), w = planeField(plane, PLANE_W), h = planeField(plane, PLANE_H);
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    for (int y = 0; y < h; y++)
        for (int x = x0; x < x0 + w; x++) {
            int draw = random(100);
            if (draw >= percent || x < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || !G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2))
                continue;
            int shifted = 0, owner = 24, lx = -1, ly = -1, last = G32(RVA_UNIT_COUNT);
            for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
                size_t at = (size_t)u * UNIT_SIZE;
                int ux = (int)G32(RVA_UNIT_X + at), uy = (int)G32(RVA_UNIT_X + at + 4);
                if (GAME_FN(int (*)(int, int), RVA_PLANE_AT)(ux, uy) != plane || (short)G16(RVA_UNIT_HP + at) <= 0 || ux != x || uy != y) continue;
                short monster = (short)G16(RVA_UNIT_MONSTER + at);
                if (monster < 0 || strcmp((const char *)monsterRecord(monster), name) || random(100) >= each) continue;
                int sx = ux, sy = uy;
                GAME_FN(void (*)(int, int *, int *), RVA_PLANE_SAME_SPOT)(0, &sx, &sy);
                GAME_FN(void (*)(const char *, ...), RVA_TRACE)("planeshiftunit %s to %d (%d,%d)\n", moverTraceName(u), 0, sx, sy);
                GAME_FN(void (*)(int, int, int, int, int, int, int), RVA_TELEPORT_ARMY)(u, 0, -1, -1, sx, sy, 1);
                owner = (short)G16(RVA_UNIT_PLAYER + at), lx = (int)G32(RVA_UNIT_X + at), ly = (int)G32(RVA_UNIT_X + at + 4);
                shifted++;
            }
            int again = (shifted * 100 + 50) / 100;
            if (again > 0) GAME_FN(int (*)(int, int, const char *, int, int, int), RVA_POPULATE)(lx, ly, name, again, owner, -1);
        }
#endif
}

#define INFERNO_SPAWN_TRIES       500000
#define INFERNO_TERRAIN_RADIUS    2

// Each faction slot's turn state in this world.
static struct { int active, ownGateway; } factionTurns[FACTION_SLOT_TARGET];

// Each slot's preset slot per behaviour, and its wanderers' gate.
enum { FACTION_AS_LIKE, FACTION_AS_WANDER, FACTION_AS_TITHE, FACTION_AS_EVENTS, FACTION_AS_CONQUEST, FACTION_AS_STATUE, FACTION_AS_COUNT };
static unsigned char factionAsBySlot[FACTION_AS_COUNT][PLAYER_SLOTS_PADDED];
typedef struct { int kind, home, active, pad; } FactionGate;
static FactionGate factionGates[PLAYER_SLOTS_PADDED];
_Static_assert(sizeof(FactionGate) == 16, "the gate stub steps 16 bytes a slot");

// Forgets every faction slot's turn state: a new world or a load.
static void refreshFactionWanderTables(void);
static void resetFactionTurnState(void) {
    memset(factionTurns, 0, sizeof factionTurns);
    refreshFactionWanderTables();
}

// The faction in slot `slot`, or NULL.
static const Faction *factionInSlot(int slot) {
    int i = slot - PLAYER_SLOTS_GAME;
    return i >= 0 && i < factionCount ? &factions[i] : NULL;
}

// A faction's home plane in this world, or -1 (none, or not in this world).
static int factionHomePlane(const Faction *f) {
    if (!f->home[0]) return -1;
    int plane = vanillaPlaneNamed(f->home);
    return plane >= 0 ? plane : customPlaneIndex(f->home);
}

static short squarePortal(int x, int y) { return *(short *)(sightSquare(x, y) + SQUARE_PORTAL); }

// Whether plane `home` holds portal number `number`.
static int planeHoldsPortal(int home, int number) {
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++)
            if (squarePortal(x, y) == number && planeHoldingSquare(x, y) == home) return 1;
    return 0;
}

// The portal number of a faction home's own gateway pair.
static int factionOwnGateway(int slot, int home) {
    int *own = &factionTurns[slot - PLAYER_SLOTS_GAME].ownGateway;
    if (*own) return *own;
    if (home == PLANE_INFERNO) return *own = PORTAL_INFERNO_GATE;
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    int depth = G32(RVA_MAP_DEPTH) < MAP_COLUMNS_MAX ? G32(RVA_MAP_DEPTH) : MAP_COLUMNS_MAX;
    int height = G32(RVA_MAP_H) < MAP_COLUMN_SQUARES ? G32(RVA_MAP_H) : MAP_COLUMN_SQUARES;
    int best = -1;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            int n = squarePortal(x, y);
            if (n <= 0 || (best > 0 && n >= best)) continue;
            for (int ox = 0; ox < depth && n != best; ox++)
                for (int oy = 0; oy < height; oy++) {
                    if (ox >= x0 && ox < x0 + w && oy >= y0 && oy < y0 + h) continue;
                    if (abs(squarePortal(ox, oy)) == n) {
                        best = n;
                        break;
                    }
                }
        }
    return *own = best;
}

// The Inferno's active test, for a faction on its home.
static int factionInfernoActive(int slot, const Faction *f, int home, int *portalsOut) {
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    int portals = 0;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) portals += squarePortal(x, y) > 0;
    int own = factionOwnGateway(slot, home);
    if (own > 0 && planeHoldsPortal(home, own)) portals--;
    *portalsOut = portals;
    if (f->active == FACTION_ACTIVE_ALWAYS) return 1;
    if (f->active == FACTION_ACTIVE_NEVER) return 0;
    if (portals >= f->active) return 1;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            unsigned char *square = sightSquare(x, y);
            if (x < G32(RVA_MAP_W) && y < G32(RVA_MAP_H)) {
                short terrain = sightTerrain(x, y);
                signed char owner = (signed char)square[SQUARE_OWNER];
                if (terrain >= 0 && (sightTerrainFlags(terrain) & 1) && owner >= 0 && owner != slot) return 1;
            }
            if (*(unsigned short *)(square + SQUARE_ARMY_OWNER) < PLAYER_SEATS) return 1;
        }
    return 0;
}

// Removes up to `most` of the slot's living units named `name` at (x, y), in unit order.
static void removeSlotMonstersAt(int slot, int x, int y, const char *name, int most) {
    int last = G32(RVA_UNIT_COUNT), removed = 0;
    for (int u = 0; u <= last; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || (int)G32(RVA_UNIT_X + at) != x || (int)G32(RVA_UNIT_X + at + 4) != y) continue;
        if ((short)G16(RVA_UNIT_PLAYER + at) != slot || strcmp((const char *)monsterRecord((short)G16(RVA_UNIT_MONSTER + at)), name)) continue;
        GAME_FN(void (*)(int), RVA_REMOVE_UNIT)(u);
        if (++removed >= most) break;
    }
}

// One spawn of a faction's turn at (x, y): the first of its spawns that applies, its fuel used up and its group made.
static void factionSpawnAt(int slot, const Faction *f, int x, int y, int fuel, const int terrain[4]) {
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    for (int i = 0; i < f->spawnCount; i++) {
        const FactionSpawn *s = &f->spawns[i];
        if (fuel < s->min || (s->terrain != FACTION_TERRAIN_ANY && terrain[s->terrain] <= 0)) continue;
        if (s->chance < 100 && random(100) >= s->chance) continue;
        if (s->use > 0) removeSlotMonstersAt(slot, x, y, f->fuel, s->use);
        writeLog("faction_spawn", "slot=%d x=%d y=%d fuel=%d line=%d group=%.60s", slot, x, y, fuel, i + 1, s->group);
        moverSitepop(slot, x, y, s->group);
        return;
    }
}

// The citadel site's spawn: a random citadel of the faction's home it owns, with the terrain around it.
static void factionCitadelSpawn(int slot, const Faction *f, int home) {
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    int x, y;
    for (int tries = 0;; tries++) {
        x = random(w) + x0, y = random(h) + y0;
        if (GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH)(TERRAIN_CODE_CITADEL, x, y)) break;
        if (tries + 1 >= INFERNO_SPAWN_TRIES) return;
    }
    if (!squareInMap(x, y) || (signed char)sightSquare(x, y)[SQUARE_OWNER] != slot) return;
    int fuel = GAME_FN(int (*)(int, int, const char *), RVA_COUNT_MONSTERS_AT)(x, y, f->fuel);
    int (*around)(int, int, int, int) = moverCountTerrainAround;
    int ice = around(x, y, INFERNO_TERRAIN_RADIUS, TERRAIN_FROZEN_WASTE), waste = around(x, y, INFERNO_TERRAIN_RADIUS, TERRAIN_INFERNAL_WASTE);
    int magma = around(x, y, INFERNO_TERRAIN_RADIUS, TERRAIN_MAGMA);
    ice = waste <= ice ? ice : 0;
    waste = magma <= waste ? waste : 0;
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("infernospawn at %s, sinners %d (ice %d, waste %d, magma %d)\n",
                                                    GAME_FN(const char *(*)(int, int), RVA_SQUARE_TEXT)(x, y), fuel, ice, waste, magma);
    int terrain[4] = {0};
    terrain[FACTION_TERRAIN_MAGMA] = magma, terrain[FACTION_TERRAIN_ICE] = ice, terrain[FACTION_TERRAIN_WASTE] = waste;
    factionSpawnAt(slot, f, x, y, fuel, terrain);
}

static void factionMonumentSpawns(int slot, const Faction *f);
static int factionMonumentPlane(const Faction *f);
static int factionMonumentCount(const Faction *f, int plane, int owner);
static void factionMonumentSteps(int slot, const Faction *f, int home, int monuments);
// The faction's turn's spawns, by its spawn site: the citadel site while it is active, the monument site's squares.
static void factionTurnSpawn(int slot, const Faction *f, int home, int active) {
    if (f->spawnSite == FACTION_SITE_OWN_CITADEL && active && home >= 0) factionCitadelSpawn(slot, f, home);
    else if (f->spawnSite == FACTION_SITE_MONUMENT) factionMonumentSpawns(slot, f);
}

// RVA_MOVE_INFERNO_PLAYER for a faction: the active test, the armies' move, the spawn.
static void factionInfernoMove(int slot, const Faction *f, void (*moveArmies)(int)) {
    int home = factionHomePlane(f), portals = 0;
    int active = home >= 0 && factionInfernoActive(slot, f, home, &portals);
    factionTurns[slot - PLAYER_SLOTS_GAME].active = active;
    factionGates[slot].home = home, factionGates[slot].active = active;
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("runinfernoplayer %d, portals %d, active %d\n", slot, portals, active);
    writeLog("faction_turn", "slot=%d preset=Inferno home=%d portals=%d active=%d", slot, home, portals, active);
    factionMonumentSteps(slot, f, home, factionMonumentCount(f, factionMonumentPlane(f), -1));
    moveArmies(slot);
    factionTurnSpawn(slot, f, home, active);
}

// ---- The wanderers ------------------------------------------------------------------------------
// The wanderers' words and gates for a faction slot.

// The chance an independent's army is refused a step onto another slot's army.
static unsigned char factionAvoidRefusal[PLAYER_SLOTS_PADDED][PLAYER_SLOTS_PADDED];
static void setFactionAvoid(int mover, int target, int refusal) {
    factionAvoidRefusal[mover][target] = (unsigned char)refusal;
    if (target < PLAYER_SLOTS_GAME)
        for (int s = PLAYER_SLOTS_GAME; s < PLAYER_SLOTS_MOVED; s++) {
            const Faction *g = factionInSlot(s);
            if (g && g->like == target) factionAvoidRefusal[mover][s] = (unsigned char)refusal;
        }
}
static void refreshFactionAvoidTable(void) {
    memset(factionAvoidRefusal, 0, sizeof factionAvoidRefusal);
    setFactionAvoid(24, 30, 75);
    for (int s = PLAYER_SLOTS_GAME; s < PLAYER_SLOTS_MOVED; s++) {
        const Faction *f = factionInSlot(s);
        for (int i = 0; f && i < f->avoidCount; i++) {
            int target = factionPresetNamed(f->avoidName[i]);
            if (target < 0) target = factionSlotNamed(f->avoidName[i]);
            if (target >= 0) setFactionAvoid(s, target, 100 - f->avoidPercent[i]);
        }
    }
}

// Fills the tables from the declared factions; a faction slot's home and active flag as its last turn left them.
static void refreshFactionWanderTables(void) {
    refreshFactionAvoidTable();
    for (int s = 0; s < PLAYER_SLOTS_PADDED; s++) {
        const Faction *f = s >= PLAYER_SLOTS_GAME && s < PLAYER_SLOTS_MOVED ? factionInSlot(s) : NULL;
        int as[FACTION_AS_COUNT] = {s, s, s, s, s, s};
        if (f) {
            as[FACTION_AS_LIKE] = f->like;
            as[FACTION_AS_WANDER] = f->wanderAs >= 0 ? f->wanderAs : s;
            as[FACTION_AS_TITHE] = f->tithe ? s : 29;
            as[FACTION_AS_EVENTS] = f->worldEvents ? 24 : s;
            as[FACTION_AS_CONQUEST] = f->conquestIndependent ? 24 : s;
            as[FACTION_AS_STATUE] = f->statue ? 30 : s;
        }
        for (int b = 0; b < FACTION_AS_COUNT; b++) factionAsBySlot[b][s] = (unsigned char)as[b];
        FactionGate *g = &factionGates[s];
        int gateAs = f ? (f->wanderGate ? as[FACTION_AS_WANDER] : -1) : -1;
        g->kind = gateAs == 28 || gateAs == 29 ? 1 : gateAs == 24 ? 2 : 0;
        g->home = f ? factionHomePlane(f) : -1;
        g->active = f ? factionTurns[s - PLAYER_SLOTS_GAME].active : 0;
    }
}

#ifndef __APPLE__
// A stub for a slot test, `[66] [REX] 80|83 /7 <reg or [mem]>, imm8`: the compare takes the slot's preset.
static int buildPresetTestStub(unsigned char *at, const unsigned char *row, unsigned char *stub, int *length) {
    int i = 0, rex = 0, wide = 0;
    if (at[i] == 0x66) wide = 1, i++;
    if ((at[i] & 0xf0) == 0x40) rex = at[i++];
    int opcode = at[i++], modrm = at[i], imm;
    if ((opcode != 0x80 && opcode != 0x83) || ((modrm >> 3) & 7) != 7) return 0;
    int byte = opcode == 0x80, reg = -1, base = -1, index = -1, memLength = 0;
    SlotOp op;
    if ((modrm >> 6) == 3) {
        reg = (modrm & 7) | (rex & 1 ? 8 : 0);
        if (byte && !rex && reg >= 4) return 0;
        i++;
    } else {
        if (!decodeSlotOp(at, &op) || (!byte && !wide)) return 0;
        base = op.base, index = op.index, memLength = op.memLength;
        i = op.modrmAt + memLength;
    }
    imm = at[i++];
    int cmpLength = i, jccLength = 0;
    unsigned char cc = 0;
    int rel = 0;
    if (cmpLength < 5) {
        jccLength = at[i] == 0x0f ? 6 : 2;
        if (jccLength == 6 && (at[i + 1] & 0xf0) != 0x80) return 0;
        if (jccLength == 2 && (at[i] & 0xf0) != 0x70) return 0;
        cc = jccLength == 6 ? at[i + 1] : (unsigned char)(0x80 | (at[i] & 0x0f));
        if (jccLength == 6) memcpy(&rel, at + i + 2, 4);
        else rel = (signed char)at[i + 1];
    }
    *length = cmpLength + jccLength;
    static const int CANDIDATES[] = {8, 9, 11};
    int scratch[2], n = 0;
    for (int k = 0; k < 3 && n < 2; k++)
        if (CANDIDATES[k] != base && CANDIDATES[k] != index && CANDIDATES[k] != reg) scratch[n++] = CANDIDATES[k];
    if (n < 2) return 0;
    int lo1 = scratch[0] & 7, lo2 = scratch[1] & 7;
    unsigned long long table = (unsigned long long)(uintptr_t)row;
    StubCode c = {stub, 0, {0}, 0};
    STUB(&c, 0x41, (unsigned char)(0x50 | lo1), 0x41, (unsigned char)(0x50 | lo2));
    if (reg >= 0 && !byte && !wide) STUB(&c, (unsigned char)(0x41 | (reg >= 8 ? 4 : 0)), 0x89, (unsigned char)(0xc0 | (reg & 7) << 3 | lo1));
    else if (reg >= 0) STUB(&c, (unsigned char)(0x44 | (reg >= 8 ? 1 : 0)), 0x0f, byte ? 0xbe : 0xbf, (unsigned char)(0xc0 | lo1 << 3 | (reg & 7)));
    else {
        STUB(&c, (unsigned char)(0x44 | (rex & 3)), 0x0f, byte ? 0xbe : 0xbf, (unsigned char)((at[op.modrmAt] & 0xc7) | lo1 << 3));
        stubBytes(&c, at + op.modrmAt + 1, memLength - 1);
    }
    STUB(&c, 0x41, 0x83, (unsigned char)(0xf8 | lo1), PLAYER_SLOTS_PADDED, 0x73, 15, 0x49, (unsigned char)(0xb8 | lo2));
    stubBytes(&c, &table, 8);
    STUB(&c, 0x47, 0x0f, 0xb6, (unsigned char)(0x04 | lo1 << 3), (unsigned char)(lo1 << 3 | lo2));
    STUB(&c, 0x41, 0x83, (unsigned char)(0xf8 | lo1), (unsigned char)imm, 0x41, (unsigned char)(0x58 | lo2), 0x41, (unsigned char)(0x58 | lo1));
    if (jccLength) stubJumpTo(&c, cc, at + *length + rel);
    stubJumpTo(&c, 0, at + *length);
    return c.n;
}

// The gate stub at `cmp ebx, 28; jne`: a faction slot's gate from factionGates, any other slot the game's chain.
static int buildWanderGateStub(unsigned char *at, unsigned char *stub, int *length) {
    *length = 5;
    unsigned char *next = at + 5 + (signed char)at[4], *fallAt = at + 5;
    unsigned long long gates = (unsigned long long)(uintptr_t)factionGates;
    unsigned long long primal = (unsigned long long)(uintptr_t)(gameBase + RVA_PRIMAL_PORTALS_OPEN);
    StubCode c = {stub, 0, {0}, 0};
    int toGame[2], toPass[4], toWait[2], notHome;
    STUB(&c, 0x83, 0xfb, PLAYER_SLOTS_GAME, 0x72, 0);
    toGame[0] = c.n;
    STUB(&c, 0x83, 0xfb, PLAYER_SLOTS_MOVED, 0x73, 0);
    toGame[1] = c.n;
    STUB(&c, 0x41, 0x52, 0x41, 0x53, 0x49, 0xbb);
    stubBytes(&c, &gates, 8);
    STUB(&c, 0x44, 0x8b, 0xd3, 0x41, 0xc1, 0xe2, 4, 0x4d, 0x01, 0xd3, 0x45, 0x8b, 0x13);
    STUB(&c, 0x41, 0x83, 0xfa, 1, 0x75, 0);
    notHome = c.n;
    STUB(&c, 0x41, 0x3b, 0x43, 4, 0x75, 0);
    toPass[0] = c.n;
    STUB(&c, 0x41, 0x83, 0x7b, 8, 0, 0x74, 0);
    toWait[0] = c.n;
    STUB(&c, 0xeb, 0);
    toPass[1] = c.n;
    stub[notHome - 1] = (unsigned char)(c.n - notHome);
    STUB(&c, 0x41, 0x83, 0xfa, 2, 0x75, 0);
    toPass[2] = c.n;
    STUB(&c, 0x41, 0x83, 0xff, 12, 0x75, 0);
    toPass[3] = c.n;
    STUB(&c, 0x83, 0xf8, 7, 0x75, 0);
    int toPassLast = c.n;
    STUB(&c, 0x49, 0xba);
    stubBytes(&c, &primal, 8);
    STUB(&c, 0x41, 0x83, 0x3a, 0, 0x74, 0);
    toWait[1] = c.n;
    for (int i = 0; i < 4; i++) stub[toPass[i] - 1] = (unsigned char)(c.n - toPass[i]);
    stub[toPassLast - 1] = (unsigned char)(c.n - toPassLast);
    STUB(&c, 0x41, 0x5b, 0x41, 0x5a);
    stubJumpTo(&c, 0, gameBase + RVA_WANDER_GATE_PASS);
    for (int i = 0; i < 2; i++) stub[toWait[i] - 1] = (unsigned char)(c.n - toWait[i]);
    STUB(&c, 0x41, 0x5b, 0x41, 0x5a);
    stubJumpTo(&c, 0, gameBase + RVA_WANDER_GATE_WAIT);
    for (int i = 0; i < 2; i++) stub[toGame[i] - 1] = (unsigned char)(c.n - toGame[i]);
    STUB(&c, 0x83, 0xfb, at[2]);
    stubJumpTo(&c, 0x85, next);
    stubJumpTo(&c, 0, fallAt);
    return c.n;
}

// The independents' step onto another army through factionAvoidRefusal: a stub in RVA_INDEP_MOVE_TRY's frame.
_Static_assert(PLAYER_SLOTS_PADDED == 64, "the avoid stub indexes its table by mover << 6");
#define AVOID_STUB_CODE     0x10
#define AVOID_STUB_SCRATCH  0x08
#define AVOID_STUB_FUNCTION 0x80
#define AVOID_STUB_UNWIND   0x90
static size_t copyMoveTryUnwind(unsigned char *out) {
    DWORD64 imageBase = 0;
    PRUNTIME_FUNCTION f = RtlLookupFunctionEntry((DWORD64)(uintptr_t)GAME_PTR(RVA_INDEP_AVOID_MOVER_TEST), &imageBase, NULL);
    if (!f || imageBase != (DWORD64)(uintptr_t)gameBase) return 0;
    const unsigned char *info = gameBase + f->UnwindData, *parent = NULL;
    int codes = info[2], parentCodes = 0;
    if ((info[0] & 7) != 1) return 0;
    if ((info[0] >> 3) & 4) {
        const RUNTIME_FUNCTION *chained = (const RUNTIME_FUNCTION *)(info + 4 + (size_t)((codes + 1) & ~1) * 2);
        parent = gameBase + chained->UnwindData;
        parentCodes = parent[2];
        if ((parent[0] & 7) != 1 || (parent[0] >> 3) & 4) return 0;
    } else if ((info[0] >> 3) & 3) return 0;
    if (codes + parentCodes > 64 || (parent && parent[3] != info[3] && info[3] && parent[3])) return 0;
    out[0] = 1, out[1] = 0, out[2] = (unsigned char)(codes + parentCodes), out[3] = info[3] ? info[3] : parent ? parent[3] : 0;
    memcpy(out + 4, info + 4, (size_t)codes * 2);
    if (parent) memcpy(out + 4 + (size_t)codes * 2, parent + 4, (size_t)parentCodes * 2);
    return 4 + (size_t)((codes + parentCodes + 1) & ~1) * 2;
}
static void emitAvoidRel32(unsigned char **p, const unsigned char *target) {
    int rel = (int)(target - (*p + 4));
    memcpy(*p, &rel, 4);
    *p += 4;
}
static int installFactionAvoidStub(void) {
    unsigned char *page = allocNear(GAME_PTR(RVA_INDEP_AVOID_MOVER_TEST));
    const char *reason = !page ? "no_near_page" : !copyMoveTryUnwind(page + AVOID_STUB_UNWIND) ? "unwind_info" : NULL;
    if (reason) {
        writeLog("faction_avoid_not_installed", "reason=%s", reason);
        return 0;
    }
    unsigned char *code = page + AVOID_STUB_CODE, *p = code, *none[3];
    unsigned long long table = (unsigned long long)(uintptr_t)factionAvoidRefusal, scratch = (unsigned long long)(uintptr_t)(page + AVOID_STUB_SCRATCH);
    unsigned long long random = (unsigned long long)(uintptr_t)GAME_PTR(RVA_RANDOM);
    *p++ = 0x89; *p++ = 0xe8;
    *p++ = 0x83; *p++ = 0xf8; *p++ = PLAYER_SLOTS_PADDED;
    *p++ = 0x73; none[0] = p++;
    *p++ = 0x44; *p++ = 0x89; *p++ = 0xd1;
    *p++ = 0x83; *p++ = 0xf9; *p++ = PLAYER_SLOTS_PADDED;
    *p++ = 0x73; none[1] = p++;
    *p++ = 0xc1; *p++ = 0xe0; *p++ = 6;
    *p++ = 0x01; *p++ = 0xc8;
    *p++ = 0x49; *p++ = 0xbb; memcpy(p, &table, 8); p += 8;
    *p++ = 0x41; *p++ = 0x0f; *p++ = 0xb6; *p++ = 0x04; *p++ = 0x03;
    *p++ = 0x85; *p++ = 0xc0;
    *p++ = 0x74; none[2] = p++;
    *p++ = 0x49; *p++ = 0xbb; memcpy(p, &scratch, 8); p += 8;
    *p++ = 0x41; *p++ = 0x88; *p++ = 0x03;
    *p++ = 0xb9; *p++ = 100; *p++ = 0; *p++ = 0; *p++ = 0;
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &random, 8); p += 8;
    *p++ = 0xff; *p++ = 0xd0;
    *p++ = 0x49; *p++ = 0xbb; memcpy(p, &scratch, 8); p += 8;
    *p++ = 0x41; *p++ = 0x0f; *p++ = 0xb6; *p++ = 0x0b;
    *p++ = 0x39; *p++ = 0xc8;
    *p++ = 0x0f; *p++ = 0x8c; emitAvoidRel32(&p, GAME_PTR(RVA_INDEP_AVOID_REFUSE));
    *p++ = 0xe9; emitAvoidRel32(&p, GAME_PTR(RVA_INDEP_AVOID_ALLOW));
    for (int i = 0; i < 3; i++) *none[i] = (unsigned char)(p - (none[i] + 1));
    *p++ = 0xe9; emitAvoidRel32(&p, GAME_PTR(RVA_INDEP_AVOID_SKIP));
    RUNTIME_FUNCTION *function = (RUNTIME_FUNCTION *)(page + AVOID_STUB_FUNCTION);
    function->BeginAddress = AVOID_STUB_CODE;
    function->EndAddress = (DWORD)(p - page);
    function->UnwindData = AVOID_STUB_UNWIND;
    if (!RtlAddFunctionTable(function, 1, (DWORD64)(uintptr_t)page)) {
        writeLog("faction_avoid_not_installed", "reason=unwind_not_added");
        return 0;
    }
    DWORD old;
    VirtualProtect(page, 0x1000, PAGE_EXECUTE_READWRITE, &old);
    unsigned char *at = GAME_PTR(RVA_INDEP_AVOID_MOVER_TEST), jump[24];
    memset(jump, 0x90, sizeof jump);
    jump[0] = 0xe9;
    int rel = (int)(code - (at + 5));
    memcpy(jump + 1, &rel, 4);
    int ok = writeGameCode(at, jump, sizeof jump);
    writeLog("faction_avoid_installed", "ok=%d stub=%p", ok, (void *)code);
    return ok;
}

// Sends the wanderers' slot tests and gate through their stubs; the number in.
#define FACTION_WANDER_STUB_COUNT 33
static int installFactionWanderStubs(void) {
    static const struct { unsigned int rva; unsigned char as; } TESTS[] = {
        {RVA_WANDER_ABILITY_TEST_29, FACTION_AS_WANDER}, {RVA_WANDER_ABILITY_TEST_24, FACTION_AS_WANDER},
        {RVA_WANDER_ABILITY_TEST_28, FACTION_AS_WANDER}, {RVA_WANDER_ABILITY_TEST_27, FACTION_AS_WANDER},
        {RVA_WANDER_SECOND_TEST_29, FACTION_AS_WANDER},  {RVA_WANDER_SECOND_TEST_24, FACTION_AS_WANDER},
        {RVA_WANDER_SECOND_TEST_28, FACTION_AS_WANDER},  {RVA_WANDER_SECOND_TEST_27, FACTION_AS_WANDER},
        {RVA_WANDERER_AT_TEST_24, FACTION_AS_WANDER},    {RVA_WANDERER_AT_TEST_28, FACTION_AS_WANDER},
        {RVA_WANDERER_AT_TEST_27, FACTION_AS_WANDER},    {RVA_AI_VIEW_SKIP_TEST_24, FACTION_AS_LIKE},
        {RVA_AI_VIEW_SKIP_TEST_28, FACTION_AS_LIKE},     {RVA_AI_VIEW_SKIP_TEST_27, FACTION_AS_LIKE},
        {RVA_TITHE_EXCEPTION_TEST, FACTION_AS_TITHE},    {RVA_EVENT_SQUARE_TEST_A, FACTION_AS_EVENTS},
        {RVA_EVENT_SQUARE_TEST_B, FACTION_AS_EVENTS},    {RVA_EVENT_SQUARE_TEST_C, FACTION_AS_EVENTS},
        {RVA_EVENT_ARMY_TEST, FACTION_AS_EVENTS},        {RVA_EVENT_SQUARE_TEST_D, FACTION_AS_EVENTS},
        {RVA_EVENT_SQUARE_TEST_E, FACTION_AS_EVENTS},    {RVA_METEOR_OWNER_TEST, FACTION_AS_EVENTS},
        {RVA_RANDOM_SQUARE_OWNER_TEST, FACTION_AS_EVENTS}, {RVA_CONQUER_SQUARE_OWNER_TEST, FACTION_AS_CONQUEST},
        {RVA_WANDERER_TARGET_CITY_TEST, FACTION_AS_STATUE}, {RVA_IMPERIAL_STATUE_TEST, FACTION_AS_STATUE},
        {RVA_GOD_VESSEL_OWNER_TEST, FACTION_AS_LIKE},
        {RVA_GOD_VESSEL_OWNER_TEST_B, FACTION_AS_LIKE},  {RVA_POPHORRORS_OWNER_TEST, FACTION_AS_LIKE},
        {RVA_HORRORS_OWNER_TEST_B, FACTION_AS_LIKE},     {RVA_HORRORS_OWNER_TEST_C, FACTION_AS_LIKE},
        {RVA_SITEPOP_HADES_OWNER_TEST, FACTION_AS_LIKE},
    };
    _Static_assert(sizeof TESTS / sizeof TESTS[0] + 1 == FACTION_WANDER_STUB_COUNT, "the gate is the last");
    if (!gameCodeMatches) {
        writeLog("faction_wander_stubs_not_installed", "reason=game_code_differs");
        return 0;
    }
    unsigned char *stubs = allocNearBytes(gameBase + RVA_TEXT_SECTION, FACTION_WANDER_STUB_COUNT * FACTION_STUB_BYTES);
    if (!stubs) {
        writeLog("faction_wander_stubs_not_installed", "reason=no_memory_near_game");
        return 0;
    }
    refreshFactionWanderTables();
    unsigned int rvas[FACTION_WANDER_STUB_COUNT];
    int lengths[FACTION_WANDER_STUB_COUNT] = {0};
    int k = 0;
    for (size_t i = 0; i < sizeof TESTS / sizeof TESTS[0]; i++, k++) {
        rvas[k] = TESTS[i].rva;
        if (!buildPresetTestStub(gameBase + rvas[k], factionAsBySlot[TESTS[i].as], stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    }
    rvas[FACTION_WANDER_STUB_COUNT - 1] = RVA_WANDER_GATE_TEST;
    buildWanderGateStub(gameBase + RVA_WANDER_GATE_TEST, stubs + (FACTION_WANDER_STUB_COUNT - 1) * FACTION_STUB_BYTES,
                        &lengths[FACTION_WANDER_STUB_COUNT - 1]);
    makePagesReadExecute(stubs, FACTION_WANDER_STUB_COUNT * FACTION_STUB_BYTES);
    int ok = 0;
    for (k = 0; k < FACTION_WANDER_STUB_COUNT; k++) {
        if (!lengths[k]) {
            writeLog("faction_stub_refused", "rva=0x%x reason=game_code_differs", rvas[k]);
            continue;
        }
        unsigned char *at = gameBase + rvas[k], jump[16];
        int rel = (int)(stubs + k * FACTION_STUB_BYTES - (at + 5));
        jump[0] = 0xe9;
        memcpy(jump + 1, &rel, 4);
        memset(jump + 5, 0x90, (size_t)lengths[k] - 5);
        ok += writeGameCode(at, jump, (size_t)lengths[k]);
    }
    return ok;
}

#endif

// The Celestial's active test, for a faction on its home.
static int factionCelestialActive(int slot, const Faction *f, int home, int *portalsOut) {
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    int portals = 0;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) portals += squarePortal(x, y) > 0;
    *portalsOut = portals;
    if (f->active == FACTION_ACTIVE_ALWAYS) return 1;
    if (f->active == FACTION_ACTIVE_NEVER) return 0;
    if (portals >= f->active) return 1;
    for (int x = x0; x < 2 * w && x < MAP_COLUMNS_MAX; x++)
        for (int y = y0; y < 2 * h && y < MAP_COLUMN_SQUARES; y++) {
            unsigned char *square = sightSquare(x, y);
            if (x < G32(RVA_MAP_W) && y < G32(RVA_MAP_H)) {
                short terrain = sightTerrain(x, y);
                signed char owner = (signed char)square[SQUARE_OWNER];
                if (terrain >= 0 && (sightTerrainFlags(terrain) & 1) && owner >= 0 && owner != slot) return 1;
            }
            if (*(unsigned short *)(square + SQUARE_ARMY_OWNER) < PLAYER_SEATS) return 1;
        }
    return 0;
}

// RVA_MOVE_CELESTIAL_PLAYER for a faction: the active test, then the armies' move (the Celestial spawns nothing).
static void factionCelestialMove(int slot, const Faction *f, void (*moveArmies)(int)) {
    int home = factionHomePlane(f), portals = 0;
    int active = home >= 0 && factionCelestialActive(slot, f, home, &portals);
    factionTurns[slot - PLAYER_SLOTS_GAME].active = active;
    factionGates[slot].home = home, factionGates[slot].active = active;
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("runcelplayer %d, portals %d, active %d\n", slot, portals, active);
    writeLog("faction_turn", "slot=%d preset=Celestial home=%d portals=%d active=%d", slot, home, portals, active);
    factionMonumentSteps(slot, f, home, factionMonumentCount(f, factionMonumentPlane(f), -1));
    moveArmies(slot);
    factionTurnSpawn(slot, f, home, active);
}

// ---- The Inferno's carriers and its infernal wanderers ------------------------------------------
// The Inferno's carriers and infernal wanderers, for a faction like the Inferno.
static const char *const INFERNO_GATEWAY_NEWS_TITLE = "World News";

// One unit's carrier step, for a faction slot and its home.
static void factionCarrierStep(int slot, const Faction *f, int home, int unit) {
    int (*total)(int, int, int, char *, int, int *) = GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL);
    int most = total(unit, f->gatherAbility, 1, NULL, 0, NULL);
    const char *fuel = f->fuel;
    if (most <= 0) return;
    size_t at = (size_t)unit * UNIT_SIZE;
    int x = (int)G32(RVA_UNIT_X + at), y = (int)G32(RVA_UNIT_X + at + 4);
    if (!squareInMap(x, y)) return;
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y), terrain = sightTerrain(x, y);
    int commander = moverUnitCommander(unit), leader = (int)G32(RVA_UNIT_LEADER + at);
    int (*carried)(int, const char *) = moverFollowersNamed;
    int (*release)(int, const char *, int) = moverReleaseFollowers;
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    void (*trace)(const char *, ...) = GAME_FN(void (*)(const char *, ...), RVA_TRACE);
    const char *(*name)(int) = moverTraceName;
    void (*change)(int, int, int) = GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN);
    int leads = plane == 0 && commander >= 0 && leader < 0;
    if (leads && f->gatewayFuel > 0 && carried(commander, fuel) > 0 && terrain == f->gatewayRuin) {
        int n = carried(commander, fuel);
        release(commander, fuel, n);
        trace("infernal %s putting %d sinners on destroyed gateway\n", name(unit), n);
        if (GAME_FN(int (*)(int, int, const char *), RVA_COUNT_MONSTERS_AT)(x, y, fuel) < f->gatewayFuel) return;
        GAME_FN(int (*)(int, int, int, const char *, int), RVA_REMOVE_MONSTERS_AT)(slot, x, y, fuel, f->gatewayFuel);
        trace("infernal %s restored gateway\n", name(unit));
        change(x, y, f->gatewayGate);
        short portal = (short)(random(10000) + 1000);
        *(short *)(sightSquare(x, y) + SQUARE_PORTAL) = portal;
        int ex = -1, ey = -1;
        int (*square)(int, int, int, int, int, int, int *, int *) = GAME_FN(int (*)(int, int, int, int, int, int, int *, int *), RVA_RANDOM_RITUAL_SQUARE);
        square(home, 1, 0, 1, f->gatewayRuin, -1, &ex, &ey);
        if (ex < 0) square(home, 1, 0, 1, TERRAIN_FROZEN_WASTE, TERRAIN_INFERNAL_WASTE, &ex, &ey);
        if (ex >= 0 && ey >= 0) {
            change(ex, ey, f->gatewayExit);
            *(short *)(sightSquare(ex, ey) + SQUARE_PORTAL) = portal;
        }
        if (f->gatewayNews[0])
            GAME_FN(void (*)(int, int, int, int, int, int, const char *, const char *), RVA_ADD_MESSAGE)(-1, -2, x, y, 0, 0, INFERNO_GATEWAY_NEWS_TITLE,
                                                                                                    f->gatewayNews);
        writeLog("faction_gateway_restored", "slot=%d x=%d y=%d exit_x=%d exit_y=%d portal=%d", slot, x, y, ex, ey, portal);
        return;
    }
    if (leads && carried(commander, fuel) < most) {
        if (!(sightTerrainFlags(terrain) & (1ULL << 32))) return;
        int people = GAME_FN(int (*)(int, int, int, int), RVA_SQUARE_INCOME)(slot, x, y, 4);
        GAME_FN(void (*)(int, int, int, int), RVA_SQUARE_SACRIFICE_TAKEN)(slot, x, y, 1);
        int n = most;
        if (people / (random(11) + 5) <= most) n = people / (random(11) + 5);
        if (n > 0) GAME_FN(int (*)(int, const char *, int), RVA_POPULATE_FOLLOWERS)(unit, fuel, n);
        trace("infernal %s trying to get sinners: sacr %d, sinners %d\n", name(unit), people, n);
        return;
    }
    if (plane == home && commander >= 0 && terrain >= 0 && leader < 0 && carried(commander, fuel) > 0 && (sightTerrainFlags(terrain) & 1)) {
        int n = carried(commander, fuel);
        release(commander, fuel, n);
        trace("infernal %s returning %d sinners\n", name(unit), n);
    }
}

// RVA_TURN_UNIT_UPKEEP: the game's, then a faction like the Inferno's carriers.
typedef void (*TurnUnitUpkeepFn)(int player);
static TurnUnitUpkeepFn gameTurnUnitUpkeep;
static void turnUnitUpkeepHook(int player) {
    gameTurnUnitUpkeep(player);
    const Faction *f = factionSlot(player) ? factionInSlot(player) : NULL;
    if (!f || f->gatherAbility <= 0 || !f->fuel[0]) return;
    int home = factionHomePlane(f), last = G32(RVA_UNIT_COUNT);
    if (home < 0) return;
    for (int u = 0; u <= last; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_PLAYER + at) == player && (short)G16(RVA_UNIT_HP + at) > 0) factionCarrierStep(player, f, home, u);
    }
}

// A random square of a plane's rectangle matching a terrain code, up to `tries` draws; -1, -1 for none.
static void randomPlaneSquareOf(int plane, int code, int tries, int *x, int *y) {
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    int (*match)(int, int, int) = GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH);
    for (int i = 0; i < tries; i++) {
        int tx = random(planeField(plane, PLANE_W)) + planeField(plane, PLANE_X);
        int ty = random(planeField(plane, PLANE_H)) + planeField(plane, PLANE_Y);
        if (match(code, tx, ty)) {
            *x = tx, *y = ty;
            return;
        }
    }
    *x = *y = -1;
}

// The infernal and Hades wanderers' raid target at home.
static int wandererAttackHomeCitadel(int slot, int home, unsigned int *outX, unsigned int *outY) {
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    int (*match)(int, int, int) = GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH);
    if (random(100) < 10)
        for (int i = 0; i < 200; i++) {
            int tx = random(planeField(home, PLANE_W)) + planeField(home, PLANE_X);
            int ty = random(planeField(home, PLANE_H)) + planeField(home, PLANE_Y);
            if (!match(TERRAIN_CODE_CITADEL, tx, ty)) continue;
            if (tx < 0) break;
            unsigned char owner = squareInMap(tx, ty) ? sightSquare(tx, ty)[SQUARE_OWNER] : 0xff;
            if (owner >= PLAYER_SLOTS_MOVED || (owner != slot && !playerSameTeam(owner, slot))) {
                *outX = (unsigned int)tx, *outY = (unsigned int)ty;
                return 1;
            }
        }
    *outX = *outY = (unsigned int)-1;
    return 0;
}

// RVA_WANDERER_PICK_TARGET's objective 7, the Hades wanderer, for a faction like Hades (0x16bc00..0x16be14).
static int factionHadesTarget(int slot, int home, int unit, unsigned int *outX, unsigned int *outY) {
    int x0 = planeField(home, PLANE_X), y0 = planeField(home, PLANE_Y), w = planeField(home, PLANE_W), h = planeField(home, PLANE_H);
    int portals = 0;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) portals += squarePortal(x, y) > 0;
    size_t at = (size_t)unit * UNIT_SIZE;
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)G32(RVA_UNIT_X + at), (int)G32(RVA_UNIT_X + at + 4));
    if (plane != 0 && portals < 1) {
        if (plane != home) {
            *outX = *outY = (unsigned int)-1;
            return 0;
        }
        return wandererAttackHomeCitadel(slot, home, outX, outY);
    }
    int x, y;
    randomPlaneSquareOf(0, TERRAIN_CODE_INFERNAL_TARGET, 500000, &x, &y);
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("hades wanderer trying to attack %s\n", GAME_FN(const char *(*)(int, int), RVA_SQUARE_TEXT)(x, y));
    if (x < 0) {
        *outX = *outY = (unsigned int)-1;
        return 0;
    }
    *outX = (unsigned int)x, *outY = (unsigned int)y;
    return 1;
}

// RVA_WANDERER_PICK_TARGET's objective 6, the infernal wanderer, for a faction like the Inferno (0x16b4f0..0x16bbbf).
static int factionInfernalTarget(int slot, int home, int commander, int unit, unsigned int *outX, unsigned int *outY) {
    const Faction *f = factionInSlot(slot);
    int (*count)(int, int) = moverPortalsLinkedTo;
    int ports = count(3, home) + count(2, home) + count(0, home);
    int sinners = f && f->fuel[0] ? moverFollowersNamed(commander, f->fuel) : 0;
    int most = f && f->gatherAbility > 0 ? GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL)(unit, f->gatherAbility, 0, NULL, 0, NULL) : 0;
    int ruin = f ? f->gatewayRuin : TERRAIN_DESTROYED_GATEWAY;
    size_t at = (size_t)unit * UNIT_SIZE;
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)G32(RVA_UNIT_X + at), (int)G32(RVA_UNIT_X + at + 4));
    int x = -1, y = -1;
    const char *what;
    if (plane == 0 && !ports) {
        if (most < 1 || sinners < most / 2 || (f && f->gatewayFuel <= 0)) randomPlaneSquareOf(0, TERRAIN_CODE_INFERNAL_TARGET, 500000, &x, &y), what = "attack %s";
        else randomPlaneSquareOf(0, ruin, 500000, &x, &y), what = "restore %s";
    } else if ((plane == home && sinners >= 1) || (most > 0 && most / 2 <= sinners)) {
        randomPlaneSquareOf(home, TERRAIN_CODE_CITADEL, 500000, &x, &y), what = "dump sinners at %s";
    } else if (plane != 0 && ports < 1) {
        return wandererAttackHomeCitadel(slot, home, outX, outY);
    } else {
        randomPlaneSquareOf(0, TERRAIN_CODE_INFERNAL_TARGET, 500000, &x, &y), what = "storm elysium at %s";
    }
    char format[160];
    snprintf(format, sizeof format, "infernal wanderer trying to %s (sinners %%d, max %%d, ports %%d)\n", what);
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)(format, GAME_FN(const char *(*)(int, int), RVA_SQUARE_TEXT)(x, y), sinners, most, ports);
    if (x >= 0) {
        *outX = (unsigned int)x, *outY = (unsigned int)y;
        return 1;
    }
    *outX = *outY = (unsigned int)-1;
    return 0;
}

// RVA_WANDERER_PICK_TARGET: a faction like the Inferno's infernal wanderer picks on its home; anything else the game's.
typedef unsigned long long (*WandererPickTargetFn)(int commander, int objective, unsigned int *x, unsigned int *y);
static WandererPickTargetFn gameWandererPickTarget;
static unsigned long long wandererPickTargetHook(int commander, int objective, unsigned int *x, unsigned int *y) {
    if ((objective != OBJECTIVE_INFERNAL_WANDERER && objective != OBJECTIVE_HADES_WANDERER) || commander < 0 || commander > G32(RVA_LAST_COMMANDER))
        return gameWandererPickTarget(commander, objective, x, y);
    int unit = (int)G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    int owner = unit >= 0 && unit <= G32(RVA_UNIT_COUNT) ? (short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE) : -1;
    const Faction *f = factionSlot(owner) ? factionInSlot(owner) : NULL;
    int preset = objective == OBJECTIVE_INFERNAL_WANDERER ? 28 : 27;
    int home = f && f->like == preset ? factionHomePlane(f) : -1;
    if (home < 0) return gameWandererPickTarget(commander, objective, x, y);
    size_t at = (size_t)unit * UNIT_SIZE;
    int ux = (int)G32(RVA_UNIT_X + at), uy = (int)G32(RVA_UNIT_X + at + 4);
    if (GAME_FN(int (*)(int, int), RVA_PLANE_AT)(ux, uy) < 0 || !squareInMap(ux, uy) || sightTerrain(ux, uy) < 0) return 0;
    if (preset == 27) return (unsigned long long)factionHadesTarget(owner, home, unit, x, y);
    return (unsigned long long)factionInfernalTarget(owner, home, commander, unit, x, y);
}

// ---- Hades ----------------------------------------------------------------------------------------
// Hades' turn, for a faction like Hades.

// The game's changeloyalty for a slot; the number of units changed.
static int changeLoyaltyTo(int x, int y, const char *name, int from, int to) {
    if (x < 0 || y < 0) return 0;
    GAME_FN(void (*)(const char *, ...), RVA_TRACE_C)("changeloyalty %d,%d %d->%d (%s)\n", x, y, from, to, name);
    int changed = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || (int)G32(RVA_UNIT_X + at) != x || (int)G32(RVA_UNIT_X + at + 4) != y) continue;
        if (name[0] && strcmp((const char *)monsterRecord((short)G16(RVA_UNIT_MONSTER + at)), name)) continue;
        short owner = (short)G16(RVA_UNIT_PLAYER + at);
        if ((from != -1 && owner != from) || owner == to) continue;
        *(short *)GAME_PTR(RVA_UNIT_PLAYER + at) = (short)to;
        changed++;
    }
    if (changed > 0) rebuildSquareArmyHook(x, y);
    return changed;
}

// The plane of a faction's monuments; -1 when it isn't in this world.
static int factionMonumentPlane(const Faction *f) {
    if (!f->spawnPlane[0]) return 0;
    int plane = vanillaPlaneNamed(f->spawnPlane);
    return plane >= 0 ? plane : customPlaneIndex(f->spawnPlane);
}

// Whether unit `u` is one of the faction's living monuments.
static int isFactionMonument(const Faction *f, int u) {
    size_t at = (size_t)u * UNIT_SIZE;
    short monster = (short)G16(RVA_UNIT_MONSTER + at);
    return f->monument[0] && (short)G16(RVA_UNIT_HP + at) > 0 && monster >= 0 && !strcmp((const char *)monsterRecord(monster), f->monument);
}

// The faction's living monuments on `plane` (any owner's, as the game counts the Monument of Hades), or only `owner`'s.
static int factionMonumentCount(const Faction *f, int plane, int owner) {
    if (!f->monument[0] || plane < 0) return 0;
    int count = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if (!isFactionMonument(f, u) || (owner >= 0 && (short)G16(RVA_UNIT_PLAYER + at) != owner)) continue;
        if (GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)G32(RVA_UNIT_X + at), (int)G32(RVA_UNIT_X + at + 4)) == plane) count++;
    }
    return count;
}

// A faction's monument founding, at the start of its turns.
static int changeLoyaltyTo(int x, int y, const char *name, int from, int to);
// "<name> the <unit's shown name>": the game's RVA_COMMANDER_THE_NAME, or on Mac CoE5SE's copy (lua_ai.h).
// @platform-split commander_the_name: Mac copied the commander's "the" name into each caller, so CoE5SE's copy runs there
static const char *moverCommanderTheName(int commander) {
#ifdef __APPLE__
    return commanderTheName(commander);
#else
    return GAME_FN(const char *(*)(int), RVA_COMMANDER_THE_NAME)(commander);
#endif
}
static void factionMonumentFounding(int slot, const Faction *f) {
    if (f->eventTurn < 0 || G32(RVA_TURN) <= f->eventTurn) return;
    if (GAME_FN(int (*)(int), RVA_RANDOM)(1000) >= f->eventChance) return;
    if (*(const unsigned char *)GAME_PTR(RVA_NO_EVENTS)) return;
    int plane = 0;
    if (f->eventPlane[0]) {
        plane = vanillaPlaneNamed(f->eventPlane);
        if (plane < 0) plane = customPlaneIndex(f->eventPlane);
        if (plane < 0) return;
    }
    int x = -1, y = -1;
    GAME_FN(int (*)(int, int, int *, int *), RVA_RANDOM_SQUARE_OF_TERRAIN)(plane, f->eventTerrain, &x, &y);
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_W) || y >= G32(RVA_MAP_H) || (signed char)sightSquare(x, y)[SQUARE_OWNER] != 24) return;
    int commander = -1;
#ifdef __APPLE__
    GAME_FN(int (*)(int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(-1, x, y, -1, -1, &commander);
#else
    GAME_FN(int (*)(int, int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(24, -1, x, y, -1, -1, &commander);
#endif
    if (commander < 0) return;
    changeLoyaltyTo(x, y, "", 24, slot);
    if (f->eventNews[0])
        GAME_FN(void (*)(int, int, int, int, int, int, const char *, const char *, ...), RVA_ADD_MESSAGE)(-1, -2, -1, -1, 0, 0, "World News", f->eventNews,
                                                                                                         moverCommanderTheName(commander));
    GAME_FN(int (*)(int, int, int, const char *, int), RVA_SUMMON_STRING)(slot, x, y, f->eventUnfinished, 100);
    GAME_FN(void (*)(int, int, int), RVA_POPULATE_SET_HOLDIT)(x, y, 1);
    writeLog("faction_monument_founded", "slot=%d x=%d y=%d commander=%d monster=%.40s", slot, x, y, commander, f->eventUnfinished);
}

// The monument's steps of a faction's turn, before its armies move.
static void factionMonumentSteps(int slot, const Faction *f, int home, int monuments) {
    if (monuments <= 0) return;
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("planeshiftmonsters\n");
    if (f->shiftMonster[0] && home >= 0) moverPlaneShiftMonsters(home, f->shiftPercent, f->shiftEach, f->shiftMonster);
    if (!f->loyaltyMonster[0]) return;
    int plane = factionMonumentPlane(f), last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        short monster = (short)G16(RVA_UNIT_MONSTER + at);
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || monster < 0 || strcmp((const char *)monsterRecord(monster), f->loyaltyMonster)) continue;
        int x = (int)G32(RVA_UNIT_X + at), y = (int)G32(RVA_UNIT_X + at + 4);
        if (GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y) != plane || (short)G16(RVA_UNIT_PLAYER + at) == slot) continue;
        changeLoyaltyTo(x, y, f->loyaltyMonster, -1, slot);
        last = G32(RVA_UNIT_COUNT);
    }
}

// RVA_SQUARE_IS_SEA: the square's terrain is sea (TERRAIN_FLAGS bit 2), else its own flag bit 13; 0 off the map.
static int moverSquareIsSea(int x, int y) {
    if (!squareInMap(x, y)) return 0;
    short terrain = sightTerrain(x, y);
    if (terrain >= 0 && (sightTerrainFlags(terrain) & 4)) return 1;
    unsigned int flags;
    memcpy(&flags, sightSquare(x, y) + (RVA_SQUARE_FLAGS - RVA_SQUARE_RECORDS), sizeof flags);
    return (flags >> 13) & 1;
}

// A faction's caravan, at the start of its turns.
static void factionCaravan(int slot, const Faction *f) {
    if (f->caravanTurn < 0 || G32(RVA_TURN) <= f->caravanTurn) return;
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    if (random(350) >= f->caravanChance && (random(350) >= 2 * f->caravanChance || G16(RVA_SOCIETY) != 2)) return;
    int plane = 0;
    if (f->caravanPlane[0]) {
        plane = vanillaPlaneNamed(f->caravanPlane);
        if (plane < 0) plane = customPlaneIndex(f->caravanPlane);
        if (plane < 0) return;
    }
    int sx = -1, sy = -1;
    GAME_FN(int (*)(int, int, int *, int *), RVA_RANDOM_SQUARE_OF_TERRAIN)(plane, f->caravanTerrain, &sx, &sy);
    if (sx < 0) return;
    if (f->caravanNews[0])
        GAME_FN(void (*)(int, int, int, int, int, int, const char *, const char *, ...), RVA_ADD_MESSAGE)(-1, -2, -1, -1, 0, 0, "World News", f->caravanNews,
                                                                                                         GAME_FN(const char *(*)(int, int, int), RVA_SQUARE_SHOWN_NAME)(-1, sx, sy));
    int x = -1, y = -1, found = 0;
    for (int tries = 0; tries < 100 && !found; tries++) {
        x = sx + random(3) - 1, y = sy + random(3) - 1;
        found = x >= 0 && y >= 0 && x < G32(RVA_MAP_DEPTH) && y < G32(RVA_MAP_H) && !G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2)
                && !moverSquareIsSea(x, y);
    }
    if (!found) return;
    if (f->caravanGroup[0]) GAME_FN(int (*)(int, int, int, const char *, int), RVA_SUMMON_STRING)(slot, x, y, f->caravanGroup, 100);
    else {
        int (*follow)(int, const char *, int) = GAME_FN(int (*)(int, const char *, int), RVA_POPULATE_FOLLOWERS);
        int (*die)(int) = GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED);
        GAME_FN(int (*)(const char *, int, int, int, const char *), RVA_POPULATE_COMMANDER)("Merchant", slot, x, y, "");
        if (random(100) < 25) follow(-2, "Wizard of the Golden Dawn", 1);
        if (random(100) < 25) follow(-2, "White Wizard", 1);
        if (random(100) < 10) follow(-2, "Ogre", 1);
        if (random(100) < 10) follow(-2, "Knight", 1);
        int guards = die(5);
        guards += die(5);
        follow(-2, "Caravan Guard", guards);
        int bows = die(5);
        bows += die(5);
        follow(-2, "Crossbowman", bows);
    }
    writeLog("faction_caravan", "slot=%d x=%d y=%d group=%.40s", slot, x, y, f->caravanGroup[0] ? f->caravanGroup : "merchants");
}

static void factionHadesMove(int slot, const Faction *f, void (*moveArmies)(int)) {
    int home = factionHomePlane(f);
    int monuments = factionMonumentCount(f, factionMonumentPlane(f), -1);
    GAME_FN(void (*)(const char *, ...), RVA_TRACE)("runhadesplayer %d, monuments %d\n", slot, monuments);
    writeLog("faction_turn", "slot=%d preset=Hades home=%d monuments=%d", slot, home, monuments);
    factionMonumentSteps(slot, f, home, monuments);
    moveArmies(slot);
    factionTurnSpawn(slot, f, home, 1);
}

// The monument site's spawns, for a faction.
static void factionMonumentSpawns(int slot, const Faction *f) {
    int plane = factionMonumentPlane(f);
    if (factionMonumentCount(f, plane, -1) <= 0) return;
    int x0 = planeField(plane, PLANE_X), y0 = planeField(plane, PLANE_Y), w = planeField(plane, PLANE_W), h = planeField(plane, PLANE_H);
    int x1 = plane == 0 ? 2 * w : x0 + w, y1 = plane == 0 ? 2 * h : y0 + h;
    for (int x = x0; x < x1; x++)
        for (int y = y0; y < y1; y++) {
            if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || !G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2))
                continue;
            int here = 0, last = G32(RVA_UNIT_COUNT);
            for (int u = 0; u <= last && !here; u++)
                here = (int)G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE) == x && (int)G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE + 4) == y && isFactionMonument(f, u);
            if (!here) continue;
            int fuel = GAME_FN(int (*)(int, int, const char *), RVA_COUNT_MONSTERS_AT)(x, y, f->fuel);
            GAME_FN(void (*)(const char *, ...), RVA_TRACE)("hadesmon: %s, spirits %d\n", GAME_FN(const char *(*)(int, int), RVA_SQUARE_TEXT)(x, y), fuel);
            static const int NO_TERRAIN[4] = {0};
            factionSpawnAt(slot, f, x, y, fuel, NO_TERRAIN);
        }
}

// ---- The Horrors' spawns ------------------------------------------------------------------------
// pophorrors' spawns at units marked by horrors go to the Horrors or a faction with factionhorrors, one each at random.
static int horrorsPending = -1;
static int pickHorrorsOwner(int vanilla) {
    if (!factionSlotsInUse()) return vanilla;
    int slots[FACTION_SLOT_TARGET], count = 0;
    for (int slot = PLAYER_SLOTS_GAME; slot < PLAYER_SLOTS_MOVED; slot++) {
        const Faction *f = factionInSlot(slot);
        if (f && f->horrors) slots[count++] = slot;
    }
    if (!count) return vanilla;
    int k = GAME_FN(int (*)(int), RVA_RANDOM)(count + 1);
    if (k) writeLog("faction_horrors_spawn", "slot=%d instead_of=%d", slots[k - 1], vanilla);
    return k == 0 ? vanilla : slots[k - 1];
}
static void horrorGroupHook(int x, int y, int strength, int player) {
    GAME_FN(void (*)(int, int, int, int), RVA_SPAWN_HORROR_GROUP)(x, y, strength, pickHorrorsOwner(player));
}
#ifdef __APPLE__
static int horrorSinglePopulateHook(int x, int y, const char *name, int count, int player, int commander) {
    return GAME_FN(int (*)(int, int, const char *, int, int, int), RVA_POPULATE)(x, y, name, count, pickHorrorsOwner(player), commander);
}
#else
static int horrorSinglePopulateHook(int x, int y, const char *name, int count, int player, int commander) {
    return GAME_FN(int (*)(int, int, const char *, int, int, int), RVA_POPULATE)(x, y, name, count, horrorsPending >= 0 ? horrorsPending : player, commander);
}
static void pophorrorsSingleHook(int x, int y) {
    horrorsPending = pickHorrorsOwner(26);
    GAME_FN(void (*)(int, int), RVA_SPAWN_HORROR_SINGLE)(x, y);
    horrorsPending = -1;
}
#endif
// Points pophorrors' spawn calls at the hooks above; the count in.
static int installHorrorsSplit(void) {
    int ok = pointCustomPlaneCall(RVA_POPHORRORS_GROUP_CALL, RVA_SPAWN_HORROR_GROUP, (void *)horrorGroupHook, "pophorrors_group");
#ifdef __APPLE__
    ok += pointCustomPlaneCall(RVA_POPHORRORS_SINGLE_CALL, RVA_POPULATE, (void *)horrorSinglePopulateHook, "pophorrors_single");
    int wanted = 2;
#else
    ok += pointCustomPlaneCall(RVA_POPHORRORS_SINGLE_CALL, RVA_SPAWN_HORROR_SINGLE, (void *)pophorrorsSingleHook, "pophorrors_single");
    ok += pointCustomPlaneCall(RVA_HORROR_SINGLE_POPULATE_CALL, RVA_POPULATE, (void *)horrorSinglePopulateHook, "horror_single_populate");
    int wanted = 3;
#endif
    writeLog("faction_horrors_installed", "calls=%d of %d", ok, wanted);
    return ok == wanted;
}

// ---- Elysium's rulership ---------------------------------------------------------------------------
// Elysium's rulership check, factions included.
typedef long long (*RulershipCheckFn)(long long a0);
static RulershipCheckFn gameRulershipCheck;
static int factionInfernalWanderersOnElysium(int slot) {
    int count = 0, last = G32(RVA_UNIT_COUNT);
    int (*total)(int, int, int, char *, int, int *) = GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || (short)G16(RVA_UNIT_MONSTER + at) < 0 || (short)G16(RVA_UNIT_PLAYER + at) != slot) continue;
        if (GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)G32(RVA_UNIT_X + at), (int)G32(RVA_UNIT_X + at + 4)) != 0) continue;
        count += total(u, ABILITY_WANDER, 0, NULL, 0, NULL) == 6;
    }
    return count;
}
// Logs a change in which faction contests Elysium's rulership (the check runs every month and turn).
static void noteFactionRulership(int slot, int pass) {
    static int last;
    int now = slot * 4 + pass;
    if (now == last) return;
    last = now;
    if (pass) writeLog("faction_rulership_contest", "slot=%d by=%s", slot, pass == 1 ? "infernal_wanderers" : "monument");
    else writeLog("faction_rulership_contest", "slot=-1 by=none");
}
static long long rulershipCheckHook(long long a0) {
    long long contest = gameRulershipCheck(a0);
    if (contest || !factionSlotsInUse()) return contest;
    for (int pass = 1; pass <= 2; pass++)
        for (int slot = PLAYER_SLOTS_GAME; slot < PLAYER_SLOTS_MOVED; slot++) {
            const Faction *f = factionInSlot(slot);
            if (!f) continue;
            int found = pass == 1 ? factionInfernalWanderersOnElysium(slot) : f->monumentRule ? factionMonumentCount(f, 0, slot) : 0;
            if (found <= 0) continue;
            G32(RVA_RULERSHIP_SLOT) = slot;
            noteFactionRulership(slot, pass);
            return pass;
        }
    noteFactionRulership(0, 0);
    return 0;
}

// RVA_MOVE_ALL_INDEPENDENTS: a faction slot's preset mover around it; any other slot the game's.
typedef void (*MoveAllIndependentsFn)(int player);
static MoveAllIndependentsFn gameMoveAllIndependents;
static void moveAllIndependentsHook(int player) {
    const Faction *f = factionSlot(player) ? factionInSlot(player) : NULL;
    if (f) factionCaravan(player, f), factionMonumentFounding(player, f);
    if (f && f->like == 28) factionInfernoMove(player, f, gameMoveAllIndependents);
    else if (f && f->like == 29) factionCelestialMove(player, f, gameMoveAllIndependents);
    else if (f && f->like == 27) factionHadesMove(player, f, gameMoveAllIndependents);
    else {
        int home = f ? factionHomePlane(f) : -1;
        if (f) factionMonumentSteps(player, f, home, factionMonumentCount(f, factionMonumentPlane(f), -1));
        gameMoveAllIndependents(player);
        if (!f || f->spawnSite == FACTION_SITE_NONE) return;
        int portals = 0;
        factionTurnSpawn(player, f, home, home >= 0 && factionInfernoActive(player, f, home, &portals));
    }
}

// Hooks the plain mover, the turn's unit upkeep, the wanderers' targets and their slot tests once; 1 when all are in.
#ifndef __APPLE__
static int installFactionMovers(void) {
    static const struct { unsigned int rva; void *hook; void **game; } HOOKS[] = {
        {RVA_MOVE_ALL_INDEPENDENTS, (void *)moveAllIndependentsHook, (void **)&gameMoveAllIndependents},
        {RVA_TURN_UNIT_UPKEEP, (void *)turnUnitUpkeepHook, (void **)&gameTurnUnitUpkeep},
        {RVA_WANDERER_PICK_TARGET, (void *)wandererPickTargetHook, (void **)&gameWandererPickTarget},
        {RVA_RULERSHIP_CHECK, (void *)rulershipCheckHook, (void **)&gameRulershipCheck},
    };
    int hooked = 0, wanted = (int)(sizeof HOOKS / sizeof HOOKS[0]);
    for (int i = 0; i < wanted; i++) {
        void *target = gameBase + HOOKS[i].rva;
        hooked += fanpatchHookCreate(target, HOOKS[i].hook, HOOKS[i].game) && fanpatchHookEnable(target, 1);
    }
    int wander = installFactionWanderStubs();
    int horrors = installHorrorsSplit();
    int avoid = installFactionAvoidStub();
    int population = installPopulateAfterVoidHook();
    writeLog("faction_movers_installed", "hooks=%d of %d wander_stubs=%d of %d horrors=%d avoid=%d population=%d", hooked, wanted, wander,
             FACTION_WANDER_STUB_COUNT, horrors, avoid, population);
    return hooked == wanted && wander == FACTION_WANDER_STUB_COUNT && horrors && avoid && population;
}
#else
// The refusal chance of a step by `mover` onto `owner`'s army (mac_site_stubs.h's MAC_SITE_AVOID), 0 none.
static int macAvoidRefusal(int mover, int owner) {
    return mover >= 0 && mover < PLAYER_SLOTS_PADDED && owner >= 0 && owner < PLAYER_SLOTS_PADDED ? factionAvoidRefusal[mover][owner] : 0;
}
// The slot a faction passes a preset's test as (mac_site_stubs.h's MAC_SITE_SLOT_TEST): its row's.
static int macSlotTestValue(int behaviour, int slot) {
    return slot >= 0 && slot < PLAYER_SLOTS_PADDED && behaviour >= 0 && behaviour < FACTION_AS_COUNT ? factionAsBySlot[behaviour][slot] : slot;
}
// A faction slot's wanderer gate (mac_site_stubs.h's MAC_SITE_WANDER_GATE): 1 waits, 0 goes on.
static int macWanderGate(int slot, int plane, int value) {
    const FactionGate *g = &factionGates[slot];
    if (g->kind == 1) return plane == g->home && !g->active;
    if (g->kind == 2) return value == 12 && plane == 7 && !*(const unsigned char *)(gameBase + RVA_PRIMAL_PORTALS_OPEN);
    return 0;
}
// Mac's copied preset tests, pointed at the faction rows; 1 when every one is in.
typedef struct { unsigned int rva; unsigned char kind, as; short field; const char *name; } MacPresetTest;
#define MAC_TEST(rva, as, name) {rva, MAC_SITE_SLOT_TEST, as, 0, name}
#define MAC_WANDER_GATE_SLOT_AT 0xf8
static const MacPresetTest MAC_PRESET_TESTS[] = {
    MAC_TEST(RVA_WANDER_ABILITY_TEST_29, FACTION_AS_WANDER, "wander_ability_29"), MAC_TEST(RVA_WANDER_ABILITY_TEST_24, FACTION_AS_WANDER, "wander_ability_24"),
    MAC_TEST(RVA_WANDER_ABILITY_TEST_28, FACTION_AS_WANDER, "wander_ability_28"), MAC_TEST(RVA_WANDER_ABILITY_TEST_27, FACTION_AS_WANDER, "wander_ability_27"),
    MAC_TEST(RVA_WANDER_SECOND_TEST_29, FACTION_AS_WANDER, "wander_second_29"),   MAC_TEST(RVA_WANDER_SECOND_TEST_24, FACTION_AS_WANDER, "wander_second_24"),
    MAC_TEST(RVA_WANDER_SECOND_TEST_28, FACTION_AS_WANDER, "wander_second_28"),   MAC_TEST(RVA_WANDER_SECOND_TEST_27, FACTION_AS_WANDER, "wander_second_27"),
    {RVA_WANDER_GATE_TEST, MAC_SITE_WANDER_GATE, 0, MAC_WANDER_GATE_SLOT_AT, "wander_gate"},
    MAC_TEST(RVA_WANDERER_AT_TEST_24, FACTION_AS_WANDER, "wanderer_at_24"),   MAC_TEST(RVA_WANDERER_AT_TEST_28, FACTION_AS_WANDER, "wanderer_at_28"),
    MAC_TEST(RVA_WANDERER_AT_TEST_27, FACTION_AS_WANDER, "wanderer_at_27"),   MAC_TEST(RVA_WANDERER_AT_TEST_24_B, FACTION_AS_WANDER, "wanderer_at_24_b"),
    MAC_TEST(RVA_WANDERER_AT_TEST_28_B, FACTION_AS_WANDER, "wanderer_at_28_b"), MAC_TEST(RVA_WANDERER_AT_TEST_27_B, FACTION_AS_WANDER, "wanderer_at_27_b"),
    MAC_TEST(RVA_AI_VIEW_SKIP_TEST_24, FACTION_AS_LIKE, "ai_view_skip_24"),   MAC_TEST(RVA_AI_VIEW_SKIP_TEST_28, FACTION_AS_LIKE, "ai_view_skip_28"),
    MAC_TEST(RVA_AI_VIEW_SKIP_TEST_27, FACTION_AS_LIKE, "ai_view_skip_27"),
    MAC_TEST(RVA_EVENT_SQUARE_TEST_A, FACTION_AS_EVENTS, "event_square_a"),  MAC_TEST(RVA_EVENT_SQUARE_TEST_B, FACTION_AS_EVENTS, "event_square_b"),
    MAC_TEST(RVA_EVENT_SQUARE_TEST_C, FACTION_AS_EVENTS, "event_square_c"),  MAC_TEST(RVA_EVENT_ARMY_TEST, FACTION_AS_EVENTS, "event_army"),
    MAC_TEST(RVA_EVENT_SQUARE_TEST_D, FACTION_AS_EVENTS, "event_square_d"),  MAC_TEST(RVA_EVENT_SQUARE_TEST_E, FACTION_AS_EVENTS, "event_square_e"),
    MAC_TEST(RVA_RANDOM_SQUARE_OWNER_TEST, FACTION_AS_EVENTS, "random_square_owner"),
    MAC_TEST(RVA_HORRORS_OWNER_TEST_B, FACTION_AS_LIKE, "horrors_owner_b"),
    MAC_TEST(RVA_CONQUER_SQUARE_OWNER_TEST, FACTION_AS_CONQUEST, "conquer_square_owner"),
    MAC_TEST(RVA_WANDERER_TARGET_CITY_TEST, FACTION_AS_STATUE, "wanderer_target_city"), MAC_TEST(RVA_IMPERIAL_STATUE_TEST, FACTION_AS_STATUE, "imperial_statue"),
    MAC_TEST(RVA_GOD_VESSEL_OWNER_TEST, FACTION_AS_LIKE, "god_vessel_owner"),
    {RVA_INDEP_AVOID_MOVER_TEST, MAC_SITE_AVOID, 0, 0, "indep_avoid"},
    MAC_TEST(RVA_GOD_VESSEL_OWNER_TEST_B, FACTION_AS_LIKE, "god_vessel_owner_b"), MAC_TEST(RVA_POPHORRORS_OWNER_TEST, FACTION_AS_LIKE, "pophorrors_owner"),
    {RVA_TITHE_EXCEPTION_TEST, MAC_SITE_LOAD_AS_ZERO, 0, 0, "tithe_pool_add"},
    {0, 0, 0, 0, NULL},
};
static int installMacPresetTests(void) {
    int ok = 0, wanted = 0;
    for (int i = 0; MAC_PRESET_TESTS[i].name; i++, wanted++) {
        const MacPresetTest *t = &MAC_PRESET_TESTS[i];
        int arg = t->kind == MAC_SITE_LOAD_AS_ZERO ? FACTION_AS_TITHE << 8 | 29 : t->as;
        ok += macSiteAdd(gameBase + t->rva, t->kind, t->field, arg, t->name);
    }
    writeLog("mac_preset_tests_installed", "ok=%d of %d", ok, wanted);
    return ok == wanted;
}
static int installFactionMovers(void) {
    gameMoveAllIndependents = GAME_FN(MoveAllIndependentsFn, RVA_MOVE_ALL_INDEPENDENTS);
    gameTurnUnitUpkeep = GAME_FN(TurnUnitUpkeepFn, RVA_TURN_UNIT_UPKEEP);
    gameWandererPickTarget = GAME_FN(WandererPickTargetFn, RVA_WANDERER_PICK_TARGET);
    gameRulershipCheck = GAME_FN(RulershipCheckFn, RVA_RULERSHIP_CHECK);
    refreshFactionWanderTables();
    int hooked = (macRedirectCalls(RVA_MOVE_ALL_INDEPENDENTS, (void *)moveAllIndependentsHook, "move_all_independents") > 0) +
                 (macRedirectCalls(RVA_TURN_UNIT_UPKEEP, (void *)turnUnitUpkeepHook, "turn_unit_upkeep") > 0) +
                 (macRedirectCalls(RVA_WANDERER_PICK_TARGET, (void *)wandererPickTargetHook, "wanderer_pick_target") > 0) +
                 (macRedirectCalls(RVA_RULERSHIP_CHECK, (void *)rulershipCheckHook, "rulership_check") > 0);
    int tests = installMacPresetTests();
    int horrors = installHorrorsSplit();
    int population = installPopulateAfterVoidHook();
    writeLog("faction_movers_installed", "hooks=%d of 4 preset_tests=%d horrors=%d population=%d", hooked, tests, horrors, population);
    return hooked == 4 && tests && horrors && population;
}
#endif
// A faction slot's turn state, for its save chunk (faction_saves.h).
static void saveFactionTurnState(SaveBuffer *b, int slot) {
    saveBufferInt(b, factionTurns[slot - PLAYER_SLOTS_GAME].active);
    saveBufferInt(b, factionTurns[slot - PLAYER_SLOTS_GAME].ownGateway);
}
static int readFactionTurnState(SaveCursor *c, int slot) {
    int active = saveCursorInt(c), own = saveCursorInt(c);
    if (c->failed) return 0;
    factionTurns[slot - PLAYER_SLOTS_GAME].active = active, factionTurns[slot - PLAYER_SLOTS_GAME].ownGateway = own;
    refreshFactionWanderTables();
    return 1;
}

// ---- Population ----------------------------------------------------------------------------------
// A faction's share of its home plane's population.
static void giveFactionsTheirPlane(const CustomPlane *p) {
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    for (int i = 0; i < factionCount; i++) {
        const Faction *f = &factions[i];
        if (!f->populate || strcmp(f->home, p->name)) continue;
        int slot = factionSlotOf(f), units = 0, squares = 0, last = G32(RVA_UNIT_COUNT);
        for (int u = 0; u <= last; u++) {
            size_t at = (size_t)u * UNIT_SIZE;
            int x = (int)G32(RVA_UNIT_X + at), y = (int)G32(RVA_UNIT_X + at + 4);
            if ((short)G16(RVA_UNIT_HP + at) <= 0 || (short)G16(RVA_UNIT_PLAYER + at) != f->like) continue;
            if (x < x0 || y < y0 || x >= x0 + p->w || y >= y0 + p->h) continue;
            *(short *)GAME_PTR(RVA_UNIT_PLAYER + at) = (short)slot;
            units++;
        }
        for (int x = x0; x < x0 + p->w; x++)
            for (int y = y0; y < y0 + p->h; y++) {
                unsigned char *square = squareRecordAt(x, y);
                if ((signed char)square[SQUARE_OWNER] == f->like) square[SQUARE_OWNER] = (unsigned char)slot, squares++;
                if (G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2)) rebuildSquareArmyHook(x, y);
            }
        writeLog("faction_population", "faction=%s slot=%d plane=%s from=%d units=%d squares=%d", f->name, slot, p->name, f->like, units, squares);
    }
}

// The world squares of the plane named `name`; 0 for none.
static int factionPlaneArea(const char *name, int *x0, int *y0, int *x1, int *y1) {
    int depth = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H);
    if (depth > MAP_COLUMNS_MAX) depth = MAP_COLUMNS_MAX;
    if (height > MAP_COLUMN_SQUARES) height = MAP_COLUMN_SQUARES;
    *x0 = *y0 = 0, *x1 = depth, *y1 = height;
    if (!name[0]) return 1;
    int plane = planeIndexNamed(name);
    if (plane < 0) return 0;
    *x0 = planeField(plane, PLANE_X), *y0 = planeField(plane, PLANE_Y);
    *x1 = *x0 + planeField(plane, PLANE_W), *y1 = *y0 + planeField(plane, PLANE_H);
    if (*x1 > depth) *x1 = depth;
    if (*y1 > height) *y1 = height;
    return 1;
}

// A faction's own units and terrain stock, after the world's last population.
static void populateFactions(void) {
    int any = 0;
    for (int i = 0; i < factionCount; i++) any |= factions[i].unitCount || factions[i].takeCount;
    if (!any) return;
    unsigned char *mark = calloc((size_t)MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES, 1);
    if (!mark) {
        writeLog("faction_population_skipped", "reason=out_of_memory");
        return;
    }
    PieceTools t = pieceTools();
    int wilder = G32(RVA_WILDER) > 0 ? G32(RVA_WILDER) : 1, x0, y0, x1, y1;
    for (int i = 0; i < factionCount; i++)
        for (int k = 0; k < factions[i].takeCount; k++) {
            const FactionTake *take = &factions[i].takes[k];
            if (!factionPlaneArea(take->plane, &x0, &y0, &x1, &y1)) {
                writeLog("faction_terrain_skipped", "faction=%s terrain=%d plane=%s reason=no_such_plane", factions[i].name, take->terrain, take->plane);
                continue;
            }
            int squares = 0;
            for (int x = x0; x < x1; x++)
                for (int y = y0; y < y1; y++) {
                    size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
                    if (mark[at] || *(signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) != PLANE_UNITS_PLAYER || ritualTerrainAt(take->terrain, x, y) <= 0) continue;
                    mark[at] = (unsigned char)(i + 1), squares++;
                }
            writeLog("faction_terrain_taken", "faction=%s terrain=%d plane=%s squares=%d", factions[i].name, take->terrain, take->plane[0] ? take->plane : "-", squares);
        }
    for (int i = 0; i < factionCount; i++)
        for (int k = 0; k < factions[i].unitCount; k++) {
            const FactionUnits *u = &factions[i].units[k];
            int monster = monsterByName(u->monster);
            const char *skip = monster < 0 ? "no_such_monster" : !factionPlaneArea(u->plane, &x0, &y0, &x1, &y1) ? "no_such_plane" : NULL;
            if (skip) {
                writeLog("faction_units_skipped", "faction=%s monster=%s plane=%s reason=%s", factions[i].name, u->monster, u->plane[0] ? u->plane : "-", skip);
                continue;
            }
            unsigned long long moves = 0;
            memcpy(&moves, monsterRecord(monster) + MONSTER_MOVE_FLAGS, sizeof moves);
            int squares = 0, groups = 0;
            for (int x = x0; x < x1; x++)
                for (int y = y0; y < y1; y++) {
                    size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
                    const unsigned char *square = squareRecordAt(x, y);
                    int owner = *(const signed char *)(square + SQUARE_OWNER), terrain = *(const short *)(square + SQUARE_TERRAIN_TYPE);
                    short apcost = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? (short)G16(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_APCOST) : 0;
                    if (mark[at] || (owner >= 0 && owner != PLANE_UNITS_PLAYER) || G16(RVA_SQUARE_ARMY + at * 2) || apcost >= TERRAIN_APCOST_WALL) continue;
                    if (u->terrain != -1 && ritualTerrainAt(u->terrain, x, y) <= 0) continue;
                    squares++;
                    if (!monsterFitsSquare(moves, squareIsWater(x, y)) || t.random(100) >= u->chance) continue;
                    t.populate(x, y, u->monster, u->count * wilder, PLANE_UNITS_PLAYER, -1);
                    mark[at] = (unsigned char)(i + 1), groups++;
                }
            writeLog("faction_units_stocked", "faction=%s monster=%s terrain=%d plane=%s squares=%d groups=%d units=%d wilder=%d", factions[i].name, u->monster,
                     u->terrain, u->plane[0] ? u->plane : "-", squares, groups, groups * u->count * wilder, wilder);
        }
    int units[FACTION_SLOT_TARGET] = {0}, squares[FACTION_SLOT_TARGET] = {0}, last = G32(RVA_UNIT_COUNT);
    for (int n = 0; n <= last; n++) {
        size_t at = (size_t)n * UNIT_SIZE;
        int x = (int)G32(RVA_UNIT_X + at), y = (int)G32(RVA_UNIT_X + at + 4);
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || (short)G16(RVA_UNIT_PLAYER + at) != PLANE_UNITS_PLAYER) continue;
        if (x < 0 || y < 0 || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) continue;
        int i = mark[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y] - 1;
        if (i < 0) continue;
        *(short *)GAME_PTR(RVA_UNIT_PLAYER + at) = (short)factionSlotOf(&factions[i]);
        units[i]++;
    }
    for (int x = 0; x < MAP_COLUMNS_MAX; x++)
        for (int y = 0; y < MAP_COLUMN_SQUARES; y++) {
            size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            if (!mark[at]) continue;
            squareRecordAt(x, y)[SQUARE_OWNER] = (unsigned char)factionSlotOf(&factions[mark[at] - 1]);
            squares[mark[at] - 1]++;
            if (G16(RVA_SQUARE_ARMY + at * 2)) rebuildSquareArmyHook(x, y);
        }
    free(mark);
    for (int i = 0; i < factionCount; i++)
        if (units[i] || squares[i])
            writeLog("faction_population_given", "faction=%s slot=%d units=%d squares=%d", factions[i].name, factionSlotOf(&factions[i]), units[i], squares[i]);
}
