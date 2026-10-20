// Mods' spell paths: new magic paths with their own names, and new names for the game's paths.

#define VANILLA_SPELL_PATH_COUNT  70
#define SPELL_PATH_NAME_MAX       32
#define NEWSPELLPATH_WORD         "newspellpath"
#define SELECTSPELLPATH_WORD      "selectspellpath"
#define SPELL_PATH_LINE_MAX       1024

static struct {
    int state;
    int writable;
    const char *shipped[VANILLA_SPELL_PATH_COUNT];
    char (*unnamed)[16];
    int added;
    int changed;
    int selected, selectedMod;
    char line[SPELL_PATH_LINE_MAX];
} spellPaths = {.selected = -1};

// The name table the game reads.
static const char **spellPathNames(void) {
    if (pathTakeover.names) return pathTakeover.names;
    return spellPathsExpanded ? spellPathCopy : (const char **)(gameBase + RVA_SPELL_PATH_NAMES);
}

// Whether a mod pass has looked at the paths yet.
static int spellPathsInUse(void) { return spellPaths.state != 0 || spellPaths.added != 0; }

// Whether the name table is as the game ships it: named 1 to 69, NULL past.
static int spellPathTableReady(void) {
    if (spellPaths.state) return spellPaths.state > 0;
    const char **names = spellPathNames();
    int size = spellPathTableSize();
    for (int p = 1; p < size; p++)
        if ((p < VANILLA_SPELL_PATH_COUNT) != (names[p] != NULL)) {
            spellPaths.state = -1;
            writeLog("spell_path_table_unexpected", "path=%d", p);
            return 0;
        }
    if (size > VANILLA_SPELL_PATH_COUNT) {
        spellPaths.unnamed = calloc((size_t)(size - VANILLA_SPELL_PATH_COUNT), sizeof *spellPaths.unnamed);
        if (!spellPaths.unnamed) {
            spellPaths.state = -1;
            writeLog("spell_path_table_unexpected", "reason=out_of_memory paths=%d", size);
            return 0;
        }
    }
    memcpy(spellPaths.shipped, names, VANILLA_SPELL_PATH_COUNT * sizeof(const char *));
    for (int p = VANILLA_SPELL_PATH_COUNT; p < size; p++)
        snprintf(spellPaths.unnamed[p - VANILLA_SPELL_PATH_COUNT], sizeof spellPaths.unnamed[0], "Path %d", p);
    spellPaths.state = 1;
    spellPaths.changed = 1;
    return 1;
}

// Whether CoE5SE can write the name table.
static int spellPathTableWritable(void) {
    if (spellPathsExpanded || pathTakeover.names) return 1;
    if (!spellPaths.writable)
        spellPaths.writable = makeGameDataWritable(spellPathNames(), GAME_SPELL_PATH_COUNT * sizeof(const char *)) ? 1 : -1;
    return spellPaths.writable > 0;
}

// Puts back the game's names and forgets every mod's path.
static void resetSpellPaths(const char *reason) {
    spellPaths.selected = -1;
    if (!spellPaths.state) spellPathTableReady();
    if (!spellPaths.changed && !spellPaths.added) return;
    if (!spellPathTableReady() || !spellPathTableWritable()) return;
    const char **names = spellPathNames();
    memcpy(names, spellPaths.shipped, VANILLA_SPELL_PATH_COUNT * sizeof(const char *));
    for (int p = VANILLA_SPELL_PATH_COUNT; p < spellPathTableSize(); p++) names[p] = spellPaths.unnamed[p - VANILLA_SPELL_PATH_COUNT];
    if (spellPaths.added) writeLog("spell_paths_reset", "removed=%d reason=%s", spellPaths.added, reason);
    spellPaths.added = 0;
    spellPaths.changed = 0;
}

// The number of the path named `name`, ignoring case.
static int spellPathByName(const char *name) {
    if (!name[0] || !spellPathTableReady()) return -1;
    const char **names = spellPathNames();
    for (int p = 1; p < spellPathTableSize(); p++) {
        if (p >= VANILLA_SPELL_PATH_COUNT && names[p] == spellPaths.unnamed[p - VANILLA_SPELL_PATH_COUNT]) continue;
        if (names[p] && !_stricmp(names[p], name)) return p;
    }
    return -1;
}

// Reads the quoted path name of a path word's line into `name`.
static int readSpellPathName(const char *word, const char *args, int mod, char *name, size_t size) {
    const char *p = args;
    int len = readSpellQuoted(&p, name, size);
    if (len <= 0 || len > SPELL_PATH_NAME_MAX) {
        refuseC5mLine(word, spellModName(mod), len <= 0 ? "no_name" : "name_too_long", "line=%d text=%.60s", G32(RVA_MOD_LINE), args);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), len <= 0 ? "%s needs a name in quotes" : "%s: a name longer than the limit of %d",
                         word, SPELL_PATH_NAME_MAX);
        return 0;
    }
    return 1;
}

// Whether the table can take a path word's line now.
static int spellPathWordTakes(const char *word, int mod) {
    const char *reason = !spellPathTableReady() ? "table_unexpected" : !spellPathTableWritable() ? "table_not_writable" : NULL;
    if (!reason) return 1;
    refuseC5mLine(word, spellModName(mod), reason, "line=%d", G32(RVA_MOD_LINE));
    noteSpellProblem(mod, G32(RVA_MOD_LINE), "CoE5SE couldn't change the spell path names");
    return 0;
}

// A path refused in a network game for lack of a free path.
#define SPELL_PATHS_SHORT_TITLE "CoE5SE: Spell paths left out"

// The game's message box for spellPathsShortInNetworkGame.
static int showSpellPathsShortBox(const char *text, int restartable) {
    if (!restartable) {
        messageBox(SPELL_PATHS_SHORT_TITLE, text, "Ok", "", 0, 0, 0);
        return 0;
    }
    return messageBox(SPELL_PATHS_SHORT_TITLE, text, "Restart", "Play on", 0, 0, 0) == 1;
}
static int (*spellPathsShortBox)(const char *text, int restartable) = showSpellPathsShortBox;
static int spellPathsShortTold;

// The pop-up's text, into `text`.
static void spellPathsShortText(int dllMod, char *text, size_t size) {
    int used = snprintf(text, size, "More spell paths than this copy supports.\n"
                                    "Spells would not match other players'.\n\n");
    if (used < 0 || (size_t)used >= size) return;
    if (dllMod < 0)
        snprintf(text + used, size - (size_t)used, "Same mods required for all players, with the same\nCoE5SE Spell Paths Extension.");
    else
        snprintf(text + used, size - (size_t)used, "%.60s on in this game. DLL loads at startup only.\nRestart, then rejoin?",
                 modName(dllMod));
}

static void spellPathsShortInNetworkGame(int mod, const char *name) {
    if (spellPathsShortTold) return;
    spellPathsShortTold = 1;
    int dllMod = dllModOnSinceSnapshot();
    int atPluginCap = pathTakeover.names && spellPathCount() == pathTakeover.count;
    writeLog("spell_paths_short_in_network_game", "mod=%s name=%s paths=%d at_plugin_cap=%d dll_mod_not_running=%s", spellModName(mod), name,
             spellPathCount(), atPluginCap, dllMod >= 0 ? modName(dllMod) : "-");
    if (atPluginCap) return;
    char text[POPUP_TEXT_SIZE];
    spellPathsShortText(dllMod, text, sizeof text);
    int restart = spellPathsShortBox(text, dllMod >= 0);
    writeLog("spell_paths_short_choice", "choice=%s", dllMod < 0 ? "ok" : restart ? "restart" : "play_on");
    if (!restart) return;
    GAME_FN(void (*)(void), RVA_WRITE_CONFIG)();
    relaunchGame("spell_paths_short_in_network_game");
}

// newspellpath "<name>": a new path, or the path that already has that name.
static void newspellpathLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    clearParserCurrentRecords();
    spellPaths.selected = -1;
    char name[SPELL_PATH_NAME_MAX + 2];
    if (!readSpellPathName(NEWSPELLPATH_WORD, args, mod, name, sizeof name) || !spellPathWordTakes(NEWSPELLPATH_WORD, mod)) return;
    int existing = spellPathByName(name);
    if (existing > 0) {
        writeLog("spell_path_shared", "mod=%s number=%d name=%s line=%d", spellModName(mod), existing, name, G32(RVA_MOD_LINE));
        return;
    }
    int number = VANILLA_SPELL_PATH_COUNT + spellPaths.added;
    if (number >= spellPathCount()) {
        refuseC5mLine(NEWSPELLPATH_WORD, spellModName(mod), "no_free_path", "line=%d name=%s", G32(RVA_MOD_LINE), name);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "no free spell path for %s", name);
        if (isNetworkGame()) spellPathsShortInNetworkGame(mod, name);
        return;
    }
    const char *kept = keptSpellName(name);
    if (!kept) {
        refuseC5mLine(NEWSPELLPATH_WORD, spellModName(mod), "out_of_memory", "line=%d name=%s", G32(RVA_MOD_LINE), name);
        return;
    }
    spellPathNames()[number] = kept;
    spellPaths.added++;
    spellPaths.changed = 1;
    writeLog("spell_path_added", "mod=%s number=%d name=%s line=%d", spellModName(mod), number, name, G32(RVA_MOD_LINE));
}

// selectspellpath <nbr> or selectspellpath "<name>": picks the path the next `name` line renames.
static void selectspellpathLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    clearParserCurrentRecords();
    spellPaths.selected = -1;
    if (!spellPathWordTakes(SELECTSPELLPATH_WORD, mod)) return;
    const char *p = args;
    skipSpaces(&p);
    long long number = -1;
    char name[SPELL_PATH_NAME_MAX + 2] = "";
    if (*p == '"') {
        if (readSpellQuoted(&p, name, sizeof name) > 0) number = spellPathByName(name);
    } else if (!readSpellNumber(&p, &number) || number < 1 || number >= spellPathCount()) {
        number = -1;
    }
    if (number < 1) {
        refuseC5mLine(SELECTSPELLPATH_WORD, spellModName(mod), name[0] ? "unknown_path" : "bad_path", "line=%d text=%.40s",
                      G32(RVA_MOD_LINE), args);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "%s needs a path number from 1 to %d, or a path's name", SELECTSPELLPATH_WORD,
                         spellPathCount() - 1);
        return;
    }
    spellPaths.selected = (int)number;
    spellPaths.selectedMod = mod;
}

// name "<name>" right after selectspellpath: the path's new name.
static void spellPathNameLine(const char *args, int mod) {
    int number = spellPaths.selected;
    spellPaths.selected = -1;
    char name[SPELL_PATH_NAME_MAX + 2];
    if (!readSpellPathName("name", args, mod, name, sizeof name)) return;
    const char *kept = keptSpellName(name);
    if (!kept) {
        refuseC5mLine("name", spellModName(mod), "out_of_memory", "line=%d name=%s", G32(RVA_MOD_LINE), name);
        return;
    }
    spellPathNames()[number] = kept;
    spellPaths.changed = 1;
    writeLog("spell_path_renamed", "mod=%s number=%d name=%s line=%d", spellModName(mod), number, name, G32(RVA_MOD_LINE));
}

// The game's words that take a path number first, which take a quoted path name through CoE5SE.
static const char *const PATH_FIRST_WORDS[] = {
    "spellweapon", "spellweaponbonus", "spellweapon50", "spellweapon50s", "spellweaponsingle",
    "pathboost", "newspell1", "newspell2", "newspell3",
};

// The words whose path number CoE5SE checks.
static const char *const CHECKED_PATH_WORDS[] = {
    "spellweapon", "spellweaponbonus", "spellweapon50", "spellweapon50s", "spellweaponsingle", "pathboost",
};

// A raw number in CHECKED_PATH_WORDS must be a path from 1 to the last.
static int rawPathNumberTakes(const char *line, size_t len, const char *args, int mod) {
    int checked = 0;
    for (size_t i = 0; i < sizeof CHECKED_PATH_WORDS / sizeof CHECKED_PATH_WORDS[0]; i++)
        if (spellWordIs(line, len, CHECKED_PATH_WORDS[i])) checked = 1;
    const char *p = args;
    long long number = 0;
    if (!checked || !readSpellNumber(&p, &number) || (number >= 1 && number < spellPathCount())) return 1;
    char word[32];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    refuseC5mLine(word, spellModName(mod), "bad_path", "line=%d path=%lld", G32(RVA_MOD_LINE), number);
    noteSpellProblem(mod, G32(RVA_MOD_LINE), "%s needs a path number from 1 to %d, or a path's name", word, spellPathCount() - 1);
    return 0;
}

// Every .c5m line outside events comes here after spellLineHook.
static const char *spellPathLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2) return line;
    const char *args = line + len;
    while (*args == ' ' || *args == '\t') args++;
    if (spellPaths.selected >= 0) {
        if (spellWordIs(line, len, "name") && mod == spellPaths.selectedMod) {
            spellPathNameLine(args, mod);
            return NULL;
        }
        spellPaths.selected = -1;
    }
    if (*args != '"') return rawPathNumberTakes(line, len, args, mod) ? line : NULL;
    int pathWord = 0;
    for (size_t i = 0; i < sizeof PATH_FIRST_WORDS / sizeof PATH_FIRST_WORDS[0]; i++)
        if (spellWordIs(line, len, PATH_FIRST_WORDS[i])) pathWord = 1;
    if (!pathWord) return line;
    char word[32], name[SPELL_PATH_NAME_MAX + 2];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    const char *rest = args;
    int nameLen = readSpellQuoted(&rest, name, sizeof name);
    int number = nameLen > 0 && nameLen <= SPELL_PATH_NAME_MAX ? spellPathByName(name) : -1;
    if (number < 0) {
        refuseC5mLine(word, spellModName(mod), "unknown_path", "line=%d text=%.60s", G32(RVA_MOD_LINE), args);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "%s: no spell path named %.40s", word, name);
        return NULL;
    }
    int n = snprintf(spellPaths.line, sizeof spellPaths.line, "%s %d%s", word, number, rest);
    if (n < 0 || (size_t)n >= sizeof spellPaths.line) {
        refuseC5mLine(word, spellModName(mod), "line_too_long", "line=%d", G32(RVA_MOD_LINE));
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "%s: a line longer than %d characters", word, SPELL_PATH_LINE_MAX - 1);
        return NULL;
    }
    return spellPaths.line;
}

// The same for declinedModCommandHook, when no plugin loaded.
static int declinedSpellPathLine(const char *word, int mod, int level) {
    static int afterSelect, lastMod, lastLevel;
    int skip = afterSelect && mod == lastMod && level == lastLevel && !strcmp(word, "name");
    afterSelect = !strcmp(word, SELECTSPELLPATH_WORD);
    lastMod = mod;
    lastLevel = level;
    return skip;
}

// Adds the words.
static void spellPathWordsInit(void) {
    int words = apiAddModCommand("newspellpath", newspellpathLine) + apiAddModCommand("selectspellpath", selectspellpathLine);
    writeLog("spell_path_words_installed", "words=%d of 2", words);
}
