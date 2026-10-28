// Mods' folders and code, and the restart prompt after the Mods screen when a mod with a DLL changed.

// ---- Restart prompt after the Mods screen -----------------------------------------------------
// Turning a mod with a DLL on or off takes effect after a restart.
static struct CodeMod { char folder[MAX_PATH]; int enabled; } *codeMods;
static int codeModCount, codeModCapacity, codeModSnapshotFailed;
static u64 (*nextModsScreen)(u64, u64, u64, u64);

// A mod's file names: <ModName>.c5m and its sidecar, <ModName>.c5se.
#define MOD_FILE_SUFFIX ".c5m"
#define SIDECAR_SUFFIX  ".c5se"

// A .c5m line that starts with #!@ holds a line the sidecar could hold.
#define SIDECAR_LINE_PREFIX "#!@"

// Where the text of a #!@ line starts, past the prefix, or NULL when `line` isn't one.
static const char *sidecarLineText(const char *line) {
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    size_t n = sizeof SIDECAR_LINE_PREFIX - 1;
    return strncmp(line, SIDECAR_LINE_PREFIX, n) ? NULL : line + n;
}

// include "<file>.c5se": a line of a mod's .c5se, or a #!@ line of its .c5m, that has the file read where it stands.
#define INCLUDE_WORD     "include"
#define INCLUDE_FILE_MAX 198

// What `line` is: 0 another line, 1 an include line with its quoted file in `file`, -1 one written wrong.
static int readIncludeLine(const char *line, char *file, size_t size) {
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    size_t n = sizeof INCLUDE_WORD - 1;
    if (strncmp(line, INCLUDE_WORD, n)) return 0;
    const char *p = line + n;
    if (*p && *p != ' ' && *p != '\t' && *p != '"' && *p != '\r' && *p != '\n' && *p != '#') return 0;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    const char *end = strchr(++p, '"');
    if (!end || (size_t)(end - p) >= size) return -1;
    snprintf(file, size, "%.*s", (int)(end - p), p);
    for (p = end + 1; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'; p++) {}
    return *p && *p != '#' ? -1 : 1;
}

// Included files' names as "/<file>", kept for the launch: what a message names after the mod's name.
static char **includeLabels;
static int includeLabelCount, includeLabelCapacity;

// The index of `file`'s label, or -1 when it can't be kept.
static int includeLabelIndex(const char *file) {
    for (int i = 0; i < includeLabelCount; i++)
        if (!strcmp(includeLabels[i] + 1, file)) return i;
    char *label = malloc(strlen(file) + 2);
    if (!label || !growList((void **)&includeLabels, &includeLabelCapacity, includeLabelCount + 1, sizeof *includeLabels)) {
        free(label);
        return -1;
    }
    snprintf(label, strlen(file) + 2, "/%s", file);
    includeLabels[includeLabelCount] = label;
    return includeLabelCount++;
}

// Label `index`, or the sidecar's suffix for one that couldn't be kept.
static const char *includeLabelText(int index) {
    return index >= 0 && index < includeLabelCount ? includeLabels[index] : SIDECAR_SUFFIX;
}

// The folder of mod `mod`.
static const char *modFolder(int mod) {
    return (const char *)(gameBase + RVA_MOD_FOLDERS + (size_t)mod * MOD_RECORD_SIZE);
}

static const char *modName(int mod);

static int modEnabled(int mod) {
    return *(short *)(gameBase + RVA_MOD_NAMES + (size_t)mod * MOD_RECORD_SIZE + MOD_RECORD_ENABLED) != 0;
}

// Turns mod `mod` on (1) or off (0) as the game's own writers do.
static void setModEnabled(int mod, int on) {
    *(short *)(gameBase + RVA_MOD_NAMES + (size_t)mod * MOD_RECORD_SIZE + MOD_RECORD_ENABLED) = (short)(on != 0);
}

static int folderHasPlugin(const char *dir) {
    char pattern[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*" PLUGIN_SUFFIX, dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    FindClose(h);
    return 1;
}

// Whether file <folder>/<name><suffix> of mod `mod` exists.
static int modHasFile(int mod, const char *suffix) {
    char path[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s%s", modFolder(mod), modName(mod), suffix)) return 0;
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

// A mod's commands file: <folder>/<name>_commands.lua, lua_scripts.h's to run.
#define LUA_COMMANDS_SUFFIX "_commands.lua"

static int modHasCode(int mod) {
    return folderHasPlugin(modFolder(mod));
}

static void takeCodeModSnapshot(void) {
    codeModCount = 0;
    codeModSnapshotFailed = 0;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modHasCode(m)) continue;
        if (!growList((void **)&codeMods, &codeModCapacity, codeModCount + 1, sizeof codeMods[0])) {
            codeModSnapshotFailed = 1;
            writeLog("mods_with_code_snapshot_failed", "reason=out_of_memory count=%d", codeModCount);
            return;
        }
        snprintf(codeMods[codeModCount].folder, MAX_PATH, "%s", modFolder(m));
        codeMods[codeModCount++].enabled = modEnabled(m);
    }
}

// Whether a mod with code (modHasCode) was turned on or off since the snapshot.
static int codeModsChanged(void) {
    if (codeModSnapshotFailed) return 0;
    int seen = 0;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modHasCode(m)) continue;
        int k = 0;
        while (k < codeModCount && _stricmp(codeMods[k].folder, modFolder(m))) k++;
        if (k == codeModCount) {
            if (modEnabled(m)) return 1;
            continue;
        }
        if (codeMods[k].enabled != modEnabled(m)) return 1;
        seen++;
    }
    for (int k = 0; k < codeModCount; k++)
        if (codeMods[k].enabled) seen--;
    return seen < 0;
}

// A mod with a DLL that is on now but wasn't at the snapshot: its index, or -1.
static int dllModOnSinceSnapshot(void) {
    if (codeModSnapshotFailed) return -1;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modEnabled(m) || !folderHasPlugin(modFolder(m))) continue;
        int k = 0;
        while (k < codeModCount && _stricmp(codeMods[k].folder, modFolder(m))) k++;
        if (k == codeModCount || !codeMods[k].enabled) return m;
    }
    return -1;
}

#ifdef __APPLE__
// Starts the game again and exits this copy, logging `reason`.
static void relaunchGame(const char *reason) {
    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, sizeof exe);
    pid_t pid;
    int error = posix_spawn(&pid, exe, NULL, NULL, *_NSGetArgv(), *_NSGetEnviron());
    if (error) {
        writeLog("restart_failed", "error=%d", error);
        return;
    }
    writeLog("restarting", "reason=%s", reason);
    exit(0);
}
#else
// Starts CoE5.exe again with the same command line, and exits this copy.
static void relaunchGame(const char *reason) {
    char exe[MAX_PATH], dir[MAX_PATH], cmd[2048];
    GetModuleFileNameA(NULL, exe, sizeof exe);
    snprintf(dir, sizeof dir, "%s", exe);
    char *slash = strrchr(dir, '\\');
    if (slash) *slash = '\0';
    snprintf(cmd, sizeof cmd, "%s", GetCommandLineA());
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(exe, cmd, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi)) {
        writeLog("restart_failed", "error=%lu", GetLastError());
        return;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    writeLog("restarting", "reason=%s", reason);
    ExitProcess(0);
}
#endif

// Runs after the Mods screen, and offers a restart when a mod with a DLL changed.
static u64 modsScreenWrapper(u64 a1, u64 a2, u64 a3, u64 a4) {
    unsigned int readingsBefore = settingsPassCount(), spellPassesBefore = spellPassCount();
    u64 result = nextModsScreen(a1, a2, a3, a4);
    rereadSettingsWithoutPass(readingsBefore);
    spellsAfterModsScreen(spellPassesBefore);
    if (!codeModsChanged()) return result;
    writeLog("mods_with_code_changed", "count=%d", codeModCount);
    if (messageBox("CoE5SE: Restart needed", "Mod with DLL turned on or off.\nRestart required. Restart now?",
                   "Yes", "No", 0, 0, 0) == 1)
        relaunchGame("mods_with_code_changed");
    takeCodeModSnapshot();
    return result;
}

// Writes the last folder and the file name of a path as "<folder>\<file>".
static void folderAndFile(const char *path, char *out, size_t outSize) {
    char copy[MAX_PATH];
    out[0] = '\0';
    if (!buildPath(copy, sizeof copy, __func__, "%s", path)) return;
    for (char *c = copy; *c; c++) if (*c == '/') *c = '\\';
    char *file = strrchr(copy, '\\');
    if (!file) {
        buildPath(out, outSize, __func__, "%s", copy);
        return;
    }
    *file = '\0';
    char *folder = strrchr(copy, '\\');
    buildPath(out, outSize, __func__, "%s\\%s", folder ? folder + 1 : copy, file + 1);
}

// One line of plugins_found.txt: name|source|state|path|sha256.
typedef struct {
    char name[MAX_PATH], source[16], path[MAX_PATH], hash[SHA256_HEX_LEN + 1];
    PluginState state;
} FoundLine;
#define FOUND_LINE_MAX (3 * MAX_PATH)

// Splits `line` at its |s into *out.
static int readFoundLine(char *line, FoundLine *out) {
    line[strcspn(line, "\r\n")] = '\0';
    char *field[5] = {line};
    int n = 1;
    for (char *c = line; *c && n < 5; c++)
        if (*c == '|') {
            *c = '\0';
            field[n++] = c + 1;
        }
    if (n < 4) return 0;
    snprintf(out->name, sizeof out->name, "%s", field[0]);
    snprintf(out->source, sizeof out->source, "%s", field[1]);
    out->state = pluginStateNamed(field[2]);
    snprintf(out->path, sizeof out->path, "%s", field[3]);
    snprintf(out->hash, sizeof out->hash, "%s", n > 4 ? field[4] : "");
    return 1;
}

// Whether the loader saw this plugin at launch, in any state in plugins_found.txt.
static int foundAtLaunch(const char *path) {
    typedef char FoundPath[MAX_PATH];
    static FoundPath *seen;
    static int seenCount = -1, seenCapacity;
    if (seenCount < 0) {
        seenCount = 0;
        static char line[FOUND_LINE_MAX];
        char file[MAX_PATH];
        FoundLine found;
        FILE *f = coe5seUserFile(PLUGINS_FOUND_FILE, file, sizeof file) ? fopen(file, "r") : NULL;
        while (f && fgets(line, sizeof line, f)) {
            if (!readFoundLine(line, &found)) continue;
            if (!growList((void **)&seen, &seenCapacity, seenCount + 1, sizeof seen[0])) break;
            folderAndFile(found.path, seen[seenCount], MAX_PATH);
            if (seen[seenCount][0]) seenCount++;
        }
        if (f) fclose(f);
    }
    char key[MAX_PATH];
    folderAndFile(path, key, sizeof key);
    for (int i = 0; key[0] && i < seenCount; i++)
        if (!_stricmp(seen[i], key)) return 1;
    return 0;
}

// Whether mod `mod`'s folder has a *.c5plugin.dll that was turned on after launch.
static int modHasUnloadedPlugin(int mod) {
    if (mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return 0;
    const char *dir = (const char *)(gameBase + RVA_MOD_FOLDERS + (size_t)mod * MOD_RECORD_SIZE);
    char pattern[MAX_PATH], path[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*" PLUGIN_SUFFIX, dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int unloaded = 0;
    do {
        if (!buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s", dir, fd.cFileName)) unloaded = 1;
        else unloaded = GetModuleHandleA(path) == NULL && !foundAtLaunch(path);
    } while (!unloaded && FindNextFileA(h, &fd));
    FindClose(h);
    return unloaded;
}

// Whether any enabled mod has a DLL that hasn't loaded, because it was turned on this session.
static int anyEnabledModWaitsForItsPlugin(void) {
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (modEnabled(m) && modHasUnloadedPlugin(m)) return 1;
    return 0;
}
