// The coe5.ui table: the game's screen widgets for scripts; and coe5.showMessageBox, the game's message box.

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

// A draw call's arguments as floats; 0 when one isn't a finite float.
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

// Whether a draw call may draw now: only inside MAP_EDITOR_DRAW's chain.
static int luaMayDraw(lua_State *L, const char *call) {
    if (mapEditorDrawing > 0) return 1;
    LuaScript *s = luaScriptOf(L);
    refuseLuaCall(s ? s->mod : "?", call, "not_drawing", "site=MAP_EDITOR_DRAW");
    return 0;
}

// coe5.ui.drawLine(x1, y1, x2, y2, r, g, b, alpha[, width]): one line at UI coordinates.
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

// coe5.ui.drawText(x, y, size, text, r, g, b, alpha): one line of text at UI coordinates.
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
    for (int m = 0; m <= last && monsterRecord(m) && !held; m++)
        held = *(const int *)(monsterRecord(m) + (RVA_MONSTER_SPRITES - RVA_MONSTER_TABLE)) == wanted;
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
    int sprite = *(const int *)(monsterRecord((int)monster) + (RVA_MONSTER_SPRITES - RVA_MONSTER_TABLE));
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

// ---- coe5.showMessageBox -----------------------------------------------------------------------
// Why coe5.showMessageBox refuses its call, or NULL when a human is at the screen to close the box.
static const char *messageBoxRefusal(int network, int planning, int inAiSite, int activePlayer, int activeController) {
    if (network) return "network_game";
    if (planning) return "planning_route";
    if (inAiSite || (activePlayer >= 0 && activeController != CONTROLLER_HUMAN)) return "ai_turn";
    return NULL;
}

// coe5.showMessageBox(title, text): the game's message box with one Ok button, shown at once.
static int lShowMessageBox(lua_State *L) {
    const char *title = luaL_checkstring(L, 1), *text = luaL_checkstring(L, 2);
    int active = G32(RVA_ACTIVE_PLAYER);
    int controller = active >= 0 && active < PLAYER_SLOTS ? G16(RVA_SLOT_CONTROLLER + (size_t)active * 2) : CONTROLLER_HUMAN;
    const char *reason =
        messageBoxRefusal(isNetworkGame(), portalHandlerRunning && portalHandlerPlanning, aiSiteRunning(), active, controller);
    if (reason) {
        refuseLuaCall(runningLuaMod(), "coe5.showMessageBox", reason, "title=%.80s active=%d controller=%d", title, active, controller);
        lua_pushboolean(L, 0);
        return 1;
    }
    char titleCut[SCREEN_TITLE_TEXT_SIZE], textCut[TEXT_BUTTON_TEXT_SIZE];
    title = fitGameText((void *)lShowMessageBox, "coe5.showMessageBox", title, titleCut, sizeof titleCut);
    text = fitGameText((void *)lShowMessageBox, "coe5.showMessageBox", text, textCut, sizeof textCut);
    writeLog("lua_message_box_shown", "mod=%s title=%.80s", runningLuaMod(), title);
    messageBox(title, text, "Ok", "", 0, UI_KEY_ENTER, 0);
    lua_pushboolean(L, 1);
    return 1;
}
