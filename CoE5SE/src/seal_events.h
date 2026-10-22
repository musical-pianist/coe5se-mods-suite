// @platform-split ceremony_sites: no Mac call
// A seal's event at the month's end (SITE_SEAL_EVENT).

#ifndef __aarch64__
#define SEAL_EVENT_STAND_IN 0x00
#define SEAL_EVENT_STUB     0x10
#define SEAL_EVENT_FUNCTION 0x80
#define SEAL_EVENT_UNWIND   0x90
#define SEAL_EVENT_UNWIND_CODES_MAX 64
static unsigned char *sealEventPage, *sealEventCall;

static void emitSealRel32(unsigned char **p, unsigned char *target) {
    int rel = (int)(target - (*p + 4));
    memcpy(*p, &rel, 4);
    *p += 4;
}

// Copies the month-end function's unwind info for the stub.
static size_t copySealEventUnwind(unsigned char *out) {
    DWORD64 imageBase = 0;
    PRUNTIME_FUNCTION f = RtlLookupFunctionEntry((DWORD64)(uintptr_t)GAME_PTR(RVA_SEAL_EVENT_SWITCH), &imageBase, NULL);
    if (!f || imageBase != (DWORD64)(uintptr_t)gameBase) return 0;
    const unsigned char *info = gameBase + f->UnwindData;
    int codes = info[2];
    if ((info[0] & 7) != 1 || (info[0] >> 3) & 4 || codes > SEAL_EVENT_UNWIND_CODES_MAX) return 0;
    out[0] = 1;
    out[1] = 0;
    out[2] = (unsigned char)codes;
    out[3] = info[3];
    memcpy(out + 4, info + 4, (size_t)codes * 2);
    return 4 + (size_t)((codes + 1) & ~1) * 2;
}

// Writes the stub and the jump once.
static int patchSealEventStep(void) {
    if (sealEventCall) return 1;
    if (!checkGameFunction("seal_event_step_check", "", RVA_SEAL_EVENT_STEP, RVA_SEAL_EVENT_THIRD, SEAL_EVENT_STEP_CODE, NULL, 0))
        return 0;
    unsigned char *page = allocNear(GAME_PTR(RVA_SEAL_EVENT_SWITCH));
    if (!page) {
        writeLog("seal_event_patched", "ok=0 reason=no_near_page");
        return 0;
    }
    if (!copySealEventUnwind(page + SEAL_EVENT_UNWIND)) {
        VirtualFree(page, 0, MEM_RELEASE);
        writeLog("seal_event_patched", "ok=0 reason=unwind_info");
        return 0;
    }
    static const unsigned char STAND_IN[] = {0xb8, 0x01, 0x00, 0x00, 0x00, 0xc3};
    memcpy(page + SEAL_EVENT_STAND_IN, STAND_IN, sizeof STAND_IN);
    unsigned char *stub = page + SEAL_EVENT_STUB, *p = stub, *call, *skipJump;
    *p++ = 0x89; *p++ = 0xc2;
    *p++ = 0xb9; *p++ = 0xff; *p++ = 0xff; *p++ = 0xff; *p++ = 0xff;
    call = p;
    *p++ = 0xe8; emitSealRel32(&p, page + SEAL_EVENT_STAND_IN);
    *p++ = 0x85; *p++ = 0xc0;
    *p++ = 0x74; skipJump = p++;
    *p++ = 0x8b; *p++ = 0x05; emitSealRel32(&p, GAME_PTR(RVA_SEAL_EVENTS_DONE));
    *p++ = 0x83; *p++ = 0xf8; *p++ = 0x03;
    *p++ = 0x0f; *p++ = 0x85; emitSealRel32(&p, GAME_PTR(RVA_SEAL_EVENT_NOT_THIRD));
    *p++ = 0xe9; emitSealRel32(&p, GAME_PTR(RVA_SEAL_EVENT_THIRD));
    *skipJump = (unsigned char)(p - (skipJump + 1));
    *p++ = 0xe9; emitSealRel32(&p, GAME_PTR(RVA_SEAL_EVENT_NONE));
    RUNTIME_FUNCTION *function = (RUNTIME_FUNCTION *)(page + SEAL_EVENT_FUNCTION);
    function->BeginAddress = SEAL_EVENT_STUB;
    function->EndAddress = (DWORD)(p - page);
    function->UnwindData = SEAL_EVENT_UNWIND;
    if (!RtlAddFunctionTable(function, 1, (DWORD64)(uintptr_t)page)) {
        VirtualFree(page, 0, MEM_RELEASE);
        writeLog("seal_event_patched", "ok=0 reason=unwind_not_added");
        return 0;
    }
    FlushInstructionCache(GetCurrentProcess(), page, 0x100);
    unsigned char *at = GAME_PTR(RVA_SEAL_EVENT_SWITCH), code[9] = {0xe9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90};
    int rel = (int)(stub - (at + 5));
    memcpy(code + 1, &rel, 4);
    DWORD old;
    if (!VirtualProtect(at, sizeof code, PAGE_EXECUTE_READWRITE, &old)) {
        writeLog("seal_event_patched", "ok=0 reason=code_unwritable");
        return 0;
    }
    memcpy(at, code, sizeof code);
    VirtualProtect(at, sizeof code, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, sizeof code);
    sealEventPage = page;
    sealEventCall = call;
    writeLog("seal_event_patched", "ok=1 address=%p stub=%p", (void *)at, (void *)stub);
    return 1;
}

// The call CoE5SE wrote for a site the game has no call for, and its stand-in function.
static unsigned char *generatedSiteCall(CallSite site) { return site == SITE_SEAL_EVENT ? sealEventCall : NULL; }
static unsigned char *generatedSiteTarget(CallSite site) {
    return site == SITE_SEAL_EVENT && sealEventPage ? sealEventPage + SEAL_EVENT_STAND_IN : NULL;
}

// installSite's step before it checks a site's calls.
static int checkSplitMinUnitsCode(void);
static int checkGameStartOnceCode(void);
static int prepareSiteCode(CallSite site) {
    if (site == SITE_GAME_SAVE) return hookSaveFileOpens();
    if (site == SITE_GAME_START) return checkGameStartOnceCode();
    if (site == SITE_AI_SPLIT_ARMY) return checkSplitMinUnitsCode();
    if (site == SITE_SEAL_EVENT) return patchSealEventStep();
    return 1;
}
#endif
