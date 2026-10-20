// The interface between glu32.dll (glu32_stub.c, the file players copy into the game folder) and
// CoE5SE's loader DLL (coe5se_loader.c, which ships in CoE5SE's mod folder and is replaced with each
// CoE5SE the player installs). glu32.dll finds and loads the loader DLL, then calls its one export,
// LOADER_ENTRY_NAME, once, with a LoaderStart it filled in.
//
// glu32.dll never changes once players have it, so every loader DLL must accept every LoaderStart a
// released glu32.dll sends. The rules for changing it:
//   - A new field goes at the end. `size` tells the loader which fields the caller filled: a field
//     whose end is past `size` is missing, and the loader uses a default for it.
//   - `version` rises only when a field's meaning changes. The loader reads every version it knows,
//     and refuses a newer one (the game then runs vanilla, with a log line).
//   - The entry's signature never changes. A different one would get a new export name.

#pragma once
#include <stdint.h>
#include <string.h>

// On Mac the stub is coe5se.dylib in the game folder (mac_stub.c), and the loader is
// coe5se_loader.dylib in CoE5SE's mod folder. The interface is the same.
#ifdef __APPLE__
#define LOADER_DLL_NAME    "coe5se_loader.dylib"
#else
#define LOADER_DLL_NAME    "coe5se_loader.dll"
#endif
#define LOADER_ENTRY_NAME  "coe5seLoaderStart"
#define LOADER_START_VERSION 1

// CoE5SE's mod name and its former Workshop items. The stub looks for CoE5SE's loader in the user
// folder's mods first and, with none there, in these two items only (stubs keep this rule for good).
// The loader loads CoE5SE's plugin DLL from neither: no Workshop copy of CoE5SE loads
// (coe5se_loader.c, isCoe5seFolder). Any other Workshop item with a CoE5SE.c5m (a fork or a
// re-upload) is ignored. The ids are built in rather than a setting, because a setting could be
// changed by the files it guards against.
#define COE5SE_MOD                "CoE5SE"
#define COE5SE_WORKSHOP_ID        "3809951834"
#define COE5SE_BACKUP_WORKSHOP_ID "3812078572"
static const char *const COE5SE_WORKSHOP_IDS[] __attribute__((unused)) = {COE5SE_WORKSHOP_ID, COE5SE_BACKUP_WORKSHOP_ID};
#define COE5SE_WORKSHOP_ID_COUNT 2

// 1 if `item` (a Workshop folder's name) is one of CoE5SE's items.
static __attribute__((unused)) int isCoe5seWorkshopItem(const char *item) {
    for (int i = 0; i < COE5SE_WORKSHOP_ID_COUNT; i++)
        if (!strcmp(item, COE5SE_WORKSHOP_IDS[i])) return 1;
    return 0;
}

// Workshop items install into the game's Steam library: the game is in
// <lib>/steamapps/common/<game>/ and the items in <lib>/steamapps/workshop/content/<app id>/<item>/.
// USER_DIR_SEP is approvals.h's.
#define GAME_STEAM_APP_ID  "1606340"
#define WORKSHOP_UNDER_STEAMAPPS USER_DIR_SEP "workshop" USER_DIR_SEP "content" USER_DIR_SEP GAME_STEAM_APP_ID
#define WORKSHOP_ROOT_FMT  "%s" USER_DIR_SEP ".." USER_DIR_SEP ".." WORKSHOP_UNDER_STEAMAPPS

// Writes the Workshop folder of the game's Steam library to `out`: the nearest folder above
// `gameDir` named steamapps (in any case), plus workshop/content/<app id>, so a game moved deeper
// (a Mac .app bundle) still finds it. With no steamapps above it (the test instances), it is
// <gameDir>/../../workshop/content/<app id>, as stubs up to version 7 had it. Returns 0 when the
// path doesn't fit (buildPath logs it).
static __attribute__((unused)) int findWorkshopRoot(const char *gameDir, char *out, size_t size) {
    size_t end = strlen(gameDir);
    while (end > 0) {
        size_t start = end;
        while (start > 0 && gameDir[start - 1] != USER_DIR_SEP[0]) start--;
        if (end - start == 9 && !_strnicmp(gameDir + start, "steamapps", 9))
            return buildPath(out, size, __func__, "%.*s" WORKSHOP_UNDER_STEAMAPPS, (int)end, gameDir);
        end = start ? start - 1 : 0;
    }
    return buildPath(out, size, __func__, WORKSHOP_ROOT_FMT, gameDir);
}

// plugins_found.txt in CoE5SE's user folder: the loader writes one name|source|state|path|sha256
// line per plugin it found at this launch, in load order, and the CoE5SE page reads it. Each state
// is written as its PLUGIN_STATE_NAMES word.
#define PLUGINS_FOUND_FILE "plugins_found.txt"
typedef enum {
    PLUGIN_FOUND,        // found in an enabled mod's folder, not tried yet
    PLUGIN_CONFLICT,     // another folder claims its mod name or its file name
    PLUGIN_INACTIVE,     // not tried, because CoE5SE's own DLL didn't load
    PLUGIN_REFUSED,      // CoE5SE refused it before loading, such as built for another game version
    PLUGIN_FAILED,       // unreadable, or the load failed
    PLUGIN_UNAPPROVED,   // its hash isn't approved and the player said No
    PLUGIN_LOADED,
    PLUGIN_STATE_COUNT
} PluginState;
static const char *const PLUGIN_STATE_NAMES[PLUGIN_STATE_COUNT] __attribute__((unused)) = {
    "found", "conflict", "inactive", "refused", "failed", "unapproved", "loaded"};

// The state a PLUGIN_STATE_NAMES word names, or PLUGIN_STATE_COUNT for a word this build doesn't know.
static __attribute__((unused)) PluginState pluginStateNamed(const char *word) {
    for (int s = 0; s < PLUGIN_STATE_COUNT; s++)
        if (!strcmp(PLUGIN_STATE_NAMES[s], word)) return (PluginState)s;
    return PLUGIN_STATE_COUNT;
}

// Where glu32.dll called the loader from (LoaderStart.startedFrom).
#define LOADER_STARTED_AT_MAIN_READY 0   // CoE5.exe's SDL_SetMainReady call: outside the loader lock
#define LOADER_STARTED_IN_DLLMAIN    1   // glu32.dll's DllMain, when the exe doesn't import SDL_SetMainReady
#define LOADER_STARTED_IN_CONSTRUCTOR 2  // the Mac stub's constructor, which dyld runs before the game's main

typedef struct LoaderStart {
    uint32_t size;           // sizeof(LoaderStart) as the caller's build has it
    uint32_t version;        // LOADER_START_VERSION of the caller's build
    uint32_t glu32Version;   // the calling stub's GLU32_VERSION or STUB_VERSION
    uint32_t startedFrom;    // one of the LOADER_STARTED_* values above
    const char *modDir;      // CoE5SE's mod folder, which glu32.dll picked and loaded the loader DLL from
    const char *source;      // "local" or "workshop": where modDir is
    const char *loaderSha256;   // the loader DLL's SHA-256 as lowercase hex, as stubs up to version 7
                                // approved it; "" from version 8, which no longer hashes the loader
    const char *userDir;     // from stub version 8: the game's user folder, found by the game's rule
                             // (approvals.h, findGameUserDir), which the stub read coe5config in
} LoaderStart;

// The loader DLL's entry. It returns 1 when CoE5SE's own DLL loaded, and 0 when nothing did.
typedef int (*LoaderStartFn)(const LoaderStart *start);
