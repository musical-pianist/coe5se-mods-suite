// Planes' tints: the Sky's Blood Sun tint on a mod's `like "Sky"` plane, and tints set from Lua.

#define PLANE_TINT_CHANNELS 4
#define PLANE_TINT_MAX      1.0f

static struct {
    int set;
    float rgba[PLANE_TINT_CHANNELS];
} luaPlaneTints[PLANE_RECORDS_MOVED];
static int planeTintHooks;

// Writes plane `plane`'s tint byte and colour in its record.
static void writePlaneTint(int plane, int on, const float rgba[PLANE_TINT_CHANNELS]) {
    unsigned char *record = planeRecord(plane);
    record[PLANE_TINT_ON] = (unsigned char)on;
    memcpy(record + PLANE_TINT, rgba, sizeof(float) * PLANE_TINT_CHANNELS);
}

// After each Blood Sun update: the Sky's tint onto `like "Sky"` planes, then scripts' tints.
static void applyPlaneTints(void) {
    int last = G32(RVA_LAST_PLANE);
    if (last >= planeTableRecords()) last = planeTableRecords() - 1;
    if (last < PLANE_SKY) return;
    int copies = 0, scripts = 0;
    const unsigned char *sky = planeRecord(PLANE_SKY);
    for (int i = 0; i < customPlaneCount; i++) {
        const CustomPlane *p = &customPlanes[i];
        if (p->recipe != PLANE_SKY || p->skipped || p->index <= PLANE_ELYSIUM2 || p->index > last) continue;
        float rgba[PLANE_TINT_CHANNELS];
        memcpy(rgba, sky + PLANE_TINT, sizeof rgba);
        writePlaneTint(p->index, sky[PLANE_TINT_ON], rgba);
        copies++;
    }
    for (int plane = 0; plane <= last && !isNetworkGame(); plane++) {
        if (!luaPlaneTints[plane].set) continue;
        writePlaneTint(plane, 1, luaPlaneTints[plane].rgba);
        scripts++;
    }
    if (copies || scripts)
        writeLog("plane_tints_applied", "sky_copies=%d script_tints=%d sky_on=%d", copies, scripts, sky[PLANE_TINT_ON]);
}

// Forgets every script's tint, at each level-2 mod pass (a load, a game's end, startup, the Mods screen).
static void clearLuaPlaneTints(int level) {
    if (level != 2) return;
    memset(luaPlaneTints, 0, sizeof luaPlaneTints);
}

#ifndef __APPLE__
// The game's Blood Sun update, then the planes' tints.
static int planeTintUpdateHook(void) {
    int result = GAME_FN(int (*)(void), RVA_BLOOD_SUN_UPDATE)();
    applyPlaneTints();
    return result;
}

static int patchPlaneTintUpdates(void) {
    return pointCustomPlaneCall(RVA_MAP_SCREEN_BLOOD_SUN_CALL, RVA_BLOOD_SUN_UPDATE, (void *)planeTintUpdateHook, "map_screen_blood_sun") +
           pointCustomPlaneCall(RVA_RITUAL_BLOOD_SUN_CALL, RVA_BLOOD_SUN_UPDATE, (void *)planeTintUpdateHook, "ritual_blood_sun");
}
#else
__attribute__((used)) void coe5sePlaneTintAfterUpdate(void) { applyPlaneTints(); }

// Calls coe5sePlaneTintAfterUpdate with every register the call could change kept.
__asm__(
    ".text\n.p2align 2\n.globl _coe5sePlaneTintCall\n_coe5sePlaneTintCall:\n"
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
    "  bl _coe5sePlaneTintAfterUpdate\n"
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
extern const unsigned char coe5sePlaneTintCall[];

#define PLANE_TINT_STUB_WORDS 12
#define PLANE_TINT_RED_ZONE   0x90

// Fills the Mac stub for the join at `at`, placed at `stub`, whose instruction there is `moved`.
static void writePlaneTintStubMac(uint32_t code[PLANE_TINT_STUB_WORDS], const unsigned char *stub, const unsigned char *at, uint32_t moved,
                                  uint64_t routine) {
    code[0] = 0xd10243ffu;
    code[1] = 0xa9007bf0u;
    code[2] = 0x58000110u;
    code[3] = 0xd63f0200u;
    code[4] = 0xa9407bf0u;
    code[5] = 0x910243ffu;
    code[6] = moved;
    code[7] = 0x14000000u | ((uint32_t)(((at + 4) - (stub + 28)) >> 2) & 0x3ffffffu);
    code[8] = code[9] = 0xd503201fu;
    memcpy(&code[10], &routine, 8);
}

// Points the join at `rva` at the stub, which runs the game's instruction there after the routine.
static int patchPlaneTintJoin(unsigned int rva, const char *name) {
    unsigned char *at = GAME_PTR(rva);
    uint32_t insn;
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=%s reason=game_code_differs", name);
        return 0;
    }
    memcpy(&insn, at, 4);
    unsigned char *stub = takePadding(sizeof(uint32_t) * PLANE_TINT_STUB_WORDS);
    if (!stub) {
        writeLog("hook_skipped", "site=%s reason=no_padding", name);
        return 0;
    }
    uint32_t code[PLANE_TINT_STUB_WORDS];
    writePlaneTintStubMac(code, stub, at, insn, (uint64_t)(uintptr_t)coe5sePlaneTintCall);
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(at, &branch, 4)) {
        writeLog("hook_skipped", "site=%s reason=code_unwritable", name);
        return 0;
    }
    return 1;
}

static int patchPlaneTintUpdates(void) {
    return patchPlaneTintJoin(RVA_MAP_SCREEN_BLOOD_SUN_DONE, "map_screen_blood_sun") +
           patchPlaneTintJoin(RVA_RITUAL_BLOOD_SUN_DONE, "ritual_blood_sun");
}
#endif

// Puts the hooks after both Blood Sun updates in, once.
static int installPlaneTintHooks(void) {
    if (planeTintHooks) return planeTintHooks > 0;
    int ok = patchPlaneTintUpdates();
    planeTintHooks = ok == 2 ? 1 : -1;
    writeLog("plane_tint_hooks_installed", "ok=%d of 2", ok);
    return planeTintHooks > 0;
}

// The index of the plane a script names for its tint; -1 for none.
static int planeTintNamed(const char *name) {
    int plane = vanillaPlaneNamed(name);
    if (plane < 0 && !strcmp(name, VANILLA_PLANE_NAMES[PLANE_ELYSIUM2])) plane = PLANE_ELYSIUM2;
    if (plane < 0) plane = customPlaneIndex(name);
    return plane;
}

// The highest plane index a script may tint now.
static int planeTintLast(void) {
    int last = G32(RVA_LAST_PLANE);
    if (last >= planeTableRecords()) last = planeTableRecords() - 1;
    return last > PLANE_ELYSIUM2 ? last : PLANE_ELYSIUM2;
}

// A script's tint on plane `plane` (0 to planeTintLast()), written at once and again after each update.
static const char *setLuaPlaneTint(int plane, const double rgba[PLANE_TINT_CHANNELS]) {
    if (plane < 0 || plane > planeTintLast()) return "no_such_plane";
    for (int c = 0; c < PLANE_TINT_CHANNELS; c++)
        if (!(rgba[c] >= 0 && rgba[c] <= PLANE_TINT_MAX)) return "out_of_range";
    if (!installPlaneTintHooks()) return "hooks_not_installed";
    luaPlaneTints[plane].set = 1;
    for (int c = 0; c < PLANE_TINT_CHANNELS; c++) luaPlaneTints[plane].rgba[c] = (float)rgba[c];
    if (plane <= G32(RVA_LAST_PLANE)) writePlaneTint(plane, 1, luaPlaneTints[plane].rgba);
    return NULL;
}

// Drops a script's tint on plane `plane`: the next update gives it the game's own. NULL, or the reason.
static const char *clearLuaPlaneTint(int plane) {
    if (plane < 0 || plane > planeTintLast()) return "no_such_plane";
    luaPlaneTints[plane].set = 0;
    return NULL;
}
