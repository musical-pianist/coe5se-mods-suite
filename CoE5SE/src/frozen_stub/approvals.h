// The game's user folder (userDir), and in it the approved plugin DLLs:
//   coe5se/approved.txt   one "<sha256>  <dll file name>" line per approved DLL (sha256sum's format)
// The players' Mod Settings picks, auto_approve among them, are settings_files.h's.
// The stubs include this file and players keep their copy for good, so what they use of it can only
// grow in ways old copies still read.
// Include build_path.h first. A file that defines USER_DIR_FROM_INCLUDER first writes userDir
// itself (CoE5SE, which reads the game's own answer once the game has one).

#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_mac.h"
#endif
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SHA256_HEX_LEN 64

// ---- The game's user folder -------------------------------------------------------------------------
// The game picks it once at startup (RVA_SETUP_DIRS, decompiled on both builds): the default,
// "%APPDATA%/coe5" on Windows and "~/.coe5" on Mac, expanded (expandGamePath); if COE5_SAVE is set,
// its value, expanded, instead. It makes that folder (makeGameDir), and when it can't, it uses
// "coe5fallback", relative to the working folder; when that can't be made either, the game stops.
// coe5config, mods, saves, settings, and approvals all live in it.
#ifdef _WIN32
#define USER_DIR_ENV     "APPDATA"
#define USER_DIR_FMT     "%s\\coe5"
#define USER_DIR_SEP     "\\"
#define USER_DIR_DEFAULT "%APPDATA%/coe5"
#else
#define USER_DIR_ENV     "HOME"
#define USER_DIR_FMT     "%s/.coe5"
#define USER_DIR_SEP     "/"
#define USER_DIR_DEFAULT "~/.coe5"
#endif
#define USER_DIR_SAVE_ENV "COE5_SAVE"
#define USER_DIR_FALLBACK "coe5fallback"
#define GAME_PATH_SIZE    2500   // the game's path buffers (0x9c4)

// CoE5SE's folder inside the game's user folder.
#define COE5SE_USER_DIR "coe5se"

// Writes a, b, and c one after another to out[size], cut to fit, as the game's snprintf cuts.
static __attribute__((unused)) void joinGamePath(char *out, size_t size, const char *a, const char *b, const char *c) {
    const char *parts[3] = {a, b, c};
    size_t n = 0;
    for (int i = 0; i < 3; i++)
        for (const char *s = parts[i]; *s && n + 1 < size; s++) out[n++] = *s;
    out[n] = '\0';
}

// The game's expand_path (RVA_EXPAND_PATH), on `path`, which holds `size` bytes: a leading %NAME%
// becomes getenv(NAME), or "." when NAME isn't set (no closing % leaves it as it is); then, when
// HOME is set, "~" and "~/" become HOME and a leading "~/" becomes HOME + "/"; then trailing slashes
// and backslashes go (a path of one character keeps its own); then, with `backslashes` (Windows),
// every slash becomes a backslash. A result longer than `size` is cut, as the game's is.
static __attribute__((unused)) void expandGamePath(char *path, size_t size, int backslashes) {
    char name[GAME_PATH_SIZE], rest[GAME_PATH_SIZE];
    if (path[0] == '%') {
        size_t end = 1;
        while (path[end] && path[end] != '%' && end < sizeof name - 1) end++;
        if (path[end]) {
            memcpy(name, path + 1, end - 1);
            name[end - 1] = '\0';
            joinGamePath(rest, sizeof rest, path + end + 1, "", "");
            const char *value = getenv(name);
            joinGamePath(path, size, value ? value : ".", rest, "");
        }
    }
    const char *home = getenv("HOME");
    if (home && (!strcmp(path, "~/") || !strcmp(path, "~"))) {
        joinGamePath(path, size, home, "", "");
    } else if (home && !strncmp(path, "~/", 2)) {
        joinGamePath(rest, sizeof rest, home, "/", path + 2);
        joinGamePath(path, size, rest, "", "");
    }
    for (size_t len = strlen(path); len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'); len--) path[len - 1] = '\0';
    for (char *c = path; backslashes && *c; c++)
        if (*c == '/') *c = '\\';
}

// The game's make_dir (RVA_MAKE_DIR): 0 when the folder is there, 1 once made (with any missing
// folders above it), -1 when it can't be made.
static __attribute__((unused)) int makeGameDir(const char *path) {
    char dir[GAME_PATH_SIZE];
    size_t len = strlen(path);
    if (!len || len >= sizeof dir) return -1;
    memcpy(dir, path, len + 1);
    if (dir[len - 1] == '/' || dir[len - 1] == '\\') dir[--len] = '\0';
    DWORD attributes = GetFileAttributesA(dir);
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) return 0;
    if (CreateDirectoryA(dir, NULL)) return 1;
    for (size_t i = 1; i < len; i++)
        if (dir[i] == '/' || dir[i] == '\\') {
            char c = dir[i];
            dir[i] = '\0';
            CreateDirectoryA(dir, NULL);   // No log: the folder's own make below says whether it worked
            dir[i] = c;
        }
    return CreateDirectoryA(dir, NULL) ? 1 : -1;
}

// Writes the folder the game will use as its user folder to `out`, made as the game makes it, by
// the rule above. Returns 0, with the reason in *reason, when there is none: no_user_dir (the game
// stops then) or path_too_long (it doesn't fit in `size`; buildPath logs it).
static __attribute__((unused)) int findGameUserDir(char *out, size_t size, const char **reason) {
    char dir[GAME_PATH_SIZE];
#ifdef _WIN32
    int backslashes = 1;
#else
    int backslashes = 0;
#endif
    snprintf(dir, sizeof dir, "%s", USER_DIR_DEFAULT);
    expandGamePath(dir, sizeof dir, backslashes);
    const char *save = getenv(USER_DIR_SAVE_ENV);
    if (save) {
        snprintf(dir, sizeof dir, "%s", save);
        expandGamePath(dir, sizeof dir, backslashes);
    }
    if (makeGameDir(dir) < 0) {
        snprintf(dir, sizeof dir, "%s", USER_DIR_FALLBACK);
        expandGamePath(dir, sizeof dir, backslashes);
        if (makeGameDir(dir) < 0) {
            *reason = "no_user_dir";
            return 0;
        }
    }
    if (!buildPath(out, size, __func__, "%s", dir)) {
        *reason = "path_too_long";
        return 0;
    }
    return 1;
}

// Writes the user folder that stubs up to version 7 read to `out`: USER_DIR_ENV's value, or "."
// when it isn't set, plus "\coe5" or "/.coe5". The loader uses it for those stubs, so they keep
// reading the folder they always read. Returns 0 when it doesn't fit (buildPath logs it).
static __attribute__((unused)) int legacyUserDir(char *out, size_t size) {
    const char *root = getenv(USER_DIR_ENV);
    return buildPath(out, size, __func__, USER_DIR_FMT, root ? root : ".");
}

// The user folder this process uses, once set: the stub sets it with findGameUserDir, the loader
// from the stub's LoaderStart, CoE5SE as userDir says.
static char userDirPath[MAX_PATH];

#ifdef USER_DIR_FROM_INCLUDER
static const char *userDir(void);
#else
// The user folder: userDirPath, found by the game's rule the first time when nothing set it
// (USER_DIR_FALLBACK when even that fails, which is the folder the game would try last).
static __attribute__((unused)) const char *userDir(void) {
    const char *reason;
    if (!userDirPath[0] && !findGameUserDir(userDirPath, sizeof userDirPath, &reason))
        snprintf(userDirPath, sizeof userDirPath, "%s", USER_DIR_FALLBACK);
    return userDirPath;
}
#endif

// Writes the full path of `file` in CoE5SE's folder to `out`, creating the folder first if needed.
// Returns 0, with `out` empty, when the path doesn't fit (buildPath logs it).
static __attribute__((unused)) int coe5seUserFile(const char *file, char *out, size_t size) {
    if (!buildPath(out, size, __func__, "%s" USER_DIR_SEP COE5SE_USER_DIR, userDir())) return 0;
    CreateDirectoryA(out, NULL);
    return buildPath(out, size, __func__, "%s" USER_DIR_SEP COE5SE_USER_DIR USER_DIR_SEP "%s", userDir(), file);
}

// ---- CoE5SE's files left in the folder older stubs read --------------------------------------------
// A player who used CoE5SE with COE5_SAVE set under a stub up to version 7 has CoE5SE's own files
// (coe5se/, jazzy_mod_settings/) in legacyUserDir's folder, not in the game's folder, which a version 8
// stub and CoE5SE use. Each such file the user folder doesn't have is read from the old folder, and
// copied into the user folder before its first write there, so no approval, pick, or untick is lost.
// The old folder is never written or deleted. With no COE5_SAVE the two folders are one, and nothing
// here does anything.

// Writes the old folder's path of `file` in its subfolder `folder` to `out`, and returns 1, when the
// old folder isn't the user folder and has the file.
static __attribute__((unused)) int oldUserFile(const char *folder, const char *file, char *out, size_t size) {
    char old[MAX_PATH];
    if (!legacyUserDir(old, sizeof old) || !strcmp(old, userDir())) return 0;   // Logged by buildPath, or the same folder
    return buildPath(out, size, __func__, "%s" USER_DIR_SEP "%s" USER_DIR_SEP "%s", old, folder, file) &&
           GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES;
}

// Writes the path to read `file` (in the user folder's subfolder `folder`) from to `out`: the user
// folder's, or the old folder's when only that has it. Returns 0 when the path doesn't fit.
static __attribute__((unused)) int userFileToRead(const char *folder, const char *file, char *out, size_t size) {
    char old[MAX_PATH];
    if (!buildPath(out, size, __func__, "%s" USER_DIR_SEP "%s" USER_DIR_SEP "%s", userDir(), folder, file)) return 0;
    if (GetFileAttributesA(out) == INVALID_FILE_ATTRIBUTES && oldUserFile(folder, file, old, sizeof old))
        snprintf(out, size, "%s", old);   // No log: as long as `out`, which held a longer path
    return 1;
}

// Copies the file at `from` to `to`, through `to`.new and a rename. Returns 0 if it didn't.
static __attribute__((unused)) int copyUserFile(const char *from, const char *to) {
    char fresh[MAX_PATH], buf[4096];
    if (!buildPath(fresh, sizeof fresh, __func__, "%s.new", to)) return 0;
    FILE *in = fopen(from, "rb"), *out = in ? fopen(fresh, "wb") : NULL;
    int ok = in && out;
    size_t n;
    while (ok && (n = fread(buf, 1, sizeof buf, in)) > 0) ok = fwrite(buf, 1, n, out) == n;
    if (in && ferror(in)) ok = 0;
    if (in) fclose(in);
    if (out && fclose(out)) ok = 0;
#ifdef _WIN32
    ok = ok && MoveFileExA(fresh, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    ok = ok && !rename(fresh, to);
#endif
    if (!ok) remove(fresh);
    return ok;
}

// Writes the user folder's path of `file` in its subfolder `folder` to `out`, making the subfolder,
// and first copying the old folder's file there when only that has it (logged
// user_file_copied, or user_file_not_copied). Returns 0 when the path doesn't fit.
static __attribute__((unused)) int userFileToWrite(const char *folder, const char *file, char *out, size_t size) {
    char old[MAX_PATH];
    if (!buildPath(out, size, __func__, "%s" USER_DIR_SEP "%s", userDir(), folder)) return 0;
    CreateDirectoryA(out, NULL);   // No log: a missing folder fails the caller's write, which logs
    if (!buildPath(out, size, __func__, "%s" USER_DIR_SEP "%s" USER_DIR_SEP "%s", userDir(), folder, file)) return 0;
    if (GetFileAttributesA(out) == INVALID_FILE_ATTRIBUTES && oldUserFile(folder, file, old, sizeof old)) {
        if (copyUserFile(old, out)) writeLog("user_file_copied", "file=%s" USER_DIR_SEP "%s", folder, file);
        else writeLog("user_file_not_copied", "file=%s" USER_DIR_SEP "%s", folder, file);
    }
    return 1;
}

static __attribute__((unused)) int isHashApproved(const char *hash) {
    char path[MAX_PATH], line[512];
    if (!userFileToRead(COE5SE_USER_DIR, "approved.txt", path, sizeof path)) return 0;
    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno != ENOENT) writeLog("approvals_unreadable", "path=%s errno=%d", path, errno);   // No log for a missing file: nothing approved yet
        return 0;
    }
    int approved = 0;
    while (!approved && fgets(line, sizeof line, f))
        approved = !_strnicmp(line, hash, SHA256_HEX_LEN) && (line[SHA256_HEX_LEN] == ' ' || line[SHA256_HEX_LEN] == '\t');
    fclose(f);
    return approved;
}

// Adds the hash to approved.txt (the old folder's approvals copied in first, when only it has them).
// Returns 0 if it couldn't be written.
static __attribute__((unused)) int approveHash(const char *hash, const char *dllName) {
    char path[MAX_PATH];
    if (!userFileToWrite(COE5SE_USER_DIR, "approved.txt", path, sizeof path)) return 0;
    FILE *f = fopen(path, "a");
    if (!f) return 0;
    int ok = fprintf(f, "%s  %s\n", hash, dllName) > 0;
    return fclose(f) == 0 && ok;
}
