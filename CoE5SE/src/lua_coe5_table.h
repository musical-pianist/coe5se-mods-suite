// The coe5 table's functions: logging, menu lines, screens, planes, restarts, settings, keys, monsters, armies.

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

// coe5.addMapEditorLine{label=, tooltip=, visible=, onClick=}: a map editor toolbox line; its id or false.
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

static int luaScriptsLoading;

static int luaPathIsLink(const char *path);

// Finds file `spec` in mod folder `folder` and writes its path; NULL, or the reason it can't.
#define PLANE_ICON_SPEC_MAX 200
static int endsWith(const char *text, const char *ending) {
    size_t n = strlen(text), e = strlen(ending);
    return n > e && !strcmp(text + n - e, ending);
}
static const char *findModFile(const char *folder, const char *spec, const char *const endings[2], const char *const reasons[4], char *path,
                               size_t size) {
    size_t length = strlen(spec);
    if (length < 4 || length > PLANE_ICON_SPEC_MAX || strncmp(spec, "./", 2) || (!endsWith(spec + 2, endings[0]) && !endsWith(spec + 2, endings[1])))
        return reasons[0];
    for (const char *part = spec + 2; *part;) {
        size_t n = strcspn(part, "/");
        if (!n || part[0] == '.') return reasons[0];
        for (size_t i = 0; i < n; i++) {
            char c = part[i];
            if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_' && c != '-' && c != '.') return reasons[0];
            if (c == '.' && i + 1 < n && part[i + 1] == '.') return reasons[0];
        }
        part += n + (part[n] == '/');
        if (part[-1] == '/' && !*part) return reasons[0];
    }
    if (!buildPath(path, size, __func__, "%s", folder)) return "path_too_long";
    for (const char *part = spec + 2; *part;) {
        size_t n = strcspn(part, "/");
        char name[PLANE_ICON_SPEC_MAX + 1], stored[MAX_PATH];
        snprintf(name, sizeof name, "%.*s", (int)n, part);
        if (!buildPath(path + strlen(path), size - strlen(path), __func__, PATH_SEP "%s", name)) return "path_too_long";
        if (!storageStoredName(path, stored, sizeof stored)) return reasons[1];
        if (strcmp(stored, name)) return reasons[2];
        if (luaPathIsLink(path)) return reasons[3];
        part += n + (part[n] == '/');
    }
    return NULL;
}

// Finds image `spec` (.tga or .png) in mod folder `folder`.
static const char *findModImage(const char *folder, const char *spec, char *path, size_t size) {
    static const char *const ENDINGS[2] = {".tga", ".png"};
    static const char *const REASONS[4] = {"bad_icon", "icon_not_found", "icon_wrong_case", "icon_link"};
    return findModFile(folder, spec, ENDINGS, REASONS, path, size);
}

#include "plane_build.h"

// coe5.addPlane{...}: declares a plane of the mod's, at script load; true, or false when refused.
static void readLuaPlaneLayout(lua_State *L, int piecesAt, int zonesAt, int unitsAt, PlaneLayout *layout, const char *mod, const char *folder,
                               const char *script, int line);
static void finishPlaneLayout(PlaneLayout *layout, int w, int h, const char *mod);
static void notePlaneLayoutProblems(const PlaneLayout *layout, const char *mod);
static int lAddPlane(lua_State *L) {
    static PlaneLayout layout;
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_getfield(L, 1, "name");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    lua_getfield(L, 1, "fill");
    lua_getfield(L, 1, "like");
    lua_getfield(L, 1, "populate");
    lua_getfield(L, 1, "swap");
    lua_getfield(L, 1, "generate");
    lua_getfield(L, 1, "icon");
    lua_getfield(L, 1, "pieces");
    lua_getfield(L, 1, "zones");
    lua_getfield(L, 1, "units");
    const char *name = luaL_checkstring(L, 2);
    PlaneBuildFields f;
    const char *reason = readPlaneBuildFields(L, 6, 7, 8, &f);
    int sizeFromLike = !reason && lua_isnil(L, 3) && lua_isnil(L, 4) && likePlaneSizeExtra(f.recipe) >= 0;
    lua_Integer w = sizeFromLike ? PLANE_LIKE_SIZE_MIN : luaL_checkinteger(L, 3);
    lua_Integer h = sizeFromLike ? PLANE_LIKE_SIZE_MIN : luaL_checkinteger(L, 4);
    if (!lua_isnil(L, 9)) luaL_checktype(L, 9, LUA_TFUNCTION);
    int built = !lua_isnil(L, 6) || !lua_isnil(L, 9);
    lua_Integer fill = built ? luaL_optinteger(L, 5, 0) : luaL_checkinteger(L, 5);
    const char *icon = luaL_optstring(L, 10, NULL);
    char iconPath[MAX_PATH] = "";
    const char *mod = runningLuaMod();
    if (!reason) {
        reason = !luaScriptsLoading ? "not_at_load"
               : w < 1 || w >= MAP_COLUMNS_MAX || h < 1 || h >= MAP_COLUMN_SQUARES ? "bad_size"
               : fill < 0 || fill > SHRT_MAX ? "bad_terrain"
               : icon && luaRunning ? findModImage(luaRunning->folder, icon, iconPath, sizeof iconPath)
               : NULL;
    }
    if (!reason && f.recipe >= 0 && !planeRecipesLoaded(L, mod ? mod : "?", "coe5.addPlane")) reason = "recipes_not_loaded";
    if (!reason) {
        lua_Debug caller;
        int line = lua_getstack(L, 1, &caller) && lua_getinfo(L, "l", &caller) ? caller.currentline : 0;
        readLuaPlaneLayout(L, 11, 12, 13, &layout, mod ? mod : "?", luaRunning ? luaRunning->folder : "", luaRunning ? luaRunning->path : "?", line);
        finishPlaneLayout(&layout, (int)w, (int)h, mod ? mod : "?");
    }
    if (!reason) reason = declareCustomPlane(mod ? mod : "?", name, (int)w, (int)h, (int)fill, iconPath);
    if (reason) refuseLuaCall(mod, "coe5.addPlane", reason, "name=%.40s w=%lld h=%lld fill=%lld icon=%.80s", name, (long long)w, (long long)h,
                              (long long)fill, icon ? icon : "-");
    else {
        CustomPlane *p = findCustomPlane(name);
        p->recipe = f.recipe, p->populate = f.populate, p->swapCount = f.swapCount, p->sizeFromLike = sizeFromLike;
        memcpy(p->swapFrom, f.swapFrom, sizeof f.swapFrom);
        memcpy(p->swapTo, f.swapTo, sizeof f.swapTo);
        p->script = luaScriptOf(L);
        if (!lua_isnil(L, 9)) {
            lua_pushvalue(L, 9);
            p->generateRef = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        p->layout = layout;
        notePlaneLayoutProblems(&p->layout, p->mod);
        writeLog("plane_recipe", "mod=%s name=%s like=%s populate=%s swaps=%d generate=%d pieces=%d zones=%d units=%d", mod ? mod : "?", name,
                 f.recipe >= 0 ? VANILLA_PLANE_SHORT_NAMES[f.recipe] : "-", f.populate >= 0 ? VANILLA_PLANE_SHORT_NAMES[f.populate] : "-",
                 f.swapCount, p->generateRef != 0, p->layout.pieceCount, p->layout.zoneCount, p->layout.units.count);
    }
    lua_pushboolean(L, !reason);
    return 1;
}

// coe5.setPlaneShape(monster, planeName, shapeMonster): a monster's shape on a mod's plane.
static int lSetPlaneShape(lua_State *L) {
    const char *monster = luaL_checkstring(L, 1), *plane = luaL_checkstring(L, 2), *shape = luaL_optstring(L, 3, NULL);
    const char *mod = runningLuaMod();
    lua_Debug caller;
    int line = lua_getstack(L, 1, &caller) && lua_getinfo(L, "l", &caller) ? caller.currentline : 0;
    const char *reason = !luaScriptsLoading ? "not_at_load" : declarePlaneShape(mod ? mod : "?", monster, plane, shape, line);
    if (reason) refuseLuaCall(mod, "coe5.setPlaneShape", reason, "monster=%.60s plane=%.40s shape=%.60s", monster, plane, shape ? shape : "-");
    lua_pushboolean(L, !reason);
    return 1;
}

static int lSetTerrainLooks(lua_State *L);

// coe5.planeIndex(name): the index of the named plane in this world, or nil.
static int lPlaneIndex(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    int index = customPlaneIndex(name);
    for (int k = 0; index < 0 && k <= G32(RVA_LAST_PLANE) && k < planeTableRecords(); k++)
        if (!strncmp((const char *)planeRecord(k), name, PLANE_NAME_SIZE)) index = k;
    if (index < 0) lua_pushnil(L);
    else lua_pushinteger(L, index);
    return 1;
}

// The plane argument `arg` of the tint calls, by index or name (plane_tint.h, planeTintNamed); -1 for none.
static int luaTintPlane(lua_State *L, int arg) {
    if (lua_type(L, arg) == LUA_TSTRING) return planeTintNamed(lua_tostring(L, arg));
    lua_Integer plane = luaL_checkinteger(L, arg);
    return plane < 0 || plane > INT_MAX ? -1 : (int)plane;
}

// coe5.setPlaneTint(plane, r, g, b, strength): lays a colour over the map while the plane is in view.
static int lSetPlaneTint(lua_State *L) {
    int plane = luaTintPlane(L, 1);
    double rgba[PLANE_TINT_CHANNELS];
    for (int c = 0; c < PLANE_TINT_CHANNELS; c++) rgba[c] = luaL_checknumber(L, 2 + c);
    const char *reason = setLuaPlaneTint(plane, rgba);
    if (reason)
        refuseLuaCall(runningLuaMod(), "coe5.setPlaneTint", reason, "plane=%.40s r=%g g=%g b=%g strength=%g min=0 max=1", luaL_tolstring(L, 1, NULL),
                      rgba[0], rgba[1], rgba[2], rgba[3]);
    lua_pushboolean(L, !reason);
    return 1;
}

// coe5.clearPlaneTint(plane): drops the script's tint on the plane.
static int lClearPlaneTint(lua_State *L) {
    const char *reason = clearLuaPlaneTint(luaTintPlane(L, 1));
    if (reason) refuseLuaCall(runningLuaMod(), "coe5.clearPlaneTint", reason, "plane=%.40s", luaL_tolstring(L, 1, NULL));
    lua_pushboolean(L, !reason);
    return 1;
}

// coe5.mapView(): the map view now as a table, or nil.
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
        for (int m = 0; m <= last && monsterRecord(m); m++)
            if (!_strnicmp((const char *)monsterRecord(m), name, NAME_SIZE)) {
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

// coe5.revealMap(player[, plane]): every plane's squares, the mods' included, or one plane's, in view for the player.
#define SIGHT_REDRAW_CONTROLLERS 0x109
static int lRevealMap(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1), plane = luaL_optinteger(L, 2, -1);
    const char *problem = NULL;
    if (G32(RVA_ACTIVE_PLAYER) < 0) problem = "no_game";
    else if (player < 0 || player >= PLAYER_SLOTS) problem = "bad_player";
    else if (plane < -1 || plane > G32(RVA_LAST_PLANE) || plane >= planeTableRecords()) problem = "bad_plane";
    if (problem) {
        refuseLuaCall(runningLuaMod(), "coe5.revealMap", problem, "player=%lld plane=%lld", (long long)player, (long long)plane);
        lua_pushboolean(L, 0);
        return 1;
    }
    GAME_FN(void (*)(int, int), RVA_REVEAL_MAP_FOR)((int)player, (int)plane);
    int controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
    if (controller >= -3 && controller < 29 && (SIGHT_REDRAW_CONTROLLERS >> (controller + 3) & 1))
        for (int index = PLANE_ELYSIUM2 + 1; index <= lastCustomPlaneIndex(); index++)
            if (plane < 0 || plane == index) GAME_FN(int (*)(int, int), RVA_MAP_BG_MARK)(index, (int)player);
    writeLog("lua_map_revealed", "mod=%s player=%lld plane=%lld", runningLuaMod(), (long long)player, (long long)plane);
    lua_pushboolean(L, 1);
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
