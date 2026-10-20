// The warning for an enabled mod whose DLL was built for the other platform only.
#include "platform_plugins.h"

#ifdef __APPLE__
#define HERE_PLATFORM      "Mac"
#define OTHER_PLATFORM     "Windows"
#define HERE_PLATFORM_LOG  "mac"
#define HERE_PLUGIN_SUFFIX  MAC_PLUGIN_SUFFIX
#define OTHER_PLUGIN_SUFFIX WINDOWS_PLUGIN_SUFFIX
#else
#define HERE_PLATFORM      "Windows"
#define OTHER_PLATFORM     "Mac"
#define HERE_PLATFORM_LOG  "windows"
#define HERE_PLUGIN_SUFFIX  WINDOWS_PLUGIN_SUFFIX
#define OTHER_PLUGIN_SUFFIX MAC_PLUGIN_SUFFIX
#endif

#define MISSING_POPUP_MODS 12

// One enabled mod with a plugin for the other platform only.
typedef struct {
    char mod[MOD_NAME_MAX], folder[MAX_PATH], files[512], missing[512];
} ModMissingPlugin;
static ModMissingPlugin *missingPluginMods;
static int missingPluginModCount, missingPluginModCapacity;

// Appends `name` to a comma-separated list, if it fits.
static void appendFileName(char *list, size_t size, const char *name) {
    size_t n = strlen(list);
    if (n + strlen(name) + 3 > size) return;
    snprintf(list + n, size - n, "%s%s", n ? ", " : "", name);
}

// findPluginsMissingHere's callback: records one file in the mod being checked, and logs it.
static int missingPluginLogIt;
static void noteMissingPlugin(void *context, const char *otherFile, const char *missingFile) {
    ModMissingPlugin *m = context;
    appendFileName(m->files, sizeof m->files, otherFile);
    appendFileName(m->missing, sizeof m->missing, missingFile);
    if (missingPluginLogIt)
        writeLog("plugin_missing_for_platform", "mod=%s file=%s missing=%s platform=%s folder=%s", m->mod, otherFile,
                 missingFile, HERE_PLATFORM_LOG, m->folder);
}

// Rebuilds the list from the game's mod table.
static int listModsMissingPlugins(int logIt) {
    missingPluginModCount = 0;
    missingPluginLogIt = logIt;
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modEnabled(m)) continue;
        int seen = 0;
        for (int k = 0; k < missingPluginModCount && !seen; k++) seen = !strcmp(missingPluginMods[k].mod, modName(m));
        if (seen) continue;
        if (!growList((void **)&missingPluginMods, &missingPluginModCapacity, missingPluginModCount + 1, sizeof missingPluginMods[0])) {
            writeLog("plugin_missing_list_cut", "reason=out_of_memory listed=%d", missingPluginModCount);
            break;
        }
        ModMissingPlugin *entry = &missingPluginMods[missingPluginModCount];
        memset(entry, 0, sizeof *entry);
        snprintf(entry->mod, sizeof entry->mod, "%s", modName(m));
        snprintf(entry->folder, sizeof entry->folder, "%s", modFolder(m));
        if (findPluginsMissingHere(entry->folder, HERE_PLUGIN_SUFFIX, OTHER_PLUGIN_SUFFIX, noteMissingPlugin, entry))
            missingPluginModCount++;
    }
    return missingPluginModCount;
}

// The pop-up, shown once per launch, between main menu frames.
static void warnModsMissingPlugins(void) {
    int count = listModsMissingPlugins(1);
    if (!count) return;
    static char text[POPUP_TEXT_SIZE];
    int one = count == 1;
    size_t n = 0;
    for (int i = 0; i < count && i < MISSING_POPUP_MODS; i++) {
        char label[80];
        modDisplayName(missingPluginMods[i].mod, label, sizeof label);
        appendText(text, sizeof text, &n, "%s\n", label);
    }
    if (count > MISSING_POPUP_MODS) appendText(text, sizeof text, &n, "...and %d more.\n", count - MISSING_POPUP_MODS);
    appendText(text, sizeof text, &n, "%s",
               one ? "\nNo " HERE_PLATFORM " DLL. Mod incomplete without it.\n\nDisable mod?"
                   : "\nNo " HERE_PLATFORM " DLLs. Mods incomplete without them.\n\nDisable mods?");
    writeLog("plugin_missing_popup", "mods=%d platform=%s", count, HERE_PLATFORM_LOG);
    int choice = messageBox("CoE5SE: Missing " HERE_PLATFORM " DLL", text, "Yes", "No", 0, 0, 0);
    if (choice != 1) {
        writeLog("plugin_missing_choice", "choice=keep mods=%d", count);
        return;
    }
    writeLog("plugin_missing_choice", "choice=disable mods=%d", count);
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        for (int k = 0; k < count; k++)
            if (modEnabled(m) && !strcmp(modName(m), missingPluginMods[k].mod)) {
                *(short *)(gameBase + RVA_MOD_NAMES + (size_t)m * MOD_RECORD_SIZE + MOD_RECORD_ENABLED) = 0;
                writeLog("mod_disabled", "mod=%s reason=plugin_missing_for_platform", missingPluginMods[k].mod);
            }
    GAME_FN(void (*)(void), RVA_WRITE_CONFIG)();
    relaunchGame("plugin_missing_for_platform");
}

// The CoE5SE page's lines under "Mods with DLLs".
static void drawModsMissingPlugins(int count) {
    char row[224], tip[1600], label[80];
    for (int i = 0; i < count; i++) {
        const ModMissingPlugin *m = &missingPluginMods[i];
        modDisplayName(m->mod, label, sizeof label);
        snprintf(row, sizeof row, "%s (no " HERE_PLATFORM " version of its DLL)", label);
        textButton(0, row);
        snprintf(tip, sizeof tip,
                 "This mod has no " HERE_PLATFORM " version of its DLL. Its folder has only the " OTHER_PLATFORM
                 " version (%.300s; " HERE_PLATFORM " needs %.300s). Without its DLL, the mod won't work as its author "
                 "meant. Don't play it this way: disable it on the Mods screen, or ask its author for a " HERE_PLATFORM
                 " version.\nFolder: %.200s",
                 m->files, m->missing, m->folder);
        tooltip(tip);
    }
}
