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
static int spellLineHook(const char *line, size_t len, int mod, int level);
static const char *spellPathLineHook(const char *line, size_t len, int mod, int level);
static const char *offsetNameLineHook(const char *line, size_t len, int mod, int level);
static int declinedOffsetNameLine(const char *line, size_t len, int level);
static const char *nameListLineHook(const char *line, size_t len, int mod, int level);
static int declinedNameListLine(const char *line, size_t len, int level);
static int settingLineHook(const char *line, size_t len, int mod, int level);
static int eventLogicLine(const char *line, int level);
static void eventLogicAfterLine(const char *line, int level);
static int isEventLogicWord(const char *line);
static int declinedSpellPathLine(const char *word, int mod, int level);
static int declinedSpellLine(const char *word, int mod, int level);
static int flagOffLineHook(const char *line, size_t len, int mod, int level);
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

// The end of CoE5SE's startup, once a launch, when the game has found and read its mods: the load
// order, the settings files no mod pass read, and the scripts. The first main menu frame runs it, or
// the map editor when the game starts with --mapeditor and opens the editor before any menu frame.
static void finishStartup(void) {
    if (startupFinished) return;
    takeCodeModSnapshot();
    readLoadOrder(1);
    readUnreadSettingsFiles();
    loadLuaScripts();
    logAllWrapOrders();
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
    if (quiet) showLuaNotices();
    return clicked;
}

// ---- Plugin API: call sites -----------------------------------------------------------------
// A site is one or more call instructions in the game with the same target.

// The catalog of sites, with both builds' addresses, is in game_addresses.h.
#define GAME_ADDRESSES_SITE_TABLE
#include "game_addresses.h"

// The sites whose calls are an inlined button_row's layout_new.
static int isInlinedButtonRowSite(CallSite site) {
    return site == SITE_PARTICIPANTS_BUTTON_ROW || site == SITE_SQUARE_INFO_BUTTON_ROW;
}

#ifndef __aarch64__
// Each site's stub is 0x40 bytes, and all of them share two pages near the game's code.
#define STUB_SIZE  0x40
#define STUB_HEAD  0x28
#define STUB_RSP   0x30
#define STUB_XMM12 0x38
#define STUB_XMM13 0x3c
#define STUB_PAGES_BYTES 0x2000
_Static_assert(SITE_COUNT * STUB_SIZE <= STUB_PAGES_BYTES, "every site's stub must fit the pages allocNearBytes gives");
static unsigned char *stubPage;
static int siteInstalled[SITE_COUNT];
static int setupIsAutomated;

static unsigned char *siteStub(CallSite site) { return stubPage + site * STUB_SIZE; }

// Writes `opcode` and a 32-bit displacement to `slot`, and moves *p past it.
static void emitRipStore(unsigned char **p, const unsigned char *opcode, int opcodeLen, unsigned char *slot) {
    memcpy(*p, opcode, opcodeLen);
    int disp = (int)(slot - (*p + opcodeLen + 4));
    memcpy(*p + opcodeLen, &disp, 4);
    *p += opcodeLen + 4;
}

// Makes game data in .rdata writable.
static int makeGameDataWritable(void *address, size_t size) {
    DWORD old;
    return VirtualProtect(address, size, PAGE_READWRITE, &old) != 0;
}

static int prepareSiteCode(CallSite site);
static unsigned char *generatedSiteCall(CallSite site), *generatedSiteTarget(CallSite site);

// Call i of a site (NULL past its last), and the function its calls go to.
static int siteThroughSlot[SITE_COUNT];
static unsigned char *siteCallAt(CallSite site, int i) {
    unsigned char *generated = generatedSiteCall(site);
    if (generated) return i == 0 ? generated : NULL;
    return i < MAX_SITE_CALLS && SITES[site].rvas[i] ? gameBase + SITES[site].rvas[i] : NULL;
}
static unsigned char *siteCallTarget(CallSite site) {
    unsigned char *generated = generatedSiteTarget(site);
    if (generated) return generated;
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

// Builds the site's stub and points every call of the site at it.
static int installSite(CallSite site) {
    const SiteInfo *info = &SITES[site];
    int rel;
    if (!prepareSiteCode(site)) {
        writeLog("site_skipped", "site=%s reason=not_the_expected_code", info->name);
        return 0;
    }
    unsigned char *first = siteCallAt(site, 0);
    if (!first) {
        writeLog("site_skipped", "site=%s reason=not_on_this_build", info->name);
        return 0;
    }
    unsigned char *slot = !generatedSiteCall(site) && first[0] == 0xff && first[1] == 0x15 ? gameBase + info->targetRva : NULL;
    unsigned char *target = slot ? *(unsigned char **)slot : siteCallTarget(site);
    for (int i = 0; siteCallAt(site, i); i++) {
        unsigned char *call = siteCallAt(site, i);
        if (!isSiteCall(call, target, slot) || !target) {
            writeLog("site_skipped", "site=%s reason=not_the_expected_call address=%p", info->name, call);
            return 0;
        }
    }
    siteThroughSlot[site] = slot != NULL;
    if (!stubPage && !(stubPage = allocNearBytes(first, STUB_PAGES_BYTES))) {
        writeLog("site_skipped", "site=%s reason=no_near_page", info->name);
        return 0;
    }
    unsigned char *stub = siteStub(site), *p = stub;
    static const unsigned char MOVSS_XMM12[] = {0xf3, 0x44, 0x0f, 0x11, 0x25};
    static const unsigned char MOVSS_XMM13[] = {0xf3, 0x44, 0x0f, 0x11, 0x2d};
    static const unsigned char MOV_RSP[] = {0x48, 0x89, 0x25};
    static const unsigned char JMP_INDIRECT[] = {0xff, 0x25};
    static const unsigned char MOV_ECX[] = {0x8b, 0x0d};
    if (info->captureMenuFade) {
        emitRipStore(&p, MOVSS_XMM12, 5, stub + STUB_XMM12);
        emitRipStore(&p, MOVSS_XMM13, 5, stub + STUB_XMM13);
    }
    if (site == SITE_AI_TURN) emitRipStore(&p, MOV_ECX, 2, GAME_PTR(RVA_ACTIVE_PLAYER));
    if (site == SITE_GAME_START) {
        static const unsigned char ZERO_ECX_EDX[] = {0x31, 0xc9, 0x31, 0xd2};
        memcpy(p, ZERO_ECX_EDX, sizeof ZERO_ECX_EDX);
        p += sizeof ZERO_ECX_EDX;
    }
    emitRipStore(&p, MOV_RSP, 3, stub + STUB_RSP);
    emitRipStore(&p, JMP_INDIRECT, 2, stub + STUB_HEAD);
    void *gameTarget = target;
    memcpy(stub + STUB_HEAD, &gameTarget, 8);

    for (int i = 0; siteCallAt(site, i); i++) {
        unsigned char *call = siteCallAt(site, i), code[6] = {0xe8, 0, 0, 0, 0, 0x90};
        size_t size = slot ? 6 : 5;
        rel = (int)(stub - (call + 5));
        memcpy(code + 1, &rel, 4);
        DWORD old;
        VirtualProtect(call, size, PAGE_EXECUTE_READWRITE, &old);
        memcpy(call, code, size);
        VirtualProtect(call, size, old, &old);
        FlushInstructionCache(GetCurrentProcess(), call, size);
    }
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
#define GATE_SIZE      0x60
#define GATE_HOOK      0x50
#define GATE_NEXT      0x58
#define GATE_PAGE_SIZE 0x10000
static unsigned char *gatePage;
static size_t gatePageUsed = GATE_PAGE_SIZE;
static void *volatile enteredGateAddress;
static void *enteredGate(void) { return enteredGateAddress; }
static void setGateNext(void *gate, void *next) { memcpy((unsigned char *)gate + GATE_NEXT, &next, 8); }
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
// Builds a gate that jumps to `next` in a network game and to `hook` otherwise.
static void *makeGate(void *hook, void *next) {
    if (gatePageUsed + GATE_SIZE > GATE_PAGE_SIZE) {
        unsigned char *page = VirtualAlloc(NULL, GATE_PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!page) return NULL;
        gatePage = page;
        gatePageUsed = 0;
    }
    unsigned char *gate = gatePage + gatePageUsed, *p = gate, *fixups[3];
    gatePageUsed += GATE_SIZE;
    const struct { unsigned int rva; unsigned char jcc; } checks[3] = {
        {RVA_NET_CLIENT, 0x7f}, {RVA_NET_SERVER, 0x75}, {RVA_NET_LOBBY, 0x75},
    };
    for (int i = 0; i < 3; i++) {
        uintptr_t flag = (uintptr_t)(gameBase + checks[i].rva);
        *p++ = 0x48; *p++ = 0xb8; memcpy(p, &flag, 8); p += 8;
        *p++ = 0x83; *p++ = 0x38; *p++ = 0x00;
        *p++ = checks[i].jcc; fixups[i] = p++;
    }
    uintptr_t self = (uintptr_t)gate, entered = (uintptr_t)&enteredGateAddress;
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &self, 8); p += 8;
    *p++ = 0x48; *p++ = 0xa3; memcpy(p, &entered, 8); p += 8;
    unsigned char *hookSlot = gate + GATE_HOOK, *nextSlot = gate + GATE_NEXT;
    emitRipStore(&p, (const unsigned char[]){0xff, 0x25}, 2, hookSlot);
    unsigned char *skip = p;
    emitRipStore(&p, (const unsigned char[]){0xff, 0x25}, 2, nextSlot);
    for (int i = 0; i < 3; i++) *fixups[i] = (unsigned char)(skip - (fixups[i] + 1));
    memcpy(hookSlot, &hook, 8);
    memcpy(nextSlot, &next, 8);
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
           site == SITE_WARN_WAIT || site == SITE_TURN_MESSAGES || site == SITE_MESSAGES_WAIT ||
           site == SITE_MAP_EDITOR_TOOLBOX_LINE;
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
    return (void **)(siteStub(site) + STUB_HEAD);
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

// The game's own function for the site: where the innermost wrapper's next goes.
static void *siteGameTarget(CallSite site) {
#ifdef __aarch64__
    return siteGameFunction(site);
#else
    if (site == SITE_GAME_SAVE) return (void *)saveGameToOpenFile;
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

// ---- Plugin .c5m commands --------------------------------------------------------------------
typedef void (*ModCommandHandler)(const char *args, int mod, int level);
static struct ModCommand { char name[48]; ModCommandHandler handler; } *modCommands;
static int modCommandCount, modCommandCapacity;
static void (*nextModCommand)(const char *, int, int);

// Returns 1 in a network game: any of the client, server, and lobby flags is set.
static int isNetworkGame(void) {
#ifdef __APPLE__
    return G8(RVA_NET_CLIENT) || G8(RVA_NET_SERVER) || G8(RVA_NET_LOBBY);
#else
    return G32(RVA_NET_CLIENT) > 0 || G32(RVA_NET_SERVER) != 0 || G32(RVA_NET_LOBBY) != 0;
#endif
}

// ---- Restart prompt after the Mods screen -----------------------------------------------------
// Turning a mod with a DLL or a script on or off takes effect after a restart.
static struct CodeMod { char folder[MAX_PATH]; int enabled; } *codeMods;
static int codeModCount, codeModCapacity, codeModSnapshotFailed;
static u64 (*nextModsScreen)(u64, u64, u64, u64);

// A mod's file names: <ModName>.c5m and its sidecar, <ModName>.c5se.
#define MOD_FILE_SUFFIX ".c5m"
#define SIDECAR_SUFFIX  ".c5se"

// The folder of mod `mod`.
static const char *modFolder(int mod) {
    return (const char *)(gameBase + RVA_MOD_FOLDERS + (size_t)mod * MOD_RECORD_SIZE);
}

static const char *modName(int mod);

static int modEnabled(int mod) {
    return *(short *)(gameBase + RVA_MOD_NAMES + (size_t)mod * MOD_RECORD_SIZE + MOD_RECORD_ENABLED) != 0;
}

// Turns mod `mod` on (1) or off (0) as the game's own writers do.
static void setModEnabled(int mod, int on) {
    *(short *)(gameBase + RVA_MOD_NAMES + (size_t)mod * MOD_RECORD_SIZE + MOD_RECORD_ENABLED) = (short)(on != 0);
}

static int folderHasPlugin(const char *dir) {
    char pattern[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*" PLUGIN_SUFFIX, dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    FindClose(h);
    return 1;
}

// Whether mod `mod` has a script, <folder>/<name>.lua.
static int modHasScript(int mod) {
    char path[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s.lua", modFolder(mod), modName(mod))) return 0;
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

static int modHasCode(int mod) {
    return folderHasPlugin(modFolder(mod)) || modHasScript(mod);
}

static void takeCodeModSnapshot(void) {
    codeModCount = 0;
    codeModSnapshotFailed = 0;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modHasCode(m)) continue;
        if (!growList((void **)&codeMods, &codeModCapacity, codeModCount + 1, sizeof codeMods[0])) {
            codeModSnapshotFailed = 1;
            writeLog("mods_with_code_snapshot_failed", "reason=out_of_memory count=%d", codeModCount);
            return;
        }
        snprintf(codeMods[codeModCount].folder, MAX_PATH, "%s", modFolder(m));
        codeMods[codeModCount++].enabled = modEnabled(m);
    }
}

// Whether a mod with code (modHasCode) was turned on or off since the snapshot.
static int codeModsChanged(void) {
    if (codeModSnapshotFailed) return 0;
    int seen = 0;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modHasCode(m)) continue;
        int k = 0;
        while (k < codeModCount && _stricmp(codeMods[k].folder, modFolder(m))) k++;
        if (k == codeModCount) {
            if (modEnabled(m)) return 1;
            continue;
        }
        if (codeMods[k].enabled != modEnabled(m)) return 1;
        seen++;
    }
    for (int k = 0; k < codeModCount; k++)
        if (codeMods[k].enabled) seen--;
    return seen < 0;
}

// A mod with a DLL that is on now but wasn't at the snapshot: its index, or -1.
static int dllModOnSinceSnapshot(void) {
    if (codeModSnapshotFailed) return -1;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modEnabled(m) || !folderHasPlugin(modFolder(m))) continue;
        int k = 0;
        while (k < codeModCount && _stricmp(codeMods[k].folder, modFolder(m))) k++;
        if (k == codeModCount || !codeMods[k].enabled) return m;
    }
    return -1;
}

#ifdef __APPLE__
// Starts the game again and exits this copy, logging `reason`.
static void relaunchGame(const char *reason) {
    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, sizeof exe);
    pid_t pid;
    int error = posix_spawn(&pid, exe, NULL, NULL, *_NSGetArgv(), *_NSGetEnviron());
    if (error) {
        writeLog("restart_failed", "error=%d", error);
        return;
    }
    writeLog("restarting", "reason=%s", reason);
    exit(0);
}
#else
// Starts CoE5.exe again with the same command line, and exits this copy.
static void relaunchGame(const char *reason) {
    char exe[MAX_PATH], dir[MAX_PATH], cmd[2048];
    GetModuleFileNameA(NULL, exe, sizeof exe);
    snprintf(dir, sizeof dir, "%s", exe);
    char *slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(cmd, sizeof cmd, "%s", GetCommandLineA());
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(exe, cmd, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi)) {
        writeLog("restart_failed", "error=%lu", GetLastError());
        return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    writeLog("restarting", "reason=%s", reason);
    ExitProcess(0);
}
#endif

// Runs after the Mods screen, and offers a restart when a mod with a DLL or a script changed.
static u64 modsScreenWrapper(u64 a1, u64 a2, u64 a3, u64 a4) {
    unsigned int readingsBefore = settingsPassCount(), spellPassesBefore = spellPassCount();
    u64 result = nextModsScreen(a1, a2, a3, a4);
    rereadSettingsWithoutPass(readingsBefore);
    spellsAfterModsScreen(spellPassesBefore);
    if (!codeModsChanged()) return result;
    writeLog("mods_with_code_changed", "count=%d", codeModCount);
    if (messageBox("CoE5SE: Restart needed", "Mod with DLL or script turned on or off.\nRestart required. Restart now?",
                   "Yes", "No", 0, 0, 0) == 1)
        relaunchGame("mods_with_code_changed");
    takeCodeModSnapshot();
    return result;
}

// Writes the last folder and the file name of a path as "<folder>\<file>".
static void folderAndFile(const char *path, char *out, size_t outSize) {
    char copy[MAX_PATH];
    out[0] = '\0';
    if (!buildPath(copy, sizeof copy, __func__, "%s", path)) return;
    for (char *c = copy; *c; c++) if (*c == '/') *c = '\\';
    char *file = strrchr(copy, '\\');
    if (!file) {
        buildPath(out, outSize, __func__, "%s", copy);
        return;
    }
    *file = '\0';
    char *folder = strrchr(copy, '\\');
    buildPath(out, outSize, __func__, "%s\\%s", folder ? folder + 1 : copy, file + 1);
}

// One line of plugins_found.txt: name|source|state|path|sha256.
typedef struct {
    char name[MAX_PATH], source[16], path[MAX_PATH], hash[SHA256_HEX_LEN + 1];
    PluginState state;
} FoundLine;
#define FOUND_LINE_MAX (3 * MAX_PATH)

// Splits `line` at its |s into *out.
static int readFoundLine(char *line, FoundLine *out) {
    line[strcspn(line, "\r\n")] = '\0';
    char *field[5] = {line};
    int n = 1;
    for (char *c = line; *c && n < 5; c++)
        if (*c == '|') {
            *c = '\0';
            field[n++] = c + 1;
        }
    if (n < 4) return 0;
    snprintf(out->name, sizeof out->name, "%s", field[0]);
    snprintf(out->source, sizeof out->source, "%s", field[1]);
    out->state = pluginStateNamed(field[2]);
    snprintf(out->path, sizeof out->path, "%s", field[3]);
    snprintf(out->hash, sizeof out->hash, "%s", n > 4 ? field[4] : "");
    return 1;
}

// Whether the loader saw this plugin at launch, in any state in plugins_found.txt.
static int foundAtLaunch(const char *path) {
    typedef char FoundPath[MAX_PATH];
    static FoundPath *seen;
    static int seenCount = -1, seenCapacity;
    if (seenCount < 0) {
        seenCount = 0;
        static char line[FOUND_LINE_MAX];
        char file[MAX_PATH];
        FoundLine found;
        FILE *f = coe5seUserFile(PLUGINS_FOUND_FILE, file, sizeof file) ? fopen(file, "r") : NULL;
        while (f && fgets(line, sizeof line, f)) {
            if (!readFoundLine(line, &found)) continue;
            if (!growList((void **)&seen, &seenCapacity, seenCount + 1, sizeof seen[0])) break;
            folderAndFile(found.path, seen[seenCount], MAX_PATH);
            if (seen[seenCount][0]) seenCount++;
        }
        if (f) fclose(f);
    }
    char key[MAX_PATH];
    folderAndFile(path, key, sizeof key);
    for (int i = 0; key[0] && i < seenCount; i++)
        if (!_stricmp(seen[i], key)) return 1;
    return 0;
}

// Whether mod `mod`'s folder has a *.c5plugin.dll that was turned on after launch.
static int modHasUnloadedPlugin(int mod) {
    if (mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return 0;
    const char *dir = (const char *)(gameBase + RVA_MOD_FOLDERS + (size_t)mod * MOD_RECORD_SIZE);
    char pattern[MAX_PATH], path[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*" PLUGIN_SUFFIX, dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int unloaded = 0;
    do {
        if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s", dir, fd.cFileName)) unloaded = 1;
        else unloaded = GetModuleHandleA(path) == NULL && !foundAtLaunch(path);
    } while (!unloaded && FindNextFileA(h, &fd));
    FindClose(h);
    return unloaded;
}

// Whether any enabled mod has a DLL that hasn't loaded, because it was turned on this session.
static int anyEnabledModWaitsForItsPlugin(void) {
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (modEnabled(m) && modHasUnloadedPlugin(m)) return 1;
    return 0;
}

// isGameC5mWord's answer from a hash table.
#define GAME_WORD_SLOTS 2048
_Static_assert(sizeof GAME_C5M_WORDS / sizeof GAME_C5M_WORDS[0] * 2 <= GAME_WORD_SLOTS, "GAME_WORD_SLOTS is too small");
static short gameWordSlots[GAME_WORD_SLOTS];

static unsigned int gameWordHash(const char *lower) {
    unsigned int h = 2166136261u;
    for (; *lower; lower++) h = (h ^ (unsigned char)*lower) * 16777619u;
    return h;
}

static int isGameWord(const char *word) {
    static int built;
    if (!built) {
        for (size_t i = 0; i < sizeof GAME_C5M_WORDS / sizeof GAME_C5M_WORDS[0]; i++) {
            unsigned int h = gameWordHash(GAME_C5M_WORDS[i]) & (GAME_WORD_SLOTS - 1);
            while (gameWordSlots[h]) h = (h + 1) & (GAME_WORD_SLOTS - 1);
            gameWordSlots[h] = (short)(i + 1);
        }
        built = 1;
    }
    char lower[64];
    size_t n = strlen(word);
    if (n >= sizeof lower) return isGameC5mWord(word);
    for (size_t i = 0; i <= n; i++) lower[i] = word[i] >= 'A' && word[i] <= 'Z' ? (char)(word[i] - 'A' + 'a') : word[i];
    for (unsigned int h = gameWordHash(lower) & (GAME_WORD_SLOTS - 1); gameWordSlots[h]; h = (h + 1) & (GAME_WORD_SLOTS - 1))
        if (!strcmp(GAME_C5M_WORDS[gameWordSlots[h] - 1], lower)) return 1;
    return 0;
}

// Wraps each .c5m line outside an event.
static int modCommandMod = -1, modCommandLevel = -1;
static void modCommandHook(const char *line, int mod, int level) {
    modCommandMod = mod;
    modCommandLevel = level;
    if (mod >= 0 && mod <= G32(RVA_MOD_LAST_INDEX)) noteModLine(modName(mod), level);
    size_t len = strcspn(line, " \t");
    if (refusedRitualLine(line, len, mod, level)) return;
    if (spellLineHook(line, len, mod, level)) return;
    line = spellPathLineHook(line, len, mod, level);
    if (!line) return;
    line = offsetNameLineHook(line, len, mod, level);
    if (!line) return;
    line = nameListLineHook(line, len, mod, level);
    if (!line) return;
    len = strcspn(line, " \t");
    if (settingLineHook(line, len, mod, level)) return;
    if (flagOffLineHook(line, len, mod, level)) return;
    for (int i = 0; i < modCommandCount; i++) {
        if (strlen(modCommands[i].name) != len || strncmp(modCommands[i].name, line, len)) continue;
        const char *args = line + len;
        while (*args == ' ' || *args == '\t') args++;
        if (!isNetworkGame() || runsInNetworkGames((void *)modCommands[i].handler))
            modCommands[i].handler(args, mod, level);
        return;
    }
    char word[64];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    if (word[0] && !isGameWord(word) && (modHasUnloadedPlugin(mod) || anyEnabledModWaitsForItsPlugin())) {
        writeLog("mod_command_deferred", "word=%s mod=%d level=%d reason=plugin_not_loaded_yet", word, mod, level);
        return;
    }
    nextModCommand(line, mod, level);
}

// The .c5m line hook used when the player declined mods on an unknown game build.
static void declinedModCommandHook(const char *line, int mod, int level) {
    char word[64];
    size_t len = strcspn(line, " \t");
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    int pathName = declinedSpellPathLine(word, mod, level);
    if (word[0] && (!isGameWord(word) || declinedSpellLine(word, mod, level) || pathName || declinedOffsetNameLine(line, len, level) || declinedNameListLine(line, len, level) || declinedFlagOffLine(line))) {
        writeLog("mod_command_skipped", "word=%s mod=%d level=%d reason=plugins_declined", word, mod, level);
        return;
    }
    nextModCommand(line, mod, level);
}

static int apiAddModCommand(const char *name, ModCommandHandler handler) {
    if (!name || !name[0]) return REFUSE_API(0, "addModCommand", "no_name", "name=%s", name ? name : "");
    if (strlen(name) >= sizeof modCommands[0].name || strcspn(name, " \t") != strlen(name))
        return REFUSE_API(0, "addModCommand", "bad_name", "name=%s", name);
    if (!handler) return REFUSE_API(0, "addModCommand", "no_callback", "name=%s", name);
    for (int i = 0; i < modCommandCount; i++)
        if (!strcmp(modCommands[i].name, name)) return REFUSE_API(0, "addModCommand", "duplicate", "name=%s", name);
    if (isGameWord(name)) return REFUSE_API(0, "addModCommand", "game_command", "name=%s", name);
    if (!nextModCommand) return REFUSE_API(0, "addModCommand", "site_unavailable", "name=%s", name);
    if (!growList((void **)&modCommands, &modCommandCapacity, modCommandCount + 1, sizeof modCommands[0]))
        return REFUSE_API(0, "addModCommand", "out_of_memory", "name=%s", name);
    snprintf(modCommands[modCommandCount].name, sizeof modCommands[0].name, "%s", name);
    modCommands[modCommandCount++].handler = handler;
    writeLog("mod_command_added", "name=%s", name);
    return 1;
}

// The .c5m line that makes a mod stop the game without CoE5SE.
static void needsCoe5seLine(const char *args, int mod, int level) {
    (void)args; (void)mod; (void)level;
}

static void needsCoe5seInit(void) {
    int ok = apiAddModCommand("needs_glu32.dll_and_CoE5SE_enabled_at_launch", needsCoe5seLine);
    writeLog("needs_coe5se_word_installed", "ok=%d", ok);
}

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
    for (int i = 0; i < PLAYER_SLOTS; i++) humans += G16(RVA_SLOT_CONTROLLER + 2 * i) == CONTROLLER_HUMAN;
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

// ---- Plugin entries in Mod Settings: screen lines ---------------------------------------------
// A mod's settings live only in its settings files.
#define ID_SCREEN_FIRST   400
#define ID_SCREEN_END     1000
enum { ENTRY_SCREEN, ENTRY_LEFT_OUT };
typedef struct {
    int type;
    char section[MAX_PATH];
    char mod[MOD_NAME_MAX];
    char modulePath[MAX_PATH];
    char label[SETTING_TEXT_MAX + 1], tip[SETTING_TIP_MAX + 1];
    void (*open)(void);
    int lua;
    char problem[160];
} SettingEntry;
static SettingEntry **entries;
static int entryCount, entryCapacity;

// Writes the calling plugin's DLL path, its folder, and its mod.
static void pluginModOf(void *caller, char *path, char *dir, char *mod, size_t modSize) {
    HMODULE m = callingPlugin(caller);
    path[0] = '\0';
    if (m) GetModuleFileNameA(m, path, MAX_PATH);
    const char *file = strrchr(path, PATH_SEP[0]) ? strrchr(path, PATH_SEP[0]) + 1 : path;
    snprintf(dir, MAX_PATH, "%s", path);
    char *slash = strrchr(dir, PATH_SEP[0]);
    if (slash) *slash = '\0';
    if (!slash || !c5mNameInFolder(dir, mod, modSize))
        snprintf(mod, modSize, "%.*s", (int)(strlen(file) > strlen(PLUGIN_SUFFIX) ? strlen(file) - strlen(PLUGIN_SUFFIX) : strlen(file)), file);
}

// Adds an entry for the plugin that called, or for mod `luaMod`'s script.
static SettingEntry *addEntry(void *caller, const char *luaMod, int type, const char *label, const char *tip) {
    SettingEntry *e = calloc(1, sizeof *e);
    if (!e) return NULL;
    if (!growList((void **)&entries, &entryCapacity, entryCount + 1, sizeof entries[0])) {
        free(e);
        return NULL;
    }
    char dir[MAX_PATH];
    if (luaMod) {
        snprintf(e->mod, sizeof e->mod, "%s", luaMod);
        snprintf(e->section, sizeof e->section, "%s.lua", luaMod);
    } else {
        pluginModOf(caller, e->modulePath, dir, e->mod, sizeof e->mod);
        const char *file = strrchr(e->modulePath, PATH_SEP[0]) ? strrchr(e->modulePath, PATH_SEP[0]) + 1 : e->modulePath;
        snprintf(e->section, sizeof e->section, "%s", file);
    }
    e->type = type;
    e->lua = -1;
    snprintf(e->label, sizeof e->label, "%.*s", SETTING_TEXT_MAX, label ? label : "");
    snprintf(e->tip, sizeof e->tip, "%.*s", SETTING_TIP_MAX, tip ? tip : "");
    entries[entryCount++] = e;
    return e;
}

// Makes `e` a left-out entry, logged with the rule's reason.
static void leaveEntryOut(SettingEntry *e, void *caller, const char *call, const SettingProblem *p) {
    refuseApiCall(caller, call, p->reason, "label=%s problem=%s", e->label, p->text);
    snprintf(e->problem, sizeof e->problem, "%.20s %.32s: %.79s", call, e->label, p->text);
    e->type = ENTRY_LEFT_OUT;
    leftOutVersion++;
}

// The lines a mod's screen lines take on its page.
static int pluginPageLines(const char *mod) {
    int lines = 0, boxOpen = 0;
    for (int i = 0; i < entryCount; i++)
        if (entries[i]->type == ENTRY_SCREEN && !_stricmp(entries[i]->mod, mod)) lines += settingPageLines(0, 0, 0, &boxOpen);
    return lines;
}

// Adds a screen line for the plugin that called, or for mod `luaMod`'s script.
static int addScreenFor(void *caller, const char *luaMod, const char *label, const char *tip, void (*open)(void), int lua) {
    if (!open && lua < 0) {
        refuseApiCall(caller, "addScreen", "no_callback", "label=%s", label ? label : "");
        return 0;
    }
    SettingEntry *e = addEntry(caller, luaMod, ENTRY_SCREEN, label, tip);
    if (!e) {
        refuseApiCall(caller, "addScreen", "out_of_memory", "label=%s", label ? label : "");
        return 0;
    }
    e->open = open;
    e->lua = lua;
    SettingProblem p;
    int bad = checkSettingText("label", label, SETTING_TEXT_MAX, &p) || checkSettingText("tip", tip, SETTING_TIP_MAX, &p);
    if (bad) {
        leaveEntryOut(e, caller, "addScreen", &p);
        return 0;
    }
    writeLog("plugin_screen_added", "plugin=%s label=%s", e->section, label);
    return 1;
}

static int apiAddScreen(const char *label, const char *tip, void (*open)(void)) {
    return addScreenFor(__builtin_return_address(0), NULL, label, tip, open, -1);
}

// api->setting: the calling plugin's own mod's setting.
static const int *pluginSetting(void *caller, const char *mod, const char *dir, const char *name);
static const int *apiSetting(const char *name) {
    void *caller = __builtin_return_address(0);
    char path[MAX_PATH], dir[MAX_PATH], mod[MOD_NAME_MAX];
    pluginModOf(caller, path, dir, mod, sizeof mod);
    return pluginSetting(caller, mod, dir, name);
}

// ---- The game's screen widgets for plugins (UiApi) --------------------------------------------
// A mod's text for a game draw function's buffer of `size` bytes, cut to fit.
static const char *fitGameText(void *caller, const char *call, const char *text, char *buf, size_t size) {
    if (strlen(text) < size) return text;
    noteApiCut(caller, call, "text", text, (int)size - 1);
    memcpy(buf, text, size - 1);
    buf[size - 1] = '\0';
    return buf;
}

// common.h's screenTitle, with the translated title cut to screen_title's buffer.
static void uiScreenBegin(const char *title) {
    static char translated[TRANSLATED_SIZE];
    _Static_assert(SCREEN_TITLE_TEXT_SIZE <= TRANSLATED_SIZE, "the title is cut inside the translation buffer");
    screenBegin();
    translateText(title ? title : "", translated, sizeof translated);
    noteApiCut(__builtin_return_address(0), "ui.screenBegin", "title", translated, SCREEN_TITLE_TEXT_SIZE - 1);
    translated[SCREEN_TITLE_TEXT_SIZE - 1] = '\0';
    GAME_FN(void (*)(FMT_ARGS), RVA_SCREEN_TITLE)("%s", translated);
    screenWidth(setupScreenWidth());
}
static void uiSectionBegin(const char *header) {
    sectionBegin();
    if (header) sectionHeader(header);
}
static void uiText(int id, const char *text) {
    char cut[TEXT_BUTTON_TEXT_SIZE];
    textButton(id, fitGameText(__builtin_return_address(0), "ui.text", text ? text : "", cut, sizeof cut));
}
static void uiCheckbox(int id, int *value, const char *label) {
    char cut[CHECKBOX_TEXT_SIZE];
    if (value) checkbox(id, value, fitGameText(__builtin_return_address(0), "ui.checkbox", label ? label : "", cut, sizeof cut));
}
static void uiChoice(int id, int selected, const char *label) {
    char cut[RADIO_OPTION_TEXT_SIZE];
    radioOption(id, selected, fitGameText(__builtin_return_address(0), "ui.choice", label ? label : "", cut, sizeof cut));
}
static void uiTooltip(const char *text) {
    char cut[TOOLTIP_TEXT_SIZE];
    if (text) tooltip(fitGameText(__builtin_return_address(0), "ui.tooltip", text, cut, sizeof cut));
}
// UI_KEY_ESCAPE binds the game's own Escape key, which is 0 while the game binds none.
static void uiBindKey(int id, int key) {
    if (key == UI_KEY_ESCAPE) key = ESCAPE_KEY();
    if (key) bindHotkey(id, key);
}

// The List layout makes the same calls, in the same order, as Load Game's list.
#define LIST_WIDTH         450.0f
#define LIST_GAP           3.0f
static int listRowIndex;
#ifdef __APPLE__
static void uiListBegin(void) {
    screenWidth(LIST_WIDTH);
    buttonRowStyle(LAYOUT_TYPE_LIST_STYLE);
    listRowIndex = 0;
}
static void uiListRow(int id, const char *text) {
    rowBegin(1);
    GAME_FN(void (*)(int, int), RVA_ROW_STRIPE)(listRowIndex++ & 1, (int)LIST_WIDTH);
    rowArea(id, LIST_WIDTH);
    columnX(0.0f);
    char cut[TEXT_BUTTON_TEXT_SIZE];
    textButton(id, fitGameText(__builtin_return_address(0), "ui.listRow", text ? text : "", cut, sizeof cut));
    rowBegin(0);
}
static void uiListEnd(void) { listGap(LIST_GAP); }
#else
static void uiListBegin(void) {
    screenWidth(LIST_WIDTH);
    GAME_FN(unsigned long long (*)(short), RVA_LIST_STYLE)(LAYOUT_TYPE_LIST_STYLE);
    listRowIndex = 0;
}
static void uiListRow(int id, const char *text) {
    GAME_FN(void (*)(int), RVA_ROW_BEGIN)(0);
    GAME_FN(void (*)(int, int), RVA_ROW_STRIPE)(listRowIndex++ & 1, (int)LIST_WIDTH);
    GAME_FN(void (*)(int, float, float, float, float, float), RVA_ROW_AREA)(id, LIST_WIDTH, 0.0f, 0.0f, 0.0f, 0.0f);
    GAME_FN(void (*)(float), RVA_COLUMN_X)(0.0f);
    char cut[TEXT_BUTTON_TEXT_SIZE];
    textButton(id, fitGameText(__builtin_return_address(0), "ui.listRow", text ? text : "", cut, sizeof cut));
    GAME_FN(void (*)(int), RVA_ROW_BEGIN)(1);
}
static void uiListEnd(void) { GAME_FN(void (*)(float), RVA_LIST_GAP)(LIST_GAP); }
#endif

static const UiApi UI = {
    .screenBegin = uiScreenBegin, .sectionBegin = uiSectionBegin, .sectionEnd = sectionEnd, .text = uiText,
    .checkbox = uiCheckbox, .choice = uiChoice, .tooltip = uiTooltip, .buttonRow = buttonRow, .bindKey = uiBindKey,
    .wait = screenWait, .listBegin = uiListBegin, .listRow = uiListRow, .listEnd = uiListEnd,
};

// The screen that More on the main menu opens: the buttons the column has no room for.
#define MORE_ID_BACK      9
#define MORE_ID_FIRST_ROW 100
static void moreButtonsScreen(int first) {
    writeLog("main_menu_more_opened", "buttons=%d", mainButtonCount - first);
    for (;;) {
        uiScreenBegin(MORE_LABEL);
        uiListBegin();
        for (int i = first; i < mainButtonCount; i++) uiListRow(MORE_ID_FIRST_ROW + i, mainButtons[i].label);
        uiListEnd();
        buttonRow();
        uiText(MORE_ID_BACK, "Back");
        uiBindKey(MORE_ID_BACK, UI_KEY_ESCAPE);
        int id = screenWait();
        if (id == MORE_ID_BACK) return;
        if (id >= MORE_ID_FIRST_ROW + first && id < MORE_ID_FIRST_ROW + mainButtonCount) runMainButton(id - MORE_ID_FIRST_ROW);
    }
}

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
#ifdef __aarch64__
    return siteCallerSp(site);
#else
    return *(uintptr_t *)(siteStub(site) + STUB_RSP) + 8;
#endif
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
typedef struct { const char *gameVersion; unsigned int timeDateStamp, imageSize; unsigned char uuid[16]; } GameBuild;
static const GameBuild KNOWN_BUILDS[] = {
#ifdef __APPLE__
    {"5.41", 0, 0, {0xd9, 0xa2, 0x7a, 0xa9, 0xc4, 0xec, 0x37, 0x07, 0x9b, 0x33, 0xb9, 0x4e, 0x2c, 0x57, 0x90, 0x3d}},
#else
    {"5.41", 0x6ad240f0, 0x13728000, {0}},
#endif
};
#define KNOWN_BUILD_COUNT (int)(sizeof KNOWN_BUILDS / sizeof KNOWN_BUILDS[0])
static const GameBuild *gameBuild;
static char gameBuildId[40];
static int pluginsDeclined;
static int notStartedByLoader;

#include "plugin_start.h"

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

// The loader calls this right after loading CoE5SE's DLL.
__declspec(dllexport) int coe5seLoadsPlugins(void) {
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
    menuBgAlpha = (float *)(siteStub(SITE_MAIN_MENU_WAIT) + STUB_XMM12);
    menuFadeTimer = (float *)(siteStub(SITE_MAIN_MENU_WAIT) + STUB_XMM13);
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
// ---- Translating what goes into the game's text --------------------------------------------
// The game translates only a GUI call's format string, not its %s arguments.
typedef int (*TextFormatFn)(u64 options, char *buf, size_t size, const char *fmt, void *locale, va_list args);
static TextFormatFn nextTextFormat;

#define MAX_FORMAT_ARGS    32
#define MAX_TRANSLATED_ARGS 8
#define TRANSLATED_ARG_SIZE 2048

// Marks each argument slot of a printf format that is a narrow %s.
static int scanFormatArgs(const char *fmt, int stringArg[MAX_FORMAT_ARGS]) {
    int slots = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') continue;
        if (*++p == '%') continue;
        while (*p && strchr("-+ #0", *p)) p++;
        if (*p == '*') { if (slots >= MAX_FORMAT_ARGS) return -1; stringArg[slots++] = 0; p++; }
        else while (*p >= '0' && *p <= '9') p++;
        if (*p == '.') {
            p++;
            if (*p == '*') { if (slots >= MAX_FORMAT_ARGS) return -1; stringArg[slots++] = 0; p++; }
            else while (*p >= '0' && *p <= '9') p++;
        }
        int wide = 0;
        if (!strncmp(p, "I64", 3) || !strncmp(p, "I32", 3)) p += 3;
        else if (*p == 'l' || *p == 'w') { wide = 1; p++; if (*p == 'l') p++; }
        else if (*p == 'h') { p++; if (*p == 'h') p++; }
        else if (*p && strchr("LIjzt", *p)) p++;
        if (!*p || !strchr("cCdiouxXeEfFgGaApsSZ", *p) || slots >= MAX_FORMAT_ARGS) return -1;
        stringArg[slots++] = *p == 's' && !wide;
    }
    return slots;
}

// Replaces each %s argument that has a translation with the translated string.
static void translateStringArgs(const int *stringArg, int slots, u64 *args,
                                char translated[MAX_TRANSLATED_ARGS][TRANSLATED_ARG_SIZE]) {
    for (int k = 0, used = 0; k < slots && used < MAX_TRANSLATED_ARGS; k++) {
        const char *s = (const char *)args[k];
        if (!stringArg[k] || !s || strlen(s) >= TRANSLATED_ARG_SIZE - 1) continue;
        translateText(s, translated[used], TRANSLATED_ARG_SIZE);
        if (strcmp(translated[used], s)) args[k] = (u64)translated[used++];
    }
}

static char translatedArgs[MAX_TRANSLATED_ARGS][TRANSLATED_ARG_SIZE];
static int busy;

// Wraps the GUI functions' formatting call, so their %s arguments translate.
static int textFormatHook(u64 options, char *buf, size_t size, const char *fmt, void *locale, va_list args) {
    int stringArg[MAX_FORMAT_ARGS];
    int slots = fmt && !busy ? scanFormatArgs(fmt, stringArg) : -1;
    if (slots <= 0) return nextTextFormat(options, buf, size, fmt, locale, args);
    u64 copy[MAX_FORMAT_ARGS];
    memcpy(copy, args, slots * sizeof copy[0]);
    busy = 1;
    translateStringArgs(stringArg, slots, copy, translatedArgs);
    busy = 0;
    return nextTextFormat(options, buf, size, fmt, locale, (va_list)copy);
}

// The display-name builders format their text before drawing it.
#define MAX_BUILD_ARGS 5
typedef int (*TextBuildFn)(char *buf, u64 size, const char *fmt, u64, u64, u64, u64, u64);
static TextBuildFn nextTextBuild;

static int textBuildHook(char *buf, u64 size, const char *fmt, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5) {
    static char translatedFormat[TRANSLATED_ARG_SIZE];
    int stringArg[MAX_FORMAT_ARGS];
    int slots = fmt && !busy ? scanFormatArgs(fmt, stringArg) : -1;
    if (slots < 0 || slots > MAX_BUILD_ARGS || strlen(fmt) >= TRANSLATED_ARG_SIZE - 1)
        return nextTextBuild(buf, size, fmt, a1, a2, a3, a4, a5);
    u64 args[MAX_BUILD_ARGS] = {a1, a2, a3, a4, a5};
    busy = 1;
    translateText(fmt, translatedFormat, sizeof translatedFormat);
    translateStringArgs(stringArg, slots, args, translatedArgs);
    busy = 0;
    return nextTextBuild(buf, size, translatedFormat, args[0], args[1], args[2], args[3], args[4]);
}

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
#include "spell_table.h"
#include "spell_paths.h"
#include "offset_names.h"
#include "name_lists.h"
#include "monster_flags.h"
#include "game_save.h"
#include "save_tail.h"
#include "save_names.h"
#include "missing_mods.h"
#include "mod_versions.h"
#include "spells_changed.h"
#include "code_check.h"
#include "trace_filter.h"
#include "seal_events.h"
#include "data_area_max.h"
#include "map_editor.h"
#include "lua_scripts.h"
#include "event_finders.h"

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

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(dll);
    commonInit("coe5se");
#ifndef __APPLE__
    if (!GetModuleHandleA(LOADER_DLL_NAME)) {
        notStartedByLoader = 1;
        writeLog("coe5se_inactive", "reason=not_started_by_loader loader=%s", LOADER_DLL_NAME);
        return TRUE;
    }
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
            return TRUE;
        }
    }
    checkNameBoxCode();
    eventFindersInit();
#ifndef __APPLE__
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
#ifndef __aarch64__
    gameSaveInit();
#endif
    keyRebindInit();
    needsCoe5seInit();
    ritualExpansionInit();
    spellPathExpansionInit();
    spellTableMoveInit();
    saveTailInit();
    missingModsInit();
    modVersionsInit();
    spellWordsInit();
    spellPathWordsInit();
    monsterFlagWordsInit();
    nameListsInit();
    maxdataWordInit();
    sidecarInit();
    pluginStartInit();
    modSettingsInit();
    loadOrderInit();
#ifdef COE5SE_EXTRA_START
    COE5SE_EXTRA_START();
#endif
    return TRUE;
}
