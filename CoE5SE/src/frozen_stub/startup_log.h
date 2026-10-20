// The log of the code that runs before CoE5SE: glu32.dll or coe5se.dylib (the stub) and the loader.
// Each line goes to coe5mod.log in the game folder as "<time> plugin=<LOG_TAG> event=<event>
// <fields>". The file that includes this header defines LOG_TAG first and sets logPath once it knows
// the game folder. Until then, and when the path doesn't fit, nothing is logged. With
// STARTUP_LOG_PID defined (the stubs), each line ends with the game process's " pid=<id>", so a
// log two launches wrote at once still says which wrote what. Plugins log through common.h instead.

#pragma once
#include <stdarg.h>
#include <stdio.h>

#ifndef LOG_TAG
#error "Define LOG_TAG before including startup_log.h"
#endif

#ifndef PRINTF_FORMAT
#if defined(__MINGW32__) && !defined(__clang__)
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(gnu_printf, fmtIndex, firstArg)))
#else
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#endif
#endif

// coe5mod.log in the game folder. Empty until the includer sets it.
static char logPath[MAX_PATH];

#ifdef STARTUP_LOG_ESCAPE
#include <stdlib.h>
#include "log_escape.h"
#endif

// Appends one line to coe5mod.log. With STARTUP_LOG_ESCAPE defined (the loader), a byte that would
// break the line is written escaped, as common.h's writeLog does (log_escape.h): the loader logs
// mods' names and file names, which a mod picks. The stubs leave it out: their bytes are frozen.
static PRINTF_FORMAT(2, 3) void writeLog(const char *event, const char *fmt, ...) {
    if (!logPath[0]) return;
    FILE *f = fopen(logPath, "a");
    if (!f) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(f, "%02d:%02d:%02d.%03d plugin=" LOG_TAG " event=%s ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, event);
    va_list ap;
    va_start(ap, fmt);
#ifdef STARTUP_LOG_ESCAPE
    va_list again;
    va_copy(again, ap);
    char stackText[1024], *text = stackText, *heap = NULL;
    int length = vsnprintf(stackText, sizeof stackText, fmt, ap);
    if (length >= (int)sizeof stackText && (heap = malloc((size_t)length + 1)) != NULL) {
        text = heap;
        vsnprintf(heap, (size_t)length + 1, fmt, again);
    } else if (length >= (int)sizeof stackText) {
        length = (int)sizeof stackText - 1;   // out of memory: the text's start
    }
    va_end(again);
    for (int i = 0; i < length; i++) {
        char escaped[4];
        size_t size = logByteSize((unsigned char)text[i]);
        escaped[0] = text[i];
        escapeLogText(escaped, 1, size - 1);
        fwrite(escaped, 1, size, f);
    }
    free(heap);
#else
    vfprintf(f, fmt, ap);
#endif
    va_end(ap);
#ifdef STARTUP_LOG_PID
    fprintf(f, " pid=%lu", (unsigned long)GetCurrentProcessId());
#endif
    fputc('\n', f);
    fclose(f);
}

#include "build_path.h"   // after writeLog, which buildPath logs through
