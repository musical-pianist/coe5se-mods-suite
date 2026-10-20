// Finds a mod's plugins that were built for the other platform only.
#pragma once
#include <string.h>

#define WINDOWS_PLUGIN_SUFFIX ".c5plugin.dll"
#define MAC_PLUGIN_SUFFIX     ".c5plugin.dylib"

// Called with each plugin of the other platform and the file this platform needed.
typedef void (*MissingPluginFn)(void *context, const char *otherFile, const char *missingFile);

// 1 if `name` ends in `suffix`, ignoring case, with a base name before it.
static int pluginHasSuffix(const char *name, const char *suffix) {
    size_t len = strlen(name), suffixLen = strlen(suffix);
    return len > suffixLen && !_stricmp(name + len - suffixLen, suffix);
}

// 1 if `dir` has a file named `base` followed by `suffix`, ignoring case.
static int folderHasPluginNamed(const char *dir, const char *base, size_t baseLen, const char *suffix) {
    char pattern[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*%s", dir, suffix)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int found = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || !pluginHasSuffix(fd.cFileName, suffix)) continue;
        found = strlen(fd.cFileName) == baseLen + strlen(suffix) && !_strnicmp(fd.cFileName, base, baseLen);
    } while (!found && FindNextFileA(h, &fd));
    FindClose(h);
    return found;
}

// Calls onMissing for each plugin in `dir` built for the other platform only.
static int findPluginsMissingHere(const char *dir, const char *hereSuffix, const char *otherSuffix,
                                  MissingPluginFn onMissing, void *context) {
    char pattern[MAX_PATH], missing[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" PATH_SEP "*%s", dir, otherSuffix)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int count = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || !pluginHasSuffix(fd.cFileName, otherSuffix)) continue;
        size_t baseLen = strlen(fd.cFileName) - strlen(otherSuffix);
        if (folderHasPluginNamed(dir, fd.cFileName, baseLen, hereSuffix)) continue;
        if (!buildPath(missing, sizeof missing, __func__, "%.*s%s", (int)baseLen, fd.cFileName, hereSuffix)) continue;
        count++;
        if (onMissing) onMissing(context, fd.cFileName, missing);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}
