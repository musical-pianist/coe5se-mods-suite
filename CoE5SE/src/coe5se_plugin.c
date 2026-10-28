// The in-game side of CoE5SE, coe5se.c5plugin.dll (coe5se.c5plugin.dylib on Mac).
#define COE5SE_RAW_ADDRESSES
#include "common.h"
#include "build_path.h"
#include "jazzy_api.h"
#define USER_DIR_FROM_INCLUDER
#include "approvals.h"
#include "auto_enable.h"
#include "loader_start.h"
#include "settings_files.h"

// The game's user folder.
static int userDirFollowsGame = -1;
static const char *userDir(void) {
    if (userDirFollowsGame < 0) {
        HMODULE loader = GetModuleHandleA(LOADER_DLL_NAME);
        const char *(*loaderUserDir)(void) = loader ? (const char *(*)(void))(void *)GetProcAddress(loader, "coe5seUserDir") : NULL;
        int (*followsGame)(void) = loader ? (int (*)(void))(void *)GetProcAddress(loader, "coe5seUserDirFollowsGame") : NULL;
        const char *dir = loaderUserDir ? loaderUserDir() : NULL;
        userDirFollowsGame = followsGame ? followsGame() != 0 : 1;
        if (dir && dir[0]) snprintf(userDirPath, sizeof userDirPath, "%s", dir);
        else if (!legacyUserDir(userDirPath, sizeof userDirPath)) snprintf(userDirPath, sizeof userDirPath, "%s", USER_DIR_FALLBACK);
    }
    const char *game = gameBase ? (const char *)GAME_PTR(RVA_USER_DIR) : "";
    if (userDirFollowsGame && game[0]) return game;
    return userDirPath;
}
#include "c5m_words.h"
#include "setting_rules.h"
#include "mod_order.h"
#include <limits.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <crt_externs.h>
#include <mach-o/loader.h>
#include <spawn.h>
#endif

typedef unsigned long long u64;

// Appends printf-style text at text + *n, in a buffer of `size` bytes, and moves *n past it.
static PRINTF_FORMAT(4, 5) void appendText(char *text, size_t size, size_t *n, const char *fmt, ...) {
    if (*n >= size - 1) return;
    va_list ap;
    va_start(ap, fmt);
    int wrote = vsnprintf(text + *n, size - *n, fmt, ap);
    va_end(ap);
    if (wrote > 0) *n = *n + (size_t)wrote < size - 1 ? *n + (size_t)wrote : size - 1;
}

// The game's message box with one Ok button, which Enter clicks.
static int okBox(const char *title, const char *text) {
    return messageBox(title, text, "Ok", "", 0, UI_KEY_ENTER, 0);
}

// ---- Refusals ----------------------------------------------------------------------------------
// Every input CoE5SE refuses or drops logs one line with its reason.
#define MAX_REFUSALS_SEEN 128
static unsigned int refusalsSeen[MAX_REFUSALS_SEEN];
static unsigned int refusalSeenCount;

static HMODULE callingPlugin(void *returnAddress);
static HMODULE moduleOf(const void *address);
static int isCoe5seCode(const void *address);
static int runsInNetworkGames(const void *address);
static const char *runningLuaMod(void);
static PRINTF_FORMAT(4, 5) void refuseLuaCall(const char *mod, const char *call, const char *reason, const char *fmt, ...);
static int isScriptObserver(const void *fn);
static void showLuaNotices(void);
static void logHeaderEnd(void);

// Logs `event` with the refusal's fields, unless the same line was already logged this launch.
static void logRefusal(const char *event, const char *call, const char *modField, const char *mod,
                       const char *reason, const char *fmt, va_list ap) {
    char fields[512], line[768];
    vsnprintf(fields, sizeof fields, fmt, ap);
    snprintf(line, sizeof line, "call=%s %s=%s reason=%s %s", call, modField, mod, reason, fields);
    unsigned int hash = 2166136261u;
    for (const char *c = line; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
    unsigned int seen = refusalSeenCount < MAX_REFUSALS_SEEN ? refusalSeenCount : MAX_REFUSALS_SEEN;
    for (unsigned int i = 0; i < seen; i++)
        if (refusalsSeen[i] == hash) return;
    refusalsSeen[refusalSeenCount++ % MAX_REFUSALS_SEEN] = hash;
    writeLog(event, "%s", line);
}

// Names who made a PluginApi call: the script's mod, or the plugin DLL's file name.
static const char *apiCallerName(void *caller, char *out, size_t size) {
    char path[MAX_PATH] = "?";
    HMODULE m = callingPlugin(caller);
    if (m) GetModuleFileNameA(m, path, sizeof path);
    const char *dll = strrchr(path, PATH_SEP[0]);
    const char *luaMod = m && isCoe5seCode((void *)m) ? runningLuaMod() : NULL;
    snprintf(out, size, "%s", luaMod ? luaMod : dll ? dll + 1 : path);
    return luaMod ? "mod" : "dll";
}

// Logs `event` for a PluginApi call.
static void logApiCall(const char *event, void *caller, const char *call, const char *reason, const char *fmt, va_list ap) {
    char name[MAX_PATH];
    const char *field = apiCallerName(caller, name, sizeof name);
    logRefusal(event, call, field, name, reason, fmt, ap);
}

// Logs api_refused for a PluginApi call that `caller` made.
static PRINTF_FORMAT(4, 5) void refuseApiCall(void *caller, const char *call, const char *reason, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logApiCall("api_refused", caller, call, reason, fmt, ap);
    va_end(ap);
}

static PRINTF_FORMAT(3, 4) void logApiCut(void *caller, const char *call, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logApiCall("api_cut", caller, call, "too_long", fmt, ap);
    va_end(ap);
}

// Logs api_cut when `text` is longer than `max` characters.
static void noteApiCut(void *caller, const char *call, const char *field, const char *text, int max) {
    if (strlen(text) <= (size_t)max) return;
    logApiCut(caller, call, "%s=%.80s max=%d", field, text, max);
}

// Refuses a PluginApi call: logs why, then returns `value`.
#define REFUSE_API(value, call, reason, ...) \
    (refuseApiCall(__builtin_return_address(0), call, reason, __VA_ARGS__), (value))

static int menuFrames;
static int startupFinished;
static float *menuBgAlpha;
static float *menuFadeTimer;

#define COE5SE_PAGE_TITLE "CoE5SE"

// Defined further down.
static void coe5sePageScreen(void);
static void readUnreadSettingsFiles(void);
static unsigned int settingsPassCount(void);
static void rereadSettingsWithoutPass(unsigned int before);
static void popupSettingErrors(void);
static void noteModLine(const char *mod, int level);
static int refusedRitualLine(const char *line, size_t len, int mod, int level);
static void popupRitualsLeftOut(void);
static void drawRitualsLeftOut(void);
static int refusedTableLine(const char *line, size_t len, int mod, int level);
static void popupTablesLeftOut(void);
static void drawTablesLeftOut(void);
static int spellLineHook(const char *line, size_t len, int mod, int level);
static const char *spellPathLineHook(const char *line, size_t len, int mod, int level);
static const char *offsetNameLineHook(const char *line, size_t len, int mod, int level);
static int declinedOffsetNameLine(const char *line, size_t len, int level);
static const char *nameListLineHook(const char *line, size_t len, int mod, int level);
static int declinedNameListLine(const char *line, size_t len, int level);
static const char *planeWordLineHook(const char *line, size_t len, int mod, int level);
static int declinedPlaneLine(const char *line, size_t len, int mod, int level);
static const char *planeNameEventLine(const char *line, int level, const char *mod);
static int declinedPlaneEventLine(const char *line);
static void planeWordsBeforePass(int level);
static void planeWordsAfterPass(int level);
static const char *factionWordLineHook(const char *line, size_t len, int mod, int level);
static void factionWordsBeforePass(int level);
static void factionWordsAfterPass(int level);
static void applyLoadedSavePlanes(int level);
static void applyLoadedSaveFactions(int level);
static const char *factionNameLineHook(const char *line, size_t len, int mod, int level);
static const char *factionNameEventLine(const char *line, int level, const char *mod);
static int declinedFactionNameLine(const char *line, size_t len);
static void factionNamesBeforePass(int level);
static void resolveFactionNames(int level);
static void clearLuaPlaneTints(int level);
static const char *terrainLooksLineHook(const char *line, size_t len, int mod, int level);
static int declinedTerrainLooksLine(const char *line, size_t len, int mod, int level);
static void terrainLooksBeforePass(int level);
static void terrainLooksAfterPass(int level);
static const char *battleMapLineHook(const char *line, size_t len, int mod, int level);
static int declinedBattleMapLine(const char *line, size_t len, int mod, int level);
static void battleMapsBeforePass(int level);
static void battleMapsAfterPass(int level);
static void popupMissingPlanes(void);
static int settingLineHook(const char *line, size_t len, int mod, int level);
static int eventLogicLine(const char *line, int level);
static void eventLogicAfterLine(const char *line, int level);
static int isEventLogicWord(const char *line);
static int declinedSpellPathLine(const char *word, int mod, int level);
static int declinedSpellLine(const char *word, int mod, int level);
static int flagOffLineHook(const char *line, size_t len, int mod, int level);
static int weaponTraitLineHook(const char *line, size_t len, int level);
static void weaponTraitsBeforePass(int level);
static void ritualWordsAfterPass(int level);
static const char *corpseWordLineHook(const char *line, size_t len, int mod, int level);
static int corpseEventLine(const char *line, int level);
static void corpseWordsBeforePass(int level);
static void corpseWordsAfterPass(int level);
static void noExplodeLineHook(const char *line, size_t len, int mod, int level);
static void aiClassLineHook(const char *line, size_t len, int mod, int level);
static void aiClassBeforePass(int level);
static void aiClassAfterPass(int level);
static void aiClassInit(void);
static const char *classNameLineHook(const char *line, size_t len, int mod, int level);
static void noExplodeAfterPass(int level);
static void noExplodeBeforePass(int level);
static void recBonusLineHook(const char *line, size_t len, int mod, int level);
static void recBonusBeforePass(int level);
static int recBonusOfferCopy(int player, const unsigned char *record, unsigned char *copy);
static struct RecBonusRoll { const unsigned char *record, *rolled; int unitsTried, unitsHooked; } recBonusRoll;
static void recBonusAfterRoll(int player, const unsigned char *record, const unsigned char *copy, int entryBefore);
static int declinedFlagOffLine(const char *line);
static void finishSpellBlock(void);
static unsigned int spellPassCount(void);
static void drawSpellProblems(void);
static void popupSpellProblems(void);
static void spellsAfterPass(int level, unsigned int passesBefore);
static void spellsAfterModsScreen(unsigned int passesBefore);
static void dropSpellsOfDisabledMods(void);
static void modDisplayName(const char *mod, char *out, size_t size);
static void readLoadOrder(int logIt);
static void drawModRequirements(void);
static void drawCustomPlaneProblems(void);
static const char *pluginModVersion(const char *pluginPath);
static void warnModsMissingPlugins(void);
static void offerStubUpdate(void);
static int listModsMissingPlugins(int logIt);
static void drawModsMissingPlugins(int count);
static int (*nextMainMenuWait)(int, int);
static int runLuaCallback(int i, int wantResult, int fallback);
static int runLuaVisible(int i);
static int luaHidesReadTip(void);

// The main menu buttons that plugins and scripts added.
#define MAIN_BUTTON_FIRST_ID 780
#define MORE_BUTTON_ID       779
#define MORE_LABEL           "More"
typedef struct { char label[48]; void (*onClick)(void); int lua, afterId; } MainButton;
static MainButton *mainButtons;
static int mainButtonCount, mainButtonCapacity;

// Adds a main menu button for the plugin or script that called.
static int addMainMenuButtonFor(void *caller, const char *label, void (*onClick)(void), int lua, int afterId) {
    if (!label || !label[0]) {
        refuseApiCall(caller, "addMainMenuButton", "no_label", "after=%d", afterId);
        return 0;
    }
    if (!onClick && lua < 0) {
        refuseApiCall(caller, "addMainMenuButton", "no_callback", "label=%s", label);
        return 0;
    }
    if (!growList((void **)&mainButtons, &mainButtonCapacity, mainButtonCount + 1, sizeof mainButtons[0])) {
        refuseApiCall(caller, "addMainMenuButton", "out_of_memory", "label=%s", label);
        return 0;
    }
    MainButton *b = &mainButtons[mainButtonCount++];
    noteApiCut(caller, "addMainMenuButton", "label", label, (int)sizeof b->label - 1);
    snprintf(b->label, sizeof b->label, "%.47s", label);
    b->onClick = onClick;
    b->lua = lua;
    b->afterId = afterId;
    char owner[MAX_PATH];
    const char *ownerField = apiCallerName(caller, owner, sizeof owner);
    writeLog("main_menu_button_added", "%s=%s label=%s after=%d", ownerField, owner, label, afterId);
    return 1;
}

static int apiAddMainMenuButtonAfter(const char *label, void (*onClick)(void), int afterId) {
    return addMainMenuButtonFor(__builtin_return_address(0), label, onClick, -1, afterId);
}

static int apiAddMainMenuButton(const char *label, void (*onClick)(void)) {
    return addMainMenuButtonFor(__builtin_return_address(0), label, onClick, -1, -1);
}

// Runs button i's onClick, or its script's function.
static void runMainButton(int i) {
    writeLog("main_menu_button_clicked", "label=%s", mainButtons[i].label);
    if (mainButtons[i].lua >= 0) runLuaCallback(mainButtons[i].lua, 0, 0);
    else mainButtons[i].onClick();
}

// The vanilla button plugins may hide with hideMainMenuButton.
#define HIDDEN_Y         (-1000.0f)
static const int **readTipHiders;
static int readTipHiderCount, readTipHiderCapacity;

static int apiHideMainMenuButton(int id, const int *hidden) {
    if (id != MAIN_MENU_READ_TIP) return REFUSE_API(0, "hideMainMenuButton", "not_hidable", "id=%d", id);
    if (!hidden) return REFUSE_API(0, "hideMainMenuButton", "no_value", "id=%d", id);
    for (int i = 0; i < readTipHiderCount; i++)
        if (readTipHiders[i] == hidden) return 1;
    if (!growList((void **)&readTipHiders, &readTipHiderCapacity, readTipHiderCount + 1, sizeof readTipHiders[0]))
        return REFUSE_API(0, "hideMainMenuButton", "out_of_memory", "id=%d", id);
    readTipHiders[readTipHiderCount++] = hidden;
    writeLog("main_menu_button_hidable", "id=%d plugins=%d", id, readTipHiderCount);
    return 1;
}

static int readTipHidden(void) {
    int hidden = luaHidesReadTip();
    for (int i = 0; i < readTipHiderCount && !hidden; i++)
        if (*readTipHiders[i]) hidden = 1;
    return hidden;
}

// The game's main menu buttons.
static const int VANILLA_MENU_IDS[] = {MAIN_MENU_START_NEW_GAME, 0, MAIN_MENU_LOAD_GAME, MAIN_MENU_NETWORK, MAIN_MENU_PREFERENCES,
                                       MAIN_MENU_MODS, MAIN_MENU_CREDITS, MAIN_MENU_READ_TIP, MAIN_MENU_QUIT};
#define VANILLA_BUTTONS  8
#define MENU_FIRST_Y     305
#define MENU_STEP        25.0f
#define MENU_X_OFFSET    80.0f
// The game's own button and background sizes.
#define MENU_LABEL_SIZE  20.0f
#define MENU_BUTTON_W    160.0f
#define MENU_BG_INSET    5.0f
#define MENU_BG_DROP     1.0f

static int isVanillaMenuId(int id) {
    for (size_t i = 0; i < sizeof VANILLA_MENU_IDS / sizeof VANILLA_MENU_IDS[0]; i++)
        if (VANILLA_MENU_IDS[i] == id) return 1;
    return 0;
}

// The id in GUI slot `slot`.
static int guiId(int slot) { return *(int *)(gameBase + RVA_GUI_TABLE + (size_t)slot * GUI_STRIDE); }

// Returns Quit's slot: the last slot before `before` that holds Quit.
static int findQuitSlot(int before) {
    for (int q = before - 1; q >= 2 * (VANILLA_BUTTONS - 1); q--) {
        if (guiId(q) != MAIN_MENU_QUIT) continue;
        int known = 1;
        for (int s = q - 2 * (VANILLA_BUTTONS - 1); s < q && known; s += 2) known = isVanillaMenuId(guiId(s));
        if (known) return q;
    }
    return -1;
}

// How many buttons fit in the column on a screen `screenH` high.
static int mainMenuRows(int screenH) {
    return screenH < MENU_FIRST_Y ? 1 : (screenH - MENU_FIRST_Y) / (int)MENU_STEP + 1;
}

// Puts the main menu's buttons in order.
typedef struct { int slot, id; float width; void *label; } MenuSlot;
static int arrangeMainMenu(const int *slots, const int *buttons, int count, int hide) {
    static MenuSlot *column, *saved;
    static int *order, *placed;
    static int columnCap, savedCap, orderCap, placedCap;
    int n = VANILLA_BUTTONS + count;
    if (!growList((void **)&column, &columnCap, n, sizeof column[0]) || !growList((void **)&saved, &savedCap, n, sizeof saved[0]) ||
        !growList((void **)&order, &orderCap, n, sizeof order[0]) || !growList((void **)&placed, &placedCap, count + 1, sizeof placed[0])) {
        static int logged;
        if (!logged) writeLog("main_menu_arrange_failed", "reason=out_of_memory buttons=%d", count);
        logged = 1;
        return 0;
    }
    int quit = findQuitSlot(slots[0]);
    if (quit < 0) return 0;
    for (int v = 0; v < VANILLA_BUTTONS; v++) column[v].slot = quit - 2 * (VANILLA_BUTTONS - 1) + 2 * v;
    for (int k = 0; k < count; k++) column[VANILLA_BUTTONS + k].slot = slots[k];
    for (int i = 0; i < n; i++) {
        unsigned char *e = gameBase + RVA_GUI_TABLE + (size_t)column[i].slot * GUI_STRIDE;
        column[i].id = *(int *)e;
        column[i].width = *(float *)(e + GUI_ELEMENT_WIDTH);
        column[i].label = *(void **)(e + GUI_ELEMENT_LABEL);
    }
    int o = 0, hidden = -1;
    memset(placed, 0, sizeof placed[0] * (size_t)(count + 1));
    for (int v = 0; v < VANILLA_BUTTONS - 1; v++) {
        if (hide && column[v].id == MAIN_MENU_READ_TIP) { hidden = v; continue; }
        order[o++] = v;
        for (int k = 0; k < count; k++)
            if (buttons[k] >= 0 && mainButtons[buttons[k]].afterId == column[v].id) { order[o++] = VANILLA_BUTTONS + k; placed[k] = 1; }
    }
    for (int k = 0; k < count; k++)
        if (!placed[k]) order[o++] = VANILLA_BUTTONS + k;
    order[o++] = VANILLA_BUTTONS - 1;
    if (hidden >= 0) order[o++] = hidden;
    memcpy(saved, column, sizeof column[0] * (size_t)n);
    for (int i = 0; i < n; i++) {
        unsigned char *e = gameBase + RVA_GUI_TABLE + (size_t)column[i].slot * GUI_STRIDE;
        *(int *)e = saved[order[i]].id;
        *(float *)(e + GUI_ELEMENT_WIDTH) = saved[order[i]].width;
        *(void **)(e + GUI_ELEMENT_LABEL) = saved[order[i]].label;
    }
    if (hidden >= 0) {
        unsigned char *last = gameBase + RVA_GUI_TABLE + (size_t)column[n - 1].slot * GUI_STRIDE;
        *(int *)last = 0;
        *(float *)(last + GUI_ELEMENT_Y) = HIDDEN_Y;
    }
    return 1;
}

static void takeCodeModSnapshot(void);
static void loadLuaScripts(void);
static void loadLuaCommandsFiles(void);
static void logAllWrapOrders(void);
static void moreButtonsScreen(int first);

// Draws one button of the column and returns its GUI slot, or -1 if the game gave it none.
static int drawMenuButton(int id, float x, float y, float fade, const char *text) {
    static char label[TRANSLATED_SIZE];
    translateText(text, label, sizeof label);
#ifdef __APPLE__
    int hint = G32(RVA_GUI_NEXT_SLOT);
    ((DrawButtonFn)(gameBase + RVA_DRAW_BUTTON))(id, x, y, MENU_LABEL_SIZE, 0.0f, 0.0f, 0.0f, fade, "%s", label);
    return G32(RVA_GUI_NEXT_SLOT) != hint ? G32(RVA_GUI_NEXT_SLOT) - 1 : -1;
#else
    return (int)(long long)((DrawButtonFn)(gameBase + RVA_DRAW_BUTTON))(id, x, y, MENU_LABEL_SIZE, 0.0f, 0.0f, 0.0f, fade, "%s", label);
#endif
}

// The one-time message for a player whose stub turned CoE5SE on.
static void showAutoEnableNotice(void) {
    char notice[MAX_PATH];
    if (!coe5seUserFile(AUTO_ENABLE_NOTICE_FILE, notice, sizeof notice)) return;
    if (!autoEnableFileExists(notice)) return;
    int deleted = remove(notice) == 0;
    writeLog("coe5se_auto_enable_notice", "deleted=%d", deleted);
    okBox("CoE5SE", "CoE5SE was turned on automatically.\nYou can turn it off at any time on the Mods screen.");
}

// The main menu column as drawMenuColumn drew it this frame.
typedef struct {
    float x, top, bgAlpha;
    int room;
    int shown, more;
    int count, moved;
} MenuColumn;

// Draws the plugins' buttons under the game's, with More last when they don't all fit.
static void drawMenuColumn(MenuColumn *col) {
    int screenW = *(int *)(gameBase + RVA_SCREEN_W);
    int screenH = *(int *)(gameBase + RVA_SCREEN_H);
    col->x = (float)(screenW / 2) - MENU_X_OFFSET;
    col->top = (float)(screenH - MENU_FIRST_Y);

    char bgPath[MAX_PATH];
    int haveBg = buildPath(bgPath, sizeof bgPath, __func__, "%s/butt512x64w.tga", (const char *)(gameBase + RVA_DATA_DIR));

    col->bgAlpha = *menuBgAlpha;
    int hide = readTipHidden();
    col->room = mainMenuRows(screenH) - (VANILLA_BUTTONS - hide);
    if (col->room < 1) col->room = 1;
    col->more = mainButtonCount > col->room;
    col->shown = col->more ? col->room - 1 : mainButtonCount;
    int drawCount = col->shown + col->more;
    if (hide && drawCount == 0) hide = 0;
    static int *slots, *buttons, slotsCap, buttonsCap;
    int holderDrawn = 0;
    col->count = 0;
    if (growList((void **)&slots, &slotsCap, drawCount, sizeof slots[0]) &&
        growList((void **)&buttons, &buttonsCap, drawCount, sizeof buttons[0])) {
        for (int k = 0; k < drawCount; k++) {
            int button = k < col->shown ? k : -1;
            int holder = hide && k == drawCount - 1;
            float y = col->top - MENU_STEP * (float)(holder ? VANILLA_BUTTONS - 1 : VANILLA_BUTTONS + k);
            float fade = *menuFadeTimer - 4.0f - 0.5f * (float)k;
            if (!holder && haveBg)
                ((DrawImageFn)(gameBase + RVA_DRAW_IMAGE))(0, col->x - MENU_BG_INSET, y - MENU_BG_DROP, MENU_BUTTON_W, MENU_LABEL_SIZE,
                                                          1.0f, 1.0f, 1.0f, col->bgAlpha, "%s", bgPath);
            int slot = drawMenuButton(button >= 0 ? MAIN_BUTTON_FIRST_ID + button : MORE_BUTTON_ID, col->x, y, fade,
                                      button >= 0 ? mainButtons[button].label : MORE_LABEL);
            if (slot < 0) continue;
            holderDrawn |= holder;
            slots[col->count] = slot;
            buttons[col->count++] = button;
        }
    }
    col->moved = col->count && arrangeMainMenu(slots, buttons, col->count, holderDrawn);
    static int moreBefore = 0;
    if (col->more != moreBefore) {
        moreBefore = col->more;
        writeLog("main_menu_more", "on=%d rows=%d in_column=%d in_more=%d", col->more, col->room, col->shown,
                 mainButtonCount - col->shown);
    }
}

// The end of CoE5SE's startup, once a launch, after the game has read its mods.
#ifdef __APPLE__
static long paddingLeft(void);
#endif
static void finishStartup(void) {
    if (startupFinished) return;
    takeCodeModSnapshot();
    readLoadOrder(1);
    readUnreadSettingsFiles();
    loadLuaCommandsFiles();
    loadLuaScripts();
    logAllWrapOrders();
#ifdef __APPLE__
    writeLog("header_padding_left", "bytes=%ld", paddingLeft());
#endif
    logHeaderEnd();
    startupFinished = 1;
}

// The first main menu frame: the game has found its mods by now.
static void onFirstMenuFrame(const MenuColumn *col) {
    finishStartup();
    writeLog("menu_hook_first_frame", "screen_w=%d screen_h=%d x=%.1f top=%.1f bg_alpha=%.3f fade_timer=%.3f buttons=%d room=%d quit_moved=%d",
             *(int *)(gameBase + RVA_SCREEN_W), *(int *)(gameBase + RVA_SCREEN_H), col->x, col->top, col->bgAlpha, *menuFadeTimer,
             col->count, col->room, col->moved);
}

// Once per launch: CoE5SE's main menu notices and pop-ups.
static void showLaunchNotices(void) {
    static int shown;
    if (shown) return;
    shown = 1;
    showAutoEnableNotice();
    offerStubUpdate();
    warnModsMissingPlugins();
}

// Whether this main menu frame may show CoE5SE's notices and pop-ups.
#define MENU_NOTICE_WAIT_FRAMES 60
static int menuClickedFrames;
static int menuNoticesDue(int clicked) {
    if (!clicked) {
        menuClickedFrames = 0;
        return 1;
    }
    if (++menuClickedFrames < MENU_NOTICE_WAIT_FRAMES) return 0;
    menuClickedFrames = 0;
    writeLog("menu_notices_forced", "clicked=%d frames=%d", clicked, MENU_NOTICE_WAIT_FRAMES);
    return 1;
}

// Wraps each main menu frame.
static int menuWaitHook(int a, int b) {
    MenuColumn col;
    drawMenuColumn(&col);
    if (menuFrames == 0) onFirstMenuFrame(&col);
    menuFrames++;
    int clicked = nextMainMenuWait(a, b);
    if (clicked == MORE_BUTTON_ID && col.more) {
        moreButtonsScreen(col.shown);
        return 0;
    }
    if (clicked >= MAIN_BUTTON_FIRST_ID && clicked < MAIN_BUTTON_FIRST_ID + mainButtonCount) {
        runMainButton(clicked - MAIN_BUTTON_FIRST_ID);
        return 0;
    }
    int quiet = menuNoticesDue(clicked);
    if (quiet) showLaunchNotices();
    if (quiet) popupSettingErrors();
    finishSpellBlock();
    if (quiet) popupSpellProblems();
    if (quiet) popupRitualsLeftOut();
    if (quiet) popupTablesLeftOut();
    if (quiet) popupMissingPlanes();
    if (quiet) showLuaNotices();
    return clicked;
}

// ---- Plugin API: call sites -----------------------------------------------------------------
// A site is one or more call instructions in the game with the same target.

// The catalog of sites, with both builds' addresses, is in game_addresses.h.
#define GAME_ADDRESSES_SITE_TABLE
#include "game_addresses.h"
#include "player_slots.h"

// 1 when the game's code section hashed to its known build's (checkGameCode): every patch of the
static int gameCodeMatches;

// The sites whose calls are an inlined button_row's layout_new.
static int isInlinedButtonRowSite(CallSite site) {
    return site == SITE_PARTICIPANTS_BUTTON_ROW || site == SITE_SQUARE_INFO_BUTTON_ROW;
}

#include "x86_code.h"

#ifndef __aarch64__
// Each site's stub is 0x40 bytes of code, and all of them share two read-execute pages near the game's
typedef struct {
    void *head;
    uintptr_t rsp;
    float xmm12;
    float xmm13;
} StubData;
#define STUB_PAGES_BYTES 0x2000
#define STUB_DATA_BYTES  0x1000
_Static_assert(SITE_COUNT * STUB_SIZE <= STUB_PAGES_BYTES, "every site's stub must fit the pages allocNearBytes gives");
_Static_assert(SITE_COUNT * sizeof(StubData) <= STUB_DATA_BYTES, "every site's data must fit the data page");
static unsigned char *stubPage;
static int siteInstalled[SITE_COUNT];
static int setupIsAutomated;

static unsigned char *siteStub(CallSite site) { return stubPage + site * STUB_SIZE; }
static StubData *siteStubData(CallSite site) { return (StubData *)(stubPage + STUB_PAGES_BYTES) + site; }

// The caller's rsp at the site's call, as on Mac.
static uintptr_t siteCallerSp(CallSite site) { return siteStubData(site)->rsp + 8; }

// The stubs' pages: code read-execute, data read-write, within 2 GB of `target`; NULL when there is no room.
static unsigned char *allocStubPages(unsigned char *target) {
    unsigned char *pages = allocNearBytes(target, STUB_PAGES_BYTES + STUB_DATA_BYTES);
    if (!pages) return NULL;
    makePagesReadExecute(pages, STUB_PAGES_BYTES);
    makePagesReadWrite(pages + STUB_PAGES_BYTES, STUB_DATA_BYTES);
    return pages;
}

// Makes game data in .rdata writable.
static int makeGameDataWritable(void *address, size_t size) {
    DWORD old;
    return VirtualProtect(address, size, PAGE_READWRITE, &old) != 0;
}

static int prepareSiteCode(CallSite site);
static void gameStartPrepOnce(void), removeGameStartOnceJump(void);
static unsigned char *generatedSiteCall(CallSite site), *generatedSiteTarget(CallSite site);

// Call i of a site (NULL past its last), and the function its calls go to.
static int siteThroughSlot[SITE_COUNT];
static unsigned char *siteCallAt(CallSite site, int i) {
    unsigned char *generated = generatedSiteCall(site);
    if (generated) return i == 0 ? generated : NULL;
    return i < MAX_SITE_CALLS && SITES[site].rvas[i] ? gameBase + SITES[site].rvas[i] : NULL;
}
static int topBarIncomeAsGame(int player, int resource, int income);
static unsigned char *siteCallTarget(CallSite site) {
    unsigned char *generated = generatedSiteTarget(site);
    if (generated) return generated;
    if (site == SITE_TOP_BAR_INCOME) return (unsigned char *)topBarIncomeAsGame;
    if (!SITES[site].targetRva) return NULL;
    unsigned char *target = gameBase + SITES[site].targetRva;
    return siteThroughSlot[site] ? *(unsigned char **)target : target;
}

// Whether the call at `call` goes to `target`.
static int isSiteCall(const unsigned char *call, const unsigned char *target, const unsigned char *slot) {
    int rel;
    if (slot) {
        memcpy(&rel, call + 2, 4);
        return call[0] == 0xff && call[1] == 0x15 && call + 6 + rel == slot;
    }
    memcpy(&rel, call + 1, 4);
    return call[0] == 0xe8 && call + 5 + rel == target;
}

// Whether CoE5SE runs the site's chain itself, from its own code, with no game call of the site's.
static int siteRunByCoe5se(CallSite site) { return site == SITE_MAP_DRAW || site == SITE_TOP_BAR_INCOME; }

// Builds the site's stub and points every call of the site at it.
static int installSite(CallSite site) {
    const SiteInfo *info = &SITES[site];
    if (!prepareSiteCode(site)) {
        writeLog("site_skipped", "site=%s reason=not_the_expected_code", info->name);
        return 0;
    }
    int ownCall = siteRunByCoe5se(site);
    unsigned char *first = ownCall ? siteCallTarget(site) : siteCallAt(site, 0);
    if (!first) {
        writeLog("site_skipped", "site=%s reason=not_on_this_build", info->name);
        return 0;
    }
    unsigned char *slot = !ownCall && !generatedSiteCall(site) && first[0] == 0xff && first[1] == 0x15 ? gameBase + info->targetRva : NULL;
    unsigned char *target = slot ? *(unsigned char **)slot : siteCallTarget(site);
    for (int i = 0; !ownCall && siteCallAt(site, i); i++) {
        unsigned char *call = siteCallAt(site, i);
        if (!isSiteCall(call, target, slot) || !target) {
            writeLog("site_skipped", "site=%s reason=not_the_expected_call address=%p", info->name, call);
            return 0;
        }
    }
    siteThroughSlot[site] = slot != NULL;
    if (!stubPage && !(stubPage = allocStubPages(ownCall ? gameBase : first))) {
        writeLog("site_skipped", "site=%s reason=no_near_page", info->name);
        return 0;
    }
    unsigned char *stub = siteStub(site), code[STUB_SIZE];
    StubData *data = siteStubData(site);
    SiteStubSlots slots = {
        .xmm12 = info->captureMenuFade ? &data->xmm12 : NULL,
        .xmm13 = info->captureMenuFade ? &data->xmm13 : NULL,
        .ecxFrom = site == SITE_AI_TURN ? GAME_PTR(RVA_ACTIVE_PLAYER) : NULL,
        .zeroEcxEdx = site == SITE_GAME_START,
        .rsp = &data->rsp,
        .head = &data->head,
    };
    size_t codeSize = buildSiteStub(code, stub, &slots);
    data->head = site == SITE_GAME_START ? (void *)gameStartPrepOnce : target;
    if (!writeGameCode(stub, code, codeSize)) {
        writeLog("site_skipped", "site=%s reason=code_unwritable", info->name);
        return 0;
    }

    unsigned char saved[MAX_SITE_CALLS + 1][6];
    for (int i = 0; !ownCall && siteCallAt(site, i); i++) {
        unsigned char *call = siteCallAt(site, i), callCode[6];
        size_t size = slot ? 6 : 5;
        buildSiteCall(callCode, call, stub);
        memcpy(saved[i], call, size);
        if (writeGameCode(call, callCode, size)) continue;
        while (--i >= 0) writeGameCode(siteCallAt(site, i), saved[i], size);
        writeLog("site_skipped", "site=%s reason=code_unwritable address=%p", info->name, call);
        return 0;
    }
    if (site == SITE_GAME_START) removeGameStartOnceJump();
    siteInstalled[site] = 1;
    return 1;
}
#else
static int siteInstalled[SITE_COUNT];
static int setupIsAutomated;
#include "patch_arm64.h"
#endif

// ---- Multiplayer gate -----------------------------------------------------------------------
// No plugin runs in network games.
#ifndef __aarch64__
#define GATE_PAGE_SIZE  0x10000
#define GATES_PER_PAGE  (GATE_PAGE_SIZE / GATE_SIZE)
#define GATE_DATA_BYTES 0x4000
typedef struct { void *hook, *next; } GateData;
_Static_assert(GATES_PER_PAGE * sizeof(GateData) <= GATE_DATA_BYTES, "every gate's data must fit its block");
static unsigned char *gatePage;
static size_t gatePageUsed = GATE_PAGE_SIZE;
static void *volatile enteredGateAddress;
static void *enteredGate(void) { return enteredGateAddress; }
static GateData *gateData(void *gate) {
    uintptr_t at = (uintptr_t)gate, block = at & ~(uintptr_t)(GATE_PAGE_SIZE - 1);
    return (GateData *)(block + GATE_PAGE_SIZE) + (at - block) / GATE_SIZE;
}
static void setGateNext(void *gate, void *next) { gateData(gate)->next = next; }
#endif

// Returns the loaded module (EXE or DLL) that holds `address`, or NULL.
static HMODULE moduleOf(const void *address) {
    HMODULE m = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)address, &m);
    return m;
}

// Returns 1 for code inside CoE5SE itself.
static int isCoe5seCode(const void *address) {
    HMODULE m = moduleOf(address);
    return m && m == moduleOf((void *)moduleOf);
}

// The path of the plugin DLL whose DllMain is running now.
static char loadingPlugin[MAX_PATH];

static void pluginLoadingDone(const char *path);

// The loader calls this with the plugin's path before the load, and with NULL after it.
__declspec(dllexport) void coe5sePluginLoading(const char *path) {
    if (path) {
        snprintf(loadingPlugin, sizeof loadingPlugin, "%s", path);
        return;
    }
    char loaded[MAX_PATH];
    snprintf(loaded, sizeof loaded, "%s", loadingPlugin);
    if (loaded[0]) pluginLoadingDone(loaded);
    loadingPlugin[0] = '\0';
}

// Returns the plugin that called an API function.
static HMODULE callingPlugin(void *returnAddress) {
    HMODULE m = loadingPlugin[0] ? GetModuleHandleA(loadingPlugin) : NULL;
    return m ? m : moduleOf(returnAddress);
}

#ifndef __APPLE__
#include "crash_log.h"
#endif

#ifndef __aarch64__
// Writes a gate's code at `gate`, in a block that is still writable.
static void writeGateCode(unsigned char *gate) {
    const void *flags[3] = {GAME_PTR(RVA_NET_CLIENT), GAME_PTR(RVA_NET_SERVER), GAME_PTR(RVA_NET_LOBBY)};
    GateData *data = gateData(gate);
    buildGateCode(gate, gate, flags, (const void *)&enteredGateAddress, &data->hook, &data->next);
}

// Builds a gate that jumps to `next` in a network game and to `hook` otherwise.
static void *makeGate(void *hook, void *next) {
    if (gatePageUsed + GATE_SIZE > GATE_PAGE_SIZE) {
        unsigned char *page = VirtualAlloc(NULL, GATE_PAGE_SIZE + GATE_DATA_BYTES, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!page) return NULL;
        for (int i = 0; i < GATES_PER_PAGE; i++) writeGateCode(page + i * GATE_SIZE);
        makePagesReadExecute(page, GATE_PAGE_SIZE);
        makePagesReadWrite(page + GATE_PAGE_SIZE, GATE_DATA_BYTES);
        gatePage = page;
        gatePageUsed = 0;
    }
    unsigned char *gate = gatePage + gatePageUsed;
    gatePageUsed += GATE_SIZE;
    GateData *data = gateData(gate);
    data->hook = hook;
    data->next = next;
    return gate;
}
#endif

// ---- Who may wrap a site ------------------------------------------------------------------------
// Observe-only and internal sites are closed to plugin wrappers.
#define OBSERVE_ONLY_SITES(X) \
    X(SITE_RITUAL_SCREEN_LOCATION_CHECK) X(SITE_RITUAL_SCREEN_CAN_CAST) \
    X(SITE_GAME_START) X(SITE_MONTH_START) X(SITE_PLAYER_TURN_START) X(SITE_HUMAN_TURN) X(SITE_AI_TURN) \
    X(SITE_PLAYER_TURN_END) X(SITE_GAME_OVER) X(SITE_GAME_SAVE) X(SITE_GAME_LOAD)

static int isInternalSite(CallSite site) {
    return site == SITE_MODS_RUN || site == SITE_MOD_COMMAND || site == SITE_TEXT_FORMAT || site == SITE_TEXT_BUILD ||
           site == SITE_MAIN_MENU_MODS_SCREEN || site == SITE_MOD_ERROR_DIALOG || site == SITE_EVENT_LINE ||
           site == SITE_UNIT_LIST_SHEET || site == SITE_POLL_EVENT || site == SITE_KEY_TEXT_BAR ||
           site == SITE_KEY_HELP_TEXT || site == SITE_KEY_TEXT_BUTTON || site == SITE_RUN_ONE_MOD ||
           site == SITE_RUN_ONE_MOD_PATH || site == SITE_GAME_MENU_SETTINGS_SECTION_END || site == SITE_ELIMINATED_WARN ||
           site == SITE_TURN_MESSAGES ||
           site == SITE_MAP_EDITOR_TOOLBOX_LINE || site == SITE_RUN_ONE_MOD_LINE_READ || site == SITE_TOP_BAR ||
           site == SITE_TOP_BAR_INCOME_DRAW;
}

static int isObserveOnlySite(CallSite site) {
#define OBSERVE_CASE(s) case s:
    switch (site) {
    OBSERVE_ONLY_SITES(OBSERVE_CASE)
        return 1;
    default:
        return 0;
    }
#undef OBSERVE_CASE
}

// ---- Wrap chains --------------------------------------------------------------------------------
// Each site keeps its wrappers in one list, in the order they run.
typedef enum { WRAP_LAYER_INNER, WRAP_LAYER_PLUGIN, WRAP_LAYER_OUTERMOST } WrapLayer;
static const char *const WRAP_LAYER_NAMES[] = {"inner", "plugins", "outermost"};

typedef struct {
    void *hook, *entry;
    void **next;
    int priority;
    WrapLayer layer;
    unsigned order;
    char owner[64];
} SiteWrap;
static struct { SiteWrap **list; int count, capacity; } siteWraps[SITE_COUNT];
static unsigned wrapsAdded;
static SiteWrap *lastWrapAdded;

static int runsBefore(const SiteWrap *a, const SiteWrap *b) {
    if (a->layer != b->layer) return a->layer > b->layer;
    if (a->priority != b->priority) return a->priority > b->priority;
    return a->order > b->order;
}

static void **siteHeadSlot(CallSite site) {
#ifdef __aarch64__
    return siteHead(site);
#else
    return &siteStubData(site)->head;
#endif
}

// Where the game's calls of `site` go: its stub, which runs every wrapper, then the game.
static void *siteEntry(CallSite site, void *game) {
    if (!siteInstalled[site]) return game;
#ifdef __aarch64__
    return (void *)(coe5seSiteStubs + site * STUB_CODE_SIZE);
#else
    return siteStub(site);
#endif
}

#ifndef __aarch64__
static int saveGameToOpenFile(const char *path, const char *name);
#endif

// GAME_START in every game.
#define GAME_START_ONCE_JNE 0x12
#ifndef __aarch64__
// GAME_START's game end on Windows: game_start_prep while it hasn't run in this launch.
static void gameStartPrepOnce(void) {
    if (G32(RVA_GAME_START_PREP_DONE) == 0) ((void (*)(void))GAME_PTR(RVA_GAME_START_PREP))();
}
// installSite's step for GAME_START before it points the call at the stub.
static int checkGameStartOnceCode(void) {
    if (gameCodeMatches) return 1;
    writeLog("game_start_once_check", "ok=0 reason=game_code_differs address=%p", (void *)GAME_PTR(RVA_GAME_START_ONCE_TEST));
    return 0;
}
// Removes play_game's jne over the GAME_START call, once the call goes to the site's stub.
static void removeGameStartOnceJump(void) {
    unsigned char *jne = GAME_PTR(RVA_GAME_START_ONCE_TEST) + GAME_START_ONCE_JNE;
    static const unsigned char NOP2[] = {0x66, 0x90};
    int ok = writeGameCode(jne, NOP2, sizeof NOP2);
    writeLog("game_start_once_removed", "ok=%d address=%p", ok, (void *)jne);
}
#endif

// The game's own function for the site: where the innermost wrapper's next goes.
static void *siteGameTarget(CallSite site) {
#ifdef __aarch64__
    return siteGameFunction(site);
#else
    if (site == SITE_GAME_SAVE) return (void *)saveGameToOpenFile;
    if (site == SITE_GAME_START) return (void *)gameStartPrepOnce;
    return isInlinedButtonRowSite(site) ? gameBase + RVA_BUTTON_ROW : siteCallTarget(site);
#endif
}

// Points `w`'s next, and its gate's, at `target`.
static void pointWrapAt(SiteWrap *w, void *target) {
    *w->next = target;
    if (w->entry != w->hook) setGateNext(w->entry, target);
}

// Links the wrapper just put at list[at] in between its neighbours.
static void linkWrapAt(CallSite site, int at) {
    SiteWrap *w = siteWraps[site].list[at];
    pointWrapAt(w, at + 1 < siteWraps[site].count ? siteWraps[site].list[at + 1]->entry : siteGameTarget(site));
    if (at == 0) *siteHeadSlot(site) = w->entry;
    else pointWrapAt(siteWraps[site].list[at - 1], w->entry);
}

// Logs the site's wrappers in the order they run, outermost first.
static void logWrapOrder(CallSite site) {
    char chain[2048];
    size_t n = 0;
    for (int i = 0; i < siteWraps[site].count && n < sizeof chain; i++) {
        const SiteWrap *w = siteWraps[site].list[i];
        int firstOfLayer = i == 0 || siteWraps[site].list[i - 1]->layer != w->layer;
        n += (size_t)snprintf(chain + n, sizeof chain - n, "%s%s%s@%d,", firstOfLayer ? WRAP_LAYER_NAMES[w->layer] : "",
                              firstOfLayer ? ":" : "", w->owner, w->priority);
    }
    if (n < sizeof chain) snprintf(chain + n, sizeof chain - n, "game");
    else snprintf(chain + sizeof chain - 5, 5, "...");
    writeLog("wrap_order", "site=%s wrappers=%d chain=%s", SITES[site].name, siteWraps[site].count, chain);
}

static void logAllWrapOrders(void) {
    for (int s = 0; s < SITE_COUNT; s++)
        if (siteWraps[s].count) logWrapOrder((CallSite)s);
}

// Where a wrapper goes in its site's chain.
typedef struct {
    int priority;
    WrapLayer layer;
    int gated;
} WrapPlace;

// Adds `hook` to the site's chain at `place` and sets *next.
static int insertWrap(CallSite site, void *hook, void **next, WrapPlace place, const char *owner, const char **reason) {
    const char *why = NULL;
    SiteWrap *w = NULL;
    if (!siteInstalled[site] && !installSite(site)) why = "site_unavailable";
    else if (!(w = calloc(1, sizeof *w)) ||
             !growList((void **)&siteWraps[site].list, &siteWraps[site].capacity, siteWraps[site].count + 1, sizeof w))
        why = "out_of_memory";
    else if (!(w->entry = place.gated ? makeGate(hook, *siteHeadSlot(site)) : hook))
        why = "out_of_memory";
    if (why) {
        free(w);
        if (reason) *reason = why;
        return 0;
    }
    w->hook = hook;
    w->next = next;
    w->priority = place.priority;
    w->layer = place.layer;
    w->order = ++wrapsAdded;
    snprintf(w->owner, sizeof w->owner, "%.63s", owner);
    int at = 0;
    while (at < siteWraps[site].count && !runsBefore(w, siteWraps[site].list[at])) at++;
    memmove(&siteWraps[site].list[at + 1], &siteWraps[site].list[at], sizeof w * (size_t)(siteWraps[site].count - at));
    siteWraps[site].list[at] = w;
    siteWraps[site].count++;
    linkWrapAt(site, at);
    lastWrapAdded = w;
    writeLog("site_wrapped", "site=%s owner=%s layer=%s priority=%d position=%d hook=%p gated=%d", SITES[site].name,
             w->owner, WRAP_LAYER_NAMES[w->layer], place.priority, at, hook, place.gated);
    if (startupFinished) logWrapOrder(site);
    return 1;
}

// The entry the chain calls for the wrapper whose next variable is `next`: its gate, or the hook.
static void *wrapEntryFor(CallSite site, void **next) {
    if (lastWrapAdded && lastWrapAdded->next == next) return lastWrapAdded->entry;
    for (int i = 0; i < siteWraps[site].count; i++)
        if (siteWraps[site].list[i]->next == next) return siteWraps[site].list[i]->entry;
    return NULL;
}

// The DLL file name holding `address`.
static void moduleFileName(const void *address, char *out, size_t size) {
    char path[MAX_PATH] = "?";
    HMODULE m = moduleOf(address);
    if (m) GetModuleFileNameA(m, path, sizeof path);
    const char *file = strrchr(path, PATH_SEP[0]);
    snprintf(out, size, "%.63s", file ? file + 1 : path);
}

// CoE5SE's own wrappers go through wrapSite.
static int wrapSite(CallSite site, void *hook, void **next) {
    if (next) *next = NULL;
    if ((unsigned)site >= SITE_COUNT || !hook || !next) return 0;
    char owner[64];
    moduleFileName(hook, owner, sizeof owner);
    return insertWrap(site, hook, next, (WrapPlace){.priority = WRAP_PRIORITY_DEFAULT, .gated = !runsInNetworkGames(hook)}, owner, NULL);
}

// wrap and wrapWithPriority, for the plugin or script that called.
static int wrapFor(void *caller, const char *call, CallSite site, void *hook, void **next, int priority) {
    if (next) *next = NULL;
    const char *reason = NULL;
    if ((unsigned)site >= SITE_COUNT) {
        refuseApiCall(caller, call, "unknown_site", "site=%d", (int)site);
        return 0;
    }
    if (isInternalSite(site)) reason = "internal_site";
    else if (isObserveOnlySite(site)) reason = "observe_only_site";
    else if (!hook) reason = "no_callback";
    else if (!next) reason = "no_next";
    if (!reason) {
        const char *luaMod = isCoe5seCode(caller) ? runningLuaMod() : NULL;
        char owner[64];
        HMODULE m = callingPlugin(caller);
        if (luaMod) snprintf(owner, sizeof owner, "%.59s.lua", luaMod);
        else moduleFileName(m ? (const void *)m : caller, owner, sizeof owner);
        WrapPlace place = {.priority = priority, .layer = luaMod ? WRAP_LAYER_INNER : WRAP_LAYER_PLUGIN,
                           .gated = luaMod || !runsInNetworkGames(hook)};
#ifdef __APPLE__
        // A step the game copied into its caller runs only Lua wraps, split at next() (lua_split_chain.h).
        // @platform-split ai_steps_mac: Mac's copied AI steps take only script wraps (Windows calls them)
        if (!luaMod && isSplitSite(site)) reason = "lua_only_site";
        else
#endif
            insertWrap(site, hook, next, place, owner, &reason);
    }
    if (reason) {
        refuseApiCall(caller, call, reason, "site=%s priority=%d", SITES[site].name, priority);
        return 0;
    }
    return 1;
}

static int apiWrap(CallSite site, void *hook, void **next) {
    return wrapFor(__builtin_return_address(0), "wrap", site, hook, next, WRAP_PRIORITY_DEFAULT);
}

static int apiWrapWithPriority(CallSite site, void *hook, void **next, int priority) {
    return wrapFor(__builtin_return_address(0), "wrapWithPriority", site, hook, next, priority);
}

// ---- Plugin and script .c5m commands ---------------------------------------------------------
typedef void (*ModCommandHandler)(const char *args, int mod, int level);
static struct ModCommand { char name[48]; ModCommandHandler handler; int lua; } *modCommands;
static int modCommandCount, modCommandCapacity;
static void (*nextModCommand)(const char *, int, int);
static void runLuaModCommand(int callback, const char *args, int mod, int level);
static int modWaitsForCommandsFile(int mod);
static int anyEnabledModWaitsForCommandsFile(void);

// Returns 1 in a network game: any of the client, server, and lobby flags is set.
static int isNetworkGame(void) {
#ifdef __APPLE__
    return G8(RVA_NET_CLIENT) || G8(RVA_NET_SERVER) || G8(RVA_NET_LOBBY);
#else
    return G32(RVA_NET_CLIENT) > 0 || G32(RVA_NET_SERVER) != 0 || G32(RVA_NET_LOBBY) != 0;
#endif
}

#include "restart_prompt.h"

#include "mod_command_hook.h"

// ---- Observers ---------------------------------------------------------------------------------
// CoE5SE puts one wrapper on each observe-only site.
typedef u64 (*ObservedCall)(u64, u64, u64, u64, u64, u64);
typedef void (*ObserverFn)(const CallInfo *call);

// A site's observers run in one list.
typedef struct { ObserverFn before, after; int scripts; } Observer;
static struct { Observer *list; int count, capacity; } observers[SITE_COUNT];
static ObservedCall nextObserved[SITE_COUNT];

// Calls every observer's `before` (or `after`) for the site.
static void notifyObservers(CallSite site, const CallInfo *call, int after) {
    int network = isNetworkGame();
    for (int i = 0; i < observers[site].count; i++) {
        ObserverFn fn = after ? observers[site].list[i].after : observers[site].list[i].before;
        if (fn && (!network || runsInNetworkGames((void *)fn))) fn(call);
    }
}

static u64 runObserved(CallSite site, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) {
    CallInfo call = {{a1, a2, a3, a4, a5, a6}, 0};
    notifyObservers(site, &call, 0);
    u64 result = nextObserved[site](a1, a2, a3, a4, a5, a6);
    call = (CallInfo){{a1, a2, a3, a4, a5, a6}, result};
    notifyObservers(site, &call, 1);
    return result;
}

// Defines observe_<site>, the wrapper for each observe-only site.
#define OBSERVE_WRAPPER(s) \
    static u64 observe_##s(u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6) { return runObserved(s, a1, a2, a3, a4, a5, a6); }
OBSERVE_ONLY_SITES(OBSERVE_WRAPPER)
#undef OBSERVE_WRAPPER

static void *observeWrapperFor(CallSite site) {
#define OBSERVE_CASE(s) case s: return (void *)observe_##s;
    switch (site) {
    OBSERVE_ONLY_SITES(OBSERVE_CASE)
    default:
        return NULL;
    }
#undef OBSERVE_CASE
}

static int apiObserve(CallSite site, ObserverFn before, ObserverFn after) {
    if ((unsigned)site >= SITE_COUNT) return REFUSE_API(0, "observe", "unknown_site", "site=%d", (int)site);
    if (!isObserveOnlySite(site)) return REFUSE_API(0, "observe", "not_observe_only", "site=%s", SITES[site].name);
    if (!before && !after) return REFUSE_API(0, "observe", "no_callback", "site=%s", SITES[site].name);
    if (!nextObserved[site] && !wrapSite(site, observeWrapperFor(site), (void **)&nextObserved[site]))
        return REFUSE_API(0, "observe", "site_unavailable", "site=%s", SITES[site].name);
    if (!growList((void **)&observers[site].list, &observers[site].capacity, observers[site].count + 1, sizeof(Observer)))
        return REFUSE_API(0, "observe", "out_of_memory", "site=%s", SITES[site].name);
    int scripts = isScriptObserver((const void *)before) || isScriptObserver((const void *)after);
    int at = observers[site].count;
    while (!scripts && at > 0 && observers[site].list[at - 1].scripts) at--;
    memmove(&observers[site].list[at + 1], &observers[site].list[at], sizeof(Observer) * (size_t)(observers[site].count - at));
    observers[site].list[at] = (Observer){before, after, scripts};
    observers[site].count++;
    writeLog("site_observed", "site=%s position=%d scripts=%d", SITES[site].name, at, scripts);
    return 1;
}

#include "game_menu.h"

// ---- Mod names ----------------------------------------------------------------------------------
// A mod's name is its .c5m's file name without ".c5m", as the game names mods.

// Writes the name of the mod in folder `dir` (its first *.c5m) to `out`.
static int c5mNameInFolder(const char *dir, char *out, size_t size) {
    char pattern[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*.c5m", dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    FindClose(h);
    size_t len = strlen(fd.cFileName);
    snprintf(out, size, "%.*s", (int)(len > 4 ? len - 4 : len), fd.cFileName);
    return 1;
}

// Writes the name a mod's rows show: its name with spaces put back.
static void modDisplayName(const char *mod, char *out, size_t size) {
    size_t o = 0;
    for (const char *c = mod; *c && o + 2 < size; c++) {
        int upper = *c >= 'A' && *c <= 'Z', digit = *c >= '0' && *c <= '9';
        int prevLower = c != mod && c[-1] >= 'a' && c[-1] <= 'z', prevUpper = c != mod && c[-1] >= 'A' && c[-1] <= 'Z';
        int nextLower = c[1] >= 'a' && c[1] <= 'z', nextDigit = c[1] >= '0' && c[1] <= '9';
        int capitalAfterLower = upper && prevLower && !nextDigit;
        int acronymEnd = upper && prevUpper && nextLower;
        int digitAfterLower = digit && prevLower;
        int wordStart = capitalAfterLower || acronymEnd || digitAfterLower;
        if (*c == '_') { out[o++] = ' '; continue; }
        if (wordStart) out[o++] = ' ';
        out[o++] = *c;
    }
    out[o] = '\0';
}

#include "plugin_ui.h"

// ---- Messages --------------------------------------------------------------------------------
// The game's add_message.
typedef unsigned long long (*AddMessageFn)(int, int, int, int, int, int, const char *, const char *, ...);

static int apiSendMessage(int player, const char *title, const char *text) {
    if (!title) return REFUSE_API(MESSAGE_NOT_ADDED, "sendMessage", "no_title", "player=%d", player);
    if (!text) return REFUSE_API(MESSAGE_NOT_ADDED, "sendMessage", "no_text", "player=%d title=%s", player, title);
    if (!(player == MESSAGE_EVERY_HUMAN || (player >= 0 && player < PLAYER_SLOTS)))
        return REFUSE_API(MESSAGE_NOT_ADDED, "sendMessage", "bad_player", "player=%d title=%s", player, title);
    if (isNetworkGame()) return REFUSE_API(MESSAGE_NOT_ADDED, "sendMessage", "network_game", "player=%d title=%s", player, title);
    char shortTitle[100];
    snprintf(shortTitle, sizeof shortTitle, "%s", title);
    char cut[ADD_MESSAGE_TEXT_SIZE];
    text = fitGameText(__builtin_return_address(0), "sendMessage", text, cut, sizeof cut);
    int index = (int)((AddMessageFn)(gameBase + RVA_ADD_MESSAGE))(-1, player, -1, -1, 0, 0, shortTitle, "%s", text);
    writeLog("message_sent", "player=%d title=%s index=%d", player, shortTitle, index);
    return index;
}

// Writes a plugin's own line to coe5mod.log.
static void apiLogLine(const void *self, const char *text) {
    char path[MAX_PATH] = "?";
    HMODULE m = moduleOf(self);
    if (m) GetModuleFileNameA(m, path, sizeof path);
    const char *dll = strrchr(path, PATH_SEP[0]);
    writeLog("plugin_log", "dll=%s text=%s", dll ? dll + 1 : path, text ? text : "");
}

static uintptr_t apiCallerRsp(CallSite site) {
    if ((unsigned)site >= SITE_COUNT) return REFUSE_API(0, "callerRsp", "unknown_site", "site=%d", (int)site);
    if (!siteInstalled[site]) return REFUSE_API(0, "callerRsp", "site_not_wrapped", "site=%s", SITES[site].name);
    return siteCallerSp(site);
}

static int apiSetupAutomated(void) { return setupIsAutomated; }
// Logs who changed the shared flag.
static void apiSetSetupAutomated(int on) {
    if (on == setupIsAutomated) return;
    setupIsAutomated = on;
    char by[64];
    nameApiCaller(__builtin_return_address(0), by, sizeof by);
    writeLog("setup_automated", "on=%d by=%s", on, by);
}

// API functions defined further down.
static int apiSettingsPluginCount(void);
static const char *apiSettingsPluginName(int plugin);
static void apiSettingsPluginDraw(int plugin);
static int apiSettingsPluginClick(int id);
static void *apiFrameVariable(CallSite site, const char *name);
static int apiAddScreen(const char *label, const char *tip, void (*open)(void));
static const int *apiSetting(const char *name);

#include "key_rebind.h"

static const PluginApi API = {
    .version = PLUGIN_API_VERSION, .wrap = apiWrap, .callerRsp = apiCallerRsp, .setupAutomated = apiSetupAutomated,
    .setSetupAutomated = apiSetSetupAutomated, .addModCommand = apiAddModCommand, .addGameMenuLine = apiAddGameMenuLine,
    .sendMessage = apiSendMessage, .ui = &UI, .addScreen = apiAddScreen, .addMainMenuButton = apiAddMainMenuButton,
    .observe = apiObserve, .settingsPluginCount = apiSettingsPluginCount, .settingsPluginName = apiSettingsPluginName,
    .settingsPluginDraw = apiSettingsPluginDraw, .settingsPluginClick = apiSettingsPluginClick,
    .hideMainMenuButton = apiHideMainMenuButton, .addMainMenuButtonAfter = apiAddMainMenuButtonAfter,
    .logLine = apiLogLine, .wrapWithPriority = apiWrapWithPriority, .restartGame = apiRestartGame,
    .restartPhase = apiRestartPhase, .frameVariable = apiFrameVariable, .setting = apiSetting,
    .rebindKey = apiRebindKey, .addActionKey = apiAddActionKey, .actionKey = apiActionKey,
};

// ---- Game build check -------------------------------------------------------------------------
// Every address in CoE5SE and in the plugins belongs to one build of the game.
typedef struct {
    const char *gameVersion;
    unsigned int timeDateStamp, imageSize;
    unsigned char uuid[16];
    const char *codeSha256;
} GameBuild;
static const GameBuild KNOWN_BUILDS[] = {
#ifdef __APPLE__
    {"5.42", 0, 0, {0x2e, 0xc7, 0xe0, 0x27, 0xac, 0xe0, 0x36, 0x5a, 0x86, 0x02, 0x4e, 0x17, 0xa6, 0x2f, 0x85, 0x60},
     "7a1cf5a99f08b6f1f42cbaad5a875c037d1acf2620940bcf554880208c4ce127"},
#else
    {"5.42", 0x6ada0282, 0x13728000, {0}, "d39ea78899effadc76a0214b2eb5c8bcbd4aac5991f7862aaccb48a99ff4bfa8"},
#endif
};
#define KNOWN_BUILD_COUNT (int)(sizeof KNOWN_BUILDS / sizeof KNOWN_BUILDS[0])
static const GameBuild *gameBuild;
static char gameBuildId[40];
static int pluginsDeclined;
static int notStartedByLoader;

#include "plugin_start.h"
#include "game_code_hash.h"

// Hashes the game's code section and sets gameCodeMatches when gameBuild's hash is that.
static void checkGameCode(void) {
    const unsigned char *code;
    size_t size;
    char hex[GAME_CODE_SHA256_HEX_LEN + 1] = "";
    if (!findGameCode(gameBase, &code, &size)) {
        writeLog("game_code_check", "match=0 reason=no_code_section");
        return;
    }
    if (!hashMemorySha256(code, size, hex)) {
        writeLog("game_code_check", "match=0 reason=hash_failed bytes=%zu", size);
        return;
    }
    gameCodeMatches = gameBuild && !strcmp(hex, gameBuild->codeSha256);
    writeLog("game_code_check", "match=%d bytes=%zu sha256=%s", gameCodeMatches, size, hex);
}

// Reads the running game's build id and sets gameBuild if it is one of KNOWN_BUILDS.
static void identifyGameBuild(void) {
#ifdef __APPLE__
    const struct mach_header_64 *header = (const struct mach_header_64 *)gameBase;
    const struct load_command *lc = (const struct load_command *)(header + 1);
    const unsigned char *uuid = NULL;
    for (uint32_t i = 0; i < header->ncmds && !uuid; i++, lc = (const void *)((const char *)lc + lc->cmdsize))
        if (lc->cmd == LC_UUID) uuid = ((const struct uuid_command *)lc)->uuid;
    if (!uuid) { snprintf(gameBuildId, sizeof gameBuildId, "no_uuid"); return; }
    for (int i = 0; i < 16; i++) snprintf(gameBuildId + 2 * i, 3, "%02x", uuid[i]);
    for (int i = 0; i < KNOWN_BUILD_COUNT; i++)
        if (!memcmp(KNOWN_BUILDS[i].uuid, uuid, 16)) gameBuild = &KNOWN_BUILDS[i];
#else
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(gameBase + ((const IMAGE_DOS_HEADER *)gameBase)->e_lfanew);
    unsigned int stamp = nt->FileHeader.TimeDateStamp, size = nt->OptionalHeader.SizeOfImage;
    snprintf(gameBuildId, sizeof gameBuildId, "%08x-%08x", stamp, size);
    for (int i = 0; i < KNOWN_BUILD_COUNT; i++)
        if (KNOWN_BUILDS[i].timeDateStamp == stamp && KNOWN_BUILDS[i].imageSize == size) gameBuild = &KNOWN_BUILDS[i];
#endif
    checkGameCode();
}

// Asks whether to load mods on a game build this CoE5SE wasn't made for.
static int askLoadOnUnknownGame(void) {
    char text[256];
    snprintf(text, sizeof text, "This CoE5SE was made for CoE5 %s, not this version. Load mods anyway?",
             KNOWN_BUILDS[KNOWN_BUILD_COUNT - 1].gameVersion);
#ifdef __APPLE__
    CFStringRef message = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFOptionFlags response = kCFUserNotificationDefaultResponse;
    CFUserNotificationDisplayAlert(0, kCFUserNotificationCautionAlertLevel, NULL, NULL, NULL,
                                   CFSTR("CoE5SE"), message, CFSTR("No"), CFSTR("Yes"), NULL, &response);
    if (message) CFRelease(message);
    return response == kCFUserNotificationAlternateResponse;
#else
    return MessageBoxA(NULL, text, "CoE5SE", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND) == IDYES;
#endif
}

static void startCoe5se(const char *via);

// The loader calls this right after loading CoE5SE's DLL, after coe5seStart.
__declspec(dllexport) int coe5seLoadsPlugins(void) {
    startCoe5se("older_loader");
    return !pluginsDeclined && !notStartedByLoader;
}

// The API for plugins.
__declspec(dllexport) const PluginApi *jazzyGetApi(int version) {
    if (notStartedByLoader) return NULL;
    const char *held = pluginRefusalFor(__builtin_return_address(0));
    if (held) return REFUSE_API(NULL, "jazzyGetApi", held, "version=%d", version);
    if (!pluginsDeclined && version == PLUGIN_API_VERSION) return &API;
    return REFUSE_API(NULL, "jazzyGetApi", pluginsDeclined ? "declined_on_unknown_game" : apiVersionReason(version),
                      "version=%d supported=%d", version, PLUGIN_API_VERSION);
}

// ---- Main menu button ----------------------------------------------------------------------

// Wraps the main menu's frame.
static void installMenuHook(void) {
    if (!wrapSite(SITE_MAIN_MENU_WAIT, (void *)menuWaitHook, (void **)&nextMainMenuWait)) return;
#ifdef __aarch64__
    menuBgAlpha = (float *)&coe5seStubData[SITE_MAIN_MENU_WAIT].d8;
    menuFadeTimer = (float *)&coe5seStubData[SITE_MAIN_MENU_WAIT].d9;
#else
    menuBgAlpha = &siteStubData(SITE_MAIN_MENU_WAIT)->xmm12;
    menuFadeTimer = &siteStubData(SITE_MAIN_MENU_WAIT)->xmm13;
#endif
    writeLog("menu_hook_installed", "base=%p", gameBase);
}

// ---- CoE5SE's own page: the DLL report, approvals, auto-approve -----------------------------
// The page uses only the game's own widgets, with Preferences as the model.

#define ID_EXIT            9
#define ID_APPROVE_FIRST   100

// A plugin as the CoE5SE page lists it: its plugins_found.txt line, and its mod's .c5m info.
typedef struct {
    FoundLine found;
    char name[64], description[400];
    int approvedNow;
} PluginEntry;

// Reads the mod's name and description from the .c5m in the plugin's folder.
static int readC5mInfo(const char *pluginPath, char *name, size_t nameSize, char *desc, size_t descSize) {
    char dir[MAX_PATH], mod[MOD_NAME_MAX], file[MAX_PATH];
    if (!buildPath(dir, sizeof dir, __func__, "%s", pluginPath)) return 0;
    char *slash = strrchr(dir, PATH_SEP[0]);
    if (!slash) return 0;
    *slash = '\0';
    if (!c5mNameInFolder(dir, mod, sizeof mod)) return 0;
    modDisplayName(mod, name, nameSize);

    if (!buildPath(file, sizeof file, __func__, "%s" PATH_SEP "%s.c5m", dir, mod)) return 1;
    FILE *f = fopen(file, "rb");
    if (!f) return 1;
    static char text[65536];
    size_t len = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[len] = '\0';
    for (char *line = text; line && *line; ) {
        char *next = strchr(line, '\n');
        while (*line == ' ' || *line == '\t') line++;
        if (!strncmp(line, "description", 11) && (line[11] == ' ' || line[11] == '\t')) {
            char *open = strchr(line, '"');
            char *close = open ? strchr(open + 1, '"') : NULL;
            if (open && close) {
                size_t d = 0;
                for (char *c = open + 1; c < close && d + 1 < descSize; c++) {
                    char ch = (*c == '\r' || *c == '\n' || *c == '\t') ? ' ' : *c;
                    if (ch == ' ' && d > 0 && desc[d - 1] == ' ') continue;
                    desc[d++] = ch;
                }
                desc[d] = '\0';
            }
            break;
        }
        line = next ? next + 1 : NULL;
    }
    return 1;
}

// ---- Plugins' screen lines, shown by Mod Settings -----------------------------------------------
// Every plugin's screen lines are on its mod's page in Mod Settings.

// Returns how many mods' plugins have entries, and sets *first to the i-th's first entry.
static int settingsPluginGroup(int i, int *first) {
    int count = 0;
    for (int e = 0; e < entryCount; e++) {
        int seen = 0;
        for (int j = 0; j < e && !seen; j++) seen = !_stricmp(entries[j]->mod, entries[e]->mod);
        if (seen) continue;
        if (count == i && first) *first = e;
        count++;
    }
    return count;
}

static int apiSettingsPluginCount(void) { return settingsPluginGroup(-1, NULL); }

// The name of the i-th mod whose plugins have entries.
static const char *apiSettingsPluginName(int plugin) {
    int first = 0;
    if (plugin < 0 || plugin >= settingsPluginGroup(plugin, &first))
        return REFUSE_API("", "settingsPluginName", "bad_index", "plugin=%d", plugin);
    return entries[first]->mod;
}

// The page drawn last.
static char drawnPluginMod[MOD_NAME_MAX];

// The j-th entry of the drawn page, or NULL.
static SettingEntry *drawnPluginEntry(int j) {
    for (int i = 0; i < entryCount; i++)
        if (!_stricmp(entries[i]->mod, drawnPluginMod) && j-- == 0) return entries[i];
    return NULL;
}

// Adds a line for each of the mod's left-out plugin calls to the "Settings left out" box.
static void drawPluginRejections(const char *mod, int *open) {
    char tip[400];
    for (int i = 0; i < entryCount; i++) {
        const SettingEntry *e = entries[i];
        if (e->type != ENTRY_LEFT_OUT || _stricmp(e->mod, mod)) continue;
        if (!*open) {
            sectionBegin();
            sectionHeader("Settings left out");
            *open = 1;
        }
        textButton(0, e->problem);
        snprintf(tip, sizeof tip, "In %.200s. Its log line is api_refused.", e->section);
        tooltip(tip);
    }
}

// Draws mod `plugin`'s screen lines in one box titled "Screens", after the mod's settings.
static void apiSettingsPluginDraw(int plugin) {
    int first = 0;
    if (plugin < 0 || plugin >= settingsPluginGroup(plugin, &first)) {
        refuseApiCall(__builtin_return_address(0), "settingsPluginDraw", "bad_index", "plugin=%d", plugin);
        return;
    }
    snprintf(drawnPluginMod, sizeof drawnPluginMod, "%s", entries[first]->mod);
    int open = 0, j = -1;
    for (int i = 0; i < entryCount; i++) {
        SettingEntry *e = entries[i];
        if (_stricmp(e->mod, drawnPluginMod)) continue;
        j++;
        if (e->type != ENTRY_SCREEN) continue;
        if (!open) {
            sectionBegin();
            sectionHeader("Screens");
            open = 1;
        }
        textButton(ID_SCREEN_FIRST + j, e->label);
        if (e->tip[0]) tooltip(e->tip);
    }
    if (open) sectionEnd();
}

static int apiSettingsPluginClick(int id) {
    SettingEntry *e;
    if (id >= ID_SCREEN_FIRST && id < ID_SCREEN_END && (e = drawnPluginEntry(id - ID_SCREEN_FIRST)) && e->type == ENTRY_SCREEN) {
        writeLog("plugin_screen_opened", "plugin=%s label=%s", e->section, e->label);
        if (e->lua >= 0) runLuaCallback(e->lua, 0, 0);
        else e->open();
        return 1;
    }
    return 0;
}

#include "mod_settings.h"
#include "mod_settings_declare.h"
#include "mod_settings_ini_sidecar.h"
#include "mod_settings_reading.h"
#include "mod_settings_screen.h"
#include "mod_settings_events.h"
#include "event_logic.h"

// Reads plugins_found.txt, one plugin per line.
static int readPlugins(PluginEntry **listOut, int *capacity) {
    static char line[FOUND_LINE_MAX];
    char file[MAX_PATH];
    FILE *f = coe5seUserFile(PLUGINS_FOUND_FILE, file, sizeof file) ? fopen(file, "r") : NULL;
    if (!f) return 0;
    int n = 0;
    while (fgets(line, sizeof line, f)) {
        if (!growList((void **)listOut, capacity, n + 1, sizeof **listOut)) {
            writeLog("coe5se_page_plugins_cut", "reason=out_of_memory listed=%d", n);
            break;
        }
        PluginEntry *list = *listOut, *p = &list[n];
        memset(p, 0, sizeof *p);
        if (!readFoundLine(line, &p->found)) continue;
        readC5mInfo(p->found.path, p->name, sizeof p->name, p->description, sizeof p->description);
        if (!p->name[0])
            snprintf(p->name, sizeof p->name, "%.*s", (int)(strlen(p->found.name) - strlen(PLUGIN_SUFFIX)), p->found.name);
        p->approvedNow = p->found.state == PLUGIN_UNAPPROVED && p->found.hash[0] && isHashApproved(p->found.hash);
        n++;
    }
    fclose(f);
    return n;
}

static const char *sourceText(const char *source) {
    if (!strcmp(source, "workshop")) return "Steam Workshop";
    if (!strcmp(source, "local")) return "Local Mods";
    return source;
}

// The note after a plugin's name on the CoE5SE page: why it didn't load, or nothing.
static const char *pluginRowNote(const PluginEntry *p) {
    switch (p->found.state) {
    case PLUGIN_FAILED: return " (failed to load)";
    case PLUGIN_CONFLICT: return " (not loaded: name clash)";
    case PLUGIN_REFUSED: return " (not loaded: mod DLLs off)";
    case PLUGIN_UNAPPROVED: return p->approvedNow ? " (approved, loads next launch)" : " (not approved)";
    default: return "";
    }
}

// The same for the plugin's tooltip, before its details.
static const char *pluginTipNote(const PluginEntry *p) {
    switch (p->found.state) {
    case PLUGIN_FAILED: return "Failed to load. Check coe5mod.log. ";
    case PLUGIN_CONFLICT:
        return "Not loaded: mod or DLL name shared with another mod folder. A copy in Local Mods wins; otherwise "
               "neither loads. ";
    case PLUGIN_REFUSED: return "Not loaded: no mod DLLs load in this version of CoE5SE. Mods use Lua scripts instead. ";
    case PLUGIN_UNAPPROVED: return "Not loaded: new or changed since last approval. ";
    default: return "";
    }
}

// CoE5SE's own page: the report of mods with DLLs, their Approve lines, and auto-approve.
static void coe5sePageScreen(void) {
    static PluginEntry *plugins;
    static int pluginCapacity;
    static ModSetting **page;
    static int pageCapacity;
    int n = readPlugins(&plugins, &pluginCapacity);
    readLoadOrder(0);
    int missing = listModsMissingPlugins(0);
    char tip[2048];
    float width = setupScreenWidth();
    screenMusic();
    writeLog("coe5se_page_opened", "plugins=%d", n);
    for (;;) {
        screenBegin();
        screenTitle(COE5SE_PAGE_TITLE);
        screenWidth(width);

        sectionBegin();
        sectionHeader("Mods with DLLs");
        if (n == 0 && missing == 0) textButton(0, "No mods with DLLs found");
        for (int i = 0; i < n; i++) {
            PluginEntry *p = &plugins[i];
            const FoundLine *f = &p->found;
            const char *version = pluginModVersion(f->path);
            char row[224], heldRow[64] = "", heldTip[400] = "";
            int held = pluginRefusalText(f->name, heldRow, sizeof heldRow, heldTip, sizeof heldTip);
            snprintf(row, sizeof row, "%s%s%s%s", p->name, version[0] ? " " : "", version, held ? heldRow : pluginRowNote(p));
            textButton(0, row);
            snprintf(tip, sizeof tip, "%s\n\n%s%sPlugin: %s. Version: %s. Source: %s.\nFolder: %.200s%s%s",
                     p->description[0] ? p->description : p->name, heldTip, held ? "" : pluginTipNote(p), f->name,
                     version[0] ? version : "none", sourceText(f->source), f->path, f->hash[0] ? "\nSHA-256: " : "", f->hash);
            tooltip(tip);
            if (f->state == PLUGIN_UNAPPROVED && !p->approvedNow) {
                textButton(ID_APPROVE_FIRST + i, "Approve");
                tooltip("Loads this DLL from the next launch on. If an update changes it, it needs approving again.");
            }
        }
        drawModsMissingPlugins(missing);
        sectionEnd();
        drawModRequirements();
        drawSpellProblems();
        drawRitualsLeftOut();
        drawTablesLeftOut();
        drawCustomPlaneProblems();

        finishReading();
        int settingCount = settingsOfModPage(COE5SE_MOD, &page, &pageCapacity);
        drawIniSettings(page, settingCount);

        buttonRow();
        textButton(ID_EXIT, "Back");
        bindHotkey(ID_EXIT, UI_KEY_ENTER);
        uiBindKey(ID_EXIT, UI_KEY_ESCAPE);
        int id = screenWait();
        if (id == ID_EXIT) return;
        clickIniSetting(id, page, settingCount);
        if (id >= ID_APPROVE_FIRST && id < ID_APPROVE_FIRST + n) {
            PluginEntry *p = &plugins[id - ID_APPROVE_FIRST];
            if (p->found.hash[0] && approveHash(p->found.hash, p->found.name)) p->approvedNow = 1;
            writeLog("plugin_approved", "name=%s sha256=%s ok=%d", p->found.name, p->found.hash, p->approvedNow);
        }
    }
}

#ifndef __APPLE__
#include "text_translation.h"

#endif

// Logs the game's Mod Error box before it shows, so a tool waiting on the log can stop at once.
typedef u64 (*MessageBoxFn)(u64, u64, u64, u64, u64, u64, u64, u64);
static MessageBoxFn nextModErrorDialog;
static u64 modErrorDialogWrapper(u64 title, u64 text, u64 a3, u64 a4, u64 a5, u64 a6, u64 a7, u64 a8) {
    char line[512];
    snprintf(line, sizeof line, "%.500s", text ? (const char *)text : "");
    for (char *c = line; *c; c++) if (*c == '\n' || *c == '\r') *c = ' ';
    writeLog("game_mod_error", "text=%s", line);
    return nextModErrorDialog(title, text, a3, a4, a5, a6, a7, a8);
}

#include "load_order.h"
#include "platform_warning.h"
#include "stub_update.h"
#include "ritual_expansion.h"
#include "spell_path_expansion.h"
#include "sidecar.h"
#define VANILLA_SPELL_COUNT 1029
#include "spell_table_move.h"
#include "plane_table_move.h"
#include "player_table_move.h"
#include "factions.h"
#include "mac_site_stubs.h"
#include "table_move.h"
#include "spell_table.h"
#include "spell_paths.h"
#include "offset_names.h"
#include "name_lists.h"
#include "monster_flags.h"
#include "ritual_words.h"
#include "game_save.h"
#include "save_tail.h"
#include "save_names.h"
#include "missing_mods.h"
#include "plane_layout.h"
#include "custom_planes.h"
#include "faction_sight.h"
#include "plane_shapes.h"
#include "plane_ground.h"
#include "custom_plane_view.h"
#include "plane_tint.h"
#include "mod_versions.h"
#include "spells_changed.h"
#include "code_check.h"
#include "corpse_kinds.h"
#include "weapon_traits.h"
#include "no_explode.h"
#include "trace_filter.h"
#include "trace_log_file.h"
#include "seal_events.h"
#include "data_area_max.h"
#include "map_editor.h"
#include "top_bar_income.h"
#include "commander_lines.h"
#include "lua_scripts.h"
#include "lua_game_data.h"
#include "lua_get_set.h"
#include "strength_index.h"
#include "lua_sites.h"
#include "lua_split_chain.h"
#include "lua_coe5_table.h"
#include "portal_crossing.h"
#include "lua_ai.h"
#include "lua_play_as.h"
#include "lua_seat_sight.h"
#include "battle_report_seat.h"
#include "shop_offers.h"
#include "rec_bonus.h"
#include "lua_units.h"
#include "lua_data_files.h"
#include "lua_ui.h"
#include "lua_loading.h"
#include "plane_words.h"
#include "class_names.h"
#include "ai_class.h"
#include "plane_pieces.h"
#include "terrain_looks.h"
#include "battle_maps.h"
#include "event_finders.h"
#include "battle_snapshot.h"
#include "fanpatch/fanpatch_host.h"
#include "faction_words.h"
#include "faction_movers.h"
#include "faction_saves.h"
#include "faction_names.h"

// Ends the launch's header in the log at the end of startup (finishStartup).
static void logHeaderEnd(void) {
    static char order[16384];
    size_t n = 0;
    order[0] = '\0';
    for (int k = 0; k < orderedModCount; k++)
        appendText(order, sizeof order, &n, "%s%s", k ? "," : "", orderedMods[loadOrder[k]].name);
    writeLog("log_header_mods", "count=%d order=%s", orderedModCount, order);
    writeLog(LOG_HEADER_END_EVENT, "game_build=%s game_version=%s api=%d", gameBuildId, gameBuild ? gameBuild->gameVersion : "unknown",
             PLUGIN_API_VERSION);
}

// Only what must run as the DLL loads: the log, and the check that the loader loaded CoE5SE.
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(dll);
    commonInit("coe5se");
#ifndef __APPLE__
    if (!GetModuleHandleA(LOADER_DLL_NAME)) {
        notStartedByLoader = 1;
        writeLog("coe5se_inactive", "reason=not_started_by_loader loader=%s", LOADER_DLL_NAME);
    }
#endif
    return TRUE;
}

// The loader calls this right after loading CoE5SE's DLL, before anything else: CoE5SE's startup.
__declspec(dllexport) void coe5seStart(void) {
    startCoe5se("coe5se_start");
}

// CoE5SE's startup, once: the game build check, then every patch, site and file read, in order.
static void startCoe5se(const char *via) {
    static int started;
    if (started || notStartedByLoader) return;
    started = 1;
    writeLog("coe5se_start", "via=%s", via);
#ifndef __APPLE__
    installCrashFilter();
#endif
    identifyGameBuild();
    writeLog("coe5se_version", "api=%d game_build=%s game_version=%s", PLUGIN_API_VERSION,
             gameBuildId, gameBuild ? gameBuild->gameVersion : "unknown");
    if (!gameBuild) {
        pluginsDeclined = !askLoadOnUnknownGame();
        writeLog("unknown_game_answer", "load=%d", !pluginsDeclined);
        if (pluginsDeclined) {
            writeLog("mod_command_hook_installed", "ok=%d declined=1",
                     wrapSite(SITE_MOD_COMMAND, (void *)declinedModCommandHook, (void **)&nextModCommand));
            writeLog("event_line_hook_installed", "ok=%d declined=1",
                     wrapSite(SITE_EVENT_LINE, (void *)declinedEventLineHook, (void **)&nextEventLine));
            return;
        }
    }
    checkNameBoxCode();
    eventFindersInit();
    battleSnapshotInit();
    strengthIndexInit();
#ifndef __APPLE__
    traceLogFileInit();
    installStackCookieHook();
#endif
    installMenuHook();
    writeLog("mod_error_hook_installed", "ok=%d",
              wrapSite(SITE_MOD_ERROR_DIALOG, (void *)modErrorDialogWrapper, (void **)&nextModErrorDialog));
#ifdef __APPLE__
    int textFormat = 0, textBuild = 0;
#else
    int textFormat = wrapSite(SITE_TEXT_FORMAT, (void *)textFormatHook, (void **)&nextTextFormat);
    int textBuild = wrapSite(SITE_TEXT_BUILD, (void *)textBuildHook, (void **)&nextTextBuild);
#endif
    int modCommand = wrapSite(SITE_MOD_COMMAND, (void *)modCommandHook, (void **)&nextModCommand);
    int modsScreen = wrapSite(SITE_MAIN_MENU_MODS_SCREEN, (void *)modsScreenWrapper, (void **)&nextModsScreen);
    writeLog("restart_prompt_installed", "ok=%d", modsScreen);
    writeLog("mod_command_hook_installed", "ok=%d", modCommand);
    writeLog("translation_reach_installed", "text_format=%d text_build=%d", textFormat, textBuild);
    installGameMenuHooks();
    installFileBrowserBounds();
    installWaitIdBounds();
    installInlinedButtonRows();
    installMapEditorHooks();
    installCommanderLineHooks();
#ifndef __aarch64__
    gameSaveInit();
#endif
    keyRebindInit();
    needsCoe5seInit();
    ritualExpansionInit();
    spellPathExpansionInit();
    spellTableMoveInit();
    tableMovesInit();
    fanpatchInstall();
    saveTailInit();
    missingModsInit();
    installSaveCountHook();
    modVersionsInit();
    spellWordsInit();
    spellPathWordsInit();
    planeWordsInit();
    factionWordsInit();
    terrainLooksWordsInit();
    battleMapWordsInit();
    monsterFlagWordsInit();
    ritualWordsInit();
    corpseKindsInit();
    weaponTraitsInit();
    noExplodeInit();
    nameListsInit();
    aiClassInit();
    maxdataWordInit();
    sidecarInit();
    pluginStartInit();
    modSettingsInit();
    loadOrderInit();
#ifdef COE5SE_EXTRA_START
    COE5SE_EXTRA_START();
#endif
}
