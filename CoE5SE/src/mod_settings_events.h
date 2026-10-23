// Mod Settings in events: +setting and -setting.

// ---- +setting and -setting in events -------------------------------------------------------------
// 1 if the event line is "+setting ..." or "-setting ...".
static int isSettingLine(const char *line) {
    return (line[0] == '+' || line[0] == '-') && !strncmp(line + 1, "setting", 7) && (line[8] == ' ' || line[8] == '\t');
}

// The value events and scripts see.
static int settingValueNow(const ModSetting *s) {
    return isNetworkGame() || s->rejected ? s->defaultValue : s->value;
}

// Every line of a .c5m, on its way to the game's event runner.
static void eventLineHook(const char *line, int level) {
    const char *file = (const char *)(gameBase + RVA_EVENT_FILE), *slash = strrchr(file, '/'), *back = strrchr(file, '\\');
    if (back > slash) slash = back;
    const char *base = slash ? slash + 1 : file;
    size_t len = strlen(base), c5m = sizeof MOD_FILE_SUFFIX - 1, side = sizeof SIDECAR_SUFFIX - 1;
    char mod[MOD_NAME_MAX] = "", name[SETTING_NAME_SIZE];
    if (len > c5m && !_stricmp(base + len - c5m, MOD_FILE_SUFFIX)) snprintf(mod, sizeof mod, "%.*s", (int)(len - c5m), base);
    else if (len > side && !_stricmp(base + len - side, SIDECAR_SUFFIX)) snprintf(mod, sizeof mod, "%.*s", (int)(len - side), base);
    noteModLine(mod, level);
    line = planeNameEventLine(line, level, mod);
    if (!line) return;
    if (eventLogicLine(line, level)) return;
    if (!isSettingLine(line)) {
        eventFindersLineBegin();
        nextEventLine(line, level);
        eventFindersLineEnd();
        eventLogicAfterLine(line, level);
        return;
    }
    int sign = line[0] == '+' ? 1 : -1;
    const char *word = sign > 0 ? "+setting" : "-setting";
    const char *p = line + 8;
    int wanted = 0;
    readWord(&p, name, sizeof name);
    int hasIndex = readInt(&p, &wanted);
    if (level == 1) {
        if (!mod[0] || G32(RVA_EVENT_STATE) < 1 || !reading.active || reading.disabled) return;
        if (!growList((void **)&reading.refs, &reading.refCapacity, reading.refCount + 1, sizeof reading.refs[0])) return;
        SettingRef *r = &reading.refs[reading.refCount++];
        memset(r, 0, sizeof *r);
        r->line = G32(RVA_MOD_LINE);
        r->hasIndex = hasIndex;
        r->index = wanted;
        r->trailing = !isLineEnd(p);
        snprintf(r->word, sizeof r->word, "%s", word);
        snprintf(r->name, sizeof r->name, "%s", name);
        return;
    }
    if (G32(RVA_EVENT_STATE) < 1 || level == 4) return;
    if (isNetworkGame()) {
        G32(RVA_EVENT_STATE) = -1;
        return;
    }
    finishReading();
    const ModSetting *s = mod[0] ? findDeclaredSetting(mod, name) : NULL;
    if (!s) {
        refuseC5mLine(word, mod[0] ? mod : "-", "unknown_setting", "name=%s file=%s", name, file);
        G32(RVA_EVENT_STATE) = -1;
        return;
    }
    int max = s->optionCount ? s->optionCount - 1 : 1;
    if (hasIndex && (s->isKey ? !keyValueIsValid(wanted) : wanted < 0 || wanted > max))
        refuseC5mLine(word, mod, "index_out_of_range", "name=%s index=%d max=%d", name, wanted, max);
    int value = settingValueNow(s);
    int holds = hasIndex ? value == wanted : value != 0;
    if (holds != (sign > 0)) G32(RVA_EVENT_STATE) = -1;
}

// Used instead of eventLineHook when the player declined mods on an unknown game build.
static void declinedEventLineHook(const char *line, int level) {
    if (isSettingLine(line)) refuseC5mLine(line[0] == '+' ? "+setting" : "-setting", "-", "plugins_declined", "line=%s", line);
    else if (isEventLogicWord(line)) refuseC5mLine("or", "-", "plugins_declined", "line=%s", line);
    else if (declinedPlaneEventLine(line)) refuseC5mLine("plane", "-", "plugins_declined", "line=%s", line);
    else nextEventLine(line, level);
}

// Adds the +setting answer, the mod pass wrapper, and the Mod Settings button.
static void modSettingsInit(void) {
    int ok = wrapSite(SITE_EVENT_LINE, (void *)eventLineHook, (void **)&nextEventLine) +
             wrapSite(SITE_MODS_RUN, (void *)modsRunHook, (void **)&nextModsRun) +
             api->addMainMenuButtonAfter("Mod Settings", settingsScreen, MAIN_MENU_MODS);
    writeLog("mod_settings_installed", "ok=%d of 3", ok);
}
