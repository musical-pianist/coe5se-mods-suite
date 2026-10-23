// The Mod Settings screen and the mods' settings files.

#include <ctype.h>

// The sidecar's words that start a block (the game has none of them).
#define SETTING_WORD        "newsetting"
#define SETTING_PAGE_WORD   "newsettingpage"
#define SETTING_HEADER_WORD "newsettingheader"

// Which file a settings line is in: IniParse.sidecar, and the inSidecar of each setting, page, and ref.
#define SETTINGS_IN_INI   0
#define SETTINGS_IN_C5SE  1
#define SETTINGS_IN_C5M   2

static const char *settingsFileSuffix(int in) {
    return in == SETTINGS_IN_C5M ? MOD_FILE_SUFFIX : in ? SIDECAR_SUFFIX : ".ini";
}

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
static PRINTF_FORMAT(5, 6) void refuseSidecarLine(int in, const char *word, const char *mod, const char *reason, const char *fmt, ...) {
    char details[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    if (in == SETTINGS_IN_C5M) refuseC5mLine(word, mod, reason, "%s", details);
    else logSidecarRefusal(word, mod, reason, details, mod);
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
