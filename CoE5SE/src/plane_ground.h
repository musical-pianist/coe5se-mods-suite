// The ground of a mod's plane that is `like` a vanilla plane: that plane's ground rules.

static int planeGroundAs[PLANE_RECORDS_MOVED];

// Fills the table for this world's planes: each plane itself, a mod's `like` plane its vanilla plane.
static void setPlaneGroundLikes(void) {
    for (int plane = 0; plane < PLANE_RECORDS_MOVED; plane++) planeGroundAs[plane] = plane;
    for (int i = 0; i < customPlaneCount; i++) {
        const CustomPlane *p = &customPlanes[i];
        if (p->skipped || p->index <= PLANE_ELYSIUM2 || p->index > PLANE_INDEX_LAST_MOVED) continue;
        if (p->recipe >= 0 && p->recipe < PLANE_ELYSIUM2) planeGroundAs[p->index] = p->recipe;
    }
}

#ifndef __APPLE__
// Writes the Windows stub for the match at `at` into `stub`; returns its length.
static size_t writePlaneGroundStubWin(unsigned char *stub, const unsigned char *at, const unsigned char *other) {
    unsigned char *p = stub;
    *p++ = 0x48; *p++ = 0xb8;
    uintptr_t table = (uintptr_t)planeGroundAs;
    memcpy(p, &table, 8);
    p += 8;
    *p++ = 0x8b; *p++ = 0x34; *p++ = 0xb0;
    *p++ = 0x8d; *p++ = 0x46; *p++ = 0xfd;
    *p++ = 0x83; *p++ = 0xf8; *p++ = 0x01;
    *p++ = 0x0f; *p++ = 0x87;
    int rel = (int)(other - (p + 4));
    memcpy(p, &rel, 4);
    p += 4;
    *p++ = 0xe9;
    rel = (int)((at + 8) - (p + 4));
    memcpy(p, &rel, 4);
    p += 4;
    return (size_t)(p - stub);
}

#define PLANE_GROUND_JA_REL 7
static int patchPlaneGroundLike(void) {
    unsigned char *at = GAME_PTR(RVA_SET_GROUND_PLANE_FOUND);
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=plane_ground reason=game_code_differs");
        return 0;
    }
    unsigned char *stub = allocNear(at);
    if (!stub) {
        writeLog("hook_skipped", "site=plane_ground reason=no_near_page");
        return 0;
    }
    writePlaneGroundStubWin(stub, at, at + 8 + (signed char)at[PLANE_GROUND_JA_REL]);
    makePagesReadExecute(stub, 0x1000);
    unsigned char code[6] = {0xe9, 0, 0, 0, 0, 0x90};
    int rel = (int)(stub - (at + 5));
    memcpy(code + 1, &rel, 4);
    if (!writeGameCode(at, code, sizeof code)) {
        writeLog("hook_skipped", "site=plane_ground reason=code_unwritable");
        return 0;
    }
    return 1;
}
#else
// Fills the Mac stub for the match at `at`, placed at `stub`.
static void writePlaneGroundStubMac(uint32_t code[8], const unsigned char *stub, const unsigned char *at, uint64_t table) {
    memcpy(&code[0], at, 4);
    code[1] = 0x580000adu;
    code[2] = 0xb87959b9u;
    code[3] = 0x51000f2cu;
    code[4] = 0x14000000u | ((uint32_t)(((at + 4) - (stub + 16)) >> 2) & 0x3ffffffu);
    code[5] = 0xd503201fu;
    memcpy(&code[6], &table, 8);
}

static int patchPlaneGroundLike(void) {
    unsigned char *at = GAME_PTR(RVA_SET_GROUND_PLANE_FOUND);
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=plane_ground reason=game_code_differs");
        return 0;
    }
    unsigned char *stub = takePadding(32);
    if (!stub) {
        writeLog("hook_skipped", "site=plane_ground reason=no_padding");
        return 0;
    }
    uint32_t code[8];
    writePlaneGroundStubMac(code, stub, at, (uint64_t)(uintptr_t)planeGroundAs);
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(at, &branch, 4)) {
        writeLog("hook_skipped", "site=plane_ground reason=code_unwritable");
        return 0;
    }
    return 1;
}
#endif

// Installs the ground pass's patch, with the planes' other hooks.
static int installPlaneGroundHooks(void) {
    setPlaneGroundLikes();
    int ok = patchPlaneGroundLike();
    writeLog("plane_ground_hook_installed", "ok=%d of 1", ok);
    return ok;
}
