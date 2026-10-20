// What the stub does on both platforms (glu32_stub.c, mac_stub.c). Players copy the stub once, so
// it does only what must never change and hands the rest to the loader (coe5se_loader.c):
//   1. Finds the game's user folder by the game's own rule (approvals.h, findGameUserDir), so it
//      reads the coe5config the game reads, COE5_SAVE included.
//   2. Finds CoE5SE's folder (findCoe5seFolder), reading only.
//   3. Loads nothing unless CoE5SE is enabled: a disabled mod's code never runs. Once per player
//      it turns CoE5SE on first (enableCoe5seOnce); after that, an untick stays.
//   4. Loads the loader and calls coe5seLoaderStart (loader_start.h).
// When a step fails, it logs loader_inactive with the reason and the game runs vanilla. Its log
// lines name folders by their last part only, so a player's user name stays out of the log.
// Include after approve_dll.h, auto_enable.h, coe5config.h, and loader_start.h.

#pragma once

// The game's folder, which startStubLog sets.
static char gameDir[MAX_PATH];

// 1 if an import of the DLL `name` may hold SDL's SDL_SetMainReady (glu32_stub.c,
// deferToMainReady): any name starting with SDL in any case, so SDL3.dll counts as SDL2.dll does.
static __attribute__((unused)) int isSdlImportName(const char *name) {
    return !_strnicmp(name, "SDL", 3);
}

// The last part of the path `path`, for log lines.
static const char *lastPathPart(const char *path) {
    const char *slash = strrchr(path, USER_DIR_SEP[0]);
    return slash ? slash + 1 : path;
}

// Copies the system's error text `text` (dlopen's names the paths it tried) to out[size], with each
// path in it cut to its last part, so a log line keeps the error and loses the user name. A path
// starts with / or a drive letter and a slash, at the text's start or after a space, (, ', or ", and
// runs to a comma, quote, ), or the end.
static void shortenPathsIn(const char *text, char *out, size_t size) {
    size_t n = 0;
    for (const char *p = text; *p && n + 1 < size;) {
        int drive = ((p[0] | 0x20) >= 'a' && (p[0] | 0x20) <= 'z' && p[1] == ':' && (p[2] == '\\' || p[2] == '/'));
        if ((*p != '/' && !drive) || (p > text && !strchr(" ('\"", p[-1]))) {
            out[n++] = *p++;
            continue;
        }
        const char *last = p;
        for (; *p && !strchr(",'\")", *p); p++)
            if (*p == '/' || *p == '\\') last = p + 1;
        while (last < p && n + 1 < size) out[n++] = *last++;
    }
    out[n] = '\0';
}

// Looks in every folder under `root` for CoE5SE.c5m. Returns how many have it, and writes the first
// one's path to `dir`. A folder whose path doesn't fit counts too, since it can't be ruled out.
static int countCoe5seFolders(const char *root, char *dir, size_t size) {
    char pattern[MAX_PATH], sub[MAX_PATH], c5m[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", root)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int count = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') continue;
        int fits = buildPath(sub, sizeof sub, __func__, "%s" USER_DIR_SEP "%s", root, fd.cFileName) &&
                   buildPath(c5m, sizeof c5m, __func__, "%s" USER_DIR_SEP COE5SE_MOD ".c5m", sub);
        if (fits && GetFileAttributesA(c5m) == INVALID_FILE_ATTRIBUTES) continue;
        if (!count++) snprintf(dir, size, "%s", fits ? sub : "");
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}

// Counts CoE5SE's Workshop items (COE5SE_WORKSHOP_IDS) under `root` that hold CoE5SE.c5m, and writes
// the first one's path to `dir`. Every other item is never looked at, so a fork or a re-upload with a
// CoE5SE.c5m can't stand in for CoE5SE or turn it off. A folder whose path doesn't fit counts too.
static int countCoe5seItems(const char *root, char *dir, size_t size) {
    char sub[MAX_PATH], c5m[MAX_PATH];
    int count = 0;
    for (int i = 0; i < COE5SE_WORKSHOP_ID_COUNT; i++) {
        int fits = buildPath(sub, sizeof sub, __func__, "%s" USER_DIR_SEP "%s", root, COE5SE_WORKSHOP_IDS[i]) &&
                   buildPath(c5m, sizeof c5m, __func__, "%s" USER_DIR_SEP COE5SE_MOD ".c5m", sub);
        if (fits && GetFileAttributesA(c5m) == INVALID_FILE_ATTRIBUTES) continue;
        if (!count++) snprintf(dir, size, "%s", fits ? sub : "");
    }
    return count;
}

// Picks CoE5SE's folder and writes it to `dir`. Returns "local" or "workshop", or NULL with the
// reason in *reason. A folder in the user folder's mods wins, as the game enables the local copy of
// a name first; with none, one of CoE5SE's Workshop items. Two copies on one side load none.
static const char *findCoe5seFolder(char *dir, size_t size, const char **reason) {
    char root[MAX_PATH];
    int local = buildPath(root, sizeof root, __func__, "%s" USER_DIR_SEP "mods", userDir()) ? countCoe5seFolders(root, dir, size) : 0;
    int workshop = 0;
    if (!local) workshop = findWorkshopRoot(gameDir, root, sizeof root) ? countCoe5seItems(root, dir, size) : 0;
    writeLog("stub_coe5se_folders", "local=%d workshop=%d", local, workshop);
    if (local + workshop > 1) *reason = "clash";
    else if (local + workshop == 0) *reason = "no_coe5se_mod";
    else if (!dir[0]) *reason = "path_too_long";
    else return local ? "local" : "workshop";
    return NULL;
}

// Step 3's first part: turns CoE5SE on in coe5config, once per player (auto_enable.h). `source` is
// what findCoe5seFolder returned, and `folderReason` why it returned NULL. Skipped without a mark
// when CoE5SE's folder or coe5config isn't there yet, so a later launch tries again. The done file
// is written before coe5config changes and removed only when the change failed, so a launch that
// stops part way never lets a later one turn CoE5SE on after the player turned it off.
static void enableCoe5seOnce(const char *config, const char *layout, const char *source, const char *folderReason) {
    char done[MAX_PATH], notice[MAX_PATH], backup[MAX_PATH], oldDone[MAX_PATH];
    if (!coe5seUserFile(AUTO_ENABLE_DONE_FILE, done, sizeof done) || !coe5seUserFile(AUTO_ENABLE_NOTICE_FILE, notice, sizeof notice) ||
        !coe5seUserFile(AUTO_ENABLE_BACKUP_FILE, backup, sizeof backup)) {
        writeLog("coe5se_auto_enable_skipped", "reason=path_too_long");
        return;
    }
    // No log: CoE5SE was turned on, or found on, before; under a stub up to version 7 with COE5_SAVE
    // set, that left its done file in the old folder (approvals.h, oldUserFile).
    if (autoEnableFileExists(done) || oldUserFile(COE5SE_USER_DIR, AUTO_ENABLE_DONE_FILE, oldDone, sizeof oldDone)) return;
    if (configEnablesMod(config, COE5SE_MOD, layout)) {
        int marked = writeAutoEnableFile(done, AUTO_ENABLE_DONE_TEXT);
        writeLog("coe5se_auto_enable_skipped", "reason=already_enabled marked=%d", marked);
        return;
    }
    if (!source) {
        writeLog("coe5se_auto_enable_skipped", "reason=%s", folderReason);
        return;
    }
    if (!writeAutoEnableFile(done, AUTO_ENABLE_DONE_TEXT)) {
        remove(done);
        writeLog("coe5se_auto_enable_skipped", "reason=done_write_failed marked=%d", autoEnableFileExists(done));
        return;
    }
    const char *reason = addModToConfigFile(config, backup, COE5SE_MOD);
    if (reason) {
        remove(done);
        writeLog("coe5se_auto_enable_skipped", "reason=%s marked=%d", reason, autoEnableFileExists(done));
        return;
    }
    writeAutoEnableFile(notice, AUTO_ENABLE_NOTICE_TEXT);   // No log: without it CoE5SE only doesn't say it turned itself on
    writeLog("coe5se_auto_enabled", "mod=" COE5SE_MOD " backup=%s", lastPathPart(backup));
}

// Steps 1 to 4 above. `startedFrom` is one of loader_start.h's LOADER_STARTED_* values, and
// `stubVersion` the stub's own version. Returns the reason nothing loaded, or NULL once the loader
// has run.
static const char *startLoader(uint32_t startedFrom, uint32_t stubVersion) {
    char config[MAX_PATH], layout[MAX_PATH] = "", dir[MAX_PATH] = "", path[MAX_PATH], error[256];
    const char *reason = "no_user_dir";
    if (!findGameUserDir(userDirPath, sizeof userDirPath, &reason)) return reason;
    if (!buildPath(config, sizeof config, __func__, "%s" USER_DIR_SEP CONFIG_FILE, userDirPath)) return "path_too_long";
    const char *folderReason = "no_coe5se_mod";
    const char *source = findCoe5seFolder(dir, sizeof dir, &folderReason);
    // The layout file comes with the CoE5SE that would run; without a folder, the name search.
    if (source) buildPath(layout, sizeof layout, __func__, "%s" USER_DIR_SEP CONFIG_LAYOUT_FILE, dir);   // Logged by buildPath: then no layout file
    enableCoe5seOnce(config, layout[0] ? layout : NULL, source, folderReason);
    if (!configEnablesMod(config, COE5SE_MOD, layout[0] ? layout : NULL)) return "no_coe5se_mod";
    if (!source) return folderReason;
    writeLog("stub_coe5se_folder", "source=%s dir=%s", source, lastPathPart(dir));
    if (!buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP LOADER_DLL_NAME, dir)) return "path_too_long";
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return "loader_missing";
    // Logged before the load, so if its start code crashes, the log's last line names it.
    writeLog("loader_plugin_loading", "name=" LOADER_DLL_NAME " source=%s", source);
    void *module = loadDllFile(path, error, sizeof error);
    if (!module) {
        char shortError[sizeof error];
        shortenPathsIn(error, shortError, sizeof shortError);
        writeLog("loader_plugin_failed", "name=" LOADER_DLL_NAME " source=%s error=%s", source, shortError);
        return "loader_load_failed";
    }
#ifdef _WIN32
    LoaderStartFn entry = (LoaderStartFn)(void *)GetProcAddress((HMODULE)module, LOADER_ENTRY_NAME);
#else
    LoaderStartFn entry = (LoaderStartFn)dlsym(module, LOADER_ENTRY_NAME);
#endif
    if (!entry) return "no_loader_entry";
    LoaderStart start = {sizeof start, LOADER_START_VERSION, stubVersion, startedFrom, dir, source, "", userDirPath};
    int result = entry(&start);
    writeLog("stub_loader_returned", "result=%d", result);
    return NULL;
}

// Runs startLoader and logs why nothing loaded, if nothing did.
static void runLoader(uint32_t startedFrom, uint32_t stubVersion) {
    const char *reason = startLoader(startedFrom, stubVersion);
    if (reason) writeLog("loader_inactive", "reason=%s", reason);
}

// Sets gameDir and the log's path, renames a log over 1 MB to coe5mod.old.log (so the log a player
// attaches holds only recent launches), and writes each launch's first line, with the date (every
// other line has only the time). When the path doesn't fit in MAX_PATH, nothing is logged.
static void startStubLog(uint32_t stubVersion) {
    GetModuleFileNameA(NULL, gameDir, sizeof gameDir);
    char *slash = strrchr(gameDir, USER_DIR_SEP[0]);
    if (slash) *slash = '\0';
    char path[MAX_PATH], old[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "coe5mod.log", gameDir) ||
        !buildPath(old, sizeof old, __func__, "%s" USER_DIR_SEP "coe5mod.old.log", gameDir)) return;
    struct stat st;
    if (!stat(path, &st) && st.st_size >= 1024 * 1024) {
        remove(old);
        rename(path, old);
    }
    memcpy(logPath, path, sizeof logPath);
    SYSTEMTIME t;
    GetLocalTime(&t);
    writeLog("loader_attached", "date=%04d-%02d-%02d loader_version=%u", t.wYear, t.wMonth, t.wDay, stubVersion);
}
