// The Mod Settings screen and the mods' settings files.

#include <ctype.h>

// The sidecar's words that start a block (the game has none of them).
#define SETTING_WORD        "newsetting"
#define SETTING_PAGE_WORD   "newsettingpage"
#define SETTING_HEADER_WORD "newsettingheader"

#define ID_MS_BACK    9
#define ID_PAGE_PREV  10
#define ID_PAGE_NEXT  11
#define ID_RESET      12
#define ID_MOD        50
// Plugins' screen lines on the page use ID_SCREEN_FIRST to ID_SCREEN_END.
#define ID_SUBPAGE    2000
#define ID_ONOFF      3000
#define ID_CHOICE     5000
#define ID_KEY        20000
// A page's ranges don't overlap.
_Static_assert(ID_SCREEN_END <= ID_SUBPAGE && ID_SUBPAGE + SETTING_PAGES_MAX <= ID_ONOFF, "a mod page's id ranges overlap");
#define SETTING_NAME_SIZE 64

typedef struct {
    int entry;
    char mod[MOD_NAME_MAX];
    char name[SETTING_NAME_SIZE];
    int declared;
    int rejected;
    int line;
    int inSidecar;
    int order;
    int defaultValue, value, optionCount;
    int isKey;
    int fixed;
    int launchValue, promptedValue;
    int launchSet;
    int restart;
    int pageLines;
    char label[SETTING_TEXT_MAX + 1], tip[SETTING_TIP_MAX + 1];
    char header[SETTING_TEXT_MAX + 1];
    char group[SETTING_TEXT_MAX + 1];
    char options[SETTING_OPTIONS_MAX][SETTING_TEXT_MAX + 1];
    char optionTips[SETTING_OPTIONS_MAX][SETTING_TIP_MAX + 1];
    char page[SETTING_NAME_MAX + 1];
} ModSetting;

// A page of a mod's settings files: a sub-page of the mod's page.
typedef struct {
    char mod[MOD_NAME_MAX];
    char name[SETTING_NAME_MAX + 1], label[SETTING_TEXT_MAX + 1];
    int line, inSidecar;
} ModPage;

// A line CoE5SE left out.
typedef struct {
    char mod[MOD_NAME_MAX];
    int line;
    char text[192];
    char file[MOD_NAME_MAX + 8];
    char source[160];
} BadLine;

// A +setting or -setting line, checked against the mod's settings once its reading ends.
typedef struct {
    int line, hasIndex, index, trailing, inSidecar;
    char word[10], name[SETTING_NAME_SIZE];
} SettingRef;

static const PluginApi *api = &API;
// The settings, each allocated on its own.
static ModSetting **settings;
static int settingCount, settingCapacity;
static BadLine *badLines;
static int badLineCount, badLineCapacity;
static ModPage *modPages;
static int modPageCount, modPageCapacity;
static void (*nextEventLine)(const char *, int);
// The mods whose settings files CoE5SE has tried to read this launch.
static char (*triedMods)[MOD_NAME_MAX];
static int triedModCount, triedModCapacity;
static unsigned int passReadings;

// The mod whose .c5m is being read at level 1, and the +setting lines it has so far.
static struct {
    int active;
    int disabled;
    char mod[MOD_NAME_MAX];
    int line;
    SettingRef *refs;
    int refCount, refCapacity;
} reading;

// ---- File stamps ---------------------------------------------------------------------------------
// What tells CoE5SE that a file changed since it last read it.
typedef struct {
    int exists;
    unsigned long long size, written, changed, node;
} FileStamp;

static void fileStampOf(const char *path, FileStamp *out) {
    memset(out, 0, sizeof *out);
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &d)) return;
    out->exists = 1;
    out->size = (unsigned long long)d.nFileSizeHigh << 32 | d.nFileSizeLow;
    out->written = (unsigned long long)d.ftLastWriteTime.dwHighDateTime << 32 | d.ftLastWriteTime.dwLowDateTime;
    out->changed = (unsigned long long)d.ftCreationTime.dwHighDateTime << 32 | d.ftCreationTime.dwLowDateTime;
#else
    struct stat st;
    if (stat(path, &st)) return;
    out->exists = 1;
    out->size = (unsigned long long)st.st_size;
    out->written = (unsigned long long)st.st_mtimespec.tv_sec * 1000000000ull + (unsigned long long)st.st_mtimespec.tv_nsec;
    out->changed = (unsigned long long)st.st_ctimespec.tv_sec * 1000000000ull + (unsigned long long)st.st_ctimespec.tv_nsec;
    out->node = (unsigned long long)st.st_ino;
#endif
}

static int sameFileStamp(const FileStamp *a, const FileStamp *b) {
    return a->exists == b->exists && a->size == b->size && a->written == b->written && a->changed == b->changed && a->node == b->node;
}

// ---- The players' picks ------------------------------------------------------------------------------
// The picks files: each mod's own coe5seModMenu/<ModName>.ini, and settings.ini.
typedef struct { const char *section, *key, *value; } SavedPick;

static unsigned int settingKeyHash(const char *mod, const char *name);

// One picks file as read.
typedef struct {
    int unsure;
    char *text;
    SavedPick *picks;
    int count, capacity;
    int *index;
    unsigned int indexSize;
    const char **sections;
    int sectionCount, sectionCapacity;
} PickTable;

// A mod's own picks file, as read.
typedef struct {
    char mod[MOD_NAME_MAX];
    PickTable table;
} ModPickFile;

static struct {
    unsigned long long seen;
    unsigned int generation;
    int loaded;
    PickTable legacy;
    ModPickFile *own;
    int ownCount, ownCapacity;
    int ownIncomplete;
} savedPicks;

// The pick for setting `name` of `mod` in `t`, or NULL.
static const SavedPick *findSavedPick(const PickTable *t, const char *mod, const char *name) {
    if (!t->indexSize) return NULL;
    for (unsigned int h = settingKeyHash(mod, name) & (t->indexSize - 1); t->index[h]; h = (h + 1) & (t->indexSize - 1)) {
        const SavedPick *p = &t->picks[t->index[h] - 1];
        if (!_stricmp(p->section, mod) && !_stricmp(p->key, name)) return p;
    }
    return NULL;
}

// Reads the line at `line` (cut at its end already) into `t`.
static int readSavedPickLine(PickTable *t, char *line, const char **section) {
    size_t length = strlen(line);
    if (length && line[length - 1] == '\r') line[--length] = '\0';
    if (length >= 1000) return 0;
    for (const unsigned char *c = (const unsigned char *)line; *c; c++)
        if (*c >= 0x80 || (*c < 0x20 && *c != '\t')) return 0;
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (!*p || ((*p == ';' || *p == '#') && !strchr(p, '='))) return 1;
    if (*p == '[') {
        char *close = strchr(p, ']');
        if (!close || close == p + 1 || strchr(p + 1, '[') || p[1] == ' ' || p[1] == '\t' || close[-1] == ' ' || close[-1] == '\t') return 0;
        for (const char *after = close + 1; *after; after++)
            if (*after != ' ' && *after != '\t') return 0;
        *close = '\0';
        for (int i = 0; i < t->sectionCount; i++)
            if (!_stricmp(t->sections[i], p + 1)) return 0;
        if (!growList((void **)&t->sections, &t->sectionCapacity, t->sectionCount + 1, sizeof t->sections[0])) return 0;
        t->sections[t->sectionCount++] = *section = p + 1;
        return 1;
    }
    char *eq = strchr(p, '=');
    if (!eq) return 0;
    if (!*section) return 1;
    char *keyEnd = eq, *value = eq + 1;
    while (keyEnd > p && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t')) keyEnd--;
    *keyEnd = '\0';
    while (*value == ' ' || *value == '\t') value++;
    char *valueEnd = value + strlen(value);
    while (valueEnd > value && (valueEnd[-1] == ' ' || valueEnd[-1] == '\t')) valueEnd--;
    *valueEnd = '\0';
    if (strchr(value, '"') || strchr(value, '\'')) return 0;
    if (!*p) return 1;
    if (findSavedPick(t, *section, p)) return 0;
    if (!growList((void **)&t->picks, &t->capacity, t->count + 1, sizeof t->picks[0])) return 0;
    if ((unsigned int)(t->count + 1) * 2 >= t->indexSize) {
        unsigned int size = t->indexSize ? t->indexSize * 2 : 64;
        int *index = calloc(size, sizeof *index);
        if (!index) return 0;
        free(t->index);
        t->index = index;
        t->indexSize = size;
        for (int i = 0; i < t->count; i++) {
            unsigned int h = settingKeyHash(t->picks[i].section, t->picks[i].key) & (size - 1);
            while (index[h]) h = (h + 1) & (size - 1);
            index[h] = i + 1;
        }
    }
    t->picks[t->count] = (SavedPick){*section, p, value};
    unsigned int h = settingKeyHash(*section, p) & (t->indexSize - 1);
    while (t->index[h]) h = (h + 1) & (t->indexSize - 1);
    t->index[h] = ++t->count;
    return 1;
}

// Reads the picks file at `path` into `t`, as it is now.
static void loadPickTable(PickTable *t, const char *path) {
    free(t->text);
    t->text = NULL;
    t->count = t->sectionCount = 0;
    if (t->index) memset(t->index, 0, t->indexSize * sizeof t->index[0]);
    t->unsure = 0;
    int missing;
    size_t length = 0;
    char *text = readWholeSettingsFile(path, &missing, &length);
    if (missing) return;
    if (!text || memchr(text, '\0', length)) {
        free(text);
        t->unsure = 1;
        return;
    }
    t->text = text;
    const char *section = NULL;
    for (char *line = text, *next; line && !t->unsure; line = next) {
        next = strchr(line, '\n');
        if (next) *next++ = '\0';
        if (!readSavedPickLine(t, line, &section)) t->unsure = 1;
    }
}

// Calls `each` for every mod's own picks file in coe5seModMenu.
static void forEachModPickFile(void (*each)(const char *mod, const char *path, void *data), void *data) {
    char pattern[MAX_PATH], path[MAX_PATH], old[MAX_PATH], mod[MOD_NAME_MAX];
    char (*seen)[MOD_NAME_MAX] = NULL;
    int seenCount = 0, seenCapacity = 0;
    int hasOld = legacyUserDir(old, sizeof old) && strcmp(old, userDir());
    for (int pass = 0; pass < 1 + hasOld; pass++) {
        if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP MOD_SETTINGS_DIR USER_DIR_SEP "*.ini", pass ? old : userDir()))
            continue;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            size_t len = strlen(fd.cFileName);
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || len <= 4 || len - 4 >= sizeof mod || _stricmp(fd.cFileName + len - 4, ".ini"))
                continue;
            snprintf(mod, sizeof mod, "%.*s", (int)(len - 4), fd.cFileName);
            int dup = 0;
            for (int i = 0; i < seenCount && !dup; i++) dup = !_stricmp(seen[i], mod);
            if (dup || !growList((void **)&seen, &seenCapacity, seenCount + 1, sizeof seen[0])) continue;
            snprintf(seen[seenCount++], sizeof seen[0], "%s", mod);
            if (modSettingsPath(mod, path, sizeof path)) each(mod, path, data);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    free(seen);
}

static void loadModPickFile(const char *mod, const char *path, void *data) {
    (void)data;
    if (!growList((void **)&savedPicks.own, &savedPicks.ownCapacity, savedPicks.ownCount + 1, sizeof savedPicks.own[0])) {
        savedPicks.ownIncomplete = 1;
        return;
    }
    ModPickFile *f = &savedPicks.own[savedPicks.ownCount++];
    memset(f, 0, sizeof *f);
    snprintf(f->mod, sizeof f->mod, "%s", mod);
    loadPickTable(&f->table, path);
}

static void freePickTable(PickTable *t) {
    free(t->text);
    free(t->picks);
    free(t->index);
    free(t->sections);
    memset(t, 0, sizeof *t);
}

// Reads every picks file into its table, as it is now.
static void loadSavedPicks(void) {
    char path[MAX_PATH];
    savedPicks.loaded = 1;
    for (int i = 0; i < savedPicks.ownCount; i++) freePickTable(&savedPicks.own[i].table);
    savedPicks.ownCount = savedPicks.ownIncomplete = 0;
    forEachModPickFile(loadModPickFile, NULL);
    if (settingsIniPath(path, sizeof path)) loadPickTable(&savedPicks.legacy, path);
    else freePickTable(&savedPicks.legacy);
}

// The table of `mod`'s own picks file, or NULL when the mod has none.
static ModPickFile *modPickFile(const char *mod) {
    for (int i = 0; i < savedPicks.ownCount; i++)
        if (!_stricmp(savedPicks.own[i].mod, mod)) return &savedPicks.own[i];
    return NULL;
}

// Adds what tells a file changed to the stamp `*hash`.
static void addFileToStamp(unsigned long long *hash, const char *name, const FileStamp *s) {
    const unsigned char *bytes[2] = {(const unsigned char *)name, (const unsigned char *)s};
    size_t sizes[2] = {strlen(name) + 1, sizeof *s};
    for (int part = 0; part < 2; part++)
        for (size_t i = 0; i < sizes[part]; i++) *hash = (*hash ^ bytes[part][i]) * 0x100000001b3ull;
}

static void addModPickFileToStamp(const char *mod, const char *path, void *data) {
    FileStamp s;
    fileStampOf(path, &s);
    addFileToStamp(data, mod, &s);
}

// One stamp over every picks file.
static unsigned long long picksFilesStamp(void) {
    unsigned long long hash = 0xcbf29ce484222325ull;
    char path[MAX_PATH];
    FileStamp s;
    if (settingsIniPath(path, sizeof path)) fileStampOf(path, &s);
    else memset(&s, 0, sizeof s);
    addFileToStamp(&hash, SETTINGS_INI_FILE, &s);
    forEachModPickFile(addModPickFileToStamp, &hash);
    return hash;
}

// Notes a change to the picks files that CoE5SE didn't make.
static void noteSettingsIniNow(void) {
    unsigned long long now = picksFilesStamp();
    if (now == savedPicks.seen) return;
    savedPicks.seen = now;
    savedPicks.generation++;
    savedPicks.loaded = 0;
}

// After CoE5SE wrote a picks file: reads the tables again.
static void noteOwnSettingsWrite(int ok) {
    savedPicks.seen = picksFilesStamp();
    if (!ok) savedPicks.generation++;
    savedPicks.loaded = 0;
}

// The saved text of `mod`'s setting `name` into out[size].
static int savedPickText(const char *mod, const char *name, char *out, size_t size) {
    out[0] = '\0';
    if (!savedPicks.loaded) loadSavedPicks();
    ModPickFile *own = modPickFile(mod);
    const PickTable *t = own ? &own->table : &savedPicks.legacy;
    if (t->unsure || (!own && savedPicks.ownIncomplete)) {
        char path[MAX_PATH];
        return settingsReadPath(mod, path, sizeof path) ? (int)GetPrivateProfileStringA(mod, name, "", out, (DWORD)size, path) : 0;
    }
    const SavedPick *p = findSavedPick(t, mod, name);
    if (p) snprintf(out, size, "%s", p->value);
    return (int)strlen(out);
}

// Returns the setting's saved value, or its default.
static int loadValue(const ModSetting *s) {
    char text[16];
    if (!savedPickText(s->mod, s->name, text, sizeof text)) return s->defaultValue;
    int v = atoi(text), max = s->optionCount ? s->optionCount - 1 : 1;
    if (s->isKey) return keyValueIsValid(v) ? v : s->defaultValue;
    return v < 0 || v > max ? s->defaultValue : v;
}

// Writes the setting's value to its mod's own picks file.
static void saveValue(const ModSetting *s) {
    char path[MAX_PATH], text[16];
    noteSettingsIniNow();
    if (!settingsWritePath(s->mod, path, sizeof path)) {
        noteOwnSettingsWrite(0);
        return;
    }
    snprintf(text, sizeof text, "%d", s->value);
    noteOwnSettingsWrite(WritePrivateProfileStringA(s->mod, s->name, text, path) != 0);
    writeLog("setting_changed", "mod=%s name=%s value=%d", s->mod, s->name, s->value);
}

// Deletes the setting's saved value, so it reads the mod's default.
static void forgetValue(const ModSetting *s) {
    char path[MAX_PATH];
    noteSettingsIniNow();
    if (!settingsWritePath(s->mod, path, sizeof path)) {
        noteOwnSettingsWrite(0);
        return;
    }
    noteOwnSettingsWrite(WritePrivateProfileStringA(s->mod, s->name, NULL, path) != 0);
}

// The name of mod `mod` in the game's mod table, which is its .c5m's file name.
static const char *modName(int mod) {
    return (const char *)(gameBase + RVA_MOD_NAMES + (size_t)mod * MOD_RECORD_SIZE);
}

// Logs c5m_refused for a line of mod `mod`'s .c5m that starts with `word`.
static int readingSidecarNow(void);
static void logSidecarRefusal(const char *word, const char *mod, const char *reason, ...) {
    va_list ap;
    va_start(ap, reason);
    logRefusal("c5m_refused", word, "mod", mod, reason, "%s file=%s" SIDECAR_SUFFIX, ap);
    va_end(ap);
}

static PRINTF_FORMAT(4, 5) void refuseC5mLine(const char *word, const char *mod, const char *reason, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (readingSidecarNow()) {
        char details[512];
        vsnprintf(details, sizeof details, fmt, ap);
        logSidecarRefusal(word, mod, reason, details, mod);
    } else {
        logRefusal("c5m_refused", word, "mod", mod, reason, fmt, ap);
    }
    va_end(ap);
}

// Logs c5m_refused for a line of mod `mod`'s sidecar.
static PRINTF_FORMAT(4, 5) void refuseSidecarLine(const char *word, const char *mod, const char *reason, const char *fmt, ...) {
    char details[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    logSidecarRefusal(word, mod, reason, details, mod);
}

// Logs ini_refused for a line of mod `mod`'s .ini.
static PRINTF_FORMAT(4, 5) void refuseIniLine(const char *section, const char *mod, const char *reason, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    logRefusal("ini_refused", section, "mod", mod, reason, fmt, ap);
    va_end(ap);
}

// Returns mod `name`'s index in the game's mod table, or -1.
static int modIndexByName(const char *name) {
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (!_stricmp(modName(m), name)) return m;
    return -1;
}

static int modEnabledByName(const char *name) {
    int m = modIndexByName(name);
    return m >= 0 && modEnabled(m);
}

// 1 if the mod's folder is in the game's local mods folder, not a Workshop item.
static int isInLocalModsFolder(const char *name) {
    int m = modIndexByName(name);
    if (m < 0) return 0;
    char dir[MAX_PATH];
    if (!buildPath(dir, sizeof dir, __func__, "%s" USER_DIR_SEP "mods" USER_DIR_SEP, userDir())) return 0;
    const char *a = modFolder(m), *b = dir;
    for (; *a && *b; a++, b++) {
        char x = *a == '\\' ? '/' : (char)tolower((unsigned char)*a), y = *b == '\\' ? '/' : (char)tolower((unsigned char)*b);
        if (x != y) return 0;
    }
    return !*b;
}

// ---- Reading a .c5m line -------------------------------------------------------------------------
static void skipSpaces(const char **p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

// Reads a number from *p and moves past it.
static int readInt(const char **p, int *out) {
    skipSpaces(p);
    char *end;
    long v = strtol(*p, &end, 10);
    if (end == *p) return 0;
    *out = (int)v;
    *p = end;
    return 1;
}

// Reads a word (anything up to a space or a quote) from *p into `out` and moves past it.
static int readWord(const char **p, char *out, size_t size) {
    skipSpaces(p);
    size_t n = 0, len = 0;
    for (; **p && **p != ' ' && **p != '\t' && **p != '"' && **p != '\r' && **p != '\n'; (*p)++, len++)
        if (n + 1 < size) out[n++] = **p;
    out[n] = '\0';
    return (int)len;
}

// 1 if nothing but spaces is left at p.
static int isLineEnd(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return !*p;
}

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

// Leaves out line `line` of `mod`'s sidecar.
static void rejectSidecarLine(const char *mod, const char *word, int line, const SettingProblem *p, const char *source) {
    refuseSidecarLine(word, mod, p->reason, "line=%d problem=%s text=%.150s", line, p->text, source);
    addBadLine(mod, SIDECAR_SUFFIX, line, p, source);
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
    if (r->sidecar) rejectSidecarLine(r->mod, sidecarWordOf(section), line, p, source);
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
    else snprintf(out, size, "in %.40s%s line %d", r->mod, inSidecar ? SIDECAR_SUFFIX : ".ini", line);
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
            refuseSidecarLine(SETTING_HEADER_WORD, r->mod, "setting_rejected", "line=%d header=%s setting_line=%d", r->headerLine,
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
    ref->inSidecar = 1;
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
static void readSidecarSettingsText(IniParse *r, const char *text) {
    r->sidecar = 1;
    sidecarBlock.open = 0;
    if (!strncmp(text, "\xEF\xBB\xBF", 3)) text += 3;
    char raw[INI_LINE_MAX + 1], word[24];
    SettingProblem p;
    for (int line = 1; *text; line++) {
        int tooLong;
        text = copySettingsLine(text, raw, &tooLong);
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
            rejectSidecarLine(r->mod, word, line, &p, source);
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

// ---- Reading a mod's settings files ---------------------------------------------------------------
// 1 if CoE5SE has tried to read `mod`'s settings files this launch.
static int settingsFileTried(const char *mod) {
    for (int i = 0; i < triedModCount; i++)
        if (!_stricmp(triedMods[i], mod)) return 1;
    return 0;
}

// The text of `mod`'s settings file at `path`, to free, or NULL when there is none.
static char *readSettingsFileText(const char *mod, const char *path, int sidecar) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *text = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (!text) {
        fclose(f);
        if (sidecar) refuseSidecarLine("-", mod, "out_of_memory", "size=%ld", size);
        else refuseIniLine("-", mod, "out_of_memory", "file=%s.ini size=%ld", mod, size);
        return NULL;
    }
    size_t got = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[got] = '\0';
    return text;
}

// Reads `mod`'s settings files in its folder `dir`.
static void logIgnoredIni(const char *mod, ...) {
    va_list ap;
    va_start(ap, mod);
    logRefusal("settings_ini_ignored", "-", "mod", mod, "c5se_has_settings", "file=%s.ini", ap);
    va_end(ap);
}

// A mod whose sidecar has settings blocks has its .ini left unread.
static void readSettingsIn(const char *mod, const char *dir) {
    if (!settingsFileTried(mod) && growList((void **)&triedMods, &triedModCapacity, triedModCount + 1, sizeof triedMods[0]))
        snprintf(triedMods[triedModCount++], MOD_NAME_MAX, "%s", mod);
    noteSettingsIniNow();
    IniParse r;
    startSettingsReading(&r, mod);
    char path[MAX_PATH], *text;
    if (buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s" SIDECAR_SUFFIX, dir, mod) &&
        (text = readSettingsFileText(mod, path, 1)) != NULL) {
        readSidecarSettingsText(&r, text);
        free(text);
    }
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s.ini", dir, mod)) {
    } else if (r.sidecarBlocks) {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) logIgnoredIni(mod, mod);
    } else if ((text = readSettingsFileText(mod, path, 0)) != NULL) {
        readIniText(&r, text);
        free(text);
    }
    resolveKeyConflicts(mod);
}

// Reads `mod`'s settings files, next to its .c5m, if it has any.
static void readSettingsFile(const char *mod) {
    int m = modIndexByName(mod);
    if (m < 0) return;
    readSettingsIn(modName(m), modFolder(m));
}

// ---- A reading whose files haven't changed -------------------------------------------------------
// Every level-1 reading of an enabled mod starts its settings afresh from its files.
typedef struct {
    char mod[MOD_NAME_MAX];
    char dir[MAX_PATH];
    FileStamp sidecar, ini;
    unsigned int picks;
    int pageLines;
    int fileBadLines;
    SettingRef *sidecarRefs;
    int sidecarRefCount, sidecarRefCapacity;
    int valid;
} SettingsRead;

static SettingsRead *settingsReads;
static int settingsReadCount, settingsReadCapacity;

static SettingsRead *settingsReadOf(const char *mod) {
    for (int i = 0; i < settingsReadCount; i++)
        if (!_stricmp(settingsReads[i].mod, mod)) return &settingsReads[i];
    return NULL;
}

// The mod's settings no longer come from one reading of its files.
static void forgetSettingsRead(const char *mod) {
    SettingsRead *e = settingsReadOf(mod);
    if (e) e->valid = 0;
}

// Fills `now` with what a reading of `mod` reads now, except the refs.
static int settingsReadNow(const char *mod, SettingsRead *now) {
    int m = modIndexByName(mod);
    if (m < 0) return 0;
    char path[MAX_PATH];
    memset(now, 0, sizeof *now);
    snprintf(now->mod, sizeof now->mod, "%s", modName(m));
    if (strlen(modFolder(m)) >= sizeof now->dir) return 0;
    snprintf(now->dir, sizeof now->dir, "%s", modFolder(m));
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s" SIDECAR_SUFFIX, now->dir, now->mod)) return 0;
    fileStampOf(path, &now->sidecar);
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s.ini", now->dir, now->mod)) return 0;
    fileStampOf(path, &now->ini);
    noteSettingsIniNow();
    now->picks = savedPicks.generation;
    now->pageLines = pluginPageLines(now->mod);
    return 1;
}

// Keeps the mod's settings from its last reading if `now` matches it.
static int keepSettingsRead(const SettingsRead *now) {
    SettingsRead *e = settingsReadOf(now->mod);
    if (!e || !e->valid || strcmp(e->dir, now->dir) || !sameFileStamp(&e->sidecar, &now->sidecar) ||
        !sameFileStamp(&e->ini, &now->ini) || e->picks != now->picks || e->pageLines != now->pageLines)
        return 0;
    BadLine *mine = NULL;
    if (e->fileBadLines && !(mine = malloc((size_t)e->fileBadLines * sizeof *mine))) return 0;
    if (e->sidecarRefCount && !growList((void **)&reading.refs, &reading.refCapacity, e->sidecarRefCount, sizeof reading.refs[0])) {
        free(mine);
        return 0;
    }
    memcpy(reading.refs, e->sidecarRefs, (size_t)e->sidecarRefCount * sizeof reading.refs[0]);
    reading.refCount = e->sidecarRefCount;
    int kept = 0, n = 0, had = 0;
    for (int i = 0; i < badLineCount; i++) {
        if (_stricmp(badLines[i].mod, now->mod)) badLines[kept++] = badLines[i];
        else if (had++ < e->fileBadLines) mine[n++] = badLines[i];
    }
    if (n) memcpy(&badLines[kept], mine, (size_t)n * sizeof *mine);
    badLineCount = kept + n;
    free(mine);
    if (had) leftOutVersion++;
    return 1;
}

// After a reading of the mod's files that `now` describes: remembers it for the next reading.
static void rememberSettingsRead(const SettingsRead *now) {
    SettingsRead *e = settingsReadOf(now->mod);
    if (!e) {
        if (!growList((void **)&settingsReads, &settingsReadCapacity, settingsReadCount + 1, sizeof settingsReads[0])) return;
        e = &settingsReads[settingsReadCount++];
        memset(e, 0, sizeof *e);
    }
    SettingRef *refs = e->sidecarRefs;
    int refCapacity = e->sidecarRefCapacity;
    *e = *now;
    e->sidecarRefs = refs;
    e->sidecarRefCapacity = refCapacity;
    for (int i = 0; i < reading.refCount; i++)
        if (!reading.refs[i].inSidecar) return;
    if (reading.refCount && !growList((void **)&e->sidecarRefs, &e->sidecarRefCapacity, reading.refCount, sizeof e->sidecarRefs[0])) return;
    if (reading.refCount) memcpy(e->sidecarRefs, reading.refs, (size_t)reading.refCount * sizeof e->sidecarRefs[0]);
    e->sidecarRefCount = reading.refCount;
    for (int i = 0; i < badLineCount; i++) e->fileBadLines += !_stricmp(badLines[i].mod, now->mod);
    e->valid = 1;
}

// ---- The game's reading of the settings blocks ---------------------------------------------------
// Settings block lines the game's reader passes to the .c5m line site.
static struct {
    int open;
    int mod, level, inSidecar, line;
} gameSettingBlock;

// Returns 1 when it took the line, a line of a settings block of mod `mod`, read at `level`.
static int settingLineHook(const char *line, size_t len, int mod, int level) {
    int number = G32(RVA_MOD_LINE), inSidecar = readingSidecarNow();
    if (mod != gameSettingBlock.mod || level != gameSettingBlock.level || inSidecar != gameSettingBlock.inSidecar ||
        number <= gameSettingBlock.line)
        gameSettingBlock.open = 0;
    gameSettingBlock.mod = mod;
    gameSettingBlock.level = level;
    gameSettingBlock.inSidecar = inSidecar;
    gameSettingBlock.line = number;
    char word[24];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    if (isSettingBlockStart(word)) {
        gameSettingBlock.open = 1;
        if (!inSidecar && level == 1 && reading.active && !reading.disabled && mod >= 0 && mod <= G32(RVA_MOD_LAST_INDEX)) {
            SettingProblem p;
            settingProblem(&p, "setting_in_c5m", "settings go in %.32s%s, not the .c5m", modName(mod), SIDECAR_SUFFIX);
            rejectC5mLine(modName(mod), word, number, &p, line);
        }
        return 1;
    }
    if (gameSettingBlock.open && isSettingBlockWord(word)) return 1;
    gameSettingBlock.open = 0;
    return 0;
}

// Starts `mod`'s settings afresh.
static void resetModSettings(const char *mod) {
    forgetSettingsRead(mod);
    for (int i = 0; i < settingCount; i++)
        if (!_stricmp(settings[i]->mod, mod)) settings[i]->declared = settings[i]->value = 0;
    int kept = 0;
    for (int i = 0; i < badLineCount; i++)
        if (_stricmp(badLines[i].mod, mod)) badLines[kept++] = badLines[i];
    if (kept != badLineCount) leftOutVersion++;
    badLineCount = kept;
    kept = 0;
    for (int i = 0; i < modPageCount; i++)
        if (_stricmp(modPages[i].mod, mod)) modPages[kept++] = modPages[i];
    modPageCount = kept;
}

// ---- A mod's reading -----------------------------------------------------------------------------
// Starts a new reading of `mod`.
static void startReading(const char *mod) {
    SettingRef *refs = reading.refs;
    int refCapacity = reading.refCapacity;
    memset(&reading, 0, sizeof reading);
    reading.refs = refs;
    reading.refCapacity = refCapacity;
    reading.active = 1;
    snprintf(reading.mod, sizeof reading.mod, "%s", mod);
    passReadings++;
    reading.disabled = !modEnabledByName(mod);
    SettingsRead now;
    int known = !reading.disabled && settingsReadNow(mod, &now);
    if (known && keepSettingsRead(&now)) return;
    resetModSettings(mod);
    if (reading.disabled) return;
    readSettingsFile(mod);
    if (known) rememberSettingsRead(&now);
}

// Ends the current reading: each +setting line must name one of the mod's settings.
static void finishReading(void) {
    if (!reading.active) return;
    reading.active = 0;
    if (reading.disabled) return;
    SettingProblem p;
    char source[160];
    for (int i = 0; i < reading.refCount; i++) {
        const SettingRef *r = &reading.refs[i];
        const ModSetting *s = findDeclaredSetting(reading.mod, r->name);
        int max = s && s->optionCount ? s->optionCount - 1 : 1;
        snprintf(source, sizeof source, "%s %s%s", r->word, r->name, r->hasIndex ? " <index>" : "");
        if (!s) settingProblem(&p, "unknown_setting", "%s %.32s: no such setting in this mod", r->word, r->name);
        else if (s->rejected) continue;
        else if (r->trailing) settingProblem(&p, "trailing_text", "%s %.32s: text after the index", r->word, r->name);
        else if (s->optionCount && !r->hasIndex)
            settingProblem(&p, "no_index", "%s %.32s: a choice needs an index (0 to %d)", r->word, r->name, max);
        else if (r->hasIndex && s->isKey && !keyValueIsValid(r->index))
            settingProblem(&p, "index_out_of_range", "%s %.32s: %d is not a key value", r->word, r->name, r->index);
        else if (r->hasIndex && !s->isKey && (r->index < 0 || r->index > max))
            settingProblem(&p, "index_out_of_range", "%s %.32s: index %d is not 0 to %d", r->word, r->name, r->index, max);
        else continue;
        if (r->inSidecar) rejectSidecarLine(reading.mod, r->word, r->line, &p, source);
        else rejectC5mLine(reading.mod, r->word, r->line, &p, source);
    }
}

// Called for every line of a .c5m at level 1.
static void noteModLine(const char *mod, int level) {
    int number = G32(RVA_MOD_LINE);
    if (level != 1 || number < 1 || !mod[0]) return;
    if (!reading.active || _stricmp(reading.mod, mod) || number < reading.line) {
        finishReading();
        startReading(mod);
    } else if (number == reading.line) {
        return;
    }
    reading.line = number;
}

// ---- Readings outside the mod passes -------------------------------------------------------------
// Reads the settings files of every enabled mod, or only those not tried yet.
static void readEnabledSettingsFiles(int all) {
    finishReading();
    int read = 0;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modEnabled(m) || !modName(m)[0] || (!all && settingsFileTried(modName(m)))) continue;
        resetModSettings(modName(m));
        readSettingsFile(modName(m));
        read++;
    }
    if (read) writeLog("settings_files_read", "mods=%d all=%d", read, all);
}

// At the end of startup (finishStartup): the enabled mods whose files no mod pass read.
static void readUnreadSettingsFiles(void) {
    readEnabledSettingsFiles(0);
}

// Around the Mods screen: reads every enabled mod's files again when needed.
static unsigned int settingsPassCount(void) { return passReadings; }
static void rereadSettingsWithoutPass(unsigned int before) {
    if (passReadings == before) readEnabledSettingsFiles(1);
}

// api->setting for the plugin whose mod is `mod` and whose DLL is in `dir`.
static int noSettingValue;
static const int *pluginSetting(void *caller, const char *mod, const char *dir, const char *name) {
    SettingProblem p;
    if (checkSettingName("name", name, &p)) {
        noSettingValue = 0;
        return (refuseApiCall(caller, "setting", p.reason, "name=%s mod=%s", name ? name : "", mod), &noSettingValue);
    }
    finishReading();
    if (!settingsFileTried(mod)) {
        resetModSettings(mod);
        readSettingsIn(mod, dir);
    }
    int created;
    ModSetting *s = settingEntry(mod, name, &created);
    if (!s) {
        noSettingValue = 0;
        return (refuseApiCall(caller, "setting", "out_of_memory", "name=%s mod=%s", name, mod), &noSettingValue);
    }
    if (!s->declared) refuseApiCall(caller, "setting", "not_declared", "name=%s mod=%s", name, mod);
    return &s->value;
}

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

// Tells the player, in the game's message box, that key `key` is `holder`'s, and asks whether to move it to
// `s`, leaving `holder` with no key; a fixed holder's key can't move.
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
    unsigned int spellPasses = spellPassCount();
    nextModsRun(level);
    finishReading();
    spellsAfterPass(level, spellPasses);
    if (level != 3) popupSettingErrors();
}

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
    else nextEventLine(line, level);
}

// Adds the +setting answer, the mod pass wrapper, and the Mod Settings button.
static void modSettingsInit(void) {
    int ok = wrapSite(SITE_EVENT_LINE, (void *)eventLineHook, (void **)&nextEventLine) +
             wrapSite(SITE_MODS_RUN, (void *)modsRunHook, (void **)&nextModsRun) +
             api->addMainMenuButtonAfter("Mod Settings", settingsScreen, MAIN_MENU_MODS);
    writeLog("mod_settings_installed", "ok=%d of 3", ok);
}
