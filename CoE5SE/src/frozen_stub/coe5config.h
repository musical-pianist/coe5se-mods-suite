// coe5config, the game's settings file in its user folder, read as the game reads it.

#pragma once
#include <stdio.h>
#include <string.h>

#define CONFIG_FILE          "coe5config"
#define CONFIG_KEY           0x4F   // each name byte is XOR this, and the byte that ends a name is this
#ifndef CONFIG_VERSION_MIN
#define CONFIG_VERSION_MIN   0x7c
#endif
#ifndef CONFIG_VERSION_MAX
#define CONFIG_VERSION_MAX   0x81
#endif
#ifndef CONFIG_SETTINGS_INTS
#define CONFIG_SETTINGS_INTS 11
#endif
#ifndef CONFIG_MODS_OFFSET
#define CONFIG_MODS_OFFSET   (4 + CONFIG_SETTINGS_INTS * 4)   // where the enabled list starts
#endif
#define CONFIG_MAX           65536   // the largest coe5config addModToConfigFile rewrites
// The layout file beside the loader: where coe5config's enabled list starts for each newer version.
#define CONFIG_LAYOUT_FILE   "coe5config_layout.txt"

static __attribute__((unused)) unsigned readLittleEndian32(const unsigned char *b) {
    return b[0] | b[1] << 8 | b[2] << 16 | (unsigned)b[3] << 24;
}

// Opens the coe5config at `path` at the start of its enabled list.
static __attribute__((unused)) FILE *openConfigModList(const char *path, int *version, const char **reason) {
    FILE *f = fopen(path, "rb");
    *version = 0;
    if (!f) {
        *reason = "no_config";
        return NULL;
    }
    unsigned char b[4];
    if (fread(b, 1, 4, f) == 4) *version = (int)readLittleEndian32(b);
    if (*version < CONFIG_VERSION_MIN || *version > CONFIG_VERSION_MAX || fseek(f, CONFIG_MODS_OFFSET, SEEK_SET)) {
        fclose(f);
        *reason = "unknown_config_version";
        return NULL;
    }
    return f;
}

// Reads the next enabled mod's name into `name`; 0 at the end of the list.
static __attribute__((unused)) int readConfigModName(FILE *f, char *name, size_t size) {
    size_t len = 0;
    int c;
    while ((c = fgetc(f)) != EOF && (c ^= CONFIG_KEY) != 0)
        if (len < size - 1) name[len++] = (char)c;
    name[len] = '\0';
    return c != EOF && len > 0;
}

// 1 if `version` is newer than any the game reads today.
static __attribute__((unused)) int isNewerConfigVersion(int version) {
    return version > CONFIG_VERSION_MAX;
}

// 1 if coe5config's bytes `data[0..len)` hold `mod` as a whole name.
static __attribute__((unused)) int configDataHasModName(const unsigned char *data, size_t len, const char *mod) {
    size_t modLen = strlen(mod);
    if (!modLen) return 0;
    for (size_t pos = 4; pos + modLen < len; pos++) {
        int start = data[pos - 1] == CONFIG_KEY || pos == CONFIG_MODS_OFFSET || (pos % 4 == 0 && data[pos - 1] == 0);
        if (!start || data[pos + modLen] != CONFIG_KEY) continue;
        size_t i = 0;
        while (i < modLen && (data[pos + i] ^ CONFIG_KEY) == (unsigned char)mod[i]) i++;
        if (i == modLen) return 1;
    }
    return 0;
}

// Reads up to `size` bytes of the file at `path` into `data`. Returns how many, 0 without the file.
static __attribute__((unused)) size_t readConfigBytes(const char *path, unsigned char *data, size_t size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t len = fread(data, 1, size, f);
    fclose(f);
    return len;
}

// Reads a decimal number of up to 9 digits at *p into *out and moves *p past it.
static __attribute__((unused)) int readLayoutNumber(const char **p, int *out) {
    int digits = 0;
    *out = 0;
    while (**p >= '0' && **p <= '9') {
        if (++digits > 9) return 0;
        *out = *out * 10 + (*(*p)++ - '0');
    }
    return digits > 0;
}

// The byte where coe5config's enabled list starts for `version`.
#define CONFIG_LAYOUT_MAX 16384
static __attribute__((unused)) int configLayoutListStart(const char *path, int version, const char **why) {
    static char text[CONFIG_LAYOUT_MAX + 1];
    FILE *f = path ? fopen(path, "rb") : NULL;
    if (!f) {
        *why = "no_layout_file";
        return -1;
    }
    size_t len = fread(text, 1, CONFIG_LAYOUT_MAX, f);
    int at = -1, garbled = ferror(f) || len == CONFIG_LAYOUT_MAX || memchr(text, 0, len) != NULL;
    fclose(f);
    text[len] = '\0';
    for (char *line = text, *next; !garbled && line; line = next) {
        next = strchr(line, '\n');
        if (next) *next++ = '\0';
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\r' || !*p) continue;   // a comment, or a blank line
        int listed, start;
        if (!readLayoutNumber(&p, &listed) || (*p != ' ' && *p != '\t')) garbled = 1;
        while (!garbled && (*p == ' ' || *p == '\t')) p++;
        if (!garbled && !readLayoutNumber(&p, &start)) garbled = 1;
        while (!garbled && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
        if (garbled || *p || start < 4 || start >= CONFIG_MAX) garbled = 1;
        else if (listed == version) {
            if (at >= 0) garbled = 1;   // listed twice
            at = start;
        }
    }
    *why = garbled ? "garbled" : at < 0 ? "unlisted" : NULL;
    return garbled ? -1 : at;
}

// 1 if the coe5config at `path` lists `mod` as enabled.
static __attribute__((unused)) int configEnablesMod(const char *path, const char *mod, const char *layoutPath) {
    const char *reason;
    int version, enabled = 0;
    char name[256];
    FILE *f = openConfigModList(path, &version, &reason);
    if (!f && isNewerConfigVersion(version)) {
        const char *why;
        int at = configLayoutListStart(layoutPath, version, &why);
        if (at >= 0) {
            f = fopen(path, "rb");
            if (f && !fseek(f, at, SEEK_SET))
                while (!enabled && readConfigModName(f, name, sizeof name)) enabled = !strcmp(name, mod);
            if (f) fclose(f);
            writeLog("config_layout_read", "version=%d list_at=%d mod=%s found=%d file=" CONFIG_LAYOUT_FILE, version, at, mod, enabled);
            return enabled;
        }
        static unsigned char data[CONFIG_MAX];
        enabled = configDataHasModName(data, readConfigBytes(path, data, sizeof data), mod);
        writeLog("config_name_search", "reason=%s version=%d mod=%s found=%d layout=%s", reason, version, mod, enabled, why);
        return enabled;
    }
    if (!f) return 0;
    while (!enabled && readConfigModName(f, name, sizeof name)) enabled = !strcmp(name, mod);
    fclose(f);
    return enabled;
}

// Adds `mod` to the enabled list in coe5config's bytes, in place.
static __attribute__((unused)) const char *addModToConfig(unsigned char *data, size_t *len, size_t cap, const char *mod) {
    if (*len < CONFIG_MODS_OFFSET) return "config_unreadable";
    unsigned version = readLittleEndian32(data);
    if (version < CONFIG_VERSION_MIN || version > CONFIG_VERSION_MAX) return "config_version";
    size_t pos = CONFIG_MODS_OFFSET, modLen = strlen(mod);
    for (;;) {
        size_t start = pos;
        while (pos < *len && data[pos] != CONFIG_KEY) pos++;
        if (pos >= *len) return "config_unreadable";
        if (pos == start) break;   // the empty string that ends the list
        int same = pos - start == modLen;
        for (size_t i = 0; same && i < modLen; i++) same = (data[start + i] ^ CONFIG_KEY) == (unsigned char)mod[i];
        if (same) return "already_enabled";
        pos++;
    }
    if (*len + modLen + 1 > cap) return "config_too_big";
    memmove(data + pos + modLen + 1, data + pos, *len - pos);
    for (size_t i = 0; i < modLen; i++) data[pos + i] = (unsigned char)mod[i] ^ CONFIG_KEY;
    data[pos + modLen] = CONFIG_KEY;
    *len += modLen + 1;
    return NULL;
}

// Writes `len` bytes to `path`. Returns 0 if any of them didn't reach the file.
static __attribute__((unused)) int writeWholeFile(const char *path, const unsigned char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(data, 1, len, f) == len;
    return fclose(f) == 0 && ok;
}

// Adds `mod` to the enabled list of the coe5config at `path`, through a backup and a rename.
static __attribute__((unused)) const char *addModToConfigFile(const char *path, const char *backup, const char *mod) {
    static unsigned char data[CONFIG_MAX];
    char fresh[MAX_PATH];
    if (!buildPath(fresh, sizeof fresh, __func__, "%s.new", path)) return "config_write_failed";
    FILE *f = fopen(path, "rb");
    if (!f) return "config_missing";
    size_t len = fread(data, 1, sizeof data, f);
    int failed = ferror(f);
    fclose(f);
    if (failed) return "config_unreadable";
    if (len == sizeof data) return "config_too_big";   // it may go on past the buffer
    if (!writeWholeFile(backup, data, len)) return "config_write_failed";
    const char *reason = addModToConfig(data, &len, sizeof data, mod);
    if (reason) return reason;
    if (!writeWholeFile(fresh, data, len)) {
        remove(fresh);
        return "config_write_failed";
    }
#ifdef _WIN32
    int replaced = MoveFileExA(fresh, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    int replaced = rename(fresh, path) == 0;
#endif
    if (!replaced) {
        remove(fresh);
        return "config_write_failed";
    }
    return NULL;
}
