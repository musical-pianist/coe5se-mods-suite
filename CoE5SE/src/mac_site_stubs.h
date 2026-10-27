// Mac: CoE5SE code at one game instruction, for the faction slots.
#pragma once
#ifdef __APPLE__

typedef struct {
    uint64_t x[31];
    uint64_t sp;
    uint64_t nzcv;
    uint64_t resume;
    __uint128_t q[32];
} MacSiteContext;
_Static_assert(sizeof(MacSiteContext) == 784, "MacSiteContext's layout is the assembly's");

enum { MAC_SITE_SQUARE_ARRAY = 1, MAC_SITE_SQUARE_MIRROR, MAC_SITE_SLOT_TEST, MAC_SITE_STACK_GUARD, MAC_SITE_CALLBACK, MAC_SITE_WANDER_GATE, MAC_SITE_LOAD_AS_ZERO, MAC_SITE_AVOID };

typedef struct {
    unsigned char *at;
    uint32_t word;
    unsigned char *back;
    unsigned char *exits;
                            // skip (+0), allow (+16) and refuse (+32)
    int kind, field, arg;
} MacSite;

#define MAC_SITE_MAX 160
static MacSite macSites[MAC_SITE_MAX];
static int macSiteCount;
static unsigned char *macSiteShared;

void macSiteRun(MacSiteContext *c, uint32_t index);

// The shared code: the context on the stack under the entry's 144 bytes, macSiteRun, then return_i.
__asm__(
    ".text\n.p2align 2\n.globl _coe5seMacSiteCommon\n_coe5seMacSiteCommon:\n"
    "  sub sp, sp, #784\n"
    "  stp x0, x1, [sp, #0]\n  stp x2, x3, [sp, #16]\n  stp x4, x5, [sp, #32]\n  stp x6, x7, [sp, #48]\n"
    "  stp x8, x9, [sp, #64]\n  stp x10, x11, [sp, #80]\n  stp x12, x13, [sp, #96]\n  stp x14, x15, [sp, #112]\n"
    "  add x2, sp, #784\n  ldp x0, x1, [x2]\n  stp x0, x1, [sp, #128]\n"
    "  stp x18, x19, [sp, #144]\n  stp x20, x21, [sp, #160]\n  stp x22, x23, [sp, #176]\n  stp x24, x25, [sp, #192]\n"
    "  stp x26, x27, [sp, #208]\n  stp x28, x29, [sp, #224]\n  str x30, [sp, #240]\n"
    "  add x0, sp, #928\n  str x0, [sp, #248]\n"
    "  mrs x0, nzcv\n  str x0, [sp, #256]\n"
    "  stp q0, q1, [sp, #272]\n  stp q2, q3, [sp, #304]\n  stp q4, q5, [sp, #336]\n  stp q6, q7, [sp, #368]\n"
    "  stp q8, q9, [sp, #400]\n  stp q10, q11, [sp, #432]\n  stp q12, q13, [sp, #464]\n  stp q14, q15, [sp, #496]\n"
    "  stp q16, q17, [sp, #528]\n  stp q18, q19, [sp, #560]\n  stp q20, q21, [sp, #592]\n  stp q22, q23, [sp, #624]\n"
    "  stp q24, q25, [sp, #656]\n  stp q26, q27, [sp, #688]\n  stp q28, q29, [sp, #720]\n  stp q30, q31, [sp, #752]\n"
    "  mov x0, sp\n  mov w1, w16\n  bl _macSiteRun\n"
    "  ldp q0, q1, [sp, #272]\n  ldp q2, q3, [sp, #304]\n  ldp q4, q5, [sp, #336]\n  ldp q6, q7, [sp, #368]\n"
    "  ldp q8, q9, [sp, #400]\n  ldp q10, q11, [sp, #432]\n  ldp q12, q13, [sp, #464]\n  ldp q14, q15, [sp, #496]\n"
    "  ldp q16, q17, [sp, #528]\n  ldp q18, q19, [sp, #560]\n  ldp q20, q21, [sp, #592]\n  ldp q22, q23, [sp, #624]\n"
    "  ldp q24, q25, [sp, #656]\n  ldp q26, q27, [sp, #688]\n  ldp q28, q29, [sp, #720]\n  ldp q30, q31, [sp, #752]\n"
    "  ldr x0, [sp, #256]\n  msr nzcv, x0\n"
    "  ldp x0, x1, [sp, #128]\n  add x2, sp, #784\n  stp x0, x1, [x2]\n"
    "  ldp x18, x19, [sp, #144]\n  ldp x20, x21, [sp, #160]\n  ldp x22, x23, [sp, #176]\n  ldp x24, x25, [sp, #192]\n"
    "  ldp x26, x27, [sp, #208]\n  ldp x28, x29, [sp, #224]\n  ldr x30, [sp, #240]\n"
    "  ldr x16, [sp, #264]\n"
    "  ldp x0, x1, [sp, #0]\n  ldp x2, x3, [sp, #16]\n  ldp x4, x5, [sp, #32]\n  ldp x6, x7, [sp, #48]\n"
    "  ldp x8, x9, [sp, #64]\n  ldp x10, x11, [sp, #80]\n  ldp x12, x13, [sp, #96]\n  ldp x14, x15, [sp, #112]\n"
    "  add sp, sp, #784\n  br x16\n");
extern const unsigned char coe5seMacSiteCommon[];

// ---- Decoding the game's instruction -------------------------------------------------------------
// A register's value as an instruction reads it: 31 is sp for a base, the zero register otherwise.
static uint64_t macReg(const MacSiteContext *c, int r, int isBase) { return r == 31 ? (isBase ? c->sp : 0) : c->x[r]; }
static void macSetReg(MacSiteContext *c, int r, uint64_t v) {
    if (r != 31) c->x[r] = v;
}

// An integer load or store with an immediate (scaled or not) or register offset, no writeback.
typedef struct {
    int load, size, signedTo, rt;
    uint64_t address;
} MacMemOp;

// The register offset of a load or store: Rm extended (UXTW, LSL, SXTW, SXTX) and shifted by the size when S.
static uint64_t macRegOffset(const MacSiteContext *c, uint32_t w, int size) {
    uint64_t m = macReg(c, (w >> 16) & 31, 0);
    int option = (w >> 13) & 7, shift = (w >> 12) & 1 ? __builtin_ctz((unsigned)size) : 0;
    if (option == 2) m = (uint32_t)m;
    else if (option == 6) m = (uint64_t)(int64_t)(int32_t)m;
    return m << shift;
}

// Decodes word `w` as an integer load or store; 0 when it is another instruction.
static int macDecodeMemOp(const MacSiteContext *c, uint32_t w, MacMemOp *op) {
    if (((w >> 26) & 1) || ((w >> 27) & 7) != 7) return 0;
    int size = 1 << (w >> 30), opc = (w >> 22) & 3, rn = (w >> 5) & 31;
    uint64_t base = macReg(c, rn, 1);
    if ((w & 0x3b000000) == 0x39000000) op->address = base + (uint64_t)((w >> 10) & 0xfff) * (uint64_t)size;
    else if ((w & 0x3b200c00) == 0x38000000) op->address = base + (uint64_t)(int64_t)((int32_t)(w << 11) >> 23);
    else if ((w & 0x3b200c00) == 0x38200800) op->address = base + macRegOffset(c, w, size);
    else return 0;
    if (opc == 3 && size == 8) return 0;
    op->load = opc != 0, op->size = size, op->rt = w & 31;
    op->signedTo = opc == 2 ? 64 : opc == 3 ? 32 : 0;
    if (opc == 2 && size == 8) return 0;
    return 1;
}

// A 16-byte SIMD store's register, size and address; 0 for another instruction.
static int macDecodeSimdStore(uint32_t w, int *rt, int *size, uint64_t *address, const MacSiteContext *c) {
    uint64_t base = macReg(c, (w >> 5) & 31, 1);
    if ((w & 0xffc00000) == 0x3d800000) *address = base + (uint64_t)((w >> 10) & 0xfff) * 16;
    else if ((w & 0xffe00c00) == 0x3c800000) *address = base + (uint64_t)(int64_t)((int32_t)(w << 11) >> 23);
    else return 0;
    *rt = w & 31, *size = 16;
    return 1;
}

// Does the decoded load or store at `address` (which may differ from the op's own, for a redirect).
static void macDoMemOp(MacSiteContext *c, const MacMemOp *op, unsigned char *address) {
    if (!op->load) {
        uint64_t v = macReg(c, op->rt, 0);
        memcpy(address, &v, (size_t)op->size);
        return;
    }
    uint64_t v = 0;
    memcpy(&v, address, (size_t)op->size);
    if (op->signedTo) {
        int bits = op->size * 8;
        int64_t s = (int64_t)(v << (64 - bits)) >> (64 - bits);
        v = op->signedTo == 32 ? (uint32_t)s : (uint64_t)s;
    }
    macSetReg(c, op->rt, v);
}

// A compare's two values, width, condition and flags.
typedef struct { int rn, add, wide, cond, nzcv; uint64_t operand; } MacCompare;

static int macDecodeCompare(const MacSiteContext *c, uint32_t w, MacCompare *cmp) {
    cmp->wide = (w >> 31) & 1, cmp->rn = (w >> 5) & 31, cmp->cond = -1, cmp->nzcv = 0;
    if ((w & 0x7fe00c10) == 0x7a400800 || (w & 0x7fe00c10) == 0x3a400800) {
        cmp->add = (w & 0x7fe00c10) == 0x3a400800;
        cmp->operand = (w >> 16) & 31, cmp->cond = (w >> 12) & 15, cmp->nzcv = w & 15;
        return 1;
    }
    if ((w & 31) != 31) return 0;
    if ((w & 0x7f800000) == 0x71000000 || (w & 0x7f800000) == 0x31000000) {
        cmp->add = (w & 0x7f800000) == 0x31000000;
        cmp->operand = (uint64_t)((w >> 10) & 0xfff) << ((w >> 22) & 1 ? 12 : 0);
        return 1;
    }
    if ((w & 0x7f200000) == 0x6b000000 || (w & 0x7f200000) == 0x2b000000) {
        cmp->add = (w & 0x7f200000) == 0x2b000000;
        uint64_t m = macReg(c, (w >> 16) & 31, 0);
        int shift = (w >> 10) & 63, type = (w >> 22) & 3;
        if (!cmp->wide) m = (uint32_t)m;
        m = type == 0 ? m << shift : type == 1 ? m >> shift : (uint64_t)((int64_t)m >> shift);
        cmp->operand = cmp->wide ? m : (uint32_t)m;
        return 1;
    }
    return 0;
}

// Whether condition `cond` holds for flags `nzcv` (N, Z, C, V in bits 31..28).
static int macConditionHolds(uint64_t nzcv, int cond) {
    int n = (nzcv >> 31) & 1, z = (nzcv >> 30) & 1, carry = (nzcv >> 29) & 1, v = (nzcv >> 28) & 1, holds;
    switch (cond >> 1) {
    case 0: holds = z; break;
    case 1: holds = carry; break;
    case 2: holds = n; break;
    case 3: holds = v; break;
    case 4: holds = carry && !z; break;
    case 5: holds = n == v; break;
    case 6: holds = n == v && !z; break;
    default: return 1;
    }
    return cond & 1 ? !holds : holds;
}

// The flags a compare of `a` with the operand sets, as the CPU sets them (N, Z, C, V in bits 31..28).
static uint64_t macCompareFlags(uint64_t a, const MacCompare *cmp) {
    uint64_t b = cmp->operand, r;
    int n, z, carry, v;
    if (cmp->wide) {
        if (cmp->add) r = a + b, carry = r < a, v = (int)((~(a ^ b) & (a ^ r)) >> 63);
        else r = a - b, carry = a >= b, v = (int)(((a ^ b) & (a ^ r)) >> 63);
        n = (int)(r >> 63), z = r == 0;
    } else {
        uint32_t a32 = (uint32_t)a, b32 = (uint32_t)b, r32;
        if (cmp->add) r32 = a32 + b32, carry = r32 < a32, v = (int)((~(a32 ^ b32) & (a32 ^ r32)) >> 31);
        else r32 = a32 - b32, carry = a32 >= b32, v = (int)(((a32 ^ b32) & (a32 ^ r32)) >> 31);
        n = (int)(r32 >> 31), z = r32 == 0;
    }
    return (uint64_t)n << 31 | (uint64_t)z << 30 | (uint64_t)carry << 29 | (uint64_t)v << 28;
}
// The flags after the compare, from flags `before`: a conditional compare whose condition fails sets its own.
static uint64_t macCompareFlagsAfter(uint64_t a, const MacCompare *cmp, uint64_t before) {
    if (cmp->cond >= 0 && !macConditionHolds(before, cmp->cond)) return (uint64_t)cmp->nzcv << 28;
    return macCompareFlags(a, cmp);
}

// ---- The kinds -----------------------------------------------------------------------------------
// The side array entry a square array address names, when it is a faction slot's; NULL for a game slot's.
static unsigned char *macFactionSquareEntry(uint64_t address, int field) {
    int size = field == SQUARE_AI_STRENGTH ? 4 : 1;
    uint64_t first = (uint64_t)(uintptr_t)(gameBase + RVA_SQUARE_RECORDS) + (uint64_t)field;
    if (address < first) return NULL;
    uint64_t d = address - first, square = d / SQUARE_SIZE;
    int player = (int)(d % SQUARE_SIZE) / size;
    if (!factionSlot(player) || square >= FACTION_SQUARES) return NULL;
    size_t i = (size_t)square * FACTION_SLOT_TARGET + (size_t)(player - PLAYER_SLOTS_GAME);
    return field == SQUARE_SEEN ? factionSquareSeen + i
         : field == SQUARE_SEEN_MARK ? factionSquareSeenMark + i
         : field == SQUARE_TRUE_TERRAIN_SEEN ? factionSquareTrueSeen + i
         : (unsigned char *)(factionSquareAiStrength + i);
}

// A mirror: the game's store set a game slot's entries of a square from `address`; its faction slots get `value` too.
static void macMirrorFactionRow(uint64_t address, int field, unsigned char value) {
    uint64_t first = (uint64_t)(uintptr_t)(gameBase + RVA_SQUARE_RECORDS) + (uint64_t)field;
    if (address < first) return;
    size_t square = (size_t)((address - first) / SQUARE_SIZE);
    if (square >= FACTION_SQUARES) return;
    unsigned char *rows = field == SQUARE_SEEN ? factionSquareSeen : field == SQUARE_SEEN_MARK ? factionSquareSeenMark : factionSquareTrueSeen;
    memset(rows + square * FACTION_SLOT_TARGET, value, FACTION_SLOT_TARGET);
}

static int macSlotTestValue(int behaviour, int slot);
static int macWanderGate(int slot, int plane, int value);
static int macAvoidRefusal(int mover, int owner);

// The steps the game unrolled over its 32 slots, done for the faction slots (MAC_SITE_CALLBACK's arg).
enum { MAC_CALLBACK_CLEAR_TURN_STARTED, MAC_CALLBACK_CLEAR_INT_C_BIT0, MAC_CALLBACK_RESET_INCOME_CACHES, MAC_CALLBACK_COUNT };
#define PLAYER_INCOME_CACHE_BYTES (14 * 4)
static void macFactionSlotsClearIntC(unsigned int bit) {
    for (int f = PLAYER_SLOTS_GAME; f < PLAYER_SLOTS_MOVED; f++) {
        unsigned int v;
        unsigned char *at = playerSlotAddress(RVA_PLAYER_INT_C, f, PLAYER_BLOCK_SIZE);
        memcpy(&v, at, 4);
        v &= ~bit;
        memcpy(at, &v, 4);
    }
}
static void macRunCallback(int which) {
    if (!factionSlotsInUse()) return;
    if (which == MAC_CALLBACK_CLEAR_TURN_STARTED) macFactionSlotsClearIntC(2);
    else if (which == MAC_CALLBACK_CLEAR_INT_C_BIT0) macFactionSlotsClearIntC(1);
    else if (which == MAC_CALLBACK_RESET_INCOME_CACHES)
        for (int f = PLAYER_SLOTS_GAME; f < PLAYER_SLOTS_MOVED; f++)
            memset(playerSlotAddress(RVA_PLAYER_INCOME_CACHE, f, PLAYER_BLOCK_SIZE), 0xff, PLAYER_INCOME_CACHE_BYTES);
}

// Does site `index`'s instruction, or its faction path.
void macSiteRun(MacSiteContext *c, uint32_t index) {
    const MacSite *s = &macSites[index];
    c->resume = (uint64_t)(uintptr_t)s->back;
    MacMemOp op;
    MacCompare cmp;
    if (s->kind == MAC_SITE_SLOT_TEST && macDecodeCompare(c, s->word, &cmp)) {
        uint64_t a = macReg(c, cmp.rn, 0);
        int slot = (int)(int32_t)a;
        if (factionSlot(slot)) a = (uint64_t)(int64_t)macSlotTestValue(s->arg, slot);
        c->nzcv = macCompareFlagsAfter(cmp.wide ? a : (uint32_t)a, &cmp, c->nzcv);
        return;
    }
    if (s->kind == MAC_SITE_WANDER_GATE && macDecodeCompare(c, s->word, &cmp)) {
        int32_t slot;
        memcpy(&slot, (const void *)(uintptr_t)(c->sp + (uint64_t)s->field), 4);
        int gate = factionSlot(slot) ? macWanderGate(slot, (int)(int32_t)c->x[8], (int)(int32_t)c->x[19]) : -1;
        if (gate >= 0) c->resume = (uint64_t)(uintptr_t)(s->exits + (gate ? 0 : 16));
        else c->nzcv = macCompareFlagsAfter(cmp.wide ? macReg(c, cmp.rn, 0) : (uint32_t)macReg(c, cmp.rn, 0), &cmp, c->nzcv);
        return;
    }
    if (s->kind == MAC_SITE_AVOID) {
        int refusal = macAvoidRefusal((int)(int32_t)c->x[9], (int)(int32_t)c->x[8]);
        int exit = !refusal ? 0 : GAME_FN(int (*)(int), RVA_RANDOM)(100) < refusal ? 32 : 16;
        c->resume = (uint64_t)(uintptr_t)(s->exits + exit);
        return;
    }
    if (s->kind == MAC_SITE_SQUARE_MIRROR && macDecodeSimdStore(s->word, &op.rt, &op.size, &op.address, c)) {
        memcpy((void *)(uintptr_t)op.address, &c->q[op.rt], 16);
        op.load = 0;
        macMirrorFactionRow(op.address, s->field, (unsigned char)(c->q[op.rt] & 0xff));
        return;
    }
    if (!macDecodeMemOp(c, s->word, &op)) return;
    if (s->kind == MAC_SITE_STACK_GUARD) {
        uint64_t offset = op.address - macReg(c, (s->word >> 5) & 31, 1);
        if (offset < (uint64_t)s->field) macDoMemOp(c, &op, (unsigned char *)(uintptr_t)op.address);
        return;
    }
    if (s->kind == MAC_SITE_CALLBACK) {
        macDoMemOp(c, &op, (unsigned char *)(uintptr_t)op.address);
        macRunCallback(s->arg);
        return;
    }
    if (s->kind == MAC_SITE_LOAD_AS_ZERO) {
        macDoMemOp(c, &op, (unsigned char *)(uintptr_t)op.address);
        int slot = (int)(int32_t)macReg(c, s->field, 0);
        if (op.load && factionSlot(slot) && macSlotTestValue(s->arg >> 8, slot) == (s->arg & 0xff)) macSetReg(c, op.rt, 0);
        return;
    }
    unsigned char *side = s->kind == MAC_SITE_SQUARE_ARRAY || s->kind == MAC_SITE_SQUARE_MIRROR ? macFactionSquareEntry(op.address, s->field) : NULL;
    if (s->kind == MAC_SITE_SQUARE_ARRAY && side) {
        macDoMemOp(c, &op, side);
        return;
    }
    macDoMemOp(c, &op, (unsigned char *)(uintptr_t)op.address);
    if (s->kind == MAC_SITE_SQUARE_MIRROR && !op.load) macMirrorFactionRow(op.address, s->field, (unsigned char)macReg(c, op.rt, 0));
}

// ---- Installing a site ---------------------------------------------------------------------------
static uint32_t macBranchTo(const unsigned char *at, const unsigned char *target) {
    return 0x14000000u | ((uint32_t)((target - at) >> 2) & 0x3ffffffu);
}

// The shared jump's 16 bytes: ldr x17, #8; br x17; .quad coe5seMacSiteCommon.
static void macSharedCode(unsigned char out[16]) {
    static const uint32_t SHARED[2] = {0x58000051u, 0xd61f0220u};
    uint64_t target = (uint64_t)(uintptr_t)coe5seMacSiteCommon;
    memcpy(out, SHARED, 8), memcpy(out + 8, &target, 8);
}

// Site `index`'s entry and return (32 bytes at `entry`), for the game's instruction at `at`.
static void macSiteCode(uint32_t code[8], const unsigned char *entry, const unsigned char *shared, const unsigned char *at, int index) {
    const unsigned char *back = entry + 16;
    code[0] = 0xd10243ffu;
    code[1] = 0xa90047f0u;
    code[2] = 0x52800010u | ((uint32_t)index << 5);
    code[3] = macBranchTo(entry + 12, shared);
    code[4] = 0xa94047f0u;
    code[5] = 0x910243ffu;
    code[6] = macBranchTo(back + 8, at + 4);
    code[7] = 0xd503201fu;
}

// Whether macSiteRun can do word `w` for `kind`.
static int macSiteDecodes(uint32_t w, int kind) {
    MacSiteContext probe = {0};
    MacMemOp op;
    MacCompare cmp;
    if (kind == MAC_SITE_SLOT_TEST || kind == MAC_SITE_WANDER_GATE || kind == MAC_SITE_AVOID) return macDecodeCompare(&probe, w, &cmp);
    if (kind == MAC_SITE_SQUARE_MIRROR && macDecodeSimdStore(w, &op.rt, &op.size, &op.address, &probe)) return 1;
    return macDecodeMemOp(&probe, w, &op);
}

// Puts site `at` on macSiteRun with `kind`; 1 when it is in.
static int macSiteAdd(unsigned char *at, int kind, int field, int arg, const char *name) {
    uint32_t live;
    memcpy(&live, at, 4);
    const char *reason = !gameCodeMatches ? "game_code_differs" : macSiteCount >= MAC_SITE_MAX ? "too_many_sites"
                       : !macSiteDecodes(live, kind) ? "not_decoded" : NULL;
    if (!reason && !macSiteShared) {
        unsigned char *spot = takePadding(16), code[16];
        macSharedCode(code);
        if (spot && writeGameCode(spot, code, 16)) macSiteShared = spot;
        else reason = spot ? "code_unwritable" : "no_padding";
    }
    unsigned char *entry = reason ? NULL : takePadding(32);
    if (!reason && !entry) reason = "no_padding";
    uint32_t code[8];
    if (!reason) {
        macSiteCode(code, entry, macSiteShared, at, macSiteCount);
        if (!writeGameCode(entry, code, sizeof code)) reason = "code_unwritable";
    }
    unsigned char *exits = NULL;
    if (!reason && (kind == MAC_SITE_WANDER_GATE || kind == MAC_SITE_AVOID)) {
        unsigned int targets[3] = {RVA_WANDER_GATE_WAIT, RVA_WANDER_GATE_PASS, 0};
        int count = 2;
        if (kind == MAC_SITE_AVOID) targets[0] = RVA_INDEP_AVOID_SKIP, targets[1] = RVA_INDEP_AVOID_ALLOW, targets[2] = RVA_INDEP_AVOID_REFUSE, count = 3;
        exits = takePadding((size_t)count * 16);
        uint32_t ways[12];
        for (int i = 0; exits && i < count; i++) {
            ways[i * 4] = 0xa94047f0u, ways[i * 4 + 1] = 0x910243ffu;
            ways[i * 4 + 2] = macBranchTo(exits + i * 16 + 8, gameBase + targets[i]), ways[i * 4 + 3] = 0xd503201fu;
        }
        if (!exits) reason = "no_padding";
        else if (!writeGameCode(exits, ways, (size_t)count * 16)) reason = "code_unwritable";
    }
    if (!reason) {
        macSites[macSiteCount++] = (MacSite){at, live, entry + 16, exits, kind, field, arg};
        uint32_t b = macBranchTo(at, entry);
        if (!writeGameCode(at, &b, 4)) macSiteCount--, reason = "code_unwritable";
    }
    if (reason) {
        writeLog("mac_site_refused", "site=%s va=0x%llx reason=%s", name, (unsigned long long)(at - gameBase) + 0x100000000ULL, reason);
        return 0;
    }
    return 1;
}

// ---- The faction slots' sites -------------------------------------------------------------------
// The game instructions CoE5SE runs for the faction slots on Mac.
typedef struct { unsigned int rva; unsigned char kind; short field, arg; const char *name; } MacFactionSite;
static const MacFactionSite MAC_FACTION_SITES[] = {
    // RVA_PLAYER_TURN_START's copy of RVA_AI_REVEAL_PORTAL_EXITS: a portal square in view, then each square of its portal seen
    {MAC(0x1003929e0), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "turn_start_portal_seen"},
    {MAC(0x100392a4c), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "turn_start_exit_seen"},
    {MAC(0x100392a58), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "turn_start_exit_seen_write"},
    // RVA_PLAY_GAME's copy of RVA_AUTO_RECRUIT's seen-mark aging: each mark up by 1 to 100, 0 on the player's squares
    {MAC(0x10038f8d8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN_MARK, 0, "aging_mark"},
    {MAC(0x10038f8e8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN_MARK, 0, "aging_mark_write"},
    {MAC(0x10038f914), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN_MARK, 0, "aging_mark_owned"},
    // the readers a faction's turn reaches
    {MAC(0x10046805c), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "unit_visible_to_seen"},
    {MAC(0x100469cb8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "random_square_seen"},
    {MAC(0x100548768), MAC_SITE_SQUARE_ARRAY, SQUARE_TRUE_TERRAIN_SEEN, 0, "wanderer_true_seen"},
    {MAC(0x100548bc0), MAC_SITE_SQUARE_ARRAY, SQUARE_TRUE_TERRAIN_SEEN, 0, "wanderer_true_seen_b"},
    {MAC(0x100498f94), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "top_unit_seen"},
    {MAC(0x100498fb8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "top_unit_seen_b"},
    {MAC(0x10059a740), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "step_portal_seen"},
    {MAC(0x10059bc98), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "far_check_seen"},
    {MAC(0x10059bd78), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "far_check_in_view"},
    {MAC(0x10059be94), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "far_check_seen_at"},
    {MAC(0x10059c5bc), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "waytogo_portal_seen"},
    {MAC(0x10059cd24), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "spread_portal_seen"},
    // RVA_EXECUTE_RITUAL for a faction caster, and its portal pair's reveal
    {MAC(0x10055f814), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_write"},
    {MAC(0x1005604d0), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_write_b"},
    {MAC(0x100560938), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_at"},
    {MAC(0x100561718), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_at_b"},
    {MAC(0x10056178c), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_at_c"},
    {MAC(0x1005618d8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_at_d"},
    {MAC(0x10056194c), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "ritual_seen_at_e"},
    {MAC(0x1005718e8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "portal_pair_seen"},
    {MAC(0x1005718f4), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "portal_pair_seen_write"},
    // the ritual reveal (0x10035b2c4), with Mac's copies of RVA_REVEAL_DISC's team step
    {MAC(0x10035b528), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_seen"},
    {MAC(0x10035b654), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team"},
    {MAC(0x10035b660), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team_write"},
    {MAC(0x10035b6f8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team_b"},
    {MAC(0x10035b704), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team_write_b"},
    {MAC(0x10035b7d8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_seen_b"},
    {MAC(0x10035b87c), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team_c"},
    {MAC(0x10035b888), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_team_write_c"},
    {MAC(0x10035b9ac), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN, 0, "terrain_reveal_seen_c"},
    // RVA_AFTER_GAME_TURN's per-slot writes.
    {MAC(0x1003a58e8), MAC_SITE_SQUARE_ARRAY, SQUARE_SEEN_MARK, 0, "cloud_owner_mark"},
    {MAC(0x1003b50bc), MAC_SITE_SQUARE_MIRROR, SQUARE_SEEN, 0, "void_seen_clear"},
    {MAC(0x1003b59f4), MAC_SITE_STACK_GUARD, 4 * PLAYER_SLOTS_GAME, 0, "crash_landing_count"},
    {MAC(0x1003b6240), MAC_SITE_STACK_GUARD, 4 * PLAYER_SLOTS_GAME, 0, "land_drown_count"},
    {MAC(0x1003b65e0), MAC_SITE_STACK_GUARD, 4 * PLAYER_SLOTS_GAME, 0, "drown_count"},
    {MAC(0x1003b68b0), MAC_SITE_STACK_GUARD, 4 * PLAYER_SLOTS_GAME, 0, "drown_count_b"},
    // RVA_DO_ALL_FIGHTS: a unit owner's entry in the int[32] of the slots in the battle (sp + 0xaf0), as Windows' guardjs
    {MAC(0x1003dc90c), MAC_SITE_STACK_GUARD, 4 * PLAYER_SLOTS_GAME, 0, "battle_slots"},
    {MAC(0x1003a1184), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_CLEAR_TURN_STARTED, "month_turn_started"},
    {MAC(0x1003a2e0c), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_CLEAR_INT_C_BIT0, "month_int_c_bit0"},
    // RVA_RESET_INCOME_CACHES and its three unrolled copies: each runs the faction slots' reset at its slot 31 store
    {MAC(0x100568214), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_RESET_INCOME_CACHES, "reset_income_caches"},
    {MAC(0x10037b59c), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_RESET_INCOME_CACHES, "map_screen_income_caches"},
    {MAC(0x1004aaff0), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_RESET_INCOME_CACHES, "conquer_income_caches"},
    {MAC(0x1005632c0), MAC_SITE_CALLBACK, 0, MAC_CALLBACK_RESET_INCOME_CACHES, "ritual_income_caches"},
};
#define MAC_FACTION_SITE_COUNT (int)(sizeof MAC_FACTION_SITES / sizeof MAC_FACTION_SITES[0])

// Puts each site on macSiteRun; 1 when every one is in.
static int installMacFactionSites(void) {
    int ok = 0;
    for (int i = 0; i < MAC_FACTION_SITE_COUNT; i++) {
        const MacFactionSite *s = &MAC_FACTION_SITES[i];
        ok += macSiteAdd(gameBase + s->rva, s->kind, s->field, s->arg, s->name);
    }
    writeLog("mac_faction_sites_installed", "ok=%d of %d padding_left=%ld", ok, MAC_FACTION_SITE_COUNT, paddingLeft());
    return ok == MAC_FACTION_SITE_COUNT;
}

// Points every branch to `rva` at `hook`; the number pointed.
static int macRedirectCalls(unsigned int rva, void *hook, const char *name) {
    const struct mach_header_64 *mh = (const struct mach_header_64 *)gameBase;
    unsigned long size;
    unsigned char *text = getsectiondata(mh, "__TEXT", "__text", &size), *target = gameBase + rva, *trampoline = NULL;
    int pointed = 0, found = 0;
    for (unsigned long i = 0; text && i + 4 <= size; i += 4) {
        uint32_t w;
        memcpy(&w, text + i, 4);
        uint32_t op = w >> 26;
        if (op != 0x25 && op != 0x05) continue;
        int32_t imm = (int32_t)(w << 6) >> 6;
        if (text + i + (intptr_t)imm * 4 != target) continue;
        found++;
        if (!trampoline && !(trampoline = newTrampoline(hook))) break;
        uint32_t nw = (op << 26) | ((uint32_t)((trampoline - (text + i)) >> 2) & 0x3ffffffu);
        pointed += writeGameCode(text + i, &nw, 4);
    }
    writeLog("mac_calls_redirected", "function=%s found=%d pointed=%d", name, found, pointed);
    return pointed;
}

#endif
