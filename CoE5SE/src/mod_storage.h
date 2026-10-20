// Mods' own data files.
#pragma once
#include <stddef.h>
#include <string.h>
#ifndef _WIN32
#include <sys/attr.h>
#endif
#include "approvals.h"
#include "settings_files.h"

#define STORAGE_NAME_MAX   64
#define STORAGE_TEXT_MAX   1024
#define STORAGE_FILE_MAX   (1024 * 1024)
#define STORAGE_AREA_DEFAULT_MB 10
#define STORAGE_AREA_CEILING_MB 100
#define STORAGE_AREA_NAME_MAX 255
#define STORAGE_DATA_DIR   "coe5se_data"

// Returns NULL when `name` can name a file or folder, or the reason word it can't.
static const char *storageNameProblemUpTo(const char *name, size_t max) {
    if (!name || !name[0]) return "empty_name";
    size_t len = strlen(name);
    if (len > max) return "name_too_long";
    for (const char *c = name; *c; c++)
        if ((unsigned char)*c < 0x20 || strchr("/\\:*?\"<>|", *c)) return "bad_character";
    if (name[0] == ' ' || name[0] == '.' || name[len - 1] == ' ' || name[len - 1] == '.') return "bad_name";
    static const char *const DEVICES[] = {"con", "prn", "aux", "nul"};
    size_t stem = strcspn(name, ".");
    for (size_t i = 0; i < sizeof DEVICES / sizeof DEVICES[0]; i++)
        if (stem == 3 && !_strnicmp(name, DEVICES[i], 3)) return "reserved_name";
    if (stem == 4 && (!_strnicmp(name, "com", 3) || !_strnicmp(name, "lpt", 3)) && name[3] >= '1' && name[3] <= '9')
        return "reserved_name";
    return NULL;
}
// A data file's name (without .ini) or a subfolder's that coe5.dataFolder names: up to 64 bytes.
static const char *storageNameProblem(const char *name) { return storageNameProblemUpTo(name, STORAGE_NAME_MAX); }
// A mod's area, named after the mod in full: up to a mod name's 255 bytes.
static const char *storageAreaProblem(const char *mod) { return storageNameProblemUpTo(mod, STORAGE_AREA_NAME_MAX); }

// @platform-split storage_stored_name: Windows asks FindFirstFileA, a Mac getattrlist
// Writes the name the file or folder at `path` is stored under to `out`.
static int storageStoredName(const char *path, char *out, size_t size) {
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    FindClose(h);
    size_t len = strlen(fd.cFileName);
    if (len >= size) return 0;
    memcpy(out, fd.cFileName, len + 1);
    return 1;
#else
    struct attrlist want = {.bitmapcount = ATTR_BIT_MAP_COUNT, .commonattr = ATTR_CMN_NAME};
    typedef struct {
        uint32_t length;
        attrreference_t name;
        char text[MAX_PATH];
    } __attribute__((aligned(4), packed)) NameReply;
    NameReply got;
    if (getattrlist(path, &want, &got, sizeof got, FSOPT_NOFOLLOW)) return 0;
    size_t start = offsetof(NameReply, name) + (size_t)got.name.attr_dataoffset;
    if (got.name.attr_dataoffset < 0 || start >= sizeof got || got.name.attr_length > sizeof got - start) return 0;
    const char *name = (const char *)&got + start;
    size_t len = strnlen(name, got.name.attr_length);
    if (len >= size) return 0;
    memcpy(out, name, len);
    out[len] = '\0';
    return 1;
#endif
}

// @platform-split storage_file_bytes: Windows asks GetFileAttributesExA, a Mac stat
// The bytes of the file at `path`, or 0 when there is none.
static long long storageFileBytes(const char *path) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) return 0;
    return (long long)(((unsigned long long)data.nFileSizeHigh << 32) | data.nFileSizeLow);
#else
    struct stat st;
    return stat(path, &st) || !S_ISREG(st.st_mode) ? 0 : (long long)st.st_size;
#endif
}

// The folders of the user folder that are the game's or CoE5SE's.
static const char *const STORAGE_GAME_FOLDERS[] = {"mods", "maps", "saves", "gamelogs", "coe5config", "oldworkshop", "oldipadr",
                                                   COE5SE_USER_DIR, MOD_SETTINGS_DIR, SETTINGS_INI_DIR, STORAGE_DATA_DIR};
static int storageIsGameFolder(const char *name) {
    for (size_t i = 0; i < sizeof STORAGE_GAME_FOLDERS / sizeof STORAGE_GAME_FOLDERS[0]; i++)
        if (!_stricmp(name, STORAGE_GAME_FOLDERS[i])) return 1;
    return 0;
}

// @platform-split storage_move: Windows MoveFileExA without MOVEFILE_REPLACE_EXISTING and RemoveDirectoryA, a Mac renamex_np with RENAME_EXCL and rmdir
// Moves the file at `from` to `to`, never over a file already at `to`; returns 1 when it moved.
static int storageMoveFile(const char *from, const char *to) {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_WRITE_THROUGH) != 0;
#else
    return renamex_np(from, to, RENAME_EXCL) == 0;
#endif
}
static int storageRemoveEmptyFolder(const char *path) {
#ifdef _WIN32
    return RemoveDirectoryA(path) != 0;
#else
    return rmdir(path) == 0;
#endif
}

// Returns NULL when `text` can be written as a section name or key, or the reason word it can't.
static const char *iniNameProblem(const char *text, int key) {
    size_t len = strlen(text);
    if (len > STORAGE_TEXT_MAX) return "too_long";
    if (key && !len) return "empty_key";
    if (strpbrk(text, "\r\n]")) return "bad_character";
    if (key && (strchr(text, '=') || strchr("[;#", text[0]))) return "bad_character";
    if (len && (text[0] == ' ' || text[0] == '\t' || text[len - 1] == ' ' || text[len - 1] == '\t')) return "outer_space";
    return NULL;
}

// Returns NULL when `text` can be written as a value, or the reason word it can't.
static const char *iniValueProblem(const char *text, size_t len) {
    if (len > STORAGE_TEXT_MAX) return "too_long";
    if (memchr(text, '\n', len) || memchr(text, '\r', len) || memchr(text, '\0', len)) return "bad_character";
    if (len && (text[0] == ' ' || text[0] == '\t' || text[len - 1] == ' ' || text[len - 1] == '\t')) return "outer_space";
    return NULL;
}

// Trims spaces and tabs at both ends of the text from `start` to `end`, in place; returns the start.
static char *iniTrim(char *start, char *end) {
    while (start < end && (*start == ' ' || *start == '\t')) start++;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
    *end = '\0';
    return start;
}

// Reads INI text in place, calling `pair` for each key=value line.
static int iniParse(char *text, void (*pair)(void *ctx, const char *section, const char *key, const char *value), void *ctx,
                    int *firstBad) {
    const char *section = "";
    int bad = 0, number = 0;
    char *line = text;
    if (firstBad) *firstBad = 0;
    while (line && *line) {
        number++;
        char *end = strchr(line, '\n');
        char *next = end ? end + 1 : NULL;
        if (!end) end = line + strlen(line);
        if (end > line && end[-1] == '\r') end--;
        char *t = iniTrim(line, end);
        if (!*t || *t == ';' || *t == '#') {
        } else if (*t == '[') {
            char *close = strchr(t, ']');
            if (close) section = iniTrim(t + 1, close);
            else if (!bad++ && firstBad) *firstBad = number;
        } else {
            char *eq = strchr(t, '=');
            if (eq) {
                char *value = iniTrim(eq + 1, eq + 1 + strlen(eq + 1));
                pair(ctx, section, iniTrim(t, eq), value);
            } else if (!bad++ && firstBad) {
                *firstBad = number;
            }
        }
        line = next;
    }
    return bad;
}
