// The Mod Settings screen: the mod list, a mod's pages, the key list, and the pop-up for local mods.

// ---- The Mod Settings screen ---------------------------------------------------------------------
// One row in the mod list.
typedef struct { char mod[MOD_NAME_MAX], label[80]; int plugin; } ListedMod;

// The kept settings of the mod's page `page` ("" for the mod's own page), in file order.
static int subPageSettings(const char *mod, const char *page, ModSetting ***out, int *capacity) {
    int n = 0;
    for (int i = 0; i < settingCount; i++) {
        ModSetting *s = settings[i];
        if (!s->declared || s->rejected || _stricmp(s->mod, mod) || _stricmp(s->page, page)) continue;
        if (!growList((void **)out, capacity, n + 1, sizeof (*out)[0])) break;
        int k = n++;
        for (; k > 0 && (*out)[k - 1]->order > s->order; k--) (*out)[k] = (*out)[k - 1];
        (*out)[k] = s;
    }
    return n;
}

// The kept settings of a mod's own page, in file order.
static int settingsOfModPage(const char *mod, ModSetting ***out, int *capacity) {
    return subPageSettings(mod, "", out, capacity);
}

// The mod's pages ([page] sections), in file order, into out[SETTING_PAGES_MAX].
static int modPagesOf(const char *mod, const ModPage **out) {
    int n = 0;
    for (int i = 0; i < modPageCount && n < SETTING_PAGES_MAX; i++)
        if (!_stricmp(modPages[i].mod, mod)) out[n++] = &modPages[i];
    return n;
}

static void drawPluginRejections(const char *mod, int *open);

// ---- A mod's pages -------------------------------------------------------------------------------
// A mod's settings take as many pages as they need, each at most SETTINGS_PAGE_LINES lines (rule 7).
typedef struct {
    int first, count;
    int titleFrom;
    int lines, screens, continued;
    int subPages;
    int page;
} PageBox;

// Lays out a mod's settings, sub-pages, and plugin screen lines on pages.
static int layoutPageBoxesOf(ModSetting **page, int count, int subPages, int screenLines, int pageLines, PageBox **boxes,
                             int *capacity) {
    int n = 0;
    const int partMax = pageLines - SETTINGS_BOX_LINES;
    for (int j = 0; j < count;) {
        int isChoice = page[j]->optionCount > 0, boxCount = 1;
        if (!isChoice)
            while (j + boxCount < count && !page[j + boxCount]->optionCount && !page[j + boxCount]->header[0]) boxCount++;
        for (int done = 0; done < boxCount;) {
            if (!growList((void **)boxes, capacity, n + 1, sizeof **boxes)) return n;
            PageBox *b = &(*boxes)[n++];
            memset(b, 0, sizeof *b);
            b->first = j + done;
            b->titleFrom = j;
            b->count = isChoice || boxCount - done <= partMax ? boxCount - done : partMax;
            b->lines = SETTINGS_BOX_LINES + (isChoice ? page[j]->optionCount : b->count);
            b->continued = done > 0;
            done += b->count;
        }
        j += boxCount;
    }
    for (int done = 0; done < subPages;) {
        if (!growList((void **)boxes, capacity, n + 1, sizeof **boxes)) return n;
        PageBox *b = &(*boxes)[n++];
        memset(b, 0, sizeof *b);
        b->first = done;
        b->count = subPages - done <= partMax ? subPages - done : partMax;
        b->lines = SETTINGS_BOX_LINES + b->count;
        b->subPages = 1;
        b->continued = done > 0;
        done += b->count;
    }
    if (screenLines && growList((void **)boxes, capacity, n + 1, sizeof **boxes)) {
        PageBox *b = &(*boxes)[n++];
        memset(b, 0, sizeof *b);
        b->lines = screenLines;
        b->screens = 1;
    }
    int pageNumber = 0, used = 0;
    for (int i = 0; i < n; i++) {
        if (used && used + (*boxes)[i].lines > pageLines) {
            pageNumber++;
            used = 0;
        }
        (*boxes)[i].page = pageNumber;
        used += (*boxes)[i].lines;
    }
    return n;
}

// Lays out a mod's pages.
static int layoutPageBoxes(ModSetting **page, int count, int subPages, int screenLines, PageBox **boxes, int *capacity) {
    int n = layoutPageBoxesOf(page, count, subPages, screenLines, SETTINGS_PAGE_LINES, boxes, capacity);
    if (n && (*boxes)[n - 1].page > 0)
        n = layoutPageBoxesOf(page, count, subPages, screenLines, SETTINGS_PAGE_LINES - SETTINGS_PAGE_BUTTON_LINES, boxes,
                              capacity);
    return n;
}

// The line a key setting shows: "End turn: y", or "End turn: (none)".
static void keySettingLine(const ModSetting *s, char *out, size_t size) {
    char key[24];
    if (s->value == KEY_NONE) snprintf(key, sizeof key, "(none)");
    else keyValueName(s->value, key, sizeof key);
    snprintf(out, size, "%s: %s", s->label, key);
}

// A text line drawn at half alpha, like a button the game doesn't allow.
static void greyedButton(int id, const char *text) {
    GAME_FN(void (*)(int, float, float, float, float, FMT_ARGS), RVA_BUTTON_CORE)(id, -1.0f, -1.0f, -1.0f, 0.5f, "%s", text);
}

// Draws the boxes of page `pageNumber`.
static void drawPageBoxes(ModSetting **page, const ModPage *const *subPages, const PageBox *boxes, int boxCount, int pageNumber,
                          int plugin) {
    char title[SETTING_TEXT_MAX + 16], line[SETTING_TEXT_MAX + 32];
    for (int i = 0; i < boxCount; i++) {
        const PageBox *b = &boxes[i];
        if (b->page != pageNumber) continue;
        if (b->screens) {
            if (plugin >= 0) api->settingsPluginDraw(plugin);
            continue;
        }
        if (b->subPages) {
            if (!subPages) continue;
            api->ui->sectionBegin(b->continued ? "Pages (continued)" : "Pages");
            for (int k = b->first; k < b->first + b->count; k++) api->ui->text(ID_SUBPAGE + k, subPages[k]->label);
            api->ui->sectionEnd();
            continue;
        }
        ModSetting *first = page[b->first];
        if (first->optionCount) {
            api->ui->sectionBegin(first->label);
            for (int k = 0; k < first->optionCount; k++) {
                api->ui->choice(ID_CHOICE + b->first * SETTING_OPTIONS_MAX + k, first->value == k, first->options[k]);
                if (first->optionTips[k][0]) api->ui->tooltip(first->optionTips[k]);
            }
            api->ui->sectionEnd();
            continue;
        }
        const char *head = page[b->titleFrom]->header[0] ? page[b->titleFrom]->header : "Settings";
        snprintf(title, sizeof title, b->continued ? "%s (continued)" : "%s", head);
        api->ui->sectionBegin(title);
        for (int j = b->first; j < b->first + b->count; j++) {
            ModSetting *s = page[j];
            if (s->isKey) {
                keySettingLine(s, line, sizeof line);
                if (s->fixed) greyedButton(0, line);
                else api->ui->text(ID_KEY + j, line);
            } else {
                api->ui->checkbox(ID_ONOFF + j, &s->value, s->label);
            }
            if (s->tip[0]) api->ui->tooltip(s->tip);
        }
        api->ui->sectionEnd();
    }
}

// Draws all of a page's settings on one screen.
static void drawIniSettings(ModSetting **page, int count) {
    static PageBox *boxes;
    static int capacity;
    int boxCount = layoutPageBoxes(page, count, 0, 0, &boxes, &capacity);
    for (int i = 0; i < boxCount; i++) boxes[i].page = 0;
    drawPageBoxes(page, NULL, boxes, boxCount, 0, -1);
}

// ---- The key list --------------------------------------------------------------------------------
// The key list a key setting's line opens.
#define PICK_WIDTH       460.0f
#define PICK_ID_BACK     9
#define PICK_ID_NONE     7
#define PICK_ID_DEFAULT  8
#define PICK_ID_TAB      20
#define PICK_ID_KEY      100
#define PICK_KEYS_MAX    160

// A row of the Settings layout with several texts side by side.
#ifdef __APPLE__
static void gridRowOpen(void) { rowBegin(1); }
static void gridRowClose(void) { rowBegin(0); }
static void gridColumn(float x) { columnX(x); }
#else
static void gridRowOpen(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(0); }
static void gridRowClose(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(1); }
static void gridColumn(float x) { GAME_FN(void (*)(float), RVA_COLUMN_X)(x); }
#endif

// One item of a pick grid: its id and text.
typedef struct { int id; char text[24]; int selected; const char *tip; } PickItem;

// Draws `count` items in rows of `columns`: a list to pick one item from.
static void drawPickGrid(const PickItem *items, int count, int columns, float width) {
    char text[32];
    for (int i = 0; i < count; i += columns) {
        gridRowOpen();
        for (int c = 0; c < columns && i + c < count; c++) {
            const PickItem *item = &items[i + c];
            snprintf(text, sizeof text, item->selected ? "[%s]" : "%s", item->text);
            gridColumn(width * (float)c);
            if (item->tip) {
                greyedButton(item->id, text);
                tooltip(item->tip);
            } else {
                textButton(item->id, text);
            }
        }
        gridRowClose();
    }
}

// The keys the list offers with `modifier`.
static int pickKeyList(int modifier, int *keys, int groupEnd[3]) {
    int n = 0;
    for (int c = 'a'; c <= 'z'; c++) keys[n++] = modifier + c;
    if (!modifier)
        for (int c = 'A'; c <= 'Z'; c++) keys[n++] = c;
    groupEnd[0] = n;
    if (modifier != KEY_CTRL_SHIFT)
        for (int c = '!'; c < 127; c++)
            if (!isalpha(c) && keyValueIsValid(modifier + c)) keys[n++] = modifier + c;
    groupEnd[1] = n;
    if (modifier != KEY_CTRL_SHIFT)
        for (int i = 0; i < KEY_NAME_COUNT; i++)
            if (KEY_NAMES[i].key != KEY_ESC && keyValueIsValid(modifier + KEY_NAMES[i].key)) keys[n++] = modifier + KEY_NAMES[i].key;
    groupEnd[2] = n;
    return n;
}

// The tooltip of a key that `holder` holds, in the key list: "Used by End turn".
static void keyHeldTip(const ModSetting *holder, char *out, size_t size) {
    snprintf(out, size, holder->fixed ? "Used by %s, which can't change" : "Used by %s", holder->label);
}

// Asks whether to move key `key` from `holder` to `s`; a fixed holder's key can't move.
static int askKeyMove(ModSetting *holder, const ModSetting *s, int key) {
    char name[24], text[400];
    keyValueName(key, name, sizeof name);
    writeLog("setting_key_held", "mod=%s name=%s key=%d fixed=%d", holder->mod, holder->name, key, holder->fixed);
    if (holder->fixed) {
        snprintf(text, sizeof text, "%s reserved for %s.", name, holder->label);
        messageBox("CoE5SE: Key in use", text, "Back", "", 0, ESCAPE_KEY(), 0);
        return 0;
    }
    snprintf(text, sizeof text, "This is assigned to \"%s\".\nReassign it to \"%s\" now?\nThis could break your hotkeys.", holder->label,
             s->label);
    if (messageBox("CoE5SE: Key in use", text, "Reassign key", "Back", 0, 0, ESCAPE_KEY()) != 1) return 0;
    holder->value = KEY_NONE;
    saveValue(holder);
    writeLog("setting_key_moved", "mod=%s name=%s from=%s key=%d", s->mod, s->name, holder->name, key);
    return 1;
}

// Runs the key list for key setting `s`.
static int keyPickerScreen(const ModSetting *s) {
    static const int MODIFIERS[4] = {0, KEY_CTRL, KEY_ALT, KEY_CTRL_SHIFT};
    static const char *const TAB_NAMES[4] = {"No modifier", "ctrl", "alt", "ctrl+shift"};
    static const char *const GROUP_TITLES[3] = {"Letters", "Digits and punctuation", "Other keys"};
    static const struct { int columns; float width; } GROUP_GRID[3] = {{13, 34.0f}, {13, 34.0f}, {7, 64.0f}};
    int tab = 0, keys[PICK_KEYS_MAX], groupEnd[3];
    for (int t = 0; t < 4; t++)
        if (s->value != KEY_NONE && keyModifier(s->value) == MODIFIERS[t]) tab = t;
    PickItem items[PICK_KEYS_MAX];
    static char tips[PICK_KEYS_MAX][SETTING_TEXT_MAX + 32];
    char now[24], line[120], defaultTip[SETTING_TEXT_MAX + 32];
    for (;;) {
        int count = pickKeyList(MODIFIERS[tab], keys, groupEnd);
        uiScreenBegin(s->label);
        screenWidth(PICK_WIDTH);
        if (s->value == KEY_NONE) snprintf(now, sizeof now, "(none)");
        else keyValueName(s->value, now, sizeof now);
        snprintf(line, sizeof line, "Now: %s", now);
        api->ui->sectionBegin(line);
        char defaultName[24];
        keyValueName(s->defaultValue, defaultName, sizeof defaultName);
        snprintf(line, sizeof line, "The mod's default is %s. Pick a key, or Back to keep this one.", defaultName);
        api->ui->text(0, line);
        api->ui->text(0, "A greyed key is used by another line. Point at it to see which.");
        PickItem tabs[4];
        for (int t = 0; t < 4; t++) {
            tabs[t].id = PICK_ID_TAB + t;
            tabs[t].selected = t == tab;
            tabs[t].tip = NULL;
            snprintf(tabs[t].text, sizeof tabs[t].text, "%s", TAB_NAMES[t]);
        }
        drawPickGrid(tabs, 4, 4, 100.0f);
        api->ui->sectionEnd();
        for (int g = 0, from = 0; g < 3; from = groupEnd[g++]) {
            if (groupEnd[g] == from) continue;
            for (int i = from; i < groupEnd[g]; i++) {
                items[i].id = PICK_ID_KEY + i;
                items[i].selected = keys[i] == s->value;
                const ModSetting *holder = keyHolder(s, keys[i]);
                if (holder) keyHeldTip(holder, tips[i], sizeof tips[i]);
                items[i].tip = holder ? tips[i] : NULL;
                keyValueName(MODIFIERS[tab] ? keyBase(keys[i]) : keys[i], items[i].text, sizeof items[i].text);
            }
            api->ui->sectionBegin(GROUP_TITLES[g]);
            drawPickGrid(items + from, groupEnd[g] - from, GROUP_GRID[g].columns, GROUP_GRID[g].width);
            api->ui->sectionEnd();
        }
        api->ui->buttonRow();
        api->ui->text(PICK_ID_NONE, "No key");
        api->ui->tooltip("The setting has no key.");
        const ModSetting *defaultHolder = keyHolder(s, s->defaultValue);
        if (defaultHolder) {
            keyHeldTip(defaultHolder, defaultTip, sizeof defaultTip);
            greyedButton(PICK_ID_DEFAULT, "Default");
            api->ui->tooltip(defaultTip);
        } else {
            api->ui->text(PICK_ID_DEFAULT, "Default");
            api->ui->tooltip("The mod's default key.");
        }
        api->ui->text(PICK_ID_BACK, "Back");
        api->ui->bindKey(PICK_ID_BACK, UI_KEY_ESCAPE);
        int id = api->ui->wait();
        if (id == PICK_ID_BACK) return -1;
        if (id == PICK_ID_NONE) return KEY_NONE;
        if (id >= PICK_ID_TAB && id < PICK_ID_TAB + 4) tab = id - PICK_ID_TAB;
        int picked = id == PICK_ID_DEFAULT ? s->defaultValue : id >= PICK_ID_KEY && id < PICK_ID_KEY + count ? keys[id - PICK_ID_KEY] : -1;
        if (picked < 0) continue;
        ModSetting *holder = keyHolder(s, picked);
        if (!holder || askKeyMove(holder, s, picked)) return picked;
    }
}

// Handles a click on one of the page's settings (drawPageBoxes) and saves the new value.
static int clickIniSetting(int id, ModSetting **page, int count) {
    if (id >= ID_ONOFF && id < ID_ONOFF + count && !page[id - ID_ONOFF]->optionCount && !page[id - ID_ONOFF]->isKey) {
        saveValue(page[id - ID_ONOFF]);
        return 1;
    }
    if (id >= ID_CHOICE && id < ID_CHOICE + count * SETTING_OPTIONS_MAX) {
        ModSetting *s = page[(id - ID_CHOICE) / SETTING_OPTIONS_MAX];
        int k = (id - ID_CHOICE) % SETTING_OPTIONS_MAX;
        if (k >= s->optionCount) return 0;
        s->value = k;
        saveValue(s);
        return 1;
    }
    if (id >= ID_KEY && id < ID_KEY + count && page[id - ID_KEY]->isKey) {
        ModSetting *s = page[id - ID_KEY];
        int picked = keyPickerScreen(s);
        if (picked < 0 || picked == s->value) return 1;
        s->value = picked;
        saveValue(s);
        return 1;
    }
    return 0;
}

// Draws the "Settings left out" box of a mod's page.
static void drawLeftOut(const char *mod) {
    int open = 0;
    char tip[400];
    for (int i = 0; i < badLineCount; i++) {
        if (_stricmp(badLines[i].mod, mod)) continue;
        if (!open) { api->ui->sectionBegin("Settings left out"); open = 1; }
        api->ui->text(0, badLines[i].text);
        snprintf(tip, sizeof tip, "%s\n\nIn %.87s. Read again on leaving the Mods screen.",
                 badLines[i].source, badLines[i].file);
        api->ui->tooltip(tip);
    }
    drawPluginRejections(mod, &open);
    if (open) api->ui->sectionEnd();
}

// The settings of `mod` that Reset to defaults resets: declared, kept, and not fixed.
static int resettableSettings(const char *mod, int *count) {
    int changed = 0;
    *count = 0;
    for (int i = 0; i < settingCount; i++) {
        const ModSetting *s = settings[i];
        if (!s->declared || s->rejected || s->fixed || _stricmp(s->mod, mod)) continue;
        (*count)++;
        changed += s->value != s->defaultValue;
    }
    return changed;
}

// Sets every setting of `mod`, on all its pages, back to its default and deletes its saved value.
static void resetModToDefaults(const char *mod) {
    int changed = 0;
    for (int i = 0; i < settingCount; i++) {
        ModSetting *s = settings[i];
        if (!s->declared || s->rejected || s->fixed || _stricmp(s->mod, mod)) continue;
        changed += s->value != s->defaultValue;
        s->value = s->defaultValue;
        forgetValue(s);
    }
    writeLog("settings_reset", "mod=%s changed=%d", mod, changed);
    resolveKeyConflicts(mod);
}

// Draws the page's Reset to defaults, greyed out when every setting is at its default.
static void drawResetButton(const char *mod) {
    int count;
    if (!resettableSettings(mod, &count)) {
        if (!count) return;
        greyedButton(0, "Reset to defaults");
        api->ui->tooltip("Every setting is already at its default.");
        return;
    }
    api->ui->text(ID_RESET, "Reset to defaults");
    api->ui->tooltip("Sets every setting of this mod, on all its pages, back to the mod's default.");
}

// Asks before Reset to defaults, then resets.
static void clickResetButton(const ListedMod *m) {
    int count;
    if (!resettableSettings(m->mod, &count)) return;
    char text[160];
    snprintf(text, sizeof text, "Reset to defaults: %.79s?", m->label);
    if (messageBox("CoE5SE: Reset to defaults", text, "Yes", "No", 0, 0, 0) == 1) resetModToDefaults(m->mod);
}

// Runs one mod's pages.
static void modScreen(const ListedMod *m, const char *subPage) {
    static ModSetting **page;
    static PageBox *boxes;
    static int pageCapacity, boxCapacity;
    const ModPage *subPages[SETTING_PAGES_MAX];
    int pageNumber = 0;
    char title[200];
    for (;;) {
        finishReading();
        const ModPage *open = subPage ? findModPage(m->mod, subPage) : NULL;
        if (subPage && !open) return;
        int count = subPageSettings(m->mod, subPage ? subPage : "", &page, &pageCapacity);
        int subCount = subPage ? 0 : modPagesOf(m->mod, subPages);
        int screenLines = !subPage && m->plugin >= 0 ? pluginPageLines(m->mod) : 0;
        int boxCount = layoutPageBoxes(page, count, subCount, screenLines, &boxes, &boxCapacity);
        int pages = boxCount ? boxes[boxCount - 1].page + 1 : 1;
        if (pageNumber >= pages) pageNumber = pages - 1;
        int n = open ? snprintf(title, sizeof title, "%.79s: %.56s", m->label, open->label)
                     : snprintf(title, sizeof title, "%.79s", m->label);
        if (pages > 1) snprintf(title + n, sizeof title - (size_t)n, ", page %d of %d", pageNumber + 1, pages);
        api->ui->screenBegin(title);
        if (pageNumber == 0 && !subPage) drawLeftOut(m->mod);
        drawPageBoxes(page, subPages, boxes, boxCount, pageNumber, subPage ? -1 : m->plugin);
        api->ui->buttonRow();
        if (pageNumber > 0) {
            api->ui->text(ID_PAGE_PREV, "Previous page");
            bindHotkey(ID_PAGE_PREV, KEY_PAGE_UP);
        }
        if (pageNumber < pages - 1) {
            api->ui->text(ID_PAGE_NEXT, "Next page");
            bindHotkey(ID_PAGE_NEXT, KEY_PAGE_DOWN);
        }
        if (!subPage) drawResetButton(m->mod);
        api->ui->text(ID_MS_BACK, "Back");
        api->ui->bindKey(ID_MS_BACK, UI_KEY_ESCAPE);
        int id = api->ui->wait();
        if (id == ID_MS_BACK) return;
        if (id == ID_RESET && !subPage) clickResetButton(m);
        else if (id == ID_PAGE_PREV && pageNumber > 0) pageNumber--;
        else if (id == ID_PAGE_NEXT && pageNumber < pages - 1) pageNumber++;
        else if (id >= ID_SUBPAGE && id < ID_SUBPAGE + subCount) {
            char name[SETTING_NAME_MAX + 1];
            snprintf(name, sizeof name, "%s", subPages[id - ID_SUBPAGE]->name);
            modScreen(m, name);
        } else if (id && !clickIniSetting(id, page, count) && !subPage) {
            api->settingsPluginClick(id);
        }
    }
}

// Adds `mod` to the list unless it is there, and returns its row.
static ListedMod *listedRow(ListedMod **list, int *count, int *capacity, const char *mod) {
    for (int k = 0; k < *count; k++)
        if (!_stricmp((*list)[k].mod, mod)) return &(*list)[k];
    if (!growList((void **)list, capacity, *count + 1, sizeof (*list)[0])) return NULL;
    ListedMod *row = &(*list)[(*count)++];
    snprintf(row->mod, sizeof row->mod, "%s", mod);
    modDisplayName(mod, row->label, sizeof row->label);
    row->plugin = -1;
    return row;
}

// Fills `list` with every mod that has a page, and returns its length.
static int listMods(ListedMod **list, int *capacity) {
    finishReading();
    int count = 0;
    if (!growList((void **)list, capacity, 1, sizeof (*list)[0])) return 0;
    snprintf((*list)[0].mod, sizeof (*list)[0].mod, "CoE5SE");
    snprintf((*list)[0].label, sizeof (*list)[0].label, "CoE5SE");
    (*list)[0].plugin = -2;
    count = 1;
    for (int i = 0; i < settingCount; i++)
        if (settings[i]->declared && modEnabledByName(settings[i]->mod)) listedRow(list, &count, capacity, settings[i]->mod);
    for (int i = 0; i < badLineCount; i++)
        if (modEnabledByName(badLines[i].mod)) listedRow(list, &count, capacity, badLines[i].mod);
    int plugins = api->settingsPluginCount();
    for (int p = 0; p < plugins; p++) {
        ListedMod *row = listedRow(list, &count, capacity, api->settingsPluginName(p));
        if (row && row->plugin != -2) row->plugin = p;
    }
    for (int a = 2; a < count; a++)
        for (int b = a; b > 1 && _stricmp((*list)[b - 1].label, (*list)[b].label) > 0; b--) {
            ListedMod t = (*list)[b];
            (*list)[b] = (*list)[b - 1];
            (*list)[b - 1] = t;
        }
    return count;
}

// After the player leaves a mod's page: offers a restart when a setting needs one.
static void offerRestartForSettings(void) {
    int changed = 0;
    for (int i = 0; i < settingCount; i++) {
        const ModSetting *s = settings[i];
        if (s->restart && s->declared && !s->rejected && s->value != s->launchValue && s->value != s->promptedValue &&
            modEnabledByName(s->mod))
            changed++;
    }
    if (!changed) return;
    writeLog("settings_need_restart", "count=%d", changed);
    if (messageBox("CoE5SE: Restart needed", "Setting changed. Takes effect after restart.\nRestart now?", "Yes", "No", 0, 0,
                   0) == 1)
        relaunchGame("setting_needs_restart");
    for (int i = 0; i < settingCount; i++) settings[i]->promptedValue = settings[i]->value;
}

// Runs the main menu's Mod Settings screen.
static void settingsScreen(void) {
    static ListedMod *list;
    static int capacity;
    for (;;) {
        int count = listMods(&list, &capacity);
        api->ui->screenBegin("Mod Settings");
        api->ui->listBegin();
        for (int k = 0; k < count; k++) api->ui->listRow(ID_MOD + k, list[k].label);
        api->ui->listEnd();
        api->ui->buttonRow();
        api->ui->text(ID_MS_BACK, "Back");
        api->ui->bindKey(ID_MS_BACK, UI_KEY_ESCAPE);
        int id = api->ui->wait();
        if (id == ID_MOD) coe5sePageScreen();
        else if (id > ID_MOD && id < ID_MOD + count) {
            ListedMod row = list[id - ID_MOD];
            modScreen(&row, NULL);
            offerRestartForSettings();
        }
        if (id == ID_MS_BACK) return;
    }
}

// ---- The pop-up for local mods -------------------------------------------------------------------
// The pop-up of a local mod's left-out setting lines and plugin calls.
#define POPUP_LINES 16
static unsigned int shownLeftOutHash, checkedLeftOutVersion;

// Appends mod `mod`'s left-out lines and calls to the pop-up's text.
static int appendLeftOut(const char *mod, char *text, size_t size, size_t *n, int *room) {
    int count = 0;
    char label[80];
    modDisplayName(mod, label, sizeof label);
    if (*room > 0) appendText(text, size, n, "\n%s\n", label);
    for (int j = 0; j < badLineCount; j++) {
        if (_stricmp(badLines[j].mod, mod)) continue;
        count++;
        if ((*room)-- > 0) appendText(text, size, n, "%s\n", badLines[j].text);
    }
    for (int j = 0; j < entryCount; j++) {
        if (entries[j]->type != ENTRY_LEFT_OUT || _stricmp(entries[j]->mod, mod)) continue;
        count++;
        if ((*room)-- > 0) appendText(text, size, n, "%s\n", entries[j]->problem);
    }
    return count;
}

// 1 if `mod` is enabled, in the local mods folder, and not yet in the pop-up.
static int popupTakesMod(const char *mod, int firstIndex, int inEntries) {
    for (int d = 0; d < (inEntries ? badLineCount : firstIndex); d++)
        if (!_stricmp(badLines[d].mod, mod)) return 0;
    for (int d = 0; inEntries && d < firstIndex; d++)
        if (entries[d]->type == ENTRY_LEFT_OUT && !_stricmp(entries[d]->mod, mod)) return 0;
    return modEnabledByName(mod) && isInLocalModsFolder(mod);
}

static void popupSettingErrors(void) {
    finishReading();
    if (checkedLeftOutVersion == leftOutVersion) return;
    checkedLeftOutVersion = leftOutVersion;
    static char text[POPUP_TEXT_SIZE];
    size_t n = 0;
    int lines = 0, mods = 0, room = POPUP_LINES;
    appendText(text, sizeof text, &n, "Settings left out (rule broken):\n");
    for (int i = 0; i < badLineCount; i++)
        if (popupTakesMod(badLines[i].mod, i, 0)) {
            lines += appendLeftOut(badLines[i].mod, text, sizeof text, &n, &room);
            mods++;
        }
    for (int i = 0; i < entryCount; i++)
        if (entries[i]->type == ENTRY_LEFT_OUT && popupTakesMod(entries[i]->mod, i, 1)) {
            lines += appendLeftOut(entries[i]->mod, text, sizeof text, &n, &room);
            mods++;
        }
    if (!lines) {
        shownLeftOutHash = 0;
        return;
    }
    if (lines > POPUP_LINES) appendText(text, sizeof text, &n, "...and %d more.\n", lines - POPUP_LINES);
    appendText(text, sizeof text, &n, "\nFull list on each mod's page in Mod Settings.");
    unsigned int hash = 2166136261u;
    for (const char *c = text; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
    if (hash == shownLeftOutHash) return;
    shownLeftOutHash = hash;
    writeLog("settings_left_out_popup", "mods=%d lines=%d", mods, lines);
    okBox("CoE5SE: Mod Settings", text);
}

// Wraps each mod pass.
static void (*nextModsRun)(int);
static void modsRunHook(int level) {
    loadLuaCommandsFiles();
    unsigned int spellPasses = spellPassCount();
    clearLuaPlaneTints(level);
    planeWordsBeforePass(level);
    terrainLooksBeforePass(level);
    weaponTraitsBeforePass(level);
    noExplodeBeforePass(level);
    nextModsRun(level);
    finishReading();
    spellsAfterPass(level, spellPasses);
    applyLoadedSavePlanes(level);
    planeWordsAfterPass(level);
    ritualWordsAfterPass(level);
    noExplodeAfterPass(level);
    terrainLooksAfterPass(level);
    if (level != 3) popupSettingErrors();
}
