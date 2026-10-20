// A mod's sidecar file, <ModName>.c5se next to <ModName>.c5m: the lines that need CoE5SE.

static void finishSpellBlock(void);
static void maxdataReadingAt(int mod, int start);

#define SIDECAR_ANNOUNCE_MAX 64

static int (*nextRunOneMod)(int mod, int level);
static int (*nextModPath)(char *buf, size_t size, const char *fmt, ...);

static struct {
    int installed;
    int reading;
    int mod;
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
    int n = nextModPath(buf, size, fmt, folder, name);
    if (sidecar.reading && !sidecarPathFromModPath(buf, size)) {
        refuseC5mLine(SIDECAR_SUFFIX, name, "sidecar_path_too_long", "folder=%.200s", folder);
        buf[0] = '\0';
    }
    return n;
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
    int result = nextRunOneMod(mod, level);
    if (!sidecar.installed || level < 2 || result <= 0 || sidecar.reading || mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return result;
    if (!sidecarExists(mod)) {
        if (level == 2) maxdataReadingAt(mod, 0);
        return result;
    }
    finishSpellBlock();
    if (level == 2) maxdataReadingAt(mod, 1);
    sidecar.reading = 1;
    sidecar.mod = mod;
    int read = nextRunOneMod(mod, level);
    finishSpellBlock();
    sidecar.reading = 0;
    if (level == 2) maxdataReadingAt(mod, 0);
    if (read > 0) announceSidecar(mod);
    return result;
}

static void noteModContent(int mod);

// run_one_mod(mod, level): the .c5m and the sidecar.
static int runOneModHook(int mod, int level) {
    int result = readModAndSidecar(mod, level);
    if (level == 2 && result > 0) noteModContent(mod);
    return result;
}

// Wraps run_one_mod's calls and its path.
static void sidecarInit(void) {
    int ok = wrapSite(SITE_RUN_ONE_MOD, (void *)runOneModHook, (void **)&nextRunOneMod) +
             wrapSite(SITE_RUN_ONE_MOD_PATH, (void *)modPathHook, (void **)&nextModPath);
    sidecar.installed = ok == 2;
    writeLog("sidecar_hooks_installed", "ok=%d of 2", ok);
}
