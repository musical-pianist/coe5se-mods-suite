// The .c5m line hook (modCommandHook): the game's own words, mods' commands, and the word that needs CoE5SE.

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
    if (refusedTableLine(line, len, mod, level)) return;
    if (spellLineHook(line, len, mod, level)) return;
    line = spellPathLineHook(line, len, mod, level);
    if (!line) return;
    line = offsetNameLineHook(line, len, mod, level);
    if (!line) return;
    line = nameListLineHook(line, len, mod, level);
    if (!line) return;
    len = strcspn(line, " \t");
    line = planeWordLineHook(line, len, mod, level);
    if (!line) return;
    len = strcspn(line, " \t");
    line = terrainLooksLineHook(line, len, mod, level);
    if (!line) return;
    if (settingLineHook(line, len, mod, level)) return;
    if (flagOffLineHook(line, len, mod, level)) return;
    weaponTraitLineHook(line, len, level);
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
    int planeLine = declinedPlaneLine(line, len, mod, level) | declinedTerrainLooksLine(line, len, mod, level);
    if (word[0] && (!isGameWord(word) || declinedSpellLine(word, mod, level) || pathName || planeLine || declinedOffsetNameLine(line, len, level) || declinedNameListLine(line, len, level) || declinedFlagOffLine(line))) {
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
