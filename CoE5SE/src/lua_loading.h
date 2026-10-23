// A script's Lua state: the coe5 table's lists, print, xpcall and warn, loading each script, require, and the player's boxes.

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
    {"keepTraceLines", lKeepTraceLines}, {"addPlane", lAddPlane}, {"setPlaneShape", lSetPlaneShape}, {"planeIndex", lPlaneIndex},
    {"setPlaneTint", lSetPlaneTint}, {"clearPlaneTint", lClearPlaneTint}, {"setTerrainLooks", lSetTerrainLooks}, COE5SE_EXTRA_LUA_FUNCTIONS {NULL, NULL},
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
    lua_newtable(L);
    addGatedFunctions(L, LUA_GEN, "coe5.gen.");
    lua_setfield(L, -2, "gen");
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
    luaScriptsLoading = 1;
    for (int k = 0; k < orderedModCount; k++) {
        const ModInfo *m = &orderedMods[loadOrder[k]];
        loadLuaScript(m->name, modFolder(m->tag));
    }
    luaScriptsLoading = 0;
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
