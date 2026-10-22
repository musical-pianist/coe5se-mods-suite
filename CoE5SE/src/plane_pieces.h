// Set pieces, zones and unit lists in mods' planes.
#pragma once

#define PLANE_PIECE_TRIES     100
#define PLANE_UNITS_PLAYER    24
#define TERRAIN_FLAG_WATER    0x4
#define MONSTER_MOVE_WATER    0x40
#define MONSTER_MOVE_NOLAND   0x200

// ---- The game's calls, which a test replaces ------------------------------------------------------
typedef struct {
    int (*random)(int n);
    int (*populate)(int x, int y, const char *name, int count, int player, int commander);
    void (*rebuildArmy)(int x, int y);
    void (*holdit)(int x, int y, int on);
    void (*changeTerrain)(int x, int y, int terrain);
    void (*setGroundRect)(int x, int y, int w, int h, int terrain);
    int (*squareOfTerrain)(int plane, int code, int *x, int *y);
    void (*populatePlane)(const CustomPlane *p);
} PieceTools;
static const PieceTools *pieceToolsForTests;

static PieceTools pieceTools(void) {
    if (pieceToolsForTests) return *pieceToolsForTests;
    return (PieceTools){
        GAME_FN(int (*)(int), RVA_RANDOM),
        GAME_FN(int (*)(int, int, const char *, int, int, int), RVA_POPULATE),
        GAME_FN(void (*)(int, int), RVA_REBUILD_SQUARE_ARMY),
        GAME_FN(void (*)(int, int, int), RVA_POPULATE_SET_HOLDIT),
        GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN),
        GAME_FN(void (*)(int, int, int, int, int), RVA_SET_GROUND_RECT),
        GAME_FN(int (*)(int, int, int *, int *), RVA_RANDOM_SQUARE_OF_TERRAIN),
        populateCustomPlane,
    };
}

// ---- The Mod Error box's lines ------------------------------------------------------------------
// A piece's missing map file, or a list's monster no mod has, with the file and line that named it.
typedef struct {
    char mod[MOD_NAME_MAX], file[MAX_PATH], text[96];
    int line, script;
} PlaneProblem;
static PlaneProblem *planeProblems[2];
static int planeProblemCount[2], planeProblemCapacity[2];

static PRINTF_FORMAT(5, 6) void addPlaneProblem(int script, const char *mod, const char *file, int line, const char *fmt, ...) {
    PlaneProblem one;
    memset(&one, 0, sizeof one);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(one.text, sizeof one.text, fmt, ap);
    va_end(ap);
    snprintf(one.mod, sizeof one.mod, "%s", mod);
    snprintf(one.file, sizeof one.file, "%s", file);
    one.line = line, one.script = script;
    int k = script ? 1 : 0;
    for (int i = 0; i < planeProblemCount[k]; i++)
        if (planeProblems[k][i].line == line && !strcmp(planeProblems[k][i].file, one.file) && !strcmp(planeProblems[k][i].text, one.text))
            return;
    if (!growList((void **)&planeProblems[k], &planeProblemCapacity[k], planeProblemCount[k] + 1, sizeof one))
        return;
    planeProblems[k][planeProblemCount[k]++] = one;
}

static void dropPassPlaneProblems(void) { planeProblemCount[0] = 0; }

// Appends each problem to the missing plane pop-up's text.
static void appendPlaneProblems(char *text, size_t size, size_t *n, int *count) {
    for (int k = 0; k < 2; k++)
        for (int i = 0; i < planeProblemCount[k]; i++) {
            const PlaneProblem *p = &planeProblems[k][i];
            if ((*count)++ == MISSING_PLANE_ENTRIES) appendText(text, size, n, "\n\n...and others.");
            if (*count > MISSING_PLANE_ENTRIES) continue;
            appendText(text, size, n, "%sError in %s '%s' at line %d\n\n%s", *n ? "\n\n" : "", p->script ? "script" : "mod", p->file, p->line, p->text);
        }
}

// ---- Declarations: the words' and coe5.addPlane's ----------------------------------------------------
// Finds map file `spec` in mod folder `folder` ("x.coem" is read as "./x.coem"): NULL, or the reason.
static const char *findPieceFile(const char *folder, const char *spec, char *path, size_t size) {
    static const char *const ENDINGS[2] = {".coem", ".coem"};
    static const char *const REASONS[4] = {"bad_file", "file_not_found", "file_wrong_case", "file_link"};
    if (!spec[0] || strlen(spec) > PLANE_ICON_SPEC_MAX) return "bad_file";
    char dotted[PLANE_ICON_SPEC_MAX + 3];
    snprintf(dotted, sizeof dotted, "%s%.200s", strncmp(spec, "./", 2) ? "./" : "", spec);
    return findModFile(folder, dotted, ENDINGS, REASONS, path, size);
}

// Whether a piece's refusal means its file isn't there, for the Mod Error box.
static int pieceFileMissing(const char *reason) { return !strcmp(reason, "file_not_found") || !strcmp(reason, "file_wrong_case"); }

// Adds a piece from map file `spec`; NULL, or the refusal's reason.
static const char *addPlanePiece(PlaneLayout *layout, const char *folder, const char *spec, int placement, long long count, long long x, long long y, int line) {
    if (layout->pieceCount >= PLANE_PIECE_LINES_MAX) return "too_many_pieces";
    if (placement == PIECE_RANDOM && (count < 1 || count > PLANE_PIECE_COUNT_MAX)) return "bad_count";
    if (placement == PIECE_AT && (x < 0 || y < 0 || x >= MAP_COLUMNS_MAX || y >= PLANE_ROWS_MAX)) return "bad_square";
    PlanePiece *piece = &layout->pieces[layout->pieceCount];
    memset(piece, 0, sizeof *piece);
    const char *reason = findPieceFile(folder, spec, piece->file, sizeof piece->file);
    if (!reason && !planeFileMapsize(piece->file, &piece->w, &piece->h)) reason = "no_mapsize";
    if (!reason && !installPlaneFileHook()) reason = "hook_not_installed";
    if (reason) return reason;
    piece->placement = placement, piece->count = placement == PIECE_RANDOM ? (int)count : 1;
    piece->x = placement == PIECE_AT ? (int)x : 0, piece->y = placement == PIECE_AT ? (int)y : 0, piece->line = line;
    layout->pieceCount++;
    return NULL;
}

// Adds a zone `name` around each copy of piece file `around`, `radius` squares out. NULL, or the reason.
static const char *addPlaneZone(PlaneLayout *layout, const char *folder, const char *name, const char *around, long long radius, int line) {
    if (layout->zoneCount >= PLANE_ZONES_MAX) return "too_many_zones";
    if (!name[0]) return "no_name";
    if (strlen(name) >= PLANE_NAME_SIZE) return "name_too_long";
    for (int z = 0; z < layout->zoneCount; z++)
        if (!strcmp(layout->zones[z].name, name)) return "name_taken";
    if (radius < 0 || radius > PLANE_ZONE_RADIUS_MAX) return "bad_radius";
    char path[MAX_PATH];
    const char *reason = findPieceFile(folder, around, path, sizeof path);
    if (reason) return reason;
    int piece = -1;
    for (int i = 0; i < layout->pieceCount && piece < 0; i++)
        if (!strcmp(layout->pieces[i].file, path)) piece = i;
    if (piece < 0) return "no_such_piece";
    PlaneZone *z = &layout->zones[layout->zoneCount++];
    memset(z, 0, sizeof *z);
    snprintf(z->name, sizeof z->name, "%s", name);
    z->piece = piece, z->radius = (int)radius, z->line = line;
    return NULL;
}

static const char *addZoneSwap(PlaneZone *z, int from, int to) {
    if (z->swapCount >= PLANE_ZONE_SWAP_MAX) return "too_many_swaps";
    z->swapFrom[z->swapCount] = (short)from, z->swapTo[z->swapCount] = (short)to;
    z->swapCount++;
    return NULL;
}

// Adds a monster to a unit list: how many with Wilder off, and the chance per square in percent.
static const char *addPlaneUnits(PlaneUnitList *list, const char *monster, long long count, long long chance, int line) {
    if (!monster[0]) return "no_name";
    if (strlen(monster) >= PLANE_UNIT_NAME_SIZE) return "bad_name";
    if (count < 1 || count > PLANE_UNIT_COUNT_MAX) return "bad_count";
    if (chance < 1 || chance > 100) return "bad_chance";
    if (list->count >= PLANE_UNIT_LINES_MAX) return "too_many_units";
    PlaneUnits *u = &list->units[list->count++];
    snprintf(u->monster, sizeof u->monster, "%s", monster);
    u->count = (int)count, u->chance = (int)chance, u->line = line;
    return NULL;
}

// Logs a refusal of a layout's entry: a .c5se line's (c5m_refused) or a script's (lua_refused).
static PRINTF_FORMAT(5, 6) void refuseLayoutLine(const PlaneLayout *layout, const char *word, const char *mod, const char *reason, const char *fmt, ...) {
    char details[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    if (layout->script) refuseLuaCall(mod, "coe5.addPlane", reason, "field=%s %s", word, details);
    else refuseC5mLine(word, mod, reason, "%s", details);
}

// At the block's or call's end, refuses pieces that don't fit the plane, with their zones.
static void finishPlaneLayout(PlaneLayout *layout, int w, int h, const char *mod) {
    int map[PLANE_PIECE_LINES_MAX], kept = 0;
    for (int i = 0; i < layout->pieceCount; i++) {
        const PlanePiece *piece = &layout->pieces[i];
        const char *reason = piece->w > w || piece->h > h                                                        ? "piece_too_large"
                           : piece->placement == PIECE_AT && (piece->x + piece->w > w || piece->y + piece->h > h) ? "off_plane"
                           : NULL;
        map[i] = reason ? -1 : kept;
        if (reason) {
            refuseLayoutLine(layout, "planepiece", mod, reason, "line=%d piece_w=%d piece_h=%d plane_w=%d plane_h=%d x=%d y=%d", piece->line, piece->w,
                             piece->h, w, h, piece->x, piece->y);
            continue;
        }
        if (kept != i) layout->pieces[kept] = layout->pieces[i];
        kept++;
    }
    layout->pieceCount = kept;
    kept = 0;
    for (int z = 0; z < layout->zoneCount; z++) {
        PlaneZone *zone = &layout->zones[z];
        if (map[zone->piece] < 0) {
            refuseLayoutLine(layout, "planezone", mod, "no_such_piece", "line=%d zone=%s", zone->line, zone->name);
            continue;
        }
        zone->piece = map[zone->piece];
        if (kept != z) layout->zones[kept] = *zone;
        kept++;
    }
    layout->zoneCount = kept;
}

// Adds a Mod Error line for each monster of the layout's lists that no mod has.
static void notePlaneLayoutProblems(const PlaneLayout *layout, const char *mod) {
    for (int z = -1; z < layout->zoneCount; z++) {
        const PlaneUnitList *list = z < 0 ? &layout->units : &layout->zones[z].units;
        for (int i = 0; i < list->count; i++)
            if (monsterByName(list->units[i].monster) < 0)
                addPlaneProblem(layout->script, mod, layout->source, list->units[i].line, "No monster called '%s'", list->units[i].monster);
    }
}

// ---- The words --------------------------------------------------------------------------------------
// Reads a bare word (letters) at *p into out; its length, 0 for none.
static size_t readPlaneBareWord(const char **p, char *out, size_t size) {
    skipSpaces(p);
    size_t len = 0;
    while (isalpha((unsigned char)(*p)[len])) len++;
    snprintf(out, size, "%.*s", (int)(len < size ? len : size - 1), *p);
    *p += len;
    return len;
}

// The open zone of the block, for a zone word: NULL, having refused the line, when there is none.
static PlaneZone *openPlaneZone(const char *word, int mod, int level, const char *args) {
    if (!planeBlockTakes(word, mod, level)) return NULL;
    if (planeBlock.zoneOpen) return &planeBlock.layout.zones[planeBlock.zoneOpen - 1];
    refusePlaneLine(word, mod, "no_zone", "plane=%s text=%.40s", planeBlock.name, args);
    return NULL;
}

// planepiece "<file.coem>" center | random [count] | at <x> <y> | nearportal
static void planepieceLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planepiece", mod, level)) return;
    const char *p = args;
    char spec[PLANE_ICON_SPEC_MAX + 2], how[16];
    int len = readSpellQuoted(&p, spec, sizeof spec);
    readPlaneBareWord(&p, how, sizeof how);
    long long count = 1, x = 0, y = 0;
    int placement = !strcmp(how, "centre") || !strcmp(how, "center") ? PIECE_CENTRE
                  : !strcmp(how, "random")                            ? PIECE_RANDOM
                  : !strcmp(how, "at")                                ? PIECE_AT
                  : !strcmp(how, "nearportal")                        ? PIECE_NEAR_PORTAL
                                                                      : -1;
    const char *reason = len <= 0 ? "no_name" : len > PLANE_ICON_SPEC_MAX ? "bad_file" : placement < 0 ? "bad_placement" : NULL;
    if (!reason && placement == PIECE_RANDOM) {
        skipSpaces(&p);
        if (*p && *p != '\r' && *p != '\n' && *p != '#' && !readSpellNumber(&p, &count)) reason = "bad_count";
    }
    if (!reason && placement == PIECE_AT && (!readSpellNumber(&p, &x) || !readSpellNumber(&p, &y))) reason = "bad_square";
    if (!reason) reason = addPlanePiece(&planeBlock.layout, modFolder(mod), spec, placement, count, x, y, G32(RVA_MOD_LINE));
    if (!reason) return;
    refusePlaneLine("planepiece", mod, reason, "plane=%s text=%.60s", planeBlock.name, args);
    if (pieceFileMissing(reason)) addPlaneProblem(0, spellModName(mod), planeBlock.layout.source, G32(RVA_MOD_LINE), "No map file '%.60s'", spec);
}

// planezone "<name>" around "<file.coem>" <radius>
static void planezoneLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planezone", mod, level)) return;
    planeBlock.zoneOpen = 0;
    const char *p = args;
    char name[PLANE_NAME_SIZE + 2], around[PLANE_ICON_SPEC_MAX + 2], word[16];
    int nameLen = readSpellQuoted(&p, name, sizeof name);
    readPlaneBareWord(&p, word, sizeof word);
    int aroundLen = readSpellQuoted(&p, around, sizeof around);
    long long radius = -1;
    const char *reason = nameLen <= 0                                       ? "no_name"
                       : nameLen >= PLANE_NAME_SIZE                         ? "name_too_long"
                       : strcmp(word, "around") || aroundLen <= 0           ? "no_piece"
                       : aroundLen > PLANE_ICON_SPEC_MAX                    ? "bad_file"
                       : !readSpellNumber(&p, &radius)                      ? "bad_radius"
                                                                            : NULL;
    if (!reason) reason = addPlaneZone(&planeBlock.layout, modFolder(mod), name, around, radius, G32(RVA_MOD_LINE));
    if (!reason) {
        planeBlock.zoneOpen = planeBlock.layout.zoneCount;
        return;
    }
    refusePlaneLine("planezone", mod, reason, "plane=%s text=%.60s", planeBlock.name, args);
    if (pieceFileMissing(reason)) addPlaneProblem(0, spellModName(mod), planeBlock.layout.source, G32(RVA_MOD_LINE), "No map file '%.60s'", around);
}

// planezoneswap <terrain> <terrain>: in the open zone.
static void planezoneswapLine(const char *args, int mod, int level) {
    PlaneZone *z = openPlaneZone("planezoneswap", mod, level, args);
    if (!z) return;
    const char *p = args, *why = NULL;
    int from = readPlaneTerrain(&p, &why), to = from >= 0 ? readPlaneTerrain(&p, &why) : -1;
    const char *reason = to < 0 ? (why ? why : "bad_terrain") : addZoneSwap(z, from, to);
    if (reason) refusePlaneLine("planezoneswap", mod, reason, "plane=%s zone=%s text=%.40s", planeBlock.name, z->name, args);
}

// Reads `"<monster>" <count> <chance>` into list: NULL, or the reason.
static const char *readPlaneUnitsLine(const char *args, PlaneUnitList *list, int line) {
    const char *p = args;
    char name[PLANE_UNIT_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    long long count = 0, chance = 0;
    if (len <= 0) return "no_name";
    if (len >= PLANE_UNIT_NAME_SIZE) return "bad_name";
    if (!readSpellNumber(&p, &count)) return "bad_count";
    if (!readSpellNumber(&p, &chance)) return "bad_chance";
    return addPlaneUnits(list, name, count, chance, line);
}

// planezoneunits "<monster>" <count> <chance>: the open zone's list.
static void planezoneunitsLine(const char *args, int mod, int level) {
    PlaneZone *z = openPlaneZone("planezoneunits", mod, level, args);
    if (!z) return;
    const char *reason = z->populateOff ? "zone_populate_off" : readPlaneUnitsLine(args, &z->units, G32(RVA_MOD_LINE));
    if (reason) refusePlaneLine("planezoneunits", mod, reason, "plane=%s zone=%s text=%.60s", planeBlock.name, z->name, args);
}

// planezonepopulate off: the open zone stays empty.
static void planezonepopulateLine(const char *args, int mod, int level) {
    PlaneZone *z = openPlaneZone("planezonepopulate", mod, level, args);
    if (!z) return;
    const char *p = args;
    char word[16];
    readPlaneBareWord(&p, word, sizeof word);
    const char *reason = strcmp(word, "off") ? "bad_populate" : z->units.count ? "zone_has_units" : NULL;
    if (reason) {
        refusePlaneLine("planezonepopulate", mod, reason, "plane=%s zone=%s text=%.40s", planeBlock.name, z->name, args);
        return;
    }
    z->populateOff = 1;
}

// planeunits "<monster>" <count> <chance>: the plane's list.
static void planeunitsLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planeunits", mod, level)) return;
    const char *reason = readPlaneUnitsLine(args, &planeBlock.layout.units, G32(RVA_MOD_LINE));
    if (reason) refusePlaneLine("planeunits", mod, reason, "plane=%s text=%.60s", planeBlock.name, args);
}

// ---- coe5.addPlane's pieces, zones and units ------------------------------------------------------
// Reads a unit list's table, {{"<monster>", count, chance}, ...}, into list; refusals logged.
static void readLuaUnitList(lua_State *L, int at, PlaneUnitList *list, const PlaneLayout *layout, const char *mod, const char *zone, int line) {
    if (lua_isnil(L, at)) return;
    luaL_checktype(L, at, LUA_TTABLE);
    lua_Integer n = luaL_len(L, at);
    for (lua_Integer i = 1; i <= n; i++) {
        lua_geti(L, at, i);
        int entry = lua_gettop(L);
        luaL_checktype(L, entry, LUA_TTABLE);
        lua_geti(L, entry, 1), lua_geti(L, entry, 2), lua_geti(L, entry, 3);
        const char *monster = luaL_checkstring(L, entry + 1);
        lua_Integer count = luaL_checkinteger(L, entry + 2), chance = luaL_checkinteger(L, entry + 3);
        const char *reason = addPlaneUnits(list, monster, count, chance, line);
        if (reason) refuseLayoutLine(layout, zone ? "zones.units" : "units", mod, reason, "zone=%s monster=%.60s count=%lld chance=%lld", zone ? zone : "-",
                                     monster, (long long)count, (long long)chance);
        lua_settop(L, entry - 1);
    }
}

// Reads coe5.addPlane's pieces, zones and units into layout.
static void readLuaPlaneLayout(lua_State *L, int piecesAt, int zonesAt, int unitsAt, PlaneLayout *layout, const char *mod, const char *folder,
                               const char *script, int line) {
    memset(layout, 0, sizeof *layout);
    layout->script = 1;
    snprintf(layout->source, sizeof layout->source, "%s", script);
    if (!lua_isnil(L, piecesAt)) {
        luaL_checktype(L, piecesAt, LUA_TTABLE);
        lua_Integer n = luaL_len(L, piecesAt);
        for (lua_Integer i = 1; i <= n; i++) {
            lua_geti(L, piecesAt, i);
            int entry = lua_gettop(L);
            luaL_checktype(L, entry, LUA_TTABLE);
            lua_getfield(L, entry, "file"), lua_getfield(L, entry, "place"), lua_getfield(L, entry, "count");
            lua_getfield(L, entry, "x"), lua_getfield(L, entry, "y");
            const char *file = luaL_checkstring(L, entry + 1), *place = luaL_optstring(L, entry + 2, NULL);
            lua_Integer count = luaL_optinteger(L, entry + 3, 1);
            int at = !lua_isnil(L, entry + 4) || !lua_isnil(L, entry + 5);
            lua_Integer x = at ? luaL_checkinteger(L, entry + 4) : 0, y = at ? luaL_checkinteger(L, entry + 5) : 0;
            int placement = at && !place                                               ? PIECE_AT
                          : place && (!strcmp(place, "centre") || !strcmp(place, "center")) ? PIECE_CENTRE
                          : place && !strcmp(place, "random")                           ? PIECE_RANDOM
                          : place && !strcmp(place, "nearportal")                       ? PIECE_NEAR_PORTAL
                                                                                        : -1;
            const char *reason = placement < 0 ? "bad_placement" : addPlanePiece(layout, folder, file, placement, count, x, y, line);
            if (reason) {
                refuseLayoutLine(layout, "pieces", mod, reason, "file=%.60s place=%s", file, place ? place : "-");
                if (pieceFileMissing(reason)) addPlaneProblem(1, mod, script, line, "No map file '%.60s'", file);
            }
            lua_settop(L, entry - 1);
        }
    }
    if (!lua_isnil(L, zonesAt)) {
        luaL_checktype(L, zonesAt, LUA_TTABLE);
        lua_Integer n = luaL_len(L, zonesAt);
        for (lua_Integer i = 1; i <= n; i++) {
            lua_geti(L, zonesAt, i);
            int entry = lua_gettop(L);
            luaL_checktype(L, entry, LUA_TTABLE);
            lua_getfield(L, entry, "name"), lua_getfield(L, entry, "around"), lua_getfield(L, entry, "radius");
            lua_getfield(L, entry, "swap"), lua_getfield(L, entry, "units"), lua_getfield(L, entry, "populate");
            const char *name = luaL_checkstring(L, entry + 1), *around = luaL_checkstring(L, entry + 2);
            lua_Integer radius = luaL_checkinteger(L, entry + 3);
            const char *reason = addPlaneZone(layout, folder, name, around, radius, line);
            if (reason) {
                refuseLayoutLine(layout, "zones", mod, reason, "zone=%.40s around=%.60s", name, around);
                if (pieceFileMissing(reason)) addPlaneProblem(1, mod, script, line, "No map file '%.60s'", around);
                lua_settop(L, entry - 1);
                continue;
            }
            PlaneZone *z = &layout->zones[layout->zoneCount - 1];
            if (!lua_isnil(L, entry + 4)) {
                luaL_checktype(L, entry + 4, LUA_TTABLE);
                lua_pushnil(L);
                while (lua_next(L, entry + 4)) {
                    int good = lua_isinteger(L, -2) && lua_isinteger(L, -1);
                    lua_Integer from = good ? lua_tointeger(L, -2) : -1, to = good ? lua_tointeger(L, -1) : -1;
                    lua_pop(L, 1);
                    const char *why = !good || from < 0 || from > SHRT_MAX || to < 0 || to > SHRT_MAX ? "bad_swap" : addZoneSwap(z, (int)from, (int)to);
                    if (why) refuseLayoutLine(layout, "zones.swap", mod, why, "zone=%s from=%lld to=%lld", z->name, (long long)from, (long long)to);
                }
            }
            int off = lua_type(L, entry + 6) == LUA_TBOOLEAN && !lua_toboolean(L, entry + 6);
            if (!lua_isnil(L, entry + 6) && !off) luaL_typeerror(L, entry + 6, "false");
            readLuaUnitList(L, entry + 5, &z->units, layout, mod, z->name, line);
            if (off && z->units.count) refuseLayoutLine(layout, "zones.populate", mod, "zone_has_units", "zone=%s", z->name);
            else z->populateOff = off;
            lua_settop(L, entry - 1);
        }
    }
    readLuaUnitList(L, unitsAt, &layout->units, layout, mod, NULL, line);
}

// ---- Where the pieces go --------------------------------------------------------------------------
// The game's square distance from (x, y) to the rectangle: 0 inside it.
static int squareDistanceToRect(int x, int y, const PlacedPiece *r) {
    int dx = x < r->x ? r->x - x : x >= r->x + r->w ? x - (r->x + r->w - 1) : 0;
    int dy = y < r->y ? r->y - y : y >= r->y + r->h ? y - (r->y + r->h - 1) : 0;
    return dx > dy ? dx : dy;
}

// Whether the w x h rectangle at (x, y) meets a placed piece, or the way in's square.
static int pieceOverlaps(const PlaneLayout *layout, int x, int y, int w, int h) {
    for (int i = 0; i < layout->placedCount; i++) {
        const PlacedPiece *r = &layout->placed[i];
        if (x < r->x + r->w && r->x < x + w && y < r->y + r->h && r->y < y + h) return 1;
    }
    return layout->wayInX >= 0 && layout->wayInX >= x && layout->wayInX < x + w && layout->wayInY >= y && layout->wayInY < y + h;
}

// Whether square (x, y) is on a placed piece.
static int onPlacedPiece(const PlaneLayout *layout, int x, int y) {
    for (int i = 0; i < layout->placedCount; i++)
        if (!squareDistanceToRect(x, y, &layout->placed[i])) return 1;
    return 0;
}

// The zone square (x, y) is in: the first zone declared whose piece's copy is within its radius; -1 for none.
static int planeZoneAt(const PlaneLayout *layout, int x, int y) {
    for (int z = 0; z < layout->zoneCount; z++)
        for (int i = 0; i < layout->placedCount; i++)
            if (layout->placed[i].piece == layout->zones[z].piece && squareDistanceToRect(x, y, &layout->placed[i]) <= layout->zones[z].radius) return z;
    return -1;
}

// Whether any square of the rectangle holds a portal.
static int rectHasPortal(int x0, int y0, int w, int h) {
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++)
            if (*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL)) return 1;
    return 0;
}

// A square of plane p with a portal whose number a square off the plane holds: 1 and the square, else 0.
static int findWayInSquare(const CustomPlane *p, int *wx, int *wy) {
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    int depth = G32(RVA_MAP_DEPTH) < MAP_COLUMNS_MAX ? G32(RVA_MAP_DEPTH) : MAP_COLUMNS_MAX;
    int height = G32(RVA_MAP_H) < MAP_COLUMN_SQUARES ? G32(RVA_MAP_H) : MAP_COLUMN_SQUARES;
    for (int x = x0; x < x0 + p->w; x++)
        for (int y = y0; y < y0 + p->h; y++) {
            int portal = abs(*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL));
            if (!portal) continue;
            for (int ox = 0; ox < depth; ox++)
                for (int oy = 0; oy < height; oy++) {
                    if (ox >= x0 && oy >= y0 && ox < x0 + p->w && oy < y0 + p->h) continue;
                    if (abs(*(short *)(squareRecordAt(ox, oy) + SQUARE_PORTAL)) == portal) {
                        *wx = x, *wy = y;
                        return 1;
                    }
                }
        }
    return 0;
}

// The way in's square on plane p, for a nearportal piece.
static int planeWayInSquare(CustomPlane *p, const PieceTools *t) {
    PlaneLayout *layout = &p->layout;
    if (layout->wayInX >= 0) return 1;
    int x, y;
    if (findWayInSquare(p, &x, &y)) {
        layout->wayInX = x, layout->wayInY = y;
        return 1;
    }
    for (int i = 0; i < PLANE_PORTAL_TRIES; i++) {
        if (!t->squareOfTerrain(p->index, TERRAIN_CODE_LAND, &x, &y)) break;
        if (entranceSquareFree(layout, x, y)) {
            layout->wayInX = x, layout->wayInY = y;
            return 1;
        }
    }
    return 0;
}

// Where copy of piece `piece` goes on plane p (world squares): 1 and the corner, else 0 with *why set.
static int placePiece(CustomPlane *p, const PlanePiece *piece, const PieceTools *t, int *px, int *py, const char **why) {
    PlaneLayout *layout = &p->layout;
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y), w = piece->w, h = piece->h;
    *why = "no_room";
    if (piece->placement == PIECE_CENTRE || piece->placement == PIECE_AT) {
        int x = x0 + (piece->placement == PIECE_AT ? piece->x : (p->w - w) / 2), y = y0 + (piece->placement == PIECE_AT ? piece->y : (p->h - h) / 2);
        if (x + w > x0 + p->w || y + h > y0 + p->h) return 0;
        if (pieceOverlaps(layout, x, y, w, h)) {
            *why = "overlap";
            return 0;
        }
        *px = x, *py = y;
        return 1;
    }
    if (piece->placement == PIECE_NEAR_PORTAL) {
        if (!planeWayInSquare(p, t)) {
            *why = "no_way_in";
            return 0;
        }
        int wx = layout->wayInX, wy = layout->wayInY, fits = 0, pick = -1;
        for (int pass = 0; pass < 2; pass++) {
            int k = 0;
            for (int x = wx - w; x <= wx + 1; x++)
                for (int y = wy - h; y <= wy + 1; y++) {
                    PlacedPiece r = {0, x, y, w, h};
                    if (x < x0 || y < y0 || x + w > x0 + p->w || y + h > y0 + p->h || squareDistanceToRect(wx, wy, &r) != 1 || pieceOverlaps(layout, x, y, w, h))
                        continue;
                    if (pass && k == pick) {
                        *px = x, *py = y;
                        return 1;
                    }
                    k++;
                }
            if (!pass) {
                if (!(fits = k)) return 0;
                pick = t->random(fits);
            }
        }
        return 0;
    }
    for (int i = 0; i < PLANE_PIECE_TRIES; i++) {
        int x = x0 + t->random(p->w - w + 1), y = y0 + t->random(p->h - h + 1);
        if (!pieceOverlaps(layout, x, y, w, h) && !rectHasPortal(x, y, w, h)) {
            *px = x, *py = y;
            return 1;
        }
    }
    return 0;
}

// ---- Stamping ----------------------------------------------------------------------------------------
// Clears portal `n` from every square of the map.
static void clearPortalEverywhere(int n) {
    int depth = G32(RVA_MAP_DEPTH) < MAP_COLUMNS_MAX ? G32(RVA_MAP_DEPTH) : MAP_COLUMNS_MAX;
    int height = G32(RVA_MAP_H) < MAP_COLUMN_SQUARES ? G32(RVA_MAP_H) : MAP_COLUMN_SQUARES;
    for (int x = 0; x < depth; x++)
        for (int y = 0; y < height; y++)
            if (abs(*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL)) == n) *(short *)(squareRecordAt(x, y) + SQUARE_PORTAL) = 0;
}

// Stamps map file `path` at square (x, y) of plane p through the game's map reader.
static void stampMapFileAt(const CustomPlane *p, const char *path, int x, int y, int w, int h, const PieceTools *t) {
    int portals = 0;
    for (int sx = x; sx < x + w; sx++)
        for (int sy = y; sy < y + h; sy++) {
            unsigned char *square = squareRecordAt(sx, sy);
            short portal = *(short *)(square + SQUARE_PORTAL);
            if (portal) clearPortalEverywhere(abs(portal)), portals++;
            memset(square + SQUARE_NAME, 0, SQUARE_NAME_SIZE);
            G32(RVA_SQUARE_FLAGS + ((size_t)sx * MAP_COLUMN_SQUARES + (size_t)sy) * SQUARE_SIZE) = 0;
        }
    int saved[3] = {planeField(p->index, PLANE_X), planeField(p->index, PLANE_W), planeField(p->index, PLANE_H)};
    setPlaneField(p->index, PLANE_X, x), setPlaneField(p->index, PLANE_W, w), setPlaneField(p->index, PLANE_H, y + h);
    readMapFileThroughGame(p, path, p->index, y, h);
    setPlaneField(p->index, PLANE_X, saved[0]), setPlaneField(p->index, PLANE_W, saved[1]), setPlaneField(p->index, PLANE_H, saved[2]);
    if (!planeFileLoad.groundRows) t->setGroundRect(x, y, w, h, -1);
    setColdFlagsFromTemperatures(x, y, w, h);
    writeLog("plane_piece_stamped", "mod=%s name=%s x=%d y=%d w=%d h=%d plane_x=%d plane_y=%d lines=%d left_out=%d portals_cleared=%d file=%s", p->mod, p->name, x, y, w,
             h, x - saved[0], y - planeField(p->index, PLANE_Y), planeFileLoad.lines, planeFileLoad.leftOut, portals, path);
}

// Adds a stamped rectangle to the plane's list; 0 when it is full.
static int notePlacedPiece(PlaneLayout *layout, int piece, int x, int y, int w, int h) {
    if (layout->placedCount >= PLANE_PIECES_PLACED_MAX) return 0;
    layout->placed[layout->placedCount++] = (PlacedPiece){piece, x, y, w, h};
    return 1;
}

// A world build's start for plane p: no piece stamped, no way in kept.
static void clearPlacedPieces(CustomPlane *p) {
    p->layout.placedCount = 0;
    p->layout.wayInX = p->layout.wayInY = -1;
}

// The order the copies of the plane's pieces go in.
static int pieceOrderRank(const PlanePiece *piece) {
    return piece->placement == PIECE_CENTRE || piece->placement == PIECE_AT ? 0 : piece->placement == PIECE_NEAR_PORTAL ? 1 : 2;
}

// Stamps plane p's pieces, after its recipe and generate function.
static void stampPlanePieces(CustomPlane *p) {
    PlaneLayout *layout = &p->layout;
    if (!layout->pieceCount) return;
    PieceTools t = pieceTools();
    int order[PLANE_PIECE_LINES_MAX];
    for (int i = 0; i < layout->pieceCount; i++) order[i] = i;
    for (int i = 1; i < layout->pieceCount; i++)
        for (int j = i; j > 0; j--) {
            const PlanePiece *a = &layout->pieces[order[j - 1]], *b = &layout->pieces[order[j]];
            int ra = pieceOrderRank(a), rb = pieceOrderRank(b);
            if (ra < rb || (ra == rb && a->w * a->h >= b->w * b->h)) break;
            int k = order[j];
            order[j] = order[j - 1], order[j - 1] = k;
        }
    int stamped = 0, skipped = 0;
    for (int i = 0; i < layout->pieceCount; i++) {
        const PlanePiece *piece = &layout->pieces[order[i]];
        for (int c = 0; c < piece->count; c++) {
            int x, y;
            const char *why;
            if (!placePiece(p, piece, &t, &x, &y, &why) || !notePlacedPiece(layout, order[i], x, y, piece->w, piece->h)) {
                if (layout->placedCount >= PLANE_PIECES_PLACED_MAX) why = "too_many_pieces";
                writeLog("plane_piece_skipped", "mod=%s name=%s line=%d copy=%d reason=%s w=%d h=%d file=%s", p->mod, p->name, piece->line, c + 1, why, piece->w,
                         piece->h, piece->file);
                skipped++;
                continue;
            }
            stampMapFileAt(p, piece->file, x, y, piece->w, piece->h, &t);
            stamped++;
        }
    }
    writeLog("plane_pieces", "mod=%s name=%s stamped=%d skipped=%d way_in_x=%d way_in_y=%d", p->mod, p->name, stamped, skipped, layout->wayInX, layout->wayInY);
}

// coe5.gen.stampPiece(file, x, y): stamps a map file onto the plane; true, or nil when refused.
static int lGenStampPiece(lua_State *L) {
    const char *call = "coe5.gen.stampPiece", *spec = luaL_checkstring(L, 1);
    int x, y;
    if (!genSquare(L, call, 2, &x, &y)) return 0;
    LuaScript *s = luaScriptOf(L);
    CustomPlane *p = NULL;
    for (int i = 0; i < customPlaneCount && !p; i++)
        if (customPlanes[i].index == planeBuild.index && !customPlanes[i].skipped) p = &customPlanes[i];
    char path[MAX_PATH];
    int w = 0, h = 0;
    const char *reason = !p ? "no_plane" : !s || !s->folder[0] ? "no_mod_folder" : findPieceFile(s->folder, spec, path, sizeof path);
    if (!reason && !planeFileMapsize(path, &w, &h)) reason = "no_mapsize";
    if (!reason && !installPlaneFileHook()) reason = "hook_not_installed";
    if (!reason && (x + w > planeBuild.area.x + planeBuild.area.w || y + h > planeBuild.area.y + planeBuild.area.h)) reason = "outside_plane";
    if (!reason && !notePlacedPiece(&p->layout, -1, x, y, w, h)) reason = "too_many_pieces";
    if (reason) {
        refuseLuaCall(genMod(L), call, reason, "file=%.60s x=%d y=%d w=%d h=%d", spec, x, y, w, h);
        return 0;
    }
    PieceTools t = pieceTools();
    stampMapFileAt(p, path, x, y, w, h, &t);
    lua_pushboolean(L, 1);
    return 1;
}

// ---- After the game's special squares ---------------------------------------------------------------
// Keeps the pieces' squares while the recipe's special step runs.
static unsigned char *guardPlacedPieces(const PlaneLayout *layout) {
    size_t squares = 0;
    for (int i = 0; i < layout->placedCount; i++) squares += (size_t)layout->placed[i].w * (size_t)layout->placed[i].h;
    if (!squares) return NULL;
    unsigned char *saved = malloc(squares * SQUARE_SIZE), *at = saved;
    if (!saved) {
        writeLog("plane_pieces_unguarded", "reason=out_of_memory squares=%zu", squares);
        return NULL;
    }
    for (int i = 0; i < layout->placedCount; i++) {
        const PlacedPiece *r = &layout->placed[i];
        for (int x = r->x; x < r->x + r->w; x++, at += (size_t)r->h * SQUARE_SIZE) memcpy(at, squareRecordAt(x, r->y), (size_t)r->h * SQUARE_SIZE);
    }
    return saved;
}
static void keepPlacedPieces(const CustomPlane *p, unsigned char *saved) {
    if (!saved) return;
    const PlaneLayout *layout = &p->layout;
    const unsigned char *at = saved;
    int kept = 0;
    for (int i = 0; i < layout->placedCount; i++) {
        const PlacedPiece *r = &layout->placed[i];
        for (int x = r->x; x < r->x + r->w; x++)
            for (int y = r->y; y < r->y + r->h; y++, at += SQUARE_SIZE)
                if (memcmp(squareRecordAt(x, y), at, SQUARE_SIZE)) memcpy(squareRecordAt(x, y), at, SQUARE_SIZE), kept++;
    }
    free(saved);
    if (kept) writeLog("plane_pieces_kept", "mod=%s name=%s squares=%d", p->mod, p->name, kept);
}

// Applies each zone's swap through the game's own terrain change, after the special squares.
static void applyZoneSwaps(const CustomPlane *p) {
    const PlaneLayout *layout = &p->layout;
    if (!layout->zoneCount || !layout->placedCount) return;
    PieceTools t = pieceTools();
    int last = G32(RVA_LAST_TERRAIN), x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    int changed[PLANE_ZONES_MAX] = {0}, squares[PLANE_ZONES_MAX] = {0};
    for (int x = x0; x < x0 + p->w; x++)
        for (int y = y0; y < y0 + p->h; y++) {
            int z = planeZoneAt(layout, x, y);
            if (z < 0) continue;
            squares[z]++;
            const PlaneZone *zone = &layout->zones[z];
            short terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
            for (int i = 0; i < zone->swapCount; i++) {
                if (terrain != zone->swapFrom[i] || zone->swapTo[i] > last) continue;
                t.changeTerrain(x, y, zone->swapTo[i]);
                changed[z]++;
                break;
            }
        }
    for (int z = 0; z < layout->zoneCount; z++) {
        for (int i = 0; i < layout->zones[z].swapCount; i++)
            if (layout->zones[z].swapTo[i] > last)
                writeLog("plane_swap_refused", "mod=%s name=%s zone=%s from=%d to=%d reason=no_such_terrain last_terrain=%d", p->mod, p->name, layout->zones[z].name,
                         layout->zones[z].swapFrom[i], layout->zones[z].swapTo[i], last);
        writeLog("plane_zone", "mod=%s name=%s zone=%s squares=%d swapped=%d", p->mod, p->name, layout->zones[z].name, squares[z], changed[z]);
    }
}

// ---- Population ------------------------------------------------------------------------------------
// Whether square (x, y) of plane p belongs to `area`.
static int zoneHasOwnStock(const PlaneZone *z) { return z->units.count || z->populateOff; }
static int inStockArea(const PlaneLayout *layout, int x, int y, int area) {
    int z = planeZoneAt(layout, x, y);
    return area >= 0 ? z == area : z < 0 || !zoneHasOwnStock(&layout->zones[z]);
}

// Stocks `area` of plane p from `list`.
static void stockPlaneArea(const CustomPlane *p, int area, const PlaneUnitList *list, const PieceTools *t) {
    const PlaneLayout *layout = &p->layout;
    int valid[PLANE_UNIT_LINES_MAX], wilder = G32(RVA_WILDER) > 0 ? G32(RVA_WILDER) : 1;
    unsigned long long moves[PLANE_UNIT_LINES_MAX];
    for (int i = 0; i < list->count; i++) {
        int monster = monsterByName(list->units[i].monster);
        valid[i] = monster >= 0;
        moves[i] = 0;
        if (valid[i]) memcpy(&moves[i], monsterRecord(monster) + MONSTER_MOVE_FLAGS, sizeof moves[i]);
        if (!valid[i])
            writeLog("plane_units_skipped", "mod=%s name=%s area=%s monster=%s line=%d reason=no_such_monster", p->mod, p->name,
                     area >= 0 ? layout->zones[area].name : "-", list->units[i].monster, list->units[i].line);
    }
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y), squares = 0, groups = 0, units = 0;
    for (int x = x0; x < x0 + p->w; x++)
        for (int y = y0; y < y0 + p->h; y++) {
            if (!inStockArea(layout, x, y, area)) continue;
            size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            int owner = *(signed char *)(squareRecordAt(x, y) + SQUARE_OWNER), terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
            short apcost = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? (short)G16(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_APCOST) : 0;
            if ((owner >= 0 && owner != PLANE_UNITS_PLAYER) || G16(RVA_SQUARE_ARMY + square * 2) || apcost >= TERRAIN_APCOST_WALL) continue;
            squares++;
            long long terrainFlags = 0;
            if (terrain >= 0 && terrain < TERRAIN_TABLE_MAX) memcpy(&terrainFlags, GAME_PTR(RVA_TERRAIN_TABLE) + (size_t)terrain * TERRAIN_SIZE + TERRAIN_FLAGS, sizeof terrainFlags);
            int water = (terrainFlags & TERRAIN_FLAG_WATER) || (G32(RVA_SQUARE_FLAGS + square * SQUARE_SIZE) & SQUARE_FLAG_FLOODED);
            int here = 0;
            for (int i = 0; i < list->count; i++) {
                if (!valid[i] || (water ? !(moves[i] & MONSTER_MOVE_WATER) : (moves[i] & MONSTER_MOVE_NOLAND) != 0)) continue;
                if (t->random(100) >= list->units[i].chance) continue;
                t->populate(x, y, list->units[i].monster, list->units[i].count * wilder, PLANE_UNITS_PLAYER, -1);
                groups++, here++, units += list->units[i].count * wilder;
            }
            if (here && area >= 0) t->holdit(x, y, 1);
        }
    writeLog("plane_units_stocked", "mod=%s name=%s area=%s squares=%d groups=%d units=%d wilder=%d", p->mod, p->name, area >= 0 ? layout->zones[area].name : "-",
             squares, groups, units, wilder);
}

// Marks the zones with their own stock so the plane's population skips them; returns how many.
static int hideZoneSquares(const CustomPlane *p, int **hidden) {
    const PlaneLayout *layout = &p->layout;
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y), n = 0;
    *hidden = NULL;
    for (int pass = 0; pass < 2; pass++) {
        int k = 0;
        for (int x = x0; x < x0 + p->w; x++)
            for (int y = y0; y < y0 + p->h; y++) {
                int z = planeZoneAt(layout, x, y);
                size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
                if (z < 0 || !zoneHasOwnStock(&layout->zones[z]) || G16(RVA_SQUARE_ARMY + square * 2)) continue;
                if (pass) {
                    (*hidden)[k] = (int)square;
                    G16(RVA_SQUARE_ARMY + square * 2) = 1;
                }
                k++;
            }
        if (!pass && (!(n = k) || !(*hidden = malloc(sizeof **hidden * (size_t)n)))) {
            if (n) writeLog("plane_zones_not_hidden", "mod=%s name=%s reason=out_of_memory squares=%d", p->mod, p->name, n);
            return 0;
        }
    }
    return n;
}

// Takes the army mark off: the game recounts each square's units (RVA_REBUILD_SQUARE_ARMY).
static void showZoneSquares(const CustomPlane *p, int *hidden, int n, const PieceTools *t) {
    int armies = 0;
    for (int i = 0; i < n; i++) {
        int x = hidden[i] / MAP_COLUMN_SQUARES, y = hidden[i] % MAP_COLUMN_SQUARES;
        t->rebuildArmy(x, y);
        armies += G16(RVA_SQUARE_ARMY + (size_t)hidden[i] * 2) != 0;
    }
    free(hidden);
    writeLog("plane_zones_hidden", "mod=%s name=%s squares=%d armies_after=%d", p->mod, p->name, n, armies);
}

// Plane p's population, then each zone's list.
static void populatePlaneWithZones(const CustomPlane *p) {
    const PlaneLayout *layout = &p->layout;
    PieceTools t = pieceTools();
    if (layout->units.count) {
        if (p->populate >= 0)
            writeLog("plane_population_replaced", "mod=%s name=%s populate=%s reason=plane_units", p->mod, p->name, VANILLA_PLANE_SHORT_NAMES[p->populate]);
    } else if (p->populate >= 0) {
        int *hidden;
        int n = hideZoneSquares(p, &hidden);
        t.populatePlane(p);
        if (n) showZoneSquares(p, hidden, n, &t);
    }
    for (int z = 0; z < layout->zoneCount; z++)
        if (layout->zones[z].units.count) stockPlaneArea(p, z, &layout->zones[z].units, &t);
    if (layout->units.count) stockPlaneArea(p, -1, &layout->units, &t);
}
