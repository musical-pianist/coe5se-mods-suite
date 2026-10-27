// The trace filter: which of the game's trace lines are written (Windows: log.txt; Mac: standard output).
#pragma once

#define TRACE_CACHE_SLOTS 4096

typedef struct {
    int count;
    char **words;
    size_t *lengths;
} TraceKeepList;

// The list in use, NULL for every line.
static TraceKeepList *volatile traceKeepList;
static volatile unsigned traceKeepGeneration;
// Each slot: the format's address << 16 | generation (15 bits) << 1 | kept.
static volatile unsigned long long traceKeepCache[TRACE_CACHE_SLOTS];
static const unsigned char *traceImageStart, *traceImageEnd;

// 1 when `format` starts with one of the list's words.
static int traceFormatKept(const TraceKeepList *list, const char *format) {
    if (!list || !format) return 1;
    while (*format == ' ' || *format == '\t' || *format == '\n' || *format == '\r') format++;
    for (int i = 0; i < list->count; i++)
        if (!strncmp(format, list->words[i], list->lengths[i])) return 1;
    return 0;
}

// Why a word can't go in a list, or NULL.
static const char *traceKeepWordProblem(const char *word, size_t length) {
    if (!length) return "empty_word";
    if (word[0] == ' ' || word[0] == '\t' || word[0] == '\n' || word[0] == '\r') return "leading_space";
    if (memchr(word, '\0', length)) return "bad_character";
    return NULL;
}

// Makes `words` the list in use; 0 words keep every line.
static int setTraceKeepList(const char *const *words, const size_t *lengths, int count) {
    TraceKeepList *list = NULL;
    if (count > 0) {
        list = calloc(1, sizeof *list);
        if (!list || !(list->words = calloc((size_t)count, sizeof *list->words)) ||
            !(list->lengths = calloc((size_t)count, sizeof *list->lengths))) {
            if (list) free(list->words);
            free(list);
            return 0;
        }
        for (int i = 0; i < count; i++) {
            if (!(list->words[i] = malloc(lengths[i] + 1))) {
                for (int k = 0; k < i; k++) free(list->words[k]);
                free(list->words);
                free(list->lengths);
                free(list);
                return 0;
            }
            memcpy(list->words[i], words[i], lengths[i]);
            list->words[i][lengths[i]] = '\0';
            list->lengths[i] = lengths[i];
        }
        list->count = count;
    }
    traceKeepGeneration++;
    traceKeepList = list;
    return 1;
}

// Whether the trace line of `format` is written: every line with no list or in a network game.
int coe5seTraceLineKept(const char *format);
__attribute__((used)) int coe5seTraceLineKept(const char *format) {
    TraceKeepList *list = traceKeepList;
    if (!list || isNetworkGame()) return 1;
    unsigned long long address = (unsigned long long)(uintptr_t)format;
    const unsigned char *p = (const unsigned char *)format;
    if (p < traceImageStart || p >= traceImageEnd || address >> 48) return traceFormatKept(list, format);
    unsigned generation = traceKeepGeneration & 0x7fff;
    size_t slot = (size_t)((address >> 3) ^ (address >> 15)) & (TRACE_CACHE_SLOTS - 1);
    unsigned long long cached = traceKeepCache[slot];
    if (cached >> 16 == address && ((cached >> 1) & 0x7fff) == generation) return (int)(cached & 1);
    int kept = traceFormatKept(list, format);
    traceKeepCache[slot] = address << 16 | (unsigned long long)generation << 1 | (unsigned long long)kept;
    return kept;
}

static int traceFilterState;

#ifndef __APPLE__
// coe5seTraceHook: entered by jumps from the game's trace functions.
void coe5seTraceHook(void);
__asm__(".text\n"
        ".globl coe5seTraceHook\n"
        ".def coe5seTraceHook; .scl 2; .type 32; .endef\n"
        ".p2align 4\n"
        "coe5seTraceHook:\n"
        ".seh_proc coe5seTraceHook\n"
        "    movq %rcx, 8(%rsp)\n"
        "    movq %rdx, 16(%rsp)\n"
        "    movq %r8, 24(%rsp)\n"
        "    movq %r9, 32(%rsp)\n"
        "    subq $56, %rsp\n"
        "    .seh_stackalloc 56\n"
        "    .seh_endprologue\n"
        "    movq %rax, 32(%rsp)\n"
        "    callq coe5seTraceLineKept\n"
        "    movq 32(%rsp), %r10\n"
        "    movq 64(%rsp), %rcx\n"
        "    movq 72(%rsp), %rdx\n"
        "    movq 80(%rsp), %r8\n"
        "    movq 88(%rsp), %r9\n"
        "    addq $56, %rsp\n"
        "    testl %eax, %eax\n"
        "    jz 1f\n"
        "    jmpq *%r10\n"
        "1:  retq\n"
        ".seh_endproc\n");

// Points the four trace functions at coe5seTraceHook, once.
static int installTraceFilter(void) {
    if (traceFilterState) return traceFilterState > 0;
    traceFilterState = -1;
    static const struct { unsigned int rva; const char *head; } FUNCTIONS[] = {
        {RVA_TRACE, "function=trace "},
        {RVA_TRACE_C, "function=trace_c "},
        {RVA_TRACE_B, "function=trace_b "},
        {RVA_TRACE_ERROR, "function=trace_error "},
    };
    enum { COUNT = sizeof FUNCTIONS / sizeof FUNCTIONS[0] };
    if (!gameBuild) {
        writeLog("trace_filter_installed", "ok=0 of %d reason=unknown_game_build", COUNT);
        return 0;
    }
    int matched = 0;
    for (int i = 0; i < COUNT; i++)
        matched += checkGameFunction("trace_filter_check", FUNCTIONS[i].head);
    if (matched != COUNT) {
        writeLog("trace_filter_installed", "ok=0 of %d reason=game_code_changed", COUNT);
        return 0;
    }
    unsigned char *page = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!page) {
        writeLog("trace_filter_installed", "ok=0 of %d reason=no_near_page", COUNT);
        return 0;
    }
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(gameBase + ((const IMAGE_DOS_HEADER *)gameBase)->e_lfanew);
    traceImageStart = gameBase;
    traceImageEnd = gameBase + nt->OptionalHeader.SizeOfImage;
    void *hook = (void *)coe5seTraceHook;
    for (int i = 0; i < COUNT; i++) {
        unsigned char *fn = gameBase + FUNCTIONS[i].rva, *entry = page + i * TRACE_STUB_SIZE;
        buildTraceStubs(entry, entry, fn, fn, hook);
    }
    makePagesReadExecute(page, 0x1000);
    int ok = 0;
    for (int i = 0; i < COUNT; i++) {
        unsigned char *fn = gameBase + FUNCTIONS[i].rva, *entry = page + i * TRACE_STUB_SIZE;
        unsigned long long word;
        memcpy(&word, fn, 8);
        word = buildTraceJumpWord(word, fn, entry);
        ok += writeGameCode(fn, &word, 8);
    }
    traceFilterState = ok ? 1 : -1;
    writeLog("trace_filter_installed", "ok=%d of %d", ok, COUNT);
    return ok > 0;
}
#else
// Asks coe5seTraceLineKept about the format in x0, with every argument register kept; the answer in x16.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seTraceKeptCall\n_coe5seTraceKeptCall:\n"
    "  sub sp, sp, #0xe0\n"
    "  stp x29, x30, [sp, #0x00]\n"
    "  mov x29, sp\n"
    "  stp x0, x1, [sp, #0x10]\n  stp x2, x3, [sp, #0x20]\n  stp x4, x5, [sp, #0x30]\n  stp x6, x7, [sp, #0x40]\n"
    "  str x8, [sp, #0x50]\n"
    "  stp q0, q1, [sp, #0x60]\n  stp q2, q3, [sp, #0x80]\n  stp q4, q5, [sp, #0xa0]\n  stp q6, q7, [sp, #0xc0]\n"
    "  bl _coe5seTraceLineKept\n"
    "  mov w16, w0\n"
    "  ldp q0, q1, [sp, #0x60]\n  ldp q2, q3, [sp, #0x80]\n  ldp q4, q5, [sp, #0xa0]\n  ldp q6, q7, [sp, #0xc0]\n"
    "  ldr x8, [sp, #0x50]\n"
    "  ldp x6, x7, [sp, #0x40]\n  ldp x4, x5, [sp, #0x30]\n  ldp x2, x3, [sp, #0x20]\n  ldp x0, x1, [sp, #0x10]\n"
    "  ldp x29, x30, [sp, #0x00]\n"
    "  add sp, sp, #0xe0\n"
    "  ret\n");
extern const unsigned char coe5seTraceKeptCall[];

#define TRACE_STUB_WORDS 10

// Fills the Mac entry stub of the trace function at `fn`, placed at `stub`, whose first instruction is `moved`.
static void writeTraceStubMac(uint32_t code[TRACE_STUB_WORDS], const unsigned char *stub, const unsigned char *fn, uint32_t moved,
                              uint64_t routine) {
    code[0] = 0xf81f0ffeu;
    code[1] = 0x580000f0u;
    code[2] = 0xd63f0200u;
    code[3] = 0xf84107feu;
    code[4] = 0xb4000070u;
    code[5] = moved;
    code[6] = 0x14000000u | ((uint32_t)(((fn + 4) - (stub + 24)) >> 2) & 0x3ffffffu);
    code[7] = 0xd65f03c0u;
    memcpy(&code[8], &routine, 8);
}

// Points the four trace functions at their entry stubs, once.
static int installTraceFilter(void) {
    if (traceFilterState) return traceFilterState > 0;
    traceFilterState = -1;
    static const struct { unsigned int rva; const char *head; } FUNCTIONS[] = {
        {RVA_TRACE, "function=trace "},
        {RVA_TRACE_C, "function=trace_c "},
        {RVA_TRACE_B, "function=trace_b "},
        {RVA_TRACE_ERROR, "function=trace_error "},
    };
    enum { COUNT = sizeof FUNCTIONS / sizeof FUNCTIONS[0] };
    if (!gameBuild) {
        writeLog("trace_filter_installed", "ok=0 of %d reason=unknown_game_build", COUNT);
        return 0;
    }
    int matched = 0;
    for (int i = 0; i < COUNT; i++)
        matched += checkGameFunction("trace_filter_check", FUNCTIONS[i].head);
    if (matched != COUNT) {
        writeLog("trace_filter_installed", "ok=0 of %d reason=game_code_changed", COUNT);
        return 0;
    }
    unsigned long cstringSize = 0;
    const unsigned char *cstrings = getsectiondata((const struct mach_header_64 *)gameBase, "__TEXT", "__cstring", &cstringSize);
    traceImageStart = cstrings, traceImageEnd = cstrings ? cstrings + cstringSize : NULL;
    int ok = 0;
    for (int i = 0; i < COUNT; i++) {
        unsigned char *fn = gameBase + FUNCTIONS[i].rva, *stub = takePadding(sizeof(uint32_t) * TRACE_STUB_WORDS);
        if (!stub) {
            writeLog("hook_skipped", "site=trace_filter %sreason=no_padding", FUNCTIONS[i].head);
            continue;
        }
        uint32_t moved, code[TRACE_STUB_WORDS];
        memcpy(&moved, fn, 4);
        writeTraceStubMac(code, stub, fn, moved, (uint64_t)(uintptr_t)coe5seTraceKeptCall);
        uint32_t branch = 0x14000000u | ((uint32_t)((stub - fn) >> 2) & 0x3ffffffu);
        if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(fn, &branch, 4)) {
            writeLog("hook_skipped", "site=trace_filter %sreason=code_unwritable", FUNCTIONS[i].head);
            continue;
        }
        ok++;
    }
    traceFilterState = ok ? 1 : -1;
    writeLog("trace_filter_installed", "ok=%d of %d", ok, COUNT);
    return ok > 0;
}
#endif
