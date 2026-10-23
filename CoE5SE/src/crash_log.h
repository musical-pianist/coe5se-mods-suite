// The crash line (Windows): one event=crash line in coe5mod.log when the game crashes.

#include <winternl.h>

#define CRASH_FRAMES 12

static char crashLine[2048];
static int crashLength;
static volatile LONG crashWriting;

static void crashText(const char *s) {
    while (*s && crashLength < (int)sizeof crashLine - 3) crashLine[crashLength++] = *s++;
}
static void crashHex(unsigned long long v, int digits) {
    char d[17];
    int n = 0;
    do {
        d[n++] = "0123456789abcdef"[v & 0xf];
        v >>= 4;
    } while ((v || n < digits) && n < 16);
    while (n && crashLength < (int)sizeof crashLine - 3) crashLine[crashLength++] = d[--n];
}
// Appends v (below 1000) as `digits` decimal digits, with leading zeros.
static void crashDecimal(int v, int digits) {
    char d[3];
    for (int i = digits - 1; i >= 0; i--, v /= 10) d[i] = (char)('0' + v % 10);
    for (int i = 0; i < digits && crashLength < (int)sizeof crashLine - 3; i++) crashLine[crashLength++] = d[i];
}

// Returns the base of the module holding `pc` and copies its file name into `name`, or returns 0.
static ULONG64 crashModule(ULONG64 pc, char *name, int size) {
    PEB *peb = NtCurrentTeb()->ProcessEnvironmentBlock;
    if (!peb || !peb->Ldr) return 0;
    LIST_ENTRY *head = &peb->Ldr->InMemoryOrderModuleList;
    int guard = 0;
    for (LIST_ENTRY *e = head->Flink; e && e != head && guard < 1000; e = e->Flink, guard++) {
        LDR_DATA_TABLE_ENTRY *m = CONTAINING_RECORD(e, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        ULONG64 base = (ULONG64)m->DllBase;
        if (!base || pc < base) continue;
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + (ULONG64)dos->e_lfanew);
        if (pc >= base + nt->OptionalHeader.SizeOfImage) continue;
        const WCHAR *path = m->FullDllName.Buffer;
        int length = m->FullDllName.Length / 2, start = 0, n = 0;
        for (int i = 0; path && i < length; i++) if (path[i] == L'\\' || path[i] == L'/') start = i + 1;
        for (int i = start; path && i < length && n < size - 1; i++) name[n++] = path[i] < 0x80 ? (char)path[i] : '?';
        name[n] = '\0';
        return base;
    }
    return 0;
}

// Walks up to `max` frames up from `c` and stores each frame's code address.
static int crashFrames(CONTEXT *c, ULONG64 *pcs, int max) {
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    ULONG64 low = c->Rsp, high = (ULONG64)tib->StackBase;
    int n = 0;
    char name[8];
    while (n < max) {
        ULONG64 pc = c->Rip;
        pcs[n++] = pc;
        if (c->Rsp < low || c->Rsp >= high) break;
        ULONG64 imageBase;
        PRUNTIME_FUNCTION f = RtlLookupFunctionEntry(pc, &imageBase, NULL);
        if (!f && !crashModule(pc, name, sizeof name)) break;
        if (f) {
            void *handlerData;
            ULONG64 establisher;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, pc, f, c, &handlerData, &establisher, NULL);
        } else {
            if (c->Rsp + 8 > high) break;
            c->Rip = *(ULONG64 *)c->Rsp;
            c->Rsp += 8;
        }
        if (!c->Rip || c->Rsp < low || c->Rsp >= high) break;
        low = c->Rsp;
    }
    return n;
}

// Appends "module+0xoffset", or the bare address outside every module.
static void crashWhere(ULONG64 pc) {
    char name[MAX_PATH];
    ULONG64 base = crashModule(pc, name, sizeof name);
    if (!base) {
        crashText("0x");
        crashHex(pc, 1);
        return;
    }
    crashText(name);
    crashText("+0x");
    crashHex(pc - base, 1);
}

// Appends the registers of `c`, as they were at the crash, after the other fields.
#define CRASH_REGISTER(name, field) {" " name "=0x", offsetof(CONTEXT, field)}
static const struct { const char *field; size_t offset; } CRASH_REGISTERS[] = {
    CRASH_REGISTER("rip", Rip), CRASH_REGISTER("rsp", Rsp), CRASH_REGISTER("rbp", Rbp), CRASH_REGISTER("rax", Rax),
    CRASH_REGISTER("rbx", Rbx), CRASH_REGISTER("rcx", Rcx), CRASH_REGISTER("rdx", Rdx), CRASH_REGISTER("rsi", Rsi),
    CRASH_REGISTER("rdi", Rdi), CRASH_REGISTER("r8", R8),   CRASH_REGISTER("r9", R9),   CRASH_REGISTER("r10", R10),
    CRASH_REGISTER("r11", R11), CRASH_REGISTER("r12", R12), CRASH_REGISTER("r13", R13), CRASH_REGISTER("r14", R14),
    CRASH_REGISTER("r15", R15),
};
static void crashRegisters(const CONTEXT *c) {
    for (size_t i = 0; i < sizeof CRASH_REGISTERS / sizeof CRASH_REGISTERS[0]; i++) {
        crashText(CRASH_REGISTERS[i].field);
        crashHex(*(const ULONG64 *)((const char *)c + CRASH_REGISTERS[i].offset), 1);
    }
    crashText(" eflags=0x");
    crashHex(c->EFlags, 1);
}

// Appends " stack=": the values above rsp that point into a loaded module, likely return addresses.
static void crashStack(const CONTEXT *c) {
    char name[MAX_PATH];
    const ULONG64 *slot = (const ULONG64 *)c->Rsp;
    int listed = 0;
    crashText(" stack=");
    for (int i = 0; i < 64 && listed < 8; i++) {
        if (!crashModule(slot[i], name, sizeof name)) continue;
        if (listed++) crashText(",");
        crashText("+0x");
        crashHex((ULONG64)i * 8, 1);
        crashText(":");
        crashWhere(slot[i]);
    }
}

// Builds and writes the crash line.
static void writeCrashLine(DWORD code, ULONG64 address, const EXCEPTION_RECORD *record, const CONTEXT *registers,
                           const ULONG64 *pcs, int frames, const char *caughtBy) {
    char name[MAX_PATH];
    SYSTEMTIME t;
    GetLocalTime(&t);
    crashLength = 0;
    crashDecimal(t.wHour, 2);
    crashText(":");
    crashDecimal(t.wMinute, 2);
    crashText(":");
    crashDecimal(t.wSecond, 2);
    crashText(".");
    crashDecimal(t.wMilliseconds, 3);
    crashText(" plugin=");
    crashText(pluginName);
    crashText(" event=");
    crashText("crash");
    crashText(" code=");
    crashHex(code, 8);
    crashText(" address=0x");
    crashHex(address, 1);
    ULONG64 base = crashModule(address, name, sizeof name);
    crashText(" module=");
    crashText(base ? name : "none");
    crashText(" offset=0x");
    crashHex(base ? address - base : 0, 1);
    if (record && code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
        ULONG_PTR kind = record->ExceptionInformation[0];
        crashText(kind == 0 ? " access=read" : kind == 1 ? " access=write" : kind == 8 ? " access=execute" : " access=other");
        crashText(" target=0x");
        crashHex(record->ExceptionInformation[1], 1);
    }
    static const char SUFFIX[] = ".c5plugin.dll";
    char plugin[MAX_PATH] = "none";
    for (int i = 0; i < frames; i++) {
        size_t length;
        if (!crashModule(pcs[i], name, sizeof name) || (length = strlen(name)) <= sizeof SUFFIX - 1 ||
            _stricmp(name + length - (sizeof SUFFIX - 1), SUFFIX))
            continue;
        memcpy(plugin, name, length + 1);
        break;
    }
    crashText(" plugin_dll=");
    crashText(plugin);
    crashText(" frames=");
    for (int i = 0; i < frames; i++) {
        if (i) crashText(",");
        crashWhere(pcs[i]);
    }
    crashText(" caught_by=");
    crashText(caughtBy);
    if (registers) crashRegisters(registers);
    if (registers && registers->Rsp) crashStack(registers);
    crashLine[crashLength++] = '\r';
    crashLine[crashLength++] = '\n';
    HANDLE h = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(h, crashLine, (DWORD)crashLength, &written, NULL);
    CloseHandle(h);
}

static CONTEXT crashWalkContext;
static ULONG64 crashPcs[CRASH_FRAMES];

// Writes the line for an exception, walking its stack from its context.
static void writeExceptionLine(const EXCEPTION_RECORD *record, const CONTEXT *context, const char *caughtBy) {
    crashWalkContext = *context;
    int frames = crashFrames(&crashWalkContext, crashPcs, CRASH_FRAMES);
    writeCrashLine(record->ExceptionCode, (ULONG64)record->ExceptionAddress, record, context, crashPcs, frames, caughtBy);
}

// ---- The unhandled-exception filter ----
static LPTOP_LEVEL_EXCEPTION_FILTER previousCrashFilter;

static LONG WINAPI logCrash(EXCEPTION_POINTERS *e) {
    if (!InterlockedExchange(&crashWriting, 1)) writeExceptionLine(e->ExceptionRecord, e->ContextRecord, "unhandled_filter");
    return previousCrashFilter ? previousCrashFilter(e) : EXCEPTION_CONTINUE_SEARCH;
}

// ---- The vectored handler: remembers the latest error-class exception ----
// An exception that is a crash: error severity and a code Windows defined.
static DWORD pendingThread;
static EXCEPTION_RECORD pendingRecord;
static CONTEXT pendingContext;

static LONG WINAPI rememberException(EXCEPTION_POINTERS *e) {
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if ((code & 0xe0000000) == 0xc0000000 && !crashWriting) {
        pendingThread = 0;
        pendingRecord = *e->ExceptionRecord;
        pendingContext = *e->ContextRecord;
        pendingThread = GetCurrentThreadId();
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// ---- The game's failed stack-cookie check ----
// Called when a stack cookie check fails.
static CONTEXT cookieContext;
static ULONG64 cookiePcs[CRASH_FRAMES + 1];

static void stackCookieFailed(ULONG64 cookie) {
    if (!InterlockedExchange(&crashWriting, 1)) {
        RtlCaptureContext(&cookieContext);
        int frames = crashFrames(&cookieContext, cookiePcs, CRASH_FRAMES + 1);
        int handling = 0;
        for (int i = 1; i < frames; i++) {
            ULONG64 imageBase;
            PRUNTIME_FUNCTION f = RtlLookupFunctionEntry(cookiePcs[i], &imageBase, NULL);
            if (f && imageBase == (ULONG64)gameBase && f->BeginAddress == RVA_GS_HANDLER_CHECK) handling = 1;
        }
        if (handling && pendingThread == GetCurrentThreadId())
            writeExceptionLine(&pendingRecord, &pendingContext, "stack_cookie_check");
        else if (frames > 1)
            writeCrashLine(STATUS_STACK_BUFFER_OVERRUN, cookiePcs[1], NULL, NULL, cookiePcs + 1, frames - 1, "stack_cookie_check");
    }
    GAME_FN(void (*)(ULONG64), RVA_REPORT_GSFAILURE)(cookie);
}

// Installs the filter.
static void installCrashFilter(void) { previousCrashFilter = SetUnhandledExceptionFilter(logCrash); }

// Installs the crash handlers.
static void installStackCookieHook(void) {
    unsigned char *site = gameBase + RVA_CHECK_COOKIE_FAIL_JMP;
    int rel;
    memcpy(&rel, site + 1, 4);
    unsigned char *stub = NULL;
    int ok = site[0] == 0xe9 && site + 5 + rel == gameBase + RVA_REPORT_GSFAILURE && (stub = allocNear(site)) != NULL;
    if (ok) {
        void *hook = (void *)stackCookieFailed;
        stub[0] = 0xff; stub[1] = 0x25; memset(stub + 2, 0, 4);
        memcpy(stub + 6, &hook, 8);
        rel = (int)(stub - (site + 5));
        ok = writeGameCode(site + 1, &rel, 4);
    }
    int vectored = ok && AddVectoredExceptionHandler(1, rememberException) != NULL;
    writeLog("crash_log_installed", "filter=1 stack_cookie_hook=%d vectored=%d", ok, vectored);
}
