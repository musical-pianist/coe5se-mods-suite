// coe5mod.log's rotation: coe5mod.log and coe5mod.1.log to coe5mod.4.log.
#pragma once
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef LOG_FILE_MAX
#define LOG_FILE_MAX (100L * 1024 * 1024)
#endif
#define LOG_FILES 5
#define LOG_STUB_OLD_FILE "coe5mod.old.log"
#define LOG_HEADER_END_EVENT "log_header_end"

// Writes the path of kept log `n` beside `path` to `out`.
static __attribute__((unused)) int logFilePath(const char *path, int n, char *out, size_t size) {
    size_t length = strlen(path);
    if (length < 4 || strcmp(path + length - 4, ".log")) return 0;
    int wrote = n == 0 ? snprintf(out, size, "%s", path)
              : n < 0  ? snprintf(out, size, "%.*s.old.log", (int)(length - 4), path)
                       : snprintf(out, size, "%.*s.%d.log", (int)(length - 4), path, n);
    return wrote > 0 && (size_t)wrote < size;
}

// Moves every kept log one older and `from` into coe5mod.1.log.
static __attribute__((unused)) int shiftLogFiles(const char *path, const char *from) {
    char older[1024], newer[1024];
    if (!logFilePath(path, LOG_FILES - 1, older, sizeof older)) return 0;
    remove(older);
    for (int n = LOG_FILES - 2; n >= 1; n--) {
        if (!logFilePath(path, n, newer, sizeof newer)) return 0;
        rename(newer, older);
        memcpy(older, newer, sizeof older);
    }
    remove(older);
    return rename(from, older) == 0;
}

// @platform-split log_truncate: Windows cuts with _chsize_s, a Mac with ftruncate
// Cuts the file at `path` to its first `bytes` bytes.
static __attribute__((unused)) int truncateLogFile(const char *path, long bytes) {
    FILE *f = fopen(path, "r+b");
    if (!f) return 0;
#ifdef _WIN32
    int ok = _chsize_s(_fileno(f), bytes) == 0;
#else
    int ok = ftruncate(fileno(f), bytes) == 0;
#endif
    return fclose(f) == 0 && ok;
}

// Returns the offset of this launch's first line in log text `text`.
static __attribute__((unused)) long launchStartIn(const char *text, long length, unsigned long pid) {
    char tail[32];
    snprintf(tail, sizeof tail, " pid=%lu", pid);
    size_t tailLength = strlen(tail);
    long found = -1;
    for (long start = 0; start < length;) {
        const char *end = memchr(text + start, '\n', (size_t)(length - start));
        long lineEnd = end ? (long)(end - text) : length;
        long trimmed = lineEnd > start && text[lineEnd - 1] == '\r' ? lineEnd - 1 : lineEnd;
        const char *line = text + start;
        size_t lineLength = (size_t)(trimmed - start);
        static const char EVENT[] = " event=loader_attached ";
        if (lineLength > tailLength && !memcmp(line + lineLength - tailLength, tail, tailLength)) {
            for (size_t i = 0; i + sizeof EVENT - 1 <= lineLength; i++)
                if (!memcmp(line + i, EVENT, sizeof EVENT - 1)) {
                    found = start;
                    break;
                }
        }
        start = lineEnd + 1;
    }
    return found;
}

// What rotateLogAtLaunch did, for the loader's log line.
typedef struct {
    int found;
    long earlier;
    int fromOld;
    int error;
} LogRotation;

// At a launch's start: moves the earlier launches' lines out of coe5mod.log.
static __attribute__((unused)) LogRotation rotateLogAtLaunch(const char *path, unsigned long pid) {
    LogRotation r = {0, 0, 0, 0};
    char old[1024];
    FILE *probe;
    if (logFilePath(path, -1, old, sizeof old) && (probe = fopen(old, "rb")) != NULL) {
        fclose(probe);
        r.fromOld = shiftLogFiles(path, old);
    }
    FILE *f = fopen(path, "rb");
    if (!f) return r;
    long size = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    char *text = size > 0 ? malloc((size_t)size) : NULL;
    if (!text || fseek(f, 0, SEEK_SET) || fread(text, 1, (size_t)size, f) != (size_t)size) {
        free(text);
        fclose(f);
        return r;
    }
    fclose(f);
    long start = launchStartIn(text, size, pid);
    r.found = start >= 0;
    if (start == 0) {
        free(text);
        return r;
    }
    if (start < 0) start = size;
    char first[1024];
    errno = 0;
    if (!logFilePath(path, 1, first, sizeof first)) {
        free(text);
        return r;
    }
    if (shiftLogFiles(path, path)) {
        r.earlier = start;
        if (start < size) {
            truncateLogFile(first, start);
            FILE *now = fopen(path, "wb");
            if (now) {
                fwrite(text + start, 1, (size_t)(size - start), now);
                fclose(now);
            }
        }
    } else {
        r.error = errno ? errno : -1;
        FILE *earlier = fopen(first, "wb");
        int copied = earlier && fwrite(text, 1, (size_t)start, earlier) == (size_t)start;
        if (earlier && fclose(earlier)) copied = 0;
        FILE *now = copied ? fopen(path, "wb") : NULL;
        if (now) {
            fwrite(text + start, 1, (size_t)(size - start), now);
            fclose(now);
            r.earlier = start;
        } else {
            remove(first);
        }
    }
    free(text);
    return r;
}
