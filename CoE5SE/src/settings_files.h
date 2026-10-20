// Where the players' Mod Settings picks are kept, in the user folder.
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Each mod's own picks file is in this folder.
#define MOD_SETTINGS_DIR  "coe5seModMenu"
// The old picks file.
#define SETTINGS_INI_DIR  "jazzy_mod_settings"
#define SETTINGS_INI_FILE "settings.ini"

// Every file here is read from the old folder when only that has it.

// Writes the path settings.ini is read from to `out`.
static __attribute__((unused)) int settingsIniPath(char *out, size_t size) {
    return userFileToRead(SETTINGS_INI_DIR, SETTINGS_INI_FILE, out, size);
}

// Writes the path `mod`'s own settings file is read from to `out`.
static __attribute__((unused)) int modSettingsPath(const char *mod, char *out, size_t size) {
    char file[MAX_PATH];
    return buildPath(file, sizeof file, __func__, "%s.ini", mod) && userFileToRead(MOD_SETTINGS_DIR, file, out, size);
}

// Writes the path of the file `mod`'s picks are read from to `out`.
static __attribute__((unused)) int settingsReadPath(const char *mod, char *out, size_t size) {
    if (!modSettingsPath(mod, out, size)) return 0;
    return GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES || settingsIniPath(out, size);
}

// Copies `mod`'s section of the settings.ini text `text` to `out`.
static __attribute__((unused)) int copySettingsSection(const char *text, const char *mod, FILE *out) {
    int ok = fprintf(out, "[%s]\n", mod) > 0, inSection = 0, seen = 0;
    size_t modLen = strlen(mod);
    for (const char *line = text; ok && line && *line;) {
        const char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) + 1 : strlen(line);
        const char *p = line;
        while (p < line + len && (*p == ' ' || *p == '\t')) p++;
        if (*p == '[') {
            const char *close = memchr(p, ']', len - (size_t)(p - line));
            int match = close && (size_t)(close - p - 1) == modLen && !_strnicmp(p + 1, mod, modLen);
            if (inSection || (match && seen)) break;
            inSection = seen = match;
        } else if (inSection) {
            ok = fwrite(line, 1, len, out) == len;
            if (ok && !next) ok = fputc('\n', out) != EOF;
        }
        line = next ? next + 1 : NULL;
    }
    return ok;
}

// Reads the whole file at `path` into a new string.
static __attribute__((unused)) char *readWholeSettingsFile(const char *path, int *missing, size_t *length) {
    FILE *f = fopen(path, "rb");
    *missing = !f;
    if (!f) return NULL;
    long size = fseek(f, 0, SEEK_END) ? -1 : ftell(f);
    char *text = size >= 0 && !fseek(f, 0, SEEK_SET) ? malloc((size_t)size + 1) : NULL;
    size_t got = text ? fread(text, 1, (size_t)size, f) : 0;
    int failed = ferror(f);
    fclose(f);
    if (!text || failed || got != (size_t)size) {
        free(text);
        return NULL;
    }
    text[got] = '\0';
    *length = got;
    return text;
}

// Writes the path of the file `mod`'s picks are written to.
static __attribute__((unused)) int settingsWritePath(const char *mod, char *out, size_t size) {
    char file[MAX_PATH], legacy[MAX_PATH], fresh[MAX_PATH];
    if (!buildPath(file, sizeof file, __func__, "%s.ini", mod) || !userFileToWrite(MOD_SETTINGS_DIR, file, out, size))
        return 0;
    if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) return 1;
    if (!settingsIniPath(legacy, sizeof legacy) || !buildPath(fresh, sizeof fresh, __func__, "%s.new", out)) return 0;
    int missing;
    size_t length;
    char *text = readWholeSettingsFile(legacy, &missing, &length);
    if (!text && !missing) {
        writeLog("settings_file_not_made", "mod=%s reason=settings_ini_unreadable", mod);
        return 0;
    }
    FILE *f = fopen(fresh, "wb");
    int ok = f && copySettingsSection(text ? text : "", mod, f);
    if (f && fclose(f)) ok = 0;
    free(text);
#ifdef _WIN32
    ok = ok && MoveFileExA(fresh, out, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    ok = ok && !rename(fresh, out);
#endif
    if (!ok) {
        remove(fresh);
        writeLog("settings_file_not_made", "mod=%s reason=write_failed", mod);
        return 0;
    }
    writeLog("settings_file_made", "mod=%s from_settings_ini=%d", mod, !missing);
    return 1;
}

// The auto-approve setting: its mod, name, and default.
#define AUTO_APPROVE_MOD     "CoE5SE"
#define AUTO_APPROVE_NAME    "auto_approve"
#define AUTO_APPROVE_DEFAULT 1

// The value of CoE5SE's setting `name` as the player picked it, or `fallback`.
static __attribute__((unused)) int readCoe5seIntSetting(const char *name, int fallback) {
    char path[MAX_PATH];
    if (!settingsReadPath(AUTO_APPROVE_MOD, path, sizeof path)) return fallback;
    return (int)GetPrivateProfileIntA(AUTO_APPROVE_MOD, name, fallback, path);
}

static __attribute__((unused)) int readAutoApprove(void) {
    return readCoe5seIntSetting(AUTO_APPROVE_NAME, AUTO_APPROVE_DEFAULT) != 0;
}
