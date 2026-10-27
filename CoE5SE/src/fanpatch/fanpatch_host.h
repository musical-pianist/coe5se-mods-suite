// The fan patch's host: what Alex the Beetrayer's three game fixes call in CoE5SE.

typedef struct {
    const char *name;
    unsigned int rva;
    int size;
    int fixupCount;
    struct { unsigned char at; unsigned int target; } fixups[2];
} EntryHookSite;

#include "entry_hook_sites.h"
#include "fanpatch_bridge.h"

static struct { unsigned char *trampoline, *stub; void *detour; int enabled; unsigned char saved[16]; } entryHooks[ENTRY_HOOK_COUNT];

// The hook at `target`, the start of a listed game function, or -1.
static int entryHookAt(const void *target) {
    for (int i = 0; i < ENTRY_HOOK_COUNT; i++)
        if (gameBase + ENTRY_HOOK_SITES[i].rva == (const unsigned char *)target) return i;
    return -1;
}

#ifndef __APPLE__
static unsigned char *entryHookPage;
#endif

int fanpatchHookCreate(void *target, void *detour, void **original) {
    int i = entryHookAt(target);
    if (i < 0) {
        writeLog("entry_hook_refused", "target=%p reason=not_listed", target);
        return 0;
    }
    const EntryHookSite *s = &ENTRY_HOOK_SITES[i];
    unsigned char *at = gameBase + s->rva;
    if (!s->size) {
        writeLog("entry_hook_refused", "name=%s reason=not_on_this_build", s->name);
        return 0;
    }
    if (!gameCodeMatches) {
        writeLog("entry_hook_refused", "name=%s rva=0x%x reason=game_code_differs", s->name, s->rva);
        return 0;
    }
    if (entryHooks[i].enabled) {
        writeLog("entry_hook_refused", "name=%s rva=0x%x reason=already_hooked", s->name, s->rva);
        return 0;
    }
    _Static_assert(sizeof entryHooks[0].saved == 16, "a hook replaces at most 16 bytes");
    memcpy(entryHooks[i].saved, at, (size_t)s->size);
#ifndef __APPLE__
    if (!entryHookPage) {
        if (!(entryHookPage = allocNearBytes(gameBase + RVA_TEXT_SECTION, ENTRY_HOOK_COUNT * ENTRY_HOOK_SLOT))) {
            writeLog("entry_hook_refused", "name=%s reason=no_near_page", s->name);
            return 0;
        }
        makePagesReadExecute(entryHookPage, ENTRY_HOOK_COUNT * ENTRY_HOOK_SLOT);
    }
    unsigned char *t = entryHookPage + i * ENTRY_HOOK_SLOT, *stub = t + ENTRY_HOOK_STUB, slot[ENTRY_HOOK_SLOT];
    CodeFixup fixups[2];
    for (int k = 0; k < s->fixupCount; k++) fixups[k] = (CodeFixup){s->fixups[k].at, gameBase + s->fixups[k].target};
    if (!buildEntryHookSlot(slot, t, at, entryHooks[i].saved, s->size, fixups, s->fixupCount, detour)) {
        writeLog("entry_hook_refused", "name=%s reason=fixup_out_of_reach", s->name);
        return 0;
    }
    if (!writeGameCode(t, slot, ENTRY_HOOK_STUB + 14)) {
        writeLog("entry_hook_refused", "name=%s reason=trampoline_unwritable", s->name);
        return 0;
    }
    entryHooks[i].trampoline = t;
    entryHooks[i].stub = stub;
#else
    unsigned char *t = takePadding(32);
    if (!t) {
        writeLog("entry_hook_refused", "name=%s reason=no_trampoline_room", s->name);
        return 0;
    }
    uint32_t code[8];
    memcpy(code, entryHooks[i].saved, 16);
    code[4] = 0x58000050u;
    code[5] = 0xd61f0200u;
    unsigned char *back = at + 16;
    memcpy(&code[6], &back, 8);
    if (!writeGameCode(t, code, sizeof code)) {
        writeLog("entry_hook_refused", "name=%s reason=trampoline_unwritable", s->name);
        return 0;
    }
    entryHooks[i].trampoline = t;
#endif
    entryHooks[i].detour = detour;
    *original = entryHooks[i].trampoline;
    return 1;
}

int fanpatchHookEnable(void *target, int enable) {
    int i = entryHookAt(target);
    if (i < 0 || !entryHooks[i].trampoline) return 0;
    const EntryHookSite *s = &ENTRY_HOOK_SITES[i];
    unsigned char *at = gameBase + s->rva, code[16];
    if (!enable) {
        memcpy(code, entryHooks[i].saved, (size_t)s->size);
    } else {
#ifndef __APPLE__
        buildEntryHookJump(code, at, entryHooks[i].stub, s->size);
#else
        uint32_t jump[2] = {0x58000050u, 0xd61f0200u};
        memcpy(code, jump, 8);
        memcpy(code + 8, &entryHooks[i].detour, 8);
#endif
    }
    int ok = writeGameCode(at, code, (size_t)s->size);
    if (ok) entryHooks[i].enabled = enable;
    writeLog("entry_hook_set", "name=%s rva=0x%x on=%d ok=%d", s->name, s->rva, enable, ok);
    return ok;
}

int fanpatchHookRemove(void *target) {
    int i = entryHookAt(target);
    if (i < 0) return 0;
    int ok = !entryHooks[i].enabled || fanpatchHookEnable(target, 0);
    if (ok) entryHooks[i].trampoline = NULL;
    return ok;
}

unsigned char *fanpatchGameBase(void) { return gameBase; }

void fanpatchLog(const char *fmt, ...) {
    char text[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    writeLog("fanpatch_log", "text=%s", text);
}

// Whether the game's code is the build's, which every function a fix hooks or reads was checked on.
int fanpatchGameCodeMatches(const char *fix) {
    if (gameCodeMatches) return 1;
    writeLog("fanpatch_check_failed", "fix=%s reason=game_code_differs", fix);
    return 0;
}

unsigned char *fanpatchItemTable(void) { return tableAddress(TABLE_ITEM); }
int fanpatchItemRecords(void) { return tableRecords(TABLE_ITEM); }
int fanpatchNetworkGame(void) { return isNetworkGame(); }

int fanpatchSpellAboveLevel(int unit) {
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return 0;
    typedef int (*UnitPathLevelFn)(int unit, int path);
    const short *memorized = (const short *)GAME_PTR(RVA_UNIT_MEMORIZED_SPELLS + (size_t)unit * UNIT_SIZE);
    const unsigned char *table = spellTableAddress();
    for (int i = 0; i < UNIT_MEMORIZED_SPELLS_MAX && memorized[i] >= 0; i++) {
        if (memorized[i] >= spellTableRecords()) continue;
        const unsigned char *spell = table + (size_t)memorized[i] * SPELL_SIZE;
        int path = *(const int *)(spell + SPELL_PATH);
        if (path >= 0 && *(const int *)(spell + SPELL_LEVEL) > GAME_FN(UnitPathLevelFn, RVA_UNIT_PATH_LEVEL)(unit, path)) return 1;
    }
    return 0;
}

int fanpatchReplacedSprite(unsigned file, unsigned spr) {
    if (file >= SPRITE_KIND_COUNT || spr >= SPRITES_PER_KIND) return -1;
    const int *const *lists = (const int *const *)GAME_PTR(RVA_SPRITE_REPLACED);
    return lists[file] ? lists[file][spr] : -1;
}


static void fanpatchReport(void) {
    fix_item_reset_report();
    fix_feeblemind_report();
    fix_token_sprites_report();
}

// Installs the three fixes, each logging what it did ("fanpatch_log").
static void fanpatchInstall(void) {
    int installed = fix_item_reset_install() + fix_feeblemind_install() + fix_token_sprites_install();
    writeLog("fanpatch_installed", "fixes=%d of 3", installed);
    atexit(fanpatchReport);
}
