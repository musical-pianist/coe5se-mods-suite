// buildPath: builds a file path into a fixed buffer, refusing one that doesn't fit.
#pragma once
#include <stdarg.h>
#include <stdio.h>

#ifndef PRINTF_FORMAT
#if defined(__MINGW32__) && !defined(__clang__)
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(gnu_printf, fmtIndex, firstArg)))
#else
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#endif
#endif

// Writes the path that fmt and its arguments give into `out`, which holds `size` bytes.
static __attribute__((unused)) PRINTF_FORMAT(4, 5) int buildPath(char *out, size_t size, const char *call,
                                                                const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(out, size, fmt, ap);
    va_end(ap);
    if (needed >= 0 && (size_t)needed < size) return 1;
    if (size == 0) {
        writeLog("path_refused", "call=%s reason=too_long needed=%d size=0", call, needed);
        return 0;
    }
    // A printf that returns -1 on a cut may leave the buffer unterminated, so end it here.
    out[size - 1] = '\0';
    writeLog("path_refused", "call=%s reason=too_long needed=%d size=%d start=%s", call, needed, (int)size, out);
    out[0] = '\0';
    return 0;
}
