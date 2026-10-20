// The Lua host for mods.
#include <float.h>
#include <math.h>
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "lua_random.h"
#include "mod_storage.h"
#include "ritual_effect_names.h"

// Lua steps one call into a script may run.
#define LUA_STEP_LIMIT      20000000
#define LUA_STEP_LIMIT_TEXT "20000000"
// How often the step hook runs.
#define LUA_HOOK_STEPS      10000

static const PluginApi *const luaApi = &API;

// The Lua memory one script's state may hold, and all scripts' together.
static size_t luaMemoryScriptMax = (size_t)1536 << 20;
static size_t luaMemoryTotalMax = (size_t)6 << 30;
static size_t luaMemoryTotal;

// Why CoE5SE stopped a script: its reason word in the log, and its error text.
enum { LUA_STOP_NONE, LUA_STOP_STEPS, LUA_STOP_MEMORY, LUA_STOP_MEMORY_TOTAL, LUA_STOP_CLOSED, LUA_STOP_KINDS };
static const struct { const char *reason, *error; } LUA_STOPS[LUA_STOP_KINDS] = {
    [LUA_STOP_STEPS] = {"step_limit", "script ran too long (more than " LUA_STEP_LIMIT_TEXT " steps in one call)"},
    [LUA_STOP_MEMORY] = {"memory_cap", "script used more than its 1.5 GB of memory"},
    [LUA_STOP_MEMORY_TOTAL] = {"memory_total", "scripts used more than 6 GB of memory together, and this one the most"},
    [LUA_STOP_CLOSED] = {"script_closed", "script was turned off"},
};
// The boxes CoE5SE shows the player about a mod's script, each once a launch.
enum { LUA_NOTICE_STOPPED = 1, LUA_NOTICE_STORAGE = 2 };
// Calls into scripts whose code runs now.
static int luaCodeRunning;

typedef struct {
    char mod[MOD_NAME_MAX], path[MAX_PATH];
    char folder[MAX_PATH];
    char dataFolder[STORAGE_NAME_MAX + 1];
    long long dataBytes;
    int dataBytesKnown;
    lua_State *L;
    int dead;
    int stopLogged;
    long long stepsLeft;
    unsigned long stepsFrame;
    int depth;
    int stopping;
    size_t memory;
    int noticesDue, noticesShown;
    int warnOff, warnContinued, warningLength, warningCapacity;
    char *warning;
} LuaScript;
// The scripts, each allocated on its own.
static LuaScript **luaScripts;
static int luaScriptCount, luaScriptCapacity;

// ---- Protected calls --------------------------------------------------------------------------
// Returns the script that owns the Lua state L, or NULL.
static LuaScript *luaScriptOf(lua_State *L) { return L ? *(LuaScript **)lua_getextraspace(L) : NULL; }

// The script whose code runs now (the innermost callLuaProtected), or NULL.
static LuaScript *luaRunning;
static const char *runningLuaMod(void) { return luaRunning ? luaRunning->mod : NULL; }

// Logs lua_refused for a script of mod `mod`.
static PRINTF_FORMAT(4, 5) void refuseLuaCall(const char *mod, const char *call, const char *reason, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logRefusal("lua_refused", call, "mod", mod ? mod : "?", reason, fmt, ap);
    va_end(ap);
}

// The registry key of each stop's error text.
static char luaStopKeys[LUA_STOP_KINDS];

static void luaStepHook(lua_State *L, lua_Debug *ar);
// Stops script s for `why`, once.
static void stopLuaScript(LuaScript *s, int why, const char *detail) {
    if (s->stopping) return;
    s->stopping = why;
    s->dead = 1;
    if (why != LUA_STOP_CLOSED) s->noticesDue |= LUA_NOTICE_STOPPED;
    refuseLuaCall(s->mod, "script", LUA_STOPS[why].reason, "%s", detail);
    if (s->L) lua_sethook(s->L, luaStepHook, LUA_MASKCOUNT, 1);
}
// Raises the stop's error in L, a thread of stopped script s.
static int raiseLuaStop(lua_State *L, LuaScript *s) {
    lua_sethook(L, luaStepHook, LUA_MASKCOUNT, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &luaStopKeys[s->stopping]);
    return lua_error(L);
}

// Stops script s for spending its step budget.
static void stopLuaScriptForSteps(lua_State *L, LuaScript *s) {
    lua_Debug ar;
    char detail[LUA_IDSIZE + 64] = "steps=" LUA_STEP_LIMIT_TEXT;
    for (int level = 0; level < 3 && lua_getstack(L, level, &ar); level++)
        if (lua_getinfo(L, "Sl", &ar) && ar.currentline > 0) {
            snprintf(detail, sizeof detail, "steps=" LUA_STEP_LIMIT_TEXT " at=%s:%d", ar.short_src, ar.currentline);
            break;
        }
    stopLuaScript(s, LUA_STOP_STEPS, detail);
}

// The step hook.
static void luaStepHook(lua_State *L, lua_Debug *ar) {
    (void)ar;
    LuaScript *s = luaScriptOf(L);
    if (!s) return;
    if (!s->stopping) {
        s->stepsLeft -= LUA_HOOK_STEPS;
        if (s->stepsLeft > 0) return;
        stopLuaScriptForSteps(L, s);
    }
    raiseLuaStop(L, s);
}
// What the library loops charge: `steps` of the budget of L's script.
void coe5seChargeLuaSteps(lua_State *L, size_t steps) {
    if (lua_gethook(L) != luaStepHook) return;
    LuaScript *s = luaScriptOf(L);
    if (!s) return;
    if (!s->stopping) {
        s->stepsLeft -= steps < LUA_STEP_LIMIT ? (long long)steps : LUA_STEP_LIMIT;
        if (s->stepsLeft > 0) return;
        stopLuaScriptForSteps(L, s);
    }
    raiseLuaStop(L, s);
}
// Starts script s's step count again.
static void startLuaSteps(LuaScript *s) {
    s->stepsLeft = LUA_STEP_LIMIT;
    s->stepsFrame = gameFrames;
}
static void resetLuaSteps(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    if (s) startLuaSteps(s);
}

// The script that last turned setupAutomated on.
static LuaScript *luaSetupAutomatedBy;

// Returns 1, with lua_refused logged for `call`, when the script of L was turned off.
static int refuseFromDeadLuaScript(lua_State *L, const char *call) {
    LuaScript *s = luaScriptOf(L);
    if (!s || !s->dead) return 0;
    refuseLuaCall(s->mod, call, "script_stopped", "depth=%d", s->depth);
    return 1;
}

// ---- Memory: each script's state allocates through luaAlloc ------------------------------------
// Closes the Lua state of script s, which is turned off and has no call running.
static void closeLuaScriptState(LuaScript *s) {
    if (!s->L || s->depth) return;
    if (luaSetupAutomatedBy == s) {
        luaSetupAutomatedBy = NULL;
        if (luaApi->setupAutomated()) {
            luaApi->setSetupAutomated(0);
            writeLog("lua_setup_automated_cleared", "mod=%s", s->mod);
        }
    }
    if (!s->stopping) s->stopping = LUA_STOP_CLOSED;
    s->warnOff = 1;
    lua_sethook(s->L, luaStepHook, LUA_MASKCOUNT, 1);
    size_t held = s->memory;
    lua_close(s->L);
    s->L = NULL;
    writeLog("lua_script_closed", "mod=%s memory=%zu", s->mod, held);
}
// After a call into script s: closes it if it was turned off meanwhile.
static void closeDeadLuaScript(LuaScript *s) {
    if (s->dead && !s->depth) closeLuaScriptState(s);
}

// Whether script s may hold `grow` more bytes.
static int luaMemoryFits(LuaScript *s, size_t grow) {
    char detail[128 + MOD_NAME_MAX];
    if (s->memory + grow > luaMemoryScriptMax) {
        snprintf(detail, sizeof detail, "bytes=%zu max=%zu", s->memory + grow, luaMemoryScriptMax);
        stopLuaScript(s, LUA_STOP_MEMORY, detail);
        return 0;
    }
    while (luaMemoryTotal + grow > luaMemoryTotalMax) {
        LuaScript *big = s;
        for (int i = 0; i < luaScriptCount; i++)
            if (luaScripts[i]->L && luaScripts[i]->memory > big->memory) big = luaScripts[i];
        snprintf(detail, sizeof detail, "bytes=%zu total=%zu max=%zu asked_by=%s", big->memory, luaMemoryTotal + grow,
                 luaMemoryTotalMax, s->mod);
        stopLuaScript(big, LUA_STOP_MEMORY_TOTAL, detail);
        if (big == s) return 0;
        if (!big->stopLogged) writeLog("lua_script_stopped", "mod=%s", big->mod);
        big->stopLogged = 1;
        if (big->depth) return 1;
        closeLuaScriptState(big);
    }
    return 1;
}

// The allocator of every script's state.
static void *luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    LuaScript *s = ud;
    size_t old = ptr ? osize : 0;
    if (nsize == 0) {
        free(ptr);
        s->memory -= old;
        luaMemoryTotal -= old;
        return NULL;
    }
    if (nsize > old && !luaMemoryFits(s, nsize - old)) return NULL;
    void *p = realloc(ptr, nsize);
    if (!p) return NULL;
    s->memory = s->memory - old + nsize;
    luaMemoryTotal = luaMemoryTotal - old + nsize;
    return p;
}
// A Lua error outside every protected call.
static int luaPanic(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    writeLog("lua_panic", "mod=%s error=%s", s ? s->mod : "?", lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "(not a string)");
    return 0;
}

// What callLuaProtected calls: `push` pushes the function and its arguments.
typedef int (*LuaPushFn)(lua_State *L, void *ctx);
typedef struct { LuaPushFn push; void *ctx; int nresults; } LuaProtectedCall;
static int runLuaProtectedCall(lua_State *L) {
    const LuaProtectedCall *c = lua_touserdata(L, 1);
    lua_settop(L, 0);
    int nargs = c->push(L, c->ctx);
    lua_call(L, nargs, c->nresults);
    return c->nresults;
}

// Calls what `push` pushes and leaves `nresults` results on the stack.
static int callLuaProtected(LuaScript *s, LuaPushFn push, void *ctx, int nresults) {
    if (!lua_checkstack(s->L, 2)) {
        refuseLuaCall(s->mod, "script", "out_of_memory", "stack=full");
        return 0;
    }
    LuaProtectedCall call = {push, ctx, nresults};
    if (!s->depth++ || s->stepsFrame != gameFrames) startLuaSteps(s);
    LuaScript *outer = luaRunning;
    luaRunning = s;
    luaCodeRunning++;
    lua_pushcfunction(s->L, runLuaProtectedCall);
    lua_pushlightuserdata(s->L, &call);
    int rc = lua_pcall(s->L, 1, nresults, 0);
    luaCodeRunning--;
    luaRunning = outer;
    s->depth--;
    if (rc == LUA_OK) {
        if (!nresults) closeDeadLuaScript(s);
        return 1;
    }
    char line[600];
    int type = lua_type(s->L, -1);
    if (type == LUA_TSTRING) snprintf(line, sizeof line, "%s", lua_tostring(s->L, -1));
    else if (type == LUA_TNUMBER && lua_isinteger(s->L, -1)) snprintf(line, sizeof line, "%lld", (long long)lua_tointeger(s->L, -1));
    else if (type == LUA_TNUMBER) snprintf(line, sizeof line, "%.14g", (double)lua_tonumber(s->L, -1));
    else snprintf(line, sizeof line, "(error object is a %s value)", lua_typename(s->L, type));
    writeLog("lua_error", "mod=%s error=%s", s->mod, line);
    lua_pop(s->L, 1);
    s->dead = 1;
    if (!s->stopLogged) writeLog("lua_script_stopped", "mod=%s", s->mod);
    s->stopLogged = 1;
    closeDeadLuaScript(s);
    return 0;
}

// ---- Callbacks: a script's main menu buttons, Game Menu lines, screen lines, and Read Tip hiders --
// The scripts' stored functions.
typedef struct { LuaScript *s; int ref; int refVisible; } LuaCallback;
static LuaCallback *luaCallbacks;
static int luaCallbackCount, luaCallbackCapacity;

// callLuaProtected's push for a function kept in the registry, called with no arguments.
static int pushLuaRef(lua_State *L, void *ref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, *(const int *)ref);
    return 0;
}

// Runs callback i's Lua function.
static int runLuaCallback(int i, int wantResult, int fallback) {
    if (i < 0 || i >= luaCallbackCount) return fallback;
    LuaCallback c = luaCallbacks[i];
    if (!c.s || c.s->dead || isNetworkGame()) return fallback;
    if (!callLuaProtected(c.s, pushLuaRef, &c.ref, 1)) return fallback;
    int r = wantResult ? (lua_isnumber(c.s->L, -1) ? (int)lua_tointeger(c.s->L, -1) : lua_toboolean(c.s->L, -1)) : fallback;
    lua_pop(c.s->L, 1);
    closeDeadLuaScript(c.s);
    return r;
}
// Runs callback i's visible function.
static int runLuaVisible(int i) {
    if (i < 0 || i >= luaCallbackCount) return 0;
    LuaCallback c = luaCallbacks[i];
    if (!c.s || c.s->dead || isNetworkGame()) return 0;
    if (c.refVisible == LUA_NOREF) return 1;
    if (!callLuaProtected(c.s, pushLuaRef, &c.refVisible, 1)) return 0;
    int r = lua_toboolean(c.s->L, -1);
    lua_pop(c.s->L, 1);
    closeDeadLuaScript(c.s);
    return r;
}

// Stores the Lua function at stack `index`, and its visible function, as a new callback.
static int newLuaCallback(lua_State *L, int index, int visibleIndex) {
    luaL_checktype(L, index, LUA_TFUNCTION);
    if (!growList((void **)&luaCallbacks, &luaCallbackCapacity, luaCallbackCount + 1, sizeof luaCallbacks[0])) return -1;
    LuaCallback *c = &luaCallbacks[luaCallbackCount];
    c->s = luaScriptOf(L);
    lua_pushvalue(L, index);
    c->ref = luaL_ref(L, LUA_REGISTRYINDEX);
    c->refVisible = LUA_NOREF;
    if (visibleIndex && lua_isfunction(L, visibleIndex)) {
        lua_pushvalue(L, visibleIndex);
        c->refVisible = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    return luaCallbackCount++;
}

// Raises a Lua error if L isn't the script's main thread.
static void checkLuaMainThread(lua_State *L, const char *what) {
    int isMain = lua_pushthread(L);
    lua_pop(L, 1);
    if (!isMain) luaL_error(L, "%s can't be called from a coroutine", what);
}

// ---- Named game data: coe5.get(name[, index]), coe5.set(name, [index,] value) ----------------
// Scripts name the values they read and write, and only this table holds the addresses.

// The slot* names take a player slot, 0-31, the game's own slot numbers.
enum {
    T_U8, T_I16, T_I32,
    T_STR,
    T_FLAG_U8,
    T_FLAG_POSITIVE,
    T_FRAME_I32,
    T_FLAG_I32,
    T_PTR_STR,
    T_F32,
    T_FLAG_I16,
    T_FLAG_TWO,
    T_SITE_I32,
    T_ARMY_OWNER,
    T_CLASS_STDTROOPS,
    T_CLASS_ABILITY,
    T_RITUAL_SUMMONS,
    T_I64,
    T_TICKS_MS,
    T_EFFECT_NAME,
    T_RITUAL_TERRAIN_NAME,
    T_RITUAL_TERRAIN_AT,
    T_RITUAL_EFFECT_MET,
    T_RITUAL_EFFECT_TERRAIN_NAME,
    T_CAST_COUNT,
    T_UNIT_AP_LEFT,
    T_UNIT_ABILITY,
    T_COMMANDER_COST,
    T_SLOT_INCOME,
    T_TERRAIN_INCOME,
    T_I8,
    T_PLANE_AT,
    T_COMMANDER_STRENGTH,
    T_WINDOWS_ONLY,
    T_GAME_CHANGED,
    T_SQUARE_STRENGTH,
    T_REMEMBERED_STRENGTH,
    T_ARMY_STATIONARY,
    T_ARMY_HOLDS,
    T_GARRISON_LEFT,
    T_UNIT_STRENGTH,
    T_UNIT_ARMOR,
    T_UNIT_VISIBLE,
    T_MONSTER_ATTACK,
    T_SQUARE_CITADEL,
    T_UNIT_STATIONARY,
    T_CEREMONY_NAME,
    T_CEREMONY_COST,
    T_CEREMONY_LAST,
    T_UNIT_MOVE_FLAGS,
    T_UNIT_FLAG_WORDS,
    T_COMMANDER_MOVE_FLAGS,
    T_COMMANDER_RITPOW,
    T_COMMANDER_RITUAL_LEVEL,
    T_COMMANDER_SPELL_LEVEL,
    T_SQUARE_LIBRARY_LEVEL,
    T_MONSTER_ABILITY,
    T_FORT_PART_WORD,
};
typedef struct {
    const char *name;
    uintptr_t addr;
    int type, count, stride, size, writable;
    CallSite site;
    int count2, stride2;
} GameVar;
// The variables that need more than two indexes, by name.
enum { LIST_NONE, LIST_TO_EMPTY_NAME, LIST_TO_BELOW_ONE, LIST_TO_BELOW_ONE_I32, LIST_TO_BELOW_ZERO_I32 };
typedef struct { const char *name; int count3, stride3, list, nameBack; uintptr_t lastAt; int everyPlayer; } GameVarShape;
// Types that differ between the two builds.
#ifdef __APPLE__
#define LOADED_GAME_TYPE T_U8
#define NETWORK_GAME_TYPE T_FLAG_U8
#define INT_FLAG_TYPE T_FLAG_U8
#define WILDER_DEFAULT_TYPE T_FLAG_U8
#else
#define LOADED_GAME_TYPE T_I32
#define NETWORK_GAME_TYPE T_FLAG_POSITIVE
#define INT_FLAG_TYPE T_FLAG_I32
#define WILDER_DEFAULT_TYPE T_FLAG_TWO
#endif
// @platform-split ai_test_values: bugChecksOff and commanderStrength read nil on Mac (RVA_NO_CHECKS and RVA_COMMANDER_STRENGTH not looked up)
// Values only the Windows build has an address for so far.
#ifdef __APPLE__
#define WINDOWS_ONLY_RVA(rva) 0
#define BUG_CHECKS_OFF_TYPE T_WINDOWS_ONLY
#define COMMANDER_STRENGTH_TYPE T_WINDOWS_ONLY
#else
#define WINDOWS_ONLY_RVA(rva) (rva)
#define BUG_CHECKS_OFF_TYPE T_FLAG_I32
#define COMMANDER_STRENGTH_TYPE T_COMMANDER_STRENGTH
#endif
// @platform-split ai_square_strength: squareStrength and squareRememberedStrength read nil on Mac (RVA_SQUARE_STRENGTH not looked up, SQUARE_AI_STRENGTH not checked there)
// A square's strength, now and as an AI player remembers it: Windows only.
#ifdef __APPLE__
#define SQUARE_STRENGTH_TYPE T_WINDOWS_ONLY
#define REMEMBERED_STRENGTH_TYPE T_WINDOWS_ONLY
#else
#define SQUARE_STRENGTH_TYPE T_SQUARE_STRENGTH
#define REMEMBERED_STRENGTH_TYPE T_REMEMBERED_STRENGTH
#endif
// @platform-split army_stationary: squareArmyStationary reads nil on Mac (RVA_SQUARE_ARMY_STATIONARY not looked up)
// Whether the game ever moves the army on a square: Windows only.
#ifdef __APPLE__
#define ARMY_STATIONARY_TYPE T_WINDOWS_ONLY
#else
#define ARMY_STATIONARY_TYPE T_ARMY_STATIONARY
#endif
// @platform-split army_holds: squareArmyHolds reads nil on Mac (RVA_SQUARE_TOP_UNIT not looked up)
// Whether the independents' move holds the army on a square: Windows only.
#ifdef __APPLE__
#define ARMY_HOLDS_TYPE T_WINDOWS_ONLY
#else
#define ARMY_HOLDS_TYPE T_ARMY_HOLDS
#endif
// @platform-split unit_visible: unitVisibleTo reads nil on Mac (RVA_UNIT_VISIBLE_TO not looked up)
// Whether a player sees a unit: Windows only.
#ifdef __APPLE__
#define UNIT_VISIBLE_TYPE T_WINDOWS_ONLY
#else
#define UNIT_VISIBLE_TYPE T_UNIT_VISIBLE
#endif
// @platform-split garrison_left: commanderGarrisonLeft reads nil on Mac (RVA_AI_GATHER_TROOPS and the functions it calls not looked up)
// What stays on a commander's square when it leaves: Windows only.
#ifdef __APPLE__
#define GARRISON_LEFT_TYPE T_WINDOWS_ONLY
#else
#define GARRISON_LEFT_TYPE T_GARRISON_LEFT
#endif
// @platform-split ai_unit_fight: unitStrength, unitArmor, monsterAttackType, monsterAttackWeapon, monsterAttackValue, weaponDmgType and weaponAoe read nil on Mac (RVA_UNIT_STRENGTH_FOR and RVA_UNIT_ARMOR not looked up, MONSTER_ATTACKS and the WEAPON_ offsets not checked there)
// What a fight estimate reads of units, monsters and weapons: Windows only.
#ifdef __APPLE__
#define UNIT_STRENGTH_TYPE T_WINDOWS_ONLY
#define UNIT_ARMOR_TYPE T_WINDOWS_ONLY
#define MONSTER_ATTACK_TYPE T_WINDOWS_ONLY
#define WEAPON_NUMBER_TYPE T_WINDOWS_ONLY
#else
#define UNIT_STRENGTH_TYPE T_UNIT_STRENGTH
#define UNIT_ARMOR_TYPE T_UNIT_ARMOR
#define MONSTER_ATTACK_TYPE T_MONSTER_ATTACK
#define WEAPON_NUMBER_TYPE T_I32
#endif
// @platform-split square_citadel: squareCitadel reads nil on Mac (RVA_SQUARE_IS_CITADEL_FOR not looked up)
// Whether a square is a citadel for a player: Windows only.
#ifdef __APPLE__
#define SQUARE_CITADEL_TYPE T_WINDOWS_ONLY
#else
#define SQUARE_CITADEL_TYPE T_SQUARE_CITADEL
#endif
// @platform-split ceremony_values: ceremonyName, ceremonyCost and ceremonyLastIndex read nil on Mac (the ceremony code and table not looked up)
// The Voice of El's ceremonies: their names, the cost table, and the last number.
#ifdef __APPLE__
#define CEREMONY_NAME_TYPE T_WINDOWS_ONLY
#define CEREMONY_COST_TYPE T_WINDOWS_ONLY
#define CEREMONY_LAST_TYPE T_WINDOWS_ONLY
#else
#define CEREMONY_NAME_TYPE T_CEREMONY_NAME
#define CEREMONY_COST_TYPE T_CEREMONY_COST
#define CEREMONY_LAST_TYPE T_CEREMONY_LAST
#endif
#define CEREMONY_INDEXES (CEREMONY_LAST + 1)
#define FORT_PART_BITS 32
// @platform-split slot_income: slotIncome reads nil on Mac (the income functions player_turn_start calls not looked up)
// A player's income at its next turn start: Windows only.
#ifdef __APPLE__
#define SLOT_INCOME_TYPE T_WINDOWS_ONLY
#else
#define SLOT_INCOME_TYPE T_SLOT_INCOME
#endif
// @platform-split commander_done: commanderDone reads nil on Mac (RVA_COMMANDER_DONE not looked up)
// Whether the player gave a commander "Commander is done for this turn": Windows only.
#ifdef __APPLE__
#define COMMANDER_DONE_TYPE T_WINDOWS_ONLY
#else
#define COMMANDER_DONE_TYPE T_FLAG_U8
#endif
// @platform-split ai_move_reads: monsterMoveFlags, commanderMoveFlags, commanderRitpow, commanderRitualLevel, terrainFlags, squareLibraryLevel, seasonTurnsLeft, aiAttackDivOther, aiAttackDivAi, aiAttackDivHuman and aiAttackMargin read nil on Mac (RVA_ARMY_MOVE_FLAGS, RVA_COMMANDER_RITPOW, RVA_SQUARE_LIBRARY_LEVEL, RVA_SEASON_TURNS_LEFT and the four RVA_AI_ATTACK_ globals not looked up, MONSTER_MOVE_FLAGS and TERRAIN_FLAGS not checked there)
// What the AI's move and target choices read: Windows only.
#ifdef __APPLE__
#define AI_MOVE_I64_TYPE T_WINDOWS_ONLY
#define AI_MOVE_I32_TYPE T_WINDOWS_ONLY
#define AI_MOVE_I16_TYPE T_WINDOWS_ONLY
#define COMMANDER_MOVE_FLAGS_TYPE T_WINDOWS_ONLY
#define COMMANDER_RITPOW_TYPE T_WINDOWS_ONLY
#define COMMANDER_RITUAL_LEVEL_TYPE T_WINDOWS_ONLY
#define SQUARE_LIBRARY_LEVEL_TYPE T_WINDOWS_ONLY
#else
#define AI_MOVE_I64_TYPE T_I64
#define AI_MOVE_I32_TYPE T_I32
#define AI_MOVE_I16_TYPE T_I16
#define COMMANDER_MOVE_FLAGS_TYPE T_COMMANDER_MOVE_FLAGS
#define COMMANDER_RITPOW_TYPE T_COMMANDER_RITPOW
#define COMMANDER_RITUAL_LEVEL_TYPE T_COMMANDER_RITUAL_LEVEL
#define SQUARE_LIBRARY_LEVEL_TYPE T_SQUARE_LIBRARY_LEVEL
#endif
#define RESOURCE_COUNT 14
#define NAME_SIZE 100
#define COUNT_AT_CALL 0
#define SQUARE_COLUMN_SIZE (MAP_COLUMN_SQUARES * SQUARE_SIZE)
// Each row: name, address, type, count, stride, size, writable, site, count2, stride2.
static const GameVar GAME_VARS[] = {
    {"turn",           RVA_TURN,                          T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"networkGame",    RVA_NET_CLIENT,                    NETWORK_GAME_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"loadedGame",     RVA_LOADED_GAME,                   LOADED_GAME_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"loadedMapName",  RVA_LOADED_MAP_NAME,               T_STR, 1, 0, 256, 0, 0, 0, 0},
    {"society",        RVA_SOCIETY,                       T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"commonCause",    RVA_COMMON_CAUSE,                  T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"scoreGraphs",    RVA_SCORE_GRAPHS,                  T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"battleReports",  RVA_BATTLE_REPORTS,                T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"startFlags",     RVA_START_FLAGS,                   T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"wilder",         RVA_WILDER,                        T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapW",           RVA_MAP_W,                         T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapH",           RVA_MAP_H,                         T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapDepth",       RVA_MAP_DEPTH,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"northPart",      RVA_NORTH_PART,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"southPart",      RVA_SOUTH_PART,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"slotController", RVA_SLOT_CONTROLLER,               T_I16, 32, 2, 0, 1, 0, 0, 0},
    {"slotClass",      RVA_SLOT_CLASS,                    T_I16, 32, 2, 0, 1, 0, 0, 0},
    {"slotAi",         RVA_SLOT_AI,                       T_I16, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotTeam",       RVA_SLOT_TEAM,                     T_I16, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotName",       RVA_SLOT_NAME,                     T_STR, 32, PLAYER_BLOCK_SIZE, 32, 1, 0, 0, 0},
    {"slotColorRed",   RVA_SLOT_COLOUR,                   T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotColorGreen", RVA_SLOT_COLOUR + 4,               T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotColorBlue",  RVA_SLOT_COLOUR + 8,               T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotResource",   RVA_PLAYER_INTS,                   T_I32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 14, 4},
    {"slotIncome",     0,                                 SLOT_INCOME_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, RESOURCE_COUNT, 0},
    {"newWorldUniqueRandom", FRAME_NEW_WORLD_UNIQUE_RANDOM, T_FRAME_I32, 1, 0, 0, 1, SITE_NEW_WORLD_CREATING_WORLD, 0, 0},
    {"cliMapW",        RVA_CLI_MAP_W,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"cliMapH",        RVA_CLI_MAP_H,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"cliSociety",     RVA_CLI_SOCIETY,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"subSociety",     RVA_SUBSOC,                        INT_FLAG_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"startDefault",   RVA_START_DEFAULT,                 T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"wilderDefault",  RVA_WILDER_DEFAULT,                WILDER_DEFAULT_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"setupNetwork",   RVA_SETUP_NETWORK,                 T_FLAG_I16, 1, 0, 0, 0, 0, 0, 0},
    {"aiLevelName",    RVA_AI_LEVEL_NAMES,                T_PTR_STR, AI_LEVEL_TABLE_MAX, 8, NAME_SIZE, 0, 0, 0, 0},
    {"screenW",        RVA_SCREEN_W,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"screenH",        RVA_SCREEN_H,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"hoveredId",      RVA_HOVERED_ID,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"bannerFade",     RVA_BANNER_FADE,                   T_F32, 1, 0, 0, 1, 0, 0, 0},
    {"activePlayer",   RVA_ACTIVE_PLAYER,                 T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"season",         RVA_SEASON,                        T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"seasonName",     RVA_SEASON_NAMES,                  T_PTR_STR, 4, 8, NAME_SIZE, 0, 0, 0, 0},
    {"date",           RVA_DATE_TEXT,                     T_STR, 1, 0, 100, 0, 0, 0, 0},
    {"worldW",         RVA_WORLD_W,                       T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"worldH",         RVA_WORLD_H,                       T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"uniqueRandomDefault", RVA_UNIQUE_RANDOM_DEFAULT,    INT_FLAG_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"slotLockClass",      RVA_SLOT_LOCK_CLASS,           T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockController", RVA_SLOT_LOCK_CONTROLLER,      T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockAi",         RVA_SLOT_LOCK_AI,              T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockTeam",       RVA_SLOT_LOCK_TEAM,            T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"eventVar",       RVA_EVENT_VARS,                    T_I32, EVENT_VAR_COUNT, 4, 0, 1, 0, 0, 0},
    {"modLastIndex",   RVA_MOD_LAST_INDEX,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"modName",        RVA_MOD_NAMES,                     T_STR, MOD_TABLE_MAX, MOD_RECORD_SIZE, 256, 0, 0, 0, 0},
    {"modFolder",      RVA_MOD_FOLDERS,                   T_STR, MOD_TABLE_MAX, MOD_RECORD_SIZE, 256, 0, 0, 0, 0},
    {"modEnabled",     RVA_MOD_NAMES + MOD_RECORD_ENABLED, T_I16, MOD_TABLE_MAX, MOD_RECORD_SIZE, 0, 0, 0, 0, 0},
    {"monsterName",    RVA_MONSTER_TABLE,                 T_STR, MONSTER_TABLE_MAX, MONSTER_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"monsterLastIndex", RVA_MONSTER_COUNT,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"monsterSprite",  RVA_MONSTER_SPRITES,               T_I32, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"itemName",       RVA_ITEM_TABLE,                    T_STR, ITEM_TABLE_MAX, ITEM_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"itemLastIndex",  RVA_ITEM_COUNT,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"className",      RVA_CLASS_TABLE,                   T_STR, CLASS_TABLE_MAX, CLASS_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"classLeader",    RVA_CLASS_LEADERS,                 T_STR, CLASS_TABLE_MAX, CLASS_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"classLastIndex", RVA_CLASS_COUNT,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"terrainName",    RVA_TERRAIN_TABLE,                 T_STR, TERRAIN_TABLE_MAX, TERRAIN_SIZE, TERRAIN_NAME_SIZE, 0, 0, 0, 0},
    {"terrainLastIndex", RVA_LAST_TERRAIN,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"terrainIncome",  RVA_TERRAIN_TABLE,                 T_TERRAIN_INCOME, TERRAIN_TABLE_MAX, TERRAIN_SIZE, 0, 0, 0, RESOURCE_COUNT, 0},
    {"ritualName",     RVA_RITUAL_TABLE,                  T_STR, COUNT_AT_CALL, RITUAL_SIZE, 48, 0, 0, 0, 0},
    {"ritualLastIndex", RVA_RITUAL_COUNT,                 T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"resourceName",   RVA_RESOURCE_NAMES,                T_PTR_STR, 14, 8, NAME_SIZE, 0, 0, 0, 0},
    {"incomeName",     RVA_INCOME_NAMES,                  T_PTR_STR, INCOME_NAME_COUNT, 8, NAME_SIZE, 0, 0, 0, 0},
    {"classRecruitName", RVA_CLASS_SIDE_A + RECRUIT_MONSTER_NAME, T_STR, CLASS_TABLE_MAX, CLASS_RECRUITS_SIZE, MONSTER_NAME_SIZE, 0, 0,
     CLASS_RECRUITS_MAX, CLASS_RECRUIT_SIZE},
    {"classRecruitCount", RVA_CLASS_SIDE_A + RECRUIT_COUNT, T_I32, CLASS_TABLE_MAX, CLASS_RECRUITS_SIZE, 0, 0, 0,
     CLASS_RECRUITS_MAX, CLASS_RECRUIT_SIZE},
    {"classStartArmy", RVA_CLASS_TABLE + CLASS_START_UNITS, T_STR, CLASS_TABLE_MAX, CLASS_SIZE, CLASS_START_UNIT_SIZE, 0, 0,
     CLASS_START_UNITS_MAX, CLASS_START_UNIT_SIZE},
    {"classStandardTroops", RVA_CLASS_TABLE, T_CLASS_STDTROOPS, CLASS_TABLE_MAX, CLASS_SIZE, 0, 0, 0, 0, 0},
    {"classAbility",   RVA_CLASS_TABLE,                   T_CLASS_ABILITY, CLASS_TABLE_MAX, CLASS_SIZE, 0, 0, 0, INT_MAX, 0},
    {"mapPointerX",    RVA_SQUARE_INFO_X,                 T_SITE_I32, 1, 0, 0, 0, SITE_MAP_SCREEN_WAIT, 0, 0},
    {"mapPointerY",    RVA_SQUARE_INFO_Y,                 T_SITE_I32, 1, 0, 0, 0, SITE_MAP_SCREEN_WAIT, 0, 0},
    {"mouseButtons",   RVA_MOUSE_BUTTONS,                 T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"squareSeen",     RVA_SQUARE_RECORDS + SQUARE_SEEN,  T_U8, PLAYER_SLOTS, 1, 0, 0, 0, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE},
    {"squareArmy",     RVA_SQUARE_ARMY,                   T_I16, MAP_COLUMNS_MAX, MAP_COLUMN_SQUARES * 2, 0, 0, 0, MAP_COLUMN_SQUARES, 2},
    {"squareArmyOwner", RVA_SQUARE_RECORDS + SQUARE_ARMY_OWNER, T_ARMY_OWNER, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0,
     MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareOwner",    RVA_SQUARE_RECORDS + SQUARE_OWNER, T_I8, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareTerrain",  RVA_SQUARE_RECORDS + SQUARE_TERRAIN_TYPE, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0,
     MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squarePortal",   RVA_SQUARE_RECORDS + SQUARE_PORTAL, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES,
     SQUARE_SIZE},
    {"squarePlane",    0,                                 T_PLANE_AT, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"commanderLastIndex", RVA_LAST_COMMANDER,            T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"commanderUnit",  RVA_COMMANDERS,                    T_I32, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitual", RVA_COMMANDER_KNOWN_RITUALS,      T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     COMMANDER_KNOWN_RITUALS_MAX, 2},
    {"commanderCostAvailable", RVA_COMMANDERS,            T_COMMANDER_COST, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     INCOME_NAME_COUNT, 0},
    {"commanderAiTargetX", RVA_COMMANDER_AI_TARGET_X,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiTargetY", RVA_COMMANDER_AI_TARGET_Y,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiMoveKind", RVA_COMMANDER_AI_MOVE_KIND,   T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderRitualPlan", RVA_COMMANDER_AI_PLAN,        T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderDone",  WINDOWS_ONLY_RVA(RVA_COMMANDER_DONE), COMMANDER_DONE_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotLeader",     RVA_PLAYER_LEADER,                 T_I32, 32, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"unitLastIndex",  RVA_UNIT_COUNT,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"unitOwner",      RVA_UNIT_PLAYER,                   T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitHp",         RVA_UNIT_HP,                       T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitMonster",    RVA_UNIT_MONSTER,                  T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitCommander",  RVA_UNITS + UNIT_COMMANDER,        T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitStationary", RVA_UNITS,                         T_UNIT_STATIONARY, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"ritualSummons",  RVA_RITUAL_TABLE + RITUAL_FX,      T_RITUAL_SUMMONS, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualString",   RVA_RITUAL_TABLE + RITUAL_STRINGS, T_STR, COUNT_AT_CALL, RITUAL_SIZE, RITUAL_STRING_SIZE, 0, 0,
     RITUAL_STRINGS_MAX, RITUAL_STRING_SIZE},
    {"ritualRitpow",   RVA_RITUAL_TABLE + RITUAL_RITPOW,  T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualLevel",    RVA_RITUAL_TABLE + RITUAL_LEVEL,   T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualCostType", RVA_RITUAL_TABLE + RITUAL_COSTS,   T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_COSTS_MAX, RITUAL_COST_SIZE},
    {"ritualCost",     RVA_RITUAL_TABLE + RITUAL_COSTS + 4, T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_COSTS_MAX,
     RITUAL_COST_SIZE},
    {"ritualTerrain",  RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualEffect",   RVA_RITUAL_TABLE + RITUAL_FX,      T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"ritualEffectValue", RVA_RITUAL_TABLE + RITUAL_FX + RITUAL_FX_VALUE, T_I64, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"ritualFlags",    RVA_RITUAL_TABLE + RITUAL_FLAGS,   T_I64, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualTerrainName", RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_RITUAL_TERRAIN_NAME, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualTerrainAt", RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_RITUAL_TERRAIN_AT, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     MAP_COLUMNS_MAX, 0},
    {"ritualEffectMet", RVA_RITUAL_TABLE + RITUAL_FX,     T_RITUAL_EFFECT_MET, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_FX_MAX,
     RITUAL_FX_SIZE},
    {"ritualEffectTerrainName", RVA_RITUAL_TABLE + RITUAL_FX, T_RITUAL_EFFECT_TERRAIN_NAME, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"effectName",     0,                                 T_EFFECT_NAME, RITUAL_EFFECT_ID_COUNT, 0, 0, 0, 0, 0, 0},
    {"slotRitualCasts", RVA_CAST_COUNTS,                  T_CAST_COUNT, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, COUNT_AT_CALL, 2},
    {"unitX",          RVA_UNIT_X,                        T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitY",          RVA_UNIT_X + 4,                    T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitApLeft",     RVA_UNITS,                         T_UNIT_AP_LEFT, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitApUsed",     RVA_UNITS + UNIT_AP_USED,          T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 1, 0, 0, 0},
    {"unitAbility",    RVA_UNITS,                         T_UNIT_ABILITY, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, RITUAL_EFFECT_ID_COUNT, 0},
    {"planeName",      RVA_PLANES,                        T_STR, PLANE_TABLE_MAX, PLANE_SIZE, PLANE_NAME_SIZE, 0, 0, 0, 0},
    {"planeLastIndex", RVA_LAST_PLANE,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"ticksMs",        RVA_TICKS_MS,                      T_TICKS_MS, 1, 0, 0, 0, 0, 0, 0},
    {"tooltipLineHeight", RVA_TOOLTIP_LINE_HEIGHT,        T_F32, 1, 0, 0, 0, 0, 0, 0},
    {"traceLevel",     RVA_DEBUG_LEVEL,                   T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"bugChecksOff",   WINDOWS_ONLY_RVA(RVA_NO_CHECKS),   BUG_CHECKS_OFF_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"commanderName",  RVA_COMMANDER_NAMES,               T_STR, COMMANDER_TABLE_MAX, COMMANDER_SIZE, COMMANDER_NAME_SIZE, 0, 0, 0, 0},
    {"commanderStrength", WINDOWS_ONLY_RVA(RVA_COMMANDERS), COMMANDER_STRENGTH_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderGarrisonLeft", WINDOWS_ONLY_RVA(RVA_COMMANDERS), GARRISON_LEFT_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotHomeX",      RVA_PLAYER_HOME_X,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotHomeY",      RVA_PLAYER_HOME_Y,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"squareGems",     RVA_SQUARE_RECORDS + SQUARE_GEMS,  T_U8, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareStrength", 0,                                 SQUARE_STRENGTH_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"squareRememberedStrength", WINDOWS_ONLY_RVA(RVA_SQUARE_RECORDS + SQUARE_AI_STRENGTH), REMEMBERED_STRENGTH_TYPE, PLAYER_SLOTS, 4, 0, 0, 0,
     MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE},
    {"squareArmyStationary", 0,                           ARMY_STATIONARY_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"squareArmyHolds", 0,                                ARMY_HOLDS_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"unitStrength",   0,                                 UNIT_STRENGTH_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, UNIT_TABLE_MAX, 0},
    {"unitArmor",      WINDOWS_ONLY_RVA(RVA_UNITS),       UNIT_ARMOR_TYPE, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitVisibleTo",  0,                                 UNIT_VISIBLE_TYPE, UNIT_TABLE_MAX, 0, 0, 0, 0, PLAYER_SLOTS, 0},
    {"monsterAttackType", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS), MONSTER_ATTACK_TYPE, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackWeapon", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_WEAPON), MONSTER_ATTACK_TYPE,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackValue", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_VALUE), MONSTER_ATTACK_TYPE,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"weaponDmgType",  WINDOWS_ONLY_RVA(RVA_WEAPON_TABLE + WEAPON_DMGTYPE), WEAPON_NUMBER_TYPE, WEAPON_SLOTS, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"weaponAoe",      WINDOWS_ONLY_RVA(RVA_WEAPON_TABLE + WEAPON_AOE), WEAPON_NUMBER_TYPE, WEAPON_SLOTS, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"squareCitadel",  0,                                 SQUARE_CITADEL_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"unitMoveFlags",  RVA_UNITS,                         T_UNIT_MOVE_FLAGS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitFlags",      RVA_UNITS,                         T_UNIT_FLAG_WORDS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"monsterFlags",   RVA_MONSTER_FLAGS,                 T_I64, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"monsterAbility", RVA_MONSTER_TABLE,                 T_MONSTER_ABILITY, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0,
     RITUAL_EFFECT_ID_COUNT, 0},
    {"monsterMoveFlags", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_MOVE_FLAGS), AI_MOVE_I64_TYPE, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, 0, 0},
    {"commanderMoveFlags", RVA_COMMANDERS,                COMMANDER_MOVE_FLAGS_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitpow", RVA_COMMANDERS,                   COMMANDER_RITPOW_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitualLevel", RVA_COMMANDERS,              COMMANDER_RITUAL_LEVEL_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderSpellLevel", RVA_COMMANDERS,               T_COMMANDER_SPELL_LEVEL, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"terrainFlags",   WINDOWS_ONLY_RVA(RVA_TERRAIN_TABLE + TERRAIN_FLAGS), AI_MOVE_I64_TYPE, TERRAIN_TABLE_MAX, TERRAIN_SIZE, 0, 0, 0, 0, 0},
    {"squareLibraryLevel", 0,                             SQUARE_LIBRARY_LEVEL_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"seasonTurnsLeft", WINDOWS_ONLY_RVA(RVA_SEASON_TURNS_LEFT), AI_MOVE_I16_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivOther", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_OTHER), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivAi",  WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_AI), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivHuman", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_HUMAN), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackMargin", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_MARGIN), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"ceremonyName",   0,                                 CEREMONY_NAME_TYPE, CEREMONY_INDEXES, 0, 0, 0, 0, 0, 0},
    {"ceremonyCost",   0,                                 CEREMONY_COST_TYPE, CEREMONY_INDEXES, 0, 0, 1, SITE_CEREMONY_COSTS, 0, 0},
    {"ceremonyLastIndex", 0,                              CEREMONY_LAST_TYPE, 1, 0, 0, 0, 0, 0, 0},
    // The terrain word for fort part bit n (walls, irongate, ...), as the game's terrain reader has it.
    {"fortPartWord",   0,                                 T_FORT_PART_WORD, FORT_PART_BITS, 0, 0, 0, 0, 0, 0},
};
static const GameVarShape GAME_VAR_SHAPES[] = {
    {"classRecruitName", 0, 0, LIST_TO_EMPTY_NAME, RECRUIT_MONSTER_NAME, 0, 0},
    {"classRecruitCount", 0, 0, LIST_TO_EMPTY_NAME, RECRUIT_COUNT, 0, 0},
    {"classStartArmy", 0, 0, LIST_TO_EMPTY_NAME, 0, 0, 0},
    {"squareSeen", MAP_COLUMN_SQUARES, SQUARE_SIZE, LIST_NONE, 0, 0, 0},
    {"commanderRitual", 0, 0, LIST_TO_BELOW_ONE, 0, 0, 0},
    {"ritualString", 0, 0, LIST_TO_EMPTY_NAME, 0, 0, 0},
    {"ritualCostType", 0, 0, LIST_TO_BELOW_ZERO_I32, 0, 0, 0},
    {"ritualCost", 0, 0, LIST_TO_BELOW_ZERO_I32, 4, 0, 0},
    {"ritualEffect", 0, 0, LIST_TO_BELOW_ONE_I32, 0, 0, 0},
    {"ritualEffectValue", 0, 0, LIST_TO_BELOW_ONE_I32, RITUAL_FX_VALUE, 0, 0},
    {"ritualTerrainAt", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 0},
    {"ritualEffectMet", COMMANDER_TABLE_MAX, 0, LIST_NONE, 0, 0, 0},
    {"planeName", 0, 0, LIST_NONE, 0, RVA_LAST_PLANE, 0},
    {"squareGems", SQUARE_GEM_KINDS, 1, LIST_NONE, 0, 0, 0},
    {"squareStrength", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
    {"squareRememberedStrength", MAP_COLUMN_SQUARES, SQUARE_SIZE, LIST_NONE, 0, 0, 0},
    {"unitStrength", 0, 0, LIST_NONE, 0, 0, 1},
    {"squareCitadel", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
};
// Variable v's GameVarShape, or one with no third index and no list.
static GameVarShape gameVarShape(const GameVar *v) {
    for (size_t i = 0; i < sizeof GAME_VAR_SHAPES / sizeof GAME_VAR_SHAPES[0]; i++)
        if (!strcmp(GAME_VAR_SHAPES[i].name, v->name)) return GAME_VAR_SHAPES[i];
    return (GameVarShape){v->name, 0, 0, LIST_NONE, 0, 0, 0};
}
static int luaSiteDepth[SITE_COUNT];

// Returns a site of the same game function as `site` whose Lua wrap runs now, or SITE_COUNT.
static CallSite runningFrameSite(CallSite site) {
    for (size_t i = 0; i < sizeof FRAME_SITES / sizeof FRAME_SITES[0]; i++) {
        int inGroup = 0;
        for (size_t k = 0; k < FRAME_SITES[i].count; k++) inGroup |= FRAME_SITES[i].sites[k] == site;
        if (!inGroup) continue;
        for (size_t k = 0; k < FRAME_SITES[i].count; k++)
            if (luaSiteDepth[FRAME_SITES[i].sites[k]]) return FRAME_SITES[i].sites[k];
    }
    return SITE_COUNT;
}

// Finds the variable named by argument 1 of coe5.get or coe5.set, and returns its address.
static unsigned char *gameVarAddress(lua_State *L, const GameVar **out, int *arg) {
    const char *name = luaL_checkstring(L, 1);
    const GameVar *v = NULL;
    for (size_t i = 0; i < sizeof GAME_VARS / sizeof GAME_VARS[0]; i++) if (!strcmp(GAME_VARS[i].name, name)) v = &GAME_VARS[i];
    if (!v) { luaL_error(L, "unknown game variable '%s'", name); return NULL; }
    int isRitualTable = v->count == COUNT_AT_CALL && v->addr >= RVA_RITUAL_TABLE && v->addr < RVA_RITUAL_TABLE + RITUAL_SIZE;
    lua_Integer index = 0;
    int count = isRitualTable ? ritualSlotCount() : v->count;
    GameVarShape shape = gameVarShape(v);
    if (count > 1) {
        int first = shape.everyPlayer ? -1 : 0;
        index = luaL_checkinteger(L, (*arg)++);
        if (index < first || index >= count) luaL_error(L, "%s: index %I out of range %d-%d", name, (LUAI_UACINT)index, first, count - 1);
    }
    lua_Integer index2 = 0, index3 = 0;
    int count2 = v->type == T_CAST_COUNT ? ritualSlotCount() : v->count2;
    if (count2 > 1) {
        index2 = luaL_checkinteger(L, (*arg)++);
        if (index2 < 0 || index2 >= count2) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index2, count2 - 1);
    }
    if (shape.count3 > 1) {
        index3 = luaL_checkinteger(L, (*arg)++);
        if (index3 < 0 || index3 >= shape.count3) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index3, shape.count3 - 1);
    }
    *out = v;
    if (v->type == T_FRAME_I32) {
        CallSite running = runningFrameSite(v->site);
        uintptr_t sp = running != SITE_COUNT ? luaApi->callerRsp(running) : 0;
        return sp ? (unsigned char *)(sp + v->addr) : NULL;
    }
    if ((v->type == T_SITE_I32 || v->type == T_CEREMONY_COST) && !luaSiteDepth[v->site]) return NULL;
    unsigned char *table = v->type == T_CAST_COUNT ? castCountsBase()
                         : gameBase + (isRitualTable ? ritualTableRva() + (v->addr - RVA_RITUAL_TABLE) : v->addr);
    if (shape.lastAt && index > G32(shape.lastAt)) return NULL;
    unsigned char *entries = table + (size_t)index * v->stride;
    for (lua_Integer k = 0; shape.list != LIST_NONE && k <= index2; k++) {
        const unsigned char *entry = entries - shape.nameBack + (size_t)k * v->stride2;
        int ended = shape.list == LIST_TO_BELOW_ONE       ? *(const short *)entry < 1
                    : shape.list == LIST_TO_BELOW_ONE_I32  ? *(const int *)entry < 1
                    : shape.list == LIST_TO_BELOW_ZERO_I32 ? *(const int *)entry < 0
                                                           : !*entry;
        if (ended) return NULL;
    }
    return entries + (size_t)index2 * v->stride2 + (size_t)index3 * shape.stride3;
}

// The player whose army stands on square (x, y), or -1.
static int armyOwnerAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return -1;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    if (!G16(RVA_SQUARE_ARMY + square * 2)) return -1;
    return G16(RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_ARMY_OWNER);
}

// A unit's action points left this turn.
static int unitApLeft(int unit) {
    int max = 0;
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) >= 0) {
        unsigned long long flags = GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(unit);
        max = flags & UNIT_FLAG_FAST ? 4 : flags & UNIT_FLAG_SLOW ? 2 : 3;
    }
    return max - G16(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_AP_USED);
}

// 1 when the unit never moves on the world map.
static int unitStationary(int unit) {
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) < 0) return 0;
    return (GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(unit) >> MONSTER_FLAG_NO_COMMANDER) & 1;
}

// The corpses on square (x, y).
static int corpsesAt(int x, int y, int humanoidOnly) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    int count = 0;
    for (int kind = 0; kind < SQUARE_CORPSE_KINDS; kind++)
        if (!humanoidOnly || kind < 3 || kind > 5) count += G16(RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_CORPSES + (size_t)kind * 2);
    return count;
}

// What the owner of commander `commander`'s unit has of cost type `type` at the unit's square.
static int commanderCostAvailable(int commander, int type, int *have) {
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return -1;
    int owner = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
    if (owner < 0 || owner >= PLAYER_SLOTS) return -1;
    int x = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE);
    if (type == 14 || type == 17) *have = corpsesAt(x, y, type == 14);
    else *have = GAME_FN(int (*)(int, int, int, int), RVA_RESOURCE_AVAILABLE)(owner, type, x, y);
    return 0;
}

// The .c5m word of ritual effect `id`, or NULL for an id the list lacks.
static const char *ritualEffectWord(int id) {
    for (size_t i = 0; i < sizeof RITUAL_EFFECT_NAMES / sizeof RITUAL_EFFECT_NAMES[0]; i++)
        if (RITUAL_EFFECT_NAMES[i].id == id) return RITUAL_EFFECT_NAMES[i].name;
    return NULL;
}

// The game's name for a terrain code.
static void pushGameTerrainName(lua_State *L, int code) {
    GAME_FN(void (*)(int), RVA_TERRAIN_NAME)(code);
    const char *text = (const char *)(gameBase + RVA_TERRAIN_NAME_TEXT);
    lua_pushlstring(L, text, strnlen(text, TERRAIN_NAME_TEXT_SIZE));
}

// Whether square (x, y) counts for a ritual's terrain `code`.
static int ritualTerrainAt(int code, int x, int y) {
    if (code == -1) return 1;
    if (code >= 0 && code < TERRAIN_TABLE_MAX) {
        int here = G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE);
        if (here < 0 || here >= TERRAIN_TABLE_MAX) return 0;
        const char *need = (const char *)(gameBase + RVA_TERRAIN_TABLE + (size_t)code * TERRAIN_SIZE);
        const char *have = (const char *)(gameBase + RVA_TERRAIN_TABLE + (size_t)here * TERRAIN_SIZE);
        return strncmp(need, have, TERRAIN_NAME_SIZE) == 0;
    }
    if (code < -1 && code >= TERRAIN_MATCH_CODE_MIN) return GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH)(code, x, y) != 0;
    return -1;
}

// The terrain code of ritual effect `fx` when it is a nearby or nonearby rule.
static int ritualNearbyRule(const unsigned char *fx, int *code) {
    int rule = *(const int *)fx - RITUAL_FX_NONEARBY1REQ;
    if (rule < 0 || rule >= 10) return -1;
    long long value = *(const long long *)(fx + RITUAL_FX_VALUE);
    if (!((value >= TERRAIN_MATCH_CODE_MIN && value < TERRAIN_TABLE_MAX) || (value >= 8001 && value <= 11999))) return -1;
    *code = (int)value;
    return rule;
}

// The game's ability total of a unit.
static int unitAbilityTotal(int unit, int ability, int add) {
    return GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL)(unit, ability, add, NULL, 0, NULL);
}

// The game's strength of a commander's army.
static int commanderStrength(int commander) {
#ifdef __APPLE__
    (void)commander;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_COMMANDER_FLAGGED_STRENGTH)(commander, 0);
#endif
}

// The strength of the units on square (x, y) of player `team` and its teammates.
static int squareStrength(int team, int x, int y) {
#ifdef __APPLE__
    (void)team, (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int, int), RVA_SQUARE_STRENGTH)(team, x, y);
#endif
}

// 1 when the game never moves the army on square (x, y).
static int armyStationaryAt(int x, int y) {
#ifdef __APPLE__
    (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_SQUARE_ARMY_STATIONARY)(x, y);
#endif
}

// 1 when the independents' move never moves the army on square (x, y).
static int holdFlagsHold(int squareFlags, int top, unsigned long long topFlags) {
    if (squareFlags & SQUARE_FLAG_HOLDIT) return 1;
    return top >= 0 && ((topFlags >> UNIT_FLAG2_HOLDIT) & 1);
}

static int armyHoldsAt(int x, int y) {
#ifdef __APPLE__
    (void)x, (void)y;
    return holdFlagsHold(0, -1, 0);
#else
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    int squareFlags = G32(RVA_SQUARE_FLAGS + square);
    if (holdFlagsHold(squareFlags, -1, 0)) return 1;
    int owner = armyOwnerAt(x, y);
    if (owner < 0) return 0;
    int top = GAME_FN(int (*)(int, int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(owner, -1, x, y, -1, -1, NULL);
    if (top < 0 || top >= UNIT_TABLE_MAX) return 0;
    return holdFlagsHold(squareFlags, top, *(const unsigned long long *)GAME_PTR(RVA_UNITS + (size_t)top * UNIT_SIZE + 8));
#endif
}

// 1 when unit u stands on square (x, y) alive.
static int unitAliveAt(int u, int x, int y) {
    if (u < 0 || u >= UNIT_TABLE_MAX || x < 0) return 0;
    size_t at = (size_t)u * UNIT_SIZE;
    return (short)G16(RVA_UNIT_HP + at) > 0 && G32(RVA_UNIT_X + at) == x && G32(RVA_UNIT_X + 4 + at) == y;
}

// 1 when unit u is in the game's unit table with hp above 0.
static int unitAliveInTable(int u) {
    return u >= 0 && u < UNIT_TABLE_MAX && u <= G32(RVA_UNIT_COUNT) && (short)G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) > 0;
}

// A unit's strength as player `player` counts it.
static int unitStrengthFor(int player, int unit) {
    if (!unitAliveInTable(unit)) return 0;
#ifdef __APPLE__
    (void)player;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_UNIT_STRENGTH_FOR)(player, unit);
#endif
}

// A living unit's armor as the unit screen shows it, or -1.
static int unitArmorNow(int unit) {
    if (!unitAliveInTable(unit)) return -1;
#ifdef __APPLE__
    return -1;
#else
    return GAME_FN(int (*)(int), RVA_UNIT_ARMOR)(unit);
#endif
}

// Whether player `player` sees unit `unit`.
static int unitVisibleToNow(int unit, int player) {
    if (!unitAliveInTable(unit)) return 0;
#ifdef __APPLE__
    (void)player;
    return 0;
#else
    size_t at = (size_t)unit * UNIT_SIZE;
    return GAME_FN(int (*)(int, int, int, int), RVA_UNIT_VISIBLE_TO)(player, unit, G32(RVA_UNIT_X + at), G32(RVA_UNIT_X + 4 + at)) != 0;
#endif
}

// The int at byte `field` of row `row` of a monster's attack list.
static int monsterAttackField(const unsigned char *rows, int row, int field, int *value) {
    for (int k = 0; k <= row && k < MONSTER_ATTACK_MAX; k++) {
        const unsigned char *r = rows + (size_t)k * MONSTER_ATTACK_SIZE;
        if (*(const int *)r < 1 || *(const int *)(r + MONSTER_ATTACK_WEAPON) < 0) return -1;
    }
    if (row < 0 || row >= MONSTER_ATTACK_MAX) return -1;
    *value = *(const int *)(rows + (size_t)row * MONSTER_ATTACK_SIZE + field);
    return 0;
}

// 1 when square (x, y) is a citadel for `player`.
static int squareCitadelFor(int player, int x, int y) {
#ifdef __APPLE__
    (void)player, (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y);
#endif
}

// ---- Move flags, flag words, ritual and spell levels, library levels ----------------------------
// A unit's move flags or flag words, into *out.
static int unitFlagsOf(int unit, int words, unsigned long long *out) {
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) < 0) return -1;
    *out = GAME_FN(unsigned long long (*)(int), words ? RVA_UNIT_RESISTANCES : RVA_UNIT_FLAGS)(unit);
    return 0;
}

// The commander's unit when it is in the table and alive, else -1.
static int livingCommanderUnit(int commander) {
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) return -1;
    return unit;
}

// The move flags of the commander's army as the AI's routes test them.
static unsigned long long commanderMoveFlags(int commander) {
#ifdef __APPLE__
    (void)commander;
    return 0;
#else
    return GAME_FN(unsigned long long (*)(int), RVA_ARMY_MOVE_FLAGS)(commander);
#endif
}
static int commanderRitpowOf(int commander, int *level) {
    *level = 0;
#ifdef __APPLE__
    (void)commander;
    return 0;
#else
    return GAME_FN(int (*)(int, int *), RVA_COMMANDER_RITPOW)(commander, level);
#endif
}

// Square (x, y)'s library level as the AI's conquest scores it.
static int squareLibraryLevel(int x, int y) {
#ifdef __APPLE__
    (void)x, (void)y;
    return 0;
#else
    if (x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    return GAME_FN(int (*)(int, int), RVA_SQUARE_LIBRARY_LEVEL)(x, y);
#endif
}

// ---- Incomes: slotIncome and terrainIncome ---------------------------------------------------------
// The field of a terrain record that holds its income of `resource`, or -1.
static int terrainIncomeField(int resource) {
    static const int FIELDS[RESOURCE_COUNT] = {TERRAIN_GOLD,  TERRAIN_IRON, TERRAIN_HERBS, TERRAIN_FUNGUS, TERRAIN_SACRIFICES,
                                               TERRAIN_HANDS, TERRAIN_WEED, -1, -1, -1, -1, TERRAIN_TRADE, -1, -1};
    return resource >= 0 && resource < RESOURCE_COUNT ? FIELDS[resource] : -1;
}

// The percent player_turn_start pays a player's income of `resource` at.
static int incomePercent(int controller, int bonus, int resource, int anySpecial) {
    if (controller == CONTROLLER_HUMAN) return 100;
    if (controller != CONTROLLER_AI) return 0;
    int quarter = bonus > 0 ? bonus / 4 : 0;
    if (resource != 0 && resource != 1 && resource != 11) return 100 + bonus + quarter;
    return 100 + bonus - (anySpecial ? quarter : 0);
}

// An income paid at `percent`, rounded as player_turn_start rounds it.
static int scaledIncome(int raw, int percent) {
    return (int)(((long long)raw * percent + 50) / 100);
}

// What player `slot` gets of `resource` at its next turn start.
#ifndef __APPLE__
static int playerGathersSpecial(int slot, int resource) {
    switch (resource) {
    case 2: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_HERBS)(slot) != 0;
    case 3: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_FUNGUS)(slot) != 0;
    case 4: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_SACRIFICES)(slot) != 0;
    case 5: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_HANDS)(slot) != 0;
    case 6: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_WEED)(slot) != 0;
    case 7: case 8: case 9: case 10:
        return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_GEMS)(slot) != 0 || GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_ANY_GEMS)(slot) != 0;
    case 12: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_RELICS)(slot) != 0;
    case 13: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_LIFEFORCE)(slot) != 0;
    default: return 1;
    }
}
static int rawIncome(int slot, int resource) {
    switch (resource) {
    case 0: return GAME_FN(int (*)(int, int *, int), RVA_PLAYER_GOLD_INCOME)(slot, NULL, 0);
    case 1: return GAME_FN(int (*)(int), RVA_PLAYER_IRON_INCOME)(slot);
    case 2: return GAME_FN(int (*)(int), RVA_PLAYER_HERBS_INCOME)(slot);
    case 3: return GAME_FN(int (*)(int), RVA_PLAYER_FUNGUS_INCOME)(slot);
    case 4: return GAME_FN(int (*)(int), RVA_PLAYER_SACRIFICES_INCOME)(slot);
    case 5: return GAME_FN(int (*)(int), RVA_PLAYER_HANDS_INCOME)(slot);
    case 6: return GAME_FN(int (*)(int), RVA_PLAYER_WEED_INCOME)(slot);
    case 11: return GAME_FN(int (*)(int), RVA_PLAYER_TRADE_INCOME)(slot);
    case 12: return GAME_FN(int (*)(int), RVA_PLAYER_RELICS_INCOME)(slot);
    case 13: return GAME_FN(int (*)(int, int), RVA_PLAYER_LIFEFORCE_INCOME)(slot, 0);
    default: return GAME_FN(int (*)(int, int), RVA_PLAYER_GEM_INCOME)(resource - 7, slot);
    }
}
#endif
static int slotIncome(int slot, int resource) {
    int controller = G16(RVA_SLOT_CONTROLLER + (size_t)slot * 2);
    int level = G16(RVA_SLOT_AI + (size_t)slot * PLAYER_BLOCK_SIZE);
    int bonus = level >= 0 && level < AI_LEVEL_TABLE_MAX ? G32(RVA_AI_LEVEL_BONUS + (size_t)level * 4) : 0;
    int percent = incomePercent(controller, bonus, resource, 0);
#ifdef __APPLE__
    return scaledIncome(0, percent);
#else
    if (percent == 0 || !playerGathersSpecial(slot, resource)) return 0;
    if (controller == CONTROLLER_AI && (resource == 0 || resource == 1 || resource == 11)) {
        static const int SPECIAL_CHECKS[] = {3, 2, 6, 13, 4, 12, 5, 7};
        int anySpecial = 0;
        for (size_t i = 0; i < sizeof SPECIAL_CHECKS / sizeof SPECIAL_CHECKS[0] && !anySpecial; i++)
            anySpecial = playerGathersSpecial(slot, SPECIAL_CHECKS[i]);
        percent = incomePercent(controller, bonus, resource, anySpecial);
    }
    return scaledIncome(rawIncome(slot, resource), percent);
#endif
}

// ---- What stays home: commanderGarrisonLeft --------------------------------------------------------
// The strength that stays on a commander's square when it leaves with its army.
#ifndef __APPLE__
enum { GARRISON_GAME, GARRISON_JOINED, GARRISON_DROPPED };
static unsigned char garrisonState[UNIT_TABLE_MAX];
static unsigned long long garrisonUnitFlags[UNIT_TABLE_MAX];

// Unit u's leader after the mirrored joins and drops of commander c's gather.
static int garrisonLeader(int c, int u) {
    if (garrisonState[u] == GARRISON_JOINED) return c;
    if (garrisonState[u] == GARRISON_DROPPED) return -1;
    return G32(RVA_UNIT_LEADER + (size_t)u * UNIT_SIZE);
}
static int garrisonUnitStrength(int u) { return GAME_FN(int (*)(int), RVA_UNIT_STRENGTH)(u); }
static unsigned long long garrisonUnitFlagsOf(int u) { return GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(u); }

// RVA_COMMANDER_STRENGTH (flags 0) or RVA_COMMANDER_FLAGGED_STRENGTH, on the mirrored leaders.
static int garrisonArmyStrength(int c, int last, unsigned long long flags) {
    int sum = 0;
    for (int u = 0; u <= last; u++)
        if (garrisonLeader(c, u) == c && (garrisonUnitFlagsOf(u) & flags) == flags) sum += garrisonUnitStrength(u);
    int unit = c <= G32(RVA_LAST_COMMANDER) ? G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) : -1;
    return sum + garrisonUnitStrength(unit);
}

// RVA_AI_REFORMS_HERE with what it changes put back: the random sequence and each unit's int64 at +8.
static int garrisonReformsHere(int player, int x, int y, int last) {
    int randomIndex = G32(RVA_RANDOM_INDEX), randomShift = G32(RVA_RANDOM_SHIFT), randomXor = G32(RVA_RANDOM_XOR);
    for (int u = 0; u <= last; u++) garrisonUnitFlags[u] = *(unsigned long long *)(gameBase + RVA_UNITS + (size_t)u * UNIT_SIZE + 8);
    int here = GAME_FN(int (*)(int, int, int), RVA_AI_REFORMS_HERE)(player, x, y);
    for (int u = 0; u <= last; u++) *(unsigned long long *)(gameBase + RVA_UNITS + (size_t)u * UNIT_SIZE + 8) = garrisonUnitFlags[u];
    G32(RVA_RANDOM_INDEX) = randomIndex, G32(RVA_RANDOM_SHIFT) = randomShift, G32(RVA_RANDOM_XOR) = randomXor;
    return here;
}

// Whether unit u may join in the gather's pick-up passes.
static int garrisonMayJoin(int c, int u, int player, int x, int y, int kind) {
    if (GAME_FN(int (*)(int), RVA_GET_UNIT_X)(u) != x || GAME_FN(int (*)(int), RVA_GET_UNIT_Y)(u) != y) return 0;
    if (GAME_FN(int (*)(int), RVA_GET_UNIT_HP)(u) <= 0 || garrisonLeader(c, u) != -1) return 0;
    if (GAME_FN(int (*)(int), RVA_UNIT_OWNER)(u) != player || GAME_FN(int (*)(int), RVA_UNIT_COMMANDER_INDEX)(u) != -1) return 0;
    return GAME_FN(int (*)(int, int), RVA_AI_UNIT_JOIN_KIND)(c, u) >= kind;
}

// Drops every follower of c for which `drop` holds.
static void garrisonDropFollowers(int c, int last, int (*drop)(int u)) {
    for (int u = 0; u <= last; u++)
        if (garrisonLeader(c, u) == c && drop(u)) garrisonState[u] = GARRISON_DROPPED;
}
static int garrisonHolds(int u) { return unitAbilityTotal(u, 0x34, 0) == 1; }
static int garrisonLacks10(int u) { return !(garrisonUnitFlagsOf(u) & 0x10); }
static int garrisonLacks2(int u) { return !(garrisonUnitFlagsOf(u) & 2); }
static int garrisonHas8(int u) { return (garrisonUnitFlagsOf(u) & 8) != 0; }

// RVA_AI_GATHER_TROOPS for commander c, on the mirrored leaders.
static void garrisonGather(int c, int unit, int player, int x, int y, int last) {
    if (GAME_FN(int (*)(int), RVA_UNIT_NO_COMMANDER_FLAG)(unit) || GAME_FN(int (*)(int), RVA_COMMANDER_NO_GATHER)(c)) return;
    if (*(unsigned long long *)(gameBase + RVA_UNITS + (size_t)unit * UNIT_SIZE) & UNIT_AI_SAILOR) return;
    int terrain = GAME_FN(int (*)(int, int), RVA_SQUARE_TERRAIN)(x, y);
    if (terrain < 0 || terrain >= TERRAIN_TABLE_MAX) return;
    int gold = G16(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_GOLD);
    int citadel = GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y) != 0;
    int keep = 0;
    if (citadel) keep = GAME_FN(int (*)(int), RVA_AI_COUNT_CITADELS)(player) <= 1 ? 60 : 20;
    if (gold >= 4 && keep < 10) keep = 10;
    if (garrisonReformsHere(player, x, y, last) && keep < 50) keep = 50;
    int own = garrisonUnitStrength(unit) + GAME_FN(int (*)(int), RVA_COMMANDER_CASTER_BONUS)(c);
    int savedCount = G32(RVA_FREE_TROOPS_COUNT);
    int free = GAME_FN(int (*)(int, int), RVA_FREE_TROOPS_STRENGTH)(x, y);
    int count = G32(RVA_FREE_TROOPS_COUNT);
    G32(RVA_FREE_TROOPS_COUNT) = savedCount;
    int army = garrisonArmyStrength(c, last, 0);
    if (free > keep && count > 1) {
        int take = (int)((unsigned)(100 - keep) * (unsigned)free) / 100;
        if (take > 0) {
            for (int u = 0; u <= last; u++)
                if (garrisonMayJoin(c, u, player, x, y, 2)) garrisonState[u] = GARRISON_JOINED, take -= garrisonUnitStrength(u), count--;
            for (int r = 0; r <= 3 && r <= last; r++)
                for (int u = r; u <= last && take > 0 && (count > 1 || keep == 0) && count > 0; u += 4)
                    if (garrisonMayJoin(c, u, player, x, y, 1)) garrisonState[u] = GARRISON_JOINED, take -= garrisonUnitStrength(u), count--;
        }
    } else if (count == 0 && army > own && keep) {
        int t = (int)((unsigned)army * (unsigned)keep) / 100;
        for (int start = 0; start <= 1; start++)
            for (int u = start; u <= last && t > own; u += 2) {
                if (garrisonLeader(c, u) != c || GAME_FN(int (*)(int), RVA_GET_UNIT_HP)(u) <= 0) continue;
                int strength = garrisonUnitStrength(u);
                if (t - strength < own || !GAME_FN(int (*)(int), RVA_AI_UNIT_MAY_GUARD)(u)) continue;
                garrisonState[u] = GARRISON_DROPPED;
                t -= strength;
            }
    }
    if (GAME_FN(int (*)(int, int), RVA_SQUARE_HAS_FORT)(x, y)) garrisonDropFollowers(c, last, garrisonHolds);
    if (gold <= 1 && !citadel) return;
    unsigned long long f = garrisonUnitFlagsOf(unit);
    if (f & 0x10) {
        army = garrisonArmyStrength(c, last, 0);
        if (army - garrisonArmyStrength(c, last, 0x10) < army / 2) garrisonDropFollowers(c, last, garrisonLacks10);
    }
    if (f & 2) {
        army = garrisonArmyStrength(c, last, 0);
        if (army - garrisonArmyStrength(c, last, 2) < army / 4) garrisonDropFollowers(c, last, garrisonLacks2);
    }
    if (!(f & 8)) {
        army = garrisonArmyStrength(c, last, 0);
        if (garrisonArmyStrength(c, last, 8) < army / 10) garrisonDropFollowers(c, last, garrisonHas8);
    }
}

// Whether unit u leaves with commander c.
static int garrisonLeaves(int c, int cUnit, int u) {
    if (u == cUnit) return 1;
    int leader = garrisonLeader(c, u);
    for (int depth = 0; depth < 8 && leader >= 0 && leader < COMMANDER_TABLE_MAX; depth++) {
        if (leader == c) return 1;
        int leaderUnit = G32(RVA_COMMANDERS + (size_t)leader * COMMANDER_SIZE);
        if (leaderUnit < 0 || leaderUnit >= UNIT_TABLE_MAX) return 0;
        leader = garrisonLeader(c, leaderUnit);
    }
    return 0;
}
#endif

// The strength that stays on commander c's square when c leaves, or -1.
static int commanderGarrisonLeft(int c) {
#ifdef __APPLE__
    (void)c;
    return -1;
#else
    int unit = G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) return -1;
    int player = GAME_FN(int (*)(int), RVA_UNIT_OWNER)(unit);
    int x = -1, y = -1;
    GAME_FN(void (*)(int, int *, int *), RVA_UNIT_SQUARE)(unit, &x, &y);
    if (player < 0 || player >= PLAYER_SLOTS || x < 0 || y < 0) return -1;
    int last = G32(RVA_UNIT_COUNT);
    if (last >= UNIT_TABLE_MAX) last = UNIT_TABLE_MAX - 1;
    memset(garrisonState, GARRISON_GAME, sizeof garrisonState);
    garrisonGather(c, unit, player, x, y, last);
    long long sum = 0;
    int count = 0, ranged = 0;
    for (int u = 0; u <= last; u++) {
        if (!GAME_FN(int (*)(int, int, int), RVA_UNIT_ALIVE_AT)(u, x, y)) continue;
        if (!GAME_FN(int (*)(int, int), RVA_SAME_TEAM)(player, GAME_FN(int (*)(int), RVA_UNIT_OWNER)(u))) continue;
        if (garrisonLeaves(c, unit, u)) continue;
        sum += GAME_FN(int (*)(int, int), RVA_UNIT_STRENGTH_FOR)(player, u);
        count++;
        ranged += GAME_FN(int (*)(int, int), RVA_UNIT_HAS_RANGED)(u, 3) != 0;
    }
    if (ranged > count / 10) sum = sum * (100 + 60 * GAME_FN(int (*)(int, int), RVA_SQUARE_FORT_LEVEL)(x, y)) / 100;
    return sum < INT_MAX ? (int)sum : INT_MAX;
#endif
}

// A player's remembered strength of square (x, y).
static int rememberedStrength(const int *memo, int x, int y, int (*measure)(int, int, int)) {
    if (x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    return *memo >= 0 ? *memo : measure(-1, x, y);
}

// Where a ritual's rules are judged.
typedef struct {
    const unsigned char *record;
    int commander, unit, player, x, y;
} RitualCaster;

// Effect i's value as the game's effect lookups read it: the int64 cut to an int.
static int ritualFxValue(const unsigned char *record, int i) {
    return (int)*(const long long *)(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE + RITUAL_FX_VALUE);
}
static int ritualFxId(const unsigned char *record, int i) { return *(const int *)(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE); }

// The value of the record's first effect `id`, or `none` without one.
static int ritualFirstFx(const unsigned char *record, int id, int none) {
    for (int i = 0; i < RITUAL_FX_MAX && ritualFxId(record, i) >= 1; i++)
        if (ritualFxId(record, i) == id) return ritualFxValue(record, i);
    return none;
}

// One RITUAL_STRINGS line as the game reads it.
static void ritualStringDecode(const char *line, char *name, size_t size, int *kind) {
    char text[RITUAL_STRING_SIZE + 1];
    size_t len = strnlen(line, RITUAL_STRING_SIZE);
    memcpy(text, line, len);
    text[len] = 0;
    *kind = 0;
    if (text[0] == '|') return;
    if (text[0] == '(') {
        static const char MARKS[] = "-+!&|";
        const char *mark = text[1] ? strchr(MARKS, text[1]) : NULL;
        if (mark && text[2] == ')') *kind = (int)(mark - MARKS) + 1;
        char *close = strchr(text, ')');
        if (close) memmove(text, close + 1, strlen(close + 1) + 1);
    }
    char *amp = strstr(text, " & ");
    if (amp) *amp = 0;
    const char *star = strchr(text, '*');
    snprintf(name, size, "%.*s", (int)(size - 1), star ? star + 1 : text);
}

// How many living units named `name` have their home at (x, y).
static int monstersHomeAt(int x, int y, const char *name) {
    int count = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        int monster = G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE);
        if (G16(RVA_UNIT_HOME_X + (size_t)u * UNIT_SIZE) != x || G16(RVA_UNIT_HOME_X + 2 + (size_t)u * UNIT_SIZE) != y) continue;
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) < 1 || monster < 0 || monster >= MONSTER_TABLE_MAX) continue;
        count += !strncmp((const char *)(gameBase + RVA_MONSTER_TABLE + (size_t)monster * MONSTER_SIZE), name, MONSTER_NAME_SIZE);
    }
    return count;
}

// How the unit rules count a line's monster.
enum { COUNT_HERE, COUNT_HOME_HERE, COUNT_WORLD, COUNT_PLAYER, COUNT_ELYSIUM };
static int ritualMonsterCount(const RitualCaster *c, int how, const char *name) {
    int (*at)(int, int, const char *) = GAME_FN(int (*)(int, int, const char *), RVA_COUNT_MONSTERS_AT);
    int (*plane)(int, const char *) = GAME_FN(int (*)(int, const char *), RVA_COUNT_MONSTERS_ON_PLANE);
    switch (how) {
    case COUNT_HERE: return at(c->x, c->y, name);
    case COUNT_HOME_HERE: return monstersHomeAt(c->x, c->y, name);
    case COUNT_WORLD: return at(-1, -1, name);
    case COUNT_PLAYER: return GAME_FN(int (*)(int, const char *), RVA_COUNT_PLAYER_MONSTERS)(c->player, name);
    default: return plane(0, name) + plane(2, name) + plane(3, name);
    }
}

// The unit rules.
static int ritualUnitRuleMet(const RitualCaster *c, int id, int value) {
    const char *strings = (const char *)c->record + RITUAL_STRINGS;
    char name[100] = "";
    int how, count = 0;
    int casterMonster = G16(RVA_UNIT_MONSTER + (size_t)c->unit * UNIT_SIZE);
    const char *casterName = casterMonster >= 0 && casterMonster < MONSTER_TABLE_MAX
        ? (const char *)(gameBase + RVA_MONSTER_TABLE + (size_t)casterMonster * MONSTER_SIZE) : NULL;
    switch (id) {
    case RITUAL_FX_NOMONREQ: case RITUAL_FX_FEWMONREQ: {
        int limit = id == RITUAL_FX_NOMONREQ ? value - 1 : value;
        if (id == RITUAL_FX_FEWMONREQ && ritualFirstFx(c->record, RITUAL_FX_NOMONREQ, 0) != 0) return 1;
        if (value == 0 || limit < 0) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind != 1 && kind != 2) continue;
            count += ritualMonsterCount(c, COUNT_HERE, name);
            if (count > limit) return kind == 2;
        }
        return 1;
    }
    case RITUAL_FX_MINMONREQ: case RITUAL_FX_MONWORLDREQ: case RITUAL_FX_MONPLAYERREQ:
        how = id == RITUAL_FX_MINMONREQ ? COUNT_HERE : id == RITUAL_FX_MONWORLDREQ ? COUNT_WORLD : COUNT_PLAYER;
        if (value < 1) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind == 4 && (count += ritualMonsterCount(c, how, name)) >= value) return 1;
        }
        return 0;
    case RITUAL_FX_NOMONMASTERYREQ:
        if (value < 1 || unitAbilityTotal(c->unit, *(const int *)(c->record + RITUAL_RITPOW), 0) > *(const int *)(c->record + RITUAL_LEVEL))
            return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind == 5 && casterName && !GAME_FN(int (*)(const char *, const char *), RVA_NAME_COMPARE)(casterName, name)) return 1;
            if ((kind == 1 || kind == 2) && ritualMonsterCount(c, COUNT_PLAYER, name) > 0) return kind == 2;
        }
        return 1;
    default:
        how = id == RITUAL_FX_NOMONHOMEREQ ? COUNT_HOME_HERE : id == RITUAL_FX_NOMONWORLDREQ ? COUNT_WORLD
            : id == RITUAL_FX_NOMONPLAYERREQ ? COUNT_PLAYER : COUNT_ELYSIUM;
        if (value < 1) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (how == COUNT_ELYSIUM) count += ritualMonsterCount(c, how, name);
            if (kind == 5 && casterName && !GAME_FN(int (*)(const char *, const char *), RVA_NAME_COMPARE)(casterName, name)) return 1;
            if (kind != 1 && kind != 2) continue;
            if (how != COUNT_ELYSIUM) count += ritualMonsterCount(c, how, name);
            if (count >= value) return kind == 2;
        }
        return 1;
    }
}

// Whether a unit on (x, y) has an ability total of 1..999999.
static int unitWithAbilityAt(int x, int y, int ability) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    if (!G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2)) return 0;
    int last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        if (G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE) != x || G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE) != y) continue;
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) < 1) continue;
        int total = unitAbilityTotal(u, ability, 1);
        if (total >= 1 && total <= 999999) return 1;
    }
    return 0;
}

// The square's SQUARE_PORTAL, 0 off the world.
static int portalAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    return G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_PORTAL);
}

// The caster's and its square's rules.
static int ritualCasterRuleMet(const RitualCaster *c, int id, int v) {
    switch (id) {
    case RITUAL_FX_LEVELREQ: {
        if (v < 1) return 1;
        if (unitAbilityTotal(c->unit, *(const int *)(c->record + RITUAL_RITPOW), 0) != v) return 0;
        int last = G32(RVA_RITPOW_COUNT);
        for (int a = 501; a <= last && a < RITUAL_EFFECT_ID_COUNT; a++)
            if (unitAbilityTotal(c->unit, a, 0) > v) return 0;
        return 1;
    }
    case RITUAL_FX_RESRESTRICT: case RITUAL_FX_RESREQ: case RITUAL_FX_BENRESTRICT: {
        if (v == 0) return 1;
        unsigned long long bits = GAME_FN(unsigned long long (*)(int), id == RITUAL_FX_BENRESTRICT ? RVA_UNIT_BENEFITS : RVA_UNIT_RESISTANCES)(c->unit);
        unsigned long long want = (unsigned long long)(long long)v;
        return id == RITUAL_FX_RESREQ ? (want & ~bits) == 0 : (bits & want) == 0;
    }
    case RITUAL_FX_FXREQ:
        return v == 0 || GAME_FN(int (*)(int, int, int), RVA_COMMANDER_ABILITY_TOTAL)(c->commander, v, 1) > 0;
    case RITUAL_FX_NOPLANEREQ: return v == 999 || GAME_FN(int (*)(int, int), RVA_PLANE_AT)(c->x, c->y) != v;
    case RITUAL_FX_SEASONREQ: return v < 0 || G16(RVA_SEASON) == v;
    case RITUAL_FX_EVENTVARREQ: return v < 1 || v >= EVENT_VAR_COUNT || G32(RVA_EVENT_VARS + (size_t)v * 4) >= 1;
    case RITUAL_FX_NOEVENTVARREQ: return v < 1 || v >= EVENT_VAR_COUNT || G32(RVA_EVENT_VARS + (size_t)v * 4) <= 0;
    case RITUAL_FX_HASPORTALREQ: return v == 0 || GAME_FN(int (*)(int, int, int), RVA_COUNT_PLAYER_PORTALS)(c->player, -1, 2) > 0;
    case RITUAL_FX_NOPORTALREQ: return v < 1 || portalAt(c->x, c->y) <= 0;
    case RITUAL_FX_ENCHHEREREQ: {
        if (v < 1) return 1;
        int onMap = c->x >= 0 && c->y >= 0 && c->x < MAP_COLUMNS_MAX && c->y < MAP_COLUMN_SQUARES;
        int terrain = onMap ? G16(RVA_SQUARE_RECORDS + ((size_t)c->x * MAP_COLUMN_SQUARES + (size_t)c->y) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE) : -1;
        return (v == 19 && terrain == 282) || GAME_FN(int (*)(int, int, int, int), RVA_ENCHANT_COUNT)(c->player, v, c->x, c->y) > 0;
    }
    case RITUAL_FX_NOFXHEREREQ: return v < 1 || !unitWithAbilityAt(c->x, c->y, v);
    case RITUAL_FX_FORTREQ: case RITUAL_FX_NOFORTREQ: {
        if (v == 0) return 1;
        unsigned parts = GAME_FN(unsigned (*)(int, int), RVA_SQUARE_FORT_PARTS)(c->x, c->y);
        return id == RITUAL_FX_FORTREQ ? (parts & (unsigned)v) == (unsigned)v : (parts & (unsigned)v) == 0;
    }
    case RITUAL_FX_HOMEREQ: {
        int home = G16(RVA_UNIT_HOME_X + (size_t)c->unit * UNIT_SIZE) == c->x && G16(RVA_UNIT_HOME_X + 2 + (size_t)c->unit * UNIT_SIZE) == c->y;
        return v == 0 || (v > 0 ? home : !home);
    }
    case RITUAL_FX_HOMECITREQ: {
        if (v == 0 || c->player < 0 || c->player >= PLAYER_SLOTS) return 1;
        int hx = G16(RVA_PLAYER_HOME_X + (size_t)c->player * 2), hy = G16(RVA_PLAYER_HOME_Y + (size_t)c->player * 2);
        return v < 0 ? !(c->x == hx && c->y == hy) : c->x == hx && c->y != hy;
    }
    case RITUAL_FX_NOMONREQ: case RITUAL_FX_FEWMONREQ: case RITUAL_FX_MINMONREQ: case RITUAL_FX_MONWORLDREQ:
    case RITUAL_FX_MONPLAYERREQ: case RITUAL_FX_NOMONMASTERYREQ: case RITUAL_FX_NOMONHOMEREQ:
    case RITUAL_FX_NOMONWORLDREQ: case RITUAL_FX_NOMONPLAYERREQ: case RITUAL_FX_NOMONELYSIUMREQ:
        return ritualUnitRuleMet(c, id, v);
    case RITUAL_FX_PROMOTION: {
        if (v == 0) return 1;
        const char *strings = (const char *)c->record + RITUAL_STRINGS;
        int (*named)(int, int, int, const char *) = GAME_FN(int (*)(int, int, int, const char *), RVA_COUNT_UNITS_NAMED);
        char line[RITUAL_STRING_SIZE + 1];
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            const char *at = strings + (size_t)i * RITUAL_STRING_SIZE;
            if (!strncmp(at, "(-)", 3)) continue;
            snprintf(line, sizeof line, "%.*s", RITUAL_STRING_SIZE, at);
            if (named(c->player, c->x, c->y, line) > 0) return 1;
            i++;
        }
        return 0;
    }
    default: return -1;
    }
}

// Whether the rule of effect i of a ritual record holds for commander `commander`.
static int ritualEffectMetAt(const unsigned char *record, int i, int commander) {
    static const int RADII[] = {1, 3, 5, 7, 99};
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return -1;
    RitualCaster c = {record, commander, unit, G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE),
                      G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE)};
    if (c.player < 0) return -1;
    int id = ritualFxId(record, i);
    for (int j = 0; j < i; j++)
        if (ritualFxId(record, j) == id) return 1;
    int code, rule = ritualNearbyRule(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE, &code);
    if (rule < 0) return ritualCasterRuleMet(&c, id, ritualFxValue(record, i));
    if (c.x < 0 || c.x >= MAP_COLUMNS_MAX || c.y < 0 || c.y >= MAP_COLUMN_SQUARES) return -1;
    int count = GAME_FN(int (*)(int, int, int, int), RVA_TERRAIN_NEAR_COUNT)(c.x, c.y, code, RADII[rule % 5]);
    return rule < 5 ? count == 0 : count > 0;
}

// Fort part words. The game keeps no table of them: its terrain reader passes each word and its bit
// to RVA_FORT_WORD_MATCH, so they are read from that code, once, and a word a game update adds needs
// no change here.
static const char *fortPartWords[FORT_PART_BITS];
static int fortPartWordsRead;

// The bit number of a value with one bit set, or -1.
static int singleBitIndex(uint32_t value) {
    if (!value || (value & (value - 1))) return -1;
    int n = 0;
    while (!(value & 1)) value >>= 1, n++;
    return n;
}

// Windows: each `mov edx, bit / lea rcx, [rip+word] / mov r8d, 1 / add ebx, eax / call match` in
// code[0..size), which sits at `at`. Returns how many words it stored in words[bit number].
static __attribute__((unused)) int readFortWordsX64(const unsigned char *code, size_t size, uintptr_t at, uintptr_t match, const char **words) {
    static const unsigned char MID[] = {0x41, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x03, 0xd8};
    int found = 0;
    for (size_t i = 20; i + 5 <= size; i++) {
        if (code[i] != 0xe8 || code[i - 20] != 0xba || memcmp(code + i - 15, "\x48\x8d\x0d", 3) || memcmp(code + i - 8, MID, sizeof MID)) continue;
        int32_t rel, disp;
        uint32_t bit;
        memcpy(&rel, code + i + 1, 4);
        if (at + i + 5 + (intptr_t)rel != match) continue;
        memcpy(&bit, code + i - 19, 4);
        memcpy(&disp, code + i - 12, 4);
        int n = singleBitIndex(bit);
        if (n < 0) continue;
        words[n] = (const char *)(at + i - 8 + (intptr_t)disp);
        found++;
    }
    return found;
}

// Mac: each `adrp x0, page / add x0, x0, #low / mov w1, #bit / bl match` in code[0..size), at `at`.
static __attribute__((unused)) int readFortWordsArm64(const unsigned char *code, size_t size, uintptr_t at, uintptr_t match, const char **words) {
    int found = 0;
    for (size_t i = 12; i + 4 <= size; i += 4) {
        uint32_t adrp, add, mov, bl;
        memcpy(&adrp, code + i - 12, 4);
        memcpy(&add, code + i - 8, 4);
        memcpy(&mov, code + i - 4, 4);
        memcpy(&bl, code + i, 4);
        if ((bl & 0xfc000000) != 0x94000000 || (adrp & 0x9f00001f) != 0x90000000 || (add & 0xffc003ff) != 0x91000000 ||
            (mov & 0xff80001f) != 0x52800001) continue;
        int32_t offset = (int32_t)(bl << 6) >> 4;
        if (at + i + (intptr_t)offset != match) continue;
        int64_t pageImm = (int64_t)(((uint64_t)((adrp >> 5) & 0x7ffff) << 2 | ((adrp >> 29) & 3)) << 43) >> 31;
        uintptr_t page = ((at + i - 12) & ~(uintptr_t)0xfff) + (intptr_t)pageImm;
        int n = singleBitIndex(((mov >> 5) & 0xffff) << (16 * ((mov >> 21) & 3)));
        if (n < 0) continue;
        words[n] = (const char *)(page + ((add >> 10) & 0xfff));
        found++;
    }
    return found;
}

// The terrain word for fort part bit `bit`, or NULL for a bit no word sets.
static const char *fortPartWord(int bit) {
    if (!fortPartWordsRead) {
        fortPartWordsRead = 1;
        const unsigned char *code = GAME_PTR(RVA_FORT_WORD_CALLS);
#ifdef __APPLE__
        int found = readFortWordsArm64(code, FORT_WORD_CALLS_SIZE, (uintptr_t)code, (uintptr_t)GAME_PTR(RVA_FORT_WORD_MATCH), fortPartWords);
#else
        int found = readFortWordsX64(code, FORT_WORD_CALLS_SIZE, (uintptr_t)code, (uintptr_t)GAME_PTR(RVA_FORT_WORD_MATCH), fortPartWords);
#endif
        writeLog("fort_part_words_read", "found=%d", found);
    }
    return bit >= 0 && bit < FORT_PART_BITS ? fortPartWords[bit] : NULL;
}

// The game's own name for Voice of El ceremony `ceremony`.
static unsigned int ceremonyNameRva(int ceremony) {
    switch (ceremony) {
    case CEREMONY_CONVERT: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_CONVERT);
    case CEREMONY_BREAK_SEAL: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_BREAK_SEAL);
    case CEREMONY_BOLSTER_DEVOTION: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_BOLSTER);
    case CEREMONY_START_CRUSADE: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_CRUSADE);
    case CEREMONY_CALL_INQUISITION: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_INQUISITION);
    case CEREMONY_MASTERY_ARCH_BISHOP:
    case CEREMONY_MASTERY_CARDINAL:
    case CEREMONY_MASTERY_PONTIFF: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_MASTERY);
    case CEREMONY_INCREASE_FAVOR: return WINDOWS_ONLY_RVA(RVA_STR_CEREMONY_FAVOR);
    default: return 0;
    }
}

// @platform-split ceremony_values: Mac keeps the one-table offsets, which only the unit tests reach
// Where ceremony `ceremony`'s cost is, as an offset from the row's address.
#ifdef __APPLE__
#define CEREMONY_COST_ENTRY(name) CEREMONY_COST_##name##_AT
#else
#define CEREMONY_COST_ENTRY(name) (int)RVA_CEREMONY_COST_##name
#endif
static int ceremonyCostAt(int ceremony, int *also) {
    *also = -1;
    switch (ceremony) {
    case CEREMONY_CONVERT: return CEREMONY_COST_ENTRY(CONVERT);
    case CEREMONY_BREAK_SEAL: *also = CEREMONY_COST_ENTRY(SEAL_BASE); return CEREMONY_COST_ENTRY(SEAL_STEP);
    case CEREMONY_BOLSTER_DEVOTION: *also = CEREMONY_COST_ENTRY(BOLSTER_BASE); return CEREMONY_COST_ENTRY(BOLSTER_STEP);
    case CEREMONY_START_CRUSADE: return CEREMONY_COST_ENTRY(CRUSADE);
    case CEREMONY_CALL_INQUISITION: return CEREMONY_COST_ENTRY(INQUISITION);
    case CEREMONY_MASTERY_ARCH_BISHOP: return CEREMONY_COST_ENTRY(ARCH_BISHOP);
    case CEREMONY_MASTERY_CARDINAL: return CEREMONY_COST_ENTRY(CARDINAL);
    case CEREMONY_MASTERY_PONTIFF: return CEREMONY_COST_ENTRY(PONTIFF);
    case CEREMONY_INCREASE_FAVOR: return CEREMONY_COST_ENTRY(FAVOR_STEP);
    default: return -1;
    }
}
// The most a script may set a ceremony's cost to.
#define CEREMONY_COST_MAX 100000

// coe5.set("ceremonyCost", ceremony, value).
static int setCeremonyCost(lua_State *L, unsigned char *table, int arg) {
    lua_Integer ceremony = lua_tointeger(L, 2), value = luaL_checkinteger(L, arg);
    int also, at = ceremonyCostAt((int)ceremony, &also);
    LuaScript *s = luaScriptOf(L);
    if (at < 0) {
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "no_cost", "name=ceremonyCost ceremony=%lld", (long long)ceremony);
        return 0;
    }
    if (value < 0 || value > CEREMONY_COST_MAX) {
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "out_of_range", "name=ceremonyCost ceremony=%lld value=%lld min=0 max=%d",
                      (long long)ceremony, (long long)value, CEREMONY_COST_MAX);
        return 0;
    }
    *(int *)(table + at) = (int)value;
    if (also >= 0) *(int *)(table + also) = (int)value;
    return 0;
}

// coe5.get(name[, index]).
static int lGet(lua_State *L) {
    const GameVar *v;
    int arg = 2;
    unsigned char *p = gameVarAddress(L, &v, &arg);
    if (!p) { lua_pushnil(L); return 1; }
    switch (v->type) {
    case T_ARMY_OWNER: lua_pushinteger(L, armyOwnerAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_RITUAL_SUMMONS: {
        int summons = 0;
        for (int i = 0; i < RITUAL_FX_MAX; i++) summons |= *(const int *)(p + (size_t)i * RITUAL_FX_SIZE) == RITUAL_FX_SUMMONING;
        lua_pushboolean(L, summons);
        break;
    }
    case T_I64: lua_pushinteger(L, (lua_Integer)*(const long long *)p); break;
    case T_TICKS_MS: lua_pushinteger(L, GAME_FN(int (*)(void), RVA_TICKS_MS)()); break;
    case T_EFFECT_NAME: {
        const char *word = ritualEffectWord((int)lua_tointeger(L, 2));
        if (word) lua_pushstring(L, word);
        else lua_pushnil(L);
        break;
    }
    case T_RITUAL_TERRAIN_NAME: pushGameTerrainName(L, *(const int *)p); break;
    case T_RITUAL_TERRAIN_AT: {
        int at = ritualTerrainAt(*(const int *)p, (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4));
        if (at < 0) lua_pushnil(L);
        else lua_pushboolean(L, at);
        break;
    }
    case T_RITUAL_EFFECT_MET: {
        int i = (int)lua_tointeger(L, 3);
        int met = ritualEffectMetAt(p - RITUAL_FX - (size_t)i * RITUAL_FX_SIZE, i, (int)lua_tointeger(L, 4));
        if (met < 0) lua_pushnil(L);
        else lua_pushboolean(L, met);
        break;
    }
    case T_RITUAL_EFFECT_TERRAIN_NAME: {
        int code;
        if (ritualNearbyRule(p, &code) < 0) lua_pushnil(L);
        else pushGameTerrainName(L, code);
        break;
    }
    case T_CAST_COUNT: lua_pushinteger(L, *(const short *)p); break;
    case T_UNIT_AP_LEFT: lua_pushinteger(L, unitApLeft((int)lua_tointeger(L, 2))); break;
    case T_UNIT_STATIONARY: lua_pushboolean(L, unitStationary((int)lua_tointeger(L, 2))); break;
    case T_COMMANDER_COST: {
        int have;
        if (commanderCostAvailable((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), &have) < 0) lua_pushnil(L);
        else lua_pushinteger(L, have);
        break;
    }
    case T_SLOT_INCOME: lua_pushinteger(L, slotIncome((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_TERRAIN_INCOME: {
        int field = terrainIncomeField((int)lua_tointeger(L, 3));
        if (field < 0) lua_pushnil(L);
        else lua_pushinteger(L, *(const short *)(p + field));
        break;
    }
    case T_UNIT_ABILITY: lua_pushinteger(L, unitAbilityTotal((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), 0)); break;
    case T_MONSTER_ABILITY: lua_pushinteger(L, monsterAbility((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_CLASS_STDTROOPS:
        lua_pushboolean(L, GAME_FN(int (*)(int, int, int), RVA_CLASS_ABILITY)((int)lua_tointeger(L, 2), CLASS_ABILITY_STDTROOPS, 0) > 0);
        break;
    case T_CLASS_ABILITY:
        lua_pushinteger(L, GAME_FN(int (*)(int, int, int), RVA_CLASS_ABILITY)((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), 0));
        break;
    case T_U8: lua_pushinteger(L, *p); break;
    case T_I8: lua_pushinteger(L, *(const signed char *)p); break;
    case T_PLANE_AT:
        lua_pushinteger(L, GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)));
        break;
    case T_COMMANDER_STRENGTH: {
        int unit = *(const int *)p;
        if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) lua_pushnil(L);
        else lua_pushinteger(L, commanderStrength((int)lua_tointeger(L, 2)));
        break;
    }
    case T_SQUARE_STRENGTH:
        lua_pushinteger(L, squareStrength((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4)));
        break;
    case T_GARRISON_LEFT: {
        int left = commanderGarrisonLeft((int)lua_tointeger(L, 2));
        if (left < 0) lua_pushnil(L);
        else lua_pushinteger(L, left);
        break;
    }
    case T_ARMY_STATIONARY: lua_pushboolean(L, armyStationaryAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)) != 0); break;
    case T_ARMY_HOLDS: lua_pushboolean(L, armyHoldsAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)) != 0); break;
    case T_UNIT_STRENGTH: lua_pushinteger(L, unitStrengthFor((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_UNIT_VISIBLE: lua_pushboolean(L, unitVisibleToNow((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_UNIT_ARMOR: {
        int armor = unitArmorNow((int)lua_tointeger(L, 2));
        if (armor < 0) lua_pushnil(L);
        else lua_pushinteger(L, armor);
        break;
    }
    case T_MONSTER_ATTACK: {
        int row = (int)lua_tointeger(L, 3), field = (int)(v->addr - (RVA_MONSTER_TABLE + MONSTER_ATTACKS)), value;
        if (monsterAttackField(p - field - (size_t)row * MONSTER_ATTACK_SIZE, row, field, &value) < 0) lua_pushnil(L);
        else lua_pushinteger(L, value);
        break;
    }
    case T_SQUARE_CITADEL:
        lua_pushboolean(L, squareCitadelFor((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4)) != 0);
        break;
    case T_REMEMBERED_STRENGTH:
        lua_pushinteger(L, rememberedStrength((const int *)p, (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4), squareStrength));
        break;
    case T_WINDOWS_ONLY: lua_pushnil(L); break;
    case T_GAME_CHANGED: lua_pushnil(L); break;
    case T_CEREMONY_NAME: {
        unsigned int rva = ceremonyNameRva((int)lua_tointeger(L, 2));
        if (!rva) lua_pushnil(L);
        else lua_pushlstring(L, (const char *)GAME_PTR(rva), strnlen((const char *)GAME_PTR(rva), NAME_SIZE));
        break;
    }
    case T_CEREMONY_COST: {
        int also, at = ceremonyCostAt((int)lua_tointeger(L, 2), &also);
        if (at < 0) lua_pushnil(L);
        else lua_pushinteger(L, *(const int *)(p + at));
        break;
    }
    case T_CEREMONY_LAST: lua_pushinteger(L, CEREMONY_LAST); break;
    case T_FORT_PART_WORD: {
        const char *word = fortPartWord((int)lua_tointeger(L, 2));
        if (!word) lua_pushnil(L);
        else lua_pushstring(L, word);
        break;
    }
    case T_UNIT_MOVE_FLAGS:
    case T_UNIT_FLAG_WORDS: {
        unsigned long long flags;
        if (unitFlagsOf((int)lua_tointeger(L, 2), v->type == T_UNIT_FLAG_WORDS, &flags) < 0) lua_pushnil(L);
        else lua_pushinteger(L, (lua_Integer)flags);
        break;
    }
    case T_COMMANDER_MOVE_FLAGS:
    case T_COMMANDER_RITPOW:
    case T_COMMANDER_RITUAL_LEVEL:
    case T_COMMANDER_SPELL_LEVEL: {
        int commander = (int)lua_tointeger(L, 2), unit = livingCommanderUnit(commander), level;
        if (unit < 0) lua_pushnil(L);
        else if (v->type == T_COMMANDER_MOVE_FLAGS) lua_pushinteger(L, (lua_Integer)commanderMoveFlags(commander));
        else if (v->type == T_COMMANDER_SPELL_LEVEL) lua_pushinteger(L, GAME_FN(int (*)(int, int), RVA_UNIT_PATH_LEVEL)(unit, -1));
        else {
            int school = commanderRitpowOf(commander, &level);
            lua_pushinteger(L, v->type == T_COMMANDER_RITPOW ? school : level);
        }
        break;
    }
    case T_SQUARE_LIBRARY_LEVEL: lua_pushinteger(L, squareLibraryLevel((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_I16: lua_pushinteger(L, *(short *)p); break;
    case T_FLAG_U8: lua_pushboolean(L, *p != 0); break;
    case T_FLAG_POSITIVE: lua_pushboolean(L, *(int *)p > 0); break;
    case T_FLAG_I32: lua_pushboolean(L, *(int *)p != 0); break;
    case T_FLAG_I16: lua_pushboolean(L, *(short *)p != 0); break;
    case T_FLAG_TWO: lua_pushboolean(L, *(int *)p == 2); break;
    case T_F32: lua_pushnumber(L, *(float *)p); break;
    case T_STR: lua_pushlstring(L, (const char *)p, strnlen((const char *)p, v->size)); break;
    case T_PTR_STR: {
        const char *s = *(const char *const *)p;
        if (s) lua_pushlstring(L, s, strnlen(s, v->size));
        else lua_pushnil(L);
        break;
    }
    default: lua_pushinteger(L, *(int *)p);
    }
    return 1;
}

// The values coe5.set takes for a variable the game indexes a fixed table by.
static const struct { const char *name; lua_Integer min, max; } GAME_VAR_RANGES[] = {
    {"mapW", INT_MIN, MAP_COLUMNS_MAX}, {"mapH", INT_MIN, MAP_COLUMN_SQUARES}, {"mapDepth", INT_MIN, MAP_COLUMNS_MAX},
    {"cliMapW", INT_MIN, MAP_COLUMNS_MAX}, {"cliMapH", INT_MIN, MAP_COLUMN_SQUARES},
    {"society", 0, SOCIETY_TABLE_MAX - 1},
    {"slotClass", INT_MIN, CLASS_TABLE_MAX - 1},
    {"slotAi", INT_MIN, AI_LEVEL_TABLE_MAX - 1},
    {"startDefault", 0, 2}, {"northPart", -1, 100}, {"southPart", -1, 100}, {"wilder", 0, 2},
    {"mouseButtons", 0, 7},
    {"traceLevel", 0, 3},
};

// A slot's colour range.
#define SLOT_COLOUR_MIN -8421505.0f
#define SLOT_COLOUR_MAX 8421504.0f

// Returns 1 for a float variable the game keeps within the slot colour's range.
static int isSlotColour(const GameVar *v) {
    return v->type == T_F32 && v->addr >= RVA_SLOT_COLOUR && v->addr <= RVA_SLOT_COLOUR + 8;
}

// The values coe5.set takes for integer variable v.
static void gameVarRange(const GameVar *v, lua_Integer *min, lua_Integer *max) {
    if (v->type == T_U8) *min = SCHAR_MIN, *max = UCHAR_MAX;
    else if (v->type == T_I16) *min = SHRT_MIN, *max = USHRT_MAX;
    else *min = INT_MIN, *max = UINT_MAX;
    for (size_t i = 0; i < sizeof GAME_VAR_RANGES / sizeof GAME_VAR_RANGES[0]; i++) {
        if (strcmp(GAME_VAR_RANGES[i].name, v->name)) continue;
        if (GAME_VAR_RANGES[i].min > *min) *min = GAME_VAR_RANGES[i].min;
        if (GAME_VAR_RANGES[i].max < *max) *max = GAME_VAR_RANGES[i].max;
    }
}

// Resets a commander's cached army action points.
static void flushCommanderCache(int commander) {
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return;
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit >= 0 && unit < UNIT_TABLE_MAX) {
        int leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
        if (leader >= 0 && leader < COMMANDER_TABLE_MAX) {
            *(signed char *)GAME_PTR(RVA_COMMANDERS + (size_t)leader * COMMANDER_SIZE + COMMANDER_AP_CACHE) = -99;
            G32(RVA_COMMANDER_CACHE_INTS + (size_t)leader * 4) = -1;
        }
    }
    *(signed char *)GAME_PTR(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE + COMMANDER_AP_CACHE) = -99;
    G32(RVA_COMMANDER_CACHE_INTS + (size_t)commander * 4) = -1;
}

// After a write of unitApUsed: resets the commander caches the game resets.
static void flushArmyApCaches(int unit) {
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX; c++) {
        if (G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) == unit) {
            flushCommanderCache(c);
            break;
        }
    }
    flushCommanderCache(G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER));
}

// coe5.set(name, [index,] value).
static int lSet(lua_State *L) {
    const GameVar *v;
    int arg = 2;
    unsigned char *p = gameVarAddress(L, &v, &arg);
    if (!v->writable) return luaL_error(L, "game variable '%s' is read-only", v->name);
    if (!p) return luaL_error(L, "%s can only be set while a wrap of its site runs", v->name);
    if (v->type == T_WINDOWS_ONLY) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "windows_only", "name=%s", v->name);
        return 0;
    }
    if (v->type == T_GAME_CHANGED) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "game_changed", "name=%s", v->name);
        return 0;
    }
    if (v->type == T_CEREMONY_COST) return setCeremonyCost(L, p, arg);
    if (v->type == T_U8 || v->type == T_I16 || v->type == T_I32 || v->type == T_FRAME_I32) {
        lua_Integer value = luaL_checkinteger(L, arg), min, max;
        gameVarRange(v, &min, &max);
        if (value < min || value > max) {
            LuaScript *s = luaScriptOf(L);
            refuseLuaCall(s ? s->mod : "?", "coe5.set", "out_of_range", "name=%s value=%lld min=%lld max=%lld", v->name,
                          (long long)value, (long long)min, (long long)max);
            return 0;
        }
    }
    int isFlag = v->type == T_FLAG_U8 || v->type == T_FLAG_I32 || v->type == T_FLAG_I16 || v->type == T_FLAG_TWO;
    if (isFlag && lua_type(L, arg) != LUA_TBOOLEAN) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "needs_boolean", "name=%s value=%s", v->name, luaL_typename(L, arg));
        return 0;
    }
    if (isSlotColour(v)) {
        lua_Number value = luaL_checknumber(L, arg);
        const char *reason = !isfinite(value) ? "not_finite"
                           : value < SLOT_COLOUR_MIN - 1.0 || value > SLOT_COLOUR_MAX + 1.0 || (float)value < SLOT_COLOUR_MIN ||
                                     (float)value > SLOT_COLOUR_MAX
                               ? "out_of_range"
                               : NULL;
        if (reason) {
            LuaScript *s = luaScriptOf(L);
            refuseLuaCall(s ? s->mod : "?", "coe5.set", reason, "name=%s value=%.14g min=%.14g max=%.14g", v->name, (double)value,
                          (double)SLOT_COLOUR_MIN, (double)SLOT_COLOUR_MAX);
            return 0;
        }
    }
    switch (v->type) {
    case T_U8: *p = (unsigned char)luaL_checkinteger(L, arg); break;
    case T_I16: *(short *)p = (short)luaL_checkinteger(L, arg); break;
    case T_FLAG_U8: *p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_I32: *(int *)p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_I16: *(short *)p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_TWO: *(int *)p = lua_toboolean(L, arg) ? 2 : 1; break;
    case T_F32: {
        lua_Number value = luaL_checknumber(L, arg);
        *(float *)p = value > FLT_MAX ? INFINITY : value < -FLT_MAX ? -INFINITY : (float)value;
        break;
    }
    case T_STR: snprintf((char *)p, v->size, "%s", luaL_checkstring(L, arg)); break;
    case T_I32:
    case T_FRAME_I32: *(int *)p = (int)luaL_checkinteger(L, arg); break;
    default: {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "not_writable", "name=%s", v->name);
    }
    }
    if (!strcmp(v->name, "unitApUsed")) flushArmyApCaches((int)((p - GAME_PTR(v->addr)) / UNIT_SIZE));
    if (!strcmp(v->name, "loadedGame") && lua_tointeger(L, arg)) {
        LuaScript *s = luaScriptOf(L);
        writeLog("lua_loaded_game_set", "mod=%s value=%lld", s ? s->mod : "?", (long long)lua_tointeger(L, arg));
    }
    return 0;
}

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
// Whether the split's two instructions at `trace` and `test` are the ones expected.
static __attribute__((unused)) int splitMinUnitsCodeOk(const unsigned char *trace, const unsigned char *test) {
    static const unsigned char TRACE[] = {0xc7, 0x44, 0x24, 0x20}, TEST[] = {0x81, 0xfb};
    return !memcmp(trace, TRACE, sizeof TRACE) && !memcmp(test, TEST, sizeof TEST) &&
           !memcmp(trace + SPLIT_MIN_UNITS_TRACE_IMM, test + SPLIT_MIN_UNITS_TEST_IMM, 4);
}
#ifdef __APPLE__
static int splitMinUnits(void) { return G32(0); }
static void setSplitMinUnits(int value) { G32(0) = value; }
#else
static int splitMinUnits(void) { return *(const int *)(GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST) + SPLIT_MIN_UNITS_TEST_IMM); }
// Writes both instructions' imm32.
static void setSplitMinUnits(int value) {
    unsigned char *trace = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TRACE), *test = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST);
    unsigned char *from = trace < test ? trace : test, *to = trace < test ? test + 6 : trace + 8;
    DWORD old;
    if (!VirtualProtect(from, (size_t)(to - from), PAGE_EXECUTE_READWRITE, &old)) {
        static int logged;
        if (!logged++) writeLog("ai_split_min_units_written", "ok=0 reason=code_unwritable address=%p", (void *)from);
        return;
    }
    memcpy(trace + SPLIT_MIN_UNITS_TRACE_IMM, &value, 4);
    memcpy(test + SPLIT_MIN_UNITS_TEST_IMM, &value, 4);
    VirtualProtect(from, (size_t)(to - from), old, &old);
    FlushInstructionCache(GetCurrentProcess(), from, (size_t)(to - from));
}
// prepareSiteCode's step for AI_SPLIT_ARMY.
static int checkSplitMinUnitsCode(void) {
    const unsigned char *trace = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TRACE), *test = GAME_PTR(RVA_AI_SPLIT_MIN_UNITS_TEST);
    if (splitMinUnitsCodeOk(trace, test)) return 1;
    writeLog("ai_split_min_units_check", "ok=0 reason=not_the_expected_code trace=%p test=%p", (void *)trace, (void *)test);
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

// ---- The coe5 table ---------------------------------------------------------------------------
// CoE5SE.c5se's script_log: whether mods' own lines go to coe5mod.log.
#define SCRIPT_LOG_SETTING "script_log"
#define SCRIPT_LOG_DEFAULT 0
static int luaScriptLogOn(void) {
    finishReading();
    const ModSetting *set = findDeclaredSetting(COE5SE_MOD, SCRIPT_LOG_SETTING);
    return set ? settingValueNow(set) != 0 : SCRIPT_LOG_DEFAULT;
}

static int lLog(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    const char *text = luaL_checkstring(L, 1);
    if (luaScriptLogOn()) writeLog("lua_log", "mod=%s text=%s", s ? s->mod : "?", text);
    return 0;
}

// coe5.addMainMenuButton(label, onClick[, afterVanillaId]).
static int lAddMainMenuButton(lua_State *L) {
    const char *label = luaL_checkstring(L, 1);
    if (refuseFromDeadLuaScript(L, "coe5.addMainMenuButton")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 2, 0);
    if (callback < 0) {
        refuseLuaCall(runningLuaMod(), "coe5.addMainMenuButton", "out_of_memory", "label=%s", label);
        lua_pushboolean(L, 0);
        return 1;
    }
    int ok = addMainMenuButtonFor((void *)lAddMainMenuButton, label, NULL, callback, lua_isinteger(L, 3) ? (int)lua_tointeger(L, 3) : -1);
    lua_pushboolean(L, ok);
    return 1;
}

// coe5.addGameMenuLine{label=, tooltip=, visible=function, onClick=function, section=}.
static int lAddGameMenuLine(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_getfield(L, 1, "label");
    lua_getfield(L, 1, "tooltip");
    lua_getfield(L, 1, "onClick");
    lua_getfield(L, 1, "visible");
    lua_getfield(L, 1, "section");
    const char *label = luaL_checkstring(L, 2);
    const char *sectionName = lua_type(L, 6) == LUA_TSTRING ? lua_tostring(L, 6) : NULL;
    int section = sectionName ? MENU_SECTION_NAMED : (int)luaL_optinteger(L, 6, GAME_MENU_SECTION_SAVING);
    if (refuseFromDeadLuaScript(L, "coe5.addGameMenuLine")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 4, 5);
    if (callback < 0) {
        refuseLuaCall(runningLuaMod(), "coe5.addGameMenuLine", "out_of_memory", "label=%s", label);
        lua_pushboolean(L, 0);
        return 1;
    }
    int ok = addGameMenuLineFor((void *)lAddGameMenuLine, label, lua_tostring(L, 3), NULL, NULL, callback, section, sectionName);
    lua_pushboolean(L, ok);
    return 1;
}

// coe5.addMapEditorLine{label=, tooltip=, visible=function, onClick=function}: a line in the map
// editor's toolbox, right under "Create Fancy Terrain" (map_editor.h). Returns the line's id, for
// coe5.ui.bindKey in a MAP_EDITOR_WAIT wrap, or false.
static int lAddMapEditorLine(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_getfield(L, 1, "label");
    lua_getfield(L, 1, "tooltip");
    lua_getfield(L, 1, "onClick");
    lua_getfield(L, 1, "visible");
    const char *label = luaL_checkstring(L, 2);
    if (refuseFromDeadLuaScript(L, "coe5.addMapEditorLine")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 4, 5);
    if (callback < 0) {
        refuseLuaCall(runningLuaMod(), "coe5.addMapEditorLine", "out_of_memory", "label=%s", label);
        lua_pushboolean(L, 0);
        return 1;
    }
    int id = addMapEditorLineFor((void *)lAddMapEditorLine, label, lua_tostring(L, 3), callback);
    if (id < 0) lua_pushboolean(L, 0);
    else lua_pushinteger(L, id);
    return 1;
}

// coe5.mapView(): the map view now, as a table (map_editor.h, MapView), or nil when the shown plane
// isn't one of the game's.
static int lMapView(lua_State *L) {
    MapView v;
    if (!currentMapView(&v)) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 18);
    const struct { const char *name; double value; } numbers[] = {
        {"zoom", v.zoom}, {"cameraX", v.cameraX}, {"cameraY", v.cameraY}, {"originX", v.originX}, {"originY", v.originY},
    };
    for (size_t i = 0; i < sizeof numbers / sizeof numbers[0]; i++) {
        lua_pushnumber(L, numbers[i].value);
        lua_setfield(L, -2, numbers[i].name);
    }
    const struct { const char *name; int value; } ints[] = {
        {"screenW", v.screenW}, {"screenH", v.screenH}, {"plane", v.plane}, {"planeX", v.planeX}, {"planeY", v.planeY},
        {"planeW", v.planeW}, {"planeH", v.planeH}, {"firstX", v.firstX}, {"lastX", v.lastX}, {"firstY", v.firstY},
        {"lastY", v.lastY},
    };
    for (size_t i = 0; i < sizeof ints / sizeof ints[0]; i++) {
        lua_pushinteger(L, ints[i].value);
        lua_setfield(L, -2, ints[i].name);
    }
    return 1;
}

// coe5.restartGame(): the API's restartGame, from a Game Menu line's onClick.
static int lRestartGame(lua_State *L) {
    checkLuaMainThread(L, "coe5.restartGame");
    lua_pushboolean(L, luaApi->restartGame());
    return 1;
}

// coe5.loadGame(): shows Load Game's list; a picked save ends this game and loads.
static int lLoadGame(lua_State *L) {
    checkLuaMainThread(L, "coe5.loadGame");
    lua_pushboolean(L, apiLoadGame());
    return 1;
}

// coe5.restartPhase(): "ending", "starting", or nil.
static int lRestartPhase(lua_State *L) {
    int phase = luaApi->restartPhase();
    if (phase == RESTART_ENDING) lua_pushstring(L, "ending");
    else if (phase == RESTART_STARTING) lua_pushstring(L, "starting");
    else lua_pushnil(L);
    return 1;
}

static int lTurn(lua_State *L) { lua_pushinteger(L, G32(RVA_TURN)); return 1; }

// coe5.setting(name): the script's own mod's setting.
static int lSetting(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    LuaScript *s = luaScriptOf(L);
    finishReading();
    const ModSetting *set = s ? findDeclaredSetting(s->mod, name) : NULL;
    if (!set) {
        refuseLuaCall(s ? s->mod : "?", "coe5.setting", "unknown_setting", "name=%s", name);
        lua_pushnil(L);
        return 1;
    }
    int value = settingValueNow(set);
    if (set->optionCount || set->isKey) lua_pushinteger(L, value);
    else lua_pushboolean(L, value != 0);
    return 1;
}
static int lSendMessage(lua_State *L) {
    if (isNetworkGame()) {
        refuseLuaCall(runningLuaMod(), "coe5.sendMessage", "network_game", "player=%lld", (long long)lua_tointeger(L, 1));
        lua_pushinteger(L, -1);
        return 1;
    }
    lua_Integer player = luaL_checkinteger(L, 1);
    const char *title = luaL_checkstring(L, 2), *text = luaL_checkstring(L, 3);
    if (player < INT_MIN || player > INT_MAX) {
        refuseLuaCall(runningLuaMod(), "coe5.sendMessage", "bad_player", "player=%lld title=%s", (long long)player, title);
        lua_pushinteger(L, MESSAGE_NOT_ADDED);
        return 1;
    }
    lua_pushinteger(L, luaApi->sendMessage((int)player, title, text));
    return 1;
}

// coe5.setupAutomated(): the API's setupAutomated flag, which coe5.setSetupAutomated sets.
static int lSetupAutomated(lua_State *L) {
    lua_pushboolean(L, luaApi->setupAutomated() != 0);
    return 1;
}

// coe5.hideMainMenuButton(id, hidden): hides the game's button `id` while hidden() returns true.
static int *luaReadTipHiders;
static int luaReadTipHiderCount, luaReadTipHiderCapacity;
static int lHideMainMenuButton(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    if (id != MAIN_MENU_READ_TIP) {
        refuseLuaCall(mod, "coe5.hideMainMenuButton", "not_hidable", "id=%lld", (long long)id);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (refuseFromDeadLuaScript(L, "coe5.hideMainMenuButton")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 2, 0);
    if (callback < 0 || !growList((void **)&luaReadTipHiders, &luaReadTipHiderCapacity, luaReadTipHiderCount + 1, sizeof luaReadTipHiders[0])) {
        refuseLuaCall(mod, "coe5.hideMainMenuButton", "out_of_memory", "id=%lld", (long long)id);
        lua_pushboolean(L, 0);
        return 1;
    }
    luaReadTipHiders[luaReadTipHiderCount++] = callback;
    writeLog("main_menu_button_hidable", "id=%d mod=%s", MAIN_MENU_READ_TIP, mod);
    lua_pushboolean(L, 1);
    return 1;
}
// Runs every script's hidden function, and returns 1 when any says to hide.
static int luaHidesReadTip(void) {
    int hidden = 0;
    for (int i = 0; i < luaReadTipHiderCount; i++)
        if (runLuaCallback(luaReadTipHiders[i], 1, 0)) hidden = 1;
    return hidden;
}

// coe5.addScreen(label, open[, tooltip]): a line in the Screens box of the mod's page.
static int lAddScreen(lua_State *L) {
    const char *label = luaL_checkstring(L, 1);
    const char *tip = luaL_optstring(L, 3, NULL);
    LuaScript *s = luaScriptOf(L);
    if (refuseFromDeadLuaScript(L, "coe5.addScreen")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 2, 0);
    if (callback < 0) {
        refuseLuaCall(s ? s->mod : "?", "coe5.addScreen", "out_of_memory", "label=%s", label);
        lua_pushboolean(L, 0);
        return 1;
    }
    int ok = addScreenFor((void *)lAddScreen, s ? s->mod : "?", label, tip, NULL, callback);
    lua_pushboolean(L, ok);
    return 1;
}

// coe5.rebindKey(action, key), coe5.addActionKey(action, key), coe5.actionKey(action).
static int luaActionKeyArgs(lua_State *L, const char *call, int *action, int *key) {
    lua_Integer a = luaL_checkinteger(L, 1), k = key ? luaL_checkinteger(L, 2) : 0;
    LuaScript *s = luaScriptOf(L);
    if (a < INT_MIN || a > INT_MAX) {
        refuseLuaCall(s ? s->mod : "?", call, "unknown_action", "action=%lld", (long long)a);
        return 0;
    }
    if (k < INT_MIN || k > INT_MAX) {
        refuseLuaCall(s ? s->mod : "?", call, "bad_key", "action=%lld key=%lld", (long long)a, (long long)k);
        return 0;
    }
    *action = (int)a;
    if (key) *key = (int)k;
    return 1;
}
static int lRebindKey(lua_State *L) {
    int action, key;
    int ok = luaActionKeyArgs(L, "coe5.rebindKey", &action, &key) && luaApi->rebindKey(action, key);
    lua_pushboolean(L, ok);
    return 1;
}
static int lAddActionKey(lua_State *L) {
    int action, key;
    int ok = luaActionKeyArgs(L, "coe5.addActionKey", &action, &key) && luaApi->addActionKey(action, key);
    lua_pushboolean(L, ok);
    return 1;
}
static int lActionKey(lua_State *L) {
    int action, key = -1;
    if (luaActionKeyArgs(L, "coe5.actionKey", &action, NULL)) key = luaApi->actionKey(action);
    if (key < 0) lua_pushnil(L);
    else lua_pushinteger(L, key);
    return 1;
}

// coe5.findMonster(name): the index of the first monster with this name, or nil.
static int lFindMonster(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int last = G32(RVA_MONSTER_COUNT);
    if (strlen(name) < NAME_SIZE)
        for (int m = 0; m <= last && m < MONSTER_TABLE_MAX; m++)
            if (!_strnicmp((const char *)(gameBase + RVA_MONSTER_TABLE + (size_t)m * MONSTER_SIZE), name, NAME_SIZE)) {
                lua_pushinteger(L, m);
                return 1;
            }
    lua_pushnil(L);
    return 1;
}

// coe5.portalExits(x, y): where the portal on square (x, y) leads.
#define PORTAL_EXITS_MAX 100
static int lPortalExits(lua_State *L) {
    lua_Integer x0 = luaL_checkinteger(L, 1), y0 = luaL_checkinteger(L, 2);
    lua_createtable(L, 0, 0);
    if (x0 < 0 || y0 < 0 || x0 >= MAP_COLUMNS_MAX || y0 >= MAP_COLUMN_SQUARES) return 1;
    int portal = G16(RVA_SQUARE_RECORDS + ((size_t)x0 * MAP_COLUMN_SQUARES + (size_t)y0) * SQUARE_SIZE + SQUARE_PORTAL);
    if (portal <= 0) return 1;
    int depth = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H), found = 0;
    for (int x = 0; x < depth && x < MAP_COLUMNS_MAX && found < PORTAL_EXITS_MAX; x++)
        for (int y = 0; y < height && y < MAP_COLUMN_SQUARES && found < PORTAL_EXITS_MAX; y++) {
            int here = G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_PORTAL);
            if ((here != portal && here != -portal) || (x == x0 && y == y0)) continue;
            lua_createtable(L, 0, 2);
            lua_pushinteger(L, x);
            lua_setfield(L, -2, "x");
            lua_pushinteger(L, y);
            lua_setfield(L, -2, "y");
            lua_rawseti(L, -2, ++found);
        }
    return 1;
}

// coe5.squareUnits(x, y): the living units on square (x, y).
static int lSquareUnits(lua_State *L) {
    lua_Integer x = luaL_checkinteger(L, 1), y = luaL_checkinteger(L, 2);
    lua_createtable(L, 0, 0);
    if (x < 0 || y < 0 || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 1;
    int last = G32(RVA_UNIT_COUNT), found = 0;
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        if (!unitAliveAt(u, (int)x, (int)y)) continue;
        lua_pushinteger(L, u);
        lua_rawseti(L, -2, ++found);
    }
    return 1;
}

// coe5.armies([viewer]): every army on the world map.
static void setIntField(lua_State *L, const char *key, lua_Integer value);
static int lArmies(lua_State *L) {
    int withViewer = !lua_isnoneornil(L, 1);
    lua_Integer viewer = withViewer ? luaL_checkinteger(L, 1) : -1;
    if (withViewer && (viewer < 0 || viewer >= PLAYER_SLOTS)) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.armies", "bad_player", "viewer=%lld", (long long)viewer);
        lua_createtable(L, 0, 0);
        return 1;
    }
    int depth = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H);
    if (depth > MAP_COLUMNS_MAX) depth = MAP_COLUMNS_MAX;
    if (height > MAP_COLUMN_SQUARES) height = MAP_COLUMN_SQUARES;
    if (depth < 0) depth = 0;
    if (height < 0) height = 0;
    int *entry = lua_newuserdatauv(L, (size_t)depth * (size_t)height * sizeof(int) + sizeof(int), 0);
    memset(entry, 0, (size_t)depth * (size_t)height * sizeof(int) + sizeof(int));
    lua_createtable(L, 0, 0);
    int n = 0;
    for (int x = 0; x < depth; x++)
        for (int y = 0; y < height; y++) {
            size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            int units = G16(RVA_SQUARE_ARMY + square * 2);
            if (!units) continue;
            if (withViewer && G8(RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_SEEN + (size_t)viewer) < 2) continue;
            lua_createtable(L, 0, 5);
            setIntField(L, "x", x);
            setIntField(L, "y", y);
            setIntField(L, "owner", armyOwnerAt(x, y));
            setIntField(L, "units", units);
            lua_createtable(L, 0, 0);
            lua_setfield(L, -2, "commanders");
            lua_rawseti(L, -2, ++n);
            entry[(size_t)x * (size_t)height + (size_t)y] = n;
        }
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX; c++) {
        int unit = livingCommanderUnit(c);
        if (unit < 0) continue;
        int x = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), y = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
        if (x < 0 || y < 0 || x >= depth || y >= height) continue;
        int at = entry[(size_t)x * (size_t)height + (size_t)y];
        if (!at) continue;
        lua_rawgeti(L, -1, at);
        lua_getfield(L, -1, "commanders");
        lua_pushinteger(L, c);
        lua_rawseti(L, -2, (lua_Integer)lua_rawlen(L, -2) + 1);
        lua_pop(L, 2);
    }
    lua_remove(L, -2);
    return 1;
}

// ---- AI routes: coe5.route --------------------------------------------------------------------
// The sites of an AI commander's move.
static const CallSite AI_MOVE_SITES[] = {SITE_AI_RALLY, SITE_AI_FRIENDLY_MOVE, SITE_AI_CONQUER, SITE_AI_EXPLORE, SITE_AI_FAR_EXPLORE,
                                         SITE_AI_MOVE_COMSTR};
static int aiMoveSiteRunning(void) {
    for (size_t i = 0; i < sizeof AI_MOVE_SITES / sizeof AI_MOVE_SITES[0]; i++)
        if (luaSiteDepth[AI_MOVE_SITES[i]]) return 1;
    return 0;
}

// The steps of the script's budget each coe5.route call costs.
#define ROUTE_CALL_STEPS 10000
// Why coe5.route refuses its call, or NULL when it can go to the game.
static const char *routeRefusal(int inAiSite, lua_Integer commander, lua_Integer x, lua_Integer y, lua_Integer comstr, int mapDepth,
                                int mapH) {
    if (!inAiSite) return "outside_ai_site";
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (x < 0 || y < 0 || x >= mapDepth || y >= mapH || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return "off_map";
    if (comstr < -1 || comstr > ROUTE_COMSTR_MAX) return "out_of_range";
    return NULL;
}

// coe5.route(commander, x, y[, comstr]): how far the commander's army has to go to reach (x, y).
static int lRoute(lua_State *L) {
    coe5seChargeLuaSteps(L, ROUTE_CALL_STEPS);
    lua_Integer commander = luaL_checkinteger(L, 1), x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    int ownStrength = lua_isnoneornil(L, 4);
    lua_Integer comstr = ownStrength ? 0 : luaL_checkinteger(L, 4);
    const char *reason = routeRefusal(aiMoveSiteRunning(), commander, x, y, comstr, G32(RVA_MAP_DEPTH), G32(RVA_MAP_H));
    if (reason) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.route", reason, "commander=%lld x=%lld y=%lld comstr=%lld", (long long)commander,
                      (long long)x, (long long)y, (long long)comstr);
        lua_pushnil(L);
        return 1;
    }
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    int fromX = -1, fromY = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0)
        fromX = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), fromY = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
    if (fromX < 0 || fromY < 0) {
        lua_pushnil(L);
        return 1;
    }
    // @platform-split ai_route: Mac takes no strength from the game (RVA_COMMANDER_STRENGTH not looked up)
    if (ownStrength) comstr = commanderStrength((int)commander);
    int randomIndex = G32(RVA_RANDOM_INDEX), randomShift = G32(RVA_RANDOM_SHIFT), randomXor = G32(RVA_RANDOM_XOR);
    int stepX = 0, stepY = 0;
#ifdef __APPLE__
    int length = GAME_FN(int (*)(int, int, int, int, int, int, int *, int *), RVA_WAYTOGO2)((int)commander, fromX, fromY, (int)x, (int)y,
                                                                                         (int)comstr, &stepX, &stepY);
#else
    int length = GAME_FN(int (*)(int, int, int, int, int, int, int, int *, int *), RVA_WAYTOGO2)(
        (int)commander, fromX, fromY, (int)x, (int)y, (int)comstr, 0, &stepX, &stepY);
#endif
    G32(RVA_RANDOM_INDEX) = randomIndex, G32(RVA_RANDOM_SHIFT) = randomShift, G32(RVA_RANDOM_XOR) = randomXor;
    if (length < 0) lua_pushnil(L);
    else lua_pushinteger(L, length);
    return 1;
}

// ---- AI ritual plans: coe5.scrapRitualPlan --------------------------------------------------------
// The AI sites.
static int aiSiteRunning(void) {
    return aiMoveSiteRunning() || luaSiteDepth[SITE_AI_RITUAL_CANDIDATE] || luaSiteDepth[SITE_AI_GATHER_TROOPS] ||
           luaSiteDepth[SITE_AI_PICK_UP_LONER] || luaSiteDepth[SITE_AI_SPLIT_ARMY] || luaObserving[SITE_AI_TURN] != NULL;
}

// Why coe5.scrapRitualPlan refuses its call, or NULL.
static const char *ritualPlanScrapRefusal(int inAiSite, lua_Integer commander, int owner, int controller, int plan) {
    if (!inAiSite) return "outside_ai_site";
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (owner < 0) return "no_unit";
    if (controller != CONTROLLER_AI) return "not_ai";
    if (plan < 0) return "not_ritual";
    return NULL;
}

// coe5.scrapRitualPlan(commander): drops an AI commander's planned ritual.
static int lScrapRitualPlan(lua_State *L) {
    lua_Integer commander = luaL_checkinteger(L, 1);
    int owner = -1, controller = CONTROLLER_REMOVED, plan = 0;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        }
        plan = G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE);
    }
    LuaScript *s = luaScriptOf(L);
    const char *reason = ritualPlanScrapRefusal(aiSiteRunning(), commander, owner, controller, plan);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.scrapRitualPlan", reason, "commander=%lld player=%d controller=%d plan=%d",
                      (long long)commander, owner, controller, plan);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (plan > 0) {
        G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE) = 0;
        writeLog("ai_plan_scrapped", "mod=%s commander=%lld player=%d ritual=%d", s ? s->mod : "?", (long long)commander, owner, plan);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// ---- AI armies: coe5.releaseFollower -------------------------------------------------------------
// Why coe5.releaseFollower refuses its call, or NULL.
static const char *followerReleaseRefusal(int inAiSite, int network, int windowsOnly, lua_Integer unit, int owner, int controller,
                                          int activePlayer) {
    if (windowsOnly) return "windows_only";
    if (!inAiSite) return "outside_ai_site";
    if (network) return "network_game";
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return "bad_unit";
    if (owner < 0) return "no_unit";
    if (controller != CONTROLLER_AI) return "not_ai";
    if (owner != activePlayer) return "not_moving_player";
    return NULL;
}

// @platform-split release_follower: Windows only, as the AI sites (Mac's AI_TURN observers refuse it)
// coe5.releaseFollower(unit): frees one unit of an AI player from the commander it follows.
#ifdef __APPLE__
static int releaseFollowerWindowsOnly = 1;
#else
static int releaseFollowerWindowsOnly = 0;
#endif
static int lReleaseFollower(lua_State *L) {
    lua_Integer unit = luaL_checkinteger(L, 1);
    int owner = -1, controller = CONTROLLER_REMOVED, leader = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
        int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
        if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
    }
    int active = G32(RVA_ACTIVE_PLAYER);
    LuaScript *s = luaScriptOf(L);
    const char *reason = followerReleaseRefusal(aiSiteRunning(), isNetworkGame(), releaseFollowerWindowsOnly, unit, owner, controller,
                                                active);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.releaseFollower", reason, "unit=%lld player=%d controller=%d active=%d commander=%d",
                      (long long)unit, owner, controller, active, leader);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (leader >= 0) {
        G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER) = -1;
        flushCommanderCache(leader);
        writeLog("ai_follower_released", "mod=%s unit=%lld commander=%d player=%d", s ? s->mod : "?", (long long)unit, leader, owner);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// ---- AI armies: coe5.transferFollower --------------------------------------------------------------
// Why coe5.transferFollower refuses its call, or NULL.
static const char *followerTransferRefusal(int inAiSite, int network, int windowsOnly, lua_Integer unit, int owner, int controller,
                                           int activePlayer, lua_Integer commander, int commanderOwner, int commanderUnit, int leader,
                                           int sameSquare) {
    const char *reason = followerReleaseRefusal(inAiSite, network, windowsOnly, unit, owner, controller, activePlayer);
    if (reason) return reason;
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (commanderOwner < 0) return "no_commander";
    if (commanderOwner != owner) return "other_player";
    if (commanderUnit == unit) return "same_unit";
    if (leader < 0) return "not_following";
    if (leader == commander) return "already_following";
    if (!sameSquare) return "not_same_square";
    return NULL;
}

// @platform-split transfer_follower: Windows only; RVA_TRANSFER_UNIT_TO_COMMANDER wasn't looked up on Mac
// The game's own one-unit transfer.
static int (*transferUnitGame)(int unit, int commander);

// coe5.transferFollower(unit, commander): moves one unit to another commander on the same square.
static int lTransferFollower(lua_State *L) {
    lua_Integer unit = luaL_checkinteger(L, 1), commander = luaL_checkinteger(L, 2);
    int owner = -1, controller = CONTROLLER_REMOVED, leader = -1, ux = -1, uy = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
        int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
        if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
        ux = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), uy = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
    }
    int commanderOwner = -1, commanderUnit = -1, cx = -2, cy = -2;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        commanderUnit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (commanderUnit >= 0 && commanderUnit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)commanderUnit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)commanderUnit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) commanderOwner = player;
            cx = G32(RVA_UNIT_X + (size_t)commanderUnit * UNIT_SIZE), cy = G32(RVA_UNIT_X + (size_t)commanderUnit * UNIT_SIZE + 4);
        }
    }
#ifndef __APPLE__
    if (!transferUnitGame) transferUnitGame = GAME_FN(int (*)(int, int), RVA_TRANSFER_UNIT_TO_COMMANDER);
#endif
    int active = G32(RVA_ACTIVE_PLAYER);
    LuaScript *s = luaScriptOf(L);
    const char *reason = followerTransferRefusal(aiSiteRunning(), isNetworkGame(), transferUnitGame == NULL, unit, owner, controller,
                                                 active, commander, commanderOwner, commanderUnit, leader, ux == cx && uy == cy);
    if (!reason && !transferUnitGame((int)unit, (int)commander)) reason = "game_refused";
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.transferFollower", reason,
                      "unit=%lld commander=%lld player=%d commander_player=%d active=%d follows=%d", (long long)unit,
                      (long long)commander, owner, commanderOwner, active, leader);
        lua_pushboolean(L, 0);
        return 1;
    }
    int now = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
    writeLog("ai_follower_transferred", "mod=%s unit=%lld from=%d to=%lld player=%d follows=%d", s ? s->mod : "?", (long long)unit, leader,
             (long long)commander, owner, now);
    lua_pushboolean(L, now == commander);
    return 1;
}

// ---- AI recruiting: coe5.shopOffers, classRecruits, recruitSquares, aiBuyOffer -------------------
// A player's recruitment offers, its class's list, its recruit squares, and the AI's buy.

// @platform-split ai_recruit: Windows only; the Mac step is inlined and its helpers not looked up
// The game functions the square rules and the buy call.
typedef struct AiRecruitCalls {
    int (*offerAvailable)(int player, int offer, int x, int y);
    int (*citadelFor)(int player, int x, int y);
    int (*squareAbilityTotal)(int player, int x, int y, int ability, int sum);
    int (*countMonsterAt)(int x, int y, int monster, int player);
    int (*seaAreaSize)(int x, int y, int limit);
    int (*squareHasFort)(int x, int y);
    void (*buyTroops)(int player, int offer, int x, int y);
    int (*buyCommander)(int player, int offer, int x, int y);
    void (*makeSpawnbooster)(int commander);
} AiRecruitCalls;
static AiRecruitCalls aiRecruitCalls;

static const AiRecruitCalls *aiRecruitGame(void) {
#ifndef __APPLE__
    if (!aiRecruitCalls.offerAvailable) {
        aiRecruitCalls.offerAvailable = GAME_FN(int (*)(int, int, int, int), RVA_SHOP_OFFER_AVAILABLE);
        aiRecruitCalls.citadelFor = GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR);
        aiRecruitCalls.squareAbilityTotal = GAME_FN(int (*)(int, int, int, int, int), RVA_SQUARE_ABILITY_TOTAL);
        aiRecruitCalls.countMonsterAt = GAME_FN(int (*)(int, int, int, int), RVA_COUNT_MONSTER_AT);
        aiRecruitCalls.seaAreaSize = GAME_FN(int (*)(int, int, int), RVA_SEA_AREA_SIZE);
        aiRecruitCalls.squareHasFort = GAME_FN(int (*)(int, int), RVA_SQUARE_HAS_FORT);
        aiRecruitCalls.buyTroops = GAME_FN(void (*)(int, int, int, int), RVA_AI_BUY_TROOPS);
        aiRecruitCalls.buyCommander = GAME_FN(int (*)(int, int, int, int), RVA_AI_BUY_COMMANDER);
        aiRecruitCalls.makeSpawnbooster = GAME_FN(void (*)(int), RVA_AI_MAKE_SPAWNBOOSTER);
    }
#endif
    return aiRecruitCalls.offerAvailable ? &aiRecruitCalls : NULL;
}

#define SHOP_SQUARES_MAX 100
#define SHIP_SEA_SQUARES 50

// A monster's action points a turn.
static int monsterMoves(int monster) {
    if (monster < 0 || monster >= MONSTER_TABLE_MAX) return 0;
    unsigned long long flags = *(unsigned long long *)(gameBase + RVA_MONSTER_FLAGS + (size_t)monster * MONSTER_SIZE);
    return flags & UNIT_FLAG_FAST ? 4 : flags & UNIT_FLAG_SLOW ? 2 : 3;
}

// The highest level of a monster's spell attacks, 0 for none.
static int monsterSpellLevel(int monster) {
    if (monster < 0 || monster >= MONSTER_TABLE_MAX) return 0;
    const unsigned char *rows = gameBase + RVA_MONSTER_TABLE + (size_t)monster * MONSTER_SIZE + MONSTER_ATTACKS;
    int best = 0;
    for (int i = 0; i < MONSTER_ATTACK_MAX; i++) {
        const int *row = (const int *)(rows + (size_t)i * MONSTER_ATTACK_SIZE);
        if (row[0] < 1 || row[2] < 0) break;
        if ((row[0] == 3 || row[0] == 6 || row[0] == 14 || row[0] == 15 || row[0] == 19) && row[4] > best) best = row[4];
    }
    return best;
}

static void setIntField(lua_State *L, const char *key, lua_Integer value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}
static void setBoolField(lua_State *L, const char *key, int value) {
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

// A string cut to `size` bytes, its 0 or the end.
static void pushRecordText(lua_State *L, const char *text, size_t size) {
    size_t len = 0;
    while (len < size && text[len]) len++;
    lua_pushlstring(L, text, len);
}

// The monster fields every offer has.
static void setOfferMonster(lua_State *L, int monster, const char *name) {
    if (monster >= 0) setIntField(L, "monster", monster);
    pushRecordText(L, name, MONSTER_NAME_SIZE);
    lua_setfield(L, -2, "name");
    setIntField(L, "moves", monster >= 0 ? monsterMoves(monster) : 0);
    setIntField(L, "spellLevel", monster >= 0 ? monsterSpellLevel(monster) : 0);
}

// An offer's costs, by cost type.
static void setOfferCosts(lua_State *L, const int *costs) {
    lua_createtable(L, 0, SHOP_COST_COUNT);
    for (int t = 0; t < SHOP_COST_COUNT; t++) {
        lua_pushinteger(L, costs[t]);
        lua_rawseti(L, -2, t);
    }
    lua_setfield(L, -2, "costs");
}

// coe5.shopOffers(player): the player's recruitment offers this turn.
static int lShopOffers(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1);
    lua_createtable(L, 0, 0);
    if (player < 0 || player >= PLAYER_SLOTS) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.shopOffers", "bad_player", "player=%lld", (long long)player);
        return 1;
    }
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int last = G32(RVA_SHOP_LAST + block), n = 0;
    for (int i = 0; i <= last && i < SHOP_OFFERS_MAX; i++) {
        int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)i * 4);
        if (count <= 0) continue;
        int monster = G32(RVA_SHOP_OFFERS + block + (size_t)i * 4);
        int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)i * 4);
        const int *costs = (const int *)(gameBase + RVA_SHOP_COSTS + block + (size_t)i * SHOP_COST_COUNT * 4);
        const char *named = (const char *)(gameBase + RVA_SHOP_OFFER_NAMES + block + (size_t)i * SHOP_OFFER_TEXT_SIZE);
        const char *limiter = (const char *)(gameBase + RVA_SHOP_OFFER_LIMITS + block + (size_t)i * SHOP_OFFER_TEXT_SIZE);
        if (monster < 0 || monster >= MONSTER_TABLE_MAX) continue;
        lua_createtable(L, 0, 16);
        setIntField(L, "offer", i);
        setOfferMonster(L, monster, (const char *)(gameBase + RVA_MONSTER_TABLE + (size_t)monster * MONSTER_SIZE));
        setIntField(L, "count", count);
        setBoolField(L, "commander", named[0] != 0);
        setOfferCosts(L, costs);
        setIntField(L, "gold", costs[0]);
        setIntField(L, "iron", costs[1]);
        setIntField(L, "flags", flags);
        setBoolField(L, "oncePerOffer", (flags & SHOP_FLAG_ONCE) != 0);
        setBoolField(L, "extra", (flags & SHOP_FLAG_EXTRA) != 0);
        setBoolField(L, "aiNever", (flags & SHOP_FLAG_AI_NEVER) != 0);
        setIntField(L, "terrain", G32(RVA_SHOP_OFFER_TERRAIN + block + (size_t)i * 4));
        if (limiter[0]) {
            pushRecordText(L, limiter, SHOP_OFFER_TEXT_SIZE);
            lua_setfield(L, -2, "limiter");
        }
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// coe5.classRecruits(class): the class's recruitment list.
static int lClassRecruits(lua_State *L) {
    lua_Integer cls = luaL_checkinteger(L, 1);
    lua_createtable(L, 0, 0);
    if (cls < 0 || cls >= CLASS_TABLE_MAX) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.classRecruits", "bad_class", "class=%lld", (long long)cls);
        return 1;
    }
    const unsigned char *list = gameBase + RVA_CLASS_SIDE_A + (size_t)cls * CLASS_RECRUITS_SIZE;
    for (int i = 0; i < CLASS_RECRUITS_MAX; i++) {
        const unsigned char *rec = list + (size_t)i * CLASS_RECRUIT_SIZE;
        if (!rec[RECRUIT_MONSTER_NAME]) break;
        char name[MONSTER_NAME_SIZE];
        memcpy(name, rec + RECRUIT_MONSTER_NAME, MONSTER_NAME_SIZE - 1);
        name[MONSTER_NAME_SIZE - 1] = 0;
        int count = *(const int *)(rec + RECRUIT_COUNT);
        int costs[SHOP_COST_COUNT] = {0};
        costs[0] = *(const int *)(rec + RECRUIT_GOLD);
        costs[1] = *(const int *)(rec + RECRUIT_IRON);
        const short *extra = (const short *)(rec + RECRUIT_EXTRA_COSTS);
        for (int k = 0; k < 2 && extra[2 * k] >= 0; k++)
            if (extra[2 * k] < SHOP_COST_COUNT) costs[extra[2 * k]] = extra[2 * k + 1];
        lua_createtable(L, 0, 14);
        setOfferMonster(L, monsterByName(name), name);
        setIntField(L, "chance", *(const int *)(rec + RECRUIT_CHANCE));
        setIntField(L, "count", count);
        setBoolField(L, "commander", count == -1);
        setOfferCosts(L, costs);
        setIntField(L, "gold", costs[0]);
        setIntField(L, "goldDie", *(const int *)(rec + RECRUIT_GOLD_DIE));
        setIntField(L, "iron", costs[1]);
        setIntField(L, "flags", *(const int *)(rec + RECRUIT_FLAGS));
        setIntField(L, "terrain", *(const int *)(rec + RECRUIT_TERRAIN));
        lua_createtable(L, 0, 0);
        for (int k = 0, n = 0; k < RECRUIT_LIMITERS_MAX; k++) {
            const char *line = (const char *)(rec + RECRUIT_LIMITERS + (size_t)k * MONSTER_NAME_SIZE);
            if (!line[0]) break;
            pushRecordText(L, line, MONSTER_NAME_SIZE);
            lua_rawseti(L, -2, ++n);
        }
        lua_setfield(L, -2, "limiters");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// The squares where the game's AI recruiting can buy for `player`.
static int recruitSquaresOf(const AiRecruitCalls *g, int player, int *xs, int *ys) {
    int depth = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H), n = 0;
    for (int y = 0; y < height && y < MAP_COLUMN_SQUARES; y++)
        for (int x = 0; x < depth && x < MAP_COLUMNS_MAX && n < SHOP_SQUARES_MAX; x++) {
            size_t rec = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
            if ((signed char)G8(RVA_SQUARE_RECORDS + rec + SQUARE_OWNER) != player) continue;
            if (G16(RVA_SQUARE_RECORDS + rec + SQUARE_TERRAIN_TYPE) < 0 || !g->citadelFor(player, x, y)) continue;
            xs[n] = x, ys[n] = y, n++;
        }
    return n;
}

// Why the game's AI recruiting wouldn't buy `player`'s offer on square (x, y), or NULL.
static const char *aiOfferSquareRefusal(const AiRecruitCalls *g, int player, int offer, int x, int y) {
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4);
    int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)offer * 4);
    int monster = G32(RVA_SHOP_OFFERS + block + (size_t)offer * 4);
    if (count < 0) return "item";
    if (count == 0) return "sold_out";
    if (flags & SHOP_FLAG_AI_NEVER) return "not_for_ai";
    int available = g->offerAvailable(player, offer, x, y);
    if (!(available > 1 || (available > 0 && (flags & SHOP_FLAG_AI_IMPORTANT))))
        return available == -1 ? "cannot_pay" : available == 1 ? "too_few_to_upgrade" : "not_here";
    if (G32(RVA_SQUARE_FLAGS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE) & SQUARE_FLAG_RECRUITED) return "square_used";
    for (int level = 1; level <= 3; level++) {
        int power = monsterAbility(monster, MONSTER_ABILITY_AIPOWCOM1 + level - 1);
        if (power > 0 && g->squareAbilityTotal(player, x, y, power, 0) < level) return "aipowcom";
    }
    int most = monsterAbility(monster, MONSTER_ABILITY_AIMAXSHOP);
    if (most == -1 || (most > 0 && g->countMonsterAt(x, y, monster, player) >= most)) return "aimaxshop";
    if (monsterAbility(monster, MONSTER_ABILITY_SHIP) > 0 && g->seaAreaSize(x, y, SHIP_SEA_SQUARES) < SHIP_SEA_SQUARES) return "no_sea";
    if (monsterAbility(monster, MONSTER_ABILITY_AIHOLD) == 1 && !g->squareHasFort(x, y)) return "aihold";
    return NULL;
}

// The player of the AI_RECRUIT call whose wrap is running now, -1 outside one.
static int aiRecruitPlayer(void) {
    for (const LuaFrame *f = luaFrames; f; f = f->up)
        if (f->wrap && luaSites[f->wrap->site].site == SITE_AI_RECRUIT) return (int)f->args->a[0];
    return -1;
}

// Why coe5.aiBuyOffer refuses its arguments, or NULL.
static const char *aiBuyRefusal(int sitePlayer, int humanTurn, lua_Integer player, int controller, lua_Integer offer, int shopLast) {
    if (sitePlayer < 0) return "outside_ai_recruit";
    if (player < 0 || player >= PLAYER_SLOTS) return "bad_player";
    if (player != sitePlayer) return "other_player";
    if (!humanTurn && controller != CONTROLLER_AI) return "not_ai";
    if (offer < 0 || offer > shopLast || offer >= SHOP_OFFERS_MAX) return "bad_offer";
    return NULL;
}

// coe5.recruitSquares(player[, offer]): where the game's AI recruiting can buy for the player.
static int lRecruitSquares(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1);
    int withOffer = !lua_isnoneornil(L, 2);
    lua_Integer offer = withOffer ? luaL_checkinteger(L, 2) : -1;
    lua_createtable(L, 0, 0);
    const AiRecruitCalls *g = aiRecruitGame();
    const char *reason = player < 0 || player >= PLAYER_SLOTS ? "bad_player" : NULL;
    if (!reason && withOffer && (offer < 0 || offer >= SHOP_OFFERS_MAX || offer > G32(RVA_SHOP_LAST + (size_t)player * PLAYER_BLOCK_SIZE)))
        reason = "bad_offer";
    if (!reason && !g) reason = "windows_only";
    if (reason) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.recruitSquares", reason, "player=%lld offer=%lld", (long long)player, (long long)offer);
        return 1;
    }
    int xs[SHOP_SQUARES_MAX], ys[SHOP_SQUARES_MAX], found = recruitSquaresOf(g, (int)player, xs, ys), n = 0;
    for (int i = 0; i < found; i++) {
        if (withOffer && aiOfferSquareRefusal(g, (int)player, (int)offer, xs[i], ys[i])) continue;
        size_t rec = ((size_t)xs[i] * MAP_COLUMN_SQUARES + (size_t)ys[i]) * SQUARE_SIZE;
        lua_createtable(L, 0, 3);
        setIntField(L, "x", xs[i]);
        setIntField(L, "y", ys[i]);
        setBoolField(L, "used", (G32(RVA_SQUARE_FLAGS + rec) & SQUARE_FLAG_RECRUITED) != 0);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// coe5.aiBuyOffer(player, offer, x, y): buys the AI player's offer on square (x, y).
typedef const char *(*AiBuySquareRule)(const AiRecruitCalls *g, int player, int offer, int x, int y);
typedef void (*AiBuyAfter)(int player, int x, int y);
static int lAiBuyOffer(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1), offer = luaL_checkinteger(L, 2);
    lua_Integer x = luaL_checkinteger(L, 3), y = luaL_checkinteger(L, 4);
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    int inRange = player >= 0 && player < PLAYER_SLOTS;
    int controller = inRange ? G16(RVA_SLOT_CONTROLLER + (size_t)player * 2) : CONTROLLER_REMOVED;
    int shopLast = inRange ? G32(RVA_SHOP_LAST + (size_t)player * PLAYER_BLOCK_SIZE) : -1;
    const AiRecruitCalls *g = aiRecruitGame();
    int sitePlayer = aiRecruitPlayer(), humanTurn = 0, onlyX = -1, onlyY = -1;
    AiBuySquareRule humanRule = NULL;
    AiBuyAfter humanAfter = NULL;
#ifdef COE5SE_AI_BUY_HOOK
    if (sitePlayer < 0) humanTurn = (sitePlayer = COE5SE_AI_BUY_HOOK(&onlyX, &onlyY, &humanRule, &humanAfter)) >= 0;
#endif
    const char *reason = aiBuyRefusal(sitePlayer, humanTurn, player, controller, offer, shopLast);
    if (!reason && !g) reason = "windows_only";
    if (!reason && (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES))
        reason = "off_map";
    if (!reason && humanTurn && (x != onlyX || y != onlyY)) reason = "not_screen_square";
    if (!reason && humanTurn && humanRule) {
        reason = humanRule(g, (int)player, (int)offer, (int)x, (int)y);
    } else if (!reason) {
        int xs[SHOP_SQUARES_MAX], ys[SHOP_SQUARES_MAX], found = recruitSquaresOf(g, (int)player, xs, ys), mine = 0;
        for (int i = 0; i < found && !mine; i++) mine = xs[i] == x && ys[i] == y;
        reason = mine ? aiOfferSquareRefusal(g, (int)player, (int)offer, (int)x, (int)y) : "not_recruit_square";
    }
    if (reason) {
        refuseLuaCall(mod, "coe5.aiBuyOffer", reason, "player=%lld offer=%lld x=%lld y=%lld", (long long)player, (long long)offer,
                      (long long)x, (long long)y);
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)offer * 4);
    int monster = G32(RVA_SHOP_OFFERS + block + (size_t)offer * 4);
    int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4);
    int commander = *(const char *)(gameBase + RVA_SHOP_OFFER_NAMES + block + (size_t)offer * SHOP_OFFER_TEXT_SIZE) != 0;
    int goldBefore = G32(RVA_PLAYER_INTS + block), made = -1;
    if (commander) {
        made = g->buyCommander((int)player, (int)offer, (int)x, (int)y);
        g->makeSpawnbooster(made);
    } else {
        g->buyTroops((int)player, (int)offer, (int)x, (int)y);
    }
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    if (!(flags & SHOP_FLAG_EXTRA)) G32(RVA_SQUARE_FLAGS + square) |= SQUARE_FLAG_RECRUITED;
    if (flags & SHOP_FLAG_ONCE) G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4) = 0;
    if (humanTurn && humanAfter) humanAfter((int)player, (int)x, (int)y);
    writeLog("ai_offer_bought", "mod=%s player=%lld offer=%lld monster=%d count=%d commander=%d new_commander=%d x=%lld y=%lld "
             "gold_before=%d gold_after=%d flags=0x%x", mod, (long long)player, (long long)offer, monster, count, commander, made,
             (long long)x, (long long)y, goldBefore, G32(RVA_PLAYER_INTS + block), flags);
    lua_pushboolean(L, !commander || made >= 0);
    return 1;
}

// Why coe5.planRitual refuses its call, or NULL.
static const char *ritualPlanRefusal(int inAiSite, lua_Integer commander, int owner, int controller, int plan, int known) {
    const char *reason = ritualPlanScrapRefusal(inAiSite, commander, owner, controller, 0);
    if (reason) return reason;
    if (plan != 0) return "has_plan";
    if (!known) return "not_known";
    return NULL;
}

// coe5.planRitual(commander, ritual): gives an AI commander with no plan a ritual to cast.
static int lPlanRitual(lua_State *L) {
    lua_Integer commander = luaL_checkinteger(L, 1), ritual = luaL_checkinteger(L, 2);
    int owner = -1, controller = CONTROLLER_REMOVED, plan = 0, known = 0;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        }
        plan = G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE);
        for (int i = 0; i < COMMANDER_KNOWN_RITUALS_MAX && ritual > 0; i++) {
            int slot = G16(RVA_COMMANDER_KNOWN_RITUALS + (size_t)commander * COMMANDER_SIZE + (size_t)i * 2);
            if (slot < 1) break;
            if (slot == ritual) { known = 1; break; }
        }
    }
    LuaScript *s = luaScriptOf(L);
    const char *reason = ritualPlanRefusal(aiSiteRunning(), commander, owner, controller, plan, known);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.planRitual", reason, "commander=%lld ritual=%lld player=%d controller=%d plan=%d",
                      (long long)commander, (long long)ritual, owner, controller, plan);
        lua_pushboolean(L, 0);
        return 1;
    }
    G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE) = (short)ritual;
    writeLog("ai_plan_set", "mod=%s commander=%lld player=%d ritual=%lld", s ? s->mod : "?", (long long)commander, owner, (long long)ritual);
    lua_pushboolean(L, 1);
    return 1;
}

// coe5.terrainMatch(code, x, y): whether square (x, y) counts for terrain code `code`.
static int lTerrainMatch(lua_State *L) {
    lua_Integer code = luaL_checkinteger(L, 1), x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    if (code < TERRAIN_MATCH_CODE_MIN || code >= TERRAIN_TABLE_MAX) {
        lua_pushnil(L);
        return 1;
    }
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, ritualTerrainAt((int)code, (int)x, (int)y) > 0);
    return 1;
}

// ---- The trace: coe5.keepTraceLines -------------------------------------------------------------
// coe5.keepTraceLines(words): the game writes only trace lines that start with one of the words.
static int lKeepTraceLines(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    if (lua_isnoneornil(L, 1)) {
        setTraceKeepList(NULL, NULL, 0);
        writeLog("trace_filter_set", "mod=%s words=0", mod);
        lua_pushboolean(L, 1);
        return 1;
    }
    if (!lua_istable(L, 1)) {
        refuseLuaCall(mod, "coe5.keepTraceLines", "not_a_list", "type=%s", luaL_typename(L, 1));
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_Unsigned n = lua_rawlen(L, 1);
    coe5seChargeLuaSteps(L, (size_t)n);
    if (n > INT_MAX) n = INT_MAX;
    const char **words = n ? calloc((size_t)n, sizeof *words) : NULL;
    size_t *lengths = n ? calloc((size_t)n, sizeof *lengths) : NULL;
    const char *reason = n && (!words || !lengths) ? "out_of_memory" : NULL;
    lua_Unsigned bad = 0;
    for (lua_Unsigned i = 1; !reason && i <= n; i++) {
        if (lua_rawgeti(L, 1, (lua_Integer)i) != LUA_TSTRING) reason = "not_a_string";
        else reason = traceKeepWordProblem(words[i - 1] = lua_tolstring(L, -1, &lengths[i - 1]), lengths[i - 1]);
        if (reason) bad = i;
        lua_pop(L, 1);
    }
#ifdef __APPLE__
    if (!reason && n) reason = "windows_only";   // @platform-split trace_filter: Windows only; the Mac trace functions weren't read for it
#else
    if (!reason && n && !installTraceFilter()) reason = "site_unavailable";
#endif
    if (!reason && !setTraceKeepList(words, lengths, (int)n)) reason = "out_of_memory";
    free(words);
    free(lengths);
    if (reason) {
        refuseLuaCall(mod, "coe5.keepTraceLines", reason, "words=%llu entry=%llu", (unsigned long long)n, (unsigned long long)bad);
        lua_pushboolean(L, 0);
        return 1;
    }
    writeLog("trace_filter_set", "mod=%s words=%llu", mod, (unsigned long long)n);
    lua_pushboolean(L, 1);
    return 1;
}

// ---- Units: coe5.spawnUnit, coe5.spawnCommander, coe5.monsterVariant ----------------------------
// A script makes units the way the game's recruit purchase and summons do.
#define SPAWN_SIGHT_RADIUS 3

// Whether `monster` is a record the game makes units of: 0 to the last monster, with a name.
static int isUsedMonster(lua_Integer monster) {
    return monster >= 0 && monster <= G32(RVA_MONSTER_COUNT) && monster < MONSTER_TABLE_MAX &&
           gameBase[RVA_MONSTER_TABLE + (size_t)monster * MONSTER_SIZE];
}

// Why a spawn of `monster` for `player` on (x, y) can't be made, or NULL.
static const char *spawnProblem(lua_Integer player, lua_Integer monster, lua_Integer x, lua_Integer y) {
    if (G32(RVA_ACTIVE_PLAYER) < 0) return "no_game";
    if (player < 0 || player >= PLAYER_SLOTS) return "bad_player";
    if (!isUsedMonster(monster)) return "bad_monster";
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES)
        return "off_map";
    return NULL;
}

// What the game does after making a unit.
static void settleSpawnedUnit(int player, int unit, int x, int y) {
    GAME_FN(void (*)(int), RVA_UNIT_FIT_TERRAIN)(unit);
    GAME_FN(void (*)(int, int), RVA_REBUILD_SQUARE_ARMY)(x, y);
    GAME_FN(int (*)(int, int, int), RVA_CONQUER_CHECK)(x, y, 0);
    GAME_FN(void (*)(int, int, int, int, int), RVA_UPDATE_SIGHT)(player, x - SPAWN_SIGHT_RADIUS, y - SPAWN_SIGHT_RADIUS,
                                                                 x + SPAWN_SIGHT_RADIUS, y + SPAWN_SIGHT_RADIUS);
    GAME_FN(void (*)(void), RVA_RESET_SQUARE_CACHE)();
}

// coe5.spawnUnit and coe5.spawnCommander: one unit, or one commander with its unit, on (x, y).
static int luaSpawn(lua_State *L, int commander) {
    const char *call = commander ? "coe5.spawnCommander" : "coe5.spawnUnit";
    lua_Integer player = luaL_checkinteger(L, 1), monster = luaL_checkinteger(L, 2);
    lua_Integer x = luaL_checkinteger(L, 3), y = luaL_checkinteger(L, 4);
    const char *problem = spawnProblem(player, monster, x, y);
    if (problem) {
        refuseLuaCall(runningLuaMod(), call, problem, "player=%lld monster=%lld x=%lld y=%lld", (long long)player,
                      (long long)monster, (long long)x, (long long)y);
        lua_pushnil(L);
        return 1;
    }
    int result, unit;
    if (commander) {
        result = GAME_FN(int (*)(int, int, int, int), RVA_NEW_COMMANDER)((int)x, (int)y, (int)player, (int)monster);
        unit = result >= 0 && result <= G32(RVA_LAST_COMMANDER) ? G32(RVA_COMMANDERS + (size_t)result * COMMANDER_SIZE) : -1;
    } else {
        result = unit = GAME_FN(int (*)(int, int, int, int, int), RVA_NEW_UNIT)((int)x, (int)y, (int)player, -1, (int)monster);
    }
    if (result < 0 || unit < 0) {
        refuseLuaCall(runningLuaMod(), call, "no_free_slot", "player=%lld monster=%lld x=%lld y=%lld", (long long)player,
                      (long long)monster, (long long)x, (long long)y);
        lua_pushnil(L);
        return 1;
    }
    settleSpawnedUnit((int)player, unit, (int)x, (int)y);
    lua_pushinteger(L, result);
    return 1;
}
static int lSpawnUnit(lua_State *L) { return luaSpawn(L, 0); }
static int lSpawnCommander(lua_State *L) { return luaSpawn(L, 1); }

// coe5.monsterVariant(monster): what the recruit purchase makes for an offer of `monster`.
static int lMonsterVariant(lua_State *L) {
    lua_Integer monster = luaL_checkinteger(L, 1);
    if (!isUsedMonster(monster)) {
        refuseLuaCall(runningLuaMod(), "coe5.monsterVariant", "bad_monster", "monster=%lld", (long long)monster);
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, GAME_FN(int (*)(int), RVA_MONSTER_VARIANT)((int)monster));
    return 1;
}

// ---- Data files: coe5.dataFolder, readIni, writeIni, listIni, deleteIni ------------------------
// A script's INI-shaped data files.

static LuaScript *checkLuaScript(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    if (!s) luaL_error(L, "not called from a mod's script");
    return s;
}

// Writes the path of script s's area to `out`.
static int dataAreaPath(LuaScript *s, const char *call, char *out, size_t size) {
    const char *problem = storageAreaProblem(s->mod);
    if (problem) {
        refuseLuaCall(s->mod, call, problem, "area=%s", s->mod);
        return 0;
    }
    if (!buildPath(out, size, call, "%s" USER_DIR_SEP STORAGE_DATA_DIR USER_DIR_SEP "%s", userDir(), s->mod)) {
        refuseLuaCall(s->mod, call, "path_too_long", "area=%s", s->mod);
        return 0;
    }
    char real[MAX_PATH];
    if (storageStoredName(out, real, sizeof real) && strcmp(real, s->mod)) {
        refuseLuaCall(s->mod, call, "area_taken", "area=%s real=%s", s->mod, real);
        return 0;
    }
    return 1;
}

// Writes the path of the script's data folder, or of file `name`.ini in it.
static int dataPath(LuaScript *s, const char *call, const char *name, char *out, size_t size) {
    char area[MAX_PATH];
    if (!dataAreaPath(s, call, area, sizeof area)) return 0;
    const char *problem;
    if (name && (problem = storageNameProblem(name))) {
        refuseLuaCall(s->mod, call, problem, "name=%s", name);
        return 0;
    }
    const char *sep = s->dataFolder[0] ? USER_DIR_SEP : "";
    int ok = name ? buildPath(out, size, call, "%s%s%s" USER_DIR_SEP "%s.ini", area, sep, s->dataFolder, name)
                  : buildPath(out, size, call, "%s%s%s", area, sep, s->dataFolder);
    if (!ok) refuseLuaCall(s->mod, call, "path_too_long", "folder=%s name=%s", s->dataFolder, name ? name : "");
    return ok;
}

// The bytes of every file in folder `dir` and its subfolders.
static long long dataFolderBytes(const char *dir, int depth) {
    char pattern[MAX_PATH], path[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    long long total = 0;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        if (!buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "%s", dir, fd.cFileName)) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (depth > 0) total += dataFolderBytes(path, depth - 1);
        } else {
            total += storageFileBytes(path);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return total;
}
// What script s's area holds now.
static long long dataAreaBytes(LuaScript *s, const char *call) {
    if (s->dataBytesKnown) return s->dataBytes;
    char area[MAX_PATH];
    if (!dataAreaPath(s, call, area, sizeof area)) return -1;
    s->dataBytes = dataFolderBytes(area, 4);
    s->dataBytesKnown = 1;
    writeLog("lua_data_area_size", "mod=%s bytes=%lld max=%lld", s->mod, s->dataBytes, storageAreaMax(s->mod));
    return s->dataBytes;
}
// Makes the folders down to the script's data folder: coe5se_data, the area, and the subfolder.
static void makeDataFolders(LuaScript *s, const char *area) {
    char path[MAX_PATH];
    if (buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP STORAGE_DATA_DIR, userDir())) CreateDirectoryA(path, NULL);
    CreateDirectoryA(area, NULL);
    if (s->dataFolder[0] && buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "%s", area, s->dataFolder))
        CreateDirectoryA(path, NULL);
}

// The folders an older CoE5SE kept scripts' files in.
static int luaScriptsLoaded;
#define OLD_DATA_FILES_MAX 4096

// Writes the folder an older CoE5SE kept script s's files in, by its name in the user folder.
static void oldDataFolderName(const LuaScript *s, char *out, size_t size) {
    snprintf(out, size, "%.63s", s->dataFolder[0] ? s->dataFolder : s->mod);
}

// Returns NULL when script s may take folder `old` of the user folder, or why not.
static const char *oldDataFolderProblem(const LuaScript *s, const char *old) {
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (_stricmp(modName(m), s->mod) && !_stricmp(modName(m), old)) return "another_mods_name";
    char other[STORAGE_NAME_MAX + 1];
    for (int i = 0; i < luaScriptCount; i++) {
        if (luaScripts[i] == s) continue;
        oldDataFolderName(luaScripts[i], other, sizeof other);
        if (!_stricmp(other, old)) return "folder_shared";
    }
    return NULL;
}

// Moves script s's .ini files from the folder an older CoE5SE kept them in to its data folder now.
static void moveOldDataFolder(LuaScript *s) {
    char old[STORAGE_NAME_MAX + 1], from[MAX_PATH], real[MAX_PATH];
    oldDataFolderName(s, old, sizeof old);
    if (storageNameProblem(old) || storageIsGameFolder(old)) return;
    if (!buildPath(from, sizeof from, __func__, "%s" USER_DIR_SEP "%s", userDir(), old)) return;
    DWORD attributes = GetFileAttributesA(from);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return;
    const char *problem = !storageStoredName(from, real, sizeof real) || strcmp(real, old) ? "folder_alias" : oldDataFolderProblem(s, old);
    if (problem) {
        writeLog("lua_data_move_skipped", "mod=%s from=%s reason=%s", s->mod, old, problem);
        return;
    }
    char area[MAX_PATH], to[MAX_PATH], pattern[MAX_PATH];
    if (!dataAreaPath(s, "move_old_data", area, sizeof area) || !dataPath(s, "move_old_data", NULL, to, sizeof to)) return;
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", from)) return;
    typedef char OldFileName[STORAGE_NAME_MAX + 5];
    OldFileName *names = malloc(OLD_DATA_FILES_MAX * sizeof *names);
    if (!names) return;
    int count = 0, moved = 0, left = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            size_t length = strlen(fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || length <= 4 || length >= sizeof *names || _stricmp(fd.cFileName + length - 4, ".ini"))
                continue;
            char stem[sizeof *names];
            snprintf(stem, sizeof stem, "%.*s", (int)(length - 4), fd.cFileName);
            if (storageNameProblem(stem)) continue;
            if (count == OLD_DATA_FILES_MAX) {
                left++;
                continue;
            }
            snprintf(names[count++], sizeof *names, "%s", fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    if (count) makeDataFolders(s, area);
    long long room = storageAreaMax(s->mod) - dataFolderBytes(area, 4);
    char source[MAX_PATH], target[MAX_PATH];
    for (int i = 0; i < count; i++) {
        if (!buildPath(source, sizeof source, __func__, "%s" USER_DIR_SEP "%s", from, names[i]) ||
            !buildPath(target, sizeof target, __func__, "%s" USER_DIR_SEP "%s", to, names[i])) {
            left++;
            continue;
        }
        long long bytes = storageFileBytes(source);
        if (bytes > room || !storageMoveFile(source, target)) {
            left++;
            continue;
        }
        room -= bytes;
        moved++;
    }
    free(names);
    int removed = storageRemoveEmptyFolder(from);
    s->dataBytesKnown = 0;
    if (moved || left)
        writeLog("lua_data_moved", "mod=%s from=%s to=%s files=%d left=%d old_folder_removed=%d", s->mod, old,
                 s->dataFolder[0] ? s->dataFolder : "-", moved, left, removed);
}

// Moves every script's older data folder, once all have loaded.
static void moveOldDataFolders(void) {
    luaScriptsLoaded = 1;
    for (int i = 0; i < luaScriptCount; i++)
        if (!luaScripts[i]->dead) moveOldDataFolder(luaScripts[i]);
}

// Argument `arg` as a data file or folder name.
static const char *luaStorageName(lua_State *L, int arg, const char *call, const char *field) {
    size_t length;
    const char *name = luaL_checklstring(L, arg, &length);
    if (strlen(name) == length) return name;
    LuaScript *s = luaScriptOf(L);
    refuseLuaCall(s ? s->mod : "?", call, "bad_character", "%s=%s", field, name);
    return NULL;
}

// coe5.dataFolder(name): names the subfolder of the mod's area that the script's data files go in.
static int lDataFolder(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.dataFolder", "folder");
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    const char *problem = storageNameProblem(name);
    if (problem) {
        refuseLuaCall(s->mod, "coe5.dataFolder", problem, "folder=%s", name);
        lua_pushboolean(L, 0);
        return 1;
    }
    snprintf(s->dataFolder, sizeof s->dataFolder, "%s", name);
    writeLog("lua_data_folder", "mod=%s folder=%s", s->mod, name);
    if (luaScriptsLoaded) moveOldDataFolder(s);
    lua_pushboolean(L, 1);
    return 1;
}

// iniParse's callback for coe5.readIni: puts one value in the result table on top of the stack.
static void readIniPair(void *ctx, const char *section, const char *key, const char *value) {
    lua_State *L = ctx;
    if (lua_getfield(L, -1, section) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, section);
    }
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 1);
}

// coe5.readIni(name): the file's table, or nil when there is no such file.
static int lReadIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.readIni", "name");
    if (!name) {
        lua_pushnil(L);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    char path[MAX_PATH];
    if (!dataPath(s, "coe5.readIni", name, path, sizeof path)) {
        lua_pushnil(L);
        return 1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        lua_pushnil(L);
        return 1;
    }
    long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    if (size < 0 || size > STORAGE_FILE_MAX) {
        fclose(f);
        refuseLuaCall(s->mod, "coe5.readIni", "too_big", "name=%s bytes=%ld max=%d", name, size, STORAGE_FILE_MAX);
        lua_pushnil(L);
        return 1;
    }
    rewind(f);
    char *text = lua_newuserdatauv(L, (size_t)size + 1, 0);
    size_t got = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[got] = '\0';
    lua_newtable(L);
    int firstBad, bad = iniParse(text, readIniPair, L, &firstBad);
    if (bad) refuseLuaCall(s->mod, "coe5.readIni", "bad_lines", "name=%s lines=%d first=%d", name, bad, firstBad);
    return 1;
}

static int compareNames(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

// Pushes the sorted string keys of the table at `t`.
static const char **sortedKeys(lua_State *L, int t, int *count) {
    t = lua_absindex(L, t);
    int n = 0;
    *count = 0;
    for (lua_pushnil(L); lua_next(L, t); lua_pop(L, 1)) {
        size_t len;
        if (lua_type(L, -2) != LUA_TSTRING || strlen(lua_tolstring(L, -2, &len)) != len) {
            lua_pop(L, 2);
            return NULL;
        }
        n++;
    }
    const char **keys = lua_newuserdatauv(L, sizeof *keys * (size_t)(n ? n : 1), 0);
    int i = 0;
    for (lua_pushnil(L); lua_next(L, t); lua_pop(L, 1)) {
        if (i == n || lua_type(L, -2) != LUA_TSTRING) {
            lua_pop(L, 3);
            *count = -1;
            return NULL;
        }
        keys[i++] = lua_tostring(L, -2);
    }
    if (i != n) {
        lua_pop(L, 1);
        *count = -1;
        return NULL;
    }
    qsort(keys, (size_t)n, sizeof *keys, compareNames);
    *count = n;
    return keys;
}

// Goes over coe5.writeIni's table (argument 2) in sorted order.
static const char *iniTableText(lua_State *L, FILE *out, size_t *total, char *detail, size_t detailSize) {
    int sectionCount, keyCount;
    const char **sections = sortedKeys(L, 2, &sectionCount);
    if (!sections) {
        snprintf(detail, detailSize, "section=(%s)", sectionCount < 0 ? "changed while read" : "not a string");
        return sectionCount < 0 ? "table_changed" : "bad_section";
    }
    const char *reason = NULL;
    for (int i = 0; i < sectionCount && !reason; i++) {
        const char *sec = sections[i], *problem = iniNameProblem(sec, 0);
        if (problem || lua_getfield(L, 2, sec) != LUA_TTABLE) {
            snprintf(detail, detailSize, "section=%.80s problem=%s", sec, problem ? problem : "not_a_table");
            if (!problem) lua_pop(L, 1);
            reason = "bad_section";
            break;
        }
        if (out && sec[0]) fprintf(out, "%s[%s]\n", i ? "\n" : "", sec);
        *total += strlen(sec) + 4;
        const char **keys = sortedKeys(L, -1, &keyCount);
        if (!keys) {
            snprintf(detail, detailSize, "section=%.80s key=(%s)", sec, keyCount < 0 ? "changed while read" : "not a string");
            lua_pop(L, 1);
            reason = keyCount < 0 ? "table_changed" : "bad_key";
            break;
        }
        for (int k = 0; k < keyCount && !reason; k++) {
            if ((problem = iniNameProblem(keys[k], 1))) {
                snprintf(detail, detailSize, "section=%.80s key=%.80s problem=%s", sec, keys[k], problem);
                reason = "bad_key";
                break;
            }
            int type = lua_getfield(L, -2, keys[k]);
            size_t len = 0;
            const char *value = type == LUA_TSTRING || type == LUA_TNUMBER ? lua_tolstring(L, -1, &len) : NULL;
            if (!value || (problem = iniValueProblem(value, len))) {
                snprintf(detail, detailSize, "section=%.80s key=%.80s problem=%s", sec, keys[k], value ? problem : "not_text");
                reason = "bad_value";
            } else {
                if (out) fprintf(out, "%s=%s\n", keys[k], value);
                *total += strlen(keys[k]) + len + 2;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
    }
    lua_pop(L, 1);
    return reason;
}

// One coe5.writeIni call.
typedef struct {
    LuaScript *s;
    const char *area, *path;
    long long areaBytes, oldBytes;
    char fresh[MAX_PATH], detail[256];
    const char *reason;
    FILE *f;
    size_t total;
    int written;
} IniWrite;

// lWriteIni's two passes, under lua_pcall: argument 1 the IniWrite, 2 the table.
static int writeIniPasses(lua_State *L) {
    IniWrite *w = lua_touserdata(L, 1);
    w->reason = iniTableText(L, NULL, &w->total, w->detail, sizeof w->detail);
    if (!w->reason && w->total > STORAGE_FILE_MAX) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu max=%d", w->total, STORAGE_FILE_MAX);
        w->reason = "too_big";
    }
    if (w->reason) return 0;
    long long max = storageAreaMax(w->s->mod);
    if (w->areaBytes >= 0 && w->areaBytes - w->oldBytes + (long long)w->total > max) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu area=%lld max=%lld", w->total, w->areaBytes - w->oldBytes, max);
        w->reason = "data_quota";
        return 0;
    }
    makeDataFolders(w->s, w->area);
    if (!(w->f = fopen(w->fresh, "wb"))) return 0;
    size_t again = 0;
    w->reason = iniTableText(L, w->f, &again, w->detail, sizeof w->detail);
    if (!w->reason && again != w->total) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu then=%zu", w->total, again);
        w->reason = "table_changed";
    }
    int failed = ferror(w->f);
    if (fclose(w->f)) failed = 1;
    w->f = NULL;
    w->written = !w->reason && !failed;
    return 0;
}

// coe5.writeIni(name, t): writes t[section][key] = value as <name>.ini.
static int lWriteIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.writeIni", "name");
    luaL_checktype(L, 2, LUA_TTABLE);
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_settop(L, 2);
    LuaScript *s = checkLuaScript(L);
    char area[MAX_PATH], path[MAX_PATH];
    if (!dataAreaPath(s, "coe5.writeIni", area, sizeof area) || !dataPath(s, "coe5.writeIni", name, path, sizeof path)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    IniWrite w = {.s = s, .area = area, .path = path, .areaBytes = dataAreaBytes(s, "coe5.writeIni"), .oldBytes = storageFileBytes(path)};
    if (!buildPath(w.fresh, sizeof w.fresh, "coe5.writeIni", "%s.new", path)) {
        refuseLuaCall(s->mod, "coe5.writeIni", "path_too_long", "folder=%s name=%s", s->dataFolder, name);
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_gc(L, LUA_GCSTOP);
    lua_pushcfunction(L, writeIniPasses);
    lua_pushlightuserdata(L, &w);
    lua_pushvalue(L, 2);
    int rc = lua_pcall(L, 2, 0, 0);
    lua_gc(L, LUA_GCRESTART);
    if (w.f) fclose(w.f);
    if (rc != LUA_OK || !w.written) remove(w.fresh);
    if (rc != LUA_OK) return lua_error(L);
    if (w.reason) {
        if (!strcmp(w.reason, "data_quota")) s->noticesDue |= LUA_NOTICE_STORAGE;
        refuseLuaCall(s->mod, "coe5.writeIni", w.reason, "name=%s %s", name, w.detail);
        lua_pushboolean(L, 0);
        return 1;
    }
#ifdef _WIN32
    int replaced = w.written && MoveFileExA(w.fresh, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    int replaced = w.written && !rename(w.fresh, path);
#endif
    if (!replaced) {
        remove(w.fresh);
        refuseLuaCall(s->mod, "coe5.writeIni", "write_failed", "name=%s path=%s", name, path);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (s->dataBytesKnown) s->dataBytes += storageFileBytes(path) - w.oldBytes;
    writeLog("lua_data_file_written", "mod=%s name=%s bytes=%zu", s->mod, name, w.total);
    lua_pushboolean(L, 1);
    return 1;
}

// coe5.listIni(): the names of the script's data files, without .ini, sorted.
static int lListIni(lua_State *L) {
    LuaScript *s = checkLuaScript(L);
    char folder[MAX_PATH], pattern[MAX_PATH];
    lua_newtable(L);
    if (!dataPath(s, "coe5.listIni", NULL, folder, sizeof folder)) return 1;
    if (!buildPath(pattern, sizeof pattern, "coe5.listIni", "%s" USER_DIR_SEP "*.ini", folder)) {
        refuseLuaCall(s->mod, "coe5.listIni", "path_too_long", "folder=%s", s->dataFolder);
        return 1;
    }
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 1;
    int n = 0;
    do {
        size_t len = strlen(fd.cFileName);
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || len <= 4 || _stricmp(fd.cFileName + len - 4, ".ini")) continue;
        fd.cFileName[len - 4] = '\0';
        if (storageNameProblem(fd.cFileName)) continue;
        lua_pushstring(L, fd.cFileName);
        lua_rawseti(L, -2, ++n);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    const char **names = lua_newuserdatauv(L, sizeof *names * (size_t)(n ? n : 1), 0);
    for (int i = 0; i < n; i++) {
        lua_rawgeti(L, -2, i + 1);
        names[i] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    qsort(names, (size_t)n, sizeof *names, compareNames);
    lua_createtable(L, n, 0);
    for (int i = 0; i < n; i++) {
        lua_pushstring(L, names[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// coe5.deleteIni(name): deletes <name>.ini.
static int lDeleteIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.deleteIni", "name");
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    char path[MAX_PATH];
    if (!dataPath(s, "coe5.deleteIni", name, path, sizeof path)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    long long bytes = storageFileBytes(path);
    if (remove(path) == 0) {
        if (s->dataBytesKnown) s->dataBytes -= bytes;
        writeLog("lua_data_file_deleted", "mod=%s name=%s", s->mod, name);
        lua_pushboolean(L, 1);
        return 1;
    }
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
        refuseLuaCall(s->mod, "coe5.deleteIni", "delete_failed", "name=%s path=%s", name, path);
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 0);
    return 1;
}

// coe5.ui: the game's widgets, the same calls as the API's ui table.
static int monsterRowIndex;
static int lScreenBegin(lua_State *L) {
    monsterRowIndex = 0;
    luaApi->ui->screenBegin(luaL_checkstring(L, 1));
    return 0;
}
static int lSectionBegin(lua_State *L) { luaApi->ui->sectionBegin(luaL_optstring(L, 1, NULL)); return 0; }
static int lSectionEnd(lua_State *L) { (void)L; luaApi->ui->sectionEnd(); return 0; }
static int lText(lua_State *L) { luaApi->ui->text((int)luaL_checkinteger(L, 1), luaL_checkstring(L, 2)); return 0; }
static int lChoice(lua_State *L) { luaApi->ui->choice((int)luaL_checkinteger(L, 1), lua_toboolean(L, 2), luaL_checkstring(L, 3)); return 0; }
// coe5.ui.checkbox(id, checked, label).
typedef struct { LuaScript *s; int id; int *value; } LuaCheckbox;
static LuaCheckbox *luaCheckboxes;
static int luaCheckboxCount, luaCheckboxCapacity;
static int *luaCheckboxValue(LuaScript *s, int id) {
    for (int i = 0; i < luaCheckboxCount; i++)
        if (luaCheckboxes[i].s == s && luaCheckboxes[i].id == id) return luaCheckboxes[i].value;
    int *value = malloc(sizeof *value);
    if (!value || !growList((void **)&luaCheckboxes, &luaCheckboxCapacity, luaCheckboxCount + 1, sizeof *luaCheckboxes)) {
        free(value);
        return NULL;
    }
    luaCheckboxes[luaCheckboxCount++] = (LuaCheckbox){s, id, value};
    return value;
}
static int lCheckbox(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1), checked = lua_toboolean(L, 2);
    const char *label = luaL_checkstring(L, 3);
    LuaScript *s = checkLuaScript(L);
    int *value = luaCheckboxValue(s, id);
    if (!value) {
        refuseLuaCall(s->mod, "coe5.ui.checkbox", "out_of_memory", "id=%d", id);
        return 0;
    }
    *value = checked;
    luaApi->ui->checkbox(id, value, label);
    return 0;
}
static int lTooltip(lua_State *L) { luaApi->ui->tooltip(luaL_checkstring(L, 1)); return 0; }
static int lButtonRow(lua_State *L) { (void)L; luaApi->ui->buttonRow(); return 0; }
static int lListBegin(lua_State *L) { (void)L; luaApi->ui->listBegin(); return 0; }
static int lListRow(lua_State *L) { luaApi->ui->listRow((int)luaL_checkinteger(L, 1), luaL_checkstring(L, 2)); return 0; }
static int lListEnd(lua_State *L) { (void)L; luaApi->ui->listEnd(); return 0; }
// The game's bind_hotkey takes an int id and key.
static int fitsInt(lua_Integer v) { return v >= INT_MIN && v <= INT_MAX; }
static int lBindEscape(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1);
    if (!fitsInt(id)) {
        refuseLuaCall(runningLuaMod(), "coe5.ui.bindEscape", "out_of_range", "id=%lld", (long long)id);
        return 0;
    }
    luaApi->ui->bindKey((int)id, UI_KEY_ESCAPE);
    return 0;
}
// coe5.ui.bindKey(id, key): a key clicks line `id`.
static int lBindKey(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1), key = luaL_checkinteger(L, 2);
    if (!fitsInt(id) || !fitsInt(key)) {
        refuseLuaCall(runningLuaMod(), "coe5.ui.bindKey", "out_of_range", "id=%lld key=%lld", (long long)id, (long long)key);
        return 0;
    }
    luaApi->ui->bindKey((int)id, key == KEY_ESC ? UI_KEY_ESCAPE : (int)key);
    return 0;
}
// A screen loop is one long call into the script, so the step count starts again every frame.
static int lWait(lua_State *L) {
    checkLuaMainThread(L, "coe5.ui.wait");
    luaCodeRunning--;
    int id = luaApi->ui->wait();
    luaCodeRunning++;
    resetLuaSteps(L);
    lua_pushinteger(L, id);
    return 1;
}
// The game's layout calls, for screens laid out another way and for wraps.
static int lSectionHeader(lua_State *L) { sectionHeader(luaL_checkstring(L, 1)); return 0; }
// Argument 1 as a float for the game's layout.
static int luaLayoutFloat(lua_State *L, const char *call, const char *field, int negativeOk, float *out) {
    lua_Number value = luaL_checknumber(L, 1);
    LuaScript *s = luaScriptOf(L);
    const char *reason = !isfinite(value) || fabs(value) > FLT_MAX ? "not_finite" : !negativeOk && value < 0 ? "negative" : NULL;
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", call, reason, "%s=%g", field, (double)value);
        return 0;
    }
    *out = (float)value;
    return 1;
}
static int lScreenWidth(lua_State *L) {
    float width;
    if (luaLayoutFloat(L, "coe5.ui.screenWidth", "width", 0, &width)) screenWidth(width);
    return 0;
}
static int lSetupScreenWidth(lua_State *L) { lua_pushnumber(L, setupScreenWidth()); return 1; }
static int lScreenMusic(lua_State *L) { (void)L; screenMusic(); return 0; }
static int lColumnX(lua_State *L) {
    float x;
    if (!luaLayoutFloat(L, "coe5.ui.columnX", "x", 1, &x)) return 0;
#ifdef __APPLE__
    columnX(x);
#else
    GAME_FN(void (*)(float), RVA_COLUMN_X)(x);
#endif
    return 0;
}

// Arguments first to last of a draw call as floats. Returns 0, with lua_refused logged for `call`, when
// one is NaN, an infinity, or past FLT_MAX: the GL calls would take them as they are.
static int luaDrawFloats(lua_State *L, const char *call, int first, int last, float *out) {
    for (int i = first; i <= last; i++) {
        lua_Number value = luaL_checknumber(L, i);
        if (!isfinite(value) || fabs(value) > FLT_MAX) {
            LuaScript *s = luaScriptOf(L);
            refuseLuaCall(s ? s->mod : "?", call, "not_finite", "argument=%d value=%g", i, (double)value);
            return 0;
        }
        out[i - first] = (float)value;
    }
    return 1;
}

// Whether a draw call may draw now: only inside MAP_EDITOR_DRAW's chain, where the frame's GL state is
// the GUI's. Logs lua_refused reason=not_drawing otherwise.
static int luaMayDraw(lua_State *L, const char *call) {
    if (mapEditorDrawing > 0) return 1;
    LuaScript *s = luaScriptOf(L);
    refuseLuaCall(s ? s->mod : "?", call, "not_drawing", "site=MAP_EDITOR_DRAW");
    return 0;
}

// coe5.ui.drawLine(x1, y1, x2, y2, r, g, b, alpha[, width]): one line at UI coordinates, `width`
// window pixels wide (1 when left out), in MAP_EDITOR_DRAW.
static int lDrawLine(lua_State *L) {
    float f[9];
    f[8] = 1.0f;
    if (!luaDrawFloats(L, "coe5.ui.drawLine", 1, 8, f)) return 0;
    if (!lua_isnoneornil(L, 9) && !luaDrawFloats(L, "coe5.ui.drawLine", 9, 9, f + 8)) return 0;
    if (f[8] <= 0) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.ui.drawLine", "out_of_range", "width=%g", (double)f[8]);
        return 0;
    }
    if (!luaMayDraw(L, "coe5.ui.drawLine")) return 0;
    drawUiLine(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
    return 0;
}

// coe5.ui.drawText(x, y, size, text, r, g, b, alpha): one line of text in the GUI font at UI
// coordinates, in MAP_EDITOR_DRAW. The text is cut to the game's line buffer.
#define DRAW_TEXT_SIZE 500
static int lDrawText(lua_State *L) {
    float pos[3], colour[4];
    const char *text = luaL_checkstring(L, 4);
    if (!luaDrawFloats(L, "coe5.ui.drawText", 1, 3, pos) || !luaDrawFloats(L, "coe5.ui.drawText", 5, 8, colour)) return 0;
    if (!luaMayDraw(L, "coe5.ui.drawText")) return 0;
    char cut[DRAW_TEXT_SIZE];
    text = fitGameText((void *)lDrawText, "coe5.ui.drawText", text, cut, sizeof cut);
    drawUiText(pos[0], pos[1], pos[2], text, colour[0], colour[1], colour[2], colour[3]);
    return 0;
}

// coe5.ui.nameBox(title, prompt, sprite[, maxLength]): the game's "Enter name" box.
#define NAME_BOX_MONSTER_SPRITE 0x21
static int lNameBox(lua_State *L) {
    const char *title = luaL_checkstring(L, 1), *prompt = luaL_checkstring(L, 2);
    lua_Integer wanted = luaL_checkinteger(L, 3);
    lua_Integer max = luaL_optinteger(L, 4, 32);
    luaL_argcheck(L, max >= 1 && max <= NAME_BOX_LENGTH_MAX, 4, "maxLength must be 1 to 42");
    checkLuaMainThread(L, "coe5.ui.nameBox");
    int held = 0, last = G32(RVA_MONSTER_COUNT);
    for (int m = 0; m <= last && m < MONSTER_TABLE_MAX && !held; m++) held = G32(RVA_MONSTER_SPRITES + (size_t)m * MONSTER_SIZE) == wanted;
    if (!held) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.ui.nameBox", "bad_sprite", "sprite=%lld", (long long)wanted);
        lua_pushnil(L);
        return 1;
    }
    int sprite = (int)wanted;
    char titleCut[SCREEN_TITLE_TEXT_SIZE], promptCut[NAME_BOX_PROMPT_SIZE];
    title = fitGameText((void *)lNameBox, "coe5.ui.nameBox", title, titleCut, sizeof titleCut);
    prompt = fitGameText((void *)lNameBox, "coe5.ui.nameBox", prompt, promptCut, sizeof promptCut);
    char name[NAME_BOX_LENGTH_MAX + 2] = "";
    luaCodeRunning--;
    int ok = runNameBox(NAME_BOX_MONSTER_SPRITE, sprite, name, (int)max, 1, title, prompt);
    luaCodeRunning++;
    resetLuaSteps(L);
    if (!ok) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, name);
    return 1;
}

// coe5.ui.monsterRow(id, monster, text[, line2]): a striped row with the monster's picture.
#define MONSTER_ROW_STRIPE_WIDTH 200
#define MONSTER_ROW_TEXT_X       30.0f
#define MONSTER_ROW_SPRITE_FLAGS 4
#define MONSTER_ROW_TALL_GAP     6.0f
#define MONSTER_ROW_SUBTEXT_SCALE 0.8f
#define MONSTER_ROW_SUBTEXT_ALPHA 0.65f
#define TEXT_COLOUR_KEEP         -1.0f
#ifdef __APPLE__
static void monsterRowBegin(void) { rowBegin(1); }
static void monsterRowEnd(void) { rowBegin(0); }
static void monsterRowColumn(float x) { columnX(x); }
// The game's text scale setter.
static void setTextScale(float scale) { GF(RVA_COLUMN_BASE) = GF(RVA_TEXT_HEIGHT_BASE) * scale; }
#else
static void monsterRowBegin(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(0); }
static void monsterRowEnd(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(1); }
static void monsterRowColumn(float x) { GAME_FN(void (*)(float), RVA_COLUMN_X)(x); }
static void setTextScale(float scale) { GAME_FN(void (*)(float), RVA_SET_TEXT_SCALE)(scale); }
#endif
// The text scale now, so a row puts back what it found.
static float textScaleNow(void) {
    float base = GF(RVA_TEXT_HEIGHT_BASE);
    return base > 0.0f ? GF(RVA_COLUMN_BASE) / base : 1.0f;
}
static int lMonsterRow(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1), monster = luaL_checkinteger(L, 2);
    const char *text = luaL_checkstring(L, 3), *line2 = luaL_optstring(L, 4, NULL);
    if (!fitsInt(id)) {
        refuseLuaCall(runningLuaMod(), "coe5.ui.monsterRow", "out_of_range", "id=%lld", (long long)id);
        return 0;
    }
    if (!isUsedMonster(monster)) {
        refuseLuaCall(runningLuaMod(), "coe5.ui.monsterRow", "bad_monster", "monster=%lld", (long long)monster);
        return 0;
    }
    int sprite = G32(RVA_MONSTER_SPRITES + (size_t)monster * MONSTER_SIZE);
    char cut[TEXT_BUTTON_TEXT_SIZE], cut2[TEXT_BUTTON_TEXT_SIZE];
    text = fitGameText((void *)lMonsterRow, "coe5.ui.monsterRow", text, cut, sizeof cut);
    int stripe = (monsterRowIndex++ & 1) + 1;
    monsterRowBegin();
    if (!line2) {
        GAME_FN(void (*)(int, int), RVA_ROW_STRIPE)(stripe, MONSTER_ROW_STRIPE_WIDTH);
        GAME_FN(void *(*)(int, int, int, int), RVA_SPRITE_BUTTON)((int)id, SPRITE_KIND_MONSTER, sprite, MONSTER_ROW_SPRITE_FLAGS);
        monsterRowColumn(MONSTER_ROW_TEXT_X);
        textButton((int)id, text);
        monsterRowEnd();
        return 0;
    }
    line2 = fitGameText((void *)lMonsterRow, "coe5.ui.monsterRow", line2, cut2, sizeof cut2);
    int width = (int)setupScreenWidth();
    GAME_FN(void (*)(int, int), RVA_ROW_STRIPE)(stripe, width);
    float scale = textScaleNow();
    setTextScale(scale * (1.0f + MONSTER_ROW_SUBTEXT_SCALE));
    GAME_FN(void *(*)(int, int, int, int), RVA_SPRITE_BUTTON)((int)id, SPRITE_KIND_MONSTER, sprite, MONSTER_ROW_SPRITE_FLAGS);
    setTextScale(scale);
    float textX = GF(RVA_COLUMN_BASE) * (1.0f + MONSTER_ROW_SUBTEXT_SCALE) + MONSTER_ROW_TALL_GAP;
    monsterRowColumn(textX);
    textButton((int)id, text);
    monsterRowEnd();
    monsterRowBegin();
    GAME_FN(void (*)(int, int), RVA_ROW_STRIPE)(stripe, width);
    monsterRowColumn(textX);
    setTextScale(scale * MONSTER_ROW_SUBTEXT_SCALE);
    GAME_FN(void (*)(int, float, float, float, float, FMT_ARGS), RVA_BUTTON_CORE)((int)id, TEXT_COLOUR_KEEP, TEXT_COLOUR_KEEP,
                                                                                  TEXT_COLOUR_KEEP, MONSTER_ROW_SUBTEXT_ALPHA, "%s", line2);
    setTextScale(scale);
    monsterRowEnd();
    return 0;
}

// A private build of CoE5SE may add coe5 functions.
#ifdef COE5SE_EXTRA_LUA_HEADER
#include COE5SE_EXTRA_LUA_HEADER
#else
#define COE5SE_EXTRA_LUA_FUNCTIONS
#endif

static const luaL_Reg LUA_COE5[] = {
    {"log", lLog}, {"addMainMenuButton", lAddMainMenuButton}, {"addGameMenuLine", lAddGameMenuLine},
    {"turn", lTurn}, {"sendMessage", lSendMessage}, {"get", lGet}, {"set", lSet}, {"wrap", lWrap},
    {"observe", lObserve}, {"setSetupAutomated", lSetSetupAutomated}, {"setupAutomated", lSetupAutomated},
    {"setting", lSetting}, {"restartGame", lRestartGame}, {"loadGame", lLoadGame}, {"restartPhase", lRestartPhase},
    {"hideMainMenuButton", lHideMainMenuButton}, {"addScreen", lAddScreen}, {"rebindKey", lRebindKey},
    {"addActionKey", lAddActionKey}, {"actionKey", lActionKey}, {"findMonster", lFindMonster},
    {"dataFolder", lDataFolder}, {"readIni", lReadIni}, {"writeIni", lWriteIni}, {"listIni", lListIni},
    {"deleteIni", lDeleteIni}, {"spawnUnit", lSpawnUnit}, {"spawnCommander", lSpawnCommander},
    {"monsterVariant", lMonsterVariant}, {"portalExits", lPortalExits}, {"squareUnits", lSquareUnits}, {"armies", lArmies}, {"route", lRoute},
    {"scrapRitualPlan", lScrapRitualPlan}, {"planRitual", lPlanRitual}, {"terrainMatch", lTerrainMatch},
    {"releaseFollower", lReleaseFollower}, {"transferFollower", lTransferFollower},
    {"shopOffers", lShopOffers}, {"classRecruits", lClassRecruits}, {"recruitSquares", lRecruitSquares},
    {"aiBuyOffer", lAiBuyOffer}, {"addMapEditorLine", lAddMapEditorLine}, {"mapView", lMapView},
    {"keepTraceLines", lKeepTraceLines}, COE5SE_EXTRA_LUA_FUNCTIONS {NULL, NULL},
};
static const luaL_Reg LUA_UI[] = {
    {"screenBegin", lScreenBegin}, {"sectionBegin", lSectionBegin}, {"sectionEnd", lSectionEnd}, {"text", lText},
    {"checkbox", lCheckbox}, {"choice", lChoice}, {"tooltip", lTooltip}, {"buttonRow", lButtonRow},
    {"listBegin", lListBegin}, {"listRow", lListRow}, {"listEnd", lListEnd}, {"bindEscape", lBindEscape},
    {"bindKey", lBindKey}, {"wait", lWait}, {"sectionHeader", lSectionHeader}, {"screenWidth", lScreenWidth},
    {"setupScreenWidth", lSetupScreenWidth}, {"screenMusic", lScreenMusic}, {"columnX", lColumnX},
    {"nameBox", lNameBox}, {"monsterRow", lMonsterRow}, {"drawLine", lDrawLine}, {"drawText", lDrawText}, {NULL, NULL},
};

// The game numbers, as coe5.<table>.<name>.
static const struct { const char *table, *name; int value; } GAME_NUMBERS[] = {
    {"mainMenu", "startNewGame", MAIN_MENU_START_NEW_GAME}, {"mainMenu", "loadGame", MAIN_MENU_LOAD_GAME},
    {"mainMenu", "preferences", MAIN_MENU_PREFERENCES}, {"mainMenu", "network", MAIN_MENU_NETWORK},
    {"mainMenu", "credits", MAIN_MENU_CREDITS}, {"mainMenu", "quit", MAIN_MENU_QUIT},
    {"mainMenu", "mods", MAIN_MENU_MODS}, {"mainMenu", "readTip", MAIN_MENU_READ_TIP},
    {"gameMenu", "stay", GAME_MENU_STAY}, {"gameMenu", "close", GAME_MENU_CLOSE}, {"gameMenu", "resign", GAME_MENU_RESIGN},
    {"gameMenuSection", "saving", GAME_MENU_SECTION_SAVING}, {"gameMenuSection", "settings", GAME_MENU_SECTION_SETTINGS},
    {"mapAction", "specialPower", MAP_ACTION_SPECIAL_POWER}, {"mapAction", "gameMenu", MAP_ACTION_GAME_MENU},
    {"mapAction", "squareInfo", MAP_ACTION_SQUARE_INFO}, {"mapEditor", "hideToolbox", MAP_EDITOR_HIDE_TOOLBOX},
    {"key", "none", KEY_NONE}, {"key", "backspace", KEY_BACKSPACE}, {"key", "tab", KEY_TAB},
    {"key", "enter", KEY_ENTER}, {"key", "esc", KEY_ESC}, {"key", "space", KEY_SPACE}, {"key", "delete", KEY_DELETE},
    {"key", "f1", KEY_F1}, {"key", "f2", KEY_F2}, {"key", "f3", KEY_F3}, {"key", "f4", KEY_F4},
    {"key", "f5", KEY_F5}, {"key", "f6", KEY_F6}, {"key", "f7", KEY_F7}, {"key", "f8", KEY_F8},
    {"key", "f9", KEY_F9}, {"key", "f10", KEY_F10}, {"key", "f11", KEY_F11}, {"key", "f12", KEY_F12},
    {"key", "pageDown", KEY_PAGE_DOWN}, {"key", "pageUp", KEY_PAGE_UP}, {"key", "endKey", KEY_END},
    {"key", "home", KEY_HOME}, {"key", "up", KEY_UP}, {"key", "down", KEY_DOWN}, {"key", "left", KEY_LEFT},
    {"key", "right", KEY_RIGHT}, {"key", "ctrl", KEY_CTRL}, {"key", "alt", KEY_ALT}, {"key", "ctrlShift", KEY_CTRL_SHIFT},
    {"message", "everyHuman", MESSAGE_EVERY_HUMAN}, {"message", "notAdded", MESSAGE_NOT_ADDED},
    {"controller", "removed", CONTROLLER_REMOVED}, {"controller", "human", CONTROLLER_HUMAN},
    {"controller", "ai", CONTROLLER_AI}, {"controller", "independent", CONTROLLER_INDEPENDENT},
    {"classAbility", "aiClass", CLASS_ABILITY_ID_AI_CLASS}, {"classAbility", "randomSkip", CLASS_ABILITY_ID_RANDOM_SKIP},
    {"classAbility", "aiLevelMin", CLASS_ABILITY_ID_AI_LEVEL_MIN}, {"classAbility", "aiLevelMax", CLASS_ABILITY_ID_AI_LEVEL_MAX},
    {"classAbility", "standardTroops", CLASS_ABILITY_ID_STDTROOPS},
    {"gameOverMode", "plain", GAME_OVER_MODE_PLAIN}, {"gameOverMode", "fog", GAME_OVER_MODE_FOG},
    {"newGame", "normal", NEW_WORLD_NORMAL}, {"newGame", "network", NEW_WORLD_NETWORK},
    {"newGame", "skipParticipants", NEW_WORLD_SKIP_PARTICIPANTS}, {"newGame", "mapScreenOk", MAP_SCREEN_OK},
    {"newGame", "mapSettingsOk", MAP_SETTINGS_OK},
    {"newGame", "mapSizeFirst", MAP_SETTINGS_SIZE_FIRST}, {"newGame", "participantsOk", PARTICIPANTS_OK},
    {"ritual", "reasonMirror", RITUAL_REASON_MIRROR}, {"ritual", "reasonMaxcast", RITUAL_REASON_MAXCAST},
    {"ritual", "reasonImmortal", RITUAL_REASON_IMMORTAL}, {"ritual", "reasonMastery", RITUAL_REASON_MASTERY},
    {"ritual", "reasonPlane", RITUAL_REASON_PLANE}, {"ritual", "rowCastable", RITUAL_ROW_CASTABLE},
    {"ritual", "rowGreyed", RITUAL_ROW_GREYED},
    {"screen", "fileBrowserRowFirst", FILE_BROWSER_ROW_FIRST}, {"screen", "fileBrowserDelete", FILE_BROWSER_DELETE},
    {"screen", "squareInfoOk", SQUARE_INFO_OK},
    {"aiMove", "none", AI_MOVE_NONE}, {"aiMove", "target", AI_MOVE_TARGET}, {"aiMove", "explore", AI_MOVE_EXPLORE},
    {"aiMove", "noSquare", AI_NO_SQUARE}, {"aiMove", "exploreBelow", AI_EXPLORE_BELOW},
    {"aiMove", "farExploreBelow", AI_FAR_EXPLORE_BELOW},
};

// The key actions as coe5.keyAction.<Lua name>.
#define LUA_KEY_ACTION(name, lua, screen, how, id, key, row, label) {#lua, KEY_ACTION_##name},
static const struct { const char *name; int value; } LUA_KEY_ACTIONS[] = {KEY_ACTION_LIST(LUA_KEY_ACTION)};
#undef LUA_KEY_ACTION

// Adds GAME_NUMBERS to the table on top of the stack, one subtable per group, and coe5.keyAction.
static void addGameNumbers(lua_State *L) {
    lua_newtable(L);
    for (size_t i = 0; i < sizeof LUA_KEY_ACTIONS / sizeof LUA_KEY_ACTIONS[0]; i++) {
        lua_pushinteger(L, LUA_KEY_ACTIONS[i].value);
        lua_setfield(L, -2, LUA_KEY_ACTIONS[i].name);
    }
    lua_setfield(L, -2, "keyAction");
    for (size_t i = 0; i < sizeof GAME_NUMBERS / sizeof GAME_NUMBERS[0]; i++) {
        if (lua_getfield(L, -1, GAME_NUMBERS[i].table) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, GAME_NUMBERS[i].table);
        }
        lua_pushinteger(L, GAME_NUMBERS[i].value);
        lua_setfield(L, -2, GAME_NUMBERS[i].name);
        lua_pop(L, 1);
    }
}

// print(...): written as a lua_print line in coe5mod.log.
static int lPrint(lua_State *L) {
    if (!luaScriptLogOn()) return 0;
    int n = lua_gettop(L);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (int i = 1; i <= n; i++) {
        if (i > 1) luaL_addchar(&b, '\t');
        luaL_tolstring(L, i, NULL);
        luaL_addvalue(&b);
    }
    luaL_pushresult(&b);
    LuaScript *s = luaScriptOf(L);
    writeLog("lua_print", "mod=%s text=%s", s ? s->mod : "?", lua_tostring(L, -1));
    return 0;
}

// xpcall(f, handler, ...): the base library's, except for CoE5SE's stop of the script.
static int luaXpcallHandler(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    if (s && s->stopping) return 1;
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    lua_call(L, lua_gettop(L) - 1, 1);
    return 1;
}
static int finishLuaXpcall(lua_State *L, int status, lua_KContext extra) {
    if (status != LUA_OK && status != LUA_YIELD) {
        lua_pushboolean(L, 0);
        lua_pushvalue(L, -2);
        return 2;
    }
    return lua_gettop(L) - (int)extra;
}
static int lXpcall(lua_State *L) {
    int n = lua_gettop(L);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    lua_pushcclosure(L, luaXpcallHandler, 1);
    lua_replace(L, 2);
    lua_pushboolean(L, 1);
    lua_pushvalue(L, 1);
    lua_rotate(L, 3, 2);
    int status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishLuaXpcall);
    return finishLuaXpcall(L, status, 2);
}

// Every coe5.* function is a closure of this gate.
static int luaGate(lua_State *L) {
    const luaL_Reg *fn = lua_touserdata(L, lua_upvalueindex(1));
    LuaScript *s = luaScriptOf(L);
    if (s && s == luaRunning && !s->stopping) return fn->func(L);
    refuseLuaCall(s ? s->mod : "?", lua_tostring(L, lua_upvalueindex(2)), s && s->stopping ? "script_stopped" : "not_running",
                  "running=%s", luaRunning ? luaRunning->mod : "none");
    return 0;
}
// Adds each function of `fns` to the table on top of the stack through luaGate.
static void addGatedFunctions(lua_State *L, const luaL_Reg *fns, const char *prefix) {
    for (; fns->name; fns++) {
        lua_pushlightuserdata(L, (void *)fns);
        lua_pushfstring(L, "%s%s", prefix, fns->name);
        lua_pushcclosure(L, luaGate, 2);
        lua_setfield(L, -2, fns->name);
    }
}

// The warning function of a script's state: each message is a lua_warning line in coe5mod.log.
static void luaWarn(void *ud, const char *message, int tocont) {
    LuaScript *s = luaScriptOf(ud);
    if (!s) return;
    if (!s->warnContinued && !tocont && message[0] == '@') {
        if (!strcmp(message, "@off")) s->warnOff = 1;
        else if (!strcmp(message, "@on")) s->warnOff = 0;
        return;
    }
    size_t length = strlen(message);
    if (!s->warnOff && length < INT_MAX / 2 - (size_t)s->warningLength &&
        growList((void **)&s->warning, &s->warningCapacity, s->warningLength + (int)length + 1, 1)) {
        memcpy(s->warning + s->warningLength, message, length + 1);
        s->warningLength += (int)length;
    }
    s->warnContinued = tocont;
    if (tocont) return;
    if (!s->warnOff && s->warning) writeLog("lua_warning", "mod=%s text=%s", s->mod, s->warning);
    s->warningLength = 0;
    if (s->warning) s->warning[0] = '\0';
}

// ---- Loading ----------------------------------------------------------------------------------
// Makes a Lua state with only the safe libraries and the coe5 table.
static int setUpLuaSandbox(lua_State *L);
static lua_State *newLuaSandbox(LuaScript *s) {
    lua_State *L = lua_newstate(luaAlloc, s, luaL_makeseed(NULL));
    if (!L) return NULL;
    lua_atpanic(L, luaPanic);
    return setUpLuaSandbox(L) ? L : NULL;
}
// Opens the libraries and the coe5 table in a new state L.
static int openLuaSandbox(lua_State *L);
static int lRequire(lua_State *L);
static char luaRequiredKey;
static int setUpLuaSandbox(lua_State *L) {
    *(LuaScript **)lua_getextraspace(L) = NULL;
    lua_sethook(L, luaStepHook, LUA_MASKCOUNT, LUA_HOOK_STEPS);
    lua_setwarnf(L, luaWarn, L);
    lua_pushcfunction(L, openLuaSandbox);
    if (lua_pcall(L, 0, 0, 0) == LUA_OK) return 1;
    lua_close(L);
    return 0;
}
static int openLuaSandbox(lua_State *L) {
    static const luaL_Reg LIBS[] = {
        {LUA_GNAME, luaopen_base}, {LUA_STRLIBNAME, luaopen_string}, {LUA_TABLIBNAME, luaopen_table},
        {LUA_MATHLIBNAME, luaopen_math}, {LUA_UTF8LIBNAME, luaopen_utf8}, {LUA_COLIBNAME, luaopen_coroutine},
    };
    for (size_t i = 0; i < sizeof LIBS / sizeof LIBS[0]; i++) {
        luaL_requiref(L, LIBS[i].name, LIBS[i].func, 1);
        lua_pop(L, 1);
    }
    static const char *const REMOVED[] = {"dofile", "loadfile", "load", "collectgarbage"};
    for (size_t i = 0; i < sizeof REMOVED / sizeof REMOVED[0]; i++) {
        lua_pushnil(L);
        lua_setglobal(L, REMOVED[i]);
    }
    static const luaL_Reg REQUIRE = {"require", lRequire};
    lua_pushlightuserdata(L, (void *)&REQUIRE);
    lua_pushliteral(L, "require");
    lua_pushcclosure(L, luaGate, 2);
    lua_setglobal(L, "require");
    lua_newtable(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &luaRequiredKey);
    lua_pushcfunction(L, lPrint);
    lua_setglobal(L, "print");
    lua_pushcfunction(L, lXpcall);
    lua_setglobal(L, "xpcall");
    for (int i = LUA_STOP_NONE + 1; i < LUA_STOP_KINDS; i++) {
        lua_pushstring(L, LUA_STOPS[i].error);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &luaStopKeys[i]);
    }
    lua_newtable(L);
    addGatedFunctions(L, LUA_COE5, "coe5.");
    lua_newtable(L);
    addGatedFunctions(L, LUA_UI, "coe5.ui.");
    lua_setfield(L, -2, "ui");
    addGameNumbers(L);
    lua_setglobal(L, "coe5");
    return 0;
}

// lua_load's reader for a script's file.
typedef struct { FILE *f; size_t pending; char buffer[BUFSIZ]; } LuaFileReader;
static const char *readLuaFile(lua_State *L, void *ud, size_t *size) {
    (void)L;
    LuaFileReader *r = ud;
    if (r->pending) {
        *size = r->pending;
        r->pending = 0;
        return r->buffer;
    }
    if (feof(r->f)) return NULL;
    *size = fread(r->buffer, 1, sizeof r->buffer, r->f);
    return r->buffer;
}
// Skips a UTF-8 BOM and a first line starting with #.
static void readLuaFileStart(LuaFileReader *r) {
    int c = getc(r->f);
    if (c == 0xEF && getc(r->f) == 0xBB && getc(r->f) == 0xBF) c = getc(r->f);
    if (c == '#') {
        do c = getc(r->f); while (c != EOF && c != '\n');
        r->buffer[r->pending++] = '\n';
        c = getc(r->f);
    }
    if (c == LUA_SIGNATURE[0]) r->pending = 0;
    if (c != EOF) r->buffer[r->pending++] = (char)c;
}

// Pushes the chunk of the script file at `path`, named `chunk`.
#define LUA_CHUNK_NAME_MAX (MOD_NAME_MAX + 8)
static void pushLuaTextFile(lua_State *L, const char *path, const char *chunk) {
    LuaFileReader *r = lua_newuserdatauv(L, sizeof *r, 0);
    r->pending = 0;
    if (!(r->f = fopen(path, "r"))) luaL_error(L, "can't open %s", chunk);
    readLuaFileStart(r);
    char name[LUA_CHUNK_NAME_MAX + 1];
    snprintf(name, sizeof name, "@%s", chunk);
    int rc = lua_load(L, readLuaFile, r, name, "t");
    int failed = ferror(r->f);
    fclose(r->f);
    if (rc != LUA_OK) lua_error(L);
    if (failed) luaL_error(L, "can't read %s", chunk);
    lua_remove(L, -2);
}

// callLuaProtected's push for a script's file: its chunk, with no arguments.
static int pushLuaScriptFile(lua_State *L, void *script) {
    const LuaScript *s = script;
    char chunk[LUA_CHUNK_NAME_MAX];
    snprintf(chunk, sizeof chunk, "%s.lua", s->mod);
    pushLuaTextFile(L, s->path, chunk);
    return 0;
}

// ---- require: a script's other files ------------------------------------------------------------
// require(name): runs <name>.lua from the script's own mod folder, once.
#define LUA_REQUIRE_NAME_MAX 200
static char luaRequireLoading;

// Returns NULL when `name` can name a required file, or the reason word it can't.
static const char *luaRequireNameProblem(const char *name, size_t length) {
    if (!length) return "empty_name";
    if (length > LUA_REQUIRE_NAME_MAX) return "name_too_long";
    for (size_t i = 0; i < length; i++) {
        char c = name[i];
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_' && c != '.') return "bad_character";
    }
    if (name[0] == '.' || name[length - 1] == '.' || strstr(name, "..")) return "bad_name";
    for (const char *part = name; *part;) {
        char text[LUA_REQUIRE_NAME_MAX + 1];
        size_t n = strcspn(part, ".");
        snprintf(text, sizeof text, "%.*s", (int)n, part);
        const char *problem = storageNameProblemUpTo(text, LUA_REQUIRE_NAME_MAX);
        if (problem) return problem;
        part += n + (part[n] == '.');
    }
    return NULL;
}

// @platform-split lua_require_link: Windows reads the reparse point attribute, a Mac lstat
// Whether the file or folder at `path` is a link.
static int luaPathIsLink(const char *path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat st;
    return !lstat(path, &st) && S_ISLNK(st.st_mode);
#endif
}

// Finds required file `name` in script s's folder.
static const char *findRequiredFile(const LuaScript *s, const char *name, char *path, size_t size, char *chunk,
                                    char *part, size_t partSize) {
    snprintf(chunk, LUA_CHUNK_NAME_MAX, "%s.lua", name);
    for (char *c = chunk; *c; c++)
        if (*c == '.' && strcmp(c, ".lua")) *c = '/';
    if (!buildPath(path, size, __func__, "%s", s->folder)) return "path_too_long";
    for (const char *at = name; *at;) {
        size_t n = strcspn(at, ".");
        int last = !at[n];
        snprintf(part, partSize, "%.*s%s", (int)n, at, last ? ".lua" : "");
        if (!buildPath(path + strlen(path), size - strlen(path), __func__, PATH_SEP "%s", part)) return "path_too_long";
        char stored[MAX_PATH];
        if (!storageStoredName(path, stored, sizeof stored)) return "not_found";
        if (strcmp(stored, part)) {
            snprintf(part, partSize, "%s", stored);
            return "wrong_case";
        }
        if (luaPathIsLink(path)) return "link";
        at += n + !last;
    }
    return NULL;
}

// require(name): see this section's top.
static int lRequire(lua_State *L) {
    size_t length;
    const char *name = luaL_checklstring(L, 1, &length);
    LuaScript *s = luaScriptOf(L);
    const char *problem = luaRequireNameProblem(name, length);
    if (problem) {
        refuseLuaCall(s->mod, "require", problem, "name=%.64s", name);
        return luaL_error(L, "require: '%s' can't name a file in the mod's folder (%s: use letters, digits, and _, with "
                             "a dot between folders)", length > LUA_REQUIRE_NAME_MAX ? "(too long)" : name, problem);
    }
    lua_settop(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &luaRequiredKey);
    lua_pushvalue(L, 1);
    if (lua_rawget(L, 2) != LUA_TNIL) {
        if (lua_touserdata(L, 3) == &luaRequireLoading)
            return luaL_error(L, "require: loop: '%s' is still loading (it requires itself, through the files it requires)", name);
        return 1;
    }
    lua_pop(L, 1);
    char path[MAX_PATH], chunk[LUA_CHUNK_NAME_MAX], part[MAX_PATH];
    if (!strcmp(name, s->mod)) {
        refuseLuaCall(s->mod, "require", "main_script", "name=%.64s", name);
        return luaL_error(L, "require: '%s' is the mod's own script, which CoE5SE runs itself", name);
    }
    problem = findRequiredFile(s, name, path, sizeof path, chunk, part, sizeof part);
    if (problem && !strcmp(problem, "not_found"))
        return luaL_error(L, "require: no file %s in %s's folder", chunk, s->mod);
    if (problem) {
        refuseLuaCall(s->mod, "require", problem, "name=%.64s part=%.64s", name, part);
        if (!strcmp(problem, "wrong_case"))
            return luaL_error(L, "require: no file %s in %s's folder (%s differs in case)", chunk, s->mod, part);
        return luaL_error(L, "require: %s refused (%s)", chunk, !strcmp(problem, "link") ? "a link" : "its path is too long");
    }
    pushLuaTextFile(L, path, chunk);
    lua_pushvalue(L, 1);
    lua_pushlightuserdata(L, &luaRequireLoading);
    lua_rawset(L, 2);
    lua_pushvalue(L, 1);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        lua_pushvalue(L, 1);
        lua_pushnil(L);
        lua_rawset(L, 2);
        return lua_error(L);
    }
    if (lua_isnil(L, 3)) {
        lua_pushboolean(L, 1);
        lua_replace(L, 3);
    }
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_rawset(L, 2);
    return 1;
}

// Loads and runs <folder>/<mod>.lua, if the mod has one.
static void loadLuaScript(const char *mod, const char *folder) {
    char path[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s.lua", folder, mod)) return;
    FILE *f = fopen(path, "rb");
    if (!f) return;
    fclose(f);
    LuaScript *s = calloc(1, sizeof *s);
    if (s) snprintf(s->mod, sizeof s->mod, "%s", mod);
    if (!s || !growList((void **)&luaScripts, &luaScriptCapacity, luaScriptCount + 1, sizeof s) || !(s->L = newLuaSandbox(s))) {
        free(s);
        refuseLuaCall(mod, "script", "out_of_memory", "path=%s", path);
        return;
    }
    snprintf(s->path, sizeof s->path, "%s", path);
    snprintf(s->folder, sizeof s->folder, "%s", folder);
    *(LuaScript **)lua_getextraspace(s->L) = s;
    luaScripts[luaScriptCount++] = s;
    if (callLuaProtected(s, pushLuaScriptFile, s, 0)) writeLog("lua_script_loaded", "mod=%s version=%s path=%s", mod, modVersionText(mod), path);
}

// Loads every enabled mod's script once, at the end of startup (finishStartup), in load order.
static void loadLuaScripts(void) {
    for (int k = 0; k < orderedModCount; k++) {
        const ModInfo *m = &orderedMods[loadOrder[k]];
        loadLuaScript(m->name, modFolder(m->tag));
    }
    moveOldDataFolders();
    writeLog("lua_scripts_done", "count=%d lua=%s", luaScriptCount, LUA_RELEASE);
}

// ---- Boxes for the player ---------------------------------------------------------------------
// The boxes about a mod's stopped script or full storage.
#define LUA_NOTICE_NAME_MAX 40
#define LUA_NOTICE_PATH_MAX 80

// Writes `in` as the boxes show a name.
static void luaNoticeName(const char *in, char *out, size_t size) {
    char clean[MOD_NAME_MAX];
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)in; *p && n < sizeof clean - 1; p++)
        if (*p >= 0x20 && *p != 0x7f) clean[n++] = (char)*p;
    clean[n] = '\0';
    if (n > LUA_NOTICE_NAME_MAX) {
        size_t cut = LUA_NOTICE_NAME_MAX - 1;
        while (cut && ((unsigned char)clean[cut] & 0xC0) == 0x80) cut--;
        snprintf(out, size, "%.*s...", (int)cut, clean);
        return;
    }
    snprintf(out, size, "%s", clean);
}

// Writes box `kind`'s title and text for mod `mod`.
static void luaNoticeText(int kind, const char *mod, int stop, const char *area, char *title, size_t titleSize, char *text,
                          size_t textSize) {
    char name[LUA_NOTICE_NAME_MAX + 8];
    luaNoticeName(mod, name, sizeof name);
    if (kind == LUA_NOTICE_STORAGE) {
        char shown[LUA_NOTICE_PATH_MAX + 4];
        size_t length = area ? strlen(area) : 0;
        if (length > LUA_NOTICE_PATH_MAX) snprintf(shown, sizeof shown, "%.30s...%s", area, area + length - (LUA_NOTICE_PATH_MAX - 33));
        else snprintf(shown, sizeof shown, "%s", area ? area : "");
        snprintf(title, titleSize, "CoE5SE: Mod storage full");
        snprintf(text, textSize,
                 "Storage full: %s, %d MB\n"
                 "%s\n"
                 "No more saves until old files are deleted.\n",
                 name, storageAreaMegabytes(mod), shown);
        return;
    }
    const char *why = stop == LUA_STOP_MEMORY ? "Too much memory.\n"
                    : stop == LUA_STOP_MEMORY_TOTAL ? "Most memory of all scripts.\n"
                    : "Ran too long.\n";
    snprintf(title, titleSize, "CoE5SE: %s's script was stopped", name);
    snprintf(text, textSize, "%sOff until restart.\n", why);
}

// Shows the first box due, if any.
static void showLuaNotices(void) {
    if (luaCodeRunning || isNetworkGame()) return;
    for (int i = 0; i < luaScriptCount; i++) {
        LuaScript *s = luaScripts[i];
        int kind = s->noticesDue & ~s->noticesShown & LUA_NOTICE_STOPPED ? LUA_NOTICE_STOPPED
                 : s->noticesDue & ~s->noticesShown & LUA_NOTICE_STORAGE ? LUA_NOTICE_STORAGE : 0;
        if (!kind) continue;
        s->noticesShown |= kind;
        char area[MAX_PATH] = "", title[128], text[POPUP_TEXT_SIZE];
        if (kind == LUA_NOTICE_STORAGE)
            buildPath(area, sizeof area, __func__, "%s" USER_DIR_SEP STORAGE_DATA_DIR USER_DIR_SEP "%s", userDir(), s->mod);
        luaNoticeText(kind, s->mod, s->stopping, area, title, sizeof title, text, sizeof text);
        writeLog("lua_notice_shown", "mod=%s kind=%s%s%s", s->mod, kind == LUA_NOTICE_STOPPED ? "stopped" : "storage_full",
                 kind == LUA_NOTICE_STOPPED ? " reason=" : "", kind == LUA_NOTICE_STOPPED ? LUA_STOPS[s->stopping].reason : "");
        okBox(title, text);
        return;
    }
}
