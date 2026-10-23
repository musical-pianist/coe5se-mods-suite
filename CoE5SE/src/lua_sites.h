// coe5.wrap and coe5.observe: the sites scripts may wrap or observe, and how a call reaches a script.

// ---- Sites: coe5.wrap(siteName, function(next, ...) ... end[, priority]) and coe5.observe ------
// Every site a plugin or script may wrap, and every observe-only site a script may observe.
enum {
    K_CALL,
    K_TEXT,
    K_ID_TEXT,
    K_BANNER,
    K_TOOLTIP_LINE,
    K_OBSERVE,
    K_TARGET_SQUARE,
    K_VETO,
    K_SEAL_BREAK,
    K_SEAL_EVENT,
};
#define LUA_CALL_ARGS 17
#define LUA_TEXT_ARGS 6
// How many arguments Mac passes in registers.
#define LUA_REGISTER_ARGS 8
#ifdef __APPLE__
#define LUA_CALL_ARGS_EXACT LUA_REGISTER_ARGS
#else
#define LUA_CALL_ARGS_EXACT LUA_CALL_ARGS
#endif
#define RITUAL_ROW_SIGNATURE "x:iiisiiiiiiiiziiiz"

typedef struct {
    LuaScript *s;
    int ref, site;
    int priority;
    int noNextLogged;
    void *next, *entry;
} LuaWrap;
typedef struct { LuaScript *s; int refBefore, refAfter; } LuaObserver;
typedef struct {
    const char *name;
    CallSite site;
    int kind;
    const char *signature;
} LuaSite;
// What scripts added to a site, at the index of its row in luaSites.
typedef struct {
    LuaWrap **wraps;
    int count, capacity;
    LuaObserver *observers;
    int observerCount, observerCapacity, observed;
} LuaSiteState;

// A wrapped call's arguments.
typedef struct {
    u64 a[LUA_CALL_ARGS];
    float f[8];
    const char *shown;
    const char *changed;
} LuaArgs;

// One running call of a Lua wrap.
typedef struct LuaFrame { struct LuaFrame *up; LuaWrap *wrap; const LuaArgs *args; int nextCalled; u64 result; lua_Integer serial; } LuaFrame;
static LuaFrame *luaFrames;
static lua_Integer luaFrameSerial;
static int lNextCall(lua_State *L);

// The sites a script can wrap or observe, by the name coe5.wrap and coe5.observe take.
static const LuaSite luaSites[] = {
    {"MAIN_MENU_WAIT",                SITE_MAIN_MENU_WAIT,                K_CALL,    "i:ii"},
    {"MAIN_MENU_PLAY_GAME",           SITE_MAIN_MENU_PLAY_GAME,           K_CALL,    "i:i"},
    {"PLAY_GAME_NEW_WORLD",           SITE_PLAY_GAME_NEW_WORLD,           K_CALL,    "i:i"},
    {"NEW_WORLD_MAP_SCREEN",          SITE_NEW_WORLD_MAP_SCREEN,          K_CALL,    "i:"},
    {"NEW_WORLD_INIT_PARTICIPANTS",   SITE_NEW_WORLD_INIT_PARTICIPANTS,   K_CALL,    "v:i"},
    {"NEW_WORLD_CREATING_WORLD",      SITE_NEW_WORLD_CREATING_WORLD,      K_ID_TEXT, "v:is"},
    {"MAP_SETTINGS_WAIT",             SITE_MAP_SETTINGS_WAIT,             K_CALL,    "i:ii"},
    {"PARTICIPANTS_WAIT",             SITE_PARTICIPANTS_WAIT,             K_CALL,    "i:ii"},
    {"PARTICIPANTS_DIFFICULTY_WAIT",  SITE_PARTICIPANTS_DIFFICULTY_WAIT,  K_CALL,    "i:ii"},
    {"PARTICIPANTS_BUTTON_ROW",       SITE_PARTICIPANTS_BUTTON_ROW,       K_CALL,    "i:h"},
    {"GAME_MENU_SAVING_SECTION_END",  SITE_GAME_MENU_SAVING_SECTION_END,  K_CALL,    "i:"},
    {"GAME_MENU_WAIT",                SITE_GAME_MENU_WAIT,                K_CALL,    "i:ii"},
    {"RITUAL_SCREEN_ROW",             SITE_RITUAL_SCREEN_ROW,             K_CALL,    RITUAL_ROW_SIGNATURE},
    {"MAP_SCREEN_WAIT",               SITE_MAP_SCREEN_WAIT,               K_CALL,    "i:ii"},
    {"MAP_MENU_BUTTON_TEXT",          SITE_MAP_MENU_BUTTON_TEXT,          K_TEXT,    "x:"},
    {"SQUARE_INFO_SCREEN",            SITE_SQUARE_INFO_SCREEN,            K_CALL,    "x:iiii"},
    {"SQUARE_INFO_BUTTON_ROW",        SITE_SQUARE_INFO_BUTTON_ROW,        K_CALL,    "i:h"},
    {"SQUARE_INFO_WAIT",              SITE_SQUARE_INFO_WAIT,              K_CALL,    "i:ii"},
    {"LOAD_GAME_BROWSER",             SITE_LOAD_GAME_BROWSER,             K_CALL,    "i:z"},
    {"FILE_BROWSER_ROW_DATE",         SITE_FILE_BROWSER_ROW_DATE,         K_ID_TEXT, "i:is"},
    {"FILE_BROWSER_WAIT",             SITE_FILE_BROWSER_WAIT,             K_CALL,    "i:ii"},
    {"FILE_BROWSER_HOVERED",          SITE_FILE_BROWSER_HOVERED,          K_CALL,    "i:"},
    {"MOD_BANNER_DRAW",               SITE_MOD_BANNER_DRAW,               K_BANNER,  NULL},
    {"GAME_OVER_WAIT",                SITE_GAME_OVER_WAIT,                K_CALL,    "i:ii"},
    {"ARMY_WAIT",                     SITE_ARMY_WAIT,                     K_CALL,    "i:ii"},
    {"BATTLE_WAIT",                   SITE_BATTLE_WAIT,                   K_CALL,    "i:ii"},
    {"MAP_EDITOR_WAIT",               SITE_MAP_EDITOR_WAIT,               K_CALL,    "i:ii"},
    {"BATTLE_EDITOR_WAIT",            SITE_BATTLE_EDITOR_WAIT,            K_CALL,    "i:ii"},
    {"RECRUIT_WAIT",                  SITE_RECRUIT_WAIT,                  K_CALL,    "i:ii"},
    {"VIEW_ARMY_WAIT",                SITE_VIEW_ARMY_WAIT,                K_CALL,    "i:ii"},
    {"UNIT_LIST_WAIT",                SITE_UNIT_LIST_WAIT,                K_CALL,    "i:ii"},
    {"KEYBOARD_SHORTCUTS_WAIT",       SITE_KEYBOARD_SHORTCUTS_WAIT,       K_CALL,    "i:ii"},
    {"TOOLTIP_LINE_DRAW",             SITE_TOOLTIP_LINE_DRAW,             K_TOOLTIP_LINE, NULL},
    {"AI_EXPLORE",                    SITE_AI_EXPLORE,                    K_TARGET_SQUARE, "i:ii"},
    {"AI_RALLY",                      SITE_AI_RALLY,                      K_TARGET_SQUARE, "i:ii"},
    {"AI_FRIENDLY_MOVE",              SITE_AI_FRIENDLY_MOVE,              K_TARGET_SQUARE, "i:ii"},
    {"AI_CONQUER",                    SITE_AI_CONQUER,                    K_TARGET_SQUARE, "i:ii"},
    {"AI_FAR_EXPLORE",                SITE_AI_FAR_EXPLORE,                K_TARGET_SQUARE, "i:ii"},
    {"AI_RITUAL_CANDIDATE",           SITE_AI_RITUAL_CANDIDATE,           K_VETO,    "i:ii"},
    {"AI_RECRUIT",                    SITE_AI_RECRUIT,                    K_CALL,    "v:i"},
    {"AI_GATHER_TROOPS",              SITE_AI_GATHER_TROOPS,              K_CALL,    "v:i"},
    {"AI_PICK_UP_LONER",              SITE_AI_PICK_UP_LONER,              K_CALL,    "i:ii"},
    {"AI_SPLIT_ARMY",                 SITE_AI_SPLIT_ARMY,                 K_CALL,    "v:ii"},
    {"AI_MOVE_COMSTR",                SITE_AI_MOVE_COMSTR,                K_CALL,    "i:i"},
    {"CEREMONY_PERFORM",              SITE_CEREMONY_PERFORM,              K_CALL,    "v:iii"},
    {"CEREMONY_COSTS",                SITE_CEREMONY_COSTS,                K_CALL,    "v:i"},
    {"SEAL_BREAK",                    SITE_SEAL_BREAK,                    K_SEAL_BREAK, "v:iii"},
    {"SEAL_EVENT",                    SITE_SEAL_EVENT,                    K_SEAL_EVENT, "i:ii"},
    {"MAP_EDITOR",                    SITE_MAP_EDITOR,                    K_CALL,    "v:"},
    {"MAP_EDITOR_DRAW",               SITE_MAP_EDITOR_DRAW,               K_CALL,    "v:"},
    {"RITUAL_SCREEN_LOCATION_CHECK",  SITE_RITUAL_SCREEN_LOCATION_CHECK,  K_OBSERVE, "i:iiii"},
    {"RITUAL_SCREEN_CAN_CAST",        SITE_RITUAL_SCREEN_CAN_CAST,        K_OBSERVE, "i:ii"},
    {"GAME_START",                    SITE_GAME_START,                    K_OBSERVE, "v:ii"},
    {"MONTH_START",                   SITE_MONTH_START,                   K_OBSERVE, "x:is"},
    {"PLAYER_TURN_START",             SITE_PLAYER_TURN_START,             K_OBSERVE, "v:ii"},
    {"HUMAN_TURN",                    SITE_HUMAN_TURN,                    K_OBSERVE, "x:i"},
    {"AI_TURN",                       SITE_AI_TURN,                       K_OBSERVE, "v:i"},
    {"PLAYER_TURN_END",               SITE_PLAYER_TURN_END,               K_OBSERVE, "i:i"},
    {"GAME_OVER",                     SITE_GAME_OVER,                     K_OBSERVE, "v:i"},
    {"GAME_SAVE",                     SITE_GAME_SAVE,                     K_OBSERVE, "i:ss"},
    {"GAME_LOAD",                     SITE_GAME_LOAD,                     K_OBSERVE, "i:si"},
};
#define LUA_SITE_COUNT ((int)(sizeof luaSites / sizeof luaSites[0]))
static LuaSiteState luaSiteState[LUA_SITE_COUNT];

// The site's result letter and its argument letters.
static char siteResult(const LuaSite *ls) {
    if (ls->signature) return ls->signature[0];
    return 'v';
}
static const char *siteArgs(const LuaSite *ls) { return ls->signature ? strchr(ls->signature, ':') + 1 : ""; }

// Pushes one argument by its signature letter.
static void pushLuaValue(lua_State *L, char type, u64 value) {
    if (type == 'i') lua_pushinteger(L, (int)value);
    else if (type == 'h') lua_pushinteger(L, (short)value);
    else if (type == 's' || type == 'z') {
        if (value) lua_pushstring(L, (const char *)(uintptr_t)value);
        else lua_pushnil(L);
    } else if (type == 'x') lua_pushnil(L);
    else lua_pushinteger(L, (lua_Integer)value);
}

// Pushes the call's arguments for the Lua wrap, and returns how many it pushed.
static int pushLuaArgs(lua_State *L, const LuaSite *ls, const LuaArgs *a) {
    switch (ls->kind) {
    case K_TEXT:
        lua_pushstring(L, a->shown);
        return 1;
    case K_ID_TEXT:
        lua_pushinteger(L, (int)a->a[0]);
        lua_pushstring(L, a->shown);
        return 2;
    case K_BANNER:
        lua_pushstring(L, (const char *)(uintptr_t)a->a[0]);
        for (int i = 0; i < 5; i++) lua_pushnumber(L, a->f[i]);
        return 6;
    case K_TOOLTIP_LINE:
        lua_pushstring(L, (const char *)(uintptr_t)a->a[0]);
        lua_pushinteger(L, (int)a->a[1]);
        for (int i = 0; i < 4; i++) lua_pushnumber(L, a->f[i]);
        return 6;
    case K_SEAL_BREAK:
        lua_pushinteger(L, (int)a->a[0]);
        lua_pushinteger(L, (int)a->a[2]);
        return 2;
    default: {
        const char *types = siteArgs(ls);
        int n = 0;
        for (; types[n]; n++) pushLuaValue(L, types[n], a->a[n]);
        return n;
    }
    }
}

// The printf buffer of the game function a text site's call draws with.
static size_t luaTextSiteSize(CallSite site) {
    switch (site) {
    case SITE_MAP_MENU_BUTTON_TEXT: return TEXT_LINE_TEXT_SIZE;
    case SITE_FILE_BROWSER_ROW_DATE: return TEXT_BUTTON_TEXT_SIZE;
    case SITE_NEW_WORLD_CREATING_WORLD: return PROGRESS_TEXT_TEXT_SIZE;
    default: return 0;
    }
}

// Reads next()'s arguments into `a`.
static void readNextArgs(lua_State *L, const LuaSite *ls, LuaArgs *a) {
    switch (ls->kind) {
    case K_TEXT:
    case K_ID_TEXT: {
        int at = 1;
        if (ls->kind == K_ID_TEXT) a->a[0] = (u64)(int)luaL_checkinteger(L, at++);
        const char *text = luaL_checkstring(L, at);
        a->changed = strcmp(text, a->shown) ? text : NULL;
        break;
    }
    case K_BANNER:
        a->a[0] = (u64)(uintptr_t)luaL_checkstring(L, 1);
        for (int i = 0; i < 5; i++) a->f[i] = (float)luaL_checknumber(L, 2 + i);
        break;
    case K_TOOLTIP_LINE:
        a->a[0] = (u64)(uintptr_t)luaL_checkstring(L, 1);
        a->a[1] = (u64)luaL_checkinteger(L, 2);
        for (int i = 0; i < 4; i++) a->f[i] = (float)luaL_checknumber(L, 3 + i);
        break;
    case K_SEAL_BREAK:
        a->a[0] = (u64)luaL_checkinteger(L, 1);
        a->a[2] = (u64)luaL_checkinteger(L, 2);
        break;
    default: {
        const char *types = siteArgs(ls);
        for (int n = 0; types[n]; n++) {
            if (types[n] == 's') a->a[n] = (u64)(uintptr_t)luaL_checkstring(L, n + 1);
            else if (types[n] == 'z') a->a[n] = lua_isnil(L, n + 1) ? 0 : (u64)(uintptr_t)luaL_checkstring(L, n + 1);
            else a->a[n] = (u64)luaL_checkinteger(L, n + 1);
        }
    }
    }
}

// Keeps the arguments a wrap changed inside the game's own bounds.
typedef struct { int index; const char *name; lua_Integer min, max; } NextArgCheck;
static void checkNextArgs(const LuaSite *ls, LuaArgs *a, const LuaArgs *orig, const char *mod) {
    if (ls->kind == K_BANNER) {
        const char *path = (const char *)(uintptr_t)a->a[0], *own = (const char *)(uintptr_t)orig->a[0];
        if (path != own && (!path || !own || strcmp(path, own))) {
            refuseLuaCall(mod, ls->name, "path_changed", "path=%.200s", path ? path : "(none)");
        }
        a->a[0] = orig->a[0];
        return;
    }
    if (ls->kind == K_TOOLTIP_LINE) {
        const char *text = (const char *)(uintptr_t)a->a[0], *own = (const char *)(uintptr_t)orig->a[0];
        if (text != own && strcmp(text, own)) refuseLuaCall(mod, ls->name, "text_changed", "text=%.200s", text);
        if (a->a[1] != orig->a[1])
            refuseLuaCall(mod, ls->name, "index_changed", "index=%lld line=%lld", (long long)a->a[1], (long long)orig->a[1]);
        a->a[0] = orig->a[0];
        a->a[1] = orig->a[1];
        return;
    }
    if (ls->kind == K_TARGET_SQUARE) {
        if ((int)a->a[0] != (int)orig->a[0])
            refuseLuaCall(mod, ls->name, "commander_changed", "commander=%d own=%d", (int)a->a[0], (int)orig->a[0]);
        a->a[0] = orig->a[0];
        return;
    }
    if (ls->kind == K_VETO) {
        if ((int)a->a[0] != (int)orig->a[0])
            refuseLuaCall(mod, ls->name, "commander_changed", "commander=%d own=%d", (int)a->a[0], (int)orig->a[0]);
        if ((int)a->a[1] != (int)orig->a[1])
            refuseLuaCall(mod, ls->name, "ritual_changed", "ritual=%d own=%d", (int)a->a[1], (int)orig->a[1]);
        a->a[0] = orig->a[0];
        a->a[1] = orig->a[1];
        return;
    }
    if (ls->site == SITE_AI_GATHER_TROOPS || ls->site == SITE_AI_PICK_UP_LONER || ls->site == SITE_AI_SPLIT_ARMY ||
        ls->site == SITE_AI_MOVE_COMSTR) {
        if ((int)a->a[0] != (int)orig->a[0])
            refuseLuaCall(mod, ls->name, "commander_changed", "commander=%d own=%d", (int)a->a[0], (int)orig->a[0]);
        a->a[0] = orig->a[0];
        if (ls->site == SITE_AI_PICK_UP_LONER) {
            if ((int)a->a[1] != (int)orig->a[1])
                refuseLuaCall(mod, ls->name, "unit_changed", "unit=%d own=%d", (int)a->a[1], (int)orig->a[1]);
            a->a[1] = orig->a[1];
        } else if (ls->site == SITE_AI_SPLIT_ARMY) {
            long long minUnits = (long long)a->a[1];
            if (minUnits < 1 || minUnits > INT_MAX) {
                refuseLuaCall(mod, ls->name, "out_of_range", "arg=minUnits value=%lld max=%d", minUnits, INT_MAX);
                a->a[1] = orig->a[1];
            }
        }
        return;
    }
    if (ls->kind == K_SEAL_BREAK || ls->kind == K_SEAL_EVENT || ls->site == SITE_CEREMONY_PERFORM || ls->site == SITE_CEREMONY_COSTS) {
        typedef struct { const char *name, *reason; } KeptArg;
        static const KeptArg PERFORM[3] = {{"ceremony", "ceremony_changed"}, {"commander", "commander_changed"}, {"target", "target_changed"}},
                             COSTS[3] = {{"commander", "commander_changed"}, {NULL, NULL}, {NULL, NULL}},
                             SEAL[3] = {{"slot", "slot_changed"}, {NULL, NULL}, {"seal", "seal_changed"}},
                             EVENT[3] = {{"slot", "slot_changed"}, {"seal", "seal_changed"}, {NULL, NULL}};
        const KeptArg *checked = ls->kind == K_SEAL_BREAK            ? SEAL
                                 : ls->kind == K_SEAL_EVENT          ? EVENT
                                 : ls->site == SITE_CEREMONY_COSTS   ? COSTS
                                                                     : PERFORM;
        for (int i = 0; i < 3; i++) {
            if (!checked[i].name) continue;
            if ((int)a->a[i] != (int)orig->a[i])
                refuseLuaCall(mod, ls->name, checked[i].reason, "%s=%d own=%d", checked[i].name, (int)a->a[i], (int)orig->a[i]);
            a->a[i] = orig->a[i];
        }
        return;
    }
    NextArgCheck checks[5];
    int n = 0;
    if (ls->site == SITE_RITUAL_SCREEN_ROW) {
        checks[n++] = (NextArgCheck){15, "mirrorMonster", LLONG_MIN, G32(RVA_MONSTER_COUNT)};
        checks[n++] = (NextArgCheck){4, "costType1", LLONG_MIN, RITUAL_COST_TYPE_MAX};
        checks[n++] = (NextArgCheck){6, "costType2", LLONG_MIN, RITUAL_COST_TYPE_MAX};
        checks[n++] = (NextArgCheck){8, "costType3", LLONG_MIN, RITUAL_COST_TYPE_MAX};
        checks[n++] = (NextArgCheck){10, "costType4", LLONG_MIN, RITUAL_COST_TYPE_MAX};
    } else if (ls->site == SITE_SQUARE_INFO_SCREEN) {
        checks[n++] = (NextArgCheck){0, "player", 0, PLAYER_SLOTS - 1};
    }
    for (int i = 0; i < n; i++) {
        lua_Integer value = (int)a->a[checks[i].index];
        if (value >= checks[i].min && value <= checks[i].max) continue;
        refuseLuaCall(mod, ls->name, "out_of_range", "arg=%s value=%lld max=%lld", checks[i].name, (long long)value,
                      (long long)checks[i].max);
        a->a[checks[i].index] = orig->a[checks[i].index];
    }
}

// Calls `next`, the wrapper after a wrap in the chain, with the site's own signature.
typedef u64 (*LuaCallFn)(u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64, u64);
#ifdef __APPLE__
// RITUAL_ROW_SIGNATURE as C types, for Mac's packed stack arguments (LUA_REGISTER_ARGS).
typedef u64 (*LuaRitualRowFn)(int, int, int, const char *, int, int, int, int, int, int, int, int, const char *, int, int, int,
                              const char *);
// Calls `next` with the arguments in `v` at the exact C types of RITUAL_ROW_SIGNATURE.
static u64 callRitualRowTyped(void *next, const u64 *v) {
    return ((LuaRitualRowFn)next)((int)v[0], (int)v[1], (int)v[2], (const char *)(uintptr_t)v[3], (int)v[4], (int)v[5], (int)v[6],
                                  (int)v[7], (int)v[8], (int)v[9], (int)v[10], (int)v[11], (const char *)(uintptr_t)v[12],
                                  (int)v[13], (int)v[14], (int)v[15], (const char *)(uintptr_t)v[16]);
}
// Whether K_CALL site `ls` is called with its exact C types.
static int luaCallIsTyped(const LuaSite *ls) { return ls->signature && !strcmp(ls->signature, RITUAL_ROW_SIGNATURE); }
#else
static int luaCallIsTyped(const LuaSite *ls) { (void)ls; return 0; }
#endif
// The AI split's minimum.
#define SPLIT_MIN_UNITS_TRACE_IMM 4
#define SPLIT_MIN_UNITS_TEST_IMM 2
// Whether the split's two instructions at `trace` and `test` hold one minimum.
static __attribute__((unused)) int splitMinUnitsCodeOk(const unsigned char *trace, const unsigned char *test) {
    return !memcmp(trace + SPLIT_MIN_UNITS_TRACE_IMM, test + SPLIT_MIN_UNITS_TEST_IMM, 4);
}
#ifdef __APPLE__
static int splitMinUnits(void) { return G32(0); }
static void setSplitMinUnits(int value) { G32(0) = value; }
#else
static int splitMinUnits(void) { return *(const int *)(GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST) + SPLIT_MIN_UNITS_TEST_IMM); }
// Writes both instructions' imm32.
static void setSplitMinUnits(int value) {
    unsigned char *trace = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TRACE), *test = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST);
    int before;
    memcpy(&before, trace + SPLIT_MIN_UNITS_TRACE_IMM, 4);
    int ok = writeGameCode(trace + SPLIT_MIN_UNITS_TRACE_IMM, &value, 4);
    if (ok && !(ok = writeGameCode(test + SPLIT_MIN_UNITS_TEST_IMM, &value, 4))) writeGameCode(trace + SPLIT_MIN_UNITS_TRACE_IMM, &before, 4);
    if (!ok) {
        static int logged;
        if (!logged++) writeLog("ai_split_min_units_written", "ok=0 reason=code_unwritable address=%p", (void *)(trace < test ? trace : test));
    }
}
// prepareSiteCode's step for AI_SPLIT_ARMY.
static int checkSplitMinUnitsCode(void) {
    const unsigned char *trace = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TRACE), *test = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST);
    if (gameCodeMatches && splitMinUnitsCodeOk(trace, test)) return 1;
    writeLog("ai_split_min_units_check", "ok=0 reason=%s trace=%p test=%p", gameCodeMatches ? "two_minimums" : "game_code_differs",
             (void *)trace, (void *)test);
    return 0;
}
#endif
// AI_SPLIT_ARMY's next.
static u64 callSplitArmyNext(void *next, const u64 *v) {
    int own = splitMinUnits();
    if ((int)v[1] != own) setSplitMinUnits((int)v[1]);
    u64 r = ((LuaCallFn)next)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15],
                              v[16]);
    if (splitMinUnits() != own) setSplitMinUnits(own);
    return r;
}
// The Seven Seals broken: Windows only so far.
#define SEALS_BROKEN_RVA WINDOWS_ONLY_RVA(RVA_SEALS_BROKEN)
// SEAL_BREAK's next.
static u64 sealBreakNext(void *next, const u64 *v) {
    G32(SEALS_BROKEN_RVA) = (int)v[2];
    return ((LuaCallFn)next)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15], v[16]);
}
typedef u64 (*LuaTextFn)(const char *, ...);
typedef u64 (*LuaIdTextFn)(int, const char *, ...);
#ifdef __APPLE__
typedef void (*LuaBannerFn)(float, float, float, float, float, const char *, int, int);
typedef void (*LuaTooltipLineFn)(const char *, float, float, float, int, float, float, float, float, const char *);
#else
typedef void (*LuaBannerFn)(const char *, int, int, float, float, float, float, float);
typedef void (*LuaTooltipLineFn)(float, float, float, const char *, float, float, float, float);
#endif
static u64 callLuaNext(const LuaSite *ls, void *next, const LuaArgs *a) {
    const u64 *v = a->a;
    switch (ls->kind) {
    case K_TEXT:
        if (a->changed) return ((LuaTextFn)next)("%s", a->changed);
        return ((LuaTextFn)next)((const char *)(uintptr_t)v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
    case K_ID_TEXT:
        if (a->changed) return ((LuaIdTextFn)next)((int)v[0], "%s", a->changed);
        return ((LuaIdTextFn)next)((int)v[0], (const char *)(uintptr_t)v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
    case K_BANNER:
#ifdef __APPLE__
        ((LuaBannerFn)next)(a->f[0], a->f[1], a->f[2], a->f[3], a->f[4], (const char *)(uintptr_t)v[0], (int)v[1], (int)v[2]);
        return 0;
#else
        ((LuaBannerFn)next)((const char *)(uintptr_t)v[0], (int)v[1], (int)v[2], a->f[0], a->f[1], a->f[2], a->f[3], a->f[4]);
        return 0;
#endif
    case K_TOOLTIP_LINE:
#ifdef __APPLE__
        ((LuaTooltipLineFn)next)((const char *)(uintptr_t)v[2], a->f[4], a->f[5], a->f[6], (int)v[3], a->f[0], a->f[1], a->f[2],
                                 a->f[3], (const char *)(uintptr_t)v[0]);
#else
        ((LuaTooltipLineFn)next)(a->f[4], a->f[5], a->f[6], (const char *)(uintptr_t)v[0], a->f[0], a->f[1], a->f[2], a->f[3]);
#endif
        return 0;
    case K_SEAL_BREAK: return sealBreakNext(next, v);
    default:
#ifdef __APPLE__
        if (luaCallIsTyped(ls)) return callRitualRowTyped(next, v);
#endif
        if (ls->site == SITE_AI_SPLIT_ARMY) return callSplitArmyNext(next, v);
        return ((LuaCallFn)next)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10], v[11], v[12], v[13],
                                 v[14], v[15], v[16]);
    }
}

// Pushes what next() returns to the script.
static int pushLuaResult(lua_State *L, const LuaSite *ls, const LuaArgs *a, u64 result) {
    if (ls->kind == K_VETO) {
        lua_pushboolean(L, (int)result == 0);
        return 1;
    }
    if (ls->kind == K_SEAL_EVENT) return 0;
    char type = siteResult(ls);
    if (type == 'v') return 0;
    pushLuaValue(L, type, result);
    if (ls->kind != K_TARGET_SQUARE) return 1;
    lua_pushinteger(L, *(const int *)(uintptr_t)a->a[2]);
    lua_pushinteger(L, *(const int *)(uintptr_t)a->a[3]);
    return 3;
}

// Why the square and score a K_TARGET_SQUARE wrap returned can't go to the game, or NULL.
static const char *targetSquareRefusal(lua_Integer score, lua_Integer x, lua_Integer y, int mapDepth, int mapH, int plane) {
    if (score < INT_MIN || score > INT_MAX) return "out_of_range";
    if (x == AI_NO_SQUARE && y == AI_NO_SQUARE) return score > 0 ? "no_square" : NULL;
    if (x < 0 || y < 0 || x >= mapDepth || y >= mapH || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return "off_map";
    if (plane < 0) return "no_plane";
    return NULL;
}

// Reads the value at `index` as a whole number.
static int luaWholeNumber(lua_State *L, int index, lua_Integer *out) {
    int isNumber = 0;
    lua_Integer value = lua_type(L, index) == LUA_TNUMBER ? lua_tointegerx(L, index, &isNumber) : 0;
    if (isNumber) *out = value;
    return isNumber;
}

// After a K_TARGET_SQUARE wrap returned: puts its square through the call's pointers and returns its score.
static u64 takeTargetSquare(lua_State *L, const LuaSite *ls, const LuaArgs *a, const LuaFrame *f, const char *mod) {
    int *outX = (int *)(uintptr_t)a->a[2], *outY = (int *)(uintptr_t)a->a[3];
    if (!f->nextCalled) *outX = *outY = AI_NO_SQUARE;
    lua_Integer ownScore = f->nextCalled ? (int)f->result : 0;
    if (lua_isnil(L, -3) && lua_isnil(L, -2) && lua_isnil(L, -1)) return (u64)ownScore;
    lua_Integer score = 0, x = *outX, y = *outY;
    const char *reason = NULL;
    if (!luaWholeNumber(L, -3, &score)) reason = "not_a_score";
    else if (!lua_isnil(L, -2) || !lua_isnil(L, -1)) {
        if (!luaWholeNumber(L, -2, &x) || !luaWholeNumber(L, -1, &y)) reason = "not_a_square";
    }
    int plane = -1;
    if (!reason && x >= 0 && y >= 0 && x < G32(RVA_MAP_DEPTH) && y < G32(RVA_MAP_H) && x < MAP_COLUMNS_MAX && y < MAP_COLUMN_SQUARES)
        plane = GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)x, (int)y);
    if (!reason) reason = targetSquareRefusal(score, x, y, G32(RVA_MAP_DEPTH), G32(RVA_MAP_H), plane);
    if (reason) {
        refuseLuaCall(mod, ls->name, reason, "score=%lld x=%lld y=%lld commander=%d", (long long)score, (long long)x, (long long)y,
                      (int)a->a[0]);
        return (u64)ownScore;
    }
    *outX = (int)x;
    *outY = (int)y;
    return (u64)score;
}

// After a K_VETO wrap returned, with its result on L's stack: false vetoes, and the game gets 1 (drop).
static u64 takeVeto(lua_State *L, const LuaSite *ls, const LuaArgs *a, const LuaFrame *f, const char *mod, int *callNext) {
    *callNext = 0;
    if (lua_isboolean(L, -1) && !lua_toboolean(L, -1)) return 1;
    if (!lua_isnil(L, -1) && !lua_isboolean(L, -1))
        refuseLuaCall(mod, ls->name, "not_a_boolean", "type=%s commander=%d ritual=%d", luaL_typename(L, -1), (int)a->a[0],
                      (int)a->a[1]);
    if (f->nextCalled) return f->result;
    *callNext = 1;
    return 0;
}

#define ROUTE_COMSTR_MAX 21474836
// After an AI_MOVE_COMSTR wrap returned: the comstr the step's route takes.
static u64 takeComstr(lua_State *L, const LuaSite *ls, const LuaArgs *a, const LuaFrame *f, const char *mod, int *callNext) {
    *callNext = 0;
    if (lua_isnil(L, -1)) return f->result;
    lua_Integer comstr = 0;
    const char *reason = !luaWholeNumber(L, -1, &comstr) ? "not_a_number"
                         : comstr < -1 || comstr > ROUTE_COMSTR_MAX ? "out_of_range"
                                                                     : NULL;
    if (!reason) return (u64)comstr;
    refuseLuaCall(mod, ls->name, reason, "type=%s comstr=%lld max=%d commander=%d", luaL_typename(L, -1), (long long)comstr,
                  ROUTE_COMSTR_MAX, (int)a->a[0]);
    if (f->nextCalled) return f->result;
    *callNext = 1;
    return 0;
}

// callLuaProtected's push for a wrap: its function, its next(), and the call's arguments.
static int pushLuaWrapCall(lua_State *L, void *frame) {
    LuaFrame *f = frame;
    luaL_checkstack(L, 2 + LUA_CALL_ARGS, "a wrap's arguments");
    lua_rawgeti(L, LUA_REGISTRYINDEX, f->wrap->ref);
    lua_pushlightuserdata(L, f);
    lua_pushinteger(L, f->serial);
    lua_pushcclosure(L, lNextCall, 2);
    return 1 + pushLuaArgs(L, &luaSites[f->wrap->site], f->args);
}

// Runs one wrap.
static u64 runLuaWrap(LuaWrap *w, const LuaArgs *a) {
    const LuaSite *ls = &luaSites[w->site];
    LuaScript *s = w->s;
    if (!s || s->dead) return callLuaNext(ls, w->next, a);
    LuaFrame f = {luaFrames, w, a, 0, 0, ++luaFrameSerial};
    luaFrames = &f;
    int results = ls->kind == K_TARGET_SQUARE ? 3 : 1;
    int ok = callLuaProtected(s, pushLuaWrapCall, &f, results);
    u64 result = 0;
    int vetoNext = 0;
    if (ok) {
        if (ls->kind == K_TARGET_SQUARE) result = takeTargetSquare(s->L, ls, a, &f, s->mod);
        else if (ls->kind == K_VETO) result = takeVeto(s->L, ls, a, &f, s->mod, &vetoNext);
        else if (ls->site == SITE_AI_MOVE_COMSTR) result = takeComstr(s->L, ls, a, &f, s->mod, &vetoNext);
        else if (ls->kind == K_SEAL_EVENT) result = f.nextCalled ? f.result : 0;
        else result = lua_isinteger(s->L, -1) ? (u64)lua_tointeger(s->L, -1) : f.result;
        lua_pop(s->L, results);
        closeDeadLuaScript(s);
        if (!vetoNext && (!f.nextCalled || (siteResult(ls) == 'i' ? (int)result != (int)f.result : result != f.result)))
            siteResultMod[luaSites[w->site].site] = s->mod;
        if (!f.nextCalled && !vetoNext && !w->noNextLogged) {
            w->noNextLogged = 1;
            writeLog("lua_wrap_no_next", "mod=%s site=%s priority=%d result=%lld", s->mod, ls->name, w->priority, (long long)result);
        }
    }
    luaFrames = f.up;
    if (!ok) result = f.nextCalled ? f.result : callLuaNext(ls, w->next, a);
    else if (vetoNext) result = callLuaNext(ls, w->next, a);
    return result;
}

// next(...) inside a wrap.
static int lNextCall(lua_State *L) {
    LuaFrame *f = lua_touserdata(L, lua_upvalueindex(1));
    lua_Integer serial = lua_tointeger(L, lua_upvalueindex(2));
    LuaFrame *on = luaFrames;
    while (on && on != f) on = on->up;
    if (!on || on->serial != serial || on->wrap->s != luaScriptOf(L))
        return luaL_error(L, "next() called after its wrap returned");
    checkLuaMainThread(L, "next()");
    const LuaSite *ls = &luaSites[f->wrap->site];
    LuaArgs a = *f->args;
    a.changed = NULL;
    readNextArgs(L, ls, &a);
    checkNextArgs(ls, &a, f->args, f->wrap->s ? f->wrap->s->mod : "?");
    size_t size = luaTextSiteSize(ls->site);
    if (a.changed && size && strlen(a.changed) >= size) {
        noteApiCut((void *)lNextCall, "next", "text", a.changed, (int)size - 1);
        lua_pushlstring(L, a.changed, size - 1);
        a.changed = lua_tostring(L, -1);
    }
    f->nextCalled = 1;
    unsigned long frames = gameFrames;
    luaCodeRunning--;
    f->result = callLuaNext(ls, f->wrap->next, &a);
    luaCodeRunning++;
    if (gameFrames != frames) resetLuaSteps(L);
    return pushLuaResult(L, ls, &a, f->result);
}

// Every script wrap by its gate.
static struct { void **gates; LuaWrap **wraps; size_t capacity, count; } luaWrapIndex;
static size_t luaGateSlot(const void *gate, size_t capacity) {
    return (size_t)(((uintptr_t)gate >> 4) * (uintptr_t)0x9E3779B97F4A7C15ull) & (capacity - 1);
}
static void putLuaWrapIndex(void *gate, LuaWrap *w) {
    size_t i = luaGateSlot(gate, luaWrapIndex.capacity);
    while (luaWrapIndex.gates[i]) i = (i + 1) & (luaWrapIndex.capacity - 1);
    luaWrapIndex.gates[i] = gate;
    luaWrapIndex.wraps[i] = w;
    luaWrapIndex.count++;
}
// Returns 0 when there was no memory to grow the index.
static int indexLuaWrap(LuaWrap *w) {
    if (2 * (luaWrapIndex.count + 1) > luaWrapIndex.capacity) {
        size_t capacity = luaWrapIndex.capacity ? 2 * luaWrapIndex.capacity : 64;
        void **gates = calloc(capacity, sizeof *gates);
        LuaWrap **wraps = calloc(capacity, sizeof *wraps);
        if (!gates || !wraps) {
            free(gates);
            free(wraps);
            return 0;
        }
        void **oldGates = luaWrapIndex.gates;
        LuaWrap **oldWraps = luaWrapIndex.wraps;
        size_t oldCapacity = luaWrapIndex.capacity;
        luaWrapIndex.gates = gates, luaWrapIndex.wraps = wraps, luaWrapIndex.capacity = capacity, luaWrapIndex.count = 0;
        for (size_t i = 0; i < oldCapacity; i++)
            if (oldGates[i]) putLuaWrapIndex(oldGates[i], oldWraps[i]);
        free(oldGates);
        free(oldWraps);
    }
    putLuaWrapIndex(w->entry, w);
    return 1;
}
static LuaWrap *luaWrapOfGate(void *gate) {
    if (luaWrapIndex.capacity)
        for (size_t i = luaGateSlot(gate, luaWrapIndex.capacity); luaWrapIndex.gates[i]; i = (i + 1) & (luaWrapIndex.capacity - 1))
            if (luaWrapIndex.gates[i] == gate) return luaWrapIndex.wraps[i];
    for (int i = 0; i < LUA_SITE_COUNT; i++)
        for (int j = 0; j < luaSiteState[i].count; j++)
            if (luaSiteState[i].wraps[j]->entry == gate) return luaSiteState[i].wraps[j];
    return NULL;
}

// Runs the wrap whose gate is `gate`.
static u64 enterLuaWrap(void *gate, const LuaArgs *a) {
    LuaWrap *w = luaWrapOfGate(gate);
    if (!w) {
        writeLog("lua_wrap_not_found", "gate=%p", gate);
        return 0;
    }
    CallSite site = luaSites[w->site].site;
    LuaArgs split;
    if (site == SITE_AI_SPLIT_ARMY) {
        split = *a;
        split.a[1] = (u64)(long long)splitMinUnits();
        a = &split;
    }
    if (luaSites[w->site].kind == K_SEAL_BREAK) G32(SEALS_BROKEN_RVA) = (int)a->a[2] - 1;
    luaSiteDepth[site]++;
    u64 r = runLuaWrap(w, a);
    luaSiteDepth[site]--;
    return r;
}

// The C functions each kind's wraps are entered through.
static u64 wCall(u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7, u64 a8, u64 a9, u64 a10, u64 a11, u64 a12,
                 u64 a13, u64 a14, u64 a15, u64 a16, u64 a17) {
    void *gate = enteredGate();
    LuaArgs a = {{a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17}, {0}, NULL, NULL};
    return enterLuaWrap(gate, &a);
}
#ifdef __APPLE__
// RITUAL_SCREEN_ROW's entry on Mac, with RITUAL_ROW_SIGNATURE's exact types.
static u64 wCallRitualRow(int a1, int a2, int a3, const char *a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11,
                          int a12, const char *a13, int a14, int a15, int a16, const char *a17) {
    void *gate = enteredGate();
    LuaArgs a = {{(u64)a1, (u64)a2, (u64)a3, (u64)(uintptr_t)a4, (u64)a5, (u64)a6, (u64)a7, (u64)a8, (u64)a9, (u64)a10, (u64)a11,
                  (u64)a12, (u64)(uintptr_t)a13, (u64)a14, (u64)a15, (u64)a16, (u64)(uintptr_t)a17},
                 {0}, NULL, NULL};
    return enterLuaWrap(gate, &a);
}
#endif

// Formats a text site's text as the game would draw it.
static void formatLuaText(LuaArgs *a, int first, const char *fmt, va_list ap, char *shown, size_t size) {
    static char translated[TRANSLATED_SIZE];
    va_list raw;
    va_copy(raw, ap);
    vsnprintf(shown, size, translateText(fmt ? fmt : "", translated, sizeof translated), ap);
    for (int i = 0; i < LUA_TEXT_ARGS; i++) a->a[first + i] = va_arg(raw, u64);
    va_end(raw);
    a->shown = shown;
}
static u64 wText(const char *fmt, ...) {
    void *gate = enteredGate();
    char shown[TRANSLATED_SIZE];
    LuaArgs a = {{(u64)(uintptr_t)fmt}, {0}, NULL, NULL};
    va_list ap;
    va_start(ap, fmt);
    formatLuaText(&a, 1, fmt, ap, shown, sizeof shown);
    va_end(ap);
    return enterLuaWrap(gate, &a);
}
static u64 wIdText(int id, const char *fmt, ...) {
    void *gate = enteredGate();
    char shown[TRANSLATED_SIZE];
    LuaArgs a = {{(u64)id, (u64)(uintptr_t)fmt}, {0}, NULL, NULL};
    va_list ap;
    va_start(ap, fmt);
    formatLuaText(&a, 2, fmt, ap, shown, sizeof shown);
    va_end(ap);
    return enterLuaWrap(gate, &a);
}
#ifdef __APPLE__
static void wBanner(float x, float y, float w, float h, float alpha, const char *path, int i1, int i2) {
    void *gate = enteredGate();
    LuaArgs a = {{(u64)(uintptr_t)path, (u64)i1, (u64)i2}, {x, y, w, h, alpha}, NULL, NULL};
    enterLuaWrap(gate, &a);
}
#else
static void wBanner(const char *path, int i1, int i2, float x, float y, float w, float h, float alpha) {
    void *gate = enteredGate();
    LuaArgs a = {{(u64)(uintptr_t)path, (u64)i1, (u64)i2}, {x, y, w, h, alpha}, NULL, NULL};
    enterLuaWrap(gate, &a);
}
#endif

// A tooltip line's index in the box.
static int tooltipLineIndex(const char *text) {
    const char *lines = (const char *)(gameBase + RVA_TOOLTIP_LINES);
    if (!text || text < lines) return -1;
    size_t at = (size_t)(text - lines);
    if (at % TOOLTIP_LINE_SIZE || at / TOOLTIP_LINE_SIZE > INT_MAX) return -1;
    return (int)(at / TOOLTIP_LINE_SIZE);
}
#ifdef __APPLE__
// @platform-split tooltip_line_draw: Mac's call is RVA_RENDER_TEXT, with another signature
static void wTooltipLine(const char *font, float x, float y, float size, int flags, float r, float g, float b, float alpha,
                         const char *text) {
    void *gate = enteredGate();
    LuaArgs a = {{(u64)(uintptr_t)text, (u64)(lua_Integer)tooltipLineIndex(text), (u64)(uintptr_t)font, (u64)flags},
                 {r, g, b, alpha, x, y, size}, NULL, NULL};
    enterLuaWrap(gate, &a);
}
#else
static void wTooltipLine(float x, float y, float size, const char *text, float r, float g, float b, float alpha) {
    void *gate = enteredGate();
    LuaArgs a = {{(u64)(uintptr_t)text, (u64)(lua_Integer)tooltipLineIndex(text)}, {r, g, b, alpha, x, y, size}, NULL, NULL};
    enterLuaWrap(gate, &a);
}
#endif

static void *luaWrapperFor(const LuaSite *ls) {
    switch (ls->kind) {
    case K_TEXT: return (void *)wText;
    case K_ID_TEXT: return (void *)wIdText;
    case K_BANNER: return (void *)wBanner;
    case K_TOOLTIP_LINE: return (void *)wTooltipLine;
    default:
#ifdef __APPLE__
        if (luaCallIsTyped(ls)) return (void *)wCallRitualRow;
#endif
        return (void *)wCall;
    }
}

// Finds the row for argument 1 of coe5.wrap or coe5.observe.
static const LuaSite *luaSiteNamed(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    for (int i = 0; i < LUA_SITE_COUNT; i++)
        if (!strcmp(luaSites[i].name, name)) return &luaSites[i];
    luaL_error(L, "no site named '%s' that a script can wrap or observe", name);
    return NULL;
}

// coe5.wrap(siteName, fn[, priority]).
static int lWrap(lua_State *L) {
    const LuaSite *ls = luaSiteNamed(L);
    LuaSiteState *st = &luaSiteState[ls - luaSites];
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int hasPriority = !lua_isnoneornil(L, 3);
    lua_Integer priority = hasPriority ? luaL_checkinteger(L, 3) : WRAP_PRIORITY_DEFAULT;
    luaL_argcheck(L, priority >= INT_MIN && priority <= INT_MAX, 3, "priority out of range");
    if (ls->kind == K_OBSERVE) return luaL_error(L, "'%s' is observe-only: use coe5.observe", ls->name);
    checkLuaMainThread(L, "coe5.wrap");
    LuaScript *s = luaScriptOf(L);
    if (!s) return luaL_error(L, "not called from a mod's script");
    if (refuseFromDeadLuaScript(L, "coe5.wrap")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (ls->kind == K_CALL && (int)strlen(siteArgs(ls)) > LUA_CALL_ARGS_EXACT && !luaCallIsTyped(ls)) {
        refuseLuaCall(s ? s->mod : "?", "coe5.wrap", "not_on_this_platform", "site=%s arguments=%d", ls->name,
                      (int)strlen(siteArgs(ls)));
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaWrap *w = calloc(1, sizeof *w);
    if (!w || !growList((void **)&st->wraps, &st->capacity, st->count + 1, sizeof w)) {
        free(w);
        refuseLuaCall(s ? s->mod : "?", "coe5.wrap", "out_of_memory", "site=%s", ls->name);
        lua_pushboolean(L, 0);
        return 1;
    }
    w->s = s;
    w->site = (int)(ls - luaSites);
    w->priority = (int)priority;
    lua_pushvalue(L, 2);
    w->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    st->wraps[st->count++] = w;
    void *wrapper = luaWrapperFor(ls);
    LuaScript *outer = luaRunning;
    luaRunning = s;
    int ok = hasPriority ? luaApi->wrapWithPriority(ls->site, wrapper, &w->next, (int)priority)
                         : luaApi->wrap(ls->site, wrapper, &w->next);
    luaRunning = outer;
    if (!ok) {
        st->count--;
        luaL_unref(L, LUA_REGISTRYINDEX, w->ref);
        free(w);
        lua_pushboolean(L, 0);
        return 1;
    }
    w->entry = wrapEntryFor(ls->site, &w->next);
    if (w->entry) indexLuaWrap(w);
    writeLog("lua_wrap_added", "mod=%s site=%s priority=%d", s ? s->mod : "?", ls->name, (int)priority);
    lua_pushboolean(L, 1);
    return 1;
}

// ---- Observers: coe5.observe(siteName, before, after) -----------------------------------------
// coe5.observe: CoE5SE's one observer per observe-only site calls every script's functions.
typedef struct { int ref; const LuaSite *ls; const CallInfo *call; int after; } LuaObserverCall;
// callLuaProtected's push for an observer: its function, the result for `after`, and the arguments.
static int pushLuaObserverCall(lua_State *L, void *ctx) {
    const LuaObserverCall *c = ctx;
    luaL_checkstack(L, 8, "an observer's arguments");
    lua_rawgeti(L, LUA_REGISTRYINDEX, c->ref);
    int n = 0;
    if (c->after) {
        if (!pushLuaResult(L, c->ls, NULL, c->call->result)) lua_pushnil(L);
        n++;
    }
    const char *types = siteArgs(c->ls);
    for (int k = 0; types[k] && k < 6; k++, n++) pushLuaValue(L, types[k], c->call->args[k]);
    return n;
}
// The call each observe-only site's script observers are running for now.
static const CallInfo *luaObserving[SITE_COUNT];
static int luaObservingAfter[SITE_COUNT];
static void runLuaObservers(CallSite site, const CallInfo *call, int after) {
    if (isNetworkGame()) return;
    const LuaSite *ls = NULL;
    for (int i = 0; i < LUA_SITE_COUNT && !ls; i++)
        if (luaSites[i].site == site && luaSites[i].kind == K_OBSERVE) ls = &luaSites[i];
    if (!ls) return;
    const LuaSiteState *st = &luaSiteState[ls - luaSites];
    const CallInfo *outerCall = luaObserving[site];
    int outerAfter = luaObservingAfter[site];
    luaObserving[site] = call, luaObservingAfter[site] = after;
    for (int i = 0; i < st->observerCount; i++) {
        LuaObserver o = st->observers[i];
        LuaObserverCall c = {after ? o.refAfter : o.refBefore, ls, call, after};
        if (!o.s || o.s->dead || c.ref == LUA_NOREF) continue;
        callLuaProtected(o.s, pushLuaObserverCall, &c, 0);
    }
    luaObserving[site] = outerCall, luaObservingAfter[site] = outerAfter;
}

// One `before` and one `after` function for each observe-only site.
#define LUA_OBSERVER_FNS(s) \
    static void luaBefore_##s(const CallInfo *call) { runLuaObservers(s, call, 0); } \
    static void luaAfter_##s(const CallInfo *call) { runLuaObservers(s, call, 1); }
OBSERVE_ONLY_SITES(LUA_OBSERVER_FNS)
#undef LUA_OBSERVER_FNS

// Returns 1 when `fn` is one of the functions above.
static int isScriptObserver(const void *fn) {
#define LUA_OBSERVER_IS(s) if (fn == (const void *)luaBefore_##s || fn == (const void *)luaAfter_##s) return 1;
    OBSERVE_ONLY_SITES(LUA_OBSERVER_IS)
#undef LUA_OBSERVER_IS
    return 0;
}

// Puts CoE5SE's observer on the site.
static int observeForLua(CallSite site) {
#define LUA_OBSERVER_CASE(s) case s: return luaApi->observe(s, luaBefore_##s, luaAfter_##s);
    switch (site) {
    OBSERVE_ONLY_SITES(LUA_OBSERVER_CASE)
    default:
        return luaApi->observe(site, NULL, NULL);
    }
#undef LUA_OBSERVER_CASE
}

// coe5.observe(siteName, before, after): either function may be nil, not both.
static int lObserve(lua_State *L) {
    const LuaSite *ls = luaSiteNamed(L);
    LuaSiteState *st = &luaSiteState[ls - luaSites];
    if (ls->kind != K_OBSERVE) return luaL_error(L, "'%s' isn't observe-only: use coe5.wrap", ls->name);
    if (!lua_isnoneornil(L, 2)) luaL_checktype(L, 2, LUA_TFUNCTION);
    if (!lua_isnoneornil(L, 3)) luaL_checktype(L, 3, LUA_TFUNCTION);
    if (lua_isnoneornil(L, 2) && lua_isnoneornil(L, 3)) return luaL_error(L, "coe5.observe needs a before or an after function");
    if (refuseFromDeadLuaScript(L, "coe5.observe")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaScript *s = luaScriptOf(L);
    LuaScript *outer = luaRunning;
    luaRunning = s;
    if (!st->observed) st->observed = observeForLua(ls->site);
    luaRunning = outer;
    if (!st->observed) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (!growList((void **)&st->observers, &st->observerCapacity, st->observerCount + 1, sizeof st->observers[0])) {
        refuseLuaCall(s ? s->mod : "?", "coe5.observe", "out_of_memory", "site=%s", ls->name);
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaObserver *o = &st->observers[st->observerCount++];
    o->s = s;
    o->refBefore = o->refAfter = LUA_NOREF;
    if (!lua_isnoneornil(L, 2)) {
        lua_pushvalue(L, 2);
        o->refBefore = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    if (!lua_isnoneornil(L, 3)) {
        lua_pushvalue(L, 3);
        o->refAfter = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    writeLog("lua_observer_added", "mod=%s site=%s", s ? s->mod : "?", ls->name);
    lua_pushboolean(L, 1);
    return 1;
}

// coe5.setSetupAutomated(on).
static int lSetSetupAutomated(lua_State *L) {
    int on = lua_toboolean(L, 1);
    luaApi->setSetupAutomated(on);
    luaSetupAutomatedBy = on ? luaScriptOf(L) : NULL;
    return 0;
}
