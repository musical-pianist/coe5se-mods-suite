// The interface between the game-folder stub and CoE5SE's loader.

#pragma once
#include <stdint.h>
#include <string.h>

// The loader's file names.
#ifdef __APPLE__
#define LOADER_DLL_NAME    "coe5se_loader.dylib"
#else
#define LOADER_DLL_NAME    "coe5se_loader.dll"
#endif
#define LOADER_ENTRY_NAME  "coe5seLoaderStart"
#define LOADER_START_VERSION 1

// CoE5SE's mod name and its former Workshop items.
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

// The game's Steam app id and Workshop folder layout.
#define GAME_STEAM_APP_ID  "1606340"
#define WORKSHOP_UNDER_STEAMAPPS USER_DIR_SEP "workshop" USER_DIR_SEP "content" USER_DIR_SEP GAME_STEAM_APP_ID
#define WORKSHOP_ROOT_FMT  "%s" USER_DIR_SEP ".." USER_DIR_SEP ".." WORKSHOP_UNDER_STEAMAPPS

// Writes the game's Workshop content folder to `out`; 0 when it doesn't fit.
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

// The loader's list of the plugins it found.
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
