// RVA_SQUARE_STRENGTH answered from lists of each square's living units, while no unit can change.
#pragma once

#define STRENGTH_CHECK_EVERY 256

typedef int (*SquareStrengthFn)(int, int, int);
typedef void (*StrengthMemoryFn)(int);
typedef uint64_t (*StrengthProgressFn)(uint64_t, uint64_t);
typedef uint64_t (*WaytogoFn)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                              uint64_t, uint64_t);

static SquareStrengthFn strengthGame;

static struct {
    unsigned epoch;
    unsigned firstEpoch;
    int *head, *next;
    size_t headCount, nextCount;
    int depth;
    int off;
    int sinceCheck;
    unsigned long long builds, indexed, game, checked;
} strengthIndex;

// The game's square strength, counted.
static int strengthAskGame(int team, int x, int y) {
    strengthIndex.game++;
    return strengthGame(team, x, y);
}

// Builds the lists from the unit table; 0 when the table or the map is outside what they hold.
static int strengthIndexBuild(void) {
    int columns = G32(RVA_MAP_DEPTH), last = G32(RVA_UNIT_COUNT);
    if (columns <= 0 || columns > MAP_COLUMNS_MAX || last >= UNIT_TABLE_MAX) return 0;   // No log: the game answers
    size_t squares = (size_t)columns * MAP_COLUMN_SQUARES, units = (size_t)(last < 0 ? 0 : last + 1);
    if (squares > strengthIndex.headCount) {
        int *p = realloc(strengthIndex.head, squares * sizeof *p);
        if (!p) return 0;   // No log: the game answers
        strengthIndex.head = p, strengthIndex.headCount = squares;
    }
    if (units > strengthIndex.nextCount) {
        int *p = realloc(strengthIndex.next, units * sizeof *p);
        if (!p) return 0;   // No log: the game answers
        strengthIndex.next = p, strengthIndex.nextCount = units;
    }
    memset(strengthIndex.head, 0xff, squares * sizeof *strengthIndex.head);
    for (int u = last; u >= 0; u--) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0) continue;
        int x = G32(RVA_UNIT_X + at), y = G32(RVA_UNIT_X + 4 + at);
        if (x < 0 || y < 0 || x >= columns || y >= MAP_COLUMN_SQUARES) continue;
        size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
        strengthIndex.next[u] = strengthIndex.head[square];
        strengthIndex.head[square] = u;
    }
    strengthIndex.builds++;
    return 1;
}

// RVA_SQUARE_STRENGTH(team, x, y) from the square's list.
static int strengthFromLists(int team, int x, int y) {
    long long sum = 0;
    int count = 0, ranged = 0;
    for (int u = strengthIndex.head[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y]; u >= 0; u = strengthIndex.next[u]) {
        if (team != -1 && !garrisonSameTeam(team, garrisonUnitOwner(u))) continue;
        sum += unitStrengthFor(team, u);
        count++;
        ranged += GAME_FN(int (*)(int, int, int), RVA_UNIT_HAS_RANGED)(u, 3, 1) != 0;
    }
    if (ranged > count / 10) sum = sum * (100 + 60 * garrisonFortLevel(x, y)) / 100;
    return sum < INT_MAX ? (int)sum : INT_MAX;
}

// A square strength query: from the lists when they hold this epoch, else the game's.
static int strengthQuery(int team, int x, int y) {
    if (!strengthGame) return GAME_FN(SquareStrengthFn, RVA_SQUARE_STRENGTH)(team, x, y);
    if (strengthIndex.off || isNetworkGame() || x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) ||
        y >= MAP_COLUMN_SQUARES)
        return strengthAskGame(team, x, y);
    if (strengthIndex.epoch != unitsChangeEpoch) {
        if (strengthIndex.firstEpoch != unitsChangeEpoch) {
            strengthIndex.firstEpoch = unitsChangeEpoch;
            return strengthAskGame(team, x, y);
        }
        if (!strengthIndexBuild()) return strengthAskGame(team, x, y);
        strengthIndex.epoch = unitsChangeEpoch;
    }
    strengthIndex.indexed++;
    int value = strengthFromLists(team, x, y);
    if (++strengthIndex.sinceCheck >= STRENGTH_CHECK_EVERY) {
        strengthIndex.sinceCheck = 0;
        strengthIndex.checked++;
        int game = strengthGame(team, x, y);
        if (game != value) {
            strengthIndex.off = 1;
            writeLog("strength_index_mismatch", "team=%d x=%d y=%d lists=%d game=%d", team, x, y, value, game);
            return game;
        }
    }
    return value;
}

// The window calls' square strength: from the lists inside a window, else the game's.
static int strengthWindowQuery(int team, int x, int y) {
    return strengthIndex.depth > 0 ? strengthQuery(team, x, y) : strengthAskGame(team, x, y);
}

static void strengthWindowOpen(void) {
    strengthIndex.depth++;
    unitsChangeEpoch++;
}
static void strengthWindowClose(void) {
    unitsChangeEpoch++;
    strengthIndex.depth--;
}

// RVA_AI_MOVE's call of RVA_AI_UPDATE_STRENGTH_MEMORY(player), as a window.
static void strengthMemoryWindow(int player) {
    static int loggedTurn = -1000, loggedPlayer = -1;
    if (G32(RVA_TURN) != loggedTurn || player != loggedPlayer) {
        loggedTurn = G32(RVA_TURN), loggedPlayer = player;
        writeLog("strength_index_stats", "turn=%d player=%d builds=%llu indexed=%llu game=%llu checked=%llu off=%d", loggedTurn, player,
                 strengthIndex.builds, strengthIndex.indexed, strengthIndex.game, strengthIndex.checked, strengthIndex.off);
    }
    strengthWindowOpen();
    GAME_FN(StrengthMemoryFn, RVA_AI_UPDATE_STRENGTH_MEMORY)(player);
    strengthWindowClose();
}

// RVA_AI_UPDATE_STRENGTH_MEMORY's progress text: a frame it draws ends the lists' epoch.
static uint64_t strengthMemoryProgress(uint64_t mode, uint64_t text) {
    int drawn = G32(RVA_PROGRESS_LAST_DRAW);
    uint64_t result = GAME_FN(StrengthProgressFn, RVA_PROGRESS_TEXT)(mode, text);
    if (G32(RVA_PROGRESS_LAST_DRAW) != drawn) unitsChangeEpoch++;
    return result;
}

// RVA_WAYTOGO2's calls of RVA_WAYTOGO, as a window: every argument passed on as it came.
static uint64_t strengthWaytogoWindow(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h,
                                      uint64_t i, uint64_t j, uint64_t k, uint64_t l) {
    strengthWindowOpen();
    uint64_t result = GAME_FN(WaytogoFn, RVA_WAYTOGO)(a, b, c, d, e, f, g, h, i, j, k, l);
    strengthWindowClose();
    return result;
}

// The calls strengthIndexInit points: the queries and the progress text first, the windows last.
typedef struct {
    unsigned int siteRva, targetRva;
    void *hook;
    const char *name;
} StrengthCall;

// Points one call at its hook; 1 when it did.
static int strengthPoint(const StrengthCall *c, int slot) {
#ifdef __APPLE__
    static void *trampolines[4], *hooks[4];
    if (blTarget(gameBase + c->siteRva) != gameBase + c->targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", c->name, c->siteRva + 0x100000000ULL);
        return 0;
    }
    if (hooks[slot] != c->hook) trampolines[slot] = newTrampoline(c->hook), hooks[slot] = c->hook;
    if (!trampolines[slot]) {
        writeLog("hook_skipped", "site=%s reason=no_padding", c->name);
        return 0;
    }
    uint32_t word = blTo(gameBase + c->siteRva, trampolines[slot]);
    if (writeGameCode(gameBase + c->siteRva, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=code_unwritable", c->name);
    return 0;
#else
    static unsigned char *page;
    if (!page) {
        if (!(page = allocNear(gameBase + RVA_TEXT_SECTION))) {
            writeLog("hook_skipped", "site=%s reason=no_near_page", c->name);
            return 0;
        }
        makePagesReadExecute(page, 0x1000);
    }
    return redirectCall(c->siteRva, c->targetRva, page + 0x20 * slot, c->hook, c->name, NULL);
#endif
}

// Points the square strength queries and their windows at the code above, and logs strength_index_installed.
static void strengthIndexInit(void) {
    static const StrengthCall MEMORY_QUERIES[] = {
        {RVA_AI_STRENGTH_MEMORY_SQUARE_CALL, RVA_SQUARE_STRENGTH, (void *)strengthWindowQuery, "strength_memory_square"},
        {RVA_AI_STRENGTH_MEMORY_PROGRESS_CALL, RVA_PROGRESS_TEXT, (void *)strengthMemoryProgress, "strength_memory_progress"},
    };
    static const StrengthCall MEMORY_WINDOWS[] = {
        {RVA_AI_STRENGTH_MEMORY_CALL, RVA_AI_UPDATE_STRENGTH_MEMORY, (void *)strengthMemoryWindow, "strength_memory"},
        {RVA_AI_STRENGTH_MEMORY_CALL_2, RVA_AI_UPDATE_STRENGTH_MEMORY, (void *)strengthMemoryWindow, "strength_memory_2"},
#ifdef __APPLE__
        {RVA_AI_STRENGTH_MEMORY_CALL_3, RVA_AI_UPDATE_STRENGTH_MEMORY, (void *)strengthMemoryWindow, "strength_memory_3"},
        {RVA_AI_STRENGTH_MEMORY_CALL_4, RVA_AI_UPDATE_STRENGTH_MEMORY, (void *)strengthMemoryWindow, "strength_memory_4"},
#endif
    };
    static const StrengthCall ROUTE_QUERIES[] = {
        {RVA_FAR_CHECK_SQUARE_CALL, RVA_SQUARE_STRENGTH, (void *)strengthWindowQuery, "far_check_square"},
#ifndef __APPLE__
        {RVA_FAR_CHECK_SQUARE_CALL_2, RVA_SQUARE_STRENGTH, (void *)strengthWindowQuery, "far_check_square_2"},
        {RVA_FAR_CHECK_SQUARE_CALL_3, RVA_SQUARE_STRENGTH, (void *)strengthWindowQuery, "far_check_square_3"},
#endif
    };
    static const StrengthCall ROUTE_WINDOWS[] = {
        {RVA_WAYTOGO_CALL, RVA_WAYTOGO, (void *)strengthWaytogoWindow, "strength_route"},
        {RVA_WAYTOGO_CALL_2, RVA_WAYTOGO, (void *)strengthWaytogoWindow, "strength_route_2"},
    };
    enum {
        MEMORY_QUERY_COUNT = sizeof MEMORY_QUERIES / sizeof MEMORY_QUERIES[0],
        MEMORY_WINDOW_COUNT = sizeof MEMORY_WINDOWS / sizeof MEMORY_WINDOWS[0],
        ROUTE_QUERY_COUNT = sizeof ROUTE_QUERIES / sizeof ROUTE_QUERIES[0],
        ROUTE_WINDOW_COUNT = sizeof ROUTE_WINDOWS / sizeof ROUTE_WINDOWS[0],
        COUNT = MEMORY_QUERY_COUNT + MEMORY_WINDOW_COUNT + ROUTE_QUERY_COUNT + ROUTE_WINDOW_COUNT,
    };
    if (!checkGameFunction("strength_index_check", "")) {
        writeLog("strength_index_installed", "ok=0 of %d reason=game_code_changed", COUNT);
        return;
    }
    int memory = 0, route = 0, windows = 0;
    for (int i = 0; i < MEMORY_QUERY_COUNT; i++) memory += strengthPoint(&MEMORY_QUERIES[i], i);
    for (int i = 0; i < ROUTE_QUERY_COUNT; i++) route += strengthPoint(&ROUTE_QUERIES[i], 0);
    if (memory == MEMORY_QUERY_COUNT)
        for (int i = 0; i < MEMORY_WINDOW_COUNT; i++) windows += strengthPoint(&MEMORY_WINDOWS[i], 2);
    if (route == ROUTE_QUERY_COUNT)
        for (int i = 0; i < ROUTE_WINDOW_COUNT; i++) windows += strengthPoint(&ROUTE_WINDOWS[i], 3);
    strengthGame = GAME_FN(SquareStrengthFn, RVA_SQUARE_STRENGTH);
    writeLog("strength_index_installed", "ok=%d of %d", memory + route + windows, COUNT);
}
