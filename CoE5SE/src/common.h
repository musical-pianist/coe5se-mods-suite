// Code every plugin shares: logging, game memory, and the game's screen functions.
#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_mac.h"
#include <errno.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "log_escape.h"
#include "log_files.h"

#define UNUSED __attribute__((unused))

// jazzy_api.h's PRINTF_FORMAT.
#ifndef PRINTF_FORMAT
#if defined(__MINGW32__) && !defined(__clang__)
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(gnu_printf, fmtIndex, firstArg)))
#else
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#endif
#endif

#ifdef COE5SE_RAW_ADDRESSES
#include "game_addresses.h"
// Read or write a byte, short, int, or float of game memory, or call a game function, by name.
#define GAME_PTR(rva) (gameBase + (rva))
#define G8(rva) (*(unsigned char *)(gameBase + (rva)))
#define G16(rva) (*(short *)(gameBase + (rva)))
#define G32(rva) (*(int *)(gameBase + (rva)))
#define GF(rva) (*(float *)(gameBase + (rva)))
#define GAME_FN(type, rva) ((type)(void *)(gameBase + (rva)))
#else
#include "address_ids.h"
#endif

#ifdef __APPLE__
#define PATH_SEP "/"
#define PLUGIN_SUFFIX ".c5plugin.dylib"
#else
#define PATH_SEP "\\"
#define PLUGIN_SUFFIX ".c5plugin.dll"
#endif

// The printf-style end of a game function's prototype.
#ifdef __APPLE__
#define FMT_ARGS const char *, ...
#else
#define FMT_ARGS const char *, const char *
#endif

// The key code the game binds to Cancel for Escape.
#ifndef __APPLE__
#define ESCAPE_KEY()         G16(RVA_ESCAPE_KEY)
#else
#define ESCAPE_KEY()         (G8(RVA_ESCAPE_ON) ? 0x1b : 0)
#endif

// wait_for_click(block, mode): block 1 waits for a click, 0 returns every frame.
typedef int (*WaitForClickFn)(int, int);
#define WAIT_UNTIL_CLICK 1
#define WAIT_MODE_MENU   10
typedef unsigned long long (*DrawButtonFn)(int id, float x, float y, float size,
                                             float r, float g, float b, float a, FMT_ARGS);
typedef unsigned long long (*DrawImageFn)(int flags, float x, float y, float w, float h,
                                            float r, float g, float b, float a, FMT_ARGS);

static unsigned char *gameBase;
static char logPath[MAX_PATH];
static const char *pluginName;

// coe5mod.log stays open from this DLL's first line on.
#ifdef _WIN32
static HANDLE logFile = INVALID_HANDLE_VALUE;
#define LOG_LINE_END "\r\n"
#else
static int logFile = -1;
#define LOG_LINE_END "\n"
#endif
static char logFileFor[MAX_PATH];

// Opens logPath for appending, or keeps the file open while logPath is the same.
static UNUSED int openLogFile(void) {
#ifdef _WIN32
    if (logFile != INVALID_HANDLE_VALUE) {
        if (!strcmp(logFileFor, logPath)) return 1;
        CloseHandle(logFile);
    }
    logFile = CreateFileA(logPath, FILE_APPEND_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (logFile == INVALID_HANDLE_VALUE) return 0;
#else
    if (logFile >= 0) {
        if (!strcmp(logFileFor, logPath)) return 1;
        close(logFile);
    }
    logFile = open(logPath, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0666);
    if (logFile < 0) return 0;
#endif
    memcpy(logFileFor, logPath, sizeof logFileFor);
    return 1;
}

// Writes `length` bytes at the end of the open log, in one write.
static UNUSED void appendLogText(const char *text, size_t length) {
#ifdef _WIN32
    DWORD written;
    WriteFile(logFile, text, (DWORD)length, &written, NULL);
#else
    while (length) {
        ssize_t n = write(logFile, text, length);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return;
        text += n;
        length -= (size_t)n;
    }
#endif
}

// ---- The log's rotation at LOG_FILE_MAX (log_files.h) --------------------------------------------
// @platform-split log_rollover: Windows asks GetFileSizeEx and GetFileInformationByHandle, a Mac fstat and stat
// Every writer checks the size of the file its handle has before each line.
static UNUSED void closeLogFile(void) {
#ifdef _WIN32
    if (logFile != INVALID_HANDLE_VALUE) CloseHandle(logFile);
    logFile = INVALID_HANDLE_VALUE;
#else
    if (logFile >= 0) close(logFile);
    logFile = -1;
#endif
    logFileFor[0] = '\0';
}
// The bytes of the file the open log handle has, or 0 when that can't be read.
static UNUSED long long logFileBytes(void) {
#ifdef _WIN32
    LARGE_INTEGER size;
    return GetFileSizeEx(logFile, &size) ? (long long)size.QuadPart : 0;
#else
    struct stat st;
    return fstat(logFile, &st) ? 0 : (long long)st.st_size;
#endif
}
// 1 when the open log handle's file is still the one at logPath.
static UNUSED int logFileIsPath(void) {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION open, named;
    HANDLE h = CreateFileA(logPath, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int same = !GetFileInformationByHandle(logFile, &open) || !GetFileInformationByHandle(h, &named) ||
               (open.dwVolumeSerialNumber == named.dwVolumeSerialNumber && open.nFileIndexHigh == named.nFileIndexHigh &&
                open.nFileIndexLow == named.nFileIndexLow);
    CloseHandle(h);
    return same;
#else
    struct stat open, named;
    if (stat(logPath, &named)) return 0;
    return fstat(logFile, &open) || (open.st_dev == named.st_dev && open.st_ino == named.st_ino);
#endif
}

static int logRolling;
static long long logRollFailedAt;
static UNUSED PRINTF_FORMAT(2, 3) void writeLog(const char *event, const char *fmt, ...);

// Copies the launch's header lines from log file `from` to the open log.
static UNUSED void copyLogHeader(const char *from) {
    FILE *f = fopen(from, "rb");
    if (!f) return;
    static char head[256 * 1024];
    size_t n = fread(head, 1, sizeof head - 1, f);
    fclose(f);
    head[n] = '\0';
    char *end = strstr(head, " event=" LOG_HEADER_END_EVENT);
    if (!end || !(end = strchr(end, '\n'))) return;
    end[1] = '\0';
    for (char *line = head; *line;) {
        char *next = strchr(line, '\n');
        next = next ? next + 1 : line + strlen(line);
        char kept = *next;
        *next = '\0';
        if (!strstr(line, " event=log_continued ")) appendLogText(line, (size_t)(next - line));
        *next = kept;
        line = next;
    }
}

// Moves to the next log file when the open one has reached LOG_FILE_MAX.
static UNUSED void rollLogOverIfFull(void) {
    long long bytes = logFileBytes();
    if (logRolling || bytes < LOG_FILE_MAX || (logRollFailedAt && bytes < logRollFailedAt + 1024 * 1024)) return;
    if (!logFileIsPath()) {
        closeLogFile();
        openLogFile();
        return;
    }
    char path[MAX_PATH], first[MAX_PATH];
    memcpy(path, logPath, sizeof path);
    closeLogFile();
    if (!shiftLogFiles(path, path) || !logFilePath(path, 1, first, sizeof first)) {
        logRollFailedAt = bytes;
        openLogFile();
        return;
    }
    logRollFailedAt = 0;
    if (!openLogFile()) return;
    logRolling = 1;
    writeLog("log_continued", "max_bytes=%lld files=%d before=coe5mod.1.log", (long long)LOG_FILE_MAX, LOG_FILES);
    copyLogHeader(first);
    logRolling = 0;
}

// Formats a line into out[size], cut to fit, and returns its whole length, or -1 on a format error.
static UNUSED PRINTF_FORMAT(4, 0) int formatLogLine(char *out, size_t size, const SYSTEMTIME *t, const char *event,
                                                    const char *fmt, va_list ap) {
    int head = snprintf(out, size, "%02d:%02d:%02d.%03d plugin=%s event=%s ", t->wHour, t->wMinute, t->wSecond,
                        t->wMilliseconds, pluginName, event);
    if (head < 0) return -1;
    size_t at = (size_t)head < size ? (size_t)head : size;
    int body = vsnprintf(at < size ? out + at : NULL, at < size ? size - at : 0, fmt, ap);
    return body < 0 ? -1 : head + body;
}

// Appends one line to coe5mod.log: the time, plugin=<name>, event=<event>, then the fields.
static UNUSED PRINTF_FORMAT(2, 3) void writeLog(const char *event, const char *fmt, ...) {
    if (!logPath[0] || !openLogFile()) return;
    rollLogOverIfFull();
    SYSTEMTIME t;
    GetLocalTime(&t);
    char stackLine[1024], *line = stackLine, *heap = NULL;
    size_t room = sizeof stackLine;
    va_list ap;
    va_start(ap, fmt);
    int formatted = formatLogLine(line, room, &t, event, fmt, ap);
    va_end(ap);
    if (formatted < 0) return;
    size_t length = (size_t)formatted;
    if (length >= room) {
        if ((heap = malloc(length + 1)) != NULL) {
            line = heap;
            room = length + 1;
            va_start(ap, fmt);
            formatLogLine(line, room, &t, event, fmt, ap);
            va_end(ap);
        } else {
            length = room - 1;
        }
    }
    size_t extra = logEscapeGrowth(line, length);
    size_t need = length + extra + sizeof LOG_LINE_END - 1;
    if (need > room) {
        char *grown = heap ? realloc(heap, need) : malloc(need);
        if (grown) {
            if (!heap) memcpy(grown, line, length);
            line = heap = grown;
            room = need;
        } else {
            while (length && length + extra + sizeof LOG_LINE_END - 1 > room)
                extra -= logByteSize((unsigned char)line[--length]) - 1;
        }
    }
    escapeLogText(line, length, extra);
    memcpy(line + length + extra, LOG_LINE_END, sizeof LOG_LINE_END - 1);
    appendLogText(line, length + extra + sizeof LOG_LINE_END - 1);
    free(heap);
}

// Sets the plugin's name for the log, gameBase, and the log path, then logs that the plugin attached.
static UNUSED void commonInit(const char *name) {
    pluginName = name;
    gameBase = (unsigned char *)GetModuleHandleA(NULL);
    GetModuleFileNameA(NULL, logPath, sizeof logPath);
    char *slash = strrchr(logPath, PATH_SEP[0]);
    char *file = slash ? slash + 1 : logPath;
    if ((size_t)(file - logPath) + sizeof "coe5mod.log" > sizeof logPath) logPath[0] = '\0';
    else memcpy(file, "coe5mod.log", sizeof "coe5mod.log");
    writeLog("plugin_attached", "pid=%lu base=%p", (unsigned long)GetCurrentProcessId(), gameBase);
}

#ifdef _WIN32
// Writes `size` bytes of code at `address`: the game's code, or a stub page made read-execute.
static UNUSED unsigned char *gameCodeOpenFrom, *gameCodeOpenTo;
static UNUSED DWORD gameCodeOpenProtection;
static UNUSED int writeGameCode(void *address, const void *bytes, size_t size) {
    DWORD old, ignored;
    if (gameCodeOpenFrom && (unsigned char *)address >= gameCodeOpenFrom && (unsigned char *)address + size <= gameCodeOpenTo) {
        memcpy(address, bytes, size);
        return 1;
    }
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old)) {
        writeLog("code_unwritable", "address=%p bytes=%zu error=%lu", address, size, GetLastError());
        return 0;
    }
    if (size == 8 && !((uintptr_t)address & 7)) {
        LONG64 word;
        memcpy(&word, bytes, 8);
        InterlockedExchange64((volatile LONG64 *)address, word);
    } else {
        memcpy(address, bytes, size);
    }
    if (!VirtualProtect(address, size, old, &ignored))
        writeLog("code_protection_not_restored", "address=%p bytes=%zu error=%lu", address, size, GetLastError());
    FlushInstructionCache(GetCurrentProcess(), address, size);
    return 1;
}

// Opens `size` bytes of the game's code for a run of writeGameCode calls, until closeGameCode.
static UNUSED int openGameCode(void *address, size_t size) {
    if (gameCodeOpenFrom || !VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &gameCodeOpenProtection)) {
        writeLog("code_unwritable", "address=%p bytes=%zu error=%lu", address, size, gameCodeOpenFrom ? 0UL : GetLastError());
        return 0;
    }
    gameCodeOpenFrom = address;
    gameCodeOpenTo = (unsigned char *)address + size;
    return 1;
}

// Puts back the protection openGameCode found, and clears the instruction cache for the range.
static UNUSED void closeGameCode(void) {
    if (!gameCodeOpenFrom) return;
    DWORD ignored;
    size_t size = (size_t)(gameCodeOpenTo - gameCodeOpenFrom);
    if (!VirtualProtect(gameCodeOpenFrom, size, gameCodeOpenProtection, &ignored))
        writeLog("code_protection_not_restored", "address=%p bytes=%zu error=%lu", (void *)gameCodeOpenFrom, size, GetLastError());
    FlushInstructionCache(GetCurrentProcess(), gameCodeOpenFrom, size);
    gameCodeOpenFrom = gameCodeOpenTo = NULL;
}

// Sets pages of code that were just written read-execute, and clears the instruction cache.
static UNUSED int makePagesReadExecute(void *address, size_t size) {
    DWORD old;
    FlushInstructionCache(GetCurrentProcess(), address, size);
    if (VirtualProtect(address, size, PAGE_EXECUTE_READ, &old)) return 1;
    writeLog("page_protection_kept", "address=%p bytes=%zu want=read_execute error=%lu", address, size, GetLastError());
    return 0;
}

// Sets pages of data that sit next to code read-write, not executable.
static UNUSED int makePagesReadWrite(void *address, size_t size) {
    DWORD old;
    if (VirtualProtect(address, size, PAGE_READWRITE, &old)) return 1;
    writeLog("page_protection_kept", "address=%p bytes=%zu want=read_write error=%lu", address, size, GetLastError());
    return 0;
}
#endif

#if !defined(__aarch64__) && defined(COE5SE_RAW_ADDRESSES)
// Allocates `bytes` of executable memory within 2 GB of `target`.
static UNUSED unsigned char *allocNearBytes(unsigned char *target, size_t bytes) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t gran = si.dwAllocationGranularity;
    uintptr_t start = ((uintptr_t)target & ~(gran - 1));
    for (uintptr_t dist = gran; dist < 0x7ff00000; dist += gran) {
        void *p = VirtualAlloc((void *)(start - dist), bytes, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (p) return p;
        p = VirtualAlloc((void *)(start + dist), bytes, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (p) return p;
    }
    return NULL;
}
// One page: allocNearBytes(target, 0x1000).
static UNUSED unsigned char *allocNear(unsigned char *target) { return allocNearBytes(target, 0x1000); }

// Points the instruction "call <targetRva>" at `siteRva` to `hook`, through a 0x20-byte stub at `stub`.
static UNUSED int redirectCall(unsigned int siteRva, unsigned int targetRva, unsigned char *stub, void *hook,
                                const char *label, uintptr_t *rspSlot) {
    unsigned char *site = gameBase + siteRva;
    int rel;
    memcpy(&rel, site + 1, 4);
    if (site[0] != 0xe8 || site + 5 + rel != gameBase + targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call address=%p", label, site);
        return 0;
    }
    unsigned char code[0x20], *p = code;
    if (rspSlot) {
        int disp = (int)((unsigned char *)rspSlot - (stub + 7));
        p[0] = 0x48; p[1] = 0x89; p[2] = 0x25; memcpy(p + 3, &disp, 4);
        p += 7;
    }
    p[0] = 0xff; p[1] = 0x25; memset(p + 2, 0, 4);
    memcpy(p + 6, &hook, 8);
    p += 14;
    rel = (int)(stub - (site + 5));
    if (!writeGameCode(stub, code, (size_t)(p - code)) || !writeGameCode(site + 1, &rel, 4)) {
        writeLog("hook_skipped", "site=%s reason=code_unwritable address=%p", label, site);
        return 0;
    }
    return 1;
}
#endif

// Runs the game's .c5m translation lookup on `text`.
#define TRANSLATED_SIZE      5000
static UNUSED const char *translateText(const char *text, char *out, int size) {
    GAME_FN(void (*)(const char *, char *, int), RVA_TRANSLATE_FORMAT)(text, out, size);
    return out;
}
#ifdef __APPLE__
// On Mac, the stores the game's callers make right after screen_begin.
#define LAYOUT_TOP_MARGIN 75.0
static UNUSED void screenBegin(void) {
    GAME_FN(void (*)(void), RVA_SCREEN_BEGIN)();
    G8(RVA_SCREEN_RESET_BYTE) = 0;
    G32(RVA_SCREEN_RESET_A) = -1;
    G32(RVA_SCREEN_RESET_B) = -1;
    G32(RVA_TEXT_FIELD_FOCUS) = -1;
    G32(RVA_TEXT_FIELD_CURSOR) = 0;
    G32(RVA_SCREEN_RESET_C) = 0;
    GF(RVA_LAYOUT_TOP_Y) = (float)((double)G32(RVA_SCREEN_H) - LAYOUT_TOP_MARGIN);
}
#else
static UNUSED void screenBegin(void) { GAME_FN(void (*)(void), RVA_SCREEN_BEGIN)(); }
#endif
static UNUSED void screenTitle(const char *t) {
    static char translated[TRANSLATED_SIZE];
    GAME_FN(void (*)(FMT_ARGS), RVA_SCREEN_TITLE)("%s", translateText(t, translated, sizeof translated));
}

#ifdef __APPLE__
// ---- Mac: helpers the game inlined ------------------------------------------------------------
// Mac copies of the game's layout helpers, writing what the inlined copies write.

// Takes a new element from the game's layout table, or NULL if the table is full.
static UNUSED unsigned char *layoutNew(void) {
    int i = GAME_FN(int (*)(void), RVA_LAYOUT_NEW)();
    return i < 0 ? NULL : GAME_PTR(RVA_LAYOUT_TABLE) + (size_t)i * LAYOUT_STRIDE;
}
#define LAYOUT_F(e, off) (*(float *)((e) + (off)))
#define LAYOUT_ID(e)     (*(int *)(e))
#define LAYOUT_TYPE(e)   (*(short *)((e) + LAYOUT_ELEMENT_TYPE))
#define LAYOUT_ENDS(e)   (*(unsigned char *)((e) + LAYOUT_ELEMENT_LINE_END))

// Ends the current line of the layout.
static UNUSED void closeLine(void) {
    if (GF(RVA_WIDEST_LINE) <= GF(RVA_LINE_WIDTH)) GF(RVA_WIDEST_LINE) = GF(RVA_LINE_WIDTH);
    GF(RVA_LINE_WIDTH) = 0.0f;
}
// The game's line-width update after an element.
static UNUSED void layoutAdvance(void) {
    int last = G32(RVA_LAYOUT_LAST);
    if (last < 0) return;
    unsigned char *e = GAME_PTR(RVA_LAYOUT_TABLE) + (size_t)last * LAYOUT_STRIDE;
    if (LAYOUT_ENDS(e)) closeLine();
    GF(RVA_LINE_WIDTH) += LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) + LAYOUT_F(e, LAYOUT_ELEMENT_COLUMN_X) * 0.1f;
    if (LAYOUT_ENDS(e)) closeLine();
}
// Mac copies of the game's screen_width, button_row, column_x, list_gap, row_area, and row_begin.
static UNUSED void screenWidth(float w) {
    float max = (float)G32(RVA_SCREEN_W);
    if (w > max) w = max;
    GF(RVA_SCREEN_WIDTH_A) = GF(RVA_SCREEN_WIDTH_B) = w;
    if (GF(RVA_WIDEST_LINE) <= w) GF(RVA_WIDEST_LINE) = w;
}
static UNUSED void buttonRowStyle(short type) {
    unsigned char *e = layoutNew();
    if (!e) return;
    LAYOUT_TYPE(e) = type;
    LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) = 0.0f;
    LAYOUT_ENDS(e) = 0;
}
static UNUSED void columnX(float x) {
    unsigned char *e = layoutNew();
    if (!e) return;
    LAYOUT_F(e, LAYOUT_ELEMENT_COLUMN_X) = GF(RVA_COLUMN_BASE);
    LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) = x;
    LAYOUT_ID(e) = 0;
    LAYOUT_TYPE(e) = LAYOUT_TYPE_COLUMN_X;
    LAYOUT_ENDS(e) = 0;
    GF(RVA_LINE_WIDTH) = x;
}
static UNUSED void listGap(float g) {
    unsigned char *e = layoutNew();
    if (!e) return;
    LAYOUT_F(e, LAYOUT_ELEMENT_COLUMN_X) = g;
    LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) = 1.0f;
    LAYOUT_TYPE(e) = LAYOUT_TYPE_LIST_GAP;
    layoutAdvance();
}
static UNUSED void rowArea(int id, float width) {
    unsigned char *e = layoutNew();
    if (!e) return;
    LAYOUT_F(e, LAYOUT_ELEMENT_COLUMN_X) = GF(RVA_COLUMN_BASE);
    LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) = width;
    LAYOUT_F(e, 0x14) = LAYOUT_F(e, 0x18) = LAYOUT_F(e, 0x1c) = LAYOUT_F(e, 0x20) = 0.0f;
    LAYOUT_ID(e) = id;
    LAYOUT_TYPE(e) = LAYOUT_TYPE_ROW_AREA;
    layoutAdvance();
}
static UNUSED void rowBegin(int on) {
    G32(RVA_ROW_OPEN) = on;
    int last = G32(RVA_LAYOUT_LAST);
    if (on || last < 0) return;
    while (last > 0 && LAYOUT_TYPE(GAME_PTR(RVA_LAYOUT_TABLE) + (size_t)last * LAYOUT_STRIDE) >= LAYOUT_TYPE_ROW_BEGIN_LIMIT) last--;
    LAYOUT_ENDS(GAME_PTR(RVA_LAYOUT_TABLE) + (size_t)last * LAYOUT_STRIDE) = 1;
    closeLine();
}
#else
static UNUSED void screenWidth(float w) { GAME_FN(void (*)(float), RVA_SCREEN_WIDTH)(w); }
#endif
static UNUSED void sectionBegin(void) { GAME_FN(int (*)(unsigned int), RVA_SECTION_BEGIN)(1); }
static UNUSED void sectionEnd(void) { GAME_FN(int (*)(void), RVA_SECTION_END)(); }
static UNUSED void sectionHeader(const char *t) { GAME_FN(void (*)(const char *), RVA_SECTION_HEADER)(t); }
static UNUSED void radioOption(int id, int selected, const char *text) {
    GAME_FN(void (*)(int, int, FMT_ARGS), RVA_RADIO_OPTION)(id, selected, "%s", text);
}
static UNUSED void textButton(int id, const char *text) {
    GAME_FN(void (*)(int, FMT_ARGS), RVA_TEXT_BUTTON)(id, "%s", text);
}
// Draws a checkbox (□/√).
static UNUSED void checkbox(int id, int *value, const char *label) {
    GAME_FN(int (*)(int, int *, FMT_ARGS), RVA_CHECKBOX)(id, value, "%s", label);
}
static UNUSED void tooltip(const char *text) {
    GAME_FN(unsigned long long (*)(FMT_ARGS), RVA_TOOLTIP)("%s", text);
}
static UNUSED void bindHotkey(int id, int key) { GAME_FN(void (*)(int, int), RVA_BIND_HOTKEY)(id, key); }
// The game's message box: `title`, `text`, and up to two buttons.
#define POPUP_TEXT_SIZE 4000
static UNUSED int messageBox(const char *title, const char *text, const char *button1, const char *button2, int cancel,
                             int key1, int key2) {
    return GAME_FN(int (*)(const char *, const char *, const char *, const char *, int, int, int), RVA_MESSAGE_BOX)(
        title, text, button1, button2, cancel, key1, key2);
}
#ifdef __APPLE__
static UNUSED void buttonRow(void) { buttonRowStyle(LAYOUT_TYPE_BUTTON_ROW); }
// Does nothing on Mac.
static UNUSED void screenMusic(void) {}

// Ends the frame the way vanilla setup screens do and returns the clicked id.
static UNUSED int screenWait(void) {
    GAME_FN(void (*)(int, float, float), RVA_LAYOUT_PASS)(-1, -1.0f, -1.0f);
    return GAME_FN(WaitForClickFn, RVA_WAIT_FOR_CLICK)(WAIT_UNTIL_CLICK, WAIT_MODE_MENU);
}

// The width Random Map Settings uses: its scale times its unit width.
#define SETUP_WIDTH_UNIT_MAC 300.0f
static UNUSED float setupScreenWidth(void) {
    return (float)(int)(GF(RVA_SETUP_WIDTH_SCALE) * SETUP_WIDTH_UNIT_MAC);
}

// Mac copies of the rest of the name box's calls.
static UNUSED void listStyle(short type) { buttonRowStyle(type); }
static UNUSED void rowOpen(void) { rowBegin(1); }
static UNUSED void rowClose(void) { rowBegin(0); }
// The layout marks the name box puts its picture between.
#define LAYOUT_MARK_OPEN  -998.0f
#define LAYOUT_MARK_CLOSE -999.0f
static UNUSED void layoutMark(float which) {
    unsigned char *e = layoutNew();
    if (!e) return;
    LAYOUT_F(e, LAYOUT_ELEMENT_WIDTH) = which;
    LAYOUT_TYPE(e) = LAYOUT_TYPE_MARK;
}
static UNUSED void layoutMarkOpen(void) { layoutMark(LAYOUT_MARK_OPEN); }
static UNUSED void layoutMarkClose(void) { layoutMark(LAYOUT_MARK_CLOSE); }
// A monster's picture at size 30.
#define DRAW_SPRITE_30_SIZE 30
static UNUSED void drawSprite30(int id, int kind, int sprite) {
    GAME_FN(void (*)(int, int, int, int), RVA_DRAW_SPRITE)(id, kind, sprite, DRAW_SPRITE_30_SIZE);
}
// screen_layout(-1, -1), which the Mac build inlined into a call of the layout pass.
static UNUSED void screenLayout(void) { GAME_FN(void (*)(int, float, float), RVA_LAYOUT_PASS)(-1, -1.0f, -1.0f); }
static UNUSED void textInputStart(void) {}
static UNUSED void textInputStop(void) {}
#else
static UNUSED void buttonRow(void) { GAME_FN(unsigned long long (*)(short), RVA_BUTTON_ROW)(LAYOUT_TYPE_BUTTON_ROW); }
static UNUSED void screenMusic(void) { GAME_FN(void (*)(void), RVA_SCREEN_MUSIC)(); }

// Ends the frame the way vanilla setup screens do and returns the clicked id.
static UNUSED int screenWait(void) {
    GAME_FN(void (*)(float, float), RVA_SCREEN_LAYOUT)(-1.0f, -1.0f);
    int id = GAME_FN(WaitForClickFn, RVA_WAIT_FOR_CLICK)(WAIT_UNTIL_CLICK, WAIT_MODE_MENU);
    GAME_FN(void (*)(int), RVA_CLICK_SOUND)(id);
    return id;
}

// The width Random Map Settings uses: its scale times its unit width.
static UNUSED float setupScreenWidth(void) {
    return (float)(int)(GF(RVA_SETUP_WIDTH_SCALE) * GF(RVA_SETUP_WIDTH_UNIT));
}

// The game's own layout helpers, under the names the Mac copies above have.
static UNUSED void listStyle(short type) { GAME_FN(unsigned long long (*)(short), RVA_LIST_STYLE)(type); }
static UNUSED void listGap(float g) { GAME_FN(void (*)(float), RVA_LIST_GAP)(g); }
static UNUSED void columnX(float x) { GAME_FN(void (*)(float), RVA_COLUMN_X)(x); }
static UNUSED void rowOpen(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(0); }
static UNUSED void rowClose(void) { GAME_FN(void (*)(int), RVA_ROW_BEGIN)(1); }
static UNUSED void layoutMarkOpen(void) { GAME_FN(void (*)(void), RVA_LAYOUT_MARK_OPEN)(); }
static UNUSED void layoutMarkClose(void) { GAME_FN(void (*)(void), RVA_LAYOUT_MARK_CLOSE)(); }
// A monster's picture at size 30, with the flags (4) the name box passes.
static UNUSED void drawSprite30(int id, int kind, int sprite) {
    GAME_FN(void (*)(int, int, int, int), RVA_DRAW_SPRITE_30)(id, kind, sprite, 4);
}
static UNUSED void screenLayout(void) { GAME_FN(void (*)(float, float), RVA_SCREEN_LAYOUT)(-1.0f, -1.0f); }
// The Steam Deck keyboard: text dialogs open it before their loop and close it after.
static UNUSED void textInputStart(void) { GAME_FN(void (*)(void), RVA_TEXT_INPUT_START)(); }
static UNUSED void textInputStop(void) { GAME_FN(void (*)(void), RVA_TEXT_INPUT_STOP)(); }
#endif

// The game's name box: title, monster picture, text field, Ok, and Cancel when allowed.
#define NAME_BOX_WIDTH       400.0f
#define NAME_BOX_TOP_GAP     5.0f
#define NAME_BOX_BUTTON_GAP  3.0f
#define NAME_BOX_FIELD_ARG   100
#define NAME_BOX_OK          1
#define NAME_BOX_CANCEL      2
static UNUSED int runNameBox(int spriteKind, int sprite, char *buf, int maxLen, int allowCancel, const char *title,
                             const char *prompt) {
    if (maxLen > NAME_BOX_LENGTH_MAX) maxLen = NAME_BOX_LENGTH_MAX;
    screenMusic();
    textInputStart();
    for (;;) {
        screenBegin();
        screenWidth(NAME_BOX_WIDTH);
        GAME_FN(void (*)(FMT_ARGS), RVA_SCREEN_TITLE)("%s", title);
        listGap(NAME_BOX_TOP_GAP);
        rowOpen();
        listStyle(LAYOUT_TYPE_LIST_STYLE);
        layoutMarkOpen();
        drawSprite30(0, spriteKind, sprite);
        layoutMarkClose();
        columnX(0.0f);
        GAME_FN(int (*)(char *, int, int, int, FMT_ARGS), RVA_TEXT_FIELD)(buf, maxLen, 1, NAME_BOX_FIELD_ARG, "%s", prompt);
        listStyle(LAYOUT_TYPE_BUTTON_ROW);
        rowClose();
        listGap(NAME_BOX_BUTTON_GAP);
        GAME_FN(void (*)(int, const char *), RVA_TEXT_BUTTON)(NAME_BOX_OK, "Ok");
        if (allowCancel) {
            GAME_FN(void (*)(int, const char *), RVA_TEXT_BUTTON)(NAME_BOX_CANCEL, "Cancel");
            if (ESCAPE_KEY()) bindHotkey(NAME_BOX_CANCEL, ESCAPE_KEY());
        }
        screenLayout();
        int id = GAME_FN(WaitForClickFn, RVA_WAIT_FOR_CLICK)(WAIT_UNTIL_CLICK, 0);
        if (id == NAME_BOX_OK || id == NAME_BOX_CANCEL) {
            if (id == NAME_BOX_CANCEL) buf[0] = '\0';
            textInputStop();
            return id == NAME_BOX_OK;
        }
    }
}
