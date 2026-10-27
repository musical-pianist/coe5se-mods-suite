// The x86-64 code CoE5SE writes on Windows, built into a buffer.
#ifndef X86_CODE_H
#define X86_CODE_H

#define STUB_SIZE         0x40
#define GATE_SIZE         0x50
#define ENTRY_HOOK_SLOT   64
#define ENTRY_HOOK_STUB   40
#define TRACE_STUB_SIZE   0x40
#define TRACE_RESUME_AT   0x20
#define TRACE_MOVED_BYTES 5

// Writes `opcode` and a 32-bit displacement to `slot`, and moves *p past it.
static UNUSED void emitRipStore(unsigned char **p, const unsigned char *opcode, int opcodeLen, const void *slot, const unsigned char *runsAt) {
    memcpy(*p, opcode, opcodeLen);
    int disp = (int)((const unsigned char *)slot - (runsAt + opcodeLen + 4));
    memcpy(*p + opcodeLen, &disp, 4);
    *p += opcodeLen + 4;
}

// What a site's stub stores and reads before it jumps to the chain's head.
typedef struct {
    const void *xmm12, *xmm13;
    const void *ecxFrom;
    int zeroEcxEdx;
    const void *rsp, *head;
} SiteStubSlots;

// Builds a site's stub, to run at `stub`, in `code` (STUB_SIZE bytes); returns its length.
static UNUSED size_t buildSiteStub(unsigned char *code, const unsigned char *stub, const SiteStubSlots *s) {
    static const unsigned char MOVSS_XMM12[] = {0xf3, 0x44, 0x0f, 0x11, 0x25};
    static const unsigned char MOVSS_XMM13[] = {0xf3, 0x44, 0x0f, 0x11, 0x2d};
    static const unsigned char MOV_ECX[] = {0x8b, 0x0d};
    static const unsigned char ZERO_ECX_EDX[] = {0x31, 0xc9, 0x31, 0xd2};
    static const unsigned char MOV_RSP[] = {0x48, 0x89, 0x25};
    static const unsigned char JMP_INDIRECT[] = {0xff, 0x25};
    unsigned char *p = code;
#define STUB_RUNS_AT (stub + (p - code))
    if (s->xmm12) emitRipStore(&p, MOVSS_XMM12, 5, s->xmm12, STUB_RUNS_AT);
    if (s->xmm13) emitRipStore(&p, MOVSS_XMM13, 5, s->xmm13, STUB_RUNS_AT);
    if (s->ecxFrom) emitRipStore(&p, MOV_ECX, 2, s->ecxFrom, STUB_RUNS_AT);
    if (s->zeroEcxEdx) {
        memcpy(p, ZERO_ECX_EDX, sizeof ZERO_ECX_EDX);
        p += sizeof ZERO_ECX_EDX;
    }
    emitRipStore(&p, MOV_RSP, 3, s->rsp, STUB_RUNS_AT);
    emitRipStore(&p, JMP_INDIRECT, 2, s->head, STUB_RUNS_AT);
#undef STUB_RUNS_AT
    return (size_t)(p - code);
}

// Builds the call at `call` that goes to the site's stub at `stub`: call rel32, then a nop.
static UNUSED void buildSiteCall(unsigned char code[6], const unsigned char *call, const unsigned char *stub) {
    int rel = (int)(stub - (call + 5));
    code[0] = 0xe8;
    memcpy(code + 1, &rel, 4);
    code[5] = 0x90;
}

// Builds a gate's code, to run at `gate`, in `code` (GATE_SIZE bytes); returns its length.
static UNUSED size_t buildGateCode(unsigned char *code, const unsigned char *gate, const void *const flags[3],
                                   const void *entered, const void *hookSlot, const void *nextSlot) {
    static const unsigned char JCC[3] = {0x7f, 0x75, 0x75};
    static const unsigned char JMP_INDIRECT[] = {0xff, 0x25};
    unsigned char *p = code, *fixups[3];
    for (int i = 0; i < 3; i++) {
        uintptr_t flag = (uintptr_t)flags[i];
        *p++ = 0x48; *p++ = 0xb8; memcpy(p, &flag, 8); p += 8;
        *p++ = 0x83; *p++ = 0x38; *p++ = 0x00;
        *p++ = JCC[i]; fixups[i] = p++;
    }
    uintptr_t self = (uintptr_t)gate, enteredAt = (uintptr_t)entered;
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &self, 8); p += 8;
    *p++ = 0x48; *p++ = 0xa3; memcpy(p, &enteredAt, 8); p += 8;
    emitRipStore(&p, JMP_INDIRECT, 2, hookSlot, gate + (p - code));
    unsigned char *skip = p;
    emitRipStore(&p, JMP_INDIRECT, 2, nextSlot, gate + (p - code));
    for (int i = 0; i < 3; i++) *fixups[i] = (unsigned char)(skip - (fixups[i] + 1));
    return (size_t)(p - code);
}

// A rel32 at `at` in copied code, and the address it reaches.
typedef struct { int at; const unsigned char *target; } CodeFixup;

// Builds an entry hook's slot, to run at `t`, in `code` (ENTRY_HOOK_SLOT bytes); 0 when a fixup is out of reach.
static UNUSED int buildEntryHookSlot(unsigned char *code, const unsigned char *t, const unsigned char *fn, const unsigned char *saved,
                                     int size, const CodeFixup *fixups, int fixupCount, const void *detour) {
    static const unsigned char JMP_RIP[6] = {0xff, 0x25, 0, 0, 0, 0};
    memset(code, 0, ENTRY_HOOK_SLOT);
    memcpy(code, saved, (size_t)size);
    for (int k = 0; k < fixupCount; k++) {
        long long rel = fixups[k].target - (t + fixups[k].at + 4);
        int rel32 = (int)rel;
        if (rel != rel32) return 0;
        memcpy(code + fixups[k].at, &rel32, 4);
    }
    code[size] = 0xe9;
    int back = (int)((fn + size) - (t + size + 5));
    memcpy(code + size + 1, &back, 4);
    memcpy(code + ENTRY_HOOK_STUB, JMP_RIP, sizeof JMP_RIP);
    memcpy(code + ENTRY_HOOK_STUB + 6, &detour, 8);
    return 1;
}

// Builds the jump an entry hook writes over the function's first `size` bytes at `fn`: jmp rel32 to `stub`, then nops.
static UNUSED void buildEntryHookJump(unsigned char *code, const unsigned char *fn, const unsigned char *stub, int size) {
    code[0] = 0xe9;
    int rel = (int)(stub - (fn + 5));
    memcpy(code + 1, &rel, 4);
    memset(code + 5, 0x90, (size_t)size - 5);
}

// Builds a trace function's two stubs, to run at `entry`, in `code` (TRACE_STUB_SIZE bytes).
static UNUSED void buildTraceStubs(unsigned char *code, const unsigned char *entry, const unsigned char *fn,
                                   const unsigned char moved[TRACE_MOVED_BYTES], const void *hook) {
    memset(code, 0, TRACE_STUB_SIZE);
    unsigned char *resume = code + TRACE_RESUME_AT;
    memcpy(resume, moved, TRACE_MOVED_BYTES);
    resume[TRACE_MOVED_BYTES] = 0xe9;
    int back = (int)(fn + TRACE_MOVED_BYTES - (entry + TRACE_RESUME_AT + TRACE_MOVED_BYTES + 5));
    memcpy(resume + TRACE_MOVED_BYTES + 1, &back, 4);
    unsigned char *p = code;
    uintptr_t resumeAddress = (uintptr_t)(entry + TRACE_RESUME_AT);
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &resumeAddress, 8); p += 8;
    *p++ = 0xff; *p++ = 0x25; memset(p, 0, 4); p += 4;
    memcpy(p, &hook, 8);
}

// A trace function's first 8 bytes, `word`, with its first 5 made a jmp rel32 from `fn` to `entry`.
static UNUSED unsigned long long buildTraceJumpWord(unsigned long long word, const unsigned char *fn, const unsigned char *entry) {
    unsigned int rel = (unsigned int)(int)(entry - (fn + 5));
    return (word & 0xffffff0000000000ULL) | (unsigned long long)rel << 8 | 0xe9;
}

#endif
