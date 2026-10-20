// The CoE5SE loader, coe5se_loader.dll (coe5se_loader.dylib on Mac).
#ifdef _WIN32
#include <windows.h>
#else
#define PLATFORM_NO_DLLMAIN
#include "platform_mac.h"
#endif
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define LOG_TAG "loader"
#define STARTUP_LOG_ESCAPE
#include "startup_log.h"
#include "approve_dll.h"
#include "settings_files.h"
#include "coe5config.h"
#include "loader_start.h"
#include "mod_order.h"
#include "log_files.h"
#ifdef __APPLE__
#include "workshop_cache.h"
#endif

static char gameDir[MAX_PATH];

#ifdef _WIN32
#define PLUGIN_SUFFIX ".c5plugin.dll"
#else
#define PLUGIN_SUFFIX ".c5plugin.dylib"
#endif

// The CoE5SE mod's own plugin: without it, nothing loads.
#define COE5SE_FILE "coe5se" PLUGIN_SUFFIX

// Every plugin found at this launch in an enabled mod's folder.
typedef struct {
    char name[MAX_PATH], source[16], mod[MAX_PATH], dir[MAX_PATH], path[MAX_PATH], hash[SHA256_HEX_LEN + 1];
    PluginState state;
    int drop;
    int rank;
    char version[MOD_VERSION_MAX];
} FoundPlugin;
static FoundPlugin *found;
static int foundCount, foundCapacity, loadedCount, foundOverflow;

// Why no plugin past CoE5SE's own loads, or NULL while CoE5SE's DLL is found and loading.
static const char *inactiveReason;

// The mods enabled on the game's own Mods screen, by name (the .c5m file name without ".c5m").
#define MAX_ENABLED 500
static char enabledMods[MAX_ENABLED][MOD_NAME_MAX];
static int enabledCount;

// coe5config's bytes when its version is newer than the layout file lists.
static unsigned char configBytes[CONFIG_MAX];
static size_t configByteCount;
static int searchConfigByName;

// Reads the enabled mods from coe5config the way the game reads it.
static void readEnabledMods(const char *userDir, const char *layoutPath) {
    char path[MAX_PATH], name[MOD_NAME_MAX];
    if (!buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP CONFIG_FILE, userDir)) {
        writeLog("loader_enabled_mods", "count=0 reason=path_too_long");
        return;
    }
    const char *reason = NULL;
    int version = 0;
    FILE *f = openConfigModList(path, &version, &reason);
    if (!f && isNewerConfigVersion(version)) {
        const char *why;
        int at = configLayoutListStart(layoutPath, version, &why);
        if (at >= 0 && (f = fopen(path, "rb")) && fseek(f, at, SEEK_SET)) {
            fclose(f);
            f = NULL;
        }
        if (f) {
            writeLog("loader_enabled_mods_at", "version=%d list_at=%d file=" CONFIG_LAYOUT_FILE, version, at);
        } else {
            configByteCount = readConfigBytes(path, configBytes, sizeof configBytes);
            searchConfigByName = 1;
            writeLog("loader_enabled_mods", "count=-1 reason=%s version=%d search=name bytes=%zu layout=%s", reason, version,
                     configByteCount, at >= 0 ? "unreadable" : why);
            return;
        }
    }
    if (!f) {
        writeLog("loader_enabled_mods", "count=0 reason=%s version=%d", reason, version);
        return;
    }
    while (readConfigModName(f, name, sizeof name)) {
        if (enabledCount == MAX_ENABLED) {
            writeLog("loader_enabled_mods_cut", "max=%d next=%s", MAX_ENABLED, name);
            break;
        }
        snprintf(enabledMods[enabledCount++], sizeof enabledMods[0], "%s", name);
        writeLog("loader_enabled_mod", "mod=%s", name);
    }
    fclose(f);
    writeLog("loader_enabled_mods", "count=%d", enabledCount);
}

// 1 if the game's enabled list has `mod`, matched exactly, as the game matches it.
static int isModEnabled(const char *mod) {
    if (searchConfigByName) return configDataHasModName(configBytes, configByteCount, mod);
    for (int i = 0; i < enabledCount; i++)
        if (!strcmp(enabledMods[i], mod)) return 1;
    return 0;
}

// Checks the folder's .c5m files against the enabled list.
static int folderModState(const char *dir, char *mod, size_t modSize) {
    char pattern[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*.c5m", dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int state = 0;
    do {
        size_t len = strlen(fd.cFileName);
        if (len <= 4 || _stricmp(fd.cFileName + len - 4, ".c5m")) continue;
        char stem[MAX_PATH];
        snprintf(stem, sizeof stem, "%.*s", (int)(len - 4), fd.cFileName);
        if (!state) snprintf(mod, modSize, "%s", stem);
        state = 1;
        if (isModEnabled(stem)) {
            snprintf(mod, modSize, "%s", stem);
            state = 2;
        }
    } while (state < 2 && FindNextFileA(h, &fd));
    FindClose(h);
    return state;
}

// 1 if COE5SE_FILE may load from this folder: the CoE5SE mod, not on the Workshop.
static int isCoe5seFolder(const char *dir, const char *source, const char *mod) {
    (void)dir;
    if (strcmp(mod, COE5SE_MOD)) return 0;
    if (strcmp(source, "workshop")) return 1;
    return 0;
}

// 1 if `dir` is a Workshop item that isn't CoE5SE's but has a CoE5SE.c5m.
static int isOtherCoe5seItem(const char *dir, const char *source) {
    char c5m[MAX_PATH];
    if (strcmp(source, "workshop")) return 0;
    if (!buildPath(c5m, sizeof c5m, __func__, "%s" USER_DIR_SEP COE5SE_MOD ".c5m", dir)) return 0;
    return GetFileAttributesA(c5m) != INVALID_FILE_ATTRIBUTES;
}

// Adds each *.c5plugin.dll in `dir` to the found list.
static void findPluginsIn(const char *dir, const char *source) {
    char pattern[MAX_PATH], mod[MAX_PATH] = "";
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*" PLUGIN_SUFFIX, dir)) return;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    int modState = folderModState(dir, mod, sizeof mod);
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (modState == 0) {
            writeLog("loader_plugin_skipped", "name=%s source=%s reason=no_c5m folder=%s", fd.cFileName, source, dir);
            continue;
        }
        if (modState == 1) {
            writeLog("loader_plugin_skipped", "name=%s source=%s reason=mod_not_enabled mod=%s", fd.cFileName, source, mod);
            continue;
        }
        if (!_stricmp(fd.cFileName, COE5SE_FILE) && !isCoe5seFolder(dir, source, mod)) {
            writeLog("loader_plugin_skipped", "name=%s source=%s reason=reserved_name mod=%s folder=%s", fd.cFileName, source, mod, dir);
            continue;
        }
        if (!growList((void **)&found, &foundCapacity, foundCount + 1, sizeof *found)) {
            foundOverflow = 1;
            break;
        }
        FoundPlugin *p = &found[foundCount];
        memset(p, 0, sizeof *p);
        if (!buildPath(p->path, sizeof p->path, __func__, "%s" USER_DIR_SEP "%s", dir, fd.cFileName)) {
            writeLog("loader_plugin_skipped", "name=%s source=%s reason=path_too_long mod=%s", fd.cFileName, source, mod);
            continue;
        }
        foundCount++;
        snprintf(p->name, sizeof p->name, "%s", fd.cFileName);
        snprintf(p->source, sizeof p->source, "%s", source);
        snprintf(p->mod, sizeof p->mod, "%s", mod);
        snprintf(p->dir, sizeof p->dir, "%s", dir);
        p->state = PLUGIN_FOUND;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

// 1 if the two folders have a .c5m with the same name, which the game enables mods by.
static int foldersShareModName(const char *a, const char *b) {
    char pattern[MAX_PATH], other[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*.c5m", a)) return 1;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int shared = 0;
    do {
        if (!buildPath(other, sizeof other, __func__, "%s" USER_DIR_SEP "%s", b, fd.cFileName)) shared = 1;
        else shared = GetFileAttributesA(other) != INVALID_FILE_ATTRIBUTES;
    } while (!shared && FindNextFileA(h, &fd));
    FindClose(h);
    return shared;
}

// The folders that hold mod folders: local mods, then Workshop items.
static char modRoots[2][MAX_PATH];
static const char *const MOD_ROOT_SOURCES[2] = {"local", "workshop"};

// Looks for another mod folder, under either root, that has p's enabled .c5m in it.
static const char *otherFolderClaimingMod(const FoundPlugin *p, char *other, size_t otherSize) {
    for (int r = 0; r < 2; r++) {
        if (!modRoots[r][0]) continue;
        char pattern[MAX_PATH], c5m[MAX_PATH];
        if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", modRoots[r])) continue;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') continue;
            int fits = buildPath(other, otherSize, __func__, "%s" USER_DIR_SEP "%s", modRoots[r], fd.cFileName);
            if (fits && (!_stricmp(other, p->dir) || isOtherCoe5seItem(other, MOD_ROOT_SOURCES[r]))) continue;
            if (fits) fits = buildPath(c5m, sizeof c5m, __func__, "%s" USER_DIR_SEP "%s.c5m", other, p->mod);
            if (!fits || GetFileAttributesA(c5m) != INVALID_FILE_ATTRIBUTES) {
                FindClose(h);
                return MOD_ROOT_SOURCES[r];
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return NULL;
}

// Drops `p` for a clash with the folder at `other`, unless p is the local copy and the other isn't.
static int dropUnlessLocalWins(FoundPlugin *p, const char *otherSource, const char *other) {
    int local = !strcmp(p->source, "local"), otherLocal = !strcmp(otherSource, "local");
    if (local && !otherLocal) return 0;
    p->drop = 1;
    writeLog("loader_plugin_skipped", "name=%s source=%s reason=%s other=%s", p->name, p->source,
             otherLocal && !local ? "shadowed_by_local" : "clash", other);
    return 1;
}

// Marks plugins PLUGIN_CONFLICT when two folders claim the same mod name or the same DLL file name.
static void dropClashingPlugins(void) {
    for (int i = 0; i < foundCount; i++)
        for (int j = 0; j < foundCount; j++) {
            if (i == j || !_stricmp(found[i].dir, found[j].dir)) continue;
            if (_stricmp(found[i].name, found[j].name) && !foldersShareModName(found[i].dir, found[j].dir)) continue;
            if (dropUnlessLocalWins(&found[i], found[j].source, found[j].path)) break;
        }
    for (int i = 0; i < foundCount; i++) {
        char other[MAX_PATH];
        const char *otherSource = found[i].drop ? NULL : otherFolderClaimingMod(&found[i], other, sizeof other);
        if (otherSource) dropUnlessLocalWins(&found[i], otherSource, other);
    }
    for (int i = 0; i < foundCount; i++)
        if (found[i].drop) found[i].state = PLUGIN_CONFLICT;
}

// Writes plugins_found.txt.
static void writeFoundList(void) {
    char path[MAX_PATH];
    if (!coe5seUserFile(PLUGINS_FOUND_FILE, path, sizeof path)) return;
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < foundCount; i++)
        fprintf(f, "%s|%s|%s|%s|%s\n", found[i].name, found[i].source, PLUGIN_STATE_NAMES[found[i].state], found[i].path,
                found[i].hash);
    fclose(f);
}

#ifdef __APPLE__
// The real paths of the local mods folder's subfolders.
static TextList localModFolders;
#endif

// Collects plugins from `dir` and from each of its subfolders.
static void findPluginsUnder(const char *dir, const char *source) {
    findPluginsIn(dir, source);
    char pattern[MAX_PATH], sub[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", dir)) return;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') continue;
        if (!buildPath(sub, sizeof sub, __func__, "%s" USER_DIR_SEP "%s", dir, fd.cFileName)) continue;
#ifdef __APPLE__
        if (!strcmp(source, "workshop") && findRealFolder(&localModFolders, sub) >= 0) continue;
#endif
        if (isOtherCoe5seItem(sub, source)) {
            writeLog("loader_folder_skipped", "source=%s folder=%s reason=not_coe5se_item", source, fd.cFileName);
            continue;
        }
        findPluginsIn(sub, source);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

#ifdef __APPLE__
// The player's Mac-only setting mac_workshop_list, read before the game starts.
#define MAC_WORKSHOP_LIST_NAME    "mac_workshop_list"
#define MAC_WORKSHOP_LIST_DEFAULT 1
static int readMacWorkshopList(void) {
    return readCoe5seIntSetting(MAC_WORKSHOP_LIST_NAME, MAC_WORKSHOP_LIST_DEFAULT) != 0;
}
#endif

// Sets both mod roots and collects the plugins under them.
static void findAllPlugins(const char *userDir) {
    if (buildPath(modRoots[0], sizeof modRoots[0], __func__, "%s" USER_DIR_SEP "mods", userDir))
        findPluginsUnder(modRoots[0], "local");
    int workshop = findWorkshopRoot(gameDir, modRoots[1], sizeof modRoots[1]);
#ifdef __APPLE__
    char cache[MAX_PATH];
    if (buildPath(cache, sizeof cache, __func__, "%s" USER_DIR_SEP WORKSHOP_CACHE_FILE, userDir))
        writeWorkshopCacheFor(cache, modRoots[0], modRoots[1], !readMacWorkshopList());
    if (!readLocalMods(modRoots[0], &localModFolders, NULL, NULL)) writeLog("loader_local_folders_cut", "reason=out_of_memory");
    if (workshop && sameRealFolder(modRoots[0], modRoots[1])) workshop = 0;
#endif
    if (workshop) findPluginsUnder(modRoots[1], "workshop");
}

// Adds each enabled mod under `root` to *mods, once per name; the first copy found counts.
static void findEnabledModsUnder(const char *root, ModInfo **mods, int *count, int *capacity) {
    char pattern[MAX_PATH], sub[MAX_PATH], c5m[MAX_PATH];
    if (!root[0] || !buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", root)) return;
    WIN32_FIND_DATAA fd, cd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') continue;
        if (!buildPath(sub, sizeof sub, __func__, "%s" USER_DIR_SEP "%s", root, fd.cFileName) ||
            !buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*.c5m", sub)) continue;
        if (root == modRoots[1] && isOtherCoe5seItem(sub, "workshop")) continue;
        HANDLE c = FindFirstFileA(pattern, &cd);
        if (c == INVALID_HANDLE_VALUE) continue;
        do {
            size_t len = strlen(cd.cFileName);
            if (len <= 4 || _stricmp(cd.cFileName + len - 4, ".c5m")) continue;
            char stem[MOD_NAME_MAX];
            if (len - 4 >= sizeof stem) continue;
            snprintf(stem, sizeof stem, "%.*s", (int)(len - 4), cd.cFileName);
            int seen = 0;
            for (int i = 0; i < *count && !seen; i++) seen = !strcmp((*mods)[i].name, stem);
            if (!isModEnabled(stem) || seen) continue;
            if (!growList((void **)mods, capacity, *count + 1, sizeof **mods)) break;
            ModInfo *m = &(*mods)[(*count)++];
            initModInfo(m, stem, 0);
            if (searchConfigByName) writeLog("loader_enabled_mod", "mod=%s search=name", stem);
            if (buildPath(c5m, sizeof c5m, __func__, "%s" USER_DIR_SEP "%s", sub, cd.cFileName)) readModOrderFile(m, c5m);
        } while (FindNextFileA(c, &cd));
        FindClose(c);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

// Puts the plugins in their mods' load order, and a mod's DLLs in name order.
static void orderPluginsByMod(void) {
    ModInfo *mods = NULL;
    int count = 0, capacity = 0;
    for (int r = 0; r < 2; r++) findEnabledModsUnder(modRoots[r], &mods, &count, &capacity);
    int *order = count ? malloc((size_t)count * sizeof(int)) : NULL;
    ModIssue *issues = NULL;
    int issueCount = 0;
    if (order) orderMods(mods, count, order, &issues, &issueCount);
    for (int i = 0; i < foundCount; i++) {
        found[i].rank = count;
        for (int k = 0; order && k < count; k++)
            if (!strcmp(mods[order[k]].name, found[i].mod)) {
                found[i].rank = k;
                snprintf(found[i].version, sizeof found[i].version, "%s", mods[order[k]].version);
                break;
            }
    }
    for (int i = 1; i < foundCount; i++)
        for (int k = i; k > 0; k--) {
            FoundPlugin *a = &found[k - 1], *b = &found[k];
            if (a->rank < b->rank || (a->rank == b->rank && compareModNames(a->name, b->name) <= 0)) break;
            FoundPlugin t = *a;
            *a = *b;
            *b = t;
        }
    writeLog("loader_mod_order", "enabled_mods=%d ordered=%d", count, !count || order != NULL);
    free(issues);
    free(order);
    for (int i = 0; i < count; i++) freeModInfo(&mods[i]);
    free(mods);
}

// Moves CoE5SE's own DLL to the front of the list, so it loads first.
static int moveCoe5seFirst(void) {
    for (int i = 0; i < foundCount; i++)
        if (!_stricmp(found[i].name, COE5SE_FILE) && found[i].state == PLUGIN_FOUND) {
            FoundPlugin coe5se = found[i];
            memmove(&found[1], &found[0], (size_t)i * sizeof found[0]);
            found[0] = coe5se;
            return 1;
        }
    return 0;
}

// Plugin DLLs from mods are set aside: only CoE5SE's own loads.
#ifdef LOADER_EXTRA_HEADER
#include LOADER_EXTRA_HEADER
#else
static void findExtraPlugins(void) {}
static int isExtraPlugin(const FoundPlugin *p) {
    (void)p;
    return 0;
}
#endif

// Refuses every found plugin but CoE5SE's own and isExtraPlugin's.
static int setAsideModPlugins(void) {
    int count = 0;
    for (int i = 0; i < foundCount; i++) {
        FoundPlugin *p = &found[i];
        if (p->state != PLUGIN_FOUND || !_stricmp(p->name, COE5SE_FILE) || isExtraPlugin(p)) continue;
        p->state = PLUGIN_REFUSED;
        count++;
        writeLog("loader_plugin_skipped", "name=%s source=%s reason=plugins_set_aside mod=%s", p->name, p->source, p->mod);
    }
    return count;
}

// CoE5SE's three exports. coe5seLoadsPlugins: called right after CoE5SE loads; 0 means load no other plugin.
static int (*coe5seLoadsPlugins)(void);
// coe5sePluginLoading: called with each plugin's path before its DllMain runs, and with NULL after.
static void (*coe5sePluginLoading)(const char *path);
// coe5seCheckPlugin, called with each plugin's path and mod before hashing or loading it.
static const char *(*coe5seCheckPlugin)(const char *path, const char *mod);

static void *coe5seExport(void *module, const char *name) {
#ifdef _WIN32
    return (void *)GetProcAddress((HMODULE)module, name);
#else
    return dlsym(module, name);
#endif
}

// Reads CoE5SE's exports once its DLL has loaded.
static void connectCoe5se(void *module) {
    coe5seLoadsPlugins = (int (*)(void))coe5seExport(module, "coe5seLoadsPlugins");
    coe5sePluginLoading = (void (*)(const char *))coe5seExport(module, "coe5sePluginLoading");
    coe5seCheckPlugin = (const char *(*)(const char *, const char *))coe5seExport(module, "coe5seCheckPlugin");
    if (!coe5seLoadsPlugins || !coe5sePluginLoading || !coe5seCheckPlugin) inactiveReason = "coe5se_exports_missing";
    else if (!coe5seLoadsPlugins()) inactiveReason = "declined_on_unknown_game";
}

// Checks, approves, and loads one plugin.
static void loadPlugin(FoundPlugin *p, int isCoe5se, int autoApprove) {
    const char *refusal = isCoe5se ? NULL : coe5seCheckPlugin(p->path, p->mod);
    if (refusal) {
        p->state = PLUGIN_REFUSED;
        writeLog("loader_plugin_skipped", "name=%s source=%s reason=%s", p->name, p->source, refusal);
        return;
    }
    HANDLE file = lockDllFile(p->path);
    if (!hashFileSha256(file, p->hash)) {
        unlockDllFile(file);
        p->state = PLUGIN_FAILED;
        writeLog("loader_plugin_failed", "name=%s source=%s reason=unreadable", p->name, p->source);
        if (isCoe5se) inactiveReason = "coe5se_unreadable";
        return;
    }
    if (!approveDll(p->name, p->mod, p->source, p->path, p->hash, autoApprove, isCoe5se)) {
        unlockDllFile(file);
        p->state = PLUGIN_UNAPPROVED;
        if (isCoe5se) inactiveReason = "coe5se_not_approved";
        return;
    }
    writeLog("loader_plugin_loading", "name=%s source=%s", p->name, p->source);
    char error[256];
    if (!isCoe5se) coe5sePluginLoading(p->path);
    void *module = loadDllFile(p->path, error, sizeof error);
    if (!isCoe5se) coe5sePluginLoading(NULL);
    unlockDllFile(file);
    if (!module) {
        p->state = PLUGIN_FAILED;
        writeLog("loader_plugin_failed", "name=%s source=%s error=%s", p->name, p->source, error);
        if (isCoe5se) inactiveReason = "coe5se_load_failed";
        return;
    }
    loadedCount++;
    p->state = PLUGIN_LOADED;
    writeLog("loader_plugin_loaded", "name=%s source=%s mod=%s version=%s path=%s", p->name, p->source, p->mod,
             p->version[0] ? p->version : "-", p->path);
    if (isCoe5se) connectCoe5se(module);
}

// Commented out: no Workshop copy of CoE5SE loads, so none can be shadowed.

// Finds, checks, approves, and loads every plugin, CoE5SE's first, then writes plugins_found.txt.
static void loadAllPlugins(const char *modDir, const char *source, const char *layoutPath) {
    findExtraPlugins();
    readEnabledMods(userDir(), layoutPath);
    findAllPlugins(userDir());
    (void)modDir;
    (void)source;
    dropClashingPlugins();
    orderPluginsByMod();
    int setAside = setAsideModPlugins();
    if (foundOverflow) inactiveReason = "out_of_memory";
    else if (!moveCoe5seFirst()) inactiveReason = "no_coe5se_mod";
    int autoApprove = readAutoApprove();
    if (!inactiveReason) writeLog("loader_approvals", "auto_approve=%d", autoApprove);
    for (int i = 0; i < foundCount; i++) {
        if (found[i].state == PLUGIN_CONFLICT || found[i].state == PLUGIN_REFUSED) continue;
        if (inactiveReason) found[i].state = PLUGIN_INACTIVE;
        else loadPlugin(&found[i], i == 0, autoApprove);
    }
    if (inactiveReason) writeLog("loader_inactive", "reason=%s found=%d", inactiveReason, foundCount);
    writeFoundList();
    writeLog("loader_done", "found=%d loaded=%d set_aside=%d", foundCount, loadedCount, setAside);
}

// Sets gameDir to the game's folder and the log's path to coe5mod.log in it.
static void setLogPath(void) {
    GetModuleFileNameA(NULL, gameDir, sizeof gameDir);
    char *slash = strrchr(gameDir, USER_DIR_SEP[0]);
    if (slash) *slash = '\0';
    char path[MAX_PATH];
    if (buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "coe5mod.log", gameDir))
        memcpy(logPath, path, sizeof logPath);
}

// 1 if the caller's LoaderStart has `field`: its `size` reaches past the field's end.
#define START_HAS(start, field) ((start)->size >= offsetof(LoaderStart, field) + sizeof (start)->field)

// The calling stub's version, or 0 when its LoaderStart has none.
static uint32_t stubVersion;

// Returns the version of the stub that started this launch.
__declspec(dllexport) uint32_t coe5seStubVersion(void) {
    return stubVersion;
}

// Returns the user folder this launch uses.
__declspec(dllexport) const char *coe5seUserDir(void) {
    return userDirPath;
}

// 1 if the stub gave the user folder (stub version 8 and up).
static int userDirFromStub;
__declspec(dllexport) int coe5seUserDirFollowsGame(void) {
    return userDirFromStub;
}

// The entry the stub calls, once, after it loaded this DLL.
__declspec(dllexport) int coe5seLoaderStart(const LoaderStart *start) {
    setLogPath();
    if (logPath[0]) {
        LogRotation r = rotateLogAtLaunch(logPath, (unsigned long)GetCurrentProcessId());
        writeLog("log_rotated", "found_launch=%d earlier_bytes=%ld from_stub_old=%d move_error=%d files=%d max_bytes=%ld", r.found,
                 r.earlier, r.fromOld, r.error, LOG_FILES, (long)LOG_FILE_MAX);
    }
    uint32_t version = start && START_HAS(start, version) ? start->version : 0;
    uint32_t glu32Version = version && START_HAS(start, glu32Version) ? start->glu32Version : 0;
    uint32_t startedFrom = version && START_HAS(start, startedFrom) ? start->startedFrom : LOADER_STARTED_AT_MAIN_READY;
    const char *modDir = version && START_HAS(start, modDir) ? start->modDir : NULL;
    const char *source = version && START_HAS(start, source) ? start->source : NULL;
    const char *stubUserDir = version && START_HAS(start, userDir) ? start->userDir : NULL;
    stubVersion = glu32Version;
    writeLog("loader_started", "glu32_version=%u interface=%u started_from=%s", glu32Version, version,
#ifdef __APPLE__
             startedFrom == LOADER_STARTED_IN_CONSTRUCTOR ? "constructor" :
#endif
             startedFrom == LOADER_STARTED_IN_DLLMAIN ? "dllmain" : "sdl_set_main_ready");
    if (!version || version > LOADER_START_VERSION) {
        writeLog("loader_inactive", "reason=unknown_interface interface=%u known=%d", version, LOADER_START_VERSION);
        return 0;
    }
    userDirFromStub = stubUserDir && stubUserDir[0];
    if (userDirFromStub) snprintf(userDirPath, sizeof userDirPath, "%s", stubUserDir);
    else if (!legacyUserDir(userDirPath, sizeof userDirPath)) snprintf(userDirPath, sizeof userDirPath, "%s", USER_DIR_FALLBACK);
    writeLog("loader_user_dir", "from=%s", stubUserDir && stubUserDir[0] ? "stub" : "legacy");
    char layout[MAX_PATH] = "";
    if (modDir) buildPath(layout, sizeof layout, __func__, "%s" USER_DIR_SEP CONFIG_LAYOUT_FILE, modDir);
    loadAllPlugins(modDir, source, layout[0] ? layout : NULL);
    return foundCount > 0 && found[0].state == PLUGIN_LOADED && !_stricmp(found[0].name, COE5SE_FILE);
}
