// Mod Settings: the settings index, left-out lines, and the checks a declared setting passes.

// ---- Settings and left-out lines -----------------------------------------------------------------
// The settings index: entries found by mod and name through a hash table.
static int *settingIndex;
static unsigned int settingIndexSize;

static unsigned int settingKeyHash(const char *mod, const char *name) {
    unsigned int h = 2166136261u;
    for (const char *part = mod; part; part = part == mod ? name : NULL) {
        for (const unsigned char *c = (const unsigned char *)part; *c; c++)
            h = (h ^ (*c >= 'A' && *c <= 'Z' ? *c - 'A' + 'a' : *c >= 0x80 ? 0x80 : *c)) * 16777619u;
        h = (h ^ 0xffu) * 16777619u;
    }
    return h;
}

static void indexSetting(int i) {
    unsigned int h = settingKeyHash(settings[i]->mod, settings[i]->name) & (settingIndexSize - 1);
    while (settingIndex[h]) h = (h + 1) & (settingIndexSize - 1);
    settingIndex[h] = i + 1;
}

// Makes room in the index for one more entry.
static int growSettingIndex(void) {
    if ((unsigned int)(settingCount + 1) * 2 < settingIndexSize) return 1;
    unsigned int size = settingIndexSize ? settingIndexSize * 2 : 1024;
    int *index = calloc(size, sizeof *index);
    if (!index) return 0;
    free(settingIndex);
    settingIndex = index;
    settingIndexSize = size;
    for (int i = 0; i < settingCount; i++) indexSetting(i);
    return 1;
}

// Calls `match` for each entry of the mod's setting `name`, and returns the one it picks.
static ModSetting *findSetting(const char *mod, const char *name, ModSetting *(*match)(ModSetting *best, ModSetting *s)) {
    ModSetting *best = NULL;
    if (!settingIndexSize) return NULL;
    for (unsigned int h = settingKeyHash(mod, name) & (settingIndexSize - 1); settingIndex[h]; h = (h + 1) & (settingIndexSize - 1)) {
        ModSetting *s = settings[settingIndex[h] - 1];
        if (!_stricmp(s->mod, mod) && !_stricmp(s->name, name)) best = match(best, s);
    }
    return best;
}

// A declared entry: the first kept one in the list, else the last left-out one.
static ModSetting *betterDeclared(ModSetting *best, ModSetting *s) {
    if (!s->declared) return best;
    if (!best) return s;
    if (!s->rejected) return best->rejected || s->entry < best->entry ? s : best;
    return best->rejected && s->entry > best->entry ? s : best;
}

// The first entry in the list.
static ModSetting *firstEntry(ModSetting *best, ModSetting *s) {
    return !best || s->entry < best->entry ? s : best;
}

// Returns the mod's setting that its latest reading declared with this name, or NULL.
static ModSetting *findDeclaredSetting(const char *mod, const char *name) {
    return findSetting(mod, name, betterDeclared);
}

// Returns the entry for the mod's setting `name`, declared or not, creating it if there is none.
static ModSetting *settingEntry(const char *mod, const char *name, int *created) {
    *created = 0;
    ModSetting *s = findSetting(mod, name, firstEntry);
    if (s) return s;
    s = calloc(1, sizeof *s);
    if (!s || !growSettingIndex() || !growList((void **)&settings, &settingCapacity, settingCount + 1, sizeof settings[0])) {
        free(s);
        return NULL;
    }
    snprintf(s->mod, sizeof s->mod, "%s", mod);
    snprintf(s->name, sizeof s->name, "%s", name);
    s->entry = settingCount;
    settings[settingCount++] = s;
    indexSetting(s->entry);
    *created = 1;
    return s;
}

// Adds line `line` of `mod`'s file to the mod's list of left-out lines.
static void addBadLine(const char *mod, const char *ext, int line, const SettingProblem *p, const char *source) {
    leftOutVersion++;
    if (!growList((void **)&badLines, &badLineCapacity, badLineCount + 1, sizeof badLines[0])) return;
    BadLine *b = &badLines[badLineCount++];
    snprintf(b->mod, sizeof b->mod, "%s", mod);
    snprintf(b->file, sizeof b->file, "%.*s%s", MOD_NAME_MAX - 1, mod, ext);
    b->line = line;
    snprintf(b->text, sizeof b->text, "%.67s line %d: %.79s", b->file, line, p->text);
    snprintf(b->source, sizeof b->source, "%.150s", source);
}

// Leaves out a +setting line of `mod`'s .c5m.
static void rejectC5mLine(const char *mod, const char *word, int line, const SettingProblem *p, const char *source) {
    refuseC5mLine(word, mod, p->reason, "line=%d problem=%s text=%.150s", line, p->text, source);
    addBadLine(mod, ".c5m", line, p, source);
}

// Leaves out line `line` of `mod`'s .ini.
static void rejectIniLine(const char *mod, const char *section, int line, const SettingProblem *p, const char *source) {
    refuseIniLine(section, mod, p->reason, "file=%s.ini line=%d problem=%s text=%.150s", mod, line, p->text, source);
    addBadLine(mod, ".ini", line, p, source);
}

// Leaves out line `line` of `mod`'s sidecar, or of a #!@ line of its .c5m.
static void rejectSidecarLine(int in, const char *mod, const char *word, int line, const SettingProblem *p, const char *source) {
    refuseSidecarLine(in, word, mod, p->reason, "line=%d problem=%s text=%.150s", line, p->text, source);
    addBadLine(mod, settingsFileSuffix(in), line, p, source);
}

// The lines a mod's plugins take on its page.
static int pluginPageLines(const char *mod);

// ---- The mod's settings files: checking and declaring ---------------------------------------------
// CoE5SE's own readers of the sidecar's settings blocks and the .ini's sections.
enum { SECTION_NONE, SECTION_ONOFF, SECTION_CHOICE, SECTION_KEY, SECTION_HEADER, SECTION_PAGE, SECTION_UNKNOWN, SECTION_SETTING };

#define INI_LINE_MAX 1024

// One section of the .ini, or one block of the sidecar, as read so far.
typedef struct {
    int type, line;
    char word[16];
    char name[INI_LINE_MAX];
    char source[160];
    int labelLine, defaultLine, tipLine, restartLine, groupLine, fixedLine;
    int hasDefault, defaultValue, restart, fixed;
    char label[SETTING_TEXT_MAX + 1], tip[SETTING_TIP_MAX + 1], group[SETTING_TEXT_MAX + 1];
    int optionLine[SETTING_OPTIONS_MAX], optionTipLine[SETTING_OPTIONS_MAX];
    char options[SETTING_OPTIONS_MAX][SETTING_TEXT_MAX + 1];
    char optionTips[SETTING_OPTIONS_MAX][SETTING_TIP_MAX + 1];
    int problems;
} IniSection;

// What the files' sections so far leave open.
typedef struct {
    const char *mod;
    const char *dir;
    int sidecar;
    int sidecarBlocks;
    int order;
    int pageLines, boxOpen;
    int headerPending, headerLine;
    char header[SETTING_TEXT_MAX + 1], headerSource[160];
    int pages;
    int pageOpen, pageLine, pageSettings;
    char page[SETTING_NAME_MAX + 1], pageSource[160];
} IniParse;

// The sidecar word that starts a block of type `section`, for the log.
static const char *sidecarWordOf(const char *section) {
    if (!strcmp(section, "-")) return "-";
    if (!strcmp(section, "header")) return SETTING_HEADER_WORD;
    if (!strcmp(section, "page")) return SETTING_PAGE_WORD;
    return SETTING_WORD;
}

// Leaves out line `line` of the file being read.
static void rejectSettingsLine(const IniParse *r, const char *section, int line, const SettingProblem *p, const char *source) {
    if (r->sidecar) rejectSidecarLine(r->sidecar, r->mod, sidecarWordOf(section), line, p, source);
    else rejectIniLine(r->mod, section, line, p, source);
}

// Lists a problem with line `line` of the file; its section's setting is then left out.
static void iniLineProblem(IniParse *r, IniSection *s, int line, const SettingProblem *p, const char *source) {
    rejectSettingsLine(r, s->type == SECTION_NONE ? "-" : s->word, line, p, source);
    if (s->type != SECTION_NONE) s->problems++;
}

// A problem with section `s` as a whole, listed at its [section] line.
static void iniSectionProblem(IniParse *r, IniSection *s, const SettingProblem *p) {
    iniLineProblem(r, s, s->line, p, s->source);
}

// The same, from the problem's reason word and printf-style text.
static PRINTF_FORMAT(4, 5) void rejectIniSection(IniParse *r, IniSection *s, const char *reason, const char *fmt, ...) {
    SettingProblem p;
    va_list ap;
    va_start(ap, fmt);
    settingProblemV(&p, reason, fmt, ap);
    va_end(ap);
    iniSectionProblem(r, s, &p);
}

static void rejectPendingHeader(IniParse *r) {
    SettingProblem p;
    settingProblem(&p, "no_setting_after", "header needs an on/off or key setting after it");
    rejectSettingsLine(r, "header", r->headerLine, &p, r->headerSource);
    r->headerPending = 0;
}

// The mod's kept page named `name` (ignoring case), or NULL.
static ModPage *findModPage(const char *mod, const char *name) {
    for (int i = 0; i < modPageCount; i++)
        if (!_stricmp(modPages[i].mod, mod) && !_stricmp(modPages[i].name, name)) return &modPages[i];
    return NULL;
}

// Ends the [page] the settings go on, if one is open.
static void endIniPage(IniParse *r) {
    if (!r->pageOpen) return;
    r->pageOpen = 0;
    SettingProblem p;
    if (checkSettingPageSettings(r->pageSettings, &p)) {
        rejectSettingsLine(r, "page", r->pageLine, &p, r->pageSource);
        ModPage *page = findModPage(r->mod, r->page);
        if (page) {
            memmove(page, page + 1, (size_t)(modPages + modPageCount - (page + 1)) * sizeof *page);
            modPageCount--;
        }
        r->pages--;
    }
    r->page[0] = '\0';
}

// Where an earlier setting or page of the mod is, for "already used" texts.
static void usedAt(const IniParse *r, int inSidecar, int line, char *out, size_t size) {
    if (inSidecar == r->sidecar) snprintf(out, size, "on line %d", line);
    else snprintf(out, size, "in %.40s%s line %d", r->mod, settingsFileSuffix(inSidecar), line);
}

// The text for a section with no `key` line.
static const char *noKeyLineText(const IniParse *r, const char *key) {
    static char text[40];
    snprintf(text, sizeof text, r->sidecar ? "no %.16s line" : "no %.16s = line", key);
    return text;
}

// Ends a page section: checks it, and opens its page for the settings after it (rule 9).
static void finishIniPage(IniParse *r, IniSection *s) {
    SettingProblem p;
    endIniPage(r);
    if (r->headerPending) {
        settingProblem(&p, "page_after", "header must come before an on/off or key setting");
        rejectSettingsLine(r, "header", r->headerLine, &p, r->headerSource);
        r->headerPending = 0;
    }
    const ModPage *same = NULL;
    char where[80];
    if (checkSettingName("name", s->name, &p)) iniSectionProblem(r, s, &p);
    else if ((same = findModPage(r->mod, s->name)) != NULL) {
        usedAt(r, same->inSidecar, same->line, where, sizeof where);
        rejectIniSection(r, s, "duplicate_page", "page name already used %s", where);
    }
    if (!s->labelLine) rejectIniSection(r, s, "no_label", "%s", noKeyLineText(r, "label"));
    if (checkSettingPageCount(r->pages, &p)) iniSectionProblem(r, s, &p);
    if (s->problems) return;
    if (!growList((void **)&modPages, &modPageCapacity, modPageCount + 1, sizeof modPages[0])) {
        rejectIniSection(r, s, "out_of_memory", "out of memory");
        return;
    }
    ModPage *page = &modPages[modPageCount++];
    snprintf(page->mod, sizeof page->mod, "%s", r->mod);
    snprintf(page->name, sizeof page->name, "%.*s", SETTING_NAME_MAX, s->name);
    snprintf(page->label, sizeof page->label, "%s", s->label);
    page->line = s->line;
    page->inSidecar = r->sidecar;
    r->pages++;
    r->pageOpen = 1;
    r->pageLine = s->line;
    r->pageSettings = 0;
    r->boxOpen = 0;
    snprintf(r->page, sizeof r->page, "%s", page->name);
    snprintf(r->pageSource, sizeof r->pageSource, "%s", s->source);
}

// 1 if `text` is a whole number, in *out.
static int parseWholeNumber(const char *text, int *out) {
    char *end;
    long v = strtol(text, &end, 10);
    if (end == text || *end || v < -1000000 || v > 1000000) return 0;
    *out = (int)v;
    return 1;
}

// Reads a [section] line, `text` (starting at its [), into `s`.
static void startIniSection(IniParse *r, IniSection *s, const char *text, int line) {
    memset(s, 0, sizeof *s);
    s->line = line;
    snprintf(s->source, sizeof s->source, "%.150s", text);
    SettingProblem p;
    const char *close = strrchr(text, ']');
    if (!close) {
        s->type = SECTION_UNKNOWN;
        snprintf(s->word, sizeof s->word, "-");
        settingProblem(&p, "unclosed_section", "[ with no ] to end the section");
        iniSectionProblem(r, s, &p);
        return;
    }
    const char *at = text + 1;
    skipSpaces(&at);
    size_t wordLen = strcspn(at, " \t]");
    snprintf(s->word, sizeof s->word, "%.*s", (int)(wordLen < sizeof s->word ? wordLen : sizeof s->word - 1), at);
    for (char *c = s->word; *c; c++) *c = (char)tolower((unsigned char)*c);
    at += wordLen;
    skipSpaces(&at);
    size_t nameLen = (size_t)(close - at);
    while (nameLen && (at[nameLen - 1] == ' ' || at[nameLen - 1] == '\t')) nameLen--;
    snprintf(s->name, sizeof s->name, "%.*s", (int)nameLen, at);
    if (!strcmp(s->word, "onoff")) s->type = SECTION_ONOFF;
    else if (!strcmp(s->word, "choice")) s->type = SECTION_CHOICE;
    else if (!strcmp(s->word, "key")) s->type = SECTION_KEY;
    else if (!strcmp(s->word, "header")) s->type = SECTION_HEADER;
    else if (!strcmp(s->word, "page")) s->type = SECTION_PAGE;
    else {
        s->type = SECTION_UNKNOWN;
        settingProblem(&p, "unknown_section", "[%.16s] isn't a section: use onoff, choice, key, header, or page", s->word);
        iniSectionProblem(r, s, &p);
        return;
    }
    if (!isLineEnd(close + 1)) {
        settingProblem(&p, "trailing_text", "text after the ]");
        iniSectionProblem(r, s, &p);
    }
}

// Reads a quoted text value into `out`.
static int readQuotedText(const char *key, const char *raw, char *out, size_t size, SettingProblem *p) {
    if (raw[0] != '"') return settingProblem(p, "needs_quotes", "%.16s needs quotes: %.16s = \"...\"", key, key);
    size_t n = 0;
    const char *c = raw + 1;
    for (;; c++) {
        if (!*c) return settingProblem(p, "unclosed_quote", "%.16s has no closing quote", key);
        if (*c == '"' && c[1] == '"') c++;
        else if (*c == '"') break;
        if (n + 1 < size) out[n++] = *c;
    }
    out[n] = '\0';
    if (!isLineEnd(c + 1)) return settingProblem(p, "text_after_quote", "text after %.16s's closing quote", key);
    return 0;
}

// The keys a section can have.
typedef enum { INI_KEY_UNKNOWN, INI_KEY_LABEL, INI_KEY_DEFAULT, INI_KEY_TIP, INI_KEY_RESTART, INI_KEY_GROUP, INI_KEY_FIXED,
               INI_KEY_OPTION, INI_KEY_OPTION_TIP } IniKey;

// Which key `key` is, ignoring case.
static IniKey iniKeyNamed(const char *key, int *option) {
    static const struct { const char *name; IniKey key; } NAMED[] = {
        {"label", INI_KEY_LABEL}, {"default", INI_KEY_DEFAULT}, {"tip", INI_KEY_TIP},
        {"restart", INI_KEY_RESTART}, {"group", INI_KEY_GROUP}, {"fixed", INI_KEY_FIXED},
    };
    *option = -1;
    for (size_t i = 0; i < sizeof NAMED / sizeof NAMED[0]; i++)
        if (!_stricmp(key, NAMED[i].name)) return NAMED[i].key;
    const char *digits = key + 6;
    if (!_strnicmp(key, "option", 6) && digits[0] && strspn(digits, "0123456789") == strlen(digits) && strlen(digits) < 4) {
        *option = atoi(digits);
        return INI_KEY_OPTION;
    }
    return INI_KEY_UNKNOWN;
}

// The line field of `s` that records where key `k` was set, or NULL for no such key.
static int *iniKeyLine(IniSection *s, IniKey k, int option) {
    switch (k) {
    case INI_KEY_LABEL: return &s->labelLine;
    case INI_KEY_DEFAULT: return &s->defaultLine;
    case INI_KEY_TIP: return &s->tipLine;
    case INI_KEY_RESTART: return &s->restartLine;
    case INI_KEY_GROUP: return &s->groupLine;
    case INI_KEY_FIXED: return &s->fixedLine;
    case INI_KEY_OPTION: return option < SETTING_OPTIONS_MAX ? &s->optionLine[option] : NULL;
    case INI_KEY_OPTION_TIP: return option < SETTING_OPTIONS_MAX ? &s->optionTipLine[option] : NULL;
    default: return NULL;
    }
}

// Returns 1, and fills *p, if section `s` can't have key `k` (written `key`), or already has it.
static int iniKeyRefused(const IniSection *s, const char *key, IniKey k, int option, const int *seen, SettingProblem *p) {
    if (s->type == SECTION_HEADER) return settingProblem(p, "unknown_key", "a [header] has no keys, only its title");
    if (!key[0]) return settingProblem(p, "unknown_key", "no key before the =");
    if (s->type == SECTION_PAGE && k != INI_KEY_LABEL) return settingProblem(p, "unknown_key", "%.16s: a [page] has only a label", key);
    if (k == INI_KEY_OPTION && s->type != SECTION_CHOICE) return settingProblem(p, "unknown_key", "%.16s: options are for a [choice]", key);
    if (k == INI_KEY_TIP && s->type == SECTION_CHOICE) return settingProblem(p, "unknown_key", "a [choice] has no tip");
    if ((k == INI_KEY_GROUP || k == INI_KEY_FIXED) && s->type != SECTION_KEY) return settingProblem(p, "unknown_key", "%.16s is for a [key]", key);
    if (k == INI_KEY_OPTION && option >= SETTING_OPTIONS_MAX)
        return settingProblem(p, "too_many_options", "%.16s: a choice has option0 to option%d", key, SETTING_OPTIONS_MAX - 1);
    if (!seen) return settingProblem(p, "unknown_key", "unknown key %.24s", key);
    if (*seen) return settingProblem(p, "duplicate_key", "%.16s is set twice (line %d)", key, *seen);
    return 0;
}

// Stores key `k`'s value, `value` as read, in section `s`.
static int storeIniKey(IniSection *s, const char *key, IniKey k, int option, const char *value, SettingProblem *p) {
    switch (k) {
    case INI_KEY_DEFAULT:
        if (s->type == SECTION_KEY) {
            s->hasDefault = keyValueFromName(value, &s->defaultValue);
            return s->hasDefault ? 0 : settingProblem(p, "not_a_key", "default %.20s is not a key name", value);
        }
        s->hasDefault = parseWholeNumber(value, &s->defaultValue);
        return s->hasDefault ? 0 : settingProblem(p, "not_a_number", "default is not a whole number");
    case INI_KEY_GROUP:
        if (checkSettingText("group", value, SETTING_TEXT_MAX, p)) return 1;
        snprintf(s->group, sizeof s->group, "%.*s", SETTING_TEXT_MAX, value);
        return 0;
    case INI_KEY_OPTION:
        if (checkSettingText("option", value, SETTING_TEXT_MAX, p)) return 1;
        snprintf(s->options[option], sizeof s->options[option], "%.*s", SETTING_TEXT_MAX, value);
        return 0;
    case INI_KEY_LABEL:
        if (checkSettingText("label", value, SETTING_TEXT_MAX, p)) return 1;
        snprintf(s->label, sizeof s->label, "%.*s", SETTING_TEXT_MAX, value);
        return 0;
    case INI_KEY_TIP:
        if (checkSettingText("tip", value, SETTING_TIP_MAX, p)) return 1;
        snprintf(s->tip, sizeof s->tip, "%.*s", SETTING_TIP_MAX, value);
        return 0;
    case INI_KEY_OPTION_TIP:
        if (checkSettingText("optiontip", value, SETTING_TIP_MAX, p)) return 1;
        snprintf(s->optionTips[option], sizeof s->optionTips[option], "%.*s", SETTING_TIP_MAX, value);
        return 0;
    default:
        if (strcmp(value, "0") && strcmp(value, "1")) return settingProblem(p, "not_0_or_1", "%.16s is 0 or 1", key);
        *(k == INI_KEY_FIXED ? &s->fixed : &s->restart) = value[0] == '1';
        return 0;
    }
}

// Reads a `key = value` line of section `s`.
static void readIniKey(IniParse *r, IniSection *s, const char *key, const char *raw, int line, const char *source) {
    if (s->type == SECTION_UNKNOWN) return;
    SettingProblem p;
    if (s->type == SECTION_NONE) {
        settingProblem(&p, "outside_section", "a key before the first [section]");
        iniLineProblem(r, s, line, &p, source);
        return;
    }
    int option;
    IniKey k = iniKeyNamed(key, &option);
    int *seen = iniKeyLine(s, k, option);
    if (iniKeyRefused(s, key, k, option, seen, &p)) {
        iniLineProblem(r, s, line, &p, source);
        return;
    }
    *seen = line;
    char value[INI_LINE_MAX + 1];
    int isText = k == INI_KEY_OPTION || k == INI_KEY_LABEL || k == INI_KEY_TIP || k == INI_KEY_GROUP ||
                 (k == INI_KEY_DEFAULT && s->type == SECTION_KEY);
    int bad = 0;
    if (isText) bad = readQuotedText(key, raw, value, sizeof value, &p);
    else if (raw[0] == '"') bad = settingProblem(&p, "quoted_number", "%.16s is a number, without quotes", key);
    else snprintf(value, sizeof value, "%s", raw);
    if (!bad) bad = storeIniKey(s, key, k, option, value, &p);
    if (bad) iniLineProblem(r, s, line, &p, source);
}

// Ends a [header] section: it titles the box that the next on/off or key setting opens.
static void finishIniHeader(IniParse *r, IniSection *s) {
    SettingProblem p;
    if (checkSettingText("header", s->name, SETTING_TEXT_MAX, &p)) iniSectionProblem(r, s, &p);
    if (s->problems) return;
    if (r->headerPending) rejectPendingHeader(r);
    r->headerPending = 1;
    r->headerLine = s->line;
    snprintf(r->header, sizeof r->header, "%.*s", SETTING_TEXT_MAX, s->name);
    snprintf(r->headerSource, sizeof r->headerSource, "%s", s->source);
}

// Checks an on/off, choice, or key section as a whole (rules 1, 4, and 5), listing each problem.
static int checkIniSetting(IniParse *r, IniSection *s) {
    SettingProblem p;
    int optionCount = 0;
    if (s->type == SECTION_UNKNOWN) return 0;
    if (checkSettingName("name", s->name, &p)) iniSectionProblem(r, s, &p);
    if (!s->labelLine) rejectIniSection(r, s, "no_label", "%s", noKeyLineText(r, "label"));
    if (!s->defaultLine) rejectIniSection(r, s, "no_default", "%s", noKeyLineText(r, "default"));
    if (s->type == SECTION_CHOICE) {
        for (int k = 0; k < SETTING_OPTIONS_MAX; k++)
            if (s->optionLine[k]) optionCount = k + 1;
        int missing = -1;
        for (int k = 0; k < optionCount && missing < 0; k++)
            if (!s->optionLine[k]) missing = k;
        if (missing >= 0)
            rejectIniSection(r, s, "missing_option", r->sidecar ? "no option %d before the last option" : "no option%d before the last option",
                             missing);
        else if (checkSettingChoice(optionCount, s->hasDefault ? s->defaultValue : 0, &p) &&
                 (s->hasDefault || strcmp(p.reason, "default_out_of_range")))
            iniSectionProblem(r, s, &p);
        for (int k = optionCount; k < SETTING_OPTIONS_MAX; k++) {
            if (!s->optionTipLine[k]) continue;
            settingProblem(&p, "no_such_option", "optiontip %d: the choice has no option %d", k, k);
            iniSectionProblem(r, s, &p);
        }
    } else if (s->type != SECTION_KEY && s->hasDefault && checkSettingOnOffDefault(s->defaultValue, &p)) {
        iniSectionProblem(r, s, &p);
    }
    return optionCount;
}

// A left-out setting whose name and default could be read.
static void declareLeftOutSetting(IniParse *r, const IniSection *s, int optionCount) {
    int created;
    ModSetting *hidden = settingEntry(r->mod, s->name, &created);
    if (!hidden) return;
    hidden->declared = hidden->rejected = 1;
    hidden->line = s->line;
    hidden->inSidecar = r->sidecar;
    hidden->order = ++r->order;
    hidden->defaultValue = hidden->value = s->defaultValue;
    hidden->optionCount = optionCount;
    hidden->isKey = s->type == SECTION_KEY;
}

// Declares the setting of section `s`.
static void declareKeptSetting(IniParse *r, IniSection *s, int optionCount, int lines, int startsBox) {
    int created, isKey = s->type == SECTION_KEY;
    ModSetting *kept = settingEntry(r->mod, s->name, &created);
    if (!kept) {
        rejectIniSection(r, s, "out_of_memory", "out of memory");
        return;
    }
    kept->declared = 1;
    kept->rejected = 0;
    kept->line = s->line;
    kept->inSidecar = r->sidecar;
    kept->order = ++r->order;
    kept->restart = s->restart;
    kept->defaultValue = s->defaultValue;
    kept->optionCount = optionCount;
    kept->isKey = isKey;
    kept->fixed = isKey && (s->fixed || s->defaultValue == KEY_ESC);
    kept->pageLines = lines;
    snprintf(kept->page, sizeof kept->page, "%s", r->page);
    if (r->pageOpen) r->pageSettings++;
    snprintf(kept->label, sizeof kept->label, "%s", s->label);
    snprintf(kept->tip, sizeof kept->tip, "%s", s->tip);
    snprintf(kept->header, sizeof kept->header, "%s", startsBox ? r->header : "");
    snprintf(kept->group, sizeof kept->group, "%s", s->group);
    for (int k = 0; k < optionCount; k++) {
        snprintf(kept->options[k], sizeof kept->options[k], "%s", s->options[k]);
        snprintf(kept->optionTips[k], sizeof kept->optionTips[k], "%s", s->optionTips[k]);
    }
    kept->value = kept->fixed ? kept->defaultValue : loadValue(kept);
    if (kept->isKey && !kept->fixed && kept->value == KEY_ESC) {
        writeLog("setting_key_esc_dropped", "mod=%s name=%s", kept->mod, kept->name);
        kept->value = KEY_NONE;
        saveValue(kept);
    }
    if (!kept->launchSet) kept->launchValue = kept->promptedValue = kept->value;
    kept->launchSet = 1;
}

// 1 if `name` holds only letters, digits, and _, and fits a setting's name buffer.
static int isPlainSettingName(const char *name) {
    if (!name[0] || strlen(name) >= SETTING_NAME_SIZE) return 0;
    for (const char *c = name; *c; c++)
        if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_')) return 0;
    return 1;
}

// Ends section `s`: checks it as a whole, then declares its setting, or leaves it out.
static void finishIniSection(IniParse *r, IniSection *s) {
    if (s->type == SECTION_NONE) return;
    if (s->type == SECTION_HEADER) {
        finishIniHeader(r, s);
        return;
    }
    if (s->type == SECTION_PAGE) {
        finishIniPage(r, s);
        return;
    }
    int isChoice = s->type == SECTION_CHOICE;
    int optionCount = checkIniSetting(r, s);
    int isWord = isPlainSettingName(s->name);
    ModSetting *same = isWord && s->type != SECTION_UNKNOWN ? findDeclaredSetting(r->mod, s->name) : NULL;
    if (same && !s->problems) {
        char where[80];
        usedAt(r, same->inSidecar, same->line, where, sizeof where);
        rejectIniSection(r, s, "duplicate_name", "name already used %s", where);
    }
    int boxOpen = r->boxOpen;
    int startsBox = r->headerPending && !isChoice;
    int lines = s->problems ? 0 : settingPageLines(isChoice, isChoice ? optionCount : 0, startsBox, &boxOpen);

    if (r->headerPending) {
        SettingProblem p;
        if (s->problems && r->sidecar)
            refuseSidecarLine(r->sidecar, SETTING_HEADER_WORD, r->mod, "setting_rejected", "line=%d header=%s setting_line=%d", r->headerLine,
                              r->header, s->line);
        else if (s->problems)
            refuseIniLine("header", r->mod, "setting_rejected", "file=%s.ini line=%d header=%s setting_line=%d", r->mod,
                          r->headerLine, r->header, s->line);
        else if (isChoice && settingProblem(&p, "choice_after", "header must come before an on/off or key setting"))
            rejectSettingsLine(r, "header", r->headerLine, &p, r->headerSource);
        r->headerPending = 0;
    }
    if (s->problems) {
        if (isWord && s->hasDefault && !same && s->type != SECTION_UNKNOWN) declareLeftOutSetting(r, s, optionCount);
        return;
    }
    declareKeptSetting(r, s, optionCount, lines, startsBox);
    if (!s->problems) {
        r->pageLines += lines;
        r->boxOpen = boxOpen;
    }
}

// The kept key of `s`'s mod and group, other than `s`, that holds `key`, or NULL (rule 8).
static ModSetting *keyHolder(const ModSetting *s, int key) {
    if (key == KEY_NONE) return NULL;
    for (int i = 0; i < settingCount; i++) {
        ModSetting *t = settings[i];
        if (t != s && t->declared && !t->rejected && t->isKey && t->value == key && !_stricmp(t->mod, s->mod) &&
            !_stricmp(t->group, s->group))
            return t;
    }
    return NULL;
}

// After a reading: settles two keys of one group on one key.
static void resolveKeyConflicts(const char *mod) {
    for (int i = 0; i < settingCount; i++) {
        ModSetting *s = settings[i];
        if (!s->declared || s->rejected || !s->isKey || s->fixed || s->value == KEY_NONE || _stricmp(s->mod, mod)) continue;
        const ModSetting *keeper = NULL;
        for (int j = 0; j < settingCount; j++) {
            const ModSetting *t = settings[j];
            if (t == s || !t->declared || t->rejected || !t->isKey || t->value != s->value || !(t->fixed || t->order < s->order) ||
                _stricmp(t->mod, s->mod) || _stricmp(t->group, s->group))
                continue;
            if (!keeper || (t->fixed && !keeper->fixed) || (t->fixed == keeper->fixed && t->order < keeper->order)) keeper = t;
        }
        if (!keeper) continue;
        writeLog("setting_key_conflict", "mod=%s name=%s key=%d kept_by=%s", s->mod, s->name, s->value, keeper->name);
        s->value = KEY_NONE;
        saveValue(s);
    }
}

// Starts reading `mod`'s settings files into `r`.
static void startSettingsReading(IniParse *r, const char *mod) {
    memset(r, 0, sizeof *r);
    r->mod = mod;
    r->pageLines = pluginPageLines(mod);
}

// Ends one file of the reading.
static void endSettingsFile(IniParse *r) {
    if (r->headerPending) rejectPendingHeader(r);
    endIniPage(r);
}

// Copies line `line` of a file's text into raw[INI_LINE_MAX + 1].
static const char *copySettingsLine(const char *text, char *raw, int *tooLong) {
    size_t len = strcspn(text, "\n");
    const char *next = text + len + (text[len] == '\n');
    *tooLong = len > INI_LINE_MAX;
    snprintf(raw, INI_LINE_MAX + 1, "%.*s", (int)(*tooLong ? INI_LINE_MAX : len), text);
    size_t end = strlen(raw);
    while (end && (raw[end - 1] == '\r' || raw[end - 1] == ' ' || raw[end - 1] == '\t')) raw[--end] = '\0';
    return next;
}
