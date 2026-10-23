// What the stub does on both platforms (glu32_stub.c, mac_stub.c). Players copy the stub once, so
#pragma once

// The game's folder, which startStubLog sets.
static char gameDir[MAX_PATH];

// 1 if an import of the DLL `name` may hold SDL's SDL_SetMainReady (glu32_stub.c,
static __attribute__((unused)) int isSdlImportName(const char *name) {
    return !_strnicmp(name, "SDL", 3);
}

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>
#include <unistd.h>

// Points every one of the game's symbol pointers for the imported function `symbol` ("_srand") at
static __attribute__((unused)) int pointGameImportAt(const char *symbol, void *to, void **previous) {
    const struct mach_header_64 *header = NULL;
    intptr_t slide = 0;
    for (uint32_t i = 0; i < _dyld_image_count() && !header; i++) {
        const struct mach_header *h = _dyld_get_image_header(i);
        if (h && h->magic == MH_MAGIC_64 && h->filetype == MH_EXECUTE) {
            header = (const struct mach_header_64 *)h;
            slide = _dyld_get_image_vmaddr_slide(i);
        }
    }
    if (!header) return 0;
    const struct symtab_command *symtab = NULL;
    const struct dysymtab_command *dysymtab = NULL;
    const struct segment_command_64 *linkedit = NULL;
    const struct load_command *lc = (const struct load_command *)(header + 1);
    for (uint32_t i = 0; i < header->ncmds; i++, lc = (const struct load_command *)((const char *)lc + lc->cmdsize)) {
        if (lc->cmd == LC_SYMTAB) symtab = (const struct symtab_command *)lc;
        else if (lc->cmd == LC_DYSYMTAB) dysymtab = (const struct dysymtab_command *)lc;
        else if (lc->cmd == LC_SEGMENT_64 && !strncmp(((const struct segment_command_64 *)lc)->segname, "__LINKEDIT", 16))
            linkedit = (const struct segment_command_64 *)lc;
    }
    if (!symtab || !dysymtab || !linkedit || !dysymtab->nindirectsyms) return 0;
    uintptr_t base = (uintptr_t)slide + linkedit->vmaddr - linkedit->fileoff;
    const struct nlist_64 *syms = (const struct nlist_64 *)(base + symtab->symoff);
    const char *strings = (const char *)(base + symtab->stroff);
    const uint32_t *indirect = (const uint32_t *)(base + dysymtab->indirectsymoff);
    size_t page = (size_t)getpagesize();
    int changed = 0;
    lc = (const struct load_command *)(header + 1);
    for (uint32_t i = 0; i < header->ncmds; i++, lc = (const struct load_command *)((const char *)lc + lc->cmdsize)) {
        if (lc->cmd != LC_SEGMENT_64) continue;
        const struct segment_command_64 *seg = (const struct segment_command_64 *)lc;
        const struct section_64 *sect = (const struct section_64 *)(seg + 1);
        for (uint32_t s = 0; s < seg->nsects; s++, sect++) {
            uint32_t type = sect->flags & SECTION_TYPE;
            if (type != S_LAZY_SYMBOL_POINTERS && type != S_NON_LAZY_SYMBOL_POINTERS) continue;
            void **slots = (void **)((uintptr_t)slide + sect->addr);
            for (uint64_t k = 0; k < sect->size / sizeof(void *); k++) {
                if (sect->reserved1 + k >= dysymtab->nindirectsyms) break;
                uint32_t index = indirect[sect->reserved1 + k];
                if (index & (INDIRECT_SYMBOL_LOCAL | INDIRECT_SYMBOL_ABS) || index >= symtab->nsyms) continue;
                if (strcmp(strings + syms[index].n_un.n_strx, symbol)) continue;
                void *pageStart = (void *)((uintptr_t)&slots[k] & ~(uintptr_t)(page - 1));
                // __DATA_CONST is read-only once dyld is done with it; __DATA stays writable.
                int readOnly = !strncmp(seg->segname, "__DATA_CONST", 16);
                if (mprotect(pageStart, page, PROT_READ | PROT_WRITE)) continue;
                if (!changed && previous) *previous = slots[k];
                slots[k] = to;
                if (readOnly) mprotect(pageStart, page, PROT_READ);
                changed++;
            }
        }
    }
    return changed;
}
#endif

// The last part of the path `path`, for log lines.
static const char *lastPathPart(const char *path) {
    const char *slash = strrchr(path, USER_DIR_SEP[0]);
    return slash ? slash + 1 : path;
}

// Copies the system's error text `text` (dlopen's names the paths it tried) to out[size], with each
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
static void enableCoe5seOnce(const char *config, const char *layout, const char *source, const char *folderReason) {
    char done[MAX_PATH], notice[MAX_PATH], backup[MAX_PATH], oldDone[MAX_PATH];
    if (!coe5seUserFile(AUTO_ENABLE_DONE_FILE, done, sizeof done) || !coe5seUserFile(AUTO_ENABLE_NOTICE_FILE, notice, sizeof notice) ||
        !coe5seUserFile(AUTO_ENABLE_BACKUP_FILE, backup, sizeof backup)) {
        writeLog("coe5se_auto_enable_skipped", "reason=path_too_long");
        return;
    }
    // No log: CoE5SE was turned on, or found on, before; under a stub up to version 7 with COE5_SAVE
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
