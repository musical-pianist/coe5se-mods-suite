// Mods' planes built the game's way: coe5.gen, the recipes, population and terrain swap.

#define PLANE_RECIPES_FILE   "plane_recipes.lua"
#define MARK_LAYER_SQUARES   (MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES)
#define TERRAIN_GATEWAY      204
#define PORTAL_NUMBER_FIRST  1000
#define PORTAL_NUMBER_SPAN   10000
#define PORTAL_NUMBER_TRIES  1000
#define TERRAIN_CODE_LAND    (-42)
#define TERRAIN_CODE_FLAT    (-71)
#define PLANE_PORTAL_TRIES   100
#define TERRAIN_APCOST_WALL  99

// The vanilla planes by the short names coe5.addPlane takes (or the record's name), and which have a recipe.
static const char *const VANILLA_PLANE_SHORT_NAMES[PLANE_ELYSIUM2] = {
    "Elysium", "Celestial", "Sky", "Agartha", "Inferno", "Hades", "Aztlan", "Primal", "Elemental", "Void",
};
static const unsigned char PLANE_HAS_RECIPE[PLANE_ELYSIUM2] = {0, 1, 1, 1, 1, 1, 0, 0, 1, 1};

// The index of vanilla plane `name` (0 Elysium to 9 the Void), or -1.
static int vanillaPlaneNamed(const char *name) {
    for (int k = 0; k < PLANE_ELYSIUM2; k++)
        if (!strcmp(name, VANILLA_PLANE_SHORT_NAMES[k]) || !strcmp(name, VANILLA_PLANE_NAMES[k])) return k;
    return -1;
}

// ---- The plane being built ----------------------------------------------------------------------
static struct {
    int active, index;
    SquareRect area;
    SquareRect previousArea;
    short *savedMarks;
} planeBuild;

static short *markLayer(void) { return (short *)GAME_PTR(RVA_GEN_SCRATCH_MARKS); }
static unsigned char *squareRecordAt(int x, int y) {
    return GAME_PTR(RVA_SQUARE_RECORDS) + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
}
static int insidePlaneBuilt(int x, int y) {
    SquareRect a = planeBuild.area;
    return x >= a.x && y >= a.y && x < a.x + a.w && y < a.y + a.h;
}

// Starts building plane `index` over `area`: the mark layer saved and emptied, square writes opened.
static int beginPlaneBuild(int index, SquareRect area, const char *mod, const char *name) {
    planeBuild.savedMarks = malloc(sizeof(short) * MARK_LAYER_SQUARES);
    if (!planeBuild.savedMarks) {
        writeLog("plane_build_skipped", "mod=%s name=%s reason=out_of_memory", mod, name);
        return 0;
    }
    memcpy(planeBuild.savedMarks, markLayer(), sizeof(short) * MARK_LAYER_SQUARES);
    memset(markLayer(), 0, sizeof(short) * MARK_LAYER_SQUARES);
    planeBuild.active = 1, planeBuild.index = index, planeBuild.area = area;
    planeBuild.previousArea = startWorldBuilding(area);
    return 1;
}

// Ends the build beginPlaneBuild started: the marks back as they were, square writes closed.
static void endPlaneBuild(void) {
    if (!planeBuild.active) return;
    memcpy(markLayer(), planeBuild.savedMarks, sizeof(short) * MARK_LAYER_SQUARES);
    free(planeBuild.savedMarks);
    planeBuild.savedMarks = NULL;
    planeBuild.active = 0;
    stopWorldBuilding(planeBuild.previousArea);
}

// Clears every mark outside the plane being built.
static void clearMarksOutsidePlane(void) {
    short *marks = markLayer();
    SquareRect a = planeBuild.area;
    for (int x = 0; x < MAP_COLUMNS_MAX; x++) {
        short *column = marks + (size_t)x * MAP_COLUMN_SQUARES;
        if (x < a.x || x >= a.x + a.w) {
            memset(column, 0, sizeof(short) * MAP_COLUMN_SQUARES);
            continue;
        }
        memset(column, 0, sizeof(short) * (size_t)a.y);
        memset(column + a.y + a.h, 0, sizeof(short) * (size_t)(MAP_COLUMN_SQUARES - a.y - a.h));
    }
}

// The squares around a tool's work that lie outside the plane, kept as they were.
typedef struct { int x0, y0, x1, y1; unsigned char *saved; } SquareGuard;
static void guardSquares(SquareGuard *g, int x0, int y0, int x1, int y1) {
    g->x0 = x0 < 0 ? 0 : x0, g->y0 = y0 < 0 ? 0 : y0;
    g->x1 = x1 > MAP_COLUMNS_MAX ? MAP_COLUMNS_MAX : x1, g->y1 = y1 > MAP_COLUMN_SQUARES ? MAP_COLUMN_SQUARES : y1;
    g->saved = NULL;
    if (g->x1 <= g->x0 || g->y1 <= g->y0) return;
    size_t column = (size_t)(g->y1 - g->y0) * SQUARE_SIZE;
    if (!(g->saved = malloc(column * (size_t)(g->x1 - g->x0)))) {
        writeLog("plane_guard_skipped", "reason=out_of_memory x0=%d y0=%d x1=%d y1=%d", g->x0, g->y0, g->x1, g->y1);
        return;
    }
    for (int x = g->x0; x < g->x1; x++) memcpy(g->saved + column * (size_t)(x - g->x0), squareRecordAt(x, g->y0), column);
}
static void unguardSquares(SquareGuard *g) {
    if (!g->saved) return;
    size_t column = (size_t)(g->y1 - g->y0) * SQUARE_SIZE;
    for (int x = g->x0; x < g->x1; x++)
        for (int y = g->y0; y < g->y1; y++)
            if (!insidePlaneBuilt(x, y))
                memcpy(squareRecordAt(x, y), g->saved + column * (size_t)(x - g->x0) + (size_t)(y - g->y0) * SQUARE_SIZE, SQUARE_SIZE);
    free(g->saved);
    g->saved = NULL;
}

// ---- coe5.gen: the game's tools, one step each --------------------------------------------------
static const char *genMod(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    return s ? s->mod : "?";
}
// 1 while a plane is being built; else logs not_world_building for `call`.
static int genBuilding(lua_State *L, const char *call) {
    if (planeBuild.active) return 1;
    refuseLuaCall(genMod(L), call, "not_world_building", "plane=-");
    return 0;
}
// Reads integer argument `arg` (`what` in the log) into *out; 0 and out_of_range outside min..max.
static int genInt(lua_State *L, const char *call, int arg, const char *what, lua_Integer min, lua_Integer max, int *out) {
    lua_Integer v = luaL_checkinteger(L, arg);
    if (v < min || v > max) {
        refuseLuaCall(genMod(L), call, "out_of_range", "%s=%lld min=%lld max=%lld", what, (long long)v, (long long)min, (long long)max);
        return 0;
    }
    *out = (int)v;
    return 1;
}
// Reads the square at arguments `arg` and `arg` + 1: 1 when a plane is being built and it is on it.
static int genSquare(lua_State *L, const char *call, int arg, int *x, int *y) {
    lua_Integer lx = luaL_checkinteger(L, arg), ly = luaL_checkinteger(L, arg + 1);
    if (!genBuilding(L, call)) return 0;
    const char *reason = squareWriteRefusal(1, planeBuild.area, lx, ly);
    if (reason) {
        refuseLuaCall(genMod(L), call, reason, "x=%lld y=%lld", (long long)lx, (long long)ly);
        return 0;
    }
    *x = (int)lx, *y = (int)ly;
    return 1;
}
// Reads the rectangle at arguments `arg`..`arg` + 3, cut to the plane being built; 0 when nothing is left.
static int genRect(lua_State *L, const char *call, int arg, SquareRect *r) {
    lua_Integer x = luaL_checkinteger(L, arg), y = luaL_checkinteger(L, arg + 1);
    lua_Integer w = luaL_checkinteger(L, arg + 2), h = luaL_checkinteger(L, arg + 3);
    if (!genBuilding(L, call)) return 0;
    SquareRect a = planeBuild.area;
    lua_Integer x0 = x > a.x ? x : a.x, y0 = y > a.y ? y : a.y;
    lua_Integer x1 = x + w < a.x + a.w ? x + w : a.x + a.w, y1 = y + h < a.y + a.h ? y + h : a.y + a.h;
    if (w < 1 || h < 1 || x1 <= x0 || y1 <= y0) {
        refuseLuaCall(genMod(L), call, "outside_plane", "x=%lld y=%lld w=%lld h=%lld", (long long)x, (long long)y, (long long)w, (long long)h);
        return 0;
    }
    *r = (SquareRect){(int)x0, (int)y0, (int)(x1 - x0), (int)(y1 - y0)};
    return 1;
}
static int genTerrain(lua_State *L, const char *call, int arg, int *out) {
    return genInt(L, call, arg, "terrain", 0, G32(RVA_LAST_TERRAIN), out);
}
static int genMark(lua_State *L, const char *call, int arg, int *out) { return genInt(L, call, arg, "mark", 1, SHRT_MAX, out); }
// A terrain to test against: a terrain number or one of the game's negative terrain codes.
static int genCode(lua_State *L, const char *call, int arg, int *out) { return genInt(L, call, arg, "code", SHRT_MIN, SHRT_MAX, out); }
static int genPlaneSquares(void) { return planeBuild.area.w * planeBuild.area.h; }

// coe5.gen.random(n): the game's own random draw, 0 to n - 1 (RVA_RANDOM), as the builders draw.
static int lGenRandom(lua_State *L) {
    int n;
    if (!genInt(L, "coe5.gen.random", 1, "n", 0, INT_MAX, &n) || !genBuilding(L, "coe5.gen.random")) return 0;
    lua_pushinteger(L, GAME_FN(int (*)(int), RVA_RANDOM)(n));
    return 1;
}

// coe5.gen.markAt(x, y) and coe5.gen.setMark(x, y, mark): a square's mark (0 for none).
static int lGenMarkAt(lua_State *L) {
    int x, y;
    if (!genSquare(L, "coe5.gen.markAt", 1, &x, &y)) return 0;
    lua_pushinteger(L, markLayer()[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y]);
    return 1;
}
static int lGenSetMark(lua_State *L) {
    int x, y, mark;
    if (!genSquare(L, "coe5.gen.setMark", 1, &x, &y) || !genInt(L, "coe5.gen.setMark", 3, "mark", 0, SHRT_MAX, &mark)) return 0;
    markLayer()[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y] = (short)mark;
    return 0;
}

// coe5.gen.setTerrain(x, y, terrain): stores a square's terrain as the builders do.
static int lGenSetTerrain(lua_State *L) {
    int x, y, terrain;
    if (!genSquare(L, "coe5.gen.setTerrain", 1, &x, &y) || !genTerrain(L, "coe5.gen.setTerrain", 3, &terrain)) return 0;
    *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE) = (short)terrain;
    return 0;
}

// coe5.gen.terrainFlags(terrain): the terrain's flag bits (the terrain table's u64 at +0x50).
static int lGenTerrainFlags(lua_State *L) {
    int terrain;
    if (!genTerrain(L, "coe5.gen.terrainFlags", 1, &terrain)) return 0;
    long long flags;
    memcpy(&flags, GAME_PTR(RVA_TERRAIN_TABLE) + (size_t)terrain * TERRAIN_SIZE + TERRAIN_FLAGS, sizeof flags);
    lua_pushinteger(L, (lua_Integer)flags);
    return 1;
}

// coe5.gen.terrainNear(x, y, code, radius): counts matching squares within radius.
static int lGenTerrainNear(lua_State *L) {
    int x, y, code, radius;
    if (!genSquare(L, "coe5.gen.terrainNear", 1, &x, &y) || !genCode(L, "coe5.gen.terrainNear", 3, &code) ||
        !genInt(L, "coe5.gen.terrainNear", 4, "radius", 0, MAP_COLUMN_SQUARES, &radius))
        return 0;
    lua_pushinteger(L, GAME_FN(int (*)(int, int, int, int), RVA_TERRAIN_NEAR_COUNT)(x, y, code, radius));
    return 1;
}

// coe5.gen.markRect(x, y, w, h, mark): marks a rectangle, cut to the plane (RVA_GEN_MARK_RECT).
static int lGenMarkRect(lua_State *L) {
    SquareRect r;
    int mark;
    if (!genRect(L, "coe5.gen.markRect", 1, &r) || !genMark(L, "coe5.gen.markRect", 5, &mark)) return 0;
    GAME_FN(void (*)(int, int, int, int, int), RVA_GEN_MARK_RECT)(r.x, r.y, r.w, r.h, mark);
    return 0;
}

// coe5.gen.markWalk(x, y, terrain or -1, count, mark): marks squares along a random walk.
static int lGenMarkWalk(lua_State *L) {
    int x, y, terrain, count, mark;
    if (!genSquare(L, "coe5.gen.markWalk", 1, &x, &y) || !genInt(L, "coe5.gen.markWalk", 3, "terrain", -1, G32(RVA_LAST_TERRAIN), &terrain) ||
        !genInt(L, "coe5.gen.markWalk", 4, "count", 1, genPlaneSquares(), &count) || !genMark(L, "coe5.gen.markWalk", 5, &mark))
        return 0;
    GAME_FN(void (*)(int, int, int, int, int), RVA_GEN_MARK_WALK)(x, y, terrain, count, mark);
    clearMarksOutsidePlane();
    return 0;
}

// coe5.gen.markBlob(x, y, size, mark): the builders' round blob (RVA_MARK_BLOB) from a square of the plane.
static int lGenMarkBlob(lua_State *L) {
    int x, y, size, mark;
    if (!genSquare(L, "coe5.gen.markBlob", 1, &x, &y) || !genInt(L, "coe5.gen.markBlob", 3, "size", 1, genPlaneSquares(), &size) ||
        !genMark(L, "coe5.gen.markBlob", 4, &mark))
        return 0;
    GAME_FN(void (*)(int, int, int, int), RVA_MARK_BLOB)(x, y, size, mark);
    clearMarksOutsidePlane();
    return 0;
}

// coe5.gen.markRiver(x1, y1, x2, y2, mark, mode): marks the cheapest path between two squares.
static int lGenMarkRiver(lua_State *L) {
    int x1, y1, x2, y2, mark, mode;
    if (!genSquare(L, "coe5.gen.markRiver", 1, &x1, &y1) || !genSquare(L, "coe5.gen.markRiver", 3, &x2, &y2) ||
        !genMark(L, "coe5.gen.markRiver", 5, &mark) || !genInt(L, "coe5.gen.markRiver", 6, "mode", 0, 2, &mode))
        return 0;
    int result = GAME_FN(int (*)(int, int, int, int, int, int), RVA_MARK_RIVER)(x1, y1, x2, y2, mark, mode);
    clearMarksOutsidePlane();
    lua_pushinteger(L, result);
    return 1;
}

// coe5.gen.marksToTerrain(mark, from, to): turns marked squares of one terrain into another.
static int lGenMarksToTerrain(lua_State *L) {
    int mark, from, to;
    if (!genMark(L, "coe5.gen.marksToTerrain", 1, &mark) || !genCode(L, "coe5.gen.marksToTerrain", 2, &from) ||
        !genTerrain(L, "coe5.gen.marksToTerrain", 3, &to) || !genBuilding(L, "coe5.gen.marksToTerrain"))
        return 0;
    lua_pushinteger(L, GAME_FN(int (*)(int, int, int), RVA_GEN_MARKS_TO_TERRAIN)(mark, from, to));
    return 1;
}

// coe5.gen.marksToTerrainChance(mark, terrain, percent): turns marked squares into a terrain by chance.
static int lGenMarksToTerrainChance(lua_State *L) {
    int mark, terrain, percent;
    if (!genMark(L, "coe5.gen.marksToTerrainChance", 1, &mark) || !genTerrain(L, "coe5.gen.marksToTerrainChance", 2, &terrain) ||
        !genInt(L, "coe5.gen.marksToTerrainChance", 3, "percent", 0, 100, &percent) || !genBuilding(L, "coe5.gen.marksToTerrainChance"))
        return 0;
    SquareRect a = planeBuild.area;
    const short *marks = markLayer();
    int changed = 0;
    for (int x = a.x; x < a.x + a.w; x++)
        for (int y = a.y; y < a.y + a.h; y++)
            if (marks[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y] == mark && GAME_FN(int (*)(int), RVA_RANDOM)(100) < percent) {
                *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE) = (short)terrain;
                changed++;
            }
    lua_pushinteger(L, changed);
    return 1;
}

// The squares of the plane marked `mark`.
static int countPlaneMarks(int mark) {
    SquareRect a = planeBuild.area;
    const short *marks = markLayer();
    int n = 0;
    for (int x = a.x; x < a.x + a.w; x++)
        for (int y = a.y; y < a.y + a.h; y++) n += marks[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y] == mark;
    return n;
}

// coe5.gen.markNearTerrain(terrain, code, mark): marks squares of a terrain next to a code.
static int lGenMarkNearTerrain(lua_State *L) {
    int terrain, code, mark;
    if (!genCode(L, "coe5.gen.markNearTerrain", 1, &terrain) || !genCode(L, "coe5.gen.markNearTerrain", 2, &code) ||
        !genMark(L, "coe5.gen.markNearTerrain", 3, &mark) || !genBuilding(L, "coe5.gen.markNearTerrain"))
        return 0;
    GAME_FN(int (*)(int, int, int), RVA_GEN_MARK_NEAR_TERRAIN)(terrain, code, mark);
    clearMarksOutsidePlane();
    lua_pushinteger(L, countPlaneMarks(mark));
    return 1;
}

// coe5.gen.markVoidNear(code, mark, radius): marks Void squares near a code.
static int lGenMarkVoidNear(lua_State *L) {
    int code, mark, radius;
    if (!genCode(L, "coe5.gen.markVoidNear", 1, &code) || !genMark(L, "coe5.gen.markVoidNear", 2, &mark) ||
        !genInt(L, "coe5.gen.markVoidNear", 3, "radius", 0, MAP_COLUMN_SQUARES, &radius) || !genBuilding(L, "coe5.gen.markVoidNear"))
        return 0;
#ifdef __APPLE__
    GAME_FN(int (*)(int, int, int), RVA_GEN_MARK_VOID_NEAR)(code, mark, radius);
#else
    GAME_FN(int (*)(int, int, int, int), RVA_GEN_MARK_VOID_NEAR)(0, code, mark, radius);
#endif
    clearMarksOutsidePlane();
    lua_pushinteger(L, countPlaneMarks(mark));
    return 1;
}

// coe5.gen.findCaveSpot(need): a random Cave Wall square with enough Cave Walls around it.
static int lGenFindCaveSpot(lua_State *L) {
    int need, x = -1, y = -1;
    if (!genInt(L, "coe5.gen.findCaveSpot", 1, "need", 0, 25, &need) || !genBuilding(L, "coe5.gen.findCaveSpot")) return 0;
#ifdef __APPLE__
    int found = GAME_FN(int (*)(int, int, int *, int *), RVA_GEN_FIND_CAVE_SPOT)(planeBuild.index, need, &x, &y);
#else
    int found = GAME_FN(int (*)(int, int, int, int *, int *), RVA_GEN_FIND_CAVE_SPOT)(planeBuild.index, 0, need, &x, &y);
#endif
    if (!found || !insidePlaneBuilt(x, y)) return 0;
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

// coe5.gen.setFixedTemperature(x, y, w, h): temperature 25 on the rectangle ("setfixtemp").
static int lGenSetFixedTemperature(lua_State *L) {
    SquareRect r;
    if (!genRect(L, "coe5.gen.setFixedTemperature", 1, &r)) return 0;
    GAME_FN(void (*)(int, int, int, int), RVA_GEN_SET_FIXED_TEMPERATURE)(r.x, r.y, r.w, r.h);
    return 0;
}

// coe5.gen.setTemperatures(x, y, w, h): sets a rectangle's temperatures as the game does.
#define HEAT_REACH 6
static int lGenSetTemperatures(lua_State *L) {
    SquareRect r;
    if (!genRect(L, "coe5.gen.setTemperatures", 1, &r)) return 0;
    SquareGuard g;
    guardSquares(&g, r.x - HEAT_REACH, r.y - HEAT_REACH, r.x + r.w + HEAT_REACH, r.y + r.h + HEAT_REACH);
    GAME_FN(void (*)(int, int, int, int), RVA_SET_TEMPERATURE_RECT)(r.x, r.y, r.w, r.h);
    unguardSquares(&g);
    return 0;
}

// coe5.gen.setGround(x, y, w, h, terrain or -1): sets a rectangle's ground kinds.
static int lGenSetGround(lua_State *L) {
    SquareRect r;
    int terrain;
    if (!genRect(L, "coe5.gen.setGround", 1, &r) || !genInt(L, "coe5.gen.setGround", 5, "terrain", -1, G32(RVA_LAST_TERRAIN), &terrain)) return 0;
    GAME_FN(void (*)(int, int, int, int, int), RVA_SET_GROUND_RECT)(r.x, r.y, r.w, r.h, terrain);
    return 0;
}

// coe5.gen.floodPlane(): floods open squares joined to water; returns how many.
static int lGenFloodPlane(lua_State *L) {
    if (!genBuilding(L, "coe5.gen.floodPlane")) return 0;
    lua_pushinteger(L, GAME_FN(int (*)(int), RVA_GEN_FLOOD_PLANE)(planeBuild.index));
    return 1;
}

// coe5.gen.updateMoistWalls(): Cave Wall next to water becomes Moist Cave Wall (RVA_GEN_UPDATE_MOIST_WALLS).
static int lGenUpdateMoistWalls(lua_State *L) {
    if (!genBuilding(L, "coe5.gen.updateMoistWalls")) return 0;
    GAME_FN(void (*)(int), RVA_GEN_UPDATE_MOIST_WALLS)(planeBuild.index);
    return 0;
}

// coe5.gen.fixDiagonals(): closes corner-only gaps in Rock Ground with Cave Wall.
static int lGenFixDiagonals(lua_State *L) {
    if (!genBuilding(L, "coe5.gen.fixDiagonals")) return 0;
#ifdef __APPLE__
    int saved[4] = {planeField(3, PLANE_X), planeField(3, PLANE_Y), planeField(3, PLANE_W), planeField(3, PLANE_H)};
    setPlaneField(3, PLANE_X, planeBuild.area.x), setPlaneField(3, PLANE_Y, planeBuild.area.y);
    setPlaneField(3, PLANE_W, planeBuild.area.w), setPlaneField(3, PLANE_H, planeBuild.area.h);
    int result = GAME_FN(int (*)(void), RVA_GEN_DIAGONAL_FIX)();
    setPlaneField(3, PLANE_X, saved[0]), setPlaneField(3, PLANE_Y, saved[1]);
    setPlaneField(3, PLANE_W, saved[2]), setPlaneField(3, PLANE_H, saved[3]);
#else
    int result = GAME_FN(int (*)(int), RVA_GEN_DIAGONAL_FIX)(planeBuild.index);
#endif
    lua_pushinteger(L, result);
    return 1;
}

// coe5.gen.spreadTerrain(x, y, tries, onto): spreads a square's terrain onto neighbours.
static int lGenSpreadTerrain(lua_State *L) {
    int x, y, tries, onto;
    if (!genSquare(L, "coe5.gen.spreadTerrain", 1, &x, &y) || !genInt(L, "coe5.gen.spreadTerrain", 3, "tries", 0, 1000, &tries) ||
        !genCode(L, "coe5.gen.spreadTerrain", 4, &onto))
        return 0;
    SquareGuard g;
    guardSquares(&g, x - 1, y - 1, x + 2, y + 2);
    int changed = GAME_FN(int (*)(int, int, int, int), RVA_GEN_SPREAD_TERRAIN)(x, y, tries, onto);
    unguardSquares(&g);
    lua_pushinteger(L, changed);
    return 1;
}

// coe5.gen.specBlob(x, y, wx, wy, n, t1, t2[, a, b, c]): grows Elysium's terrain blob.
static int lGenSpecBlob(lua_State *L) {
    int x, y, wx, wy, n, t1, t2, a = 0, b = 0, c = 0;
    const char *call = "coe5.gen.specBlob";
    if (!genSquare(L, call, 1, &x, &y) || !genInt(L, call, 3, "wx", 0, 100000, &wx) || !genInt(L, call, 4, "wy", 0, 100000, &wy) ||
        !genInt(L, call, 5, "n", 1, genPlaneSquares(), &n) || !genTerrain(L, call, 6, &t1) || !genTerrain(L, call, 7, &t2) ||
        (!lua_isnoneornil(L, 8) && !genInt(L, call, 8, "a", INT_MIN, INT_MAX, &a)) ||
        (!lua_isnoneornil(L, 9) && !genInt(L, call, 9, "b", INT_MIN, INT_MAX, &b)) ||
        (!lua_isnoneornil(L, 10) && !genInt(L, call, 10, "c", INT_MIN, INT_MAX, &c)))
        return 0;
    SquareGuard g;
    guardSquares(&g, x - n, y - n, x + n + 1, y + n + 1);
    GAME_FN(void (*)(int, int, int, int, int, int, int, int, int, int), RVA_MAPGEN_SPEC_BLOB)(x, y, wx, wy, n, t1, t2, a, b, c);
    unguardSquares(&g);
    return 0;
}

// coe5.gen.drawRiver(x, y, tx, ty): draws Elysium's river between two squares.
#define RIVER_REACH 8
static int lGenDrawRiver(lua_State *L) {
    int x, y, tx, ty;
    if (!genSquare(L, "coe5.gen.drawRiver", 1, &x, &y) || !genSquare(L, "coe5.gen.drawRiver", 3, &tx, &ty)) return 0;
    SquareRect a = planeBuild.area;
    SquareGuard g;
    guardSquares(&g, a.x - RIVER_REACH, a.y - RIVER_REACH, a.x + a.w + RIVER_REACH, a.y + a.h + RIVER_REACH);
    GAME_FN(void (*)(int, int, int, int), RVA_MAPGEN_DRAW_RIVER)(x, y, tx, ty);
    unguardSquares(&g);
    return 0;
}

// coe5.gen.decorateSquare(x, y): adds Elysium's sites to one square.
#define DECORATE_REACH 16
static int lGenDecorateSquare(lua_State *L) {
    int x, y;
    if (!genSquare(L, "coe5.gen.decorateSquare", 1, &x, &y)) return 0;
    SquareGuard g;
    guardSquares(&g, x - DECORATE_REACH, y - DECORATE_REACH, x + DECORATE_REACH + 1, y + DECORATE_REACH + 1);
    GAME_FN(void (*)(int, int), RVA_DECORATE_SQUARE)(x, y);
    unguardSquares(&g);
    return 0;
}

// coe5.gen.addGroundItems(x, y, count, level): puts random items on a square's ground.
static int lGenAddGroundItems(lua_State *L) {
    int x, y, count, level;
    if (!genSquare(L, "coe5.gen.addGroundItems", 1, &x, &y) || !genInt(L, "coe5.gen.addGroundItems", 3, "count", 1, 50, &count) ||
        !genInt(L, "coe5.gen.addGroundItems", 4, "level", 0, 1000, &level))
        return 0;
    GAME_FN(void (*)(int, int, int, int), RVA_ADD_GROUND_ITEMS)(x, y, count, level);
    return 0;
}

// coe5.gen.randomSquareOfTerrain(code): a random square matching a terrain; x, y, or nil.
static int lGenRandomSquareOfTerrain(lua_State *L) {
    int code, x = -1, y = -1;
    if (!genCode(L, "coe5.gen.randomSquareOfTerrain", 1, &code) || !genBuilding(L, "coe5.gen.randomSquareOfTerrain")) return 0;
    int found = GAME_FN(int (*)(int, int, int *, int *), RVA_RANDOM_SQUARE_OF_TERRAIN)(planeBuild.index, code, &x, &y);
    if (!found || !insidePlaneBuilt(x, y)) return 0;
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

// Whether (x, y) can hold the far end of a portal: a square of a plane, inside the world.
static int portalEndAllowed(lua_Integer x, lua_Integer y) {
    return x >= 0 && y >= 0 && x < G32(RVA_MAP_DEPTH) && x < MAP_COLUMNS_MAX && y < G32(RVA_MAP_H) && y < MAP_COLUMN_SQUARES &&
           GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)x, (int)y) >= 0;
}

// Writes a portal pair between two squares, as the game does.
static void setPortalPair(int x1, int y1, int x2, int y2, int twoway, int n) {
    *(short *)(squareRecordAt(x1, y1) + SQUARE_PORTAL) = (short)n;
    *(short *)(squareRecordAt(x2, y2) + SQUARE_PORTAL) = (short)(twoway ? n : -n);
}

// A portal number no square holds yet, picked as the game picks it.
static int newPortalNumber(void) {
    int n = PORTAL_NUMBER_FIRST, x, y;
    for (int i = 0; i < PORTAL_NUMBER_TRIES; i++) {
        n = PORTAL_NUMBER_FIRST + GAME_FN(int (*)(int), RVA_RANDOM)(PORTAL_NUMBER_SPAN);
        if (!GAME_FN(int (*)(int, int *, int *, int, int), RVA_RANDOM_PORTAL_EXIT)(n, &x, &y, -1, -1)) break;
    }
    return n;
}

// coe5.gen.setPortalPair and coe5.gen.linkNewPortal: a portal from the plane to any square.
static int genPortal(lua_State *L, const char *call, int pick) {
    int x1, y1, n = 0;
    if (!genSquare(L, call, 1, &x1, &y1)) return 0;
    lua_Integer x2 = luaL_checkinteger(L, 3), y2 = luaL_checkinteger(L, 4);
    int twoway = lua_toboolean(L, 5);
    if (!portalEndAllowed(x2, y2)) {
        refuseLuaCall(genMod(L), call, "no_plane", "x=%lld y=%lld", (long long)x2, (long long)y2);
        return 0;
    }
    if (!pick && !genInt(L, call, 6, "n", 1, SHRT_MAX, &n)) return 0;
    if (pick) n = newPortalNumber();
    setPortalPair(x1, y1, (int)x2, (int)y2, twoway, n);
    lua_pushinteger(L, n);
    return 1;
}
static int lGenSetPortalPair(lua_State *L) { return genPortal(L, "coe5.gen.setPortalPair", 0); }
static int lGenLinkNewPortal(lua_State *L) { return genPortal(L, "coe5.gen.linkNewPortal", 1); }

// coe5.gen.refuseSmallPlane(step, minW, minH): true when the plane is big enough for the step.
static int lGenRefuseSmallPlane(lua_State *L) {
    const char *step = luaL_checkstring(L, 1);
    int minW, minH;
    if (!genInt(L, "coe5.gen.refuseSmallPlane", 2, "min_w", 1, MAP_COLUMNS_MAX, &minW) ||
        !genInt(L, "coe5.gen.refuseSmallPlane", 3, "min_h", 1, MAP_COLUMN_SQUARES, &minH) || !genBuilding(L, "coe5.gen.refuseSmallPlane"))
        return 0;
    int fits = planeBuild.area.w >= minW && planeBuild.area.h >= minH;
    if (!fits)
        refuseLuaCall(genMod(L), "plane_step", "plane_too_small", "step=%.60s plane=%d w=%d h=%d min_w=%d min_h=%d", step, planeBuild.index,
                      planeBuild.area.w, planeBuild.area.h, minW, minH);
    lua_pushboolean(L, fits);
    return 1;
}

static int lGenStampPiece(lua_State *L);
static const luaL_Reg LUA_GEN[] = {
    {"random", lGenRandom}, {"markAt", lGenMarkAt}, {"setMark", lGenSetMark}, {"setTerrain", lGenSetTerrain},
    {"terrainFlags", lGenTerrainFlags}, {"terrainNear", lGenTerrainNear}, {"markRect", lGenMarkRect}, {"markWalk", lGenMarkWalk},
    {"markBlob", lGenMarkBlob}, {"markRiver", lGenMarkRiver}, {"marksToTerrain", lGenMarksToTerrain},
    {"marksToTerrainChance", lGenMarksToTerrainChance}, {"markNearTerrain", lGenMarkNearTerrain}, {"markVoidNear", lGenMarkVoidNear},
    {"findCaveSpot", lGenFindCaveSpot}, {"setFixedTemperature", lGenSetFixedTemperature}, {"setTemperatures", lGenSetTemperatures},
    {"setGround", lGenSetGround}, {"floodPlane", lGenFloodPlane}, {"updateMoistWalls", lGenUpdateMoistWalls},
    {"fixDiagonals", lGenFixDiagonals}, {"spreadTerrain", lGenSpreadTerrain}, {"specBlob", lGenSpecBlob}, {"drawRiver", lGenDrawRiver},
    {"decorateSquare", lGenDecorateSquare}, {"addGroundItems", lGenAddGroundItems}, {"randomSquareOfTerrain", lGenRandomSquareOfTerrain},
    {"setPortalPair", lGenSetPortalPair}, {"linkNewPortal", lGenLinkNewPortal}, {"refuseSmallPlane", lGenRefuseSmallPlane},
    {"stampPiece", lGenStampPiece}, {NULL, NULL},
};

// ---- The recipes and the generate functions -----------------------------------------------------
static char planeRecipesKey;
static void pushLuaTextFile(lua_State *L, const char *path, const char *chunk);

// The path of CoE5SE's own file `file` (beside CoE5SE.c5m); 0 when CoE5SE's folder isn't known.
static const char *planeRecipesPathForTests;
static int coe5seFilePath(const char *file, char *path, size_t size) {
    if (planeRecipesPathForTests) return buildPath(path, size, __func__, "%s", planeRecipesPathForTests);
    for (int k = 0; k < orderedModCount; k++)
        if (!strcmp(orderedMods[k].name, COE5SE_MOD))
            return buildPath(path, size, __func__, "%s" PATH_SEP "%s", modFolder(orderedMods[k].tag), file);
    return 0;
}

// Runs the recipes file in L, under its own protected call: argument 1 its path; returns its table.
static int lLoadPlaneRecipes(lua_State *L) {
    pushLuaTextFile(L, lua_tostring(L, 1), PLANE_RECIPES_FILE);
    lua_call(L, 0, 1);
    if (!lua_istable(L, -1)) return luaL_error(L, "%s returned no table", PLANE_RECIPES_FILE);
    return 1;
}

// Loads the recipes into script state L once; 1 when they are there. `call` names what asked for them: coe5.addPlane, or a block's planelike.
static int planeRecipesLoaded(lua_State *L, const char *mod, const char *call) {
    if (lua_rawgetp(L, LUA_REGISTRYINDEX, &planeRecipesKey) == LUA_TTABLE) {
        lua_pop(L, 1);
        return 1;
    }
    lua_pop(L, 1);
    char path[MAX_PATH];
    if (!coe5seFilePath(PLANE_RECIPES_FILE, path, sizeof path)) {
        refuseLuaCall(mod, call, "recipes_not_loaded", "error=no_coe5se_folder");
        return 0;
    }
    lua_pushcfunction(L, lLoadPlaneRecipes);
    lua_pushstring(L, path);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        refuseLuaCall(mod, call, "recipes_not_loaded", "error=%s", lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "?");
        lua_pop(L, 1);
        return 0;
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &planeRecipesKey);
    return 1;
}

// The steps CoE5SE calls in a plane's script: the recipe's build, its special squares, the generate function.
enum { PLANE_STEP_BUILD, PLANE_STEP_SPECIAL, PLANE_STEP_GENERATE };
typedef struct { const CustomPlane *p; int step; } PlaneStepCall;
static int lNoPlaneStep(lua_State *L) {
    (void)L;
    return 0;
}

// callLuaProtected's push: the step's function, then the plane as a table.
static int pushPlaneStep(lua_State *L, void *ctx) {
    const PlaneStepCall *c = ctx;
    const CustomPlane *p = c->p;
    if (c->step == PLANE_STEP_GENERATE) lua_rawgeti(L, LUA_REGISTRYINDEX, p->generateRef);
    else {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &planeRecipesKey);
        if (lua_type(L, -1) != LUA_TTABLE || lua_getfield(L, -1, VANILLA_PLANE_SHORT_NAMES[p->recipe]) != LUA_TTABLE ||
            lua_getfield(L, -1, c->step == PLANE_STEP_BUILD ? "build" : "special") != LUA_TFUNCTION) {
            lua_settop(L, 0);
            lua_pushcfunction(L, lNoPlaneStep);
        } else {
            lua_replace(L, 1);
            lua_settop(L, 1);
        }
    }
    lua_createtable(L, 0, 8);
    lua_pushstring(L, p->name), lua_setfield(L, -2, "name");
    lua_pushinteger(L, p->index), lua_setfield(L, -2, "index");
    lua_pushinteger(L, planeField(p->index, PLANE_X)), lua_setfield(L, -2, "x");
    lua_pushinteger(L, planeField(p->index, PLANE_Y)), lua_setfield(L, -2, "y");
    lua_pushinteger(L, p->w), lua_setfield(L, -2, "w");
    lua_pushinteger(L, p->h), lua_setfield(L, -2, "h");
    if (p->recipe >= 0) lua_pushstring(L, VANILLA_PLANE_SHORT_NAMES[p->recipe]), lua_setfield(L, -2, "like");
    lua_pushstring(L, p->mod), lua_setfield(L, -2, "mod");
    lua_pushinteger(L, planeField(0, PLANE_W)), lua_setfield(L, -2, "elysiumW");
    lua_pushinteger(L, planeField(0, PLANE_H)), lua_setfield(L, -2, "elysiumH");
#ifdef __APPLE__
    lua_pushliteral(L, "mac");
#else
    lua_pushliteral(L, "windows");
#endif
    lua_setfield(L, -2, "platform");
    return 1;
}

// Runs step `step` of plane p in its script, as a build of the plane.
static void runPlaneStep(const CustomPlane *p, int step) {
    LuaScript *s = p->script;
    if (!s || s->dead || !s->L) {
        writeLog("plane_step_skipped", "mod=%s name=%s step=%d reason=script_stopped", p->mod, p->name, step);
        return;
    }
    SquareRect area = {planeField(p->index, PLANE_X), planeField(p->index, PLANE_Y), p->w, p->h};
    if (!beginPlaneBuild(p->index, area, p->mod, p->name)) return;
    PlaneStepCall call = {p, step};
    int ran = callLuaProtected(s, pushPlaneStep, &call, 0);
    endPlaneBuild();
    writeLog("plane_step", "mod=%s name=%s index=%d step=%s like=%s ran=%d", p->mod, p->name, p->index,
             step == PLANE_STEP_BUILD ? "build" : step == PLANE_STEP_SPECIAL ? "special" : "generate",
             p->recipe >= 0 ? VANILLA_PLANE_SHORT_NAMES[p->recipe] : "-", ran);
}

// Builds a placed plane: its recipe, then its generate function.
static LuaScript *wordPlaneScript(const CustomPlane *p);
static void clearPlacedPieces(CustomPlane *p);
static void stampPlanePieces(CustomPlane *p);
static void buildCustomPlane(CustomPlane *p) {
    clearPlacedPieces(p);
    if (!p->script && p->fromWords && p->recipe >= 0) p->script = wordPlaneScript(p);
    if (p->recipe >= 0) runPlaneStep(p, PLANE_STEP_BUILD);
    if (p->generateRef) runPlaneStep(p, PLANE_STEP_GENERATE);
    stampPlanePieces(p);
}

// ---- Special squares and the way in --------------------------------------------------------------
// Whether plane p has a portal leading in from off the plane.
static int planeHasWayIn(const CustomPlane *p) {
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    static unsigned char onPlane[SHRT_MAX + 1];
    memset(onPlane, 0, sizeof onPlane);
    int any = 0;
    for (int x = x0; x < x0 + p->w; x++)
        for (int y = y0; y < y0 + p->h; y++) {
            int portal = abs(*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL));
            if (portal > 0 && portal <= SHRT_MAX) onPlane[portal] = 1, any = 1;
        }
    if (!any) return 0;
    int depth = G32(RVA_MAP_DEPTH) < MAP_COLUMNS_MAX ? G32(RVA_MAP_DEPTH) : MAP_COLUMNS_MAX;
    int height = G32(RVA_MAP_H) < MAP_COLUMN_SQUARES ? G32(RVA_MAP_H) : MAP_COLUMN_SQUARES;
    for (int x = 0; x < depth; x++)
        for (int y = 0; y < height; y++) {
            if (x >= x0 && y >= y0 && x < x0 + p->w && y < y0 + p->h) continue;
            int portal = abs(*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL));
            if (portal > 0 && portal <= SHRT_MAX && onPlane[portal]) return 1;
        }
    return 0;
}

// Whether square (x, y) can take a gateway.
static int onPlacedPiece(const PlaneLayout *layout, int x, int y);
static int entranceSquareFree(const PlaneLayout *layout, int x, int y) {
    int terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
    short apcost = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? (short)G16(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_APCOST) : 0;
    return apcost < TERRAIN_APCOST_WALL && !*(short *)(squareRecordAt(x, y) + SQUARE_PORTAL) && (!layout || !onPlacedPiece(layout, x, y));
}

// A random matching square of a plane that can take a gateway; 0 when none.
static int randomFreeSquare(int index, int code, const PlaneLayout *layout, int *x, int *y) {
    for (int i = 0; i < PLANE_PORTAL_TRIES; i++) {
        if (!GAME_FN(int (*)(int, int, int *, int *), RVA_RANDOM_SQUARE_OF_TERRAIN)(index, code, x, y)) return 0;
        if (entranceSquareFree(layout, *x, *y)) return 1;
    }
    return 0;
}

// A random square of plane p for its gateway, water included; 0 when none.
static int anyFreeEntranceSquare(const CustomPlane *p, int *x, int *y) {
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y), count = 0;
    for (int sx = x0; sx < x0 + p->w; sx++)
        for (int sy = y0; sy < y0 + p->h; sy++) count += entranceSquareFree(&p->layout, sx, sy);
    if (!count) return 0;
    int pick = GAME_FN(int (*)(int), RVA_RANDOM)(count);
    for (int sx = x0; sx < x0 + p->w; sx++)
        for (int sy = y0; sy < y0 + p->h; sy++)
            if (entranceSquareFree(&p->layout, sx, sy) && pick-- == 0) {
                *x = sx, *y = sy;
                return 1;
            }
    return 0;
}

// Gives plane p a two-way gateway from Elysium when it has no way in.
static void addDefaultPortal(const CustomPlane *p) {
    if (planeHasWayIn(p)) {
        writeLog("plane_portal", "mod=%s name=%s result=has_way_in", p->mod, p->name);
        return;
    }
    int ex, ey, px = p->layout.wayInX, py = p->layout.wayInY;
    if (!randomFreeSquare(0, TERRAIN_CODE_FLAT, NULL, &ex, &ey) && !randomFreeSquare(0, TERRAIN_CODE_LAND, NULL, &ex, &ey)) {
        writeLog("plane_portal", "mod=%s name=%s result=no_elysium_square", p->mod, p->name);
        return;
    }
    if (px < 0 && !randomFreeSquare(p->index, TERRAIN_CODE_LAND, &p->layout, &px, &py) && !anyFreeEntranceSquare(p, &px, &py)) {
        writeLog("plane_entrance_missing", "mod=%s name=%s reason=no_free_square", p->mod, p->name);
        return;
    }
    int was = *(short *)(squareRecordAt(px, py) + SQUARE_TERRAIN_TYPE);
    GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN)(ex, ey, TERRAIN_GATEWAY);
    GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN)(px, py, TERRAIN_GATEWAY);
    int n = newPortalNumber();
    setPortalPair(px, py, ex, ey, 1, n);
    writeLog("plane_portal", "mod=%s name=%s result=added x=%d y=%d elysium_x=%d elysium_y=%d portal=%d terrain_before=%d", p->mod, p->name, px, py,
             ex, ey, n, was);
}

// After the game's special squares: each plane's recipe step, zone swaps and way in.
static unsigned char *guardPlacedPieces(const PlaneLayout *layout);
static void keepPlacedPieces(const CustomPlane *p, unsigned char *saved);
static void applyZoneSwaps(const CustomPlane *p);
static void addCustomPlanesSpecialSquares(void) {
    for (int i = 0; i < customPlaneCount; i++) {
        CustomPlane *p = &customPlanes[i];
        if (p->skipped || p->index < 0) continue;
        if (p->recipe >= 0) {
            unsigned char *pieces = guardPlacedPieces(&p->layout);
            runPlaneStep(p, PLANE_STEP_SPECIAL);
            keepPlacedPieces(p, pieces);
        }
        applyZoneSwaps(p);
        addDefaultPortal(p);
    }
}

// ---- Population and the swap --------------------------------------------------------------------
#ifdef __APPLE__
#define PLAYER_HORRORS 26
// The living units of the monster named `name`, as populate_void counts them before a unique.
static int livingUnitsNamed(const char *name) {
    int last = G32(RVA_UNIT_COUNT), n = 0;
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        int monster = G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE);
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) <= 0 || monster < 0 || monster >= MONSTER_TABLE_MAX) continue;
        n += !strcmp((const char *)GAME_PTR(RVA_MONSTER_TABLE) + (size_t)monster * MONSTER_SIZE, name);
    }
    return n;
}

// Mac's populate_void, which the game only inlines there.
typedef int (*PopulateFn)(int x, int y, const char *name, int count, int player, int commander);
static void populateVoidPort(void) {
    int x0 = planeField(9, PLANE_X), y0 = planeField(9, PLANE_Y), w = planeField(9, PLANE_W), h = planeField(9, PLANE_H);
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H), wilder = G32(RVA_WILDER);
    PopulateFn populate = GAME_FN(PopulateFn, RVA_POPULATE);
    int (*random)(int) = GAME_FN(int (*)(int), RVA_RANDOM);
    int (*die)(int) = GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED);
    void (*pool)(int, int, int) = GAME_FN(void (*)(int, int, int), RVA_POPULATE_VOID_POOL);
    void (*holdit)(int, int, int) = GAME_FN(void (*)(int, int, int), RVA_POPULATE_SET_HOLDIT);
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            int onMap = x >= 0 && y >= 0 && x < mapW && y < mapH;
            int owner = onMap ? *(signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) : -1;
            int terrain = onMap ? *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE) : -1;
            if ((owner >= 0 && owner != 24 && owner != PLAYER_HORRORS) || G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2))
                continue;
            int poolChance = 9;
            if (terrain == 0x102) {
                if (random(100) < 50 && !livingUnitsNamed("King in Yellow")) {
                    populate(x, y, "King in Yellow", -999, PLAYER_HORRORS, -1);
                    continue;
                }
                poolChance = 75;
            } else if (terrain == 0xe5 || terrain == 0x103) poolChance = 75;
            else if (terrain == 0xfa) {
                if (random(100) < 50 && !livingUnitsNamed("Vastness")) {
                    populate(x, y, "Vastness", -999, PLAYER_HORRORS, -1);
                    holdit(x, y, 1);
                }
                continue;
            } else if (terrain == 0xe8) {
                populate(x, y, "Void Lord", -999, PLAYER_HORRORS, -1);
                int hounds = random(wilder * 6) + 1;
                hounds += random(wilder * 6) + 1;
                populate(x, y, "Void Hound", hounds, PLAYER_HORRORS, -1);
                holdit(x, y, 1);
                continue;
            }
            if (random(100) < poolChance) pool(x, y, die(wilder * 12));
        }
}

// ---- Agartha's and the Celestial Plane's populations, ported for Mac ----
#define PLAYER_INDEPENDENTS 24
#define PLAYER_CELESTIAL    29
#define PLANE_POP_MAX(a, b) ((a) > (b) ? (a) : (b))
typedef struct {
    PopulateFn populate;
    int (*random)(int);
    int (*die)(int);
    int (*dice)(int, int);
    void (*holdit)(int, int, int);
    int (*summon)(int player, int x, int y, const char *text, int percent);
    int wilder;
} PopulateTools;
static PopulateTools populateTools(void) {
    return (PopulateTools){GAME_FN(PopulateFn, RVA_POPULATE), GAME_FN(int (*)(int), RVA_RANDOM), GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED),
                           GAME_FN(int (*)(int, int), RVA_DICE_OPEN_ENDED), GAME_FN(void (*)(int, int, int), RVA_POPULATE_SET_HOLDIT),
                           GAME_FN(int (*)(int, int, int, const char *, int), RVA_SUMMON_STRING), G32(RVA_WILDER)};
}

// Agartha's cave pool (Rock Ground).
static void agarthaCavePool(const PopulateTools *t, int x, int y, int n) {
    const char *name;
    switch (t->random(18)) {
    case 0: name = "Cave Grub", n = PLANE_POP_MAX(1, n / 3); break;
    case 1: name = "Purple Worm", n = 1; break;
    case 2:
        n /= 2;
        t->populate(x, y, "Strange Thing", PLANE_POP_MAX(1, n), PLAYER_INDEPENDENTS, -1);
        if (t->random(100) > 49) return;
        name = "Elder Thing", n = t->random(n) + 1;
        break;
    case 3: case 4: name = "Giant Ant", n = PLANE_POP_MAX(1, n / 3); break;
    case 5: name = "Large Spider", n = PLANE_POP_MAX(1, n); break;
    case 6: name = "Giant Spider", n = PLANE_POP_MAX(1, n / 4); break;
    case 7: name = "Caveman Hunter", n = PLANE_POP_MAX(1, n / 4); break;
    case 8:
        t->populate(x, y, "Caveman Shaman", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Caveman", PLANE_POP_MAX(1, n / 2), PLAYER_INDEPENDENTS, -1);
        n /= 4;
        t->populate(x, y, "Caveman Hunter", PLANE_POP_MAX(1, n), PLAYER_INDEPENDENTS, -1);
        name = "Caveman Warrior", n = PLANE_POP_MAX(1, n);
        break;
    case 12: case 13: name = "Troglodyte", n = PLANE_POP_MAX(1, n / 3); break;
    case 14: case 15: name = "Cave Cow", n = PLANE_POP_MAX(1, n / 3); break;
    case 16: name = "Yithian", n = PLANE_POP_MAX(1, n / 4); break;
    case 17: name = "Naga", n = PLANE_POP_MAX(1, n); break;
    default:
        t->populate(x, y, "Green Ooze", n / 4 + 1, PLAYER_INDEPENDENTS, -1);
        name = "Small Ooze", n = t->random(2);
        break;
    }
    t->populate(x, y, name, n, PLAYER_INDEPENDENTS, -1);
}

// Agartha's Pale One pool (the deep deposits), then holdit.
static void agarthaPaleOnePool(const PopulateTools *t, int x, int y, int n) {
    const char *name;
    int r = t->random(24);
    switch (r) {
    case 0:
        if (t->random(2) == 0) t->populate(x, y, "Earth Reader", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Ancient Commander", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Ancient Pale One", n / 3, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Cavern Guard", n, PLAYER_INDEPENDENTS, -1);
        name = "Pale One";
        break;
    case 6: case 7: name = "Earth Elemental", n = 1; break;
    case 8: case 9: name = "Lesser Earth", n = t->random(3) + 1; break;
    case 10: case 11: t->populate(x, y, "Umbral", n / 2, PLAYER_INDEPENDENTS, -1), name = "Penumbral"; break;
    case 12: case 13: t->populate(x, y, "Olm Sage", 1, PLAYER_INDEPENDENTS, -1), name = "Olm"; break;
    case 14: case 15: name = "Magma Child"; break;
    case 16: case 17: name = "Cave Grub"; break;
    case 18: case 19: name = "Stone Drake", n = PLANE_POP_MAX(1, n / 3); break;
    case 20: case 21: name = "Earth Gnome", n = PLANE_POP_MAX(1, n / 2); break;
    case 22: case 23: t->populate(x, y, "Naga Warrior", n / 2, PLAYER_INDEPENDENTS, -1), name = "Naga"; break;
    default:
        t->populate(x, y, "Pale One Commander", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Pale One Soldier", n, PLAYER_INDEPENDENTS, -1);
        name = "Pale One", n *= 2;
        break;
    }
    t->populate(x, y, name, n, PLAYER_INDEPENDENTS, -1);
    t->holdit(x, y, 1);
}

// Agartha's crystal pool (Crystal Forest).
static void agarthaCrystalPool(const PopulateTools *t, int x, int y, int n) {
    const char *name;
    switch (t->random(11)) {
    case 0:
        t->populate(x, y, "Crystal", n, PLAYER_INDEPENDENTS, -1);
        if (t->random(100) > 4) return;
        name = "Crystal Golem", n = 1;
        break;
    case 1: name = "Cave Cow", n = PLANE_POP_MAX(1, n / 3); break;
    case 2: name = "Cave Grub", n = PLANE_POP_MAX(1, n / 3); break;
    case 3:
        t->populate(x, y, "Elder Thing", PLANE_POP_MAX(1, n / 3), PLAYER_INDEPENDENTS, -1);
        name = "Strange Thing", n = PLANE_POP_MAX(1, n / 2);
        break;
    case 4:
        t->populate(x, y, "Yithian", t->random(4) + 1, PLAYER_INDEPENDENTS, -1);
        name = "Strange Thing", n = PLANE_POP_MAX(1, n / 2);
        break;
    case 5:
        t->populate(x, y, "Caveman Shaman", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Caveman", PLANE_POP_MAX(1, n / 2), PLAYER_INDEPENDENTS, -1);
        n /= 4;
        t->populate(x, y, "Caveman Hunter", PLANE_POP_MAX(1, n), PLAYER_INDEPENDENTS, -1);
        name = "Caveman Warrior", n = PLANE_POP_MAX(1, n);
        break;
    case 6: name = "Caveman Hunter", n = PLANE_POP_MAX(1, n / 4); break;
    case 9: case 10: t->populate(x, y, "Naga Warrior", n / 2, PLAYER_INDEPENDENTS, -1), name = "Naga"; break;
    default:
        t->populate(x, y, "Ochre Jelly", n / 2 + 1, PLAYER_INDEPENDENTS, -1);
        name = "Small Jelly", n = t->random(2);
        break;
    }
    t->populate(x, y, name, n, PLAYER_INDEPENDENTS, -1);
}

// Agartha's cave temple pool, then holdit.
static void agarthaTemplePool(const PopulateTools *t, int x, int y, int n) {
    const char *name;
    int r = t->random(10);
    if (r <= 1) {
        t->populate(x, y, "Earth Reader", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Ancient Commander", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Ancient Pale One", n / 3, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Cavern Guard", n, PLAYER_INDEPENDENTS, -1);
        name = "Pale One";
    } else if (r <= 5) {
        t->populate(x, y, "Earth Reader", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Pale One Commander", 1, PLAYER_INDEPENDENTS, -1);
        t->populate(x, y, "Pale One Soldier", n, PLAYER_INDEPENDENTS, -1);
        name = "Pale One", n *= 2;
    } else if (r == 6) t->populate(x, y, "Olm Sage", 1, PLAYER_INDEPENDENTS, -1), name = "Olm";
    else if (r == 7) name = "Magma Child";
    else if (r == 8) name = "Earth Gnome", n = PLANE_POP_MAX(1, n / 2);
    else t->populate(x, y, "Naga Warrior", n / 2, PLAYER_INDEPENDENTS, -1), name = "Naga";
    t->populate(x, y, name, n, PLAYER_INDEPENDENTS, -1);
    t->holdit(x, y, 1);
}

// populate_agartha, ported for Mac: every free square of record 3 by its terrain.
static void populateAgarthaPort(void) {
    PopulateTools t = populateTools();
    int x0 = planeField(3, PLANE_X), y0 = planeField(3, PLANE_Y), w = planeField(3, PLANE_W), h = planeField(3, PLANE_H);
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H), W = t.wilder;
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            int onMap = x >= 0 && y >= 0 && x < mapW && y < mapH;
            int owner = onMap ? *(signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) : -1;
            if ((G32(RVA_SQUARE_FLAGS + square * SQUARE_SIZE) & 0x40) || (owner >= 0 && owner != PLAYER_INDEPENDENTS) ||
                G16(RVA_SQUARE_ARMY + square * 2))
                continue;
            int terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
            switch (terrain) {
            case 0x157: agarthaTemplePool(&t, x, y, t.dice(W * 5, 2)); break;
            case 0x14b:
                t.summon(PLAYER_INDEPENDENTS, x, y, "Hound of Hades", 100);
                t.holdit(x, y, 1);
                break;
            case 0x2f:
                if (t.random(100) < 8) agarthaCavePool(&t, x, y, t.die(W * 6));
                break;
            case 0x34: case 0x35:
                if (t.random(100) < 20) {
                    int n = t.die(W * 6);
                    int r = t.random(4);
                    if (r == 0) {
                        t.populate(x, y, "Fire Elemental", n / 4, PLAYER_INDEPENDENTS, -1);
                        t.populate(x, y, "Lesser Fire", n, PLAYER_INDEPENDENTS, -1);
                    } else if (r == 1) t.populate(x, y, "Fire Giant", t.random(n / 5) + 1, PLAYER_INDEPENDENTS, -1);
                    else if (r == 2) {
                        t.populate(x, y, "Ember Warrior", t.random(n / 2) + 1, PLAYER_INDEPENDENTS, -1);
                        t.populate(x, y, "Lesser Fire", PLANE_POP_MAX(1, n / 3), PLAYER_INDEPENDENTS, -1);
                    } else t.populate(x, y, "Flame Spirit", PLANE_POP_MAX(1, n / 4), PLAYER_INDEPENDENTS, -1);
                }
                break;
            case 0xae: case 0xaf: case 0xb0: case 0xb1: case 0xbb: agarthaPaleOnePool(&t, x, y, t.dice(W * 5, 2)); break;
            case 0xb3: agarthaPaleOnePool(&t, x, y, t.dice(W * 6, 3)); break;
            case 0xba: agarthaPaleOnePool(&t, x, y, t.dice(W * 5, 3)); break;
            case 0xc3:
                if (t.random(100) < 40) {
                    int n = t.die(W * 6);
                    switch (t.random(8)) {
                    case 4: t.populate(x, y, "Giant Ant", n / 3 + 1, PLAYER_INDEPENDENTS, -1); break;
                    case 5: t.populate(x, y, "Large Spider", n, PLAYER_INDEPENDENTS, -1); break;
                    case 6:
                        t.populate(x, y, "Caveman Shaman", 1, PLAYER_INDEPENDENTS, -1);
                        t.populate(x, y, "Caveman", PLANE_POP_MAX(1, n / 2), PLAYER_INDEPENDENTS, -1);
                        t.populate(x, y, "Caveman Hunter", PLANE_POP_MAX(1, n / 4), PLAYER_INDEPENDENTS, -1);
                        t.populate(x, y, "Caveman Warrior", PLANE_POP_MAX(1, n / 4), PLAYER_INDEPENDENTS, -1);
                        break;
                    case 7: t.populate(x, y, "Caveman Hunter", n / 4 + 1, PLAYER_INDEPENDENTS, -1); break;
                    default: GAME_FN(void (*)(int, int, int), RVA_POPULATE_FUNGUS)(x, y, n); break;
                    }
                }
                break;
            case 0xc4:
                if (t.random(100) < 30) agarthaCrystalPool(&t, x, y, t.die(W * 8));
                break;
            case 0xd1: {
                t.populate(x, y, "King of Mountains", -999, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Cyclops", 2, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Purple Worm", 2, PLAYER_INDEPENDENTS, -1);
                int a = t.random(4), b = t.random(4), c = t.random(4);
                t.populate(x, y, "Child of Oreigenes", a + b + 3 + c, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Champion of Mountains", t.random(4) + 1, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Earth Elemental", t.random(6) + 1, PLAYER_INDEPENDENTS, -1);
                a = t.random(4), b = t.random(4);
                t.populate(x, y, "Lesser Earth", a + 2 + b, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Stone Drake", t.random(6) + 1, PLAYER_INDEPENDENTS, -1);
                int crystals = 0;
                for (int i = 0; i < 4; i++) crystals += t.random(W * 6) + 1;
                t.populate(x, y, "Crystal", crystals, PLAYER_INDEPENDENTS, -1);
                GAME_FN(void (*)(int, int, int, int), RVA_ADD_GROUND_ITEMS)(x, y, t.random(2) + 1, 30);
                t.holdit(x, y, 1);
                break;
            }
            case 0xe1:
                t.populate(x, y, "Oracle of Allatu", 1, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Onyx Oracle", 1, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Granite Guardian", 2, PLAYER_INDEPENDENTS, -1);
                t.populate(x, y, "Seal Guard", 10, PLAYER_INDEPENDENTS, -1);
                t.holdit(x, y, 1);
                break;
            default: break;
            }
        }
}

// A Celestial court's priests: 10% each of a Saint, Pilgrim, Templar, Paladin and Bishop.
static void celestialPriests(const PopulateTools *t, int x, int y) {
    static const char *const PRIESTS[] = {"Saint", "Pilgrim", "Templar", "Paladin", "Bishop"};
    for (size_t i = 0; i < sizeof PRIESTS / sizeof PRIESTS[0]; i++)
        if (t->random(100) < 10) t->populate(x, y, PRIESTS[i], 1, PLAYER_CELESTIAL, -1);
}

// The Celestial Plane's forest pool (player 24).
static void celestialForestPool(const PopulateTools *t, int x, int y, int n) {
    const char *name;
    switch (t->random(14)) {
    case 0:
        if (t->random(100) < 10) t->populate(x, y, "Bear", t->random(n / 3) + 1, PLAYER_INDEPENDENTS, -1);
        name = "Serpent", n = PLANE_POP_MAX(1, n * 2 / 3);
        break;
    case 1: name = "Snake"; break;
    case 2:
        if (t->random(100) < 5) t->populate(x, y, "Bear", t->random(n / 3) + 1, PLAYER_INDEPENDENTS, -1);
        name = "Wolf";
        break;
    case 3: GAME_FN(void (*)(int, int, int), RVA_POPULATE_FUNGUS)(x, y, n); return;
    case 4:
        if (t->random(100) < 15) t->populate(x, y, "Bear", t->random(n / 3) + 1, PLAYER_INDEPENDENTS, -1);
        name = "Deer";
        break;
    case 5:
        n /= 3;
        t->populate(x, y, "Moose", PLANE_POP_MAX(1, n), PLAYER_INDEPENDENTS, -1);
        if (t->random(100) > 4) return;
        name = "Bear", n = t->random(n) + 1;
        break;
    case 6: name = "Bear", n = PLANE_POP_MAX(1, n / 4); break;
    case 7: name = "Venus Trap", n = n / 3 + 1; break;
    case 8:
        if (t->random(100) < 7) name = "Giant Mantis", n = 1;
        else name = "Giant Ant", n = PLANE_POP_MAX(1, n / 3);
        break;
    case 9: name = "Boar"; break;
    case 10: name = "Blue Giant", n = PLANE_POP_MAX(1, n / 3); break;
    case 11:
        n /= 3;
        t->populate(x, y, "Giant Spider", PLANE_POP_MAX(1, n), PLAYER_INDEPENDENTS, -1);
        if (t->random(100) > 4) return;
        name = "Bear", n = t->random(n) + 1;
        break;
    case 12: name = "Moose", n = PLANE_POP_MAX(1, n / 3); break;
    default: name = "Giant Moose", n = PLANE_POP_MAX(1, n / 5); break;
    }
    t->populate(x, y, name, n, PLAYER_INDEPENDENTS, -1);
}

// The Celestial Plane's farm pool (towns and villages too), then holdit.
static void celestialFarmPool(const PopulateTools *t, int x, int y, int n) {
    int r = t->random(4);
    if (r == 0) {
        t->populate(x, y, "Spearman", n / 2, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Archer", t->random(n) + 1, PLAYER_CELESTIAL, -1);
        celestialPriests(t, x, y);
    } else if (r == 1) {
        t->populate(x, y, "Angel", PLANE_POP_MAX(1, n / 2), PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Harbinger", (n + 6) / 8, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Arch Angel", (n + 5) / 10, PLAYER_CELESTIAL, -1);
    } else if (r == 2) {
        n /= 2;
        t->populate(x, y, "Archer", n, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Swordsman", t->random(n) + 1, PLAYER_CELESTIAL, -1);
        int angels = t->random(n) + 1 > 0 ? t->random(n) + 1 : 1;
        t->populate(x, y, "Angel", angels, PLAYER_CELESTIAL, -1);
    } else {
        t->populate(x, y, "Saint", n, PLAYER_CELESTIAL, -1);
        celestialPriests(t, x, y);
    }
    t->holdit(x, y, 1);
}

// The Celestial Plane's castle pool (Castle, Bronze Castle), then holdit.
static void celestialCastlePool(const PopulateTools *t, int x, int y, int n) {
    int r = t->random(3);
    if (r == 0) {
        t->summon(PLAYER_CELESTIAL, x, y, "c*Seraph & 1d4*Harbinger & 2d4*Angelic Warrior", 100);
        t->populate(x, y, "Harbinger", t->random(n / 8) + 1, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Arch Angel", (n + 5) / 10, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Angel", PLANE_POP_MAX(1, n / 2), PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Angelic Warrior", PLANE_POP_MAX(2, n / 4), PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Archer", n, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Swordsman", t->random(n / 2) + 1, PLAYER_CELESTIAL, -1);
    } else if (r == 1) {
        t->summon(PLAYER_CELESTIAL, x, y, "c*Seraph & 2d6*Angel & 8*Heavenly Sword", 100);
        t->populate(x, y, "Angel", t->random(n / 4) + 1, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Harbinger", (n + 4) / 8, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Arch Angel", (n + 5) / 10, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Archer", n, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Saint", t->random(n) + 1, PLAYER_CELESTIAL, -1);
    } else {
        t->summon(PLAYER_CELESTIAL, x, y, "c*Arch Angel & Harbinger & Angel & 2d6*Angelic Warrior", 100);
        t->populate(x, y, "Seraph", 1, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Harbinger", (n + 4) / 8, PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Angel", PLANE_POP_MAX(1, n / 2), PLAYER_CELESTIAL, -1);
        t->populate(x, y, "Angelic Warrior", t->random(n / 4) + 1, PLAYER_CELESTIAL, -1);
        int harbingers = t->random(n / 5) + 1 > 0 ? t->random(n / 5) + 1 : 1;
        t->populate(x, y, "Harbinger", harbingers, PLAYER_CELESTIAL, -1);
    }
    t->populate(x, y, "Heavenly Sword", 4, PLAYER_CELESTIAL, -1);
    celestialPriests(t, x, y);
    t->holdit(x, y, 1);
}

// The square populate_celestial puts an Apocalyptic Rider on, or -1, -1.
static void celestialRiderSquare(const PopulateTools *t, int *rx, int *ry) {
    *rx = *ry = -1;
    int depth = G32(RVA_MAP_DEPTH), mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H), last = G32(RVA_LAST_PLANE);
    for (int tries = 0; tries <= 100000; tries++) {
        int x = t->random(depth), y = t->random(mapH);
        if (x < 0 || y < 0 || x >= mapW || y >= mapH || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) continue;
        int terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
        if (terrain < 0) continue;
        int first = -1;
        for (int k = 0; k <= last && first < 0; k++)
            if (x >= planeField(k, PLANE_X) && x < planeField(k, PLANE_X) + planeField(k, PLANE_W) && y >= planeField(k, PLANE_Y) &&
                y < planeField(k, PLANE_Y) + planeField(k, PLANE_H))
                first = k;
        if (first == 1 && terrain == 0x149) {
            *rx = x, *ry = y;
            return;
        }
    }
}

// populate_celestial, ported for Mac: every free square of record 1 by its terrain, then the four riders.
static void populateCelestialPort(void) {
    PopulateTools t = populateTools();
    int x0 = planeField(1, PLANE_X), y0 = planeField(1, PLANE_Y), w = planeField(1, PLANE_W), h = planeField(1, PLANE_H);
    int mapW = G32(RVA_MAP_W), mapH = G32(RVA_MAP_H), W = t.wilder;
    static const char *const PLAIN_GROUPS[4] = {
        "c*Arch Angel & 1d6*Angelic Warrior & 1d6*Angel & 1d8*Heavenly Sword", "c*Harbinger & 2d6*Angel",
        "c*Arch Angel & Celestial Lion & 1d6*Angelic Warrior & 1d6*Heavenly Sword", "1d3*Celestial Lion",
    };
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            int onMap = x >= 0 && y >= 0 && x < mapW && y < mapH;
            int owner = onMap ? *(signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) : -1;
            if ((owner >= 0 && owner != PLAYER_INDEPENDENTS) || G16(RVA_SQUARE_ARMY + square * 2)) continue;
            int terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
            switch (terrain) {
            case 0:
                if (t.random(100) < W * 7) t.summon(PLAYER_CELESTIAL, x, y, PLAIN_GROUPS[t.random(4)], 100);
                break;
            case 2: case 4: case 0x73:
                if (t.random(100) < 15) celestialForestPool(&t, x, y, t.dice(W * 5, 2));
                break;
            case 5: case 6:
                if (t.random(100) < 75) celestialFarmPool(&t, x, y, t.die(W * 4));
                break;
            case 0x11:
                celestialFarmPool(&t, x, y, t.dice(W * 10, 4));
                t.populate(x, y, "Heavenly Sword", 4, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Angelic Warrior", 2, PLAYER_CELESTIAL, -1);
                break;
            case 0x12:
                celestialFarmPool(&t, x, y, t.dice(W * 6, 2));
                t.populate(x, y, "Heavenly Sword", 2, PLAYER_CELESTIAL, -1);
                break;
            case 0x16: case 0x149:
                celestialCastlePool(&t, x, y, t.dice(W * 12, 4));
                t.populate(x, y, "Heavenly Sword", 4, PLAYER_CELESTIAL, -1);
                break;
            case 0xfc: {
                GAME_FN(int (*)(const char *, int, int, int, const char *), RVA_POPULATE_COMMANDER)("Great Archon", PLAYER_CELESTIAL, x, y, "Abrasax");
                t.populate(x, y, "Seraph", 1, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Harbinger", 2, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Arch Angel", 2, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Ophan", 4, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Angelic Warrior", 13, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Angel", 13, PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Heavenly Sword", t.dice(W * 6, 4), PLAYER_CELESTIAL, -1);
                t.populate(x, y, "Saint", t.dice(W * 6, 4), PLAYER_CELESTIAL, -1);
                int lions = t.die(4);
                lions += t.die(4);
                t.populate(x, y, "Celestial Lion", lions, PLAYER_CELESTIAL, -1);
                t.holdit(x, y, 1);
                break;
            }
            default: break;
            }
        }
    static const char *const RIDERS[4] = {"c*0:Apocalyptic Rider", "c*1:Apocalyptic Rider", "c*2:Apocalyptic Rider", "c*3:Apocalyptic Rider"};
    for (int i = 0; i < 4; i++) {
        int rx, ry;
        celestialRiderSquare(&t, &rx, &ry);
        t.summon(PLAYER_CELESTIAL, rx, ry, RIDERS[i], 100);
    }
}
#endif

// Each vanilla plane's populate function, 0 where the build has none.
// @platform-split plane_populate_inlined: Mac has no populate function for Celestial, Agartha and the Void (new_world holds copies), so those three run as CoE5SE's ports of the Windows functions
static unsigned int populateFunction(int plane) {
#ifdef __APPLE__
    static const unsigned int FUNCTIONS[PLANE_ELYSIUM2] = {
        RVA_POPULATE_ELYSIUM, 0, RVA_POPULATE_SKY, 0, RVA_POPULATE_INFERNO, RVA_POPULATE_HADES,
        RVA_POPULATE_AZTLAN, RVA_POPULATE_PRIMAL, RVA_POPULATE_ELEMENTAL, 0,
    };
#else
    static const unsigned int FUNCTIONS[PLANE_ELYSIUM2] = {
        RVA_POPULATE_ELYSIUM, RVA_POPULATE_CELESTIAL, RVA_POPULATE_SKY, RVA_POPULATE_AGARTHA, RVA_POPULATE_INFERNO, RVA_POPULATE_HADES,
        RVA_POPULATE_AZTLAN, RVA_POPULATE_PRIMAL, RVA_POPULATE_ELEMENTAL, RVA_POPULATE_VOID,
    };
#endif
    return plane >= 0 && plane < PLANE_ELYSIUM2 ? FUNCTIONS[plane] : 0;
}

// The living units of unique monsters and where they stand.
typedef struct { int unit, x, y; } UniqueSpot;
static int listLivingUniques(UniqueSpot **out) {
    int last = G32(RVA_UNIT_COUNT), n = 0;
    *out = NULL;
    if (last < 0 || last >= UNIT_TABLE_MAX) return 0;
    for (int pass = 0; pass < 2; pass++) {
        int k = 0;
        for (int u = 0; u <= last; u++) {
            if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) <= 0) continue;
            if (!monsterAbility(G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE), MONSTER_ABILITY_UNIQUE)) continue;
            if (pass) (*out)[k] = (UniqueSpot){u, G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE), G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE)};
            k++;
        }
        if (!pass) {
            if (!(n = k) || !(*out = malloc(sizeof **out * (size_t)n))) return 0;
        }
    }
    return n;
}

// Puts back each unique a population run moved; returns how many.
typedef void (*TeleportArmyFn)(int unit, int a, int b, int c, int x, int y, int flag);
static int keepUniquesInPlace(const UniqueSpot *spots, int n, const CustomPlane *p) {
    int kept = 0;
    for (int i = 0; i < n; i++) {
        int u = spots[i].unit;
        int x = G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE);
        if (x == spots[i].x && y == spots[i].y) continue;
        GAME_FN(TeleportArmyFn, RVA_TELEPORT_ARMY)(u, 0, -1, -1, spots[i].x, spots[i].y, 1);
        writeLog("plane_unique_kept", "mod=%s name=%s unit=%d monster=%d x=%d y=%d", p->mod, p->name, u,
                 G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE), spots[i].x, spots[i].y);
        kept++;
    }
    return kept;
}

// The living units on plane p.
static int countPlaneUnits(const CustomPlane *p) {
    int last = G32(RVA_UNIT_COUNT), n = 0, x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) <= 0) continue;
        int x = G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE);
        n += x >= x0 && y >= y0 && x < x0 + p->w && y < y0 + p->h;
    }
    return n;
}

// Stocks plane p with its vanilla plane's monsters.
static void populateCustomPlane(const CustomPlane *p) {
    int k = p->populate;
    if (k < 0 || k >= PLANE_ELYSIUM2) return;
    unsigned int fn = populateFunction(k);
    void (*run)(void) = fn ? GAME_FN(void (*)(void), fn) : NULL;
#ifdef __APPLE__
    if (k == 1) run = populateCelestialPort;
    if (k == 3) run = populateAgarthaPort;
    if (k == 9) run = populateVoidPort;
#endif
    if (!run) {
        writeLog("plane_populate_skipped", "mod=%s name=%s populate=%s reason=no_function", p->mod, p->name, VANILLA_PLANE_SHORT_NAMES[k]);
        return;
    }
    UniqueSpot *uniques;
    int uniqueCount = listLivingUniques(&uniques);
    int saved[4] = {planeField(k, PLANE_X), planeField(k, PLANE_Y), planeField(k, PLANE_W), planeField(k, PLANE_H)};
    setPlaneField(k, PLANE_X, planeField(p->index, PLANE_X)), setPlaneField(k, PLANE_Y, planeField(p->index, PLANE_Y));
    setPlaneField(k, PLANE_W, p->w), setPlaneField(k, PLANE_H, p->h);
    run();
    setPlaneField(k, PLANE_X, saved[0]), setPlaneField(k, PLANE_Y, saved[1]);
    setPlaneField(k, PLANE_W, saved[2]), setPlaneField(k, PLANE_H, saved[3]);
    int kept = uniques ? keepUniquesInPlace(uniques, uniqueCount, p) : 0;
    free(uniques);
    writeLog("plane_populated", "mod=%s name=%s populate=%s units=%d uniques_kept=%d", p->mod, p->name, VANILLA_PLANE_SHORT_NAMES[k],
             countPlaneUnits(p), kept);
}

// Applies plane p's terrain swap.
static void applyPlaneSwap(const CustomPlane *p) {
    if (!p->swapCount) return;
    int last = G32(RVA_LAST_TERRAIN), changed = 0, x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    for (int i = 0; i < p->swapCount; i++)
        if (p->swapTo[i] > last)
            writeLog("plane_swap_refused", "mod=%s name=%s from=%d to=%d reason=no_such_terrain last_terrain=%d", p->mod, p->name, p->swapFrom[i],
                     p->swapTo[i], last);
    for (int x = x0; x < x0 + p->w; x++)
        for (int y = y0; y < y0 + p->h; y++) {
            short terrain = *(short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
            for (int i = 0; i < p->swapCount; i++) {
                if (terrain != p->swapFrom[i] || p->swapTo[i] > last) continue;
                GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN)(x, y, p->swapTo[i]);
                changed++;
                break;
            }
        }
    writeLog("plane_swapped", "mod=%s name=%s pairs=%d squares=%d", p->mod, p->name, p->swapCount, changed);
}

// After new_world's last population: each placed plane's population and its zones' lists, then its swap.
static void populatePlaneWithZones(const CustomPlane *p);
static void populateCustomPlanes(void) {
    for (int i = 0; i < customPlaneCount; i++) {
        const CustomPlane *p = &customPlanes[i];
        if (p->skipped || p->index < 0) continue;
        populatePlaneWithZones(p);
        applyPlaneSwap(p);
    }
}

// ---- coe5.addPlane's recipe fields -----------------------------------------------------------------
// What coe5.addPlane reads past name, w, h and fill.
typedef struct {
    int recipe, populate, swapCount;
    short swapFrom[PLANE_SWAP_MAX], swapTo[PLANE_SWAP_MAX];
} PlaneBuildFields;

// Reads like (at stack index `likeAt`), populate, and swap into f; NULL, or the refusal's reason word.
static const char *readPlaneBuildFields(lua_State *L, int likeAt, int populateAt, int swapAt, PlaneBuildFields *f) {
    f->recipe = f->populate = -1, f->swapCount = 0;
    if (!lua_isnil(L, likeAt)) {
        f->recipe = vanillaPlaneNamed(luaL_checkstring(L, likeAt));
        if (f->recipe < 0) return "unknown_plane";
        if (!PLANE_HAS_RECIPE[f->recipe]) return "no_recipe";
        f->populate = f->recipe;
    }
    if (lua_type(L, populateAt) == LUA_TBOOLEAN) {
        if (lua_toboolean(L, populateAt)) luaL_typeerror(L, populateAt, "plane name or false");
        f->populate = -1;
    } else if (!lua_isnil(L, populateAt)) {
        f->populate = vanillaPlaneNamed(luaL_checkstring(L, populateAt));
        if (f->populate < 0) return "unknown_plane";
    }
    if (lua_isnil(L, swapAt)) return NULL;
    luaL_checktype(L, swapAt, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, swapAt)) {
        int good = lua_isinteger(L, -2) && lua_isinteger(L, -1);
        lua_Integer from = good ? lua_tointeger(L, -2) : -1, to = good ? lua_tointeger(L, -1) : -1;
        lua_pop(L, 1);
        if (!good || from < 0 || from > SHRT_MAX || to < 0 || to > SHRT_MAX) {
            lua_pop(L, 1);
            return "bad_swap";
        }
        if (f->swapCount >= PLANE_SWAP_MAX) {
            lua_pop(L, 1);
            return "too_many_swaps";
        }
        f->swapFrom[f->swapCount] = (short)from, f->swapTo[f->swapCount] = (short)to;
        f->swapCount++;
    }
    return NULL;
}
