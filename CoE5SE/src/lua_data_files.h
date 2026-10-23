// A script's data files: coe5.dataFolder, readIni, writeIni, listIni, deleteIni.

// ---- Data files: coe5.dataFolder, readIni, writeIni, listIni, deleteIni ------------------------
// A script's INI-shaped data files.

static LuaScript *checkLuaScript(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    if (!s) luaL_error(L, "not called from a mod's script");
    return s;
}

// Writes the path of script s's area to `out`.
static int dataAreaPath(LuaScript *s, const char *call, char *out, size_t size) {
    const char *problem = storageAreaProblem(s->mod);
    if (problem) {
        refuseLuaCall(s->mod, call, problem, "area=%s", s->mod);
        return 0;
    }
    if (!buildPath(out, size, call, "%s" USER_DIR_SEP STORAGE_DATA_DIR USER_DIR_SEP "%s", userDir(), s->mod)) {
        refuseLuaCall(s->mod, call, "path_too_long", "area=%s", s->mod);
        return 0;
    }
    char real[MAX_PATH];
    if (storageStoredName(out, real, sizeof real) && strcmp(real, s->mod)) {
        refuseLuaCall(s->mod, call, "area_taken", "area=%s real=%s", s->mod, real);
        return 0;
    }
    return 1;
}

// Writes the path of the script's data folder, or of file `name`.ini in it.
static int dataPath(LuaScript *s, const char *call, const char *name, char *out, size_t size) {
    char area[MAX_PATH];
    if (!dataAreaPath(s, call, area, sizeof area)) return 0;
    const char *problem;
    if (name && (problem = storageNameProblem(name))) {
        refuseLuaCall(s->mod, call, problem, "name=%s", name);
        return 0;
    }
    const char *sep = s->dataFolder[0] ? USER_DIR_SEP : "";
    int ok = name ? buildPath(out, size, call, "%s%s%s" USER_DIR_SEP "%s.ini", area, sep, s->dataFolder, name)
                  : buildPath(out, size, call, "%s%s%s", area, sep, s->dataFolder);
    if (!ok) refuseLuaCall(s->mod, call, "path_too_long", "folder=%s name=%s", s->dataFolder, name ? name : "");
    return ok;
}

// The bytes of every file in folder `dir` and its subfolders.
static long long dataFolderBytes(const char *dir, int depth) {
    char pattern[MAX_PATH], path[MAX_PATH];
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", dir)) return 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    long long total = 0;
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        if (!buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "%s", dir, fd.cFileName)) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (depth > 0) total += dataFolderBytes(path, depth - 1);
        } else {
            total += storageFileBytes(path);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return total;
}
// What script s's area holds now.
static long long dataAreaBytes(LuaScript *s, const char *call) {
    if (s->dataBytesKnown) return s->dataBytes;
    char area[MAX_PATH];
    if (!dataAreaPath(s, call, area, sizeof area)) return -1;
    s->dataBytes = dataFolderBytes(area, 4);
    s->dataBytesKnown = 1;
    writeLog("lua_data_area_size", "mod=%s bytes=%lld max=%lld", s->mod, s->dataBytes, storageAreaMax(s->mod));
    return s->dataBytes;
}
// Makes the folders down to the script's data folder: coe5se_data, the area, and the subfolder.
static void makeDataFolders(LuaScript *s, const char *area) {
    char path[MAX_PATH];
    if (buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP STORAGE_DATA_DIR, userDir())) CreateDirectoryA(path, NULL);
    CreateDirectoryA(area, NULL);
    if (s->dataFolder[0] && buildPath(path, sizeof path, __func__, "%s" USER_DIR_SEP "%s", area, s->dataFolder))
        CreateDirectoryA(path, NULL);
}

// The folders an older CoE5SE kept scripts' files in.
static int luaScriptsLoaded;
#define OLD_DATA_FILES_MAX 4096

// Writes the folder an older CoE5SE kept script s's files in, by its name in the user folder.
static void oldDataFolderName(const LuaScript *s, char *out, size_t size) {
    snprintf(out, size, "%.63s", s->dataFolder[0] ? s->dataFolder : s->mod);
}

// Returns NULL when script s may take folder `old` of the user folder, or why not.
static const char *oldDataFolderProblem(const LuaScript *s, const char *old) {
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (_stricmp(modName(m), s->mod) && !_stricmp(modName(m), old)) return "another_mods_name";
    char other[STORAGE_NAME_MAX + 1];
    for (int i = 0; i < luaScriptCount; i++) {
        if (luaScripts[i] == s) continue;
        oldDataFolderName(luaScripts[i], other, sizeof other);
        if (!_stricmp(other, old)) return "folder_shared";
    }
    return NULL;
}

// Moves script s's .ini files from the folder an older CoE5SE kept them in to its data folder now.
static void moveOldDataFolder(LuaScript *s) {
    char old[STORAGE_NAME_MAX + 1], from[MAX_PATH], real[MAX_PATH];
    oldDataFolderName(s, old, sizeof old);
    if (storageNameProblem(old) || storageIsGameFolder(old)) return;
    if (!buildPath(from, sizeof from, __func__, "%s" USER_DIR_SEP "%s", userDir(), old)) return;
    DWORD attributes = GetFileAttributesA(from);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return;
    const char *problem = !storageStoredName(from, real, sizeof real) || strcmp(real, old) ? "folder_alias" : oldDataFolderProblem(s, old);
    if (problem) {
        writeLog("lua_data_move_skipped", "mod=%s from=%s reason=%s", s->mod, old, problem);
        return;
    }
    char area[MAX_PATH], to[MAX_PATH], pattern[MAX_PATH];
    if (!dataAreaPath(s, "move_old_data", area, sizeof area) || !dataPath(s, "move_old_data", NULL, to, sizeof to)) return;
    if (!buildPath(pattern, sizeof pattern, __func__, "%s" USER_DIR_SEP "*", from)) return;
    typedef char OldFileName[STORAGE_NAME_MAX + 5];
    OldFileName *names = malloc(OLD_DATA_FILES_MAX * sizeof *names);
    if (!names) return;
    int count = 0, moved = 0, left = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            size_t length = strlen(fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || length <= 4 || length >= sizeof *names || _stricmp(fd.cFileName + length - 4, ".ini"))
                continue;
            char stem[sizeof *names];
            snprintf(stem, sizeof stem, "%.*s", (int)(length - 4), fd.cFileName);
            if (storageNameProblem(stem)) continue;
            if (count == OLD_DATA_FILES_MAX) {
                left++;
                continue;
            }
            snprintf(names[count++], sizeof *names, "%s", fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    if (count) makeDataFolders(s, area);
    long long room = storageAreaMax(s->mod) - dataFolderBytes(area, 4);
    char source[MAX_PATH], target[MAX_PATH];
    for (int i = 0; i < count; i++) {
        if (!buildPath(source, sizeof source, __func__, "%s" USER_DIR_SEP "%s", from, names[i]) ||
            !buildPath(target, sizeof target, __func__, "%s" USER_DIR_SEP "%s", to, names[i])) {
            left++;
            continue;
        }
        long long bytes = storageFileBytes(source);
        if (bytes > room || !storageMoveFile(source, target)) {
            left++;
            continue;
        }
        room -= bytes;
        moved++;
    }
    free(names);
    int removed = storageRemoveEmptyFolder(from);
    s->dataBytesKnown = 0;
    if (moved || left)
        writeLog("lua_data_moved", "mod=%s from=%s to=%s files=%d left=%d old_folder_removed=%d", s->mod, old,
                 s->dataFolder[0] ? s->dataFolder : "-", moved, left, removed);
}

// Moves every script's older data folder, once all have loaded.
static void moveOldDataFolders(void) {
    luaScriptsLoaded = 1;
    for (int i = 0; i < luaScriptCount; i++)
        if (!luaScripts[i]->dead) moveOldDataFolder(luaScripts[i]);
}

// Argument `arg` as a data file or folder name.
static const char *luaStorageName(lua_State *L, int arg, const char *call, const char *field) {
    size_t length;
    const char *name = luaL_checklstring(L, arg, &length);
    if (strlen(name) == length) return name;
    LuaScript *s = luaScriptOf(L);
    refuseLuaCall(s ? s->mod : "?", call, "bad_character", "%s=%s", field, name);
    return NULL;
}

// coe5.dataFolder(name): names the subfolder of the mod's area that the script's data files go in.
static int lDataFolder(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.dataFolder", "folder");
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    const char *problem = storageNameProblem(name);
    if (problem) {
        refuseLuaCall(s->mod, "coe5.dataFolder", problem, "folder=%s", name);
        lua_pushboolean(L, 0);
        return 1;
    }
    snprintf(s->dataFolder, sizeof s->dataFolder, "%s", name);
    writeLog("lua_data_folder", "mod=%s folder=%s", s->mod, name);
    if (luaScriptsLoaded) moveOldDataFolder(s);
    lua_pushboolean(L, 1);
    return 1;
}

// iniParse's callback for coe5.readIni: puts one value in the result table on top of the stack.
static void readIniPair(void *ctx, const char *section, const char *key, const char *value) {
    lua_State *L = ctx;
    if (lua_getfield(L, -1, section) != LUA_TTABLE) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, section);
    }
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 1);
}

// coe5.readIni(name): the file's table, or nil when there is no such file.
static int lReadIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.readIni", "name");
    if (!name) {
        lua_pushnil(L);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    char path[MAX_PATH];
    if (!dataPath(s, "coe5.readIni", name, path, sizeof path)) {
        lua_pushnil(L);
        return 1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        lua_pushnil(L);
        return 1;
    }
    long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    if (size < 0 || size > STORAGE_FILE_MAX) {
        fclose(f);
        refuseLuaCall(s->mod, "coe5.readIni", "too_big", "name=%s bytes=%ld max=%d", name, size, STORAGE_FILE_MAX);
        lua_pushnil(L);
        return 1;
    }
    rewind(f);
    char *text = lua_newuserdatauv(L, (size_t)size + 1, 0);
    size_t got = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[got] = '\0';
    lua_newtable(L);
    int firstBad, bad = iniParse(text, readIniPair, L, &firstBad);
    if (bad) refuseLuaCall(s->mod, "coe5.readIni", "bad_lines", "name=%s lines=%d first=%d", name, bad, firstBad);
    return 1;
}

static int compareNames(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

// Pushes the sorted string keys of the table at `t`.
static const char **sortedKeys(lua_State *L, int t, int *count) {
    t = lua_absindex(L, t);
    int n = 0;
    *count = 0;
    for (lua_pushnil(L); lua_next(L, t); lua_pop(L, 1)) {
        size_t len;
        if (lua_type(L, -2) != LUA_TSTRING || strlen(lua_tolstring(L, -2, &len)) != len) {
            lua_pop(L, 2);
            return NULL;
        }
        n++;
    }
    const char **keys = lua_newuserdatauv(L, sizeof *keys * (size_t)(n ? n : 1), 0);
    int i = 0;
    for (lua_pushnil(L); lua_next(L, t); lua_pop(L, 1)) {
        if (i == n || lua_type(L, -2) != LUA_TSTRING) {
            lua_pop(L, 3);
            *count = -1;
            return NULL;
        }
        keys[i++] = lua_tostring(L, -2);
    }
    if (i != n) {
        lua_pop(L, 1);
        *count = -1;
        return NULL;
    }
    qsort(keys, (size_t)n, sizeof *keys, compareNames);
    *count = n;
    return keys;
}

// Goes over coe5.writeIni's table (argument 2) in sorted order.
static const char *iniTableText(lua_State *L, FILE *out, size_t *total, char *detail, size_t detailSize) {
    int sectionCount, keyCount;
    const char **sections = sortedKeys(L, 2, &sectionCount);
    if (!sections) {
        snprintf(detail, detailSize, "section=(%s)", sectionCount < 0 ? "changed while read" : "not a string");
        return sectionCount < 0 ? "table_changed" : "bad_section";
    }
    const char *reason = NULL;
    for (int i = 0; i < sectionCount && !reason; i++) {
        const char *sec = sections[i], *problem = iniNameProblem(sec, 0);
        if (problem || lua_getfield(L, 2, sec) != LUA_TTABLE) {
            snprintf(detail, detailSize, "section=%.80s problem=%s", sec, problem ? problem : "not_a_table");
            if (!problem) lua_pop(L, 1);
            reason = "bad_section";
            break;
        }
        if (out && sec[0]) fprintf(out, "%s[%s]\n", i ? "\n" : "", sec);
        *total += strlen(sec) + 4;
        const char **keys = sortedKeys(L, -1, &keyCount);
        if (!keys) {
            snprintf(detail, detailSize, "section=%.80s key=(%s)", sec, keyCount < 0 ? "changed while read" : "not a string");
            lua_pop(L, 1);
            reason = keyCount < 0 ? "table_changed" : "bad_key";
            break;
        }
        for (int k = 0; k < keyCount && !reason; k++) {
            if ((problem = iniNameProblem(keys[k], 1))) {
                snprintf(detail, detailSize, "section=%.80s key=%.80s problem=%s", sec, keys[k], problem);
                reason = "bad_key";
                break;
            }
            int type = lua_getfield(L, -2, keys[k]);
            size_t len = 0;
            const char *value = type == LUA_TSTRING || type == LUA_TNUMBER ? lua_tolstring(L, -1, &len) : NULL;
            if (!value || (problem = iniValueProblem(value, len))) {
                snprintf(detail, detailSize, "section=%.80s key=%.80s problem=%s", sec, keys[k], value ? problem : "not_text");
                reason = "bad_value";
            } else {
                if (out) fprintf(out, "%s=%s\n", keys[k], value);
                *total += strlen(keys[k]) + len + 2;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 2);
    }
    lua_pop(L, 1);
    return reason;
}

// One coe5.writeIni call.
typedef struct {
    LuaScript *s;
    const char *area, *path;
    long long areaBytes, oldBytes;
    char fresh[MAX_PATH], detail[256];
    const char *reason;
    FILE *f;
    size_t total;
    int written;
} IniWrite;

// lWriteIni's two passes, under lua_pcall: argument 1 the IniWrite, 2 the table.
static int writeIniPasses(lua_State *L) {
    IniWrite *w = lua_touserdata(L, 1);
    w->reason = iniTableText(L, NULL, &w->total, w->detail, sizeof w->detail);
    if (!w->reason && w->total > STORAGE_FILE_MAX) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu max=%d", w->total, STORAGE_FILE_MAX);
        w->reason = "too_big";
    }
    if (w->reason) return 0;
    long long max = storageAreaMax(w->s->mod);
    if (w->areaBytes >= 0 && w->areaBytes - w->oldBytes + (long long)w->total > max) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu area=%lld max=%lld", w->total, w->areaBytes - w->oldBytes, max);
        w->reason = "data_quota";
        return 0;
    }
    makeDataFolders(w->s, w->area);
    if (!(w->f = fopen(w->fresh, "wb"))) return 0;
    size_t again = 0;
    w->reason = iniTableText(L, w->f, &again, w->detail, sizeof w->detail);
    if (!w->reason && again != w->total) {
        snprintf(w->detail, sizeof w->detail, "bytes=%zu then=%zu", w->total, again);
        w->reason = "table_changed";
    }
    int failed = ferror(w->f);
    if (fclose(w->f)) failed = 1;
    w->f = NULL;
    w->written = !w->reason && !failed;
    return 0;
}

// coe5.writeIni(name, t): writes t[section][key] = value as <name>.ini.
static int lWriteIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.writeIni", "name");
    luaL_checktype(L, 2, LUA_TTABLE);
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_settop(L, 2);
    LuaScript *s = checkLuaScript(L);
    char area[MAX_PATH], path[MAX_PATH];
    if (!dataAreaPath(s, "coe5.writeIni", area, sizeof area) || !dataPath(s, "coe5.writeIni", name, path, sizeof path)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    IniWrite w = {.s = s, .area = area, .path = path, .areaBytes = dataAreaBytes(s, "coe5.writeIni"), .oldBytes = storageFileBytes(path)};
    if (!buildPath(w.fresh, sizeof w.fresh, "coe5.writeIni", "%s.new", path)) {
        refuseLuaCall(s->mod, "coe5.writeIni", "path_too_long", "folder=%s name=%s", s->dataFolder, name);
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_gc(L, LUA_GCSTOP);
    lua_pushcfunction(L, writeIniPasses);
    lua_pushlightuserdata(L, &w);
    lua_pushvalue(L, 2);
    int rc = lua_pcall(L, 2, 0, 0);
    lua_gc(L, LUA_GCRESTART);
    if (w.f) fclose(w.f);
    if (rc != LUA_OK || !w.written) remove(w.fresh);
    if (rc != LUA_OK) return lua_error(L);
    if (w.reason) {
        if (!strcmp(w.reason, "data_quota")) s->noticesDue |= LUA_NOTICE_STORAGE;
        refuseLuaCall(s->mod, "coe5.writeIni", w.reason, "name=%s %s", name, w.detail);
        lua_pushboolean(L, 0);
        return 1;
    }
#ifdef _WIN32
    int replaced = w.written && MoveFileExA(w.fresh, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    int replaced = w.written && !rename(w.fresh, path);
#endif
    if (!replaced) {
        remove(w.fresh);
        refuseLuaCall(s->mod, "coe5.writeIni", "write_failed", "name=%s path=%s", name, path);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (s->dataBytesKnown) s->dataBytes += storageFileBytes(path) - w.oldBytes;
    writeLog("lua_data_file_written", "mod=%s name=%s bytes=%zu", s->mod, name, w.total);
    lua_pushboolean(L, 1);
    return 1;
}

// coe5.listIni(): the names of the script's data files, without .ini, sorted.
static int lListIni(lua_State *L) {
    LuaScript *s = checkLuaScript(L);
    char folder[MAX_PATH], pattern[MAX_PATH];
    lua_newtable(L);
    if (!dataPath(s, "coe5.listIni", NULL, folder, sizeof folder)) return 1;
    if (!buildPath(pattern, sizeof pattern, "coe5.listIni", "%s" USER_DIR_SEP "*.ini", folder)) {
        refuseLuaCall(s->mod, "coe5.listIni", "path_too_long", "folder=%s", s->dataFolder);
        return 1;
    }
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return 1;
    int n = 0;
    do {
        size_t len = strlen(fd.cFileName);
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || len <= 4 || _stricmp(fd.cFileName + len - 4, ".ini")) continue;
        fd.cFileName[len - 4] = '\0';
        if (storageNameProblem(fd.cFileName)) continue;
        lua_pushstring(L, fd.cFileName);
        lua_rawseti(L, -2, ++n);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    const char **names = lua_newuserdatauv(L, sizeof *names * (size_t)(n ? n : 1), 0);
    for (int i = 0; i < n; i++) {
        lua_rawgeti(L, -2, i + 1);
        names[i] = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    qsort(names, (size_t)n, sizeof *names, compareNames);
    lua_createtable(L, n, 0);
    for (int i = 0; i < n; i++) {
        lua_pushstring(L, names[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// coe5.deleteIni(name): deletes <name>.ini.
static int lDeleteIni(lua_State *L) {
    const char *name = luaStorageName(L, 1, "coe5.deleteIni", "name");
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }
    LuaScript *s = checkLuaScript(L);
    char path[MAX_PATH];
    if (!dataPath(s, "coe5.deleteIni", name, path, sizeof path)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    long long bytes = storageFileBytes(path);
    if (remove(path) == 0) {
        if (s->dataBytesKnown) s->dataBytes -= bytes;
        writeLog("lua_data_file_deleted", "mod=%s name=%s", s->mod, name);
        lua_pushboolean(L, 1);
        return 1;
    }
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
        refuseLuaCall(s->mod, "coe5.deleteIni", "delete_failed", "name=%s path=%s", name, path);
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, 0);
    return 1;
}
