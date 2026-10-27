// Mods' factions' sight: the game's sight steps for a faction slot, in its own per-square arrays.
#pragma once

#define FACTION_SIGHT_SKY_HOLE_A 0xfb
#define FACTION_SIGHT_SKY_HOLE_B 0x132

typedef void (*UpdateSightFn)(int player, int x0, int y0, int x1, int y1);
typedef void (*SightRaysFn)(int x, int y, int player, int range);
typedef void (*SightBoxFn)(int x, int y, int player, int radius, int level);
typedef int (*PlaneSeenFn)(int player, int plane);
typedef void (*ResetSquaresFn)(void);
static UpdateSightFn gameUpdateSight;
static SightRaysFn gameSightRays;
static SightBoxFn gameSightBox;
static PlaneSeenFn gamePlaneSeen;
static ResetSquaresFn gameResetSquares;

// A faction slot's SQUARE_SEEN for square (x, y).
static signed char *factionSeen(int x, int y, int faction) { return (signed char *)factionSquareSeen + factionSquareIndex(x, y, faction); }

// Raises a faction slot's SQUARE_SEEN to `level`, never lowering it (the game's signed byte compare).
static void raiseFactionSeen(int x, int y, int faction, int level) {
    signed char *seen = factionSeen(x, y, faction);
    if (*seen <= level) *seen = (signed char)level;
}

// The first plane (0 to RVA_LAST_PLANE) whose rectangle holds the square, or -1: the game's inline scan.
static int planeHoldingSquare(int x, int y) {
    int last = G32(RVA_LAST_PLANE);
    for (int p = 0; p <= last && p < planeTableRecords(); p++) {
        int px = planeField(p, PLANE_X), py = planeField(p, PLANE_Y);
        if (x >= px && x < px + planeField(p, PLANE_W) && y >= py && y < py + planeField(p, PLANE_H)) return p;
    }
    return -1;
}

static int squareInMap(int x, int y) { return x >= 0 && y >= 0 && x < G32(RVA_MAP_W) && y < G32(RVA_MAP_H); }

// A square's record field, the terrain word at RVA_SQUARES and the flags at RVA_SQUARE_FLAGS.
static unsigned char *sightSquare(int x, int y) { return GAME_PTR(RVA_SQUARE_RECORDS) + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE; }
static short sightTerrain(int x, int y) { return (short)G16(RVA_SQUARES + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE); }
static int sightSquareFlags(int x, int y) { return (int)G32(RVA_SQUARE_FLAGS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE); }
static unsigned long long sightTerrainFlags(int terrain) {
    unsigned long long flags;
    memcpy(&flags, GAME_PTR(RVA_TERRAIN_TABLE) + (size_t)terrain * TERRAIN_SIZE + TERRAIN_FLAGS, sizeof flags);
    return flags;
}

// ---- The game's helpers the faction paths call ---------------------------------------------------
// @platform-split sight_helpers: Mac inlines these into RVA_UPDATE_SIGHT, so CoE5SE's copies run there
#ifndef __APPLE__
static int sightFirstTeamUnitAt(int player, int x, int y) { return GAME_FN(int (*)(int, int, int), RVA_FIRST_TEAM_UNIT_AT)(player, x, y); }
static int sightTeamFarsight(int player, int x, int y) { return GAME_FN(int (*)(int, int, int), RVA_SQUARE_TEAM_ABILITY)(player, x, y); }
static int sightTeamFarsightSea(int player, int x, int y) { return GAME_FN(int (*)(int, int, int), RVA_SQUARE_TEAM_ABILITY_B)(player, x, y); }
static int sightSourceNear(int player, int x, int y) { return GAME_FN(int (*)(int, int, int), RVA_SIGHT_SOURCE_NEAR)(player, x, y); }
static int sightSquareOwnerAt(int x, int y) { return GAME_FN(int (*)(int, int), RVA_SQUARE_OWNER_AT)(x, y); }
// The game's RVA_UPDATE_SIGHT, which the entry hook runs for every caller, CoE5SE's too.
static void sightUpdate(int player, int x0, int y0, int x1, int y1) { GAME_FN(UpdateSightFn, RVA_UPDATE_SIGHT)(player, x0, y0, x1, y1); }
#else
// RVA_FIRST_TEAM_UNIT_AT: the first living unit on (x, y) of the player's team, -1 none (x below 0: none).
static int sightFirstTeamUnitAt(int player, int x, int y) {
    if (x < 0) return -1;
    int last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y) continue;
        if (playerSameTeam((short)G16(RVA_UNIT_PLAYER + at), player)) return u;
    }
    return -1;
}

// The most of an ability over a team's living units on a square.
static int sightTeamAbility(int player, int x, int y, int ability) {
    if (x < 0 || y < 0) return 0;
    int best = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0 || G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y) continue;
        if (player != -1 && !playerSameTeam(player, (short)G16(RVA_UNIT_PLAYER + at))) continue;
        int v = GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL)(u, ability, 0, NULL, 0, NULL);
        if (v > best) best = v;
    }
    return best;
}
static int sightTeamFarsight(int player, int x, int y) { return sightTeamAbility(player, x, y, ABILITY_FARSIGHT); }
static int sightTeamFarsightSea(int player, int x, int y) { return sightTeamAbility(player, x, y, ABILITY_FARSIGHTSEA); }

// Whether a team's sight source is within 3 squares.
static int sightSourceNear(int player, int x, int y) {
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int tx = x - 3; tx <= x + 3; tx++)
        for (int ty = y - 3; ty <= y + 3; ty++) {
            if (tx < 0 || ty < 0 || tx >= mapW || ty >= mapH) continue;
            short terrain = sightTerrain(tx, ty);
            if (terrain < 0) continue;
            int owner = (signed char)sightSquare(tx, ty)[SQUARE_OWNER];
            if (owner >= 0 && playerSameTeam(player, owner) && !(sightTerrainFlags(terrain) & TERRAIN_FLAG_SIGHT_SQUARE_ONLY)) return 1;
            short army = (short)G16(RVA_SQUARE_ARMY + ((size_t)tx * MAP_COLUMN_SQUARES + (size_t)ty) * 2), armyOwner;
            memcpy(&armyOwner, sightSquare(tx, ty) + SQUARE_ARMY_OWNER, 2);
            if (army && (army == -1 || playerSameTeam(player, armyOwner)) && sightFirstTeamUnitAt(player, tx, ty) >= 0) return 1;
        }
    return 0;
}

// RVA_SQUARE_OWNER_AT: SQUARE_OWNER, -1 off RVA_MAP_W x RVA_MAP_H.
static int sightSquareOwnerAt(int x, int y) { return squareInMap(x, y) ? (signed char)sightSquare(x, y)[SQUARE_OWNER] : -1; }

static void updateSightHook(int player, int x0, int y0, int x1, int y1);
// RVA_UPDATE_SIGHT with the faction slots: the hook, as every redirected caller runs it.
static void sightUpdate(int player, int x0, int y0, int x1, int y1) { updateSightHook(player, x0, y0, x1, y1); }
#endif

// RVA_SIGHT_RAYS for a faction slot: the square in view, then rays toward the edge of the box.
static void factionSightRays(int x, int y, int faction, int range) {
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y);
    if (plane < 0) return;
    int step = range >= 3 ? 0x40 : 0x80;
    for (int dx = -0x100; dx <= 0x100; dx += step)
        for (int dy = -0x100; dy <= 0x100; dy += step) {
            if (abs(dx) != 0x100 && abs(dy) != 0x100) continue;
            raiseFactionSeen(x, y, faction, 2);
            for (int i = 1, ax = dx, ay = dy; i <= range; i++, ax += dx, ay += dy) {
                int tx = x + ax / 0x100, ty = y + ay / 0x100;
                if (planeHoldingSquare(tx, ty) != plane) break;
                short terrain = squareInMap(tx, ty) ? sightTerrain(tx, ty) : -1;
                unsigned long long flags = terrain >= 0 ? sightTerrainFlags(terrain) : 0;
                raiseFactionSeen(tx, ty, faction, 2);
                if (!(flags & TERRAIN_FLAG_SEEPAST)) break;
            }
        }
}

// RVA_SIGHT_BOX for a faction slot: every square within `radius` on the square's plane at least `level`.
static void factionSightBox(int x, int y, int faction, int radius, int level) {
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y);
    if (plane < 0) return;
    for (int tx = x - radius; tx <= x + radius; tx++)
        for (int ty = y - radius; ty <= y + radius; ty++)
            if (planeHoldingSquare(tx, ty) == plane) raiseFactionSeen(tx, ty, faction, level);
}

// A faction slot's part of RVA_UPDATE_SIGHT over the box: sight from its team's squares and armies.
static void factionSightPass(int faction, int x0, int y0, int x1, int y1) {
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) {
            short terrain = sightTerrain(x, y);
            if (terrain < 0) continue;
            unsigned char *record = sightSquare(x, y);
            int valid = x >= 0 && y >= 0 && x < mapW && y < mapH;
            int owner = valid ? (signed char)record[SQUARE_OWNER] : -1;
            int ownTeam = owner >= 0 && playerSameTeam(faction, owner);
            if (ownTeam) {
                if (!(sightTerrainFlags(terrain) & TERRAIN_FLAG_SIGHT_SQUARE_ONLY) && !(sightSquareFlags(x, y) & SQUARE_FLAG_NO_SIGHT))
                    factionSightRays(x, y, faction, 2);
                else if (valid)
                    raiseFactionSeen(x, y, faction, 2);
            }
            short army = (short)G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2);
            short armyOwner;
            memcpy(&armyOwner, record + SQUARE_ARMY_OWNER, 2);
            if (army && (army == -1 || armyOwner == -1 || playerSameTeam(faction, armyOwner)) &&
                sightFirstTeamUnitAt(faction, x, y) >= 0) {
                int seeing = 0;
                if (!(sightSquareFlags(x, y) & SQUARE_FLAG_NO_SIGHT) && x >= 0 && y >= 0) {
                    int last = G32(RVA_UNIT_COUNT);
                    for (int u = 0; u <= last && u < UNIT_TABLE_MAX && !seeing; u++) {
                        size_t at = (size_t)u * UNIT_SIZE;
                        short unitOwner = (short)G16(RVA_UNIT_PLAYER + at);
                        if (!playerSameTeam(unitOwner, faction) || (short)G16(RVA_UNIT_HP + at) <= 0 || G32(RVA_UNIT_X + at) != x ||
                            G32(RVA_UNIT_X + 4 + at) != y)
                            continue;
                        seeing = GAME_FN(int (*)(int), RVA_UNIT_BLINDNESS)(u) <= 1;
                    }
                }
                if (seeing) {
                    factionSightRays(x, y, faction, 2);
                    int farsight = ownTeam && (sightTerrainFlags(terrain) & TERRAIN_FLAG_SIGHT_FARSIGHT)
                                  ? 1
                                  : sightTeamFarsight(faction, x, y) > 0;
                    int farSea = sightTeamFarsightSea(faction, x, y) > 0;
                    if (farsight) factionSightBox(x, y, faction, 2, 2);
                    if (farSea) factionSightRays(x, y, faction, 3);
                } else {
                    if (valid) raiseFactionSeen(x, y, faction, 2);
                    factionSightBox(x, y, faction, 1, 1);
                }
            }
            if (*factionSeen(x, y, faction) < 2 && (sightTerrainFlags(terrain) & TERRAIN_FLAG_SEEN_NEAR) &&
                sightSourceNear(faction, x, y) > 0 && !(sightSquareFlags(x, y) & SQUARE_FLAG_NO_SIGHT) && valid)
                *factionSeen(x, y, faction) = 2;
        }
}

// RVA_UPDATE_SIGHT's second pass for a faction slot: the Sky's holes in view show the Elysium square below.
static void factionSkyHoles(int faction, int x0, int x1) {
    int sx = planeField(PLANE_SKY, PLANE_X), sy = planeField(PLANE_SKY, PLANE_Y), sw = planeField(PLANE_SKY, PLANE_W), sh = planeField(PLANE_SKY, PLANE_H);
    if (x1 < sx || x0 >= sx + sw) return;
    int ex = planeField(0, PLANE_X), ey = planeField(0, PLANE_Y);
    for (int x = sx; x < sx + sw; x++)
        for (int y = sy; y < sy + sh; y++) {
            short terrain = sightTerrain(x, y);
            if ((terrain != FACTION_SIGHT_SKY_HOLE_A && terrain != FACTION_SIGHT_SKY_HOLE_B) || *factionSeen(x, y, faction) < 2 ||
                (sightSquareFlags(x, y) & SQUARE_FLAG_NO_SIGHT))
                continue;
            int tx = ex - sx + x, ty = ey - sy + y;
            if (planeHoldingSquare(tx, ty) == 0) raiseFactionSeen(tx, ty, faction, 1);
        }
}

// RVA_UPDATE_SIGHT: the game's, then each faction slot the call covers.
static void updateSightHook(int player, int x0, int y0, int x1, int y1) {
    (gameUpdateSight ? gameUpdateSight : GAME_FN(UpdateSightFn, RVA_UPDATE_SIGHT))(player, x0, y0, x1, y1);
    if (!factionSlotsInUse()) return;
    int depth = G32(RVA_MAP_DEPTH), mapH = G32(RVA_MAP_H);
    int cx0 = x0 < 0 ? 0 : x0 >= depth - 1 ? depth - 1 : x0, cx1 = x1 < 0 ? 0 : x1 >= depth - 1 ? depth - 1 : x1;
    int cy0 = y0 < 0 ? 0 : y0 >= mapH - 1 ? mapH - 1 : y0, cy1 = y1 < 0 ? 0 : y1 >= mapH - 1 ? mapH - 1 : y1;
    for (int f = PLAYER_SLOTS_GAME; f < PLAYER_SLOTS_MOVED; f++) {
        if (playerSlotController(f) == CONTROLLER_REMOVED) continue;
        if (player != -1 && player != f && !playerSameTeam(player, f)) continue;
        factionSightPass(f, cx0, cy0, cx1, cy1);
        factionSkyHoles(f, cx0, cx1);
    }
}

#ifndef __APPLE__
static void sightRaysHook(int x, int y, int player, int range) {
    if (factionSlot(player)) factionSightRays(x, y, player, range);
    else gameSightRays(x, y, player, range);
}
#endif

static void sightBoxHook(int x, int y, int player, int radius, int level) {
    if (factionSlot(player)) factionSightBox(x, y, player, radius, level);
    else gameSightBox(x, y, player, radius, level);
}

#ifndef __APPLE__
// RVA_PLANE_SEEN for a faction slot: any square of the plane seen, cached in RVA_PLAYER_PLANE_SEEN.
static int planeSeenHook(int player, int plane) {
    if (!factionSlot(player)) return gamePlaneSeen(player, plane);
    if (plane < 0 || plane > PLANE_RECORDS_GAME - 1) return 0;
    signed char *cache = (signed char *)playerSlotAddress(RVA_PLAYER_PLANE_SEEN, player, PLANE_RECORDS_GAME) + plane;
    if (*cache != -1) return *cache;
    int px = planeField(plane, PLANE_X), py = planeField(plane, PLANE_Y), pw = planeField(plane, PLANE_W), ph = planeField(plane, PLANE_H);
    for (int y = py; y < py + ph; y++)
        for (int x = px; x < px + pw; x++)
            if (*factionSeen(x, y, player) > 0) return *cache = 1;
    return *cache = 0;
}
#endif

// ---- Readers a faction slot reaches --------------------------------------------------------------
typedef int (*IllusionTerrainFn)(int player, int x, int y, int *shown);
typedef int (*UnitVisibleToFn)(int player, int unit, int x, int y);
typedef void (*ShareTeamSightFn)(int player);
static IllusionTerrainFn gameIllusionTerrain;
static UnitVisibleToFn gameUnitVisibleTo;
static ShareTeamSightFn gameShareTeamSight;

#define SIGHT_UNIT_NEAR_FLAGS   0x20
#define SIGHT_UNIT_NEAR_FLAGS_B 0x10020

// Whether a unit of `player` stands on a square next to (x, y) or on it, RVA_PLAYER_UNIT_AT with `flags`.
static int playerUnitNear(int player, int x, int y, int flags) {
    for (int ty = y - 1; ty <= y + 1; ty++)
        for (int tx = x - 1; tx <= x + 1; tx++)
            if (GAME_FN(int (*)(int, int, int, int), RVA_PLAYER_UNIT_AT)(player, tx, ty, flags)) return 1;
    return 0;
}

// RVA_ILLUSION_TERRAIN for a faction slot: whether an illusion shows the square as another terrain to it.
static int factionIllusionTerrain(int player, int x, int y, int *shown) {
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    if (x >= 0 && y >= 0 && !(sightSquareFlags(x, y) & SQUARE_FLAG_ILLUSION)) return 0;
    const unsigned char *found = NULL;
    int last = G32(RVA_LAST_ENCHANT);
    for (int i = 0; i <= last && !found; i++) {
        const unsigned char *e = GAME_PTR(RVA_ENCHANTS) + (size_t)i * ENCHANT_SIZE;
        int kind, ex, ey;
        memcpy(&kind, e + ENCHANT_KIND, 4), memcpy(&ex, e + ENCHANT_X, 4), memcpy(&ey, e + ENCHANT_Y, 4);
        if (kind != ENCHANT_KIND_ILLUSION) continue;
        int match = x == -998  ? ex < 0 || ey < 0 || ex >= mapW || ey >= mapH || (signed char)sightSquare(ex, ey)[SQUARE_OWNER] == -1
                  : x == -997  ? 0
                  : x == -2    ? ex == -2
                  : x < -1     ? 1
                               : ex == x && ey == y;
        if (match) found = e;
    }
    if (!found) return 0;
    int caster;
    memcpy(&caster, found + ENCHANT_OWNER, 4);
    if (caster == player) return 0;
    int level = 2;
    if (player >= 0) {
        if (sightSquareOwnerAt(x, y) == player) return 0;
        signed char seen = x >= 0 && y >= 0 ? *factionSeen(x, y, player) : 0;
        if (x >= 0 && y >= 0 && seen > 0 && seen >= 3) return 0;
        if (x >= 0 && y >= 0 && seen == 2 && playerUnitNear(player, x, y, SIGHT_UNIT_NEAR_FLAGS_B) && playerUnitNear(player, x, y, SIGHT_UNIT_NEAR_FLAGS))
            return 0;
    }
    if (playerSameTeam(caster, player) && x >= 0 && y >= 0 && G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2)) {
        int count = G32(RVA_UNIT_COUNT);
        for (int u = 0; u <= count && u < UNIT_TABLE_MAX; u++) {
            size_t at = (size_t)u * UNIT_SIZE;
            short owner = (short)G16(RVA_UNIT_PLAYER + at);
            if (owner == PLAYER_SLOTS_GAME - 1 || (short)G16(RVA_UNIT_HP + at) <= 0 || G32(RVA_UNIT_X + at) != x || G32(RVA_UNIT_X + 4 + at) != y) continue;
            if (player == -1 || owner == player) return 0;
        }
    }
    int valid = x >= 0 && y >= 0 && x < mapW && y < mapH;
    short terrain = valid ? sightTerrain(x, y) : -1, ground = -1;
    if (valid) memcpy(&ground, sightSquare(x, y) + SQUARE_GROUND, 2);
    if (terrain < 0) return 0;
    if (terrain == 0x30 || terrain == 0x3b || terrain == 0xa8 || terrain == 0xa9 || terrain == 0x118 || terrain == 0x119) level = 0x43;
    else {
        unsigned long long flags = sightTerrainFlags(terrain);
        level = flags & (1ull << 16) ? 0xc
              : flags & (1ull << 13) ? level
              : flags & (1ull << 47) ? 0x73
              : flags & 8            ? 0x75
              : flags & (1ull << 58) ? 0x45
              : flags & 4            ? 0x1b
              : ground == 3          ? 0x77
              : ground == 4          ? 0x75
              : ground == 2          ? 0x73
                                     : 0;
    }
    if (shown) *shown = level;
    return 1;
}

#ifndef __APPLE__
// RVA_UNIT_VISIBLE_TO for a faction slot: its own units and those without stealth always; else by its sight.
static int factionUnitVisibleTo(int player, int unit, int x, int y) {
    if (unit < 0 || unit > G32(RVA_UNIT_COUNT) || x < 0 || y < 0) return 0;
    if ((short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE) == player) return 1;
    int stealth = GAME_FN(int (*)(int), RVA_UNIT_STEALTH_LEVEL)(unit);
    if (stealth <= 0) return 1;
    signed char seen = *factionSeen(x, y, player);
    if (seen == 3) return 1;
    if (seen <= 1) return 0;
    return playerUnitNear(player, x, y, stealth >= 2 ? SIGHT_UNIT_NEAR_FLAGS : SIGHT_UNIT_NEAR_FLAGS_B);
}
#endif

// The team sight share for a faction slot, at its turn's start.
static void factionShareTeamSight(int faction) {
    int depth = G32(RVA_MAP_DEPTH), mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int ally = 0; ally < PLAYER_SEATS; ally++) {
        short controller = playerSlotController(ally);
        if ((unsigned short)controller > CONTROLLER_AI || !playerSameTeam(ally, faction)) continue;
        for (int x = 0; x < depth; x++)
            for (int y = 0; y < mapH; y++) {
                signed char theirs = (signed char)sightSquare(x, y)[SQUARE_SEEN + ally];
                if (theirs > *factionSeen(x, y, faction)) *factionSeen(x, y, faction) = theirs;
            }
    }
    for (int x = 0; x < mapW; x++)
        for (int y = 0; y < mapH; y++)
            if (*factionSeen(x, y, faction) >= 2) *factionSeen(x, y, faction) = 1;
    sightUpdate(faction, 0, 0, depth - 1, mapH - 1);
    for (int x = 0; x < depth; x++)
        for (int y = 0; y < mapH; y++) {
            signed char *trueSeen = (signed char *)factionSquareTrueSeen + factionSquareIndex(x, y, faction);
            if (*factionSeen(x, y, faction) >= 1 && *trueSeen <= 0 && !factionIllusionTerrain(faction, x, y, NULL)) *trueSeen = 1;
        }
}

static int illusionTerrainHook(int player, int x, int y, int *shown) {
    return factionSlot(player) ? factionIllusionTerrain(player, x, y, shown) : gameIllusionTerrain(player, x, y, shown);
}
#ifndef __APPLE__
static int unitVisibleToHook(int player, int unit, int x, int y) {
    return factionSlot(player) ? factionUnitVisibleTo(player, unit, x, y) : gameUnitVisibleTo(player, unit, x, y);
}
#endif
static void shareTeamSightHook(int player) {
    if (factionSlot(player)) factionShareTeamSight(player);
    else gameShareTeamSight(player);
}

#ifndef __APPLE__
// RVA_RESET_SQUARES: the game's, then the faction slots' arrays as it leaves its own (seen 0, marks and AI memory -1).
static void resetSquaresHook(void) {
    gameResetSquares();
    if (!factionSquareSeen) return;
    size_t entries = (size_t)FACTION_SQUARES * FACTION_SLOT_TARGET;
    memset(factionSquareSeen, 0, entries);
    memset(factionSquareTrueSeen, 0, entries);
    memset(factionSquareSeenMark, 0xff, entries);
    memset(factionSquareAiStrength, 0xff, entries * sizeof(int));
}
#endif

// Hooks the sight functions; the number in.
#ifndef __APPLE__
static int installFactionSightHooks(void) {
    static const struct { unsigned int rva; void *hook; void **game; } HOOKS[] = {
        {RVA_UPDATE_SIGHT, (void *)updateSightHook, (void **)&gameUpdateSight},
        {RVA_SIGHT_RAYS, (void *)sightRaysHook, (void **)&gameSightRays},
        {RVA_SIGHT_BOX, (void *)sightBoxHook, (void **)&gameSightBox},
        {RVA_PLANE_SEEN, (void *)planeSeenHook, (void **)&gamePlaneSeen},
        {RVA_RESET_SQUARES, (void *)resetSquaresHook, (void **)&gameResetSquares},
        {RVA_ILLUSION_TERRAIN, (void *)illusionTerrainHook, (void **)&gameIllusionTerrain},
        {RVA_UNIT_VISIBLE_TO, (void *)unitVisibleToHook, (void **)&gameUnitVisibleTo},
        {RVA_SHARE_TEAM_SIGHT, (void *)shareTeamSightHook, (void **)&gameShareTeamSight},
    };
    int ok = 0;
    for (size_t i = 0; i < sizeof HOOKS / sizeof HOOKS[0]; i++) {
        void *target = gameBase + HOOKS[i].rva;
        ok += fanpatchHookCreate(target, HOOKS[i].hook, HOOKS[i].game) && fanpatchHookEnable(target, 1);
    }
    return ok;
}
_Static_assert(FACTION_SIGHT_HOOK_COUNT == 8, "factions.h counts the hooks");
#else
static int installFactionSightHooks(void) {
    gameUpdateSight = GAME_FN(UpdateSightFn, RVA_UPDATE_SIGHT);
    gameSightBox = GAME_FN(SightBoxFn, RVA_SIGHT_BOX);
    gameIllusionTerrain = GAME_FN(IllusionTerrainFn, RVA_ILLUSION_TERRAIN);
    gameShareTeamSight = GAME_FN(ShareTeamSightFn, RVA_SHARE_TEAM_SIGHT);
    int ok = (macRedirectCalls(RVA_UPDATE_SIGHT, (void *)updateSightHook, "update_sight") > 0) +
             (macRedirectCalls(RVA_SIGHT_BOX, (void *)sightBoxHook, "sight_box") > 0) +
             (macRedirectCalls(RVA_ILLUSION_TERRAIN, (void *)illusionTerrainHook, "illusion_terrain") > 0) +
             (macRedirectCalls(RVA_SHARE_TEAM_SIGHT, (void *)shareTeamSightHook, "share_team_sight") > 0);
    return ok;
}
#endif

#ifndef __APPLE__
// ---- Stubs at single instructions ----------------------------------------------------------------
// Stubs that give the faction slots their own per-square bytes.
typedef struct { unsigned int rva; unsigned char array; } FactionRedirectSite;
static const FactionRedirectSite FACTION_REDIRECT_SITES[] = {
    {RVA_WAYTOGO_BACK_SEEN_READ, SQUARE_SEEN},             {RVA_SPREAD_PORTAL_SEEN_READ, SQUARE_SEEN},
    {RVA_FAR_CHECK_SEEN_READ, SQUARE_SEEN},                {RVA_FAR_CHECK_IN_VIEW_READ, SQUARE_SEEN},
    {RVA_STEP_PORTAL_SEEN_READ, SQUARE_SEEN},              {RVA_RANDOM_SQUARE_SEEN_READ, SQUARE_SEEN},
    {RVA_WANDERER_TRUE_SEEN_READ, SQUARE_TRUE_TERRAIN_SEEN}, {RVA_WANDERER_TRUE_SEEN_READ_B, SQUARE_TRUE_TERRAIN_SEEN},
    {RVA_TOP_UNIT_SEEN_READ, SQUARE_SEEN},                 {RVA_TOP_UNIT_SEEN_READ_B, SQUARE_SEEN},
    {RVA_RITUAL_PORTAL_SEEN_WRITE, SQUARE_SEEN},           {RVA_RITUAL_PORTAL_SEEN_WRITE_B, SQUARE_SEEN},
    {RVA_RITUAL_PORTAL_SEEN_WRITE_C, SQUARE_SEEN},
};
#define FACTION_REDIRECT_COUNT (int)(sizeof FACTION_REDIRECT_SITES / sizeof FACTION_REDIRECT_SITES[0])
typedef struct { unsigned int rva; unsigned char array, value; } FactionMirrorSite;
static const FactionMirrorSite FACTION_MIRROR_SITES[] = {
    {RVA_METEOR_SEEN_WRITE, SQUARE_SEEN, 1}, {RVA_UPDATE_VOID_SEEN_CLEAR, SQUARE_SEEN, 0},
};
#define FACTION_MIRROR_COUNT (int)(sizeof FACTION_MIRROR_SITES / sizeof FACTION_MIRROR_SITES[0])
_Static_assert(FACTION_REDIRECT_COUNT + FACTION_MIRROR_COUNT + 3 == FACTION_STUB_COUNT, "factions.h counts the stubs");

#define FACTION_STUB_BYTES  160
#define FACTION_STUB_DIV    869866795ULL
_Static_assert(SQUARE_SIZE == 0x13c && (FACTION_STUB_DIV * SQUARE_SIZE >> 38) == 1 && FACTION_STUB_DIV * SQUARE_SIZE - (1ULL << 38) < SQUARE_SIZE, "the multiply divides by the record size");
_Static_assert((unsigned long long)FACTION_SQUARES * SQUARE_SIZE < (1ULL << 38) / 276, "every square's offset divides exactly");

// A stub being written: its bytes and the jumps still to point at its pass label.
typedef struct { unsigned char *code; int n, passJumps[4], passCount; } StubCode;
static void stubBytes(StubCode *c, const void *bytes, int size) { memcpy(c->code + c->n, bytes, (size_t)size); c->n += size; }
#define STUB(c, ...) do { const unsigned char b_[] = {__VA_ARGS__}; stubBytes(c, b_, (int)sizeof b_); } while (0)
static void stubJumpToPass(StubCode *c, unsigned char cc) {
    STUB(c, 0x0f, cc, 0, 0, 0, 0);
    c->passJumps[c->passCount++] = c->n;
}
static void stubPassHere(StubCode *c) {
    for (int j = 0; j < c->passCount; j++) {
        int rel = c->n - c->passJumps[j];
        memcpy(c->code + c->passJumps[j] - 4, &rel, 4);
    }
    c->passCount = 0;
}
// `jmp target` or `j<cc> target` (cc 0 for jmp), rel32.
static void stubJumpTo(StubCode *c, unsigned char cc, unsigned char *target) {
    if (cc) STUB(c, 0x0f, cc);
    else STUB(c, 0xe9);
    int rel = (int)(target - (c->code + c->n + 4));
    stubBytes(c, &rel, 4);
}

// An instruction with one memory operand: where its parts are, and the registers it names.
typedef struct { int length, rex, opcodeAt, opcodeLength, modrmAt, memLength, immLength, reg, regIsRegister, base, index; } SlotOp;
static int decodeSlotOp(const unsigned char *at, SlotOp *op) {
    int i = 0;
    memset(op, 0, sizeof *op);
    if (at[i] == 0x66) i++;
    if ((at[i] & 0xf0) == 0x40) op->rex = at[i++];
    op->opcodeAt = i;
    int opcode = at[i], wide = at[0] == 0x66 ? 2 : 4;
    if (opcode == 0x0f && (at[i + 1] == 0xb6 || at[i + 1] == 0xbe)) op->opcodeLength = 2, op->regIsRegister = 1;
    else if (opcode == 0x80 || opcode == 0xc6 || opcode == 0x83) op->opcodeLength = 1, op->immLength = 1;
    else if (opcode == 0x38 || opcode == 0x88) op->opcodeLength = 1, op->regIsRegister = 1;
    else if (opcode == 0xc7) op->opcodeLength = 1, op->immLength = wide;
    else return 0;
    i += op->opcodeLength;
    op->modrmAt = i;
    int modrm = at[i], mod = modrm >> 6, rm = modrm & 7;
    op->reg = ((modrm >> 3) & 7) | (op->rex & 4 ? 8 : 0);
    if (((opcode == 0x80 || opcode == 0x83) && (op->reg & 7) != 7) || ((opcode == 0xc6 || opcode == 0xc7) && (op->reg & 7) != 0)) return 0;
    if ((opcode == 0x38 || opcode == 0x88) && !op->rex && op->reg >= 4) return 0;
    if (mod == 3 || (mod == 0 && rm == 5)) return 0;
    op->index = -1;
    if (rm == 4) {
        int sib = at[i + 1];
        if (mod == 0 && (sib & 7) == 5) return 0;
        op->base = (sib & 7) | (op->rex & 1 ? 8 : 0);
        int index = ((sib >> 3) & 7) | (op->rex & 2 ? 8 : 0);
        if (index != 4) op->index = index;
        op->memLength = 2;
    } else {
        op->base = rm | (op->rex & 1 ? 8 : 0);
        op->memLength = 1;
    }
    if (op->base == 4) return 0;
    op->memLength += mod == 1 ? 1 : mod == 2 ? 4 : 0;
    op->length = i + op->memLength + op->immLength;
    return 1;
}

// A scratch register r8..r11 the instruction doesn't name (their low bits are never 4 or 5).
static int slotOpScratch(const SlotOp *op) {
    int s = 8;
    while (s == op->base || s == op->index || (op->regIsRegister && s == op->reg)) s++;
    return s;
}

// A stub's common start: the offset within the square's record, or a jump to pass.
static void stubSquareOffset(StubCode *c, const unsigned char *at, const SlotOp *op, int s, unsigned char *first) {
    int lo = s & 7;
    STUB(c, 0x41, (unsigned char)(0x50 | lo), 0x9c);
    STUB(c, (unsigned char)(0x4c | (op->rex & 3)), 0x8d, (unsigned char)((at[op->modrmAt] & 0xc7) | lo << 3));
    stubBytes(c, at + op->modrmAt + 1, op->memLength - 1);
    STUB(c, 0x50, 0x51, 0x52, 0x4c, 0x89, (unsigned char)(0xc0 | lo << 3));
    unsigned long long from = (unsigned long long)(uintptr_t)first;
    unsigned int squaresBytes = (unsigned int)FACTION_SQUARES * SQUARE_SIZE, mul = (unsigned int)FACTION_STUB_DIV, size = SQUARE_SIZE;
    STUB(c, 0x48, 0xb9);
    stubBytes(c, &from, 8);
    STUB(c, 0x48, 0x29, 0xc8);
    stubJumpToPass(c, 0x82);
    STUB(c, 0x48, 0x3d);
    stubBytes(c, &squaresBytes, 4);
    stubJumpToPass(c, 0x83);
    STUB(c, 0x48, 0x69, 0xd0);
    stubBytes(c, &mul, 4);
    STUB(c, 0x48, 0xc1, 0xea, 38, 0x48, 0x69, 0xca);
    stubBytes(c, &size, 4);
    STUB(c, 0x48, 0x29, 0xc8);
}

// rdx = side + square * 16 (rax kept, rcx used).
static void stubSideRow(StubCode *c, unsigned char *side) {
    unsigned long long sideAt = (unsigned long long)(uintptr_t)side;
    STUB(c, 0x48, 0xc1, 0xe2, 4, 0x48, 0xb9);
    stubBytes(c, &sideAt, 8);
    STUB(c, 0x48, 0x01, 0xca);
}

// `pop rdx, rcx, rax; popfq`.
static void stubRestore(StubCode *c) { STUB(c, 0x5a, 0x59, 0x58, 0x9d); }

// A redirect stub for the instruction at `at`; its length, or 0 when the instruction isn't one this takes.
static int buildRedirectStub(unsigned char *at, unsigned char array, unsigned char *side, unsigned char *stub, int *length) {
    SlotOp op;
    if (!decodeSlotOp(at, &op)) return 0;
    *length = op.length;
    int s = slotOpScratch(&op), lo = s & 7;
    StubCode c = {stub, 0, {0}, 0};
    stubSquareOffset(&c, at, &op, s, gameBase + RVA_SQUARE_RECORDS + array + PLAYER_SLOTS_GAME);
    STUB(&c, 0x48, 0x83, 0xf8, FACTION_SLOT_TARGET);
    stubJumpToPass(&c, 0x83);
    stubSideRow(&c, side);
    STUB(&c, 0x48, 0x01, 0xc2, 0x49, 0x89, (unsigned char)(0xd0 | lo));
    stubPassHere(&c);
    stubRestore(&c);
    if (at[0] == 0x66) STUB(&c, 0x66);
    STUB(&c, (unsigned char)(0x41 | (op.rex & 0x0c)));
    stubBytes(&c, at + op.opcodeAt, op.opcodeLength);
    STUB(&c, (unsigned char)((at[op.modrmAt] & 0x38) | lo));
    stubBytes(&c, at + op.length - op.immLength, op.immLength);
    STUB(&c, 0x41, (unsigned char)(0x58 | lo));
    stubJumpTo(&c, 0, at + op.length);
    return c.n;
}

// A mirror stub that also sets the faction slots' bytes; its length, or 0.
static int buildMirrorStub(unsigned char *at, unsigned char array, unsigned char value, unsigned char *side, unsigned char *stub, int *length) {
    SlotOp op;
    if (!decodeSlotOp(at, &op) || op.regIsRegister) return 0;
    *length = op.length;
    int s = slotOpScratch(&op);
    StubCode c = {stub, 0, {0}, 0};
    stubBytes(&c, at, op.length);
    stubSquareOffset(&c, at, &op, s, gameBase + RVA_SQUARE_RECORDS + array);
    STUB(&c, 0x48, 0x85, 0xc0);
    stubJumpToPass(&c, 0x85);
    stubSideRow(&c, side);
    unsigned long long bytes = 0x0101010101010101ULL * value;
    STUB(&c, 0x48, 0xb8);
    stubBytes(&c, &bytes, 8);
    STUB(&c, 0x48, 0x89, 0x02, 0x48, 0x89, 0x42, 0x08);
    stubPassHere(&c);
    stubRestore(&c);
    STUB(&c, 0x41, (unsigned char)(0x58 | (s & 7)));
    stubJumpTo(&c, 0, at + op.length);
    return c.n;
}

// The clouds' stub at `cmp bl, 0x1f; ja done` (rdi the square's record offset): a faction owner's mark 0xff too.
static int buildCloudStub(unsigned char *at, unsigned char *stub, int *length) {
    *length = 5;
    unsigned char *done = at + 5 + (signed char)at[4];
    unsigned long long marks = (unsigned long long)(uintptr_t)factionSquareSeenMark - PLAYER_SLOTS_GAME;
    unsigned int mul = (unsigned int)FACTION_STUB_DIV;
    StubCode c = {stub, 0, {0}, 0};
    STUB(&c, 0x80, 0xfb, PLAYER_SLOTS_GAME - 1);
    stubJumpTo(&c, 0x86, at + 5);
    STUB(&c, 0x80, 0xfb, PLAYER_SLOTS_MOVED - 1);
    stubJumpTo(&c, 0x87, done);
    STUB(&c, 0x48, 0x69, 0xc7);
    stubBytes(&c, &mul, 4);
    STUB(&c, 0x48, 0xc1, 0xe8, 38, 0x48, 0xc1, 0xe0, 4, 0x0f, 0xb6, 0xd3, 0x48, 0x01, 0xd0, 0x48, 0xba);
    stubBytes(&c, &marks, 8);
    STUB(&c, 0xc6, 0x04, 0x10, 0xff);
    stubJumpTo(&c, 0, done);
    return c.n;
}

// RVA_AUTO_RECRUIT's stub at `lea rbp, [rax + SQUARE_SEEN_MARK]` (r14 the player): past 31, on to the loop's end.
static int buildAgingStub(unsigned char *at, unsigned char *stub, int *length) {
    *length = 7;
    StubCode c = {stub, 0, {0}, 0};
    STUB(&c, 0x41, 0x83, 0xfe, PLAYER_SLOTS_GAME - 1);
    stubJumpTo(&c, 0x87, gameBase + RVA_AUTO_RECRUIT_AGING_END);
    stubBytes(&c, at, 7);
    stubJumpTo(&c, 0, at + 7);
    return c.n;
}

static int squareSeenAtPort(int player, int x, int y);

// A stub jumping to `port`, for a function's entry; its length (5, the jump written there), or 0.
static int buildPortStub(void *port, unsigned char *stub, int *length) {
    *length = 5;
    StubCode c = {stub, 0, {0}, 0};
    unsigned long long to = (unsigned long long)(uintptr_t)port;
    STUB(&c, 0xff, 0x25, 0, 0, 0, 0);
    stubBytes(&c, &to, 8);
    return c.n;
}

// Sends each instruction through its stub; the number in.
static int installFactionStubs(void) {
    if (!gameCodeMatches) {
        writeLog("faction_stubs_not_installed", "reason=game_code_differs");
        return 0;
    }
    unsigned char *stubs = allocNearBytes(gameBase + RVA_TEXT_SECTION, FACTION_STUB_COUNT * FACTION_STUB_BYTES);
    if (!stubs) {
        writeLog("faction_stubs_not_installed", "reason=no_memory_near_game");
        return 0;
    }
    unsigned int rvas[FACTION_STUB_COUNT];
    int lengths[FACTION_STUB_COUNT] = {0}, k = 0;
    for (int i = 0; i < FACTION_REDIRECT_COUNT; i++, k++) {
        const FactionRedirectSite *site = &FACTION_REDIRECT_SITES[i];
        unsigned char *side = site->array == SQUARE_SEEN ? factionSquareSeen : factionSquareTrueSeen;
        rvas[k] = site->rva;
        if (!buildRedirectStub(gameBase + site->rva, site->array, side, stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    }
    for (int i = 0; i < FACTION_MIRROR_COUNT; i++, k++) {
        const FactionMirrorSite *site = &FACTION_MIRROR_SITES[i];
        rvas[k] = site->rva;
        if (!buildMirrorStub(gameBase + site->rva, site->array, site->value, factionSquareSeen, stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    }
    rvas[k] = RVA_CLOUD_OWNER_MARK_TEST;
    if (!buildCloudStub(gameBase + rvas[k], stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    k++;
    rvas[k] = RVA_AUTO_RECRUIT_AGING_START;
    if (!buildAgingStub(gameBase + rvas[k], stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    k++;
    rvas[k] = RVA_SQUARE_SEEN_AT;
    if (!buildPortStub((void *)squareSeenAtPort, stubs + k * FACTION_STUB_BYTES, &lengths[k])) lengths[k] = 0;
    makePagesReadExecute(stubs, FACTION_STUB_COUNT * FACTION_STUB_BYTES);
    int ok = 0;
    for (k = 0; k < FACTION_STUB_COUNT; k++) {
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

// ---- Functions over every slot, and readers with no bound ----------------------------------------
// RVA_SQUARE_SEEN_AT in full: SQUARE_SEEN, a faction slot's from its own array; -1 off the map or below 0.
static int squareSeenAtPort(int player, int x, int y) {
    if (!squareInMap(x, y) || player < 0) return -1;
    if (factionSlot(player)) return *factionSeen(x, y, player);
    return (signed char)sightSquare(x, y)[SQUARE_SEEN + player];
}
#endif

// Sets a faction slot's SQUARE_SEEN to `level` over RVA_SET_SEEN_DISC's disc.
static void factionSetSeenDisc(int faction, int x, int y, float radius, int level) {
    int plane = planeHoldingSquare(x, y);
    if (plane < 0) return;
    int r = (int)((double)radius + 0.99);
    for (int ty = y - r; ty <= y + r; ty++)
        for (int tx = x - r; tx <= x + r; tx++) {
            if (planeHoldingSquare(tx, ty) != plane) continue;
            if ((float)((x - tx) * (x - tx) + (y - ty) * (y - ty)) > radius * radius) continue;
            *factionSeen(tx, ty, faction) = (signed char)level;
        }
}

// RVA_SET_SEEN_DISC: the game's, then the faction slots it covers (all with -1, a faction slot itself).
typedef void (*SetSeenDiscFn)(int player, int x, int y, float radius, int level);
static SetSeenDiscFn gameSetSeenDisc;
static void setSeenDiscHook(int player, int x, int y, float radius, int level) {
    gameSetSeenDisc(player, x, y, radius, level);
    for (int f = PLAYER_SLOTS_GAME; f < PLAYER_SLOTS_MOVED; f++)
        if (player == -1 || player == f) factionSetSeenDisc(f, x, y, radius, level);
}

// RVA_REVEAL_DISC for a faction slot: the game's steps, the faction's team in its own arrays.
static void factionRevealDisc(int player, int x, int y, float radius, int value, int flag) {
    int plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y);
    if (plane < 0) return;
    int r = (int)((double)radius + 0.99), mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int ty = y - r; ty <= y + r; ty++)
        for (int tx = x - r; tx <= x + r; tx++) {
            if ((float)((x - tx) * (x - tx) + (y - ty) * (y - ty)) > radius * radius || planeHoldingSquare(tx, ty) != plane) continue;
            for (int s = PLAYER_SLOTS_GAME; s < PLAYER_SLOTS_MOVED; s++)
                if (playerSameTeam(player, s) && squareInMap(tx, ty)) raiseFactionSeen(tx, ty, s, 2);
            *factionSeen(tx, ty, player) = (signed char)value;
        }
    if (flag >= 0)
        for (int s = PLAYER_SLOTS_GAME; s < PLAYER_SLOTS_MOVED; s++)
            if (playerSameTeam(player, s)) sightUpdate(s, 0, 0, G32(RVA_MAP_DEPTH) - 1, mapH - 1);
    int cleared = -2;
    short clearedShort = -2;
    for (int tx = 0; tx < mapW; tx++)
        for (int ty = 0; ty < mapH; ty++) {
            unsigned char *square = sightSquare(tx, ty);
            memcpy(square + 0x130, &cleared, 4);
            memcpy(square + 0x134, &cleared, 4);
            memcpy(square + 0x138, &clearedShort, 2);
        }
}

typedef void (*RevealDiscFn)(int player, int x, int y, float radius, int value, int flag);
static RevealDiscFn gameRevealDisc;
static void revealDiscHook(int player, int x, int y, float radius, int value, int flag) {
    if (factionSlot(player)) factionRevealDisc(player, x, y, radius, value, flag);
    else gameRevealDisc(player, x, y, radius, value, flag);
}

#ifndef __APPLE__
// RVA_AUTO_RECRUIT: the game's (its aging stub skips a faction slot), then a faction slot's own seen marks aged.
typedef void (*AutoRecruitFn)(int player);
static AutoRecruitFn gameAutoRecruit;
static void autoRecruitHook(int player) {
    gameAutoRecruit(player);
    int f = G32(RVA_ACTIVE_PLAYER);
    if (!factionSlot(f)) return;
    int depth = G32(RVA_MAP_DEPTH), mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H);
    for (int y = 0; y < mapH; y++)
        for (int x = 0; x < depth; x++) {
            unsigned char *mark = factionSquareSeenMark + factionSquareIndex(x, y, f);
            if (*mark <= 0x63) (*mark)++;
            if (x < mapW && (signed char)sightSquare(x, y)[SQUARE_OWNER] == f) *mark = 0;
        }
}

// Hooks the functions above; the number in.
static int installFactionSlotHooks(void) {
    static const struct { unsigned int rva; void *hook; void **game; } HOOKS[] = {
        {RVA_SET_SEEN_DISC, (void *)setSeenDiscHook, (void **)&gameSetSeenDisc},
        {RVA_REVEAL_DISC, (void *)revealDiscHook, (void **)&gameRevealDisc},
        {RVA_AUTO_RECRUIT, (void *)autoRecruitHook, (void **)&gameAutoRecruit},
    };
    int ok = 0;
    for (size_t i = 0; i < sizeof HOOKS / sizeof HOOKS[0]; i++) {
        void *target = gameBase + HOOKS[i].rva;
        ok += fanpatchHookCreate(target, HOOKS[i].hook, HOOKS[i].game) && fanpatchHookEnable(target, 1);
    }
    return ok;
}
_Static_assert(FACTION_SLOT_HOOK_COUNT == 3, "factions.h counts the hooks");
#else
static int installFactionSlotHooks(void) {
    gameSetSeenDisc = GAME_FN(SetSeenDiscFn, RVA_SET_SEEN_DISC);
    gameRevealDisc = GAME_FN(RevealDiscFn, RVA_REVEAL_DISC);
    return (macRedirectCalls(RVA_SET_SEEN_DISC, (void *)setSeenDiscHook, "set_seen_disc") > 0) +
           (macRedirectCalls(RVA_REVEAL_DISC, (void *)revealDiscHook, "reveal_disc") > 0);
}
#endif
