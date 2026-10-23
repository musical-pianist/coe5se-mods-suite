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

// CoE5SE's mod name and Workshop items.
#define COE5SE_MOD                "CoE5SE"
#define COE5SE_WORKSHOP_ID        "3809951834"
#define COE5SE_BACKUP_WORKSHOP_ID "3812078572"
static const char *const COE5SE_WORKSHOP_IDS[] __attribute__((unused)) = {COE5SE_WORKSHOP_ID, COE5SE_BACKUP_WORKSHOP_ID};
#define COE5SE_WORKSHOP_ID_COUNT 2

// 1 if `item` is one of CoE5SE's Workshop items.
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
    PLUGIN_FOUND,
    PLUGIN_CONFLICT,
    PLUGIN_INACTIVE,
    PLUGIN_REFUSED,
    PLUGIN_FAILED,
    PLUGIN_UNAPPROVED,
    PLUGIN_LOADED,
    PLUGIN_STATE_COUNT
} PluginState;
static const char *const PLUGIN_STATE_NAMES[PLUGIN_STATE_COUNT] __attribute__((unused)) = {
    "found", "conflict", "inactive", "refused", "failed", "unapproved", "loaded"};

// The state a word names; PLUGIN_STATE_COUNT for an unknown one.
static __attribute__((unused)) PluginState pluginStateNamed(const char *word) {
    for (int s = 0; s < PLUGIN_STATE_COUNT; s++)
        if (!strcmp(PLUGIN_STATE_NAMES[s], word)) return (PluginState)s;
    return PLUGIN_STATE_COUNT;
}

// Where the stub called the loader from.
#define LOADER_STARTED_AT_MAIN_READY 0
#define LOADER_STARTED_IN_DLLMAIN    1
#define LOADER_STARTED_IN_CONSTRUCTOR 2
#define LOADER_STARTED_AT_SRAND      3

typedef struct LoaderStart {
    uint32_t size;
    uint32_t version;
    uint32_t glu32Version;
    uint32_t startedFrom;
    const char *modDir;
    const char *source;
    const char *loaderSha256;
    const char *userDir;
} LoaderStart;

// The loader's entry: 1 when CoE5SE's own DLL loaded, 0 when nothing did.
typedef int (*LoaderStartFn)(const LoaderStart *start);
