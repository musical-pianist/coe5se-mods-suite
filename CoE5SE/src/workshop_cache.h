// The Mac game's Workshop items.
#pragma once
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define WORKSHOP_CACHE_FILE     "oldworkshop"
// The game's Workshop item table holds 500 items.
#define WORKSHOP_CACHE_MAX      500
#define WORKSHOP_ITEM_INSTALLED 2
// The game keeps 1499 bytes of an item's path, so any path the system can resolve fits.
_Static_assert(PATH_MAX < 1500, "a real path fits the game's Workshop path field");

// A growing list of paths or mod names.
typedef struct {
    char (*items)[PATH_MAX];
    int count, capacity;
} TextList;

// Adds `text` to `list`.
static int addText(TextList *list, const char *text) {
    if (!growList((void **)&list->items, &list->capacity, list->count + 1, sizeof list->items[0])) return 0;
    snprintf(list->items[list->count++], sizeof list->items[0], "%s", text);
    return 1;
}

// The index of `text` in `list`, matched exactly and with case, or -1.
static int findText(const TextList *list, const char *text) {
    for (int i = 0; i < list->count; i++)
        if (!strcmp(list->items[i], text)) return i;
    return -1;
}

static void freeTextList(TextList *list) {
    free(list->items);
    memset(list, 0, sizeof *list);
}

// The index in `folders` of the folder `dir` resolves to, or -1.
static int findRealFolder(const TextList *folders, const char *dir) {
    char real[PATH_MAX];
    return realpath(dir, real) ? findText(folders, real) : -1;
}

// 1 if the folders `a` and `b` both exist and are one folder.
static int sameRealFolder(const char *a, const char *b) {
    char realA[PATH_MAX], realB[PATH_MAX];
    return realpath(a, realA) && realpath(b, realB) && !strcmp(realA, realB);
}

// Adds the names of the mods in `dir` to `names`.
static int readFolderModNames(const char *dir, TextList *names) {
    DIR *d = opendir(dir);
    if (!d) return 0;
    int added = 0;
    for (struct dirent *e; (e = readdir(d));) {
        const char *dot = strrchr(e->d_name, '.');
        size_t stem = dot ? (size_t)(dot - e->d_name) : 0;
        if (!dot || strcmp(dot, ".c5m") || !stem || stem >= MOD_NAME_MAX) continue;
        char name[MOD_NAME_MAX];
        snprintf(name, sizeof name, "%.*s", (int)stem, e->d_name);
        if (!addText(names, name)) {
            added = -1;
            break;
        }
        added++;
    }
    closedir(d);
    return added;
}

// What the mods folder at `root` lists.
static int readLocalMods(const char *root, TextList *folders, TextList *paths, TextList *names) {
    DIR *d = opendir(root);
    if (!d) return 1;
    int ok = 1;
    for (struct dirent *e; ok && (e = readdir(d));) {
        char path[PATH_MAX], real[PATH_MAX];
        if (e->d_name[0] == '.' || !buildPath(path, sizeof path, __func__, "%s/%s", root, e->d_name)) continue;
        if (!realpath(path, real)) continue;
        ok = addText(folders, real) && (!paths || addText(paths, path)) && (!names || readFolderModNames(path, names) >= 0);
    }
    closedir(d);
    return ok;
}

// The Workshop folder's item ids in age order.
static int compareItemIds(const void *a, const void *b) {
    size_t la = strlen((const char *)a), lb = strlen((const char *)b);
    return la != lb ? (la < lb ? -1 : 1) : strcmp((const char *)a, (const char *)b);
}

// Writes the cache file for the item folders in `items`.
static int writeWorkshopCacheFile(const char *path, const TextList *items) {
    char temp[PATH_MAX];
    if (!buildPath(temp, sizeof temp, __func__, "%s.new", path)) return 0;
    FILE *f = fopen(temp, "wb");
    if (!f) {
        writeLog("workshop_cache_failed", "reason=open path=%s", temp);
        return 0;
    }
    int32_t last = items->count - 1;
    int ok = fwrite(&last, sizeof last, 1, f) == 1;
    for (int i = 0; ok && i < items->count; i++) {
        uint16_t state = WORKSHOP_ITEM_INSTALLED;
        ok = fwrite(&state, sizeof state, 1, f) == 1;
        for (const char *c = items->items[i]; ok; c++) {
            ok = fputc((unsigned char)*c ^ CONFIG_KEY, f) != EOF;
            if (!*c) break;
        }
    }
    if (fclose(f) || !ok) {
        writeLog("workshop_cache_failed", "reason=write path=%s", temp);
        remove(temp);
        return 0;
    }
    if (rename(temp, path)) {
        writeLog("workshop_cache_failed", "reason=rename path=%s", path);
        remove(temp);
        return 0;
    }
    return 1;
}

// A Workshop item holding CoE5SE.
#define WORKSHOP_CACHE_ALWAYS_MOD "CoE5SE"

// Writes the game's Workshop cache at `cachePath`.
static int writeWorkshopCacheFor(const char *cachePath, const char *localRoot, const char *workshopRoot, int coe5seOnly) {
    TextList localFolders = {0}, localPaths = {0}, names = {0}, ids = {0}, items = {0}, itemNames = {0};
    char workshopReal[PATH_MAX];
    const char *empty = NULL;
    int ok = 1;
    if (!workshopRoot[0] || !realpath(workshopRoot, workshopReal)) empty = "no_workshop_folder";
    else if (sameRealFolder(localRoot, workshopRoot)) empty = "mods_folder_is_workshop_folder";
    if (!empty) {
        ok = readLocalMods(localRoot, &localFolders, &localPaths, &names);
        DIR *d = ok ? opendir(workshopReal) : NULL;
        for (struct dirent *e; d && ok && (e = readdir(d));)
            if (e->d_name[0] != '.') ok = addText(&ids, e->d_name);
        if (d) closedir(d);
        if (ids.count) qsort(ids.items, (size_t)ids.count, sizeof ids.items[0], compareItemIds);
    }
    for (int i = 0; ok && i < ids.count; i++) {
        const char *id = ids.items[i];
        char path[PATH_MAX], real[PATH_MAX];
        struct stat st;
        if (!buildPath(path, sizeof path, __func__, "%s/%s", workshopReal, id)) continue;
        if (!realpath(path, real) || stat(real, &st) || !S_ISDIR(st.st_mode)) continue;
        int local = findText(&localFolders, real);
        if (local >= 0) {
            writeLog("workshop_item_skipped", "item=%s reason=same_folder local=%s", id, localPaths.items[local]);
            continue;
        }
        itemNames.count = 0;
        int found = readFolderModNames(path, &itemNames);
        if (found < 0) {
            ok = 0;
            break;
        }
        if (!found) {
            writeLog("workshop_item_skipped", "item=%s reason=no_c5m", id);
            continue;
        }
        if (coe5seOnly && findText(&itemNames, WORKSHOP_CACHE_ALWAYS_MOD) < 0) {
            writeLog("workshop_item_skipped", "item=%s reason=setting_off mod=%s", id, itemNames.items[0]);
            continue;
        }
        int listed = -1;
        for (int n = 0; n < itemNames.count && listed < 0; n++)
            if (findText(&names, itemNames.items[n]) >= 0) listed = n;
        if (listed >= 0) {
            writeLog("workshop_item_skipped", "item=%s reason=same_mod_name mod=%s", id, itemNames.items[listed]);
            continue;
        }
        if (items.count == WORKSHOP_CACHE_MAX) {
            writeLog("workshop_item_skipped", "item=%s reason=list_full max=%d", id, WORKSHOP_CACHE_MAX);
            continue;
        }
        for (int n = 0; ok && n < itemNames.count; n++) ok = addText(&names, itemNames.items[n]);
        ok = ok && addText(&items, path);
        if (ok) writeLog("workshop_item_listed", "item=%s mod=%s", id, itemNames.items[0]);
    }
    int written = -1;
    if (!ok) writeLog("workshop_cache_failed", "reason=out_of_memory");
    else if (writeWorkshopCacheFile(cachePath, &items)) {
        written = items.count;
        writeLog("workshop_cache_written", "items=%d reason=%s coe5se_only=%d path=%s", written, empty ? empty : "-", coe5seOnly, cachePath);
    }
    freeTextList(&localFolders);
    freeTextList(&localPaths);
    freeTextList(&names);
    freeTextList(&ids);
    freeTextList(&items);
    freeTextList(&itemNames);
    return written;
}

// Every Workshop item, as the Mac Workshop list is on by default.
static __attribute__((unused)) int writeWorkshopCache(const char *cachePath, const char *localRoot, const char *workshopRoot) {
    return writeWorkshopCacheFor(cachePath, localRoot, workshopRoot, 0);
}
