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
static void *siteGameFunction(CallSite site) {
    if (isInlinedButtonRowSite(site)) return (void *)buttonRowMac;
    return gameBase + SITES[site].targetRva;
}

// Points every call listed for `site` at one trampoline to the site's stub.
static int installSite(CallSite site) {
    const SiteInfo *info = &SITES[site];
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
