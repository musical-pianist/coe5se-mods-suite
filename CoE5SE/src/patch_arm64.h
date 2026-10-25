// CoE5SE's call-site patching for the Mac build (coe5_mac, arm64).
#include <libkern/OSCacheControl.h>
#include <mach-o/getsect.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

#define MAX_STUBS 96
_Static_assert(SITE_COUNT <= MAX_STUBS, "one stub per site");

// One per site: the head the stub jumps to, and the sp, d8, and d9 the stub saw at the call.
typedef struct { void *head; uintptr_t sp; double d8, d9; } StubData;
__attribute__((used)) StubData coe5seStubData[MAX_STUBS];

// MAX_STUBS stubs of 9 instructions each.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seSiteStubs\n_coe5seSiteStubs:\n"
    ".set stubIndex, 0\n"
    ".rept 96\n"
    "  adrp x16, _coe5seStubData@PAGE\n"
    "  add x16, x16, _coe5seStubData@PAGEOFF\n"
    "  add x16, x16, #(stubIndex * 32)\n"
    "  mov x17, sp\n"
    "  str x17, [x16, #8]\n"
    "  str d8, [x16, #16]\n"
    "  str d9, [x16, #24]\n"
    "  ldr x16, [x16]\n"
    "  br x16\n"
    "  .set stubIndex, stubIndex + 1\n"
    ".endr\n");
extern const unsigned char coe5seSiteStubs[];
#define STUB_CODE_SIZE 36

static void **siteHead(CallSite site) { return &coe5seStubData[site].head; }

#define MAC_PAGE_MASK 0x3fff

// Writes code into the game's read-only __TEXT.
static int writeGameCode(void *address, const void *bytes, size_t size) {
    mach_vm_address_t page = (mach_vm_address_t)address & ~(mach_vm_address_t)MAC_PAGE_MASK;
    mach_vm_size_t length = ((mach_vm_address_t)address + size - page + MAC_PAGE_MASK) & ~(mach_vm_size_t)MAC_PAGE_MASK;
    if (mach_vm_protect(mach_task_self(), page, length, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY)) return 0;
    memcpy(address, bytes, size);
    if (mach_vm_protect(mach_task_self(), page, length, FALSE, VM_PROT_READ | VM_PROT_EXECUTE)) return 0;
    sys_icache_invalidate(address, size);
    return 1;
}

// The game's instruction word `live` with the bits of `mask` set to `value`: a new immediate.
static uint32_t patchedWord(uint32_t live, uint32_t mask, uint32_t value) { return (live & ~mask) | (value & mask); }

// Makes game data in __DATA_CONST writable.
static int makeGameDataWritable(void *address, size_t size) {
    mach_vm_address_t page = (mach_vm_address_t)address & ~(mach_vm_address_t)MAC_PAGE_MASK;
    mach_vm_size_t length = ((mach_vm_address_t)address + size - page + MAC_PAGE_MASK) & ~(mach_vm_size_t)MAC_PAGE_MASK;
    if (!mach_vm_protect(mach_task_self(), page, length, FALSE, VM_PROT_READ | VM_PROT_WRITE)) return 1;
    return !mach_vm_protect(mach_task_self(), page, length, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
}

// Returns where the `bl` at `at` goes, or NULL if the instruction isn't a `bl`.
static unsigned char *blTarget(const unsigned char *at) {
    uint32_t insn;
    memcpy(&insn, at, 4);
    if ((insn & 0xfc000000u) != 0x94000000u) return NULL;
    int32_t imm = (int32_t)(insn << 6) >> 6;
    return (unsigned char *)at + (intptr_t)imm * 4;
}

// Encodes a `bl` at `at` that calls `target`.
static uint32_t blTo(const unsigned char *at, const unsigned char *target) {
    return 0x94000000u | ((uint32_t)((target - at) >> 2) & 0x3ffffffu);
}

// The next free spot of the header padding, or NULL when there is no room.
static unsigned char *takePadding(size_t size) {
    static unsigned char *next, *end;
    if (!next) {
        const struct mach_header_64 *mh = (const struct mach_header_64 *)gameBase;
        next = gameBase + ((sizeof *mh + mh->sizeofcmds + 15) & ~(size_t)15);
        unsigned long textSize;
        unsigned char *text = getsectiondata(mh, "__TEXT", "__text", &textSize);
        end = text ? text : next;
    }
    size = (size + 15) & ~(size_t)15;
    if (next + size > end) return NULL;
    for (size_t i = 0; i < size; i++)
        if (next[i]) return NULL;
    unsigned char *spot = next;
    next += size;
    return spot;
}

// Writes a 16-byte trampoline to `target` into the header padding.
static unsigned char *newTrampoline(void *target) {
    unsigned char *t = takePadding(16);
    if (!t) return NULL;
    uint32_t code[4] = {0x58000050u, 0xd61f0200u};
    memcpy(&code[2], &target, 8);
    return writeGameCode(t, code, sizeof code) ? t : NULL;
}

// ---- Inlined button rows ------------------------------------------------------------------------
// @platform-split participants_button_row: Mac runs CoE5SE's copy of button_row at the inlined copies
// The sites whose calls are an inlined button_row's layout_new end their chain in button_row.
static uint64_t buttonRowMac(short style) {
    int index = ((int (*)(void))(gameBase + RVA_LAYOUT_NEW))();
    if (index < 0) return (uint64_t)(int64_t)index;
    unsigned char *element = gameBase + RVA_LAYOUT_ELEMENTS + (size_t)index * LAYOUT_STRIDE;
    *(short *)(element + LAYOUT_ELEMENT_TYPE) = style;
    *(int *)(element + LAYOUT_ELEMENT_WIDTH) = 0;
    element[LAYOUT_ELEMENT_LINE_END] = 0;
    return (uint64_t)(int64_t)G32(RVA_LAYOUT_LAST);
}

// @platform-split square_info_button_row: Mac runs CoE5SE's copy of button_row at the inlined copy
// The function at the inner end of the site's chain.
static int sealEventStandInMac(int slot, int seal);
static void initParticipantsMac(int mode);
static u64 splitStepGame(void);
static int installSplitSiteMac(CallSite site);
// The sites whose step Mac copied into its caller, whose Lua wraps run split at next() (lua_split_chain.h).
static int isSplitSite(CallSite site) {
    switch (site) {
    case SITE_AI_GATHER_TROOPS:
    case SITE_AI_SPLIT_ARMY:
    case SITE_AI_RECRUIT:
    case SITE_AI_EXPLORE:
    case SITE_AI_RALLY:
    case SITE_AI_FRIENDLY_MOVE:
    case SITE_AI_CONQUER: return 1;
    default: return 0;
    }
}
static void *siteGameFunction(CallSite site) {
    if (isSplitSite(site)) return (void *)splitStepGame;
    if (isInlinedButtonRowSite(site)) return (void *)buttonRowMac;
    if (site == SITE_SEAL_EVENT) return (void *)sealEventStandInMac;
    if (site == SITE_NEW_WORLD_INIT_PARTICIPANTS) return (void *)initParticipantsMac;
    return gameBase + SITES[site].targetRva;
}

// ---- Step hooks: a hook at each end of a game step Mac copied into its caller ----------------------
typedef struct {
    uint64_t x[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t nzcv;
    __uint128_t q[32];
} StepContext;
_Static_assert(sizeof(StepContext) == 0x310, "coe5seStepHookCall's frame");
typedef struct StepHook {
    uint64_t resume;
    void (*handler)(StepContext *ctx, const struct StepHook *hook);
    CallSite site;
} StepHook;

__attribute__((used)) void coe5seStepHook(StepContext *ctx, const StepHook *hook);
__attribute__((used)) void coe5seStepHook(StepContext *ctx, const StepHook *hook) { hook->handler(ctx, hook); }
__asm__(
    ".text\n.p2align 2\n.globl _coe5seStepHookCall\n_coe5seStepHookCall:\n"
    "  sub sp, sp, #0x310\n"
    "  stp x0, x1, [sp, #0x00]\n  stp x2, x3, [sp, #0x10]\n  stp x4, x5, [sp, #0x20]\n  stp x6, x7, [sp, #0x30]\n"
    "  stp x8, x9, [sp, #0x40]\n  stp x10, x11, [sp, #0x50]\n  stp x12, x13, [sp, #0x60]\n  stp x14, x15, [sp, #0x70]\n"
    "  add x2, sp, #0x310\n"
    "  ldp x0, x1, [x2]\n"
    "  stp x0, x1, [sp, #0x80]\n"
    "  stp x18, x19, [sp, #0x90]\n  stp x20, x21, [sp, #0xa0]\n  stp x22, x23, [sp, #0xb0]\n  stp x24, x25, [sp, #0xc0]\n"
    "  stp x26, x27, [sp, #0xd0]\n  stp x28, x29, [sp, #0xe0]\n"
    "  add x0, sp, #0x320\n"
    "  stp x30, x0, [sp, #0xf0]\n"
    "  ldr x0, [x16]\n"
    "  mrs x1, nzcv\n"
    "  stp x0, x1, [sp, #0x100]\n"
    "  stp q0, q1, [sp, #0x110]\n  stp q2, q3, [sp, #0x130]\n  stp q4, q5, [sp, #0x150]\n  stp q6, q7, [sp, #0x170]\n"
    "  stp q8, q9, [sp, #0x190]\n  stp q10, q11, [sp, #0x1b0]\n  stp q12, q13, [sp, #0x1d0]\n  stp q14, q15, [sp, #0x1f0]\n"
    "  stp q16, q17, [sp, #0x210]\n  stp q18, q19, [sp, #0x230]\n  stp q20, q21, [sp, #0x250]\n  stp q22, q23, [sp, #0x270]\n"
    "  stp q24, q25, [sp, #0x290]\n  stp q26, q27, [sp, #0x2b0]\n  stp q28, q29, [sp, #0x2d0]\n  stp q30, q31, [sp, #0x2f0]\n"
    "  mov x0, sp\n  mov x1, x16\n"
    "  bl _coe5seStepHook\n"
    "  ldp q0, q1, [sp, #0x110]\n  ldp q2, q3, [sp, #0x130]\n  ldp q4, q5, [sp, #0x150]\n  ldp q6, q7, [sp, #0x170]\n"
    "  ldp q8, q9, [sp, #0x190]\n  ldp q10, q11, [sp, #0x1b0]\n  ldp q12, q13, [sp, #0x1d0]\n  ldp q14, q15, [sp, #0x1f0]\n"
    "  ldp q16, q17, [sp, #0x210]\n  ldp q18, q19, [sp, #0x230]\n  ldp q20, q21, [sp, #0x250]\n  ldp q22, q23, [sp, #0x270]\n"
    "  ldp q24, q25, [sp, #0x290]\n  ldp q26, q27, [sp, #0x2b0]\n  ldp q28, q29, [sp, #0x2d0]\n  ldp q30, q31, [sp, #0x2f0]\n"
    "  ldp x0, x1, [sp, #0x100]\n"
    "  msr nzcv, x1\n"
    "  ldr x1, [sp, #0x80]\n"
    "  add x2, sp, #0x310\n"
    "  stp x0, x1, [x2]\n"
    "  ldr x17, [sp, #0x88]\n"
    "  ldp x18, x19, [sp, #0x90]\n  ldp x20, x21, [sp, #0xa0]\n  ldp x22, x23, [sp, #0xb0]\n  ldp x24, x25, [sp, #0xc0]\n"
    "  ldp x26, x27, [sp, #0xd0]\n  ldp x28, x29, [sp, #0xe0]\n  ldr x30, [sp, #0xf0]\n"
    "  ldp x14, x15, [sp, #0x70]\n  ldp x12, x13, [sp, #0x60]\n  ldp x10, x11, [sp, #0x50]\n  ldp x8, x9, [sp, #0x40]\n"
    "  ldp x6, x7, [sp, #0x30]\n  ldp x4, x5, [sp, #0x20]\n  ldp x2, x3, [sp, #0x10]\n  ldp x0, x1, [sp, #0x00]\n"
    "  add sp, sp, #0x320\n"
    "  ldur x16, [sp, #-16]\n"
    "  br x16\n");
extern const unsigned char coe5seStepHookCall[];

#define STEP_STUB_WORDS 8
#define STEP_TRAMPOLINE_WORDS 6
#define STEP_LOAD_X16 0xf85f83f0u
#define STEP_NOP 0xd503201fu

// Fills the entry stub, placed at `stub`, for `hook`.
static void writeStepStubMac(uint32_t code[STEP_STUB_WORDS], uint64_t hook, uint64_t routine) {
    code[0] = 0xa9bf47f0u;
    code[1] = 0x58000070u;
    code[2] = 0x58000091u;
    code[3] = 0xd61f0220u;
    memcpy(&code[4], &hook, 8);
    memcpy(&code[6], &routine, 8);
}
// Whether `insn` is an adrp, which a trampoline turns into a load of the page it computes.
static int isAdrp(uint32_t insn) { return (insn & 0x9f000000u) == 0x90000000u; }
// The page an adrp at `at` computes.
static uint64_t adrpPage(uint32_t insn, const unsigned char *at) {
    int64_t imm = (int64_t)((((insn >> 5) & 0x7ffffu) << 2) | ((insn >> 29) & 3u));
    if (imm & (1 << 20)) imm -= 1 << 21;
    return ((uint64_t)(uintptr_t)at & ~(uint64_t)0xfff) + (uint64_t)(imm << 12);
}
// Fills a hook's trampoline, placed at `tramp`, for the instruction `moved` at `at`.
static void writeStepTrampolineMac(uint32_t code[STEP_TRAMPOLINE_WORDS], const unsigned char *tramp, const unsigned char *at, uint32_t moved) {
    uint64_t page = 0;
    code[0] = STEP_LOAD_X16;
    code[1] = moved;
    if (isAdrp(moved)) {
        page = adrpPage(moved, at);
        code[1] = 0x58000060u | (moved & 0x1fu);
    }
    code[2] = 0x14000000u | ((uint32_t)(((at + 4) - (tramp + 8)) >> 2) & 0x3ffffffu);
    code[3] = STEP_NOP;
    memcpy(&code[4], &page, 8);
}
// Whether `insn` may run from a trampoline: not PC-relative (adr, a branch, a literal load), but an adrp.
static int stepInstructionMovable(uint32_t insn) {
    if (isAdrp(insn)) return 1;
    if ((insn & 0x9f000000u) == 0x10000000u) return 0;
    if ((insn & 0x7c000000u) == 0x14000000u) return 0;
    if ((insn & 0x7e000000u) == 0x34000000u) return 0;
    if ((insn & 0x7e000000u) == 0x36000000u) return 0;
    if ((insn & 0xff000010u) == 0x54000000u) return 0;
    if ((insn & 0x3b000000u) == 0x18000000u) return 0;
    return 1;
}

// Puts a step hook at `rva` with `hook`'s handler: writes its trampoline and stub, then the `b`.
static int putStepHookMac(unsigned int rva, StepHook *hook, const char *name, int replace) {
    unsigned char *at = gameBase + rva;
    uint32_t moved;
    memcpy(&moved, at, 4);
    if (replace) moved = STEP_NOP;
    if (!stepInstructionMovable(moved)) {
        writeLog("site_skipped", "site=%s reason=not_the_expected_code address=%p", name, (void *)at);
        return 0;
    }
    unsigned char *stub = takePadding(sizeof(uint32_t) * STEP_STUB_WORDS);
    unsigned char *tramp = stub ? takePadding(sizeof(uint32_t) * STEP_TRAMPOLINE_WORDS) : NULL;
    if (!tramp) {
        writeLog("site_skipped", "site=%s reason=no_padding", name);
        return 0;
    }
    uint32_t stubCode[STEP_STUB_WORDS], trampCode[STEP_TRAMPOLINE_WORDS];
    writeStepStubMac(stubCode, (uint64_t)(uintptr_t)hook, (uint64_t)(uintptr_t)coe5seStepHookCall);
    writeStepTrampolineMac(trampCode, tramp, at, moved);
    hook->resume = (uint64_t)(uintptr_t)tramp;
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(tramp, trampCode, sizeof trampCode) || !writeGameCode(stub, stubCode, sizeof stubCode) ||
        !writeGameCode(at, &branch, 4)) {
        writeLog("site_failed", "site=%s reason=write address=%p", name, (void *)at);
        return 0;
    }
    return 1;
}

// ---- ceremony_costs on Mac: the Mastery costs as ints, and the Ceremonies screen's fill -------------
// @platform-split ceremony_costs_mac: Mac's site also patches the Mastery costs and the Ceremonies screen
static const unsigned int MASTERY_FILL_RVAS[MASTERY_COST_FILLS] = {RVA_MASTERY_COST_FILL_1, RVA_MASTERY_COST_FILL_2,
                                                                    RVA_MASTERY_COST_FILL_3, RVA_MASTERY_COST_FILL_4,
                                                                    RVA_MASTERY_COST_FILL_5, RVA_MASTERY_COST_FILL_6};
static const unsigned int MASTERY_READ_RVAS[MASTERY_COST_READS] = {
    RVA_MASTERY_COST_READ_1, RVA_MASTERY_COST_READ_2, RVA_MASTERY_COST_READ_3, RVA_MASTERY_COST_READ_4,
    RVA_MASTERY_COST_READ_5, RVA_MASTERY_COST_READ_6, RVA_MASTERY_COST_READ_7, RVA_MASTERY_COST_READ_8,
    RVA_MASTERY_COST_READ_9, RVA_MASTERY_COST_READ_10, RVA_MASTERY_COST_READ_11};

// The gold of the Mastery cost whose slot is at byte `offset` of its page (0x4d8, 0x4dc, 0x4e0), or -1.
static int masteryGoldAt(unsigned offset) {
    unsigned page = RVA_CEREMONY_COST_ARCH_BISHOP_SET & 0xfff;
    return offset == page ? CEREMONY_COST_ARCH_BISHOP_GOLD
           : offset == (RVA_CEREMONY_COST_CARDINAL_SET & 0xfff) ? CEREMONY_COST_CARDINAL_GOLD
           : offset == (RVA_CEREMONY_COST_PONTIFF_SET & 0xfff) ? CEREMONY_COST_PONTIFF_GOLD
                                                                : -1;
}
static uint32_t movzW16(int value) { return 0x52800010u | ((uint32_t)value << 5); }
static uint32_t strW16(unsigned base, unsigned offset) { return 0xb9000010u | ((offset / 4) << 10) | (base << 5); }

// A fill's strb of a flag, rewritten to store the gold; returns 0 for code that isn't that.
static int rewriteMasteryFill(uint32_t code[2], int strbFirst) {
    uint32_t strb = code[strbFirst ? 0 : 1], other = code[strbFirst ? 1 : 0];
    if ((strb & 0xffc00000u) != 0x39000000u || other != 0xd503201fu) return 0;
    unsigned offset = (strb >> 10) & 0xfff, base = (strb >> 5) & 31;
    int gold = masteryGoldAt(offset);
    if (gold < 0 || offset % 4) return 0;
    code[0] = movzW16(gold);
    code[1] = strW16(base, offset);
    return 1;
}

// A reader's four instructions, rewritten to load the cost as an int; returns 0 for code that isn't that.
static int rewriteMasteryRead(uint32_t code[4]) {
    uint32_t ldrb = code[0], mov = 0, cmp = 0, csel = code[3];
    int negative = 0;
    for (int i = 1; i <= 2; i++) {
        if ((code[i] & 0xff800000u) == 0x52800000u) mov = code[i];
        else if ((code[i] & 0xff800000u) == 0x12800000u) mov = code[i], negative = 1;
        else cmp = code[i];
    }
    if ((ldrb & 0xffc00000u) != 0x39400000u || !mov || (mov >> 21 & 3) || cmp != (0x7100001fu | ((ldrb & 31) << 5)) ||
        (csel & 0xfffffc00u) != 0x1a9f1000u)
        return 0;
    unsigned offset = (ldrb >> 10) & 0xfff, base = (ldrb >> 5) & 31, dest = csel & 31, temp = mov & 31;
    int gold = masteryGoldAt(offset), value = (int)((mov >> 5) & 0xffff);
    if (gold < 0 || offset % 4 || dest == temp || ((csel >> 5) & 31) != temp || (negative ? value + 1 : value) != gold) return 0;
    code[0] = 0xb9400000u | ((offset / 4) << 10) | (base << 5) | dest;
    code[1] = mov;
    if (negative) {
        code[2] = 0x4b0003e0u | (dest << 16) | dest;
        code[3] = 0x7100001fu | (dest << 5);
    } else {
        code[2] = 0x7100001fu | (dest << 5);
        code[3] = 0xd503201fu;
    }
    return 1;
}

// The Ceremonies screen's call of RVA_SCREEN_BEGIN: runs the site's chain for its commander first.
__attribute__((used)) static uint64_t ceremonyScreenCostsMac(int commander) {
    ((void (*)(int))(coe5seSiteStubs + SITE_CEREMONY_COSTS * STUB_CODE_SIZE))(commander);
    return ((uint64_t (*)(void))(gameBase + RVA_SCREEN_BEGIN))();
}

// Puts the Mastery cost patches and the screen's call in, before the site's own calls.
static int installCeremonyCostsMac(void) {
    if (!gameCodeMatches) {
        writeLog("site_skipped", "site=ceremony_costs reason=game_code_differs");
        return 0;
    }
    uint32_t fills[MASTERY_COST_FILLS][2], reads[MASTERY_COST_READS][4];
    int ok = 1;
    for (int i = 0; i < MASTERY_COST_FILLS; i++) {
        unsigned char *at = gameBase + MASTERY_FILL_RVAS[i];
        int strbFirst = i < 3;
        memcpy(fills[i], strbFirst ? at : at - 4, 8);
        if (i == 2) {
            memcpy(fills[i], at, 4);
            fills[i][1] = 0xd503201fu;
        }
        ok &= rewriteMasteryFill(fills[i], strbFirst);
    }
    for (int i = 0; i < MASTERY_COST_READS; i++) {
        memcpy(reads[i], gameBase + MASTERY_READ_RVAS[i], 16);
        ok &= rewriteMasteryRead(reads[i]);
    }
    unsigned char *screenCall = gameBase + RVA_CEREMONY_SCREEN_COSTS;
    ok &= blTarget(screenCall) == gameBase + RVA_SCREEN_BEGIN;
    unsigned char *fillStub = ok ? takePadding(16) : NULL, *screenStub = fillStub ? takePadding(32) : NULL;
    if (!ok || !screenStub) {
        writeLog("site_skipped", "site=ceremony_costs reason=%s", ok ? "no_padding" : "not_the_expected_code");
        return 0;
    }
    unsigned char *third = gameBase + MASTERY_FILL_RVAS[2];
    uint32_t stub[4] = {fills[2][0], fills[2][1], 0x14000000u | ((uint32_t)(((third + 4) - (fillStub + 8)) >> 2) & 0x3ffffffu), 0xd503201fu};
    uint32_t screen[6] = {0xaa1c03e0u, 0x58000070u, 0xd61f0200u, 0xd503201fu};
    uint64_t routine = (uint64_t)(uintptr_t)ceremonyScreenCostsMac;
    memcpy(&screen[4], &routine, 8);
    if (!writeGameCode(fillStub, stub, sizeof stub) || !writeGameCode(screenStub, screen, sizeof screen)) {
        writeLog("site_skipped", "site=ceremony_costs reason=code_unwritable");
        return 0;
    }
    G32(RVA_CEREMONY_COST_ARCH_BISHOP_SET) = CEREMONY_COST_ARCH_BISHOP_GOLD;
    G32(RVA_CEREMONY_COST_CARDINAL_SET) = CEREMONY_COST_CARDINAL_GOLD;
    G32(RVA_CEREMONY_COST_PONTIFF_SET) = CEREMONY_COST_PONTIFF_GOLD;
    int written = 1;
    for (int i = 0; i < MASTERY_COST_READS; i++) written &= writeGameCode(gameBase + MASTERY_READ_RVAS[i], reads[i], 16);
    for (int i = 0; i < MASTERY_COST_FILLS; i++) {
        unsigned char *at = gameBase + MASTERY_FILL_RVAS[i];
        if (i == 2) {
            uint32_t branch = 0x14000000u | ((uint32_t)((fillStub - at) >> 2) & 0x3ffffffu);
            written &= writeGameCode(at, &branch, 4);
        } else {
            written &= writeGameCode(i < 3 ? at : at - 4, fills[i], 8);
        }
    }
    uint32_t call = blTo(screenCall, screenStub);
    written &= writeGameCode(screenCall, &call, 4);
    if (!written) writeLog("site_failed", "site=ceremony_costs reason=write");
    return written;
}

// ---- SEAL_EVENT on Mac: a call where the month-end step picks a seal's event -----------------------
// @platform-split seal_event_mac: Mac's step jumps through a table, so its stub sits before the table
static int sealEventStandInMac(int slot, int seal) {
    (void)slot, (void)seal;
    return 1;
}
__attribute__((used)) int coe5seSealEventAsk(int seal);
__attribute__((used)) int coe5seSealEventAsk(int seal) {
    return ((int (*)(int, int))(coe5seSiteStubs + SITE_SEAL_EVENT * STUB_CODE_SIZE))(-1, seal);
}

// Calls coe5seSealEventAsk(w9) with every register the call could change kept.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seSealEventCall\n_coe5seSealEventCall:\n"
    "  sub sp, sp, #0x2c0\n"
    "  stp x29, x30, [sp, #0x00]\n"
    "  mov x29, sp\n"
    "  stp x0, x1, [sp, #0x10]\n  stp x2, x3, [sp, #0x20]\n  stp x4, x5, [sp, #0x30]\n  stp x6, x7, [sp, #0x40]\n"
    "  stp x8, x9, [sp, #0x50]\n  stp x10, x11, [sp, #0x60]\n  stp x12, x13, [sp, #0x70]\n  stp x14, x15, [sp, #0x80]\n"
    "  stp x17, x18, [sp, #0x90]\n"
    "  mrs x9, nzcv\n  str x9, [sp, #0xa0]\n"
    "  stp q0, q1, [sp, #0xb0]\n  stp q2, q3, [sp, #0xd0]\n  stp q4, q5, [sp, #0xf0]\n  stp q6, q7, [sp, #0x110]\n"
    "  stp q8, q9, [sp, #0x130]\n  stp q10, q11, [sp, #0x150]\n  stp q12, q13, [sp, #0x170]\n  stp q14, q15, [sp, #0x190]\n"
    "  stp q16, q17, [sp, #0x1b0]\n  stp q18, q19, [sp, #0x1d0]\n  stp q20, q21, [sp, #0x1f0]\n  stp q22, q23, [sp, #0x210]\n"
    "  stp q24, q25, [sp, #0x230]\n  stp q26, q27, [sp, #0x250]\n  stp q28, q29, [sp, #0x270]\n  stp q30, q31, [sp, #0x290]\n"
    "  ldr w0, [sp, #0x58]\n"
    "  bl _coe5seSealEventAsk\n"
    "  sxtw x0, w0\n"
    "  str x0, [sp, #0x2d0]\n"
    "  ldp q0, q1, [sp, #0xb0]\n  ldp q2, q3, [sp, #0xd0]\n  ldp q4, q5, [sp, #0xf0]\n  ldp q6, q7, [sp, #0x110]\n"
    "  ldp q8, q9, [sp, #0x130]\n  ldp q10, q11, [sp, #0x150]\n  ldp q12, q13, [sp, #0x170]\n  ldp q14, q15, [sp, #0x190]\n"
    "  ldp q16, q17, [sp, #0x1b0]\n  ldp q18, q19, [sp, #0x1d0]\n  ldp q20, q21, [sp, #0x1f0]\n  ldp q22, q23, [sp, #0x210]\n"
    "  ldp q24, q25, [sp, #0x230]\n  ldp q26, q27, [sp, #0x250]\n  ldp q28, q29, [sp, #0x270]\n  ldp q30, q31, [sp, #0x290]\n"
    "  ldr x9, [sp, #0xa0]\n  msr nzcv, x9\n"
    "  ldp x17, x18, [sp, #0x90]\n"
    "  ldp x14, x15, [sp, #0x80]\n  ldp x12, x13, [sp, #0x70]\n  ldp x10, x11, [sp, #0x60]\n  ldp x8, x9, [sp, #0x50]\n"
    "  ldp x6, x7, [sp, #0x40]\n  ldp x4, x5, [sp, #0x30]\n  ldp x2, x3, [sp, #0x20]\n  ldp x0, x1, [sp, #0x10]\n"
    "  ldp x29, x30, [sp, #0x00]\n"
    "  add sp, sp, #0x2c0\n"
    "  ret\n");
extern const unsigned char coe5seSealEventCall[];

#define SEAL_EVENT_STUB_WORDS 16
#define SEAL_EVENT_SWITCH_INSN 0x51000908u

// Fills the stub for the switch at `at`, placed at `stub`, with the no-event path `none`.
static void writeSealEventStubMac(uint32_t code[SEAL_EVENT_STUB_WORDS], const unsigned char *stub, const unsigned char *at,
                                  const unsigned char *none, uint32_t moved, uint64_t routine) {
    code[0] = 0xd10243ffu;
    code[1] = 0xa9007bf0u;
    code[2] = 0x58000190u;
    code[3] = 0xd63f0200u;
    code[4] = 0xf9400bf0u;
    code[5] = 0xb40000b0u;
    code[6] = 0xa9407bf0u;
    code[7] = 0x910243ffu;
    code[8] = moved;
    code[9] = 0x14000000u | ((uint32_t)(((at + 4) - (stub + 36)) >> 2) & 0x3ffffffu);
    code[10] = 0xa9407bf0u;
    code[11] = 0x910243ffu;
    code[12] = 0x14000000u | ((uint32_t)((none - (stub + 48)) >> 2) & 0x3ffffffu);
    code[13] = 0xd503201fu;
    memcpy(&code[14], &routine, 8);
}

// Puts the stub in and the site's chain behind it, once.
static int installSealEventMac(void) {
    unsigned char *at = gameBase + RVA_SEAL_EVENT_SWITCH;
    uint32_t moved;
    memcpy(&moved, at, 4);
    if (!gameCodeMatches || moved != SEAL_EVENT_SWITCH_INSN) {
        writeLog("site_skipped", "site=seal_event reason=not_the_expected_code");
        return 0;
    }
    unsigned char *stub = takePadding(sizeof(uint32_t) * SEAL_EVENT_STUB_WORDS);
    if (!stub) {
        writeLog("site_skipped", "site=seal_event reason=no_padding");
        return 0;
    }
    uint32_t code[SEAL_EVENT_STUB_WORDS];
    writeSealEventStubMac(code, stub, at, gameBase + RVA_SEAL_EVENT_NONE, moved, (uint64_t)(uintptr_t)coe5seSealEventCall);
    *siteHead(SITE_SEAL_EVENT) = siteGameFunction(SITE_SEAL_EVENT);
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(at, &branch, 4)) {
        writeLog("site_failed", "site=seal_event reason=write");
        return 0;
    }
    siteInstalled[SITE_SEAL_EVENT] = 1;
    return 1;
}

// ---- NEW_WORLD_INIT_PARTICIPANTS on Mac: the slot defaults new_world copies in ----------------------
// @platform-split participants_init_mac: Mac copies init_participants into new_world, so CoE5SE branches over the copy
static void initParticipantsMac(int mode) {
    int width = G32(RVA_MAP_W), slots = width > 75 ? 9 : width > 67 ? 7 : width > 57 ? 5 : 3;
    G16(RVA_SLOT_CONTROLLER) = 0;
    G16(RVA_SLOT_CLASS) = -1;
    for (int i = 1; i <= slots; i++) {
        G16(RVA_SLOT_CLASS + (size_t)i * 2) = -1;
        G16(RVA_SLOT_CONTROLLER + (size_t)i * 2) = G16(RVA_INIT_PARTICIPANTS_CONTROLLERS + (size_t)(i - 1) * 2);
    }
    for (int i = 1; i <= slots; i++) {
        G16(RVA_SLOT_TEAM + (size_t)i * PLAYER_BLOCK_SIZE) = -1;
        G16(RVA_SLOT_AI + (size_t)i * PLAYER_BLOCK_SIZE) = 2;
    }
    if (mode) G32(RVA_SLOT_CONTROLLER) = (int)0xfffefffe;
    if (*(const unsigned char *)GAME_PTR(RVA_TWO_PLAYER_TEAMS) == 1) {
        static const short TEAMS[8] = {1, 1, 2, 2, 3, 3, 4, 4};
        for (int i = 0; i < 8; i++) G16(RVA_SLOT_TEAM + (size_t)i * PLAYER_BLOCK_SIZE) = TEAMS[i];
    }
}

// The stub's call: the site's chain for new_world's call init_participants(0).
__attribute__((used)) void coe5seParticipantsInit(uintptr_t worldSp, uint64_t d8, uint64_t d9);
__attribute__((used)) void coe5seParticipantsInit(uintptr_t worldSp, uint64_t d8, uint64_t d9) {
    StubData *data = &coe5seStubData[SITE_NEW_WORLD_INIT_PARTICIPANTS];
    data->sp = worldSp;
    memcpy(&data->d8, &d8, 8);
    memcpy(&data->d9, &d9, 8);
    ((void (*)(int))data->head)(0);
}

// Calls the function in x17 with x0 the stub's entry sp, x1 and x2 d8 and d9, and registers kept.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seKeptFrameCall\n_coe5seKeptFrameCall:\n"
    "  sub sp, sp, #0x2c0\n"
    "  stp x29, x30, [sp, #0x00]\n"
    "  mov x29, sp\n"
    "  stp x0, x1, [sp, #0x10]\n  stp x2, x3, [sp, #0x20]\n  stp x4, x5, [sp, #0x30]\n  stp x6, x7, [sp, #0x40]\n"
    "  stp x8, x9, [sp, #0x50]\n  stp x10, x11, [sp, #0x60]\n  stp x12, x13, [sp, #0x70]\n  stp x14, x15, [sp, #0x80]\n"
    "  stp x17, x18, [sp, #0x90]\n"
    "  mrs x9, nzcv\n  str x9, [sp, #0xa0]\n"
    "  stp q0, q1, [sp, #0xb0]\n  stp q2, q3, [sp, #0xd0]\n  stp q4, q5, [sp, #0xf0]\n  stp q6, q7, [sp, #0x110]\n"
    "  stp q8, q9, [sp, #0x130]\n  stp q10, q11, [sp, #0x150]\n  stp q12, q13, [sp, #0x170]\n  stp q14, q15, [sp, #0x190]\n"
    "  stp q16, q17, [sp, #0x1b0]\n  stp q18, q19, [sp, #0x1d0]\n  stp q20, q21, [sp, #0x1f0]\n  stp q22, q23, [sp, #0x210]\n"
    "  stp q24, q25, [sp, #0x230]\n  stp q26, q27, [sp, #0x250]\n  stp q28, q29, [sp, #0x270]\n  stp q30, q31, [sp, #0x290]\n"
    "  ldr x0, [sp, #0x2d8]\n"
    "  fmov x1, d8\n  fmov x2, d9\n"
    "  blr x17\n"
    "  ldp q0, q1, [sp, #0xb0]\n  ldp q2, q3, [sp, #0xd0]\n  ldp q4, q5, [sp, #0xf0]\n  ldp q6, q7, [sp, #0x110]\n"
    "  ldp q8, q9, [sp, #0x130]\n  ldp q10, q11, [sp, #0x150]\n  ldp q12, q13, [sp, #0x170]\n  ldp q14, q15, [sp, #0x190]\n"
    "  ldp q16, q17, [sp, #0x1b0]\n  ldp q18, q19, [sp, #0x1d0]\n  ldp q20, q21, [sp, #0x1f0]\n  ldp q22, q23, [sp, #0x210]\n"
    "  ldp q24, q25, [sp, #0x230]\n  ldp q26, q27, [sp, #0x250]\n  ldp q28, q29, [sp, #0x270]\n  ldp q30, q31, [sp, #0x290]\n"
    "  ldr x9, [sp, #0xa0]\n  msr nzcv, x9\n"
    "  ldp x17, x18, [sp, #0x90]\n"
    "  ldp x14, x15, [sp, #0x80]\n  ldp x12, x13, [sp, #0x70]\n  ldp x10, x11, [sp, #0x60]\n  ldp x8, x9, [sp, #0x50]\n"
    "  ldp x6, x7, [sp, #0x40]\n  ldp x4, x5, [sp, #0x30]\n  ldp x2, x3, [sp, #0x20]\n  ldp x0, x1, [sp, #0x10]\n"
    "  ldp x29, x30, [sp, #0x00]\n"
    "  add sp, sp, #0x2c0\n"
    "  ret\n");
extern const unsigned char coe5seKeptFrameCall[];

#define FRAME_CALL_STUB_WORDS 16
#define PARTICIPANTS_COPY_INSN 0x90000000u

// Fills the stub that replaces code from `at` to `join` with a call of `function`, placed at `stub`.
static void writeFrameCallStubMac(uint32_t code[FRAME_CALL_STUB_WORDS], const unsigned char *stub, const unsigned char *join,
                                  uint64_t routine, uint64_t function) {
    code[0] = 0xd10243ffu;
    code[1] = 0xa9007bf0u;
    code[2] = 0xf9000bf1u;
    code[3] = 0x910243f0u;
    code[4] = 0xf9000ff0u;
    code[5] = 0x580000f0u;
    code[6] = 0x58000111u;
    code[7] = 0xd63f0200u;
    code[8] = 0xf9400bf1u;
    code[9] = 0xa9407bf0u;
    code[10] = 0x910243ffu;
    code[11] = 0x14000000u | ((uint32_t)((join - (stub + 44)) >> 2) & 0x3ffffffu);
    memcpy(&code[12], &routine, 8);
    memcpy(&code[14], &function, 8);
}

// Puts the stub in and the site's chain behind it, once.
static int installParticipantsMac(void) {
    unsigned char *at = gameBase + RVA_INIT_PARTICIPANTS_COPY;
    uint32_t first;
    memcpy(&first, at, 4);
    if (!gameCodeMatches || (first & 0x9f00001fu) != (PARTICIPANTS_COPY_INSN | 8u)) {
        writeLog("site_skipped", "site=new_world_init_participants reason=not_the_expected_code");
        return 0;
    }
    unsigned char *stub = takePadding(sizeof(uint32_t) * FRAME_CALL_STUB_WORDS);
    if (!stub) {
        writeLog("site_skipped", "site=new_world_init_participants reason=no_padding");
        return 0;
    }
    uint32_t code[FRAME_CALL_STUB_WORDS];
    writeFrameCallStubMac(code, stub, gameBase + RVA_INIT_PARTICIPANTS_JOIN, (uint64_t)(uintptr_t)coe5seKeptFrameCall,
                          (uint64_t)(uintptr_t)coe5seParticipantsInit);
    *siteHead(SITE_NEW_WORLD_INIT_PARTICIPANTS) = siteGameFunction(SITE_NEW_WORLD_INIT_PARTICIPANTS);
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(at, &branch, 4)) {
        writeLog("site_failed", "site=new_world_init_participants reason=write");
        return 0;
    }
    siteInstalled[SITE_NEW_WORLD_INIT_PARTICIPANTS] = 1;
    return 1;
}

// Points every call listed for `site` at one trampoline to the site's stub.
static int installSite(CallSite site) {
    const SiteInfo *info = &SITES[site];
    if (site == SITE_SEAL_EVENT) return installSealEventMac();
    if (site == SITE_NEW_WORLD_INIT_PARTICIPANTS) return installParticipantsMac();
    if (isSplitSite(site)) return installSplitSiteMac(site);
    if (!info->rvas[0]) {
        writeLog("site_skipped", "site=%s reason=not_on_this_build", info->name);
        return 0;
    }
    for (int i = 0; i < MAX_SITE_CALLS && info->rvas[i]; i++) {
        unsigned char *call = gameBase + info->rvas[i];
        if (blTarget(call) != gameBase + info->targetRva) {
            writeLog("site_skipped", "site=%s reason=not_the_expected_call address=%p", info->name, call);
            return 0;
        }
    }
    if (site == SITE_CEREMONY_COSTS && !installCeremonyCostsMac()) return 0;
    *siteHead(site) = siteGameFunction(site);
    unsigned char *trampoline = newTrampoline((void *)(coe5seSiteStubs + site * STUB_CODE_SIZE));
    if (!trampoline) {
        writeLog("site_skipped", "site=%s reason=no_trampoline_room", info->name);
        return 0;
    }
    for (int i = 0; i < MAX_SITE_CALLS && info->rvas[i]; i++) {
        unsigned char *call = gameBase + info->rvas[i];
        uint32_t insn = blTo(call, trampoline);
        if (!writeGameCode(call, &insn, 4)) {
            writeLog("site_failed", "site=%s reason=write address=%p", info->name, call);
            return 0;
        }
    }
    siteInstalled[site] = 1;
    return 1;
}

// ---- Multiplayer gate ---------------------------------------------------------------------------
// Every plugin wrapper is entered through a gate, which skips it in network games.
#include <sys/mman.h>
#define MAX_GATES 4096
typedef struct { void *hook, *next; } GatePair;
__attribute__((used)) GatePair coe5seGateData[MAX_GATES];
__attribute__((used)) void *volatile coe5seGateEntered;
static int gateCount;
static int isNetworkGame(void);
__attribute__((used)) int coe5seGateIsNetwork(void) { return isNetworkGame(); }

// Fixed gate g puts g in x17 and jumps to the common code, which turns it into its pair in x16.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seGates\n_coe5seGates:\n"
    ".set gateIndex, 0\n"
    ".rept 4096\n"
    "  mov x17, #gateIndex\n"
    "  b _coe5seGateCommon\n"
    "  .set gateIndex, gateIndex + 1\n"
    ".endr\n"
    "_coe5seGateCommon:\n"
    "  adrp x16, _coe5seGateData@PAGE\n"
    "  add x16, x16, _coe5seGateData@PAGEOFF\n"
    "  add x16, x16, x17, lsl #4\n"
    ".globl _coe5seGateEnter\n_coe5seGateEnter:\n"
    "  stp x29, x30, [sp, #-0xb0]!\n"
    "  mov x29, sp\n"
    "  stp x0, x1, [sp, #0x10]\n  stp x2, x3, [sp, #0x20]\n  stp x4, x5, [sp, #0x30]\n  stp x6, x7, [sp, #0x40]\n"
    "  stp d0, d1, [sp, #0x50]\n  stp d2, d3, [sp, #0x60]\n  stp d4, d5, [sp, #0x70]\n  stp d6, d7, [sp, #0x80]\n"
    "  stp x8, x16, [sp, #0x90]\n"
    "  bl _coe5seGateIsNetwork\n"
    "  mov w17, w0\n"
    "  ldp x0, x1, [sp, #0x10]\n  ldp x2, x3, [sp, #0x20]\n  ldp x4, x5, [sp, #0x30]\n  ldp x6, x7, [sp, #0x40]\n"
    "  ldp d0, d1, [sp, #0x50]\n  ldp d2, d3, [sp, #0x60]\n  ldp d4, d5, [sp, #0x70]\n  ldp d6, d7, [sp, #0x80]\n"
    "  ldp x8, x16, [sp, #0x90]\n"
    "  ldp x29, x30, [sp], #0xb0\n"
    "  cbnz w17, 1f\n"
    "  adrp x17, _coe5seGateEntered@PAGE\n"
    "  str x16, [x17, _coe5seGateEntered@PAGEOFF]\n"
    "  ldr x16, [x16]\n"
    "  br x16\n"
    "1:\n"
    "  ldr x16, [x16, #8]\n"
    "  br x16\n");
extern const unsigned char coe5seGates[], coe5seGateEnter[];
#define GATE_CODE_SIZE 8

// A run-time gate is 32 bytes.
#define RUN_GATE_SIZE      32
#define RUN_GATE_PAIR      16
#define RUN_GATE_PAGE      (MAC_PAGE_MASK + 1)
#define RUN_GATES_PER_PAGE (RUN_GATE_PAGE / RUN_GATE_SIZE)
typedef struct { void *hook, *next, *gate; } RunGatePair;
static RunGatePair *runGatePairs;
static int runGatesUsed = RUN_GATES_PER_PAGE;

// Makes a page of run-time gates.
static int newRunGatePage(void) {
    RunGatePair *pairs = calloc(RUN_GATES_PER_PAGE, sizeof *pairs);
    unsigned char *page = mmap(NULL, RUN_GATE_PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (pairs && page != MAP_FAILED) {
        const uint32_t code[4] = {0x58000090u, 0x580000b1u, 0xd61f0220u, 0};
        const void *enter = coe5seGateEnter;
        for (int i = 0; i < RUN_GATES_PER_PAGE; i++) {
            unsigned char *gate = page + i * RUN_GATE_SIZE;
            const RunGatePair *pair = &pairs[i];
            memcpy(gate, code, sizeof code);
            memcpy(gate + RUN_GATE_PAIR, &pair, 8);
            memcpy(gate + RUN_GATE_PAIR + 8, &enter, 8);
            pairs[i].gate = gate;
        }
        if (!mprotect(page, RUN_GATE_PAGE, PROT_READ | PROT_EXEC)) {
            sys_icache_invalidate(page, RUN_GATE_PAGE);
            runGatePairs = pairs;
            runGatesUsed = 0;
            return 1;
        }
    }
    if (page != MAP_FAILED) munmap(page, RUN_GATE_PAGE);
    free(pairs);
    return 0;
}

static int isFixedGate(const void *gate) {
    uintptr_t g = (uintptr_t)gate, first = (uintptr_t)coe5seGates;
    return g >= first && g < first + GATE_CODE_SIZE * MAX_GATES;
}

// Returns a new gate for `hook` and `next`: a fixed one while any is left, then a run-time one.
static void *makeGate(void *hook, void *next) {
    if (gateCount < MAX_GATES) {
        coe5seGateData[gateCount].hook = hook;
        coe5seGateData[gateCount].next = next;
        return (void *)(coe5seGates + GATE_CODE_SIZE * gateCount++);
    }
    if (runGatesUsed == RUN_GATES_PER_PAGE && !newRunGatePage()) return NULL;
    RunGatePair *pair = &runGatePairs[runGatesUsed++];
    pair->hook = hook;
    pair->next = next;
    gateCount++;
    return pair->gate;
}

static void setGateNext(void *gate, void *next) {
    if (isFixedGate(gate)) {
        coe5seGateData[((const unsigned char *)gate - coe5seGates) / GATE_CODE_SIZE].next = next;
        return;
    }
    RunGatePair *pair;
    memcpy(&pair, (const unsigned char *)gate + RUN_GATE_PAIR, 8);
    pair->next = next;
}

// The gate that entered the wrapper running now, as makeGate returned it.
static void *enteredGate(void) {
    const void *pair = coe5seGateEntered;
    if (!pair) return NULL;
    uintptr_t p = (uintptr_t)pair, first = (uintptr_t)coe5seGateData;
    if (p < first || p >= first + sizeof coe5seGateData) return ((const RunGatePair *)pair)->gate;
    size_t g = (size_t)(p - first) / sizeof coe5seGateData[0];
    return (void *)(coe5seGates + GATE_CODE_SIZE * g);
}

// The caller's sp at the site's call.
static uintptr_t siteCallerSp(CallSite site) { return coe5seStubData[site].sp; }
