// Mods' screens: their entries in Mod Settings, the game's screen widgets for plugins (UiApi), and the main menu's More screen.

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
