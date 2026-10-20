// The key actions behind rebindKey, addActionKey, and actionKey.

#include "key_names.h"

// One action, as key_actions.h lists it.
typedef struct {
    const char *name;
    unsigned char screen;
    unsigned char readMode;
    unsigned char row;
    int id, gameKey;
} KeyActionInfo;

// Each screen's name for the log.
static const char *const KEY_SCREEN_NAMES[KEY_SCREEN_COUNT] = {
    "none", "map", "army", "battle", "map_editor", "battle_editor", "recruit", "participants", "view_army", "unit_list",
    "file_browser",
};

#define KEY_ACTION_INFO_ROW(name, lua, screen, readMode, id, key, row, label) {#name, KEY_SCREEN_##screen, KEY_##readMode, row, id, key},
static const KeyActionInfo KEY_ACTION_INFO[KEY_ACTION_COUNT] = {{"NONE", 0, 0, 0, 0, 0}, KEY_ACTION_LIST(KEY_ACTION_INFO_ROW)};
#undef KEY_ACTION_INFO_ROW

#define KEY_STATE_SIZE     400
#define KEY_UNBOUND        0x7ffffff0
#define HOTKEY_TABLE_SIZE  150
#define SHORTCUTS_ROWS     172

typedef struct { int id, key; } HotkeyEntry;

static int actionKeys[KEY_ACTION_COUNT];
static int actionExtraKeys[KEY_ACTION_COUNT];
// Who changed each action's key last, for the log.
static char actionKeySetBy[KEY_ACTION_COUNT][2][80];
static unsigned char screenChanged[KEY_SCREEN_COUNT];
static int keyScreen;
static int shortcutsListNetwork = -1;
static void refreshShortcutsList(void);
static int unitListSheetOpen;
static int unitListNeedsSheet;

// ---- Key names for players ------------------------------------------------------------------------
// Writes the key an action shows in texts: "y", "Esc/F10" with a second key, "" with none.
static void actionKeyText(int action, char *out, size_t size) {
    char main[24] = "", extra[24] = "";
    if (actionKeys[action]) keyValueName(actionKeys[action], main, sizeof main);
    if (actionExtraKeys[action]) keyValueName(actionExtraKeys[action], extra, sizeof extra);
    if (extra[0] && main[0]) snprintf(out, size, "%s/%s", extra, main);
    else snprintf(out, size, "%s", extra[0] ? extra : main);
}

static int actionIsGame(int action) {
    return actionKeys[action] == KEY_ACTION_INFO[action].gameKey && !actionExtraKeys[action];
}

// The action's name for the log: "map_end_turn".
static void actionLogName(int action, char *out, size_t size) {
    snprintf(out, size, "%s", KEY_ACTION_INFO[action].name);
    for (char *c = out; *c; c++) *c = (char)tolower((unsigned char)*c);
}

static void markScreenChanged(int screen) {
    int changed = 0;
    for (int a = 1; a < KEY_ACTION_COUNT && !changed; a++)
        if (KEY_ACTION_INFO[a].screen == screen && !actionIsGame(a)) changed = 1;
    screenChanged[screen] = (unsigned char)changed;
}

// ---- Bound keys ---------------------------------------------------------------------------------
// The bound action of `screen` whose game binding is {id, key}, or 0.
static int boundActionOf(int screen, int id, int key) {
    for (int a = 1; a < KEY_ACTION_COUNT; a++) {
        const KeyActionInfo *k = &KEY_ACTION_INFO[a];
        if (k->screen == screen && k->readMode == KEY_BOUND && k->id == id && k->gameKey == key) return a;
    }
    return 0;
}

// The changed action of `screen` that has `key` now, as its key or second key, or 0.
static int actionTakingKey(int screen, int key) {
    if (key == KEY_NONE || key == KEY_UNBOUND) return 0;
    for (int a = 1; a < KEY_ACTION_COUNT; a++)
        if (KEY_ACTION_INFO[a].screen == screen && !actionIsGame(a) && (actionKeys[a] == key || actionExtraKeys[a] == key))
            return a;
    return 0;
}

// Logs once per launch that the game's binding {id, key} on `screen` was dropped for `action`.
static void logBindingDropped(int screen, int id, int key, int action) {
    static unsigned int seen[64];
    static int seenCount;
    unsigned int mark = (unsigned int)screen * 1000003u ^ (unsigned int)id * 7919u ^ (unsigned int)key;
    for (int i = 0; i < seenCount; i++)
        if (seen[i] == mark) return;
    if (seenCount < 64) seen[seenCount++] = mark;
    char name[48], keyName[24];
    actionLogName(action, name, sizeof name);
    keyValueName(key, keyName, sizeof keyName);
    writeLog("key_binding_dropped", "screen=%s id=%d key=%s for=%s", KEY_SCREEN_NAMES[screen], id, keyName, name);
}

// Rewrites this frame's shortcut table for `screen`.
static void rewriteShortcuts(int screen) {
    if (!screenChanged[screen] || isNetworkGame()) return;
    HotkeyEntry *table = (HotkeyEntry *)GAME_PTR(RVA_HOTKEY_TABLE);
    int last = G32(RVA_HOTKEY_COUNT);
    if (last >= HOTKEY_TABLE_SIZE) last = HOTKEY_TABLE_SIZE - 1;
    for (int i = 0; i <= last; i++) {
        HotkeyEntry *e = &table[i];
        if (!e->id) continue;
        int action = boundActionOf(screen, e->id, e->key);
        if (action) {
            e->key = actionKeys[action] ? actionKeys[action] : KEY_UNBOUND;
            continue;
        }
        int taker = actionTakingKey(screen, e->key);
        if (taker) {
            logBindingDropped(screen, e->id, e->key, taker);
            e->key = KEY_UNBOUND;
        }
    }
    for (int a = 1; a < KEY_ACTION_COUNT; a++)
        if (KEY_ACTION_INFO[a].screen == screen && KEY_ACTION_INFO[a].readMode == KEY_BOUND && actionExtraKeys[a])
            bindHotkey(KEY_ACTION_INFO[a].id, actionExtraKeys[a]);
}

// Runs one frame's wait of `screen`, with its keys rewritten.
static int runKeyScreenWait(int screen, int (**next)(int, int), int a, int b) {
    int outer = keyScreen;
    keyScreen = screen;
    if (isNetworkGame() != shortcutsListNetwork) refreshShortcutsList();
    rewriteShortcuts(screen);
    int id = (*next)(a, b);
    keyScreen = outer;
    return id;
}

#define KEY_SCREEN_WAIT(screen)                                                               \
    static int (*nextKeyWait_##screen)(int, int);                                             \
    static int keyWait_##screen(int a, int b) {                                               \
        return runKeyScreenWait(KEY_SCREEN_##screen, &nextKeyWait_##screen, a, b);            \
    }
KEY_SCREEN_WAIT(MAP)
KEY_SCREEN_WAIT(ARMY)
KEY_SCREEN_WAIT(BATTLE)
KEY_SCREEN_WAIT(MAP_EDITOR)
KEY_SCREEN_WAIT(BATTLE_EDITOR)
KEY_SCREEN_WAIT(RECRUIT)
KEY_SCREEN_WAIT(PARTICIPANTS)
KEY_SCREEN_WAIT(VIEW_ARMY)
KEY_SCREEN_WAIT(FILE_BROWSER)
#undef KEY_SCREEN_WAIT

// The unit list's wait.
static int (*nextKeyWait_UNIT_LIST)(int, int);
static int keyWait_UNIT_LIST(int a, int b) {
    if (unitListNeedsSheet && !unitListSheetOpen) return nextKeyWait_UNIT_LIST(a, b);
    return runKeyScreenWait(KEY_SCREEN_UNIT_LIST, &nextKeyWait_UNIT_LIST, a, b);
}
// unsigned int sheet(void): the unit list's call to the list sheet.
static unsigned int (*nextUnitListSheet)(void);
static unsigned int unitListSheetHook(void) {
    int outer = unitListSheetOpen;
    unitListSheetOpen = 1;
    unsigned int result = nextUnitListSheet();
    unitListSheetOpen = outer;
    return result;
}

// ---- Held keys ----------------------------------------------------------------------------------
// SDL2's key and window events, as SDL_PollEvent fills them.
#define SDL_WINDOW_EVENT       0x200
#define SDL_KEY_DOWN           0x300
#define SDL_KEY_UP             0x301
#define SDL_FOCUS_GAINED       12
#define SDL_FOCUS_LOST         13
#define SDL_EVENT_SYM          20
#define SDL_EVENT_WINDOW_EVENT 12
#define SDLK_LCTRL             0x400000e0
#define SDLK_LSHIFT            0x400000e1
#define SDLK_LALT              0x400000e2
#define SDLK_RCTRL             0x400000e4
#define SDLK_RSHIFT            0x400000e5
#define SDLK_RALT              0x400000e6

static unsigned char keysHeld[KEY_STATE_SIZE];
static unsigned char keyStateWritten[KEY_STATE_SIZE];
static int modifierCodes[6];
static int (*nextPollEvent)(void *);

static int keySymToCode(int sym) { return (int)GAME_FN(unsigned int (*)(unsigned int), RVA_KEY_SYM_TO_CODE)((unsigned int)sym); }

static int keyCodeHeld(int code) { return code > 0 && code < KEY_STATE_SIZE && keysHeld[code]; }

// 1 while the keys of key value `key` are held down: its key and its modifiers.
static int keyValueHeld(int key) {
    if (key == KEY_NONE) return 0;
    int base = keyBase(key), modifier = keyModifier(key), shift = 0;
    if (base >= 'A' && base <= 'Z') {
        base += 'a' - 'A';
        shift = 1;
    }
    if (modifier == KEY_CTRL_SHIFT) shift = 1;
    int ctrl = keyCodeHeld(modifierCodes[0]) || keyCodeHeld(modifierCodes[1]);
    int shiftHeld = keyCodeHeld(modifierCodes[2]) || keyCodeHeld(modifierCodes[3]);
    int alt = keyCodeHeld(modifierCodes[4]) || keyCodeHeld(modifierCodes[5]);
    if (shift && !shiftHeld) return 0;
    if ((modifier == KEY_CTRL || modifier == KEY_CTRL_SHIFT) && !ctrl) return 0;
    if (modifier == KEY_ALT && !alt) return 0;
    return keyCodeHeld(base);
}

// At the end of a poll: sets each changed held action's game key state from its own keys.
static void writeHeldKeys(void) {
    unsigned char want[KEY_STATE_SIZE], set[KEY_STATE_SIZE];
    memset(want, 0, sizeof want);
    memset(set, 0, sizeof set);
    int screen = isNetworkGame() ? 0 : keyScreen;
    if (screen && screenChanged[screen])
        for (int a = 1; a < KEY_ACTION_COUNT; a++) {
            const KeyActionInfo *k = &KEY_ACTION_INFO[a];
            if (k->screen != screen || k->readMode != KEY_HELD || actionIsGame(a)) continue;
            int code = k->gameKey;
            if (code <= 0 || code >= KEY_STATE_SIZE) continue;
            want[code] = (unsigned char)((set[code] && want[code]) || keyValueHeld(actionKeys[a]) || keyValueHeld(actionExtraKeys[a]));
            set[code] = 1;
        }
    unsigned char *state = GAME_PTR(RVA_KEY_STATE);
    for (int c = 1; c < KEY_STATE_SIZE; c++) {
        if (set[c]) state[c] = want[c];
        else if (keyStateWritten[c]) state[c] = keysHeld[c];
        keyStateWritten[c] = set[c];
    }
}

// Frames the game has run: its event poll ends once a frame.
static unsigned long gameFrames;

// SDL_PollEvent, as the game's event poll calls it until it returns 0.
static int pollEventHook(void *event) {
    int got = nextPollEvent(event);
    if (!got) {
        gameFrames++;
        writeHeldKeys();
        return 0;
    }
    if (!event) return got;
    const unsigned char *e = event;
    unsigned int type;
    memcpy(&type, e, sizeof type);
    if (type == SDL_KEY_DOWN || type == SDL_KEY_UP) {
        int sym;
        memcpy(&sym, e + SDL_EVENT_SYM, sizeof sym);
        int code = keySymToCode(sym);
        if (code > 0 && code < KEY_STATE_SIZE) keysHeld[code] = type == SDL_KEY_DOWN;
    } else if (type == SDL_WINDOW_EVENT && (e[SDL_EVENT_WINDOW_EVENT] == SDL_FOCUS_GAINED || e[SDL_EVENT_WINDOW_EVENT] == SDL_FOCUS_LOST)) {
        memset(keysHeld, 0, sizeof keysHeld);
    }
    return got;
}

// ---- Texts that name a key ----------------------------------------------------------------------
// Each text that names a key action's key.
typedef struct {
    CallSite site;
    const char *text;
    const char *marks[2];
    int actions[2];
} KeyText;
static const KeyText KEY_TEXTS[] = {
    {SITE_KEY_TEXT_BAR, "(y) End Turn, current turn is %s", {"(y)"}, {KEY_ACTION_MAP_END_TURN}},
    {SITE_KEY_TEXT_BAR, "(n) Next commander", {"(n)"}, {KEY_ACTION_MAP_NEXT_COMMANDER}},
    {SITE_KEY_TEXT_BAR, "(r) Recruit new units", {"(r)"}, {KEY_ACTION_MAP_RECRUIT}},
    {SITE_KEY_TEXT_BAR, "(l) Trade administration", {"(l)"}, {KEY_ACTION_MAP_TRADE}},
    {SITE_KEY_TEXT_BAR, "(m) Read this month's message", {"(m)"}, {KEY_ACTION_MAP_MESSAGES}},
    {SITE_KEY_TEXT_BAR, "(ctrl-a) Astrology", {"(ctrl-a)"}, {KEY_ACTION_MAP_ASTROLOGY}},
    {SITE_KEY_TEXT_BAR, "(F1) Unit overview", {"(F1)"}, {KEY_ACTION_MAP_UNIT_OVERVIEW}},
    {SITE_KEY_TEXT_BAR, "(F2) Player overview", {"(F2)"}, {KEY_ACTION_MAP_PLAYER_OVERVIEW}},
    {SITE_KEY_TEXT_BAR, "(F5) Scoregraphs", {"(F5)"}, {KEY_ACTION_MAP_SCORE_GRAPHS}},
    {SITE_KEY_TEXT_BAR, "(F5) Scoregraphs (disabled)", {"(F5)"}, {KEY_ACTION_MAP_SCORE_GRAPHS}},
    {SITE_KEY_TEXT_BAR, "(s) Put commander in sentry mode. This will make him inactive until you select him manually.",
     {"(s)"}, {KEY_ACTION_MAP_SENTRY}},
    {SITE_KEY_TEXT_BAR, "(t) Select which troops the active commander should lead.", {"(t)"}, {KEY_ACTION_MAP_TRANSFER}},
    {SITE_KEY_TEXT_BAR, "(p) Use a special power the active commander possesses.", {"(p)"}, {KEY_ACTION_MAP_SPECIAL_POWER}},
    {SITE_KEY_TEXT_BAR, "(b) Have the commander or his troops chop or burn down the forest.", {"(b)"},
     {KEY_ACTION_MAP_BURN_FOREST_SCREEN}},
    {SITE_KEY_TEXT_BAR, "(u) Transport commander and followers with the plane shift ability to another plane.", {"(u)"},
     {KEY_ACTION_MAP_PLANE_SHIFT}},
    {SITE_KEY_TEXT_BAR, "(,) Move as long as possible on the currently set path. (.) Continue moving for all commanders.",
     {"(,)", "(.)"}, {KEY_ACTION_MAP_CONTINUE, KEY_ACTION_MAP_CONTINUE_ALL}},
    {SITE_KEY_TEXT_BAR, "(h) Have all able commanders catch and train slaves for the army.", {"(h)"}, {KEY_ACTION_MAP_HUNT_SLAVES}},
    {SITE_KEY_TEXT_BAR, "(o) Deactivate the special power of this location.", {"(o)"}, {KEY_ACTION_MAP_LOCATION_POWER}},
    {SITE_KEY_TEXT_BAR, "(o) Use the special power of this location.", {"(o)"}, {KEY_ACTION_MAP_LOCATION_POWER}},
    {SITE_MAP_MENU_BUTTON_TEXT, "(F10) Menu", {"(F10)"}, {KEY_ACTION_MAP_GAME_MENU}},
    {SITE_KEY_TEXT_BUTTON, "(i) terrain info", {"(i)"}, {KEY_ACTION_MAP_SQUARE_INFO}},
    {SITE_KEY_HELP_TEXT, "(r) Makes a new random world", {"(r)"}, {KEY_ACTION_MAP_EDITOR_RANDOM_WORLD}},
    {SITE_KEY_HELP_TEXT, "(t) Select a terrain from a list. You can also select terrains by right clicking on the map.", {"(t)"},
     {KEY_ACTION_MAP_EDITOR_TERRAIN_LIST}},
    {SITE_KEY_HELP_TEXT, "(F) This removes all mines, villages, towns, bridges, swamps, hills etc.", {"(F)"},
     {KEY_ACTION_MAP_EDITOR_REMOVE_FANCY}},
    {SITE_KEY_HELP_TEXT, "(tab) Hides/shows the toolbox", {"(tab)"}, {KEY_ACTION_MAP_EDITOR_MENU}},
    {SITE_KEY_HELP_TEXT, "(q) Exit the map editor, map will be lost if it has not been saved", {"(q)"},
     {KEY_ACTION_MAP_EDITOR_QUIT}},
};
#define KEY_TEXT_COUNT (int)(sizeof KEY_TEXTS / sizeof KEY_TEXTS[0])

// Replaces the first `mark` in `text` with "(<key>)", or removes it when the action has no key.
static void replaceKeyMark(char *text, size_t size, const char *mark, int action) {
    char *at = strstr(text, mark);
    if (!at) return;
    char key[56], rest[TRANSLATED_SIZE];
    actionKeyText(action, key, sizeof key);
    const char *after = at + strlen(mark);
    if (!key[0] && *after == ' ') after++;
    snprintf(rest, sizeof rest, "%s", after);
    size_t head = (size_t)(at - text);
    if (key[0]) snprintf(at, size - head, "(%s)%s", key, rest);
    else snprintf(at, size - head, "%s", rest);
}

// Formats a key text site's text with each action's key.
static const char *formatKeyText(CallSite site, const char *fmt, va_list ap) {
    static char translated[TRANSLATED_SIZE], out[TRANSLATED_SIZE];
    static unsigned char logged[KEY_TEXT_COUNT];
    translateText(fmt ? fmt : "", translated, sizeof translated);
    if (fmt && !isNetworkGame())
        for (int i = 0; i < KEY_TEXT_COUNT; i++) {
            const KeyText *t = &KEY_TEXTS[i];
            if (t->site != site || strcmp(fmt, t->text)) continue;
            int changed = 0;
            for (int m = 0; m < 2 && t->marks[m]; m++)
                if (!actionIsGame(t->actions[m])) {
                    replaceKeyMark(translated, sizeof translated, t->marks[m], t->actions[m]);
                    changed = 1;
                }
            if (changed && !logged[i]) {
                logged[i] = 1;
                writeLog("key_text_rewritten", "site=%s text=%.60s", SITES[site].name, translated);
            }
            break;
        }
    vsnprintf(out, sizeof out, translated, ap);
    return out;
}

static u64 (*nextKeyTextBar)(const char *, ...);
static u64 keyTextBarHook(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *text = formatKeyText(SITE_KEY_TEXT_BAR, fmt, ap);
    va_end(ap);
    return nextKeyTextBar("%s", text);
}
static u64 (*nextMenuButtonText)(const char *, ...);
static u64 menuButtonTextHook(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *text = formatKeyText(SITE_MAP_MENU_BUTTON_TEXT, fmt, ap);
    va_end(ap);
    return nextMenuButtonText("%s", text);
}
static u64 (*nextKeyHelpText)(const char *, ...);
static u64 keyHelpTextHook(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *text = formatKeyText(SITE_KEY_HELP_TEXT, fmt, ap);
    va_end(ap);
    return nextKeyHelpText("%s", text);
}
static u64 (*nextKeyTextButton)(int, const char *, ...);
static u64 keyTextButtonHook(int id, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *text = formatKeyText(SITE_KEY_TEXT_BUTTON, fmt, ap);
    va_end(ap);
    return nextKeyTextButton(id, "%s", text);
}

// ---- The Keyboard Shortcuts list ----------------------------------------------------------------
// The list's {key text, description} table is the game's data.
static const char *gameRowTexts[SHORTCUTS_ROWS];
static char rowTexts[SHORTCUTS_ROWS][64];
static int shortcutsTableReady;

// Writes a row's key text from its actions' keys.
static void buildRowText(int row, char *out, size_t size) {
    char names[12][56];
    int count = 0, allSingle = 1;
    for (int a = 1; a < KEY_ACTION_COUNT && count < 12; a++) {
        if (KEY_ACTION_INFO[a].row != row) continue;
        actionKeyText(a, names[count], sizeof names[count]);
        if (!names[count][0]) snprintf(names[count], sizeof names[count], "-");
        if (strlen(names[count]) != 1) allSingle = 0;
        count++;
    }
    const char *game = gameRowTexts[row] ? gameRowTexts[row] : "";
    const char *sep;
    if (strstr(game, " / ")) sep = " / ";
    else if (strchr(game, '/')) sep = "/";
    else if (strchr(game, ',') || strchr(game, '-')) sep = ",";
    else if (allSingle) sep = "";
    else sep = "/";
    size_t n = 0;
    out[0] = '\0';
    for (int i = 0; i < count && n < size; i++)
        n += (size_t)snprintf(out + n, size - n, "%s%s", i ? sep : "", names[i]);
}

// Points each row of the Keyboard Shortcuts table at its keys now, or at the game's text.
static void refreshShortcutsList(void) {
    const char **pairs = (const char **)GAME_PTR(RVA_SHORTCUTS_TABLE);
    if (!shortcutsTableReady) {
        if (!makeGameDataWritable(pairs, SHORTCUTS_ROWS * 2 * sizeof *pairs)) {
            writeLog("shortcuts_list_skipped", "reason=not_writable");
            shortcutsTableReady = -1;
        }
        if (shortcutsTableReady == 0) {
            for (int r = 0; r < SHORTCUTS_ROWS; r++) {
                gameRowTexts[r] = pairs[2 * r];
                if (pairs[2 * r] && !strcmp(pairs[2 * r], "__end")) break;
            }
            shortcutsTableReady = 1;
        }
    }
    if (shortcutsTableReady != 1) return;
    int network = isNetworkGame();
    shortcutsListNetwork = network;
    for (int a = 1; a < KEY_ACTION_COUNT; a++) {
        int row = KEY_ACTION_INFO[a].row;
        if (!gameRowTexts[row]) continue;
        int game = 1;
        for (int b = 1; b < KEY_ACTION_COUNT; b++)
            if (KEY_ACTION_INFO[b].row == row && !actionIsGame(b)) game = 0;
        if (game || network) {
            pairs[2 * row] = gameRowTexts[row];
            continue;
        }
        buildRowText(row, rowTexts[row], sizeof rowTexts[row]);
        pairs[2 * row] = rowTexts[row];
    }
}

// ---- The API --------------------------------------------------------------------------------------
// The screens other than the action's own whose game keys include `key`, for the log.
static void screensReadingKey(int key, int ownScreen, char *out, size_t size) {
    size_t n = 0;
    out[0] = '\0';
    for (int s = 1; s < KEY_SCREEN_COUNT && n < size; s++) {
        if (s == ownScreen || key == KEY_NONE) continue;
        for (int a = 1; a < KEY_ACTION_COUNT; a++)
            if (KEY_ACTION_INFO[a].screen == s && KEY_ACTION_INFO[a].gameKey == key) {
                n += (size_t)snprintf(out + n, size - n, "%s%s", n ? "," : "", KEY_SCREEN_NAMES[s]);
                break;
            }
    }
}

// rebindKey (extra 0) and addActionKey (extra 1).
static int setActionKeyFor(void *caller, const char *call, int action, int key, int extra) {
    if (action <= KEY_ACTION_NONE || action >= KEY_ACTION_COUNT) {
        refuseApiCall(caller, call, "unknown_action", "action=%d key=%d", action, key);
        return 0;
    }
    char name[48], keyName[24], shared[64];
    actionLogName(action, name, sizeof name);
    if (!keyValueIsValid(key)) {
        refuseApiCall(caller, call, "bad_key", "action=%s key=%d", name, key);
        return 0;
    }
    int *slot = extra ? &actionExtraKeys[action] : &actionKeys[action];
    if (*slot == key) return 1;
    *slot = key;
    markScreenChanged(KEY_ACTION_INFO[action].screen);
    keyValueName(key, keyName, sizeof keyName);
    screensReadingKey(key, KEY_ACTION_INFO[action].screen, shared, sizeof shared);
    char who[64], by[sizeof actionKeySetBy[0][0]];
    const char *field = apiCallerName(caller, who, sizeof who);
    snprintf(by, sizeof by, "%s=%.63s", field, who);
    writeLog("key_rebound", "%s action=%s key=%s second=%d game_key=%d also_read_on_screens=%s", by, name, keyName, extra,
             KEY_ACTION_INFO[action].gameKey, shared[0] ? shared : "none");
    char *setBy = actionKeySetBy[action][extra ? 1 : 0];
    if (setBy[0] && strcmp(setBy, by))
        writeLog("key_rebound_over", "%s action=%s second=%d over_%s", by, name, extra, setBy);
    snprintf(setBy, sizeof actionKeySetBy[0][0], "%s", by);
    if (shortcutsTableReady == 1) refreshShortcutsList();
    return 1;
}

static int apiRebindKey(int action, int key) {
    return setActionKeyFor(__builtin_return_address(0), "rebindKey", action, key, 0);
}
static int apiAddActionKey(int action, int key) {
    return setActionKeyFor(__builtin_return_address(0), "addActionKey", action, key, 1);
}
static int apiActionKey(int action) {
    if (action <= KEY_ACTION_NONE || action >= KEY_ACTION_COUNT)
        return REFUSE_API(-1, "actionKey", "unknown_action", "action=%d", action);
    return actionKeys[action];
}

// ---- Start --------------------------------------------------------------------------------------
// Wraps every screen's wait, the event poll, the key texts, and the Keyboard Shortcuts list.
static int wrapInnermost(CallSite site, void *hook, void **next) {
    char owner[64];
    moduleFileName(hook, owner, sizeof owner);
    return insertWrap(site, hook, next, (WrapPlace){.priority = INT_MIN}, owner, NULL);
}

static void keyRebindInit(void) {
    for (int a = 1; a < KEY_ACTION_COUNT; a++) actionKeys[a] = KEY_ACTION_INFO[a].gameKey;
    static const int MODIFIER_SYMS[6] = {SDLK_LCTRL, SDLK_RCTRL, SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LALT, SDLK_RALT};
    for (int i = 0; i < 6; i++) modifierCodes[i] = keySymToCode(MODIFIER_SYMS[i]);
    unitListNeedsSheet = wrapSite(SITE_UNIT_LIST_SHEET, (void *)unitListSheetHook, (void **)&nextUnitListSheet);
    int waits = wrapInnermost(SITE_MAP_SCREEN_WAIT, (void *)keyWait_MAP, (void **)&nextKeyWait_MAP) +
                wrapInnermost(SITE_ARMY_WAIT, (void *)keyWait_ARMY, (void **)&nextKeyWait_ARMY) +
                wrapInnermost(SITE_BATTLE_WAIT, (void *)keyWait_BATTLE, (void **)&nextKeyWait_BATTLE) +
                wrapInnermost(SITE_MAP_EDITOR_WAIT, (void *)keyWait_MAP_EDITOR, (void **)&nextKeyWait_MAP_EDITOR) +
                wrapInnermost(SITE_BATTLE_EDITOR_WAIT, (void *)keyWait_BATTLE_EDITOR, (void **)&nextKeyWait_BATTLE_EDITOR) +
                wrapInnermost(SITE_RECRUIT_WAIT, (void *)keyWait_RECRUIT, (void **)&nextKeyWait_RECRUIT) +
                wrapInnermost(SITE_PARTICIPANTS_WAIT, (void *)keyWait_PARTICIPANTS, (void **)&nextKeyWait_PARTICIPANTS) +
                wrapInnermost(SITE_VIEW_ARMY_WAIT, (void *)keyWait_VIEW_ARMY, (void **)&nextKeyWait_VIEW_ARMY) +
                wrapInnermost(SITE_UNIT_LIST_WAIT, (void *)keyWait_UNIT_LIST, (void **)&nextKeyWait_UNIT_LIST) +
                wrapInnermost(SITE_FILE_BROWSER_WAIT, (void *)keyWait_FILE_BROWSER, (void **)&nextKeyWait_FILE_BROWSER);
    int poll = wrapSite(SITE_POLL_EVENT, (void *)pollEventHook, (void **)&nextPollEvent);
    int texts = wrapSite(SITE_KEY_TEXT_BAR, (void *)keyTextBarHook, (void **)&nextKeyTextBar) +
                wrapInnermost(SITE_MAP_MENU_BUTTON_TEXT, (void *)menuButtonTextHook, (void **)&nextMenuButtonText) +
                wrapSite(SITE_KEY_HELP_TEXT, (void *)keyHelpTextHook, (void **)&nextKeyHelpText) +
                wrapSite(SITE_KEY_TEXT_BUTTON, (void *)keyTextButtonHook, (void **)&nextKeyTextButton);
    refreshShortcutsList();
    writeLog("key_actions_installed", "actions=%d screen_waits=%d of 10 poll=%d texts=%d of 4 list=%d unit_list_sheet=%d",
             KEY_ACTION_COUNT - 1, waits, poll, texts, shortcutsTableReady == 1, unitListNeedsSheet);
}
