// The Game Menu: mods' lines, restartGame and loadGame, play_game's wrappers, the inlined button rows, and the bounds on screens' waits.

// ---- Game Menu lines -------------------------------------------------------------------------
// A Game Menu line: a plugin's C callbacks, or a script's Lua callback.
#define MENU_LINE_FIRST_ID  9000
#define MENU_SECTION_NAMED  2
typedef struct {
    char label[64], tip[256], sectionName[64];
    int (*visible)(void);
    int (*onClick)(void);
    int lua, section;
} MenuLine;
static MenuLine *menuLines;
static int menuLineCount, menuLineCapacity;
static int (*nextMenuSectionEnd)(void);
static int (*nextSettingsSectionEnd)(void);
static int (*nextGameMenuWait)(int, int);
static int menuLineClicking;

// ---- Restarting a game (restartGame) ---------------------------------------------------------
// A mod asks from its Game Menu line's onClick.
static int restartPhase;
static int restartAsked;
static char restartAskedBy[64];
static int playGameEntries;
static u64 (*nextPlayGame)(int);

// Names the plugin DLL or script that made an API call.
static void nameApiCaller(void *caller, char *out, size_t size) {
    HMODULE m = callingPlugin(caller);
    const char *luaMod = m && isCoe5seCode((void *)m) ? runningLuaMod() : NULL;
    if (luaMod) snprintf(out, size, "%.59s.lua", luaMod);
    else moduleFileName(m ? (const void *)m : caller, out, size);
}

// The human players in the game: slots whose controller is CONTROLLER_HUMAN.
static int humanPlayerCount(void) {
    int humans = 0;
    for (int i = 0; i < PLAYER_SLOTS; i++) humans += PG16(RVA_SLOT_CONTROLLER + 2 * i) == CONTROLLER_HUMAN;
    return humans;
}

static int apiRestartGame(void) {
    if (!menuLineClicking) return REFUSE_API(0, "restartGame", "not_in_game_menu", "phase=%d", restartPhase);
    if (isNetworkGame()) return REFUSE_API(0, "restartGame", "network_game", "phase=%d", restartPhase);
    int humans = humanPlayerCount();
    if (humans != 1) return REFUSE_API(0, "restartGame", "not_single_player", "humans=%d", humans);
    if (!nextPlayGame) return REFUSE_API(0, "restartGame", "site_unavailable", "site=%s", SITES[SITE_MAIN_MENU_PLAY_GAME].name);
    restartAsked = 1;
    nameApiCaller(__builtin_return_address(0), restartAskedBy, sizeof restartAskedBy);
    return 1;
}

static int apiRestartPhase(void) { return restartPhase; }

// ---- Loading a save from the Game Menu (coe5.loadGame) ---------------------------------------
// A script's Game Menu line shows the game's Load Game list in the middle of a game.
#define LOAD_PATH_SIZE 2500
static char loadPath[LOAD_PATH_SIZE];
static int loadAsked;
static char loadAskedBy[64];
static int pickingSave;

typedef int (*FileBrowserFn)(const char *title, int a, int mode, int b, int c, const char *folder, const char *const *skips,
                             const char *name, char *path, int pathSize);
typedef u64 (*LoadGameFn)(const char *path, int arg);

// Load Game's list, as main_menu shows it.
static int pickSaveToLoad(char *path) {
    static const char *const skips[] = {".", "END"};
    FileBrowserFn browse = (FileBrowserFn)siteEntry(SITE_LOAD_GAME_BROWSER, GAME_PTR(RVA_FILE_BROWSER));
    for (;;) {
        int result = browse((const char *)GAME_PTR(RVA_STR_LOAD_GAME_TITLE), -1, 2, 0, 2, (const char *)GAME_PTR(RVA_SAVES_DIR),
                            skips, (const char *)GAME_PTR(RVA_LOAD_GAME_BROWSER_NAME), path, LOAD_PATH_SIZE);
        if (result == 0) return 0;
        if (result > 0) return 1;
        ((void (*)(int))GAME_PTR(RVA_KEYBOARD_SHORTCUTS))(10);
    }
}

static int apiLoadGame(void) {
    if (!menuLineClicking) return REFUSE_API(0, "loadGame", "not_in_game_menu", "phase=%d", restartPhase);
    if (isNetworkGame()) return REFUSE_API(0, "loadGame", "network_game", "phase=%d", restartPhase);
    int humans = humanPlayerCount();
    if (humans != 1) return REFUSE_API(0, "loadGame", "not_single_player", "humans=%d", humans);
    if (!nextPlayGame) return REFUSE_API(0, "loadGame", "site_unavailable", "site=%s", SITES[SITE_MAIN_MENU_PLAY_GAME].name);
    if (pickingSave) return REFUSE_API(0, "loadGame", "already_picking", "phase=%d", restartPhase);
    char path[LOAD_PATH_SIZE] = "";
    pickingSave = 1;
    int picked = pickSaveToLoad(path);
    pickingSave = 0;
    if (!picked) {
        writeLog("load_game", "phase=cancelled reason=no_save_picked");
        return 0;
    }
    snprintf(loadPath, sizeof loadPath, "%s", path);
    loadAsked = 1;
    nameApiCaller(__builtin_return_address(0), loadAskedBy, sizeof loadAskedBy);
    return 1;
}

// Ends a game for loadGame, answering each screen the resign waits on.
#define WARN_OK        1
#define GAME_OVER_EXIT 1
#define MESSAGES_OK    1
static u64 (*nextEliminatedWarn)(const char *fmt, ...);
static int (*nextWarnWait)(int, int);
static int (*nextGameOverWait)(int, int);
static u64 (*nextTurnMessages)(int);
static int (*nextMessagesWait)(int, int);
static int inEliminatedBox;
static int inTurnMessages;
static int loadedTurn = -1;

static int isEndingForLoad(void) { return restartPhase == RESTART_ENDING && loadPath[0]; }

static u64 watchEliminatedWarn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *text = va_arg(args, const char *);
    va_end(args);
    inEliminatedBox = 1;
    u64 result = nextEliminatedWarn(fmt, text);
    inEliminatedBox = 0;
    return result;
}

static int answerWarnWait(int block, int mode) {
    if (!inEliminatedBox || !isEndingForLoad()) return nextWarnWait(block, mode);
    writeLog("load_game", "phase=ending answered=eliminated_box");
    return WARN_OK;
}

static int answerGameOverWait(int block, int mode) {
    if (!isEndingForLoad()) return nextGameOverWait(block, mode);
    writeLog("load_game", "phase=ending answered=game_over");
    return GAME_OVER_EXIT;
}

static u64 watchTurnMessages(int player) {
    inTurnMessages = 1;
    u64 result = nextTurnMessages(player);
    inTurnMessages = 0;
    return result;
}

// The loaded turn's number, so a later turn's Messages aren't answered.
static int answerMessagesWait(int block, int mode) {
    if (!inTurnMessages) return nextMessagesWait(block, mode);
    if (isEndingForLoad()) {
        writeLog("load_game", "phase=ending answered=messages");
        return MESSAGES_OK;
    }
    int loaded = loadedTurn >= 0 && loadedTurn == G32(RVA_TURN);
    loadedTurn = -1;
    if (!loaded) return nextMessagesWait(block, mode);
    writeLog("load_game", "phase=loaded answered=messages");
    return MESSAGES_OK;
}

// main_menu's Load Game, with the save picked during the game that just ended.
static int loadPickedSave(void) {
    char path[LOAD_PATH_SIZE];
    snprintf(path, sizeof path, "%s", loadPath);
    loadPath[0] = 0;
    ((void (*)(void))GAME_PTR(RVA_RESET_GAME_STATE))();
    LoadGameFn load = (LoadGameFn)siteEntry(SITE_GAME_LOAD, GAME_PTR(RVA_LOAD_GAME));
    int loaded = (int)load(path, 0);
    writeLog("load_game", "phase=loaded result=%d", loaded);
    if (loaded < 1) return 0;
    loadedTurn = G32(RVA_TURN);
#ifdef __APPLE__
    G8(RVA_LOADED_GAME) = 1;
#else
    G32(RVA_LOADED_GAME) = 1;
#endif
    ((void (*)(const char *, int))GAME_PTR(RVA_SPELL_BUG_CHECK))((const char *)GAME_PTR(RVA_STR_LOADED_START_MM), 100);
    if (G32(RVA_LOAD_START_TEST) == 0) {
#ifdef __APPLE__
        G8(RVA_LOAD_START_MARK) = 1;
#else
        G32(RVA_LOAD_START_MARK) = 1;
#endif
    }
    return 1;
}

// The loaded-game flag: an int on Windows, a byte on Mac.
static int isLoadedGame(void) {
#ifdef __APPLE__
    return G8(RVA_LOADED_GAME) != 0;
#else
    return G32(RVA_LOADED_GAME) != 0;
#endif
}

static void clearLoadedGame(void) {
#ifdef __APPLE__
    G8(RVA_LOADED_GAME) = 0;
#else
    G32(RVA_LOADED_GAME) = 0;
#endif
}

static int askAboutChangedSpells(void);

// CoE5SE's outermost play_game wrapper.
static u64 playGameHook(int arg) {
    restartPhase = RESTART_NONE;
    loadedTurn = -1;
    dropSpellsOfDisabledMods();
    if (!askAboutChangedSpells()) return 0;
    playGameEntries = 0;
    u64 result = nextPlayGame(arg);
    while (restartPhase == RESTART_ENDING) {
        if (loadPath[0]) {
            restartPhase = RESTART_NONE;
            if (!loadPickedSave()) break;
            dropSpellsOfDisabledMods();
            if (!askAboutChangedSpells()) break;
            playGameEntries = 0;
            result = nextPlayGame(0);
            continue;
        }
        restartPhase = RESTART_STARTING;
        clearLoadedGame();
        writeLog("restart_game", "phase=starting");
        playGameEntries = 0;
        result = nextPlayGame(0);
    }
    restartPhase = RESTART_NONE;
    return result;
}

// CoE5SE's innermost play_game wrapper.
static u64 (*nextPlayGameInner)(int);
static u64 playGameInnermost(int arg) {
    if (playGameEntries++) loadedTurn = -1;
    if (playGameEntries > 1 && restartPhase != RESTART_NONE) {
        writeLog("restart_game", "phase=cancelled reason=play_game_reentered");
        restartPhase = RESTART_NONE;
        loadPath[0] = 0;
    }
    return nextPlayGameInner(arg);
}

// A line shows when its `visible` says so.
static int isMenuLineShown(int i) {
    if (menuLines[i].lua >= 0) return runLuaVisible(menuLines[i].lua);
    if (isNetworkGame() && !runsInNetworkGames((void *)menuLines[i].onClick)) return 0;
    return !menuLines[i].visible || menuLines[i].visible();
}

static void drawMenuLine(int i) {
    textButton(MENU_LINE_FIRST_ID + i, menuLines[i].label);
    if (menuLines[i].tip[0]) tooltip(menuLines[i].tip);
}

// Draws the shown lines of one section, in the order they were added.
static void drawSectionLines(int section) {
    for (int i = 0; i < menuLineCount; i++) {
        if (menuLines[i].section != section || !isMenuLineShown(i)) continue;
        drawMenuLine(i);
    }
}

// 1 when lines i and j are in the same named section: names match whatever their case.
static int isSameNamedSection(int i, int j) {
    return menuLines[i].section == MENU_SECTION_NAMED && menuLines[j].section == MENU_SECTION_NAMED &&
           !_stricmp(menuLines[i].sectionName, menuLines[j].sectionName);
}

// Draws the scripts' named sections after "Saving & Exiting".
static void drawNamedSections(void) {
    for (int i = 0; i < menuLineCount; i++) {
        if (menuLines[i].section != MENU_SECTION_NAMED) continue;
        int first = 1;
        for (int k = 0; k < i && first; k++) first = !isSameNamedSection(k, i);
        if (!first) continue;
        int begun = 0;
        for (int j = i; j < menuLineCount; j++) {
            if (!isSameNamedSection(i, j) || !isMenuLineShown(j)) continue;
            if (!begun) {
                sectionBegin();
                sectionHeader(menuLines[i].sectionName);
                begun = 1;
            }
            drawMenuLine(j);
        }
        if (begun) sectionEnd();
    }
}

// Draws the lines at the end of the Game Menu's "Saving & Exiting" section, then the named sections.
static int drawMenuLines(void) {
    drawSectionLines(GAME_MENU_SECTION_SAVING);
    int result = nextMenuSectionEnd();
    drawNamedSections();
    return result;
}

// Draws the lines at the end of the Game Menu's "Settings" section.
static int drawSettingsMenuLines(void) {
    drawSectionLines(GAME_MENU_SECTION_SETTINGS);
    return nextSettingsSectionEnd();
}

// CoE5SE's outermost Game Menu wrapper.
static int dispatchMenuLineClick(int a, int b) {
    loadedTurn = -1;
    if (restartPhase == RESTART_ENDING) {
        restartPhase = RESTART_NONE;
        loadPath[0] = 0;
        writeLog("restart_game", "phase=cancelled reason=game_continued");
    }
    int id = nextGameMenuWait(a, b);
    int i = id - MENU_LINE_FIRST_ID;
    if (i < 0 || i >= menuLineCount || !isMenuLineShown(i)) return id;
    writeLog("game_menu_line_clicked", "label=%s", menuLines[i].label);
    restartAsked = loadAsked = 0;
    menuLineClicking = 1;
    int result = menuLines[i].lua >= 0 ? runLuaCallback(menuLines[i].lua, 1, 0) : menuLines[i].onClick();
    menuLineClicking = 0;
    if (!restartAsked && !loadAsked) return result;
    if (!loadAsked) loadPath[0] = 0;
    if (loadAsked)
        writeLog("load_game", "phase=ending label=%s asked_by=%s turn=%d", menuLines[i].label, loadAskedBy, G32(RVA_TURN));
    else writeLog("restart_game", "phase=ending label=%s asked_by=%s turn=%d", menuLines[i].label, restartAskedBy, G32(RVA_TURN));
    restartAsked = loadAsked = 0;
    restartPhase = RESTART_ENDING;
    return GAME_MENU_RESIGN;
}

// Wraps the Game Menu's three sites and play_game, at startup.
static int installGameMenuHooks(void) {
    char owner[64];
    moduleFileName((void *)drawMenuLines, owner, sizeof owner);
    int sectionEnd = wrapSite(SITE_GAME_MENU_SAVING_SECTION_END, (void *)drawMenuLines, (void **)&nextMenuSectionEnd);
    int settingsEnd = wrapSite(SITE_GAME_MENU_SETTINGS_SECTION_END, (void *)drawSettingsMenuLines, (void **)&nextSettingsSectionEnd);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int wait = insertWrap(SITE_GAME_MENU_WAIT, (void *)dispatchMenuLineClick, (void **)&nextGameMenuWait, outermost, owner, NULL);
    int playGame = insertWrap(SITE_MAIN_MENU_PLAY_GAME, (void *)playGameHook, (void **)&nextPlayGame, outermost, owner, NULL) &&
                   insertWrap(SITE_MAIN_MENU_PLAY_GAME, (void *)playGameInnermost, (void **)&nextPlayGameInner,
                              (WrapPlace){.priority = INT_MIN}, owner, NULL);
    int eliminated = wrapSite(SITE_ELIMINATED_WARN, (void *)watchEliminatedWarn, (void **)&nextEliminatedWarn) &&
                     wrapSite(SITE_WARN_WAIT, (void *)answerWarnWait, (void **)&nextWarnWait);
    int gameOver = insertWrap(SITE_GAME_OVER_WAIT, (void *)answerGameOverWait, (void **)&nextGameOverWait, outermost, owner, NULL);
    int messages = wrapSite(SITE_TURN_MESSAGES, (void *)watchTurnMessages, (void **)&nextTurnMessages) &&
                   wrapSite(SITE_MESSAGES_WAIT, (void *)answerMessagesWait, (void **)&nextMessagesWait);
    writeLog("game_menu_hooks_installed",
             "section_end=%d settings_end=%d wait=%d play_game=%d eliminated_box=%d game_over=%d messages=%d", sectionEnd,
             settingsEnd, wait, playGame, eliminated, gameOver, messages);
    return sectionEnd && wait;
}

// ---- Inlined button rows ------------------------------------------------------------------------
// Setup Participants and the square info window start their Ok row with an inlined button_row(9).
static uint64_t (*nextParticipantsRowInlined)(short style);
static int participantsRowInlined(void) {
    nextParticipantsRowInlined(LAYOUT_TYPE_BUTTON_ROW);
    return -1;
}

static uint64_t (*nextSquareInfoRowInlined)(short style);
static int squareInfoRowInlined(void) {
    nextSquareInfoRowInlined(LAYOUT_TYPE_BUTTON_ROW);
    return -1;
}

// CoE5SE's outermost wrapper of each, so every other wrapper sees a button_row(9) call.
static void installInlinedButtonRows(void) {
    char owner[64];
    moduleFileName((void *)participantsRowInlined, owner, sizeof owner);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int participants = insertWrap(SITE_PARTICIPANTS_BUTTON_ROW, (void *)participantsRowInlined, (void **)&nextParticipantsRowInlined,
                                  outermost, owner, NULL);
    int squareInfo = insertWrap(SITE_SQUARE_INFO_BUTTON_ROW, (void *)squareInfoRowInlined, (void **)&nextSquareInfoRowInlined,
                                outermost, owner, NULL);
    writeLog("button_row_sites_installed", "participants=%d square_info=%d", participants, squareInfo);
}

// ---- The file browser's rows ------------------------------------------------------------------
// The browser lists rows 0 to RVA_FILE_BROWSER_LAST and gives row n the id FILE_BROWSER_ROW_FIRST + n.
static const char *siteResultMod[SITE_COUNT];

static int isRowPastList(int id, int last) {
    return id >= FILE_BROWSER_ROW_FIRST && id - FILE_BROWSER_ROW_FIRST > last;
}

static int (*nextFileBrowserWait)(int, int);
static int boundFileBrowserWait(int a, int b) {
    siteResultMod[SITE_FILE_BROWSER_WAIT] = NULL;
    int id = nextFileBrowserWait(a, b);
    int last = G32(RVA_FILE_BROWSER_LAST);
    const char *mod = siteResultMod[SITE_FILE_BROWSER_WAIT];
    if (isRowPastList(id, last)) {
        refuseLuaCall(mod ? mod : "?", "FILE_BROWSER_WAIT", "row_past_list", "id=%d last=%d", id, last);
        return 0;
    }
    int hovered = G32(RVA_HOVERED_ID);
    if (id == FILE_BROWSER_DELETE && isRowPastList(hovered, last)) {
        refuseLuaCall(mod ? mod : "?", "FILE_BROWSER_WAIT", "hovered_row_past_list", "hovered=%d last=%d", hovered, last);
        return 0;
    }
    return id;
}

// At startup.
static void installFileBrowserBounds(void) {
    char owner[64];
    moduleFileName((void *)boundFileBrowserWait, owner, sizeof owner);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int wait = insertWrap(SITE_FILE_BROWSER_WAIT, (void *)boundFileBrowserWait, (void **)&nextFileBrowserWait, outermost, owner, NULL);
    writeLog("file_browser_bounds_installed", "wait=%d", wait);
}

// The other screens whose wait's id indexes a game table with no upper bound.
#define UNIT_LIST_UNIT_ID 100000
#define MAP_COMMANDER_ID  20000

static int unitIdPast(int id, int lastUnit) {
    if (id > -UNIT_LIST_UNIT_ID && id < UNIT_LIST_UNIT_ID) return 0;
    long long unit = (id < 0 ? -(long long)id : (long long)id) - UNIT_LIST_UNIT_ID;
    return unit > lastUnit;
}

static int commanderIdPast(int id, int lastCommander) {
    long long magnitude = id < 0 ? -(long long)id : (long long)id;
    if (magnitude < MAP_COMMANDER_ID || magnitude >= 2 * MAP_COMMANDER_ID) return 0;
    return magnitude - MAP_COMMANDER_ID > lastCommander;
}

static int unitListSheetOpen, unitListNeedsSheet;

static int (*nextUnitListWait)(int, int);
static int boundUnitListWait(int a, int b) {
    siteResultMod[SITE_UNIT_LIST_WAIT] = NULL;
    int id = nextUnitListWait(a, b);
    if (unitListNeedsSheet && !unitListSheetOpen) return id;
    int last = G32(RVA_UNIT_COUNT);
    const char *mod = siteResultMod[SITE_UNIT_LIST_WAIT];
    if (unitIdPast(id, last)) {
        refuseLuaCall(mod ? mod : "?", "UNIT_LIST_WAIT", "unit_past_table", "id=%d last=%d", id, last);
        return 0;
    }
    return id;
}

static int dispatchCommanderLineClick(int id);
static int (*nextMapScreenWait)(int, int);
static int boundMapScreenWait(int a, int b) {
    siteResultMod[SITE_MAP_SCREEN_WAIT] = NULL;
    int id = nextMapScreenWait(a, b);
    int last = G32(RVA_LAST_COMMANDER);
    const char *mod = siteResultMod[SITE_MAP_SCREEN_WAIT];
    if (commanderIdPast(id, last)) {
        refuseLuaCall(mod ? mod : "?", "MAP_SCREEN_WAIT", "commander_past_table", "id=%d last=%d", id, last);
        return 0;
    }
    if (dispatchCommanderLineClick(id)) return 0;   // Logged by dispatchCommanderLineClick
    if (!id) showLuaNotices();
    return id;
}

// At startup.
static void installWaitIdBounds(void) {
    char owner[64];
    moduleFileName((void *)boundUnitListWait, owner, sizeof owner);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int unitList = insertWrap(SITE_UNIT_LIST_WAIT, (void *)boundUnitListWait, (void **)&nextUnitListWait, outermost, owner, NULL);
    int mapScreen = insertWrap(SITE_MAP_SCREEN_WAIT, (void *)boundMapScreenWait, (void **)&nextMapScreenWait, outermost, owner, NULL);
    writeLog("wait_id_bounds_installed", "unit_list=%d map_screen=%d", unitList, mapScreen);
}

// Adds a Game Menu line to `section` for the plugin or script that called.
static int addGameMenuLineFor(void *caller, const char *label, const char *tip, int (*visible)(void), int (*onClick)(void), int lua,
                              int section, const char *sectionName) {
    if (!label || !label[0]) {
        refuseApiCall(caller, "addGameMenuLine", "no_label", "tooltip=%s", tip ? tip : "");
        return 0;
    }
    if (!onClick && lua < 0) {
        refuseApiCall(caller, "addGameMenuLine", "no_callback", "label=%s", label);
        return 0;
    }
    int named = section == MENU_SECTION_NAMED && sectionName;
    if (section != GAME_MENU_SECTION_SAVING && section != GAME_MENU_SECTION_SETTINGS && !named) {
        refuseApiCall(caller, "addGameMenuLine", "unknown_section", "label=%s section=%d", label, section);
        return 0;
    }
    if (named && !sectionName[0]) {
        refuseApiCall(caller, "addGameMenuLine", "no_section_name", "label=%s", label);
        return 0;
    }
    if (!nextGameMenuWait || !(section == GAME_MENU_SECTION_SETTINGS ? nextSettingsSectionEnd : nextMenuSectionEnd)) {
        refuseApiCall(caller, "addGameMenuLine", "site_unavailable", "label=%s section=%d", label, section);
        return 0;
    }
    if (!growList((void **)&menuLines, &menuLineCapacity, menuLineCount + 1, sizeof menuLines[0])) {
        refuseApiCall(caller, "addGameMenuLine", "out_of_memory", "label=%s", label);
        return 0;
    }
    MenuLine *line = &menuLines[menuLineCount++];
    noteApiCut(caller, "addGameMenuLine", "label", label, (int)sizeof line->label - 1);
    noteApiCut(caller, "addGameMenuLine", "tooltip", tip ? tip : "", (int)sizeof line->tip - 1);
    snprintf(line->label, sizeof line->label, "%.63s", label);
    snprintf(line->tip, sizeof line->tip, "%.255s", tip ? tip : "");
    if (named) noteApiCut(caller, "addGameMenuLine", "section", sectionName, (int)sizeof line->sectionName - 1);
    snprintf(line->sectionName, sizeof line->sectionName, "%.63s", named ? sectionName : "");
    line->visible = visible;
    line->onClick = onClick;
    line->lua = lua;
    line->section = section;
    char owner[MAX_PATH];
    const char *ownerField = apiCallerName(caller, owner, sizeof owner);
    if (named) writeLog("game_menu_line_added", "%s=%s label=%s section=%d section_name=%s", ownerField, owner, label, section, line->sectionName);
    else writeLog("game_menu_line_added", "%s=%s label=%s section=%d", ownerField, owner, label, section);
    return 1;
}

static int apiAddGameMenuLine(const char *label, const char *tip, int (*visible)(void), int (*onClick)(void)) {
    return addGameMenuLineFor(__builtin_return_address(0), label, tip, visible, onClick, -1, GAME_MENU_SECTION_SAVING, NULL);
}
