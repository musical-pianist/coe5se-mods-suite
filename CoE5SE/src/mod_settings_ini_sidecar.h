// Mod Settings: the readers of a mod's <ModName>.ini and the settings blocks of its <ModName>.c5se.

// ---- The mod's .ini, <ModName>.ini ----------------------------------------------------------------
// Reads the text of `mod`'s .ini into reading `r`, declaring its settings in file order.
static void readIniText(IniParse *r, const char *text) {
    r->sidecar = 0;
    IniSection s;
    memset(&s, 0, sizeof s);
    if (!strncmp(text, "\xEF\xBB\xBF", 3)) text += 3;
    char raw[INI_LINE_MAX + 1], key[64];
    SettingProblem p;
    for (int line = 1; *text; line++) {
        int tooLong;
        text = copySettingsLine(text, raw, &tooLong);
        if (tooLong) {
            raw[150] = '\0';
            settingProblem(&p, "line_too_long", "line over %d characters", INI_LINE_MAX);
            iniLineProblem(r, &s, line, &p, raw);
            continue;
        }
        const char *at = raw;
        skipSpaces(&at);
        if (!*at || *at == ';' || *at == '#') continue;
        if (*at == '[') {
            finishIniSection(r, &s);
            startIniSection(r, &s, at, line);
            continue;
        }
        const char *eq = strchr(at, '=');
        if (!eq) {
            settingProblem(&p, "not_key_value", "not a [section] or a key = value line");
            iniLineProblem(r, &s, line, &p, at);
            continue;
        }
        size_t keyLen = (size_t)(eq - at);
        while (keyLen && (at[keyLen - 1] == ' ' || at[keyLen - 1] == '\t')) keyLen--;
        snprintf(key, sizeof key, "%.*s", (int)(keyLen < sizeof key ? keyLen : sizeof key - 1), at);
        const char *v = eq + 1;
        skipSpaces(&v);
        readIniKey(r, &s, key, v, line, at);
    }
    finishIniSection(r, &s);
    endSettingsFile(r);
}

// ---- The mod's sidecar, <ModName>.c5se ------------------------------------------------------------
// The sidecar's settings blocks.
static const char *const SETTING_BLOCK_WORDS[] = {"onoff", "choice", "key", "default", "label", "option", "optiontip", "tip",
                                                  "restart", "group", "fixed"};

static int isSettingLine(const char *line);
static void eventFindersLineBegin(void);
static void eventFindersLineEnd(void);

// One line of the open block.
typedef struct {
    int line, hashInText;
    char word[24];
    char args[INI_LINE_MAX + 1];
    char source[160];
} BlockLine;

// The block being read.
static struct {
    int open;
    IniSection s;
    BlockLine *lines;
    int count, capacity;
} sidecarBlock;

static int isSettingBlockStart(const char *word) {
    return !strcmp(word, SETTING_WORD) || !strcmp(word, SETTING_PAGE_WORD) || !strcmp(word, SETTING_HEADER_WORD);
}

static int isSettingBlockWord(const char *word) {
    for (size_t i = 0; i < sizeof SETTING_BLOCK_WORDS / sizeof SETTING_BLOCK_WORDS[0]; i++)
        if (!strcmp(word, SETTING_BLOCK_WORDS[i])) return 1;
    return 0;
}

// The section type of kind line `word` (onoff, choice, or key), or 0 for any other word.
static int settingKindType(const char *word) {
    if (!strcmp(word, "onoff")) return SECTION_ONOFF;
    if (!strcmp(word, "choice")) return SECTION_CHOICE;
    if (!strcmp(word, "key")) return SECTION_KEY;
    return 0;
}

// Reads "quoted text" at *p (spaces first) into `out`, cut to fit, and moves past it.
static int readSidecarText(const char **p, const char *what, char *out, size_t size, SettingProblem *pr) {
    skipSpaces(p);
    if (**p != '"') return settingProblem(pr, "needs_quotes", "%.16s needs its text in quotes: %.16s \"...\"", what, what);
    const char *start = *p + 1, *end = strchr(start, '"');
    if (!end) return settingProblem(pr, "unclosed_quote", "%.16s has no closing quote", what);
    size_t len = (size_t)(end - start);
    snprintf(out, size, "%.*s", (int)(len < size ? len : size - 1), start);
    *p = end + 1;
    return 0;
}

// Reads a name at *p, quoted or not, into `out` (INI_LINE_MAX bytes), and moves past it.
static int readSidecarName(const char **p, char *out, SettingProblem *pr) {
    skipSpaces(p);
    if (**p == '"') return readSidecarText(p, "name", out, INI_LINE_MAX, pr);
    readWord(p, out, INI_LINE_MAX);
    return 0;
}

// Starts a block at the line `word args`, line `line` of the sidecar.
static void startSidecarBlock(IniParse *r, const char *word, const char *args, int line, const char *source, int hashInText) {
    IniSection *s = &sidecarBlock.s;
    memset(s, 0, sizeof *s);
    sidecarBlock.open = 1;
    sidecarBlock.count = 0;
    s->line = line;
    snprintf(s->source, sizeof s->source, "%.150s", source);
    SettingProblem p;
    const char *at = args;
    int bad = 0;
    if (!strcmp(word, SETTING_HEADER_WORD)) {
        s->type = SECTION_HEADER;
        snprintf(s->word, sizeof s->word, "header");
        bad = readSidecarText(&at, "header", s->name, sizeof s->name, &p);
    } else {
        s->type = !strcmp(word, SETTING_PAGE_WORD) ? SECTION_PAGE : SECTION_SETTING;
        snprintf(s->word, sizeof s->word, "%s", s->type == SECTION_PAGE ? "page" : "setting");
        bad = readSidecarName(&at, s->name, &p);
    }
    if (!bad && hashInText) bad = settingProblem(&p, "hash_in_text", "text can't hold #: the game reads it as a comment");
    if (!bad && !isLineEnd(at)) bad = settingProblem(&p, "trailing_text", "text after the %s", s->type == SECTION_HEADER ? "title" : "name");
    if (bad) iniSectionProblem(r, s, &p);
}

// Which IniKey a block word is (not a kind word), for iniKeyLine and storeIniKey.
static IniKey sidecarKeyNamed(const char *word) {
    int option;
    if (!strcmp(word, "option")) return INI_KEY_OPTION;
    if (!strcmp(word, "optiontip")) return INI_KEY_OPTION_TIP;
    return iniKeyNamed(word, &option);
}

// Reads line `b` of setting or page block `s`.
static int readSidecarBlockLine(IniSection *s, const BlockLine *b, SettingProblem *p) {
    const char *at = b->args;
    if (s->type == SECTION_HEADER) return settingProblem(p, "unknown_key", "%s: a header has only its title line", b->word);
    if (s->type == SECTION_PAGE && strcmp(b->word, "label")) return settingProblem(p, "unknown_key", "%s: a page has only a label line", b->word);
    IniKey k = sidecarKeyNamed(b->word);
    if (k == INI_KEY_OPTION && s->type != SECTION_CHOICE) return settingProblem(p, "unknown_key", "option is for a choice");
    if (k == INI_KEY_OPTION_TIP && s->type != SECTION_CHOICE) return settingProblem(p, "unknown_key", "optiontip is for a choice");
    if (k == INI_KEY_TIP && s->type == SECTION_CHOICE) return settingProblem(p, "unknown_key", "a choice has no tip: give each option its own with optiontip");
    if ((k == INI_KEY_GROUP || k == INI_KEY_FIXED) && s->type != SECTION_KEY) return settingProblem(p, "unknown_key", "%s is for a key", b->word);
    int option = -1;
    if (k == INI_KEY_OPTION || k == INI_KEY_OPTION_TIP) {
        if (!readInt(&at, &option)) return settingProblem(p, "not_a_number", "%s needs a number, then its text in quotes", b->word);
        if (option < 0 || option >= SETTING_OPTIONS_MAX)
            return settingProblem(p, "too_many_options", "%s %d: a choice has options 0 to %d", b->word, option, SETTING_OPTIONS_MAX - 1);
    }
    int *seen = iniKeyLine(s, k, option);
    if (!seen) return settingProblem(p, "unknown_key", "%s doesn't go in this block", b->word);
    if (*seen && option >= 0) return settingProblem(p, "duplicate_key", "%s %d is there twice (line %d)", b->word, option, *seen);
    if (*seen) return settingProblem(p, "duplicate_key", "%s is there twice (line %d)", b->word, *seen);
    *seen = b->line;
    if (b->hashInText) return settingProblem(p, "hash_in_text", "text can't hold #: the game reads it as a comment");
    char value[INI_LINE_MAX + 1];
    if (k == INI_KEY_RESTART || k == INI_KEY_FIXED) {
        snprintf(value, sizeof value, "1");
    } else if (k == INI_KEY_DEFAULT && s->type != SECTION_KEY) {
        skipSpaces(&at);
        if (*at == '"') return settingProblem(p, "quoted_number", "default is a number, without quotes");
        readWord(&at, value, sizeof value);
    } else if (readSidecarText(&at, b->word, value, sizeof value, p)) {
        return 1;
    }
    if (!isLineEnd(at)) {
        if (k == INI_KEY_RESTART || k == INI_KEY_FIXED) return settingProblem(p, "trailing_text", "%s takes nothing after it", b->word);
        return settingProblem(p, "trailing_text", "text after %s's value", b->word);
    }
    return storeIniKey(s, b->word, k, option, value, p);
}

// Ends the open block: checks and declares its setting.
static void finishSidecarBlock(IniParse *r) {
    if (!sidecarBlock.open) return;
    sidecarBlock.open = 0;
    IniSection *s = &sidecarBlock.s;
    SettingProblem p;
    int isSetting = s->type == SECTION_SETTING;
    if (isSetting) {
        int kindLine = 0;
        for (int i = 0; i < sidecarBlock.count; i++) {
            const BlockLine *b = &sidecarBlock.lines[i];
            int type = settingKindType(b->word);
            if (!type) continue;
            if (kindLine) {
                settingProblem(&p, "two_kinds", "%s: the setting's kind is on line %d", b->word, kindLine);
                iniLineProblem(r, s, b->line, &p, b->source);
                continue;
            }
            kindLine = b->line;
            s->type = type;
            snprintf(s->word, sizeof s->word, "%s", type == SECTION_ONOFF ? "onoff" : type == SECTION_CHOICE ? "choice" : "key");
            if (!isLineEnd(b->args) && settingProblem(&p, "trailing_text", "%s takes nothing after it", b->word))
                iniLineProblem(r, s, b->line, &p, b->source);
        }
        if (!kindLine) {
            settingProblem(&p, "no_kind", "no onoff, choice, or key line");
            iniSectionProblem(r, s, &p);
            s->type = SECTION_UNKNOWN;
        }
    }
    for (int i = 0; s->type != SECTION_UNKNOWN && i < sidecarBlock.count; i++) {
        const BlockLine *b = &sidecarBlock.lines[i];
        if (isSetting && settingKindType(b->word)) continue;
        if (readSidecarBlockLine(s, b, &p)) iniLineProblem(r, s, b->line, &p, b->source);
    }
    finishIniSection(r, s);
}

// Records a +setting or -setting line of the sidecar.
static void noteSidecarSettingRef(const IniParse *r, const char *at, int line) {
    if (!reading.active || reading.disabled || _stricmp(reading.mod, r->mod)) return;
    if (!growList((void **)&reading.refs, &reading.refCapacity, reading.refCount + 1, sizeof reading.refs[0])) return;
    SettingRef *ref = &reading.refs[reading.refCount++];
    memset(ref, 0, sizeof *ref);
    snprintf(ref->word, sizeof ref->word, "%s", at[0] == '+' ? "+setting" : "-setting");
    const char *p = at + 8;
    readWord(&p, ref->name, sizeof ref->name);
    ref->hasIndex = readInt(&p, &ref->index);
    ref->trailing = !isLineEnd(p);
    ref->line = line;
    ref->inSidecar = r->sidecar;
}

// Cuts raw at its first #, which starts a comment, as the game does.
static int cutSidecarComment(char *raw) {
    int inQuotes = 0;
    for (char *c = raw; *c; c++) {
        if (*c == '"') inQuotes = !inQuotes;
        else if (*c == '#') {
            *c = '\0';
            return inQuotes;
        }
    }
    return 0;
}

// Reads the text of `mod`'s sidecar into reading `r`.
static void readSidecarSettingsText(IniParse *r, const char *text, int in) {
    r->sidecar = in;
    sidecarBlock.open = 0;
    if (!strncmp(text, "\xEF\xBB\xBF", 3)) text += 3;
    char raw[INI_LINE_MAX + 1], word[24];
    SettingProblem p;
    for (int line = 1; *text; line++) {
        int tooLong;
        text = copySettingsLine(text, raw, &tooLong);
        if (in == SETTINGS_IN_C5M) {
            const char *own = sidecarLineText(raw);
            if (!own) {
                cutSidecarComment(raw);
                const char *rest = raw;
                skipSpaces(&rest);
                if (*rest) finishSidecarBlock(r);
                continue;
            }
            memmove(raw, own, strlen(own) + 1);
        }
        int hashInText = cutSidecarComment(raw);
        const char *at = raw;
        skipSpaces(&at);
        if (!*at) continue;
        if (isSettingLine(at)) noteSidecarSettingRef(r, at, line);
        const char *args = at;
        readWord(&args, word, sizeof word);
        skipSpaces(&args);
        int starts = isSettingBlockStart(word), inBlock = sidecarBlock.open && isSettingBlockWord(word);
        if (!starts && !inBlock) {
            finishSidecarBlock(r);
            continue;
        }
        if (tooLong) {
            char source[160];
            snprintf(source, sizeof source, "%.150s", at);
            settingProblem(&p, "line_too_long", "line over %d characters", INI_LINE_MAX);
            if (starts) finishSidecarBlock(r);
            rejectSidecarLine(r->sidecar, r->mod, word, line, &p, source);
            if (inBlock) sidecarBlock.s.problems++;
            continue;
        }
        if (starts) {
            finishSidecarBlock(r);
            startSidecarBlock(r, word, args, line, at, hashInText);
            r->sidecarBlocks++;
            continue;
        }
        if (!growList((void **)&sidecarBlock.lines, &sidecarBlock.capacity, sidecarBlock.count + 1, sizeof sidecarBlock.lines[0])) {
            settingProblem(&p, "out_of_memory", "out of memory");
            iniLineProblem(r, &sidecarBlock.s, line, &p, at);
            continue;
        }
        BlockLine *b = &sidecarBlock.lines[sidecarBlock.count++];
        b->line = line;
        b->hashInText = hashInText;
        snprintf(b->word, sizeof b->word, "%s", word);
        snprintf(b->args, sizeof b->args, "%s", args);
        snprintf(b->source, sizeof b->source, "%.150s", at);
    }
    finishSidecarBlock(r);
    endSettingsFile(r);
}
