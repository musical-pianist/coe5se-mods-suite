// The spell table move: SPELL_TABLE_RECORDS_MOVED records instead of the game's 1500.
#include <stdint.h>
#ifndef __APPLE__
#include "spell_table_sites.h"
#else
#include "spell_table_sites_mac.h"
#endif

#define SPELL_TABLE_RECORDS_GAME   1500
#define MOD_SPELL_TARGET           3000
#define SPELL_SLOT_BOUND_MOVED     (VANILLA_SPELL_COUNT + MOD_SPELL_TARGET)
#define SPELL_TABLE_RECORDS_MOVED  (SPELL_SLOT_BOUND_MOVED + 1)
_Static_assert(SPELL_SLOT_BOUND_MOVED <= 0xfff, "the Mac's bounds hold it in a compare's 12 bits");
_Static_assert(SPELL_SLOT_BOUND_MOVED <= 10000, "the unit screen's spell row ids, spell + 10000, hold every slot");

static int spellTableMoved;
static unsigned char *movedSpellTable;

// Where the game reads the spell table now, and its records.
static unsigned char *spellTableAddress(void) { return spellTableMoved ? movedSpellTable : gameBase + RVA_SPELL_TABLE; }
static int spellTableRecords(void) { return spellTableMoved ? SPELL_TABLE_RECORDS_MOVED : SPELL_TABLE_RECORDS_GAME; }

static unsigned char *oldSpellTableStart, *oldSpellTableEnd;

#ifndef __APPLE__
// ---- Windows: the patch ------------------------------------------------------------------------
// A rewrite whose new value isn't a distance: the 4-byte field at rva + fieldOffset, and the new value.
typedef struct {
    unsigned int rva;
    int fieldOffset;
    int value;
    const char *name;
} SpellValueSite;

static const SpellValueSite SPELL_VALUE_SITES[] = {
    {RVA_UNMEMORIZE_SPELL_BOUND,   2, SPELL_SLOT_BOUND_MOVED, "unmemorize_spell"},
    {RVA_NET_RECV_MEMORIZED_BOUND, 2, SPELL_SLOT_BOUND_MOVED, "net_recv_memorized"},
};
#define SPELL_VALUE_SITE_COUNT ((int)(sizeof SPELL_VALUE_SITES / sizeof SPELL_VALUE_SITES[0]))
#define SPELL_MOVE_SITE_COUNT  (SPELL_TABLE_SITE_COUNT + SPELL_VALUE_SITE_COUNT)

static struct { unsigned char *at; int value, old; } spellMoveWrites[SPELL_MOVE_SITE_COUNT];
static int spellMoveWriteCount;

// Queues every site's new field, for the table at `table`.
static int queueSpellTableWrites(unsigned char *table) {
    long long delta = table - (gameBase + RVA_SPELL_TABLE);
    int matched = 0;
    spellMoveWriteCount = 0;
    for (int i = 0; i < SPELL_TABLE_SITE_COUNT; i++) {
        const SpellPatchSite *s = &SPELL_TABLE_SITES[i];
        unsigned char *at = gameBase + s->rva;
        int field;
        memcpy(&field, at + s->fieldOffset, 4);
        long long moved = field + delta;
        if (moved < INT32_MIN || moved > INT32_MAX) {
            writeLog("spell_table_site_mismatch", "group=table rva=0x%x out_of_reach=1", s->rva);
            continue;
        }
        spellMoveWrites[spellMoveWriteCount].at = at + s->fieldOffset;
        spellMoveWrites[spellMoveWriteCount++].value = (int)moved;
        matched++;
    }
    for (int i = 0; i < SPELL_VALUE_SITE_COUNT; i++) {
        const SpellValueSite *s = &SPELL_VALUE_SITES[i];
        spellMoveWrites[spellMoveWriteCount].at = gameBase + s->rva + s->fieldOffset;
        spellMoveWrites[spellMoveWriteCount++].value = s->value;
        matched++;
    }
    return matched;
}

// Logs a read or write of the old table before the game stops on it.
static LONG WINAPI logOldSpellTableAccess(EXCEPTION_POINTERS *e) {
    const EXCEPTION_RECORD *r = e->ExceptionRecord;
    if (r->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || r->NumberParameters < 2) return EXCEPTION_CONTINUE_SEARCH;
    const unsigned char *target = (const unsigned char *)r->ExceptionInformation[1];
    if (target < oldSpellTableStart || target >= oldSpellTableEnd) return EXCEPTION_CONTINUE_SEARCH;
    const unsigned char *at = r->ExceptionAddress;
    ptrdiff_t fromTable = target - (gameBase + RVA_SPELL_TABLE);
    writeLog("spell_old_table_access", "instruction=%p instruction_rva=0x%llx write=%d slot=%d field=0x%x", (const void *)at,
             (unsigned long long)(at - gameBase), (int)r->ExceptionInformation[0], (int)(fromTable / SPELL_SIZE),
             (unsigned)(fromTable % SPELL_SIZE));
    return EXCEPTION_CONTINUE_SEARCH;
}

// Makes the old table's whole pages unreadable.
static void protectOldSpellTable(void) {
    unsigned char *old = gameBase + RVA_SPELL_TABLE;
    oldSpellTableStart = (unsigned char *)(((uintptr_t)old + 0xfff) & ~(uintptr_t)0xfff);
    oldSpellTableEnd = (unsigned char *)(((uintptr_t)old + (size_t)SPELL_TABLE_RECORDS_GAME * SPELL_SIZE) & ~(uintptr_t)0xfff);
    DWORD previous;
    int guarded = VirtualProtect(oldSpellTableStart, (size_t)(oldSpellTableEnd - oldSpellTableStart), PAGE_NOACCESS, &previous) != 0;
    if (guarded) AddVectoredExceptionHandler(1, logOldSpellTableAccess);
    writeLog("spell_old_table_protected", "ok=%d from=%p to=%p error=%lu", guarded, (void *)oldSpellTableStart, (void *)oldSpellTableEnd,
             guarded ? 0UL : GetLastError());
}

// Moves the table, or changes nothing.
static int installSpellTableMove(void) {
    if (!gameCodeMatches) {
        writeLog("spell_table_not_moved", "reason=game_code_differs sites=0 of %d", SPELL_MOVE_SITE_COUNT);
        return 0;
    }
    size_t size = (size_t)SPELL_TABLE_RECORDS_MOVED * SPELL_SIZE;
    unsigned char *table = allocNearImage(size);
    if (!table) {
        writeLog("spell_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", size, SPELL_MOVE_SITE_COUNT);
        return 0;
    }
    int matched = queueSpellTableWrites(table);
    const char *reason = matched != SPELL_MOVE_SITE_COUNT ? "game_build_differs"
                         : !openGameCode(gameBase + RVA_TEXT_SECTION, (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION)) ? "code_unwritable" : NULL;
    if (reason) {
        writeLog("spell_table_not_moved", "reason=%s sites=%d of %d", reason, matched, SPELL_MOVE_SITE_COUNT);
        VirtualFree(table, 0, MEM_RELEASE);
        return 0;
    }
    memcpy(table, gameBase + RVA_SPELL_TABLE, (size_t)(VANILLA_SPELL_COUNT + 1) * SPELL_SIZE);
    for (int i = 0; i < spellMoveWriteCount; i++) {
        memcpy(&spellMoveWrites[i].old, spellMoveWrites[i].at, 4);
        if (writeGameCode(spellMoveWrites[i].at, &spellMoveWrites[i].value, 4)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(spellMoveWrites[j].at, &spellMoveWrites[j].old, 4);
        closeGameCode();
        writeLog("spell_table_not_moved", "reason=code_unwritable sites=%d of %d", matched, SPELL_MOVE_SITE_COUNT);
        VirtualFree(table, 0, MEM_RELEASE);
        return 0;
    }
    closeGameCode();
    movedSpellTable = table;
    spellTableMoved = 1;
    writeLog("spell_table_moved", "from=%p to=%p rva=0x%llx records=%d bound=%d sites=%d of %d", (void *)(gameBase + RVA_SPELL_TABLE),
             (void *)table, (unsigned long long)(table - gameBase), SPELL_TABLE_RECORDS_MOVED, SPELL_SLOT_BOUND_MOVED, matched,
             SPELL_MOVE_SITE_COUNT);
    protectOldSpellTable();
    return 1;
}
#else
// ---- Mac: the patch ----------------------------------------------------------------------------
// The Mac side: the ADRPs move their page, and every other change is one instruction word.
typedef struct { unsigned int rva; uint32_t mask, value; const char *name; } SpellWordSite;

// The bounds, each a cmp's 12-bit immediate.
#define SPELL_BOUND_MASK (0x1fffu << 10)
#define SPELL_WORD_SITE_COUNT 4
static const SpellWordSite SPELL_WORD_SITES[SPELL_WORD_SITE_COUNT] = {
    {RVA_UNMEMORIZE_SPELL_BOUND,   SPELL_BOUND_MASK, SPELL_SLOT_BOUND_MOVED << 10, "unmemorize_spell"},
    {RVA_UNMEMORIZE_SPELL_BOUND_2, SPELL_BOUND_MASK, SPELL_SLOT_BOUND_MOVED << 10, "unmemorize_spell_2"},
    {RVA_UNMEMORIZE_SPELL_BOUND_3, SPELL_BOUND_MASK, SPELL_SLOT_BOUND_MOVED << 10, "unmemorize_spell_3"},
    {RVA_NET_RECV_MEMORIZED_BOUND, SPELL_BOUND_MASK, (SPELL_SLOT_BOUND_MOVED - 1) << 10, "net_recv_memorized"},
};

#define SPELL_MOVE_SITE_COUNT (SPELL_MAC_TABLE_SITE_COUNT + SPELL_WORD_SITE_COUNT)
static struct { unsigned char *at; uint32_t word, old; } spellMoveWrites[SPELL_MOVE_SITE_COUNT];
static int spellMoveWriteCount;

static void queueSpellWord(unsigned int rva, uint32_t word) {
    spellMoveWrites[spellMoveWriteCount].at = gameBase + rva;
    spellMoveWrites[spellMoveWriteCount].word = word;
    memcpy(&spellMoveWrites[spellMoveWriteCount++].old, gameBase + rva, 4);
}

static uint32_t spellGameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

// Queues every site's new word, for the table at `table`.
static int queueSpellTableWrites(unsigned char *table) {
    long long pages = ((long long)(uintptr_t)table >> ADRP_PAGE_SHIFT) - ((long long)(uintptr_t)(gameBase + RVA_SPELL_TABLE) >> ADRP_PAGE_SHIFT);
    int matched = 0;
    spellMoveWriteCount = 0;
    for (int i = 0; i < SPELL_MAC_TABLE_SITE_COUNT; i++) {
        const SpellMacSite *s = &SPELL_MAC_TABLE_SITES[i];
        uint32_t moved = 0;
        if (!adrpMovedBy(spellGameWord(s->rva), pages, &moved)) {
            writeLog("spell_table_site_mismatch", "group=table va=0x%llx", s->rva + 0x100000000ULL);
            continue;
        }
        queueSpellWord(s->rva, moved);
        matched++;
    }
    for (int i = 0; i < SPELL_WORD_SITE_COUNT; i++) {
        const SpellWordSite *s = &SPELL_WORD_SITES[i];
        queueSpellWord(s->rva, patchedWord(spellGameWord(s->rva), s->mask, s->value));
        matched++;
    }
    return matched;
}

// Logs a read or write of the old table before the game stops on it.
static struct sigaction previousSpellSegv, previousSpellBus;
static void logOldSpellTableAccess(int signal, siginfo_t *info, void *context) {
    const unsigned char *target = info->si_addr;
    if (target >= oldSpellTableStart && target < oldSpellTableEnd) {
        const ucontext_t *uc = context;
        const unsigned char *at = (const unsigned char *)__darwin_arm_thread_state64_get_pc(uc->uc_mcontext->__ss);
        ptrdiff_t fromTable = target - (gameBase + RVA_SPELL_TABLE);
        writeLog("spell_old_table_access", "instruction=%p instruction_va=0x%llx slot=%d field=0x%x", (const void *)at,
                 (unsigned long long)(at - gameBase) + 0x100000000ULL, (int)(fromTable / SPELL_SIZE), (unsigned)(fromTable % SPELL_SIZE));
    }
    sigaction(signal, signal == SIGBUS ? &previousSpellBus : &previousSpellSegv, NULL);
}

// Makes the old table's whole 16 KB pages unreadable.
static void protectOldSpellTable(void) {
    unsigned char *old = gameBase + RVA_SPELL_TABLE;
    oldSpellTableStart = (unsigned char *)(((uintptr_t)old + MAC_PAGE_MASK) & ~(uintptr_t)MAC_PAGE_MASK);
    oldSpellTableEnd = (unsigned char *)(((uintptr_t)old + (size_t)SPELL_TABLE_RECORDS_GAME * SPELL_SIZE) & ~(uintptr_t)MAC_PAGE_MASK);
    kern_return_t error = mach_vm_protect(mach_task_self(), (mach_vm_address_t)oldSpellTableStart,
                                          (mach_vm_size_t)(oldSpellTableEnd - oldSpellTableStart), FALSE, VM_PROT_NONE);
    if (!error) {
        struct sigaction action;
        memset(&action, 0, sizeof action);
        action.sa_sigaction = logOldSpellTableAccess;
        action.sa_flags = SA_SIGINFO;
        sigemptyset(&action.sa_mask);
        sigaction(SIGSEGV, &action, &previousSpellSegv);
        sigaction(SIGBUS, &action, &previousSpellBus);
    }
    writeLog("spell_old_table_protected", "ok=%d from=%p to=%p error=%d", !error, (void *)oldSpellTableStart, (void *)oldSpellTableEnd, error);
}

// Moves the table, or changes nothing.
static int installSpellTableMove(void) {
    if (!gameCodeMatches) {
        writeLog("spell_table_not_moved", "reason=game_code_differs sites=0 of %d", SPELL_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *old = gameBase + RVA_SPELL_TABLE;
    size_t at = (uintptr_t)old & ADRP_PAGE_MASK;
    size_t total = (at + (size_t)SPELL_TABLE_RECORDS_MOVED * SPELL_SIZE + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    unsigned char *memory = allocAfterImage(total);
    if (!memory) {
        writeLog("spell_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", total, SPELL_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *table = memory + at;
    int matched = queueSpellTableWrites(table);
    if (matched != SPELL_MOVE_SITE_COUNT) {
        writeLog("spell_table_not_moved", "reason=game_build_differs sites=%d of %d", matched, SPELL_MOVE_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    memcpy(table, old, (size_t)(VANILLA_SPELL_COUNT + 1) * SPELL_SIZE);
    for (int i = 0; i < spellMoveWriteCount; i++) {
        if (writeGameCode(spellMoveWrites[i].at, &spellMoveWrites[i].word, 4)) continue;
        for (int j = i; j >= 0; j--) writeGameCode(spellMoveWrites[j].at, &spellMoveWrites[j].old, 4);
        writeLog("spell_table_not_moved", "reason=code_unwritable va=0x%llx sites=%d of %d",
                 (unsigned long long)(spellMoveWrites[i].at - gameBase) + 0x100000000ULL, matched, SPELL_MOVE_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    movedSpellTable = table;
    spellTableMoved = 1;
    writeLog("spell_table_moved", "from=%p to=%p va=0x%llx records=%d bound=%d sites=%d of %d", (void *)old, (void *)table,
             (unsigned long long)(table - gameBase) + 0x100000000ULL, SPELL_TABLE_RECORDS_MOVED, SPELL_SLOT_BOUND_MOVED, matched,
             SPELL_MOVE_SITE_COUNT);
    protectOldSpellTable();
    return 1;
}
#endif

// Moves the table.
static void spellTableMoveInit(void) {
    installSpellTableMove();
}
