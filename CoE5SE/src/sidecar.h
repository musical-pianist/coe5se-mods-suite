// A mod's sidecar file, <ModName>.c5se next to <ModName>.c5m: the lines that need CoE5SE.

static void finishSpellBlock(void);
static void maxdataReadingAt(int mod, int start);
static const char *findModFile(const char *folder, const char *spec, const char *const endings[2], const char *const reasons[4], char *path,
                               size_t size);

#define SIDECAR_ANNOUNCE_MAX 64

static int (*nextRunOneMod)(int mod, int level);
static int (*nextModPath)(char *buf, size_t size, const char *fmt, ...);
static char *(*nextModLineRead)(char *buf, int size, void *file);
static void (*modErrorBox)(const char *fmt, ...);

typedef struct { char file[INCLUDE_FILE_MAX + 1]; } IncludedFile;

static struct {
    int installed;
    int reading;
    int prefixedLines;
    int prefixedLine;
    int mod;
    int lineMod, lineLevel;
    int including;
    int includeLabel;
    char includeFile[INCLUDE_FILE_MAX + 1];
    unsigned int file;
    IncludedFile *included;
    int includedCount, includedCapacity;
    unsigned int announced;
    char seen[SIDECAR_ANNOUNCE_MAX][MOD_NAME_MAX];
} sidecar;

// Whether CoE5SE is reading mod `mod`'s sidecar right now.
static int readingSidecarOf(int mod) {
    return sidecar.reading && sidecar.mod == mod;
}

static int readingSidecarNow(void) {
    return sidecar.reading;
}

// Whether the line the game is handling is a #!@ line of a .c5m, which may hold what a sidecar may.
static int readingPrefixedLine(void) {
    return sidecar.prefixedLine;
}

// Which file the game is reading: a number that changes at each file's start and each include's end.
static unsigned int modFileNow(void) {
    return sidecar.file;
}

// The name of the mod whose included file the game reads now, else NULL.
static const char *includingModName(void) {
    return sidecar.including ? modName(sidecar.mod) : NULL;
}

// The includeLabelIndex of the file the game reads now, -1 outside an included file.
static int includedLabelNow(void) {
    return sidecar.including ? sidecar.includeLabel : -1;
}

// Turns "<folder>/<name>.c5m" in buf into "<folder>/<name>.c5se".
static int sidecarPathFromModPath(char *buf, size_t size) {
    size_t n = strlen(buf), old = sizeof MOD_FILE_SUFFIX - 1, add = sizeof SIDECAR_SUFFIX - 1;
    if (n < old || _stricmp(buf + n - old, MOD_FILE_SUFFIX)) return 0;
    if (n - old + add + 1 > size) return 0;
    memcpy(buf + n - old, SIDECAR_SUFFIX, add + 1);
    return 1;
}

// The game's snprintf(buf, 2500, "%s/%s.c5m", folder, name) in run_one_mod.
static int modPathHook(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *folder = va_arg(ap, const char *);
    const char *name = va_arg(ap, const char *);
    va_end(ap);
    if (sidecar.including) {
        int n = snprintf(buf, size, "%s/%s", folder, sidecar.includeFile);
        if (n < 0 || (size_t)n >= size) {
            refuseC5mLine(INCLUDE_WORD, name, "include_path_too_long", "folder=%.200s file=%s", folder, sidecar.includeFile);
            buf[0] = '\0';
        }
        return n;
    }
    int n = nextModPath(buf, size, fmt, folder, name);
    if (sidecar.reading && !sidecarPathFromModPath(buf, size)) {
        refuseC5mLine(SIDECAR_SUFFIX, name, "sidecar_path_too_long", "folder=%.200s", folder);
        buf[0] = '\0';
    }
    return n;
}

// Finds `file`, an include line's, in the folder of the mod named `name`: NULL, or the refusal's reason.
static const char *findIncludedFileIn(const char *folder, const char *name, const char *file, char *path, size_t size) {
    static const char *const ENDINGS[2] = {SIDECAR_SUFFIX, SIDECAR_SUFFIX};
    static const char *const REASONS[4] = {"bad_file", "file_not_found", "file_wrong_case", "file_link"};
    char own[MOD_NAME_MAX + sizeof SIDECAR_SUFFIX], dotted[INCLUDE_FILE_MAX + 3];
    snprintf(own, sizeof own, "%s" SIDECAR_SUFFIX, name);
    if (!_stricmp(file, own)) return "own_file";
    if (strlen(file) > INCLUDE_FILE_MAX) return "bad_file";
    snprintf(dotted, sizeof dotted, "./%s", file);
    return findModFile(folder, dotted, ENDINGS, REASONS, path, size);
}

static const char *findIncludedFile(int mod, const char *file, char *path, size_t size) {
    return findIncludedFileIn(modFolder(mod), modName(mod), file, path, size);
}

// Logs the refusal of mod `mod`'s include line, then shows the game's Mod Error box, which names its file and line.
static PRINTF_FORMAT(5, 6) void refuseIncludeLine(int mod, int ahead, const char *reason, const char *file, const char *fmt, ...) {
    char text[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    int line = G32(RVA_MOD_LINE);
    refuseC5mLine(INCLUDE_WORD, modName(mod), reason, "line=%d include=%.200s", line + ahead, file);
    G32(RVA_MOD_LINE) = line + ahead;
    if (modErrorBox) modErrorBox("CoE5SE: %s", text);
    G32(RVA_MOD_LINE) = line;
}

// Has the game read `file` of mod `mod` at `level`, where its include line stands.
static void readIncludedFile(int mod, int level, const char *file) {
    char outer[GAME_PATH_SIZE];
    memcpy(outer, GAME_PTR(RVA_EVENT_FILE), sizeof outer);
    int line = G32(RVA_MOD_LINE), reading = sidecar.reading, owner = sidecar.mod;
    finishSpellBlock();
    sidecar.reading = sidecar.including = 1;
    sidecar.mod = mod;
    sidecar.includeLabel = includeLabelIndex(file);
    snprintf(sidecar.includeFile, sizeof sidecar.includeFile, "%s", file);
    sidecar.file++;
    int read = nextRunOneMod(mod, level);
    finishSpellBlock();
    sidecar.reading = reading, sidecar.including = 0, sidecar.mod = owner;
    sidecar.file++;
    memcpy(GAME_PTR(RVA_EVENT_FILE), outer, sizeof outer);
    G32(RVA_MOD_LINE) = line;
    if (read <= 0) writeLog("include_not_read", "mod=%s file=%s level=%d", modName(mod), file, level);
}

// An include line of mod `mod`'s sidecar or #!@ line of its .c5m, read at `level`: reads its file, or refuses it.
static void takeIncludeLine(int mod, int level, int kind, const char *file) {
    char path[MAX_PATH];
    const char *name = modName(mod);
    if (kind < 0) {
        refuseIncludeLine(mod, 1, "bad_include_line", "-", "Include needs one quoted file: include \"folder/file%s\"", SIDECAR_SUFFIX);
        return;
    }
    if (G32(RVA_EVENT_STATE)) {
        refuseIncludeLine(mod, 1, "include_in_event", file, "Include inside an event");
        return;
    }
    const char *reason = findIncludedFile(mod, file, path, sizeof path);
    if (reason && !strcmp(reason, "own_file")) {
        refuseIncludeLine(mod, 1, reason, file, "Include of %.60s%s, which is read already", name, SIDECAR_SUFFIX);
        return;
    }
    if (reason && (!strcmp(reason, "file_not_found") || !strcmp(reason, "file_wrong_case"))) {
        refuseIncludeLine(mod, 1, reason, file, "Include file '%.120s' not found%s", file, !strcmp(reason, "file_wrong_case") ? " (case differs)" : "");
        return;
    }
    if (reason && !strcmp(reason, "file_link")) {
        refuseIncludeLine(mod, 1, reason, file, "Include file '%.120s' is a link", file);
        return;
    }
    if (reason) {
        refuseIncludeLine(mod, 1, reason, file, "Bad include file '%.120s': a %s in the mod's folder, letters, digits, _ - . and / only, no ..", file,
                          SIDECAR_SUFFIX);
        return;
    }
    for (int i = 0; i < sidecar.includedCount; i++)
        if (!strcmp(sidecar.included[i].file, file)) {
            refuseIncludeLine(mod, 1, "duplicate_include", file, "Include file '%.120s' included already", file);
            return;
        }
    if (growList((void **)&sidecar.included, &sidecar.includedCapacity, sidecar.includedCount + 1, sizeof *sidecar.included))
        snprintf(sidecar.included[sidecar.includedCount++].file, sizeof sidecar.included[0].file, "%s", file);
    readIncludedFile(mod, level, file);
}

// The game's fgets(buf, 50000, file) of each line in run_one_mod.
static char *modLineReadHook(char *buf, int size, void *file) {
    char *got = nextModLineRead(buf, size, file);
    sidecar.prefixedLine = 0;
    if (!got || !sidecar.prefixedLines || sidecar.including) return got;
    const char *text = buf;
    if (!sidecar.reading) {
        text = sidecarLineText(buf);
        if (!text) return got;
        memset((char *)text - (sizeof SIDECAR_LINE_PREFIX - 1), ' ', sizeof SIDECAR_LINE_PREFIX - 1);
        sidecar.prefixedLine = 1;
    }
    char include[INCLUDE_FILE_MAX + 2];
    int kind = readIncludeLine(text, include, sizeof include);
    if (!kind) return got;
    takeIncludeLine(sidecar.lineMod, sidecar.lineLevel, kind, include);
    sidecar.prefixedLine = 0;
    snprintf(buf, (size_t)size, "\n");
    return got;
}

// include in a file that can't include: an included file, or a plain line of the .c5m.
static void includeWordLine(const char *args, int mod, int level) {
    if (level != 2 || mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return;
    char file[INCLUDE_FILE_MAX + 2] = "-";
    char line[INCLUDE_FILE_MAX + 16];
    snprintf(line, sizeof line, INCLUDE_WORD " %.*s", INCLUDE_FILE_MAX + 4, args);
    if (readIncludeLine(line, file, sizeof file) < 0) snprintf(file, sizeof file, "-");
    const char *name = modName(mod);
    refuseIncludeLine(mod, 0, "include_not_here", file, "Include only in %.60s%s or on %s lines of %.60s%s", name, SIDECAR_SUFFIX, SIDECAR_LINE_PREFIX, name,
                      MOD_FILE_SUFFIX);
}

static int sidecarExists(int mod) {
    char path[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s" SIDECAR_SUFFIX, modFolder(mod), modName(mod))) return 0;
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

// Logs sidecar_read the first time mod `mod`'s sidecar is read this launch.
static void announceSidecar(int mod) {
    const char *name = modName(mod);
    for (unsigned int i = 0; i < sidecar.announced; i++)
        if (!_stricmp(sidecar.seen[i], name)) return;
    if (sidecar.announced < SIDECAR_ANNOUNCE_MAX) snprintf(sidecar.seen[sidecar.announced++], MOD_NAME_MAX, "%s", name);
    writeLog("sidecar_read", "mod=%s file=%s" SIDECAR_SUFFIX, name, name);
}

// Reads mod `mod`'s .c5m at `level`, then at level 2 to 4 its sidecar the same way.
static int readModAndSidecar(int mod, int level) {
    int capped = sidecar.installed && level == 2 && !sidecar.reading && mod >= 0 && mod <= G32(RVA_MOD_LAST_INDEX);
    if (capped) maxdataReadingAt(mod, 1);
    sidecar.file++;
    int result = nextRunOneMod(mod, level);
    if (!sidecar.installed || level < 2 || result <= 0 || sidecar.reading || mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) {
        if (capped) maxdataReadingAt(mod, -1);
        return result;
    }
    if (!sidecarExists(mod)) {
        if (capped) maxdataReadingAt(mod, 0);
        return result;
    }
    finishSpellBlock();
    sidecar.reading = 1;
    sidecar.mod = mod;
    sidecar.file++;
    int read = nextRunOneMod(mod, level);
    finishSpellBlock();
    sidecar.reading = 0;
    if (capped) maxdataReadingAt(mod, 0);
    if (read > 0) announceSidecar(mod);
    return result;
}

static void noteModContent(int mod);

// run_one_mod(mod, level): the .c5m and the sidecar.
static int runOneModHook(int mod, int level) {
    int outer = sidecar.prefixedLines, outerMod = sidecar.lineMod, outerLevel = sidecar.lineLevel;
    sidecar.prefixedLines = level >= 2;
    sidecar.lineMod = mod, sidecar.lineLevel = level;
    sidecar.includedCount = 0;
    int result = readModAndSidecar(mod, level);
    sidecar.prefixedLines = outer;
    sidecar.lineMod = outerMod, sidecar.lineLevel = outerLevel;
    sidecar.prefixedLine = 0;
    if (level == 2 && result > 0) noteModContent(mod);
    return result;
}

// Wraps run_one_mod's calls and its path, and adds the include word for the lines that can't include.
static void sidecarInit(void) {
    int ok = wrapSite(SITE_RUN_ONE_MOD, (void *)runOneModHook, (void **)&nextRunOneMod) +
             wrapSite(SITE_RUN_ONE_MOD_PATH, (void *)modPathHook, (void **)&nextModPath);
    sidecar.installed = ok == 2;
    modErrorBox = GAME_FN(void (*)(const char *, ...), RVA_MOD_ERROR);
    int lineRead = wrapSite(SITE_RUN_ONE_MOD_LINE_READ, (void *)modLineReadHook, (void **)&nextModLineRead);
    int word = apiAddModCommand("include", includeWordLine);
    writeLog("sidecar_hooks_installed", "ok=%d of 2 line_read=%d include_word=%d", ok, lineRead, word);
}
