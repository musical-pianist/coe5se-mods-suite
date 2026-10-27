// The game's trace lines into log.txt through one open file, flushed after every line.
#pragma once

#ifndef __APPLE__
typedef void *(*TraceFopenFn)(const char *, const char *);
typedef int (*TraceFileFn)(void *);

static void *volatile traceLogFileOpen;

// The trace writers' fopen: log.txt opened once for the launch.
static void *traceLogOpen(const char *name, const char *mode) {
    void *file = traceLogFileOpen;
    if (file) return file;
    if (!(file = GAME_FN(TraceFopenFn, RVA_FOPEN)(name, mode))) return NULL;   // No log: the game's own trace skips the line too
    void *earlier = InterlockedCompareExchangePointer((void *volatile *)&traceLogFileOpen, file, NULL);
    if (!earlier) return file;
    GAME_FN(TraceFileFn, RVA_FCLOSE)(file);
    return earlier;
}

// The trace writers' fclose: the kept file is flushed, any other closed.
static int traceLogClose(void *file) {
    if (file && file == traceLogFileOpen) return GAME_FN(TraceFileFn, RVA_FFLUSH)(file);
    return GAME_FN(TraceFileFn, RVA_FCLOSE)(file);
}

#define TRACE_LOG_STUB_SIZE 0x20

// Points the four trace writers' fopen and fclose calls at the two above, and logs trace_log_file_installed.
static void traceLogFileInit(void) {
    static const struct { unsigned int siteRva, targetRva; void *hook; const char *name; } CALLS[] = {
        {RVA_TRACE_LOG_OPEN_CALL, RVA_FOPEN, (void *)traceLogOpen, "trace_log_open"},
        {RVA_TRACE_LOG_CLOSE_CALL, RVA_FCLOSE, (void *)traceLogClose, "trace_log_close"},
        {RVA_TRACE_C_LOG_OPEN_CALL, RVA_FOPEN, (void *)traceLogOpen, "trace_c_log_open"},
        {RVA_TRACE_C_LOG_CLOSE_CALL, RVA_FCLOSE, (void *)traceLogClose, "trace_c_log_close"},
        {RVA_TRACE_B_LOG_OPEN_CALL, RVA_FOPEN, (void *)traceLogOpen, "trace_b_log_open"},
        {RVA_TRACE_B_LOG_CLOSE_CALL, RVA_FCLOSE, (void *)traceLogClose, "trace_b_log_close"},
        {RVA_TRACE_ERROR_LOG_OPEN_CALL, RVA_FOPEN, (void *)traceLogOpen, "trace_error_log_open"},
        {RVA_TRACE_ERROR_LOG_CLOSE_JUMP, RVA_FCLOSE, (void *)traceLogClose, "trace_error_log_close"},
    };
    enum { COUNT = sizeof CALLS / sizeof CALLS[0] };
    if (!checkGameFunction("trace_log_file_check", "")) {
        writeLog("trace_log_file_installed", "ok=0 of %d reason=game_code_changed", COUNT);
        return;
    }
    for (int i = 0; i < COUNT; i++) {
        const unsigned char *site = gameBase + CALLS[i].siteRva;
        int rel;
        memcpy(&rel, site + 1, 4);
        if ((site[0] != 0xe8 && site[0] != 0xe9) || site + 5 + rel != gameBase + CALLS[i].targetRva) {
            writeLog("trace_log_file_installed", "ok=0 of %d reason=not_the_expected_call site=%s", COUNT, CALLS[i].name);
            return;
        }
    }
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!stubs) {
        writeLog("trace_log_file_installed", "ok=0 of %d reason=no_near_page", COUNT);
        return;
    }
    makePagesReadExecute(stubs, 0x1000);
    int ok = 0;
    for (int i = 0; i < COUNT; i += 2) {
        int closeTaken = redirectCall(CALLS[i + 1].siteRva, CALLS[i + 1].targetRva, stubs + TRACE_LOG_STUB_SIZE * (i + 1),
                                      CALLS[i + 1].hook, CALLS[i + 1].name, NULL);
        ok += closeTaken;
        if (closeTaken)
            ok += redirectCall(CALLS[i].siteRva, CALLS[i].targetRva, stubs + TRACE_LOG_STUB_SIZE * i, CALLS[i].hook,
                               CALLS[i].name, NULL);
    }
    writeLog("trace_log_file_installed", "ok=%d of %d", ok, COUNT);
}
#endif
