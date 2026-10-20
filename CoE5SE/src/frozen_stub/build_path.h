// buildPath: builds a file path into a fixed buffer, and refuses a path that doesn't fit instead of
// cutting it. A cut path fails in quiet ways: a lookup misses with no log line, and a cut search
// pattern can match other files ("dir\*.c5m" cut to "dir\*" matches every file in dir).
//
// The file that includes this header must define writeLog(event, fmt, ...) first. common.h defines
// it for the plugins, and glu32_stub.c and coe5se_loader.c define their own.
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

// Writes the path that fmt and its arguments give into `out`, which holds `size` bytes, and returns
// 1. When the whole path doesn't fit, it returns 0, leaves `out` empty, and logs path_refused.
// `call` names the calling function; pass __func__.
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
