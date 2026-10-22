// Mac versions of the Win32 calls the plugins use, so one source builds for both platforms.
#pragma once
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fnmatch.h>
#include <mach-o/dyld.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX_PATH 1024
#define WINAPI
#define __declspec(x) __attribute__((visibility("default")))
typedef int BOOL;
typedef unsigned int DWORD, UINT;
typedef unsigned short WORD;
typedef void *LPVOID, *HANDLE;
typedef const void *HMODULE, *HINSTANCE;
typedef const char *LPCSTR;
#define TRUE 1
#define FALSE 0
#define DLL_PROCESS_ATTACH 1
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 4
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 2
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define ZeroMemory(p, n) memset((p), 0, (n))

#define PLATFORM_UNUSED __attribute__((unused))

static PLATFORM_UNUSED void DisableThreadLibraryCalls(HINSTANCE dll) { (void)dll; }
static PLATFORM_UNUSED DWORD GetCurrentProcessId(void) { return (DWORD)getpid(); }

typedef struct { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME;
static PLATFORM_UNUSED void GetLocalTime(SYSTEMTIME *t) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm lt;
    localtime_r(&tv.tv_sec, &lt);
    t->wYear = (WORD)(lt.tm_year + 1900);
    t->wMonth = (WORD)(lt.tm_mon + 1);
    t->wDayOfWeek = (WORD)lt.tm_wday;
    t->wDay = (WORD)lt.tm_mday;
    t->wHour = (WORD)lt.tm_hour;
    t->wMinute = (WORD)lt.tm_min;
    t->wSecond = (WORD)lt.tm_sec;
    t->wMilliseconds = (WORD)(tv.tv_usec / 1000);
}

// ---- Files -------------------------------------------------------------------------------------
static PLATFORM_UNUSED BOOL CreateDirectoryA(const char *path, void *security) {
    (void)security;
    return mkdir(path, 0755) == 0;
}

static PLATFORM_UNUSED DWORD GetFileAttributesA(const char *path) {
    struct stat st;
    if (stat(path, &st)) return INVALID_FILE_ATTRIBUTES;
    return S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : 0;
}

typedef struct {
    DWORD dwFileAttributes;
    char cFileName[MAX_PATH];
} WIN32_FIND_DATAA;

typedef struct {
    DIR *dir;
    char folder[MAX_PATH], pattern[256];
} FindState;

// Fills `fd` with the next entry that matches the pattern. Returns 0 when there are no more.
static PLATFORM_UNUSED int findNextMatch(FindState *s, WIN32_FIND_DATAA *fd) {
    struct dirent *e;
    while ((e = readdir(s->dir))) {
        if (fnmatch(s->pattern, e->d_name, FNM_CASEFOLD)) continue;
        char full[MAX_PATH * 2];
        snprintf(full, sizeof full, "%s/%s", s->folder, e->d_name);
        snprintf(fd->cFileName, sizeof fd->cFileName, "%s", e->d_name);
        fd->dwFileAttributes = GetFileAttributesA(full) & FILE_ATTRIBUTE_DIRECTORY;
        return 1;
    }
    return 0;
}

static PLATFORM_UNUSED HANDLE FindFirstFileA(const char *pattern, WIN32_FIND_DATAA *fd) {
    FindState *s = calloc(1, sizeof *s);
    if (!s) return INVALID_HANDLE_VALUE;
    const char *slash = strrchr(pattern, '/');
    snprintf(s->folder, sizeof s->folder, "%.*s", slash ? (int)(slash - pattern) : 1, slash ? pattern : ".");
    snprintf(s->pattern, sizeof s->pattern, "%s", slash ? slash + 1 : pattern);
    if (!(s->dir = opendir(s->folder)) || !findNextMatch(s, fd)) {
        if (s->dir) closedir(s->dir);
        free(s);
        return INVALID_HANDLE_VALUE;
    }
    return s;
}

static PLATFORM_UNUSED BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA *fd) { return findNextMatch((FindState *)h, fd); }

static PLATFORM_UNUSED BOOL FindClose(HANDLE h) {
    FindState *s = h;
    closedir(s->dir);
    free(s);
    return TRUE;
}

// ---- Profile (INI) files -------------------------------------------------------------------------
// Reads the value of `key` in `[section]` into `out`, with spaces around it trimmed.
static PLATFORM_UNUSED int iniRead(const char *file, const char *section, const char *key, char *out, size_t size) {
    FILE *f = fopen(file, "r");
    if (!f) return 0;
    char line[1024];
    int inSection = 0, found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '[') {
            char *end = strchr(p, ']');
            inSection = end && (size_t)(end - p - 1) == strlen(section) && !strncasecmp(p + 1, section, end - p - 1);
            continue;
        }
        char *eq = strchr(p, '=');
        if (!inSection || !eq) continue;
        char *k = eq;
        while (k > p && isspace((unsigned char)k[-1])) k--;
        if ((size_t)(k - p) != strlen(key) || strncasecmp(p, key, k - p)) continue;
        char *v = eq + 1, *end = v + strlen(v);
        while (isspace((unsigned char)*v)) v++;
        while (end > v && isspace((unsigned char)end[-1])) *--end = '\0';
        snprintf(out, size, "%s", v);
        found = 1;
    }
    fclose(f);
    return found;
}

static PLATFORM_UNUSED DWORD GetPrivateProfileStringA(const char *section, const char *key, const char *fallback,
                                                      char *out, DWORD size, const char *file) {
    if (!iniRead(file, section, key, out, size)) snprintf(out, size, "%s", fallback ? fallback : "");
    return (DWORD)strlen(out);
}

static PLATFORM_UNUSED UINT GetPrivateProfileIntA(const char *section, const char *key, int fallback, const char *file) {
    char text[64];
    return iniRead(file, section, key, text, sizeof text) ? (UINT)strtol(text, NULL, 10) : (UINT)fallback;
}

// Rewrites the file with `key` set to `value` in `[section]`; NULL deletes the key.
static PLATFORM_UNUSED BOOL WritePrivateProfileStringA(const char *section, const char *key, const char *value, const char *file) {
    char *old = NULL;
    size_t oldLen = 0;
    FILE *f = fopen(file, "r");
    if (f) {
        // A file whose size can't be read fails the write.
        long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
        if (size < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return FALSE; }
        oldLen = (size_t)size;
        old = malloc(oldLen + 1);
        if (!old) { fclose(f); return FALSE; }
        oldLen = fread(old, 1, oldLen, f);
        old[oldLen] = '\0';
        fclose(f);
    }
    char tmp[MAX_PATH + 8];
    snprintf(tmp, sizeof tmp, "%s.tmp", file);
    FILE *o = fopen(tmp, "w");
    if (!o) { free(old); return FALSE; }
    int inSection = 0, sectionSeen = 0, written = 0;
    for (char *line = old; line && *line; ) {
        char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line) : strlen(line);
        char *p = line;
        while (p < line + len && isspace((unsigned char)*p)) p++;
        if (*p == '[') {
            if (inSection && !written && value) { fprintf(o, "%s=%s\n", key, value); written = 1; }
            char *end = memchr(p, ']', len - (p - line));
            inSection = end && (size_t)(end - p - 1) == strlen(section) && !strncasecmp(p + 1, section, end - p - 1);
            sectionSeen |= inSection;
        } else if (inSection && !written) {
            char *eq = memchr(p, '=', len - (p - line));
            char *k = eq;
            while (k && k > p && isspace((unsigned char)k[-1])) k--;
            if (eq && (size_t)(k - p) == strlen(key) && !strncasecmp(p, key, k - p)) {
                if (value) fprintf(o, "%s=%s\n", key, value);
                written = 1;
                line = next ? next + 1 : NULL;
                continue;
            }
        }
        fprintf(o, "%.*s\n", (int)len, line);
        line = next ? next + 1 : NULL;
    }
    if (!written && value) {
        if (!sectionSeen) fprintf(o, "[%s]\n", section);
        fprintf(o, "%s=%s\n", key, value);
    }
    fclose(o);
    free(old);
    return rename(tmp, file) == 0;
}

// ---- Modules -----------------------------------------------------------------------------------
// Returns the Mach-O header of the game's executable.
static PLATFORM_UNUSED HMODULE mainExecutableHeader(void) {
    for (uint32_t i = 0; i < _dyld_image_count(); i++)
        if (_dyld_get_image_header(i)->filetype == MH_EXECUTE) return _dyld_get_image_header(i);
    return NULL;
}

// The game with NULL, else the loaded image named `name`, ignoring case.
static PLATFORM_UNUSED HMODULE GetModuleHandleA(const char *name) {
    if (!name) return mainExecutableHeader();
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char *path = _dyld_get_image_name(i), *base = strrchr(path, '/');
        if (!strcasecmp(base ? base + 1 : path, name) || !strcasecmp(path, name)) return _dyld_get_image_header(i);
    }
    return NULL;
}

static PLATFORM_UNUSED DWORD GetModuleFileNameA(HMODULE module, char *out, DWORD size) {
    if (!module) module = mainExecutableHeader();
    Dl_info info;
    if (!module || !dladdr(module, &info) || !info.dli_fname) { if (size) out[0] = '\0'; return 0; }
    snprintf(out, size, "%s", info.dli_fname);
    return (DWORD)strlen(out);
}

static PLATFORM_UNUSED void *GetProcAddress(HMODULE module, const char *symbol) {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(module, path, sizeof path)) return NULL;
    void *handle = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
    void *p = handle ? dlsym(handle, symbol) : NULL;
    if (handle) dlclose(handle);
    return p;
}

static PLATFORM_UNUSED BOOL GetModuleHandleExA(DWORD flags, LPCSTR address, HMODULE *out) {
    (void)flags;
    Dl_info info;
    *out = dladdr(address, &info) ? info.dli_fbase : NULL;
    return *out != NULL;
}

// Calls DllMain with DLL_PROCESS_ATTACH when the dylib loads, if it has one.
#ifndef PLATFORM_NO_DLLMAIN
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved);
__attribute__((constructor)) static void platformRunDllMain(void) {
    Dl_info info;
    if (!dladdr((void *)platformRunDllMain, &info)) return;
    void *self = dlopen(info.dli_fname, RTLD_LAZY | RTLD_NOLOAD);
    BOOL (*dllMain)(HINSTANCE, DWORD, LPVOID) = self ? (BOOL (*)(HINSTANCE, DWORD, LPVOID))dlsym(self, "DllMain") : NULL;
    if (dllMain) dllMain(info.dli_fbase, DLL_PROCESS_ATTACH, NULL);
    if (self) dlclose(self);
}
#endif
