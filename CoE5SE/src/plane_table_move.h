// The plane table move: PLANE_RECORDS_MOVED plane records instead of the game's 11, for mods' planes.
#include <stdint.h>
#ifndef __APPLE__
#include "plane_table_sites.h"
#else
#include "plane_table_sites_mac.h"
#endif

#define PLANE_RECORDS_GAME       11
#define CUSTOM_PLANE_TARGET      8
#define PLANE_RECORDS_MOVED      (PLANE_RECORDS_GAME + CUSTOM_PLANE_TARGET)
#define PLANE_INDEX_LAST_MOVED   (PLANE_RECORDS_MOVED - 1)
#define PLANE_CAMERA_BYTES       (PLANE_RECORDS_MOVED * 4)
_Static_assert(PLANE_RECORDS_MOVED <= 0x7f, "the Windows count checks hold it in an imm8");

#define PLANE_GROUP_COUNT        (PLANE_GROUP_BG_SIZE + 1)
#define PLANE_BACKGROUND_FIRST   PLANE_GROUP_BG_BUILT

static int planeTableMoved;
static unsigned char *movedPlaneTable, *movedPlaneCameraX, *movedPlaneCameraY;
static unsigned char *movedPlaneBuilt;

// Each moved array: the game's address and one plane's entry size.
static const struct { unsigned int rva; int entry; } PLANE_ARRAYS[PLANE_GROUP_COUNT] = {
    [PLANE_GROUP_TABLE] = {RVA_PLANES, PLANE_SIZE},       [PLANE_GROUP_CAMERA_X] = {RVA_PLANE_CAMERA_X, 4},
    [PLANE_GROUP_CAMERA_Y] = {RVA_PLANE_CAMERA_Y, 4},     [PLANE_GROUP_BG_BUILT] = {RVA_MAP_BG_BUILT, 4},
    [PLANE_GROUP_BG_PIXELS] = {RVA_MAP_BG_PIXELS, 8},     [PLANE_GROUP_BG_DIRTY] = {RVA_MAP_BG_DIRTY, 4},
    [PLANE_GROUP_BG_SIZE] = {RVA_MAP_BG_SIZE, 8},
};

// A mod's planes' background images, rebuilt: the game clears only 0 to 10 when a game ends.
static void resetCustomPlaneImages(void) {
    if (!planeTableMoved) return;
    memset(movedPlaneBuilt + 4 * PLANE_RECORDS_GAME, 0, 4 * (PLANE_RECORDS_MOVED - PLANE_RECORDS_GAME));
}

// Where the game reads the plane table now, and how many records it holds.
static unsigned char *planeTableAddress(void) { return planeTableMoved ? movedPlaneTable : gameBase + RVA_PLANES; }
static int planeTableRecords(void) { return planeTableMoved ? PLANE_RECORDS_MOVED : PLANE_RECORDS_GAME; }
static unsigned char *planeRecord(int plane) { return planeTableAddress() + (size_t)plane * PLANE_SIZE; }
static int planeField(int plane, int field) {
    int value;
    memcpy(&value, planeRecord(plane) + field, 4);
    return value;
}
static void setPlaneField(int plane, int field, int value) { memcpy(planeRecord(plane) + field, &value, 4); }

// The old address of each site group's array.
static unsigned char *planeGroupOld(int group) { return gameBase + PLANE_ARRAYS[group].rva; }

// Copies the game's 11 entries of each array into its new place, and notes the new places.
static void finishPlaneArraysMove(unsigned char *const arrays[PLANE_GROUP_COUNT]) {
    for (int g = 0; g < PLANE_GROUP_COUNT; g++) memcpy(arrays[g], planeGroupOld(g), (size_t)PLANE_RECORDS_GAME * (size_t)PLANE_ARRAYS[g].entry);
    movedPlaneTable = arrays[PLANE_GROUP_TABLE], movedPlaneCameraX = arrays[PLANE_GROUP_CAMERA_X], movedPlaneCameraY = arrays[PLANE_GROUP_CAMERA_Y];
    movedPlaneBuilt = arrays[PLANE_GROUP_BG_BUILT];
}

#ifndef __APPLE__
// ---- Windows: the patch ------------------------------------------------------------------------
// A rewrite whose new value isn't a distance.
typedef struct {
    unsigned int rva;
    int size, fieldOffset, valueSize;
    unsigned char shipped[8];
    int value;
    const char *name;
} PlaneValueSite;

#define PLANE_KEY_NEXT_STOP_JUMP (RVA_MAP_VIEW_PLANE_LAST_STOP + 3)
static const PlaneValueSite PLANE_VALUE_SITES[] = {
    {RVA_PLANE_SAME_SPOT_CAP_CHECK,  3, 2, 1, {0x83, 0xfb, 0x0a},             PLANE_INDEX_LAST_MOVED, "plane_same_spot"},
    {RVA_RITUAL_PLANESWAP_DST_CHECK, 4, 3, 1, {0x49, 0x83, 0xfe, 0x0a},       PLANE_INDEX_LAST_MOVED, "planeswap_dst"},
    {RVA_VIEW_PLANE_CLAMP,           5, 1, 4, {0xbb, 0x0a, 0x00, 0x00, 0x00}, PLANE_INDEX_LAST_MOVED, "view_plane_clamp"},
    {RVA_MAP_DRAW_PLANE_CLAMP,       3, 2, 1, {0x83, 0xfa, 0x0a},             PLANE_INDEX_LAST_MOVED, "map_draw_clamp"},
    {RVA_MAP_BG_DIRTY_CHECK,         4, 3, 1, {0x41, 0x83, 0xf8, 0x0a},       PLANE_INDEX_LAST_MOVED, "bg_dirty_check"},
    {RVA_MAP_BG_DIRTY_CHECK_EDITOR,  4, 3, 1, {0x41, 0x83, 0xfb, 0x0b},       PLANE_RECORDS_MOVED,    "bg_dirty_check_editor"},
    {RVA_MAP_BG_MARK_CHECK,          3, 2, 1, {0x83, 0xfd, 0x0b},             PLANE_RECORDS_MOVED,    "bg_mark_check"},
    {PLANE_KEY_NEXT_STOP_JUMP,       2, 0, 1, {0x74, 0x2f},                   0x7d,                   "view_next_stop"},
};

// The top bar's plane slot width, (free - x - 20) times 1/11: one of 19 instead.
static const unsigned char PLANE_SLOT_WIDTH_SHIPPED[9] = {0xf3, 0x44, 0x0f, 0x59, 0x0d, 0xa0, 0x92, 0x36, 0x00};
#define PLANE_SLOT_WIDTH_SIZE   9
#define PLANE_SLOT_WIDTH_FIELD  5
#define PLANE_VALUE_SITE_COUNT ((int)(sizeof PLANE_VALUE_SITES / sizeof PLANE_VALUE_SITES[0]))
#define PLANE_MOVE_SITE_COUNT  (PLANE_TABLE_SITE_COUNT + PLANE_COUNT_SITE_COUNT + PLANE_VALUE_SITE_COUNT + 1)

static struct { unsigned char *at; int value, size; } planeMoveWrites[PLANE_MOVE_SITE_COUNT];
static int planeMoveWriteCount;

static void queuePlaneWrite(unsigned char *at, int value, int size) {
    planeMoveWrites[planeMoveWriteCount].at = at;
    planeMoveWrites[planeMoveWriteCount].value = value;
    planeMoveWrites[planeMoveWriteCount++].size = size;
}

// Checks every site against the game's shipped bytes and queues its new field.
static int queuePlaneTableWrites(unsigned char *const arrays[PLANE_GROUP_COUNT], unsigned char *slotScale) {
    int matched = 0;
    planeMoveWriteCount = 0;
    for (int i = 0; i < PLANE_TABLE_SITE_COUNT; i++) {
        const PlanePatchSite *s = &PLANE_TABLE_SITES[i];
        unsigned char *at = gameBase + s->rva;
        long long delta = arrays[s->group] - planeGroupOld(s->group);
        int field;
        memcpy(&field, at + s->fieldOffset, 4);
        long long moved = field + delta;
        if (memcmp(at, s->bytes, (size_t)s->fieldOffset + 4) || moved < INT32_MIN || moved > INT32_MAX) {
            writeLog("plane_table_site_mismatch", "group=%d rva=0x%x out_of_reach=%d", s->group, s->rva, moved < INT32_MIN || moved > INT32_MAX);
            continue;
        }
        queuePlaneWrite(at + s->fieldOffset, (int)moved, 4);
        matched++;
    }
    for (int i = 0; i < PLANE_COUNT_SITE_COUNT; i++) {
        const PlaneCountSite *s = &PLANE_COUNT_SITES[i];
        if (memcmp(gameBase + s->rva, s->bytes, 3)) {
            writeLog("plane_table_site_mismatch", "group=count rva=0x%x", s->rva);
            continue;
        }
        queuePlaneWrite(gameBase + s->rva + 2, PLANE_RECORDS_MOVED, 1);
        matched++;
    }
    for (int i = 0; i < PLANE_VALUE_SITE_COUNT; i++) {
        const PlaneValueSite *s = &PLANE_VALUE_SITES[i];
        if (memcmp(gameBase + s->rva, s->shipped, (size_t)s->size)) {
            writeLog("plane_table_site_mismatch", "group=value name=%s rva=0x%x", s->name, s->rva);
            continue;
        }
        queuePlaneWrite(gameBase + s->rva + s->fieldOffset, s->value, s->valueSize);
        matched++;
    }
    unsigned char *at = gameBase + RVA_TOP_BAR_PLANE_SLOT_WIDTH;
    long long rel = slotScale - (at + PLANE_SLOT_WIDTH_SIZE);
    if (memcmp(at, PLANE_SLOT_WIDTH_SHIPPED, PLANE_SLOT_WIDTH_SIZE) || rel < INT32_MIN || rel > INT32_MAX)
        writeLog("plane_table_site_mismatch", "group=value name=top_bar_slot_width rva=0x%x", RVA_TOP_BAR_PLANE_SLOT_WIDTH);
    else {
        queuePlaneWrite(at + PLANE_SLOT_WIDTH_FIELD, (int)rel, 4);
        matched++;
    }
    return matched;
}

// Moves the table, or changes nothing.
static const unsigned short PLANE_BLOCK_AT[PLANE_GROUP_COUNT] = {
    [PLANE_GROUP_TABLE] = 0, [PLANE_GROUP_CAMERA_X] = 0x600, [PLANE_GROUP_CAMERA_Y] = 0x680, [PLANE_GROUP_BG_BUILT] = 0x700,
    [PLANE_GROUP_BG_DIRTY] = 0x780, [PLANE_GROUP_BG_PIXELS] = 0x800, [PLANE_GROUP_BG_SIZE] = 0x900,
};
#define PLANE_SLOT_SCALE_AT 0xa00
_Static_assert(PLANE_RECORDS_MOVED * PLANE_SIZE <= 0x600 && PLANE_RECORDS_MOVED * 8 <= 0x100 && PLANE_RECORDS_MOVED * 4 <= 0x80,
               "the arrays don't overlap");
static int installPlaneTableMove(void) {
    size_t size = 0x1000;
    unsigned char *block = allocNearImage(size);
    if (!block) {
        writeLog("plane_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", size, PLANE_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *arrays[PLANE_GROUP_COUNT];
    for (int g = 0; g < PLANE_GROUP_COUNT; g++) arrays[g] = block + PLANE_BLOCK_AT[g];
    float slotScale = 1.0f / (float)PLANE_RECORDS_MOVED;
    memcpy(block + PLANE_SLOT_SCALE_AT, &slotScale, 4);
    unsigned char *table = arrays[PLANE_GROUP_TABLE];
    int matched = queuePlaneTableWrites(arrays, block + PLANE_SLOT_SCALE_AT);
    DWORD textProtection = 0;
    unsigned char *text = gameBase + RVA_TEXT_SECTION;
    size_t textSize = (size_t)(RVA_RDATA_SECTION - RVA_TEXT_SECTION);
    const char *reason = matched != PLANE_MOVE_SITE_COUNT ? "game_build_differs"
                         : !VirtualProtect(text, textSize, PAGE_EXECUTE_READWRITE, &textProtection) ? "code_unwritable" : NULL;
    if (reason) {
        writeLog("plane_table_not_moved", "reason=%s sites=%d of %d", reason, matched, PLANE_MOVE_SITE_COUNT);
        VirtualFree(block, 0, MEM_RELEASE);
        return 0;
    }
    finishPlaneArraysMove(arrays);
    for (int i = 0; i < planeMoveWriteCount; i++) memcpy(planeMoveWrites[i].at, &planeMoveWrites[i].value, (size_t)planeMoveWrites[i].size);
    VirtualProtect(text, textSize, textProtection, &textProtection);
    FlushInstructionCache(GetCurrentProcess(), text, textSize);
    planeTableMoved = 1;
    writeLog("plane_table_moved", "from=%p to=%p rva=0x%llx records=%d sites=%d of %d", (void *)(gameBase + RVA_PLANES), (void *)table,
             (unsigned long long)(table - gameBase), PLANE_RECORDS_MOVED, matched, PLANE_MOVE_SITE_COUNT);
    return 1;
}
#else
// ---- Mac: the patch ----------------------------------------------------------------------------
// The Mac side: the ADRPs move their page, and every other change is one instruction word.
typedef struct { unsigned int rva; uint32_t shipped, word; const char *name; } PlaneWordSite;

#define PLANE_CMP_IMM(word, value) (((word) & ~(0xfffu << 10)) | (uint32_t)(value) << 10)
#define PLANE_SHARED_ADRP_NOP  0x100516260
#define PLANE_MOVZ_W(rd, imm16)   (0x52800000u | (uint32_t)(imm16) << 5 | (uint32_t)(rd))
#define PLANE_MOVK_W16(rd, imm16) (0x72a00000u | (uint32_t)(imm16) << 5 | (uint32_t)(rd))
#define PLANE_SLOT_OFFSET_BITS 0xbf86bca2u
#define PLANE_SLOT_SCALE_BITS  0x3d579436u
static const PlaneWordSite PLANE_WORD_SITES[] = {
    {RVA_ADDPLANE_CAP_CHECK,          0x71002b1fu, PLANE_CMP_IMM(0x71002b1fu, PLANE_INDEX_LAST_MOVED), "addplane_cap"},
    {RVA_PLANE_SAME_SPOT_CAP_CHECK,   0x7100281fu, PLANE_CMP_IMM(0x7100281fu, PLANE_INDEX_LAST_MOVED), "plane_same_spot"},
    {RVA_PLANE_SAME_SPOT_CAP_CHECK + 0x10, 0x7100281fu, PLANE_CMP_IMM(0x7100281fu, PLANE_INDEX_LAST_MOVED), "plane_same_spot_2"},
    {RVA_RITUAL_PLANESWAP_DST_CHECK,  0xf1002edfu, PLANE_CMP_IMM(0xf1002edfu, PLANE_RECORDS_MOVED), "planeswap_dst"},
    {MAC(PLANE_SHARED_ADRP_NOP),      0xd503201fu, 0x9008dfe8u, "shared_adrp_restore"},
    {RVA_VIEW_PLANE_CLAMP,            0x52800149u, PLANE_MOVZ_W(9, PLANE_INDEX_LAST_MOVED), "view_plane_clamp"},
    {RVA_VIEW_PLANE_CLAMP + 4,        0x7100291fu, PLANE_CMP_IMM(0x7100291fu, PLANE_INDEX_LAST_MOVED), "view_plane_clamp_cmp"},
    {RVA_MAP_BG_DIRTY_CHECK,          0x7100291fu, PLANE_CMP_IMM(0x7100291fu, PLANE_INDEX_LAST_MOVED), "bg_dirty_check"},
    {RVA_MAP_BG_DIRTY_CHECK_EDITOR,   0x7100291fu, PLANE_CMP_IMM(0x7100291fu, PLANE_INDEX_LAST_MOVED), "bg_dirty_check_editor"},
    {RVA_MAP_BG_MARK_CHECK,           0x51002c08u, PLANE_CMP_IMM(0x51002c08u, PLANE_RECORDS_MOVED), "bg_mark_check"},
    {RVA_MAP_BG_MARK_CHECK + 4,       0x3100311fu, PLANE_CMP_IMM(0x3100311fu, PLANE_RECORDS_MOVED + 1), "bg_mark_check_cmn"},
    {RVA_MAP_VIEW_PLANE_LAST_STOP + 4, 0x54000800u, 0x5400080au, "view_next_stop"},
    {RVA_TOP_BAR_PLANE_SLOT_WIDTH,        0x529745e8u, PLANE_MOVZ_W(8, PLANE_SLOT_OFFSET_BITS & 0xffffu), "slot_width_offset_lo"},
    {RVA_TOP_BAR_PLANE_SLOT_WIDTH + 4,    0x72b7fd08u, PLANE_MOVK_W16(8, PLANE_SLOT_OFFSET_BITS >> 16), "slot_width_offset_hi"},
    {RVA_TOP_BAR_PLANE_SLOT_WIDTH + 0xc,  0x5285d188u, PLANE_MOVZ_W(8, PLANE_SLOT_SCALE_BITS & 0xffffu), "slot_width_scale_lo"},
    {RVA_TOP_BAR_PLANE_SLOT_WIDTH + 0x10, 0x72a7b748u, PLANE_MOVK_W16(8, PLANE_SLOT_SCALE_BITS >> 16), "slot_width_scale_hi"},
};
#define PLANE_WORD_SITE_COUNT ((int)(sizeof PLANE_WORD_SITES / sizeof PLANE_WORD_SITES[0]))
#define PLANE_MOVE_SITE_COUNT (PLANE_MAC_TABLE_SITE_COUNT + PLANE_WORD_SITE_COUNT)

static struct { unsigned char *at; uint32_t word, old; } planeMoveWrites[PLANE_MOVE_SITE_COUNT];
static int planeMoveWriteCount;

static void queuePlaneWord(unsigned int rva, uint32_t word) {
    planeMoveWrites[planeMoveWriteCount].at = gameBase + rva;
    planeMoveWrites[planeMoveWriteCount].word = word;
    memcpy(&planeMoveWrites[planeMoveWriteCount++].old, gameBase + rva, 4);
}

static uint32_t planeGameWord(unsigned int rva) {
    uint32_t word;
    memcpy(&word, gameBase + rva, 4);
    return word;
}

// Checks every site against the game's shipped words and queues its new word.
static int queuePlaneTableWrites(unsigned char *const arrays[PLANE_GROUP_COUNT]) {
    int matched = 0;
    planeMoveWriteCount = 0;
    for (int i = 0; i < PLANE_MAC_TABLE_SITE_COUNT; i++) {
        const PlaneMacSite *s = &PLANE_MAC_TABLE_SITES[i];
        long long pages = ((long long)(uintptr_t)arrays[s->group] >> ADRP_PAGE_SHIFT) -
                          ((long long)(uintptr_t)planeGroupOld(s->group) >> ADRP_PAGE_SHIFT);
        uint32_t moved = 0;
        if (planeGameWord(s->rva) != s->word || !adrpMovedBy(s->word, pages, &moved)) {
            writeLog("plane_table_site_mismatch", "group=%d va=0x%llx", s->group, s->rva + 0x100000000ULL);
            continue;
        }
        queuePlaneWord(s->rva, moved);
        matched++;
    }
    for (int i = 0; i < PLANE_WORD_SITE_COUNT; i++) {
        const PlaneWordSite *s = &PLANE_WORD_SITES[i];
        if (planeGameWord(s->rva) != s->shipped) {
            writeLog("plane_table_site_mismatch", "group=value name=%s va=0x%llx", s->name, s->rva + 0x100000000ULL);
            continue;
        }
        queuePlaneWord(s->rva, s->word);
        matched++;
    }
    return matched;
}

// Writes the queued words, one protection change per run of words on the same page.
static int writePlaneWords(void) {
    int i = 0;
    while (i < planeMoveWriteCount) {
        uintptr_t page = (uintptr_t)planeMoveWrites[i].at & ~(uintptr_t)MAC_PAGE_MASK;
        int end = i;
        while (end < planeMoveWriteCount && ((uintptr_t)planeMoveWrites[end].at & ~(uintptr_t)MAC_PAGE_MASK) == page) end++;
        if (mach_vm_protect(mach_task_self(), page, MAC_PAGE_MASK + 1, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY)) return i;
        for (int k = i; k < end; k++) memcpy(planeMoveWrites[k].at, &planeMoveWrites[k].word, 4);
        if (mach_vm_protect(mach_task_self(), page, MAC_PAGE_MASK + 1, FALSE, VM_PROT_READ | VM_PROT_EXECUTE)) return i;
        sys_icache_invalidate((void *)page, MAC_PAGE_MASK + 1);
        i = end;
    }
    return i;
}

// Moves the table, or changes nothing.
static int installPlaneTableMove(void) {
    size_t total = ((size_t)PLANE_GROUP_COUNT * 0x1000 + MAC_PAGE_MASK) & ~(size_t)MAC_PAGE_MASK;
    unsigned char *memory = allocAfterImage(total);
    if (!memory) {
        writeLog("plane_table_not_moved", "reason=no_memory_near_game bytes=%zu sites=0 of %d", total, PLANE_MOVE_SITE_COUNT);
        return 0;
    }
    unsigned char *arrays[PLANE_GROUP_COUNT];
    for (int g = 0; g < PLANE_GROUP_COUNT; g++) {
        arrays[g] = memory + (size_t)g * 0x1000 + ((uintptr_t)planeGroupOld(g) & ADRP_PAGE_MASK);
        if (arrays[g] + (size_t)PLANE_RECORDS_MOVED * (size_t)PLANE_ARRAYS[g].entry > memory + (size_t)(g + 1) * 0x1000) {
            writeLog("plane_table_not_moved", "reason=array_crosses_page group=%d sites=0 of %d", g, PLANE_MOVE_SITE_COUNT);
            mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
            return 0;
        }
    }
    unsigned char *table = arrays[PLANE_GROUP_TABLE];
    int matched = queuePlaneTableWrites(arrays);
    if (matched != PLANE_MOVE_SITE_COUNT) {
        writeLog("plane_table_not_moved", "reason=game_build_differs sites=%d of %d", matched, PLANE_MOVE_SITE_COUNT);
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    unsigned char *moved[3] = {movedPlaneTable, movedPlaneCameraX, movedPlaneCameraY};
    unsigned char *movedBuilt = movedPlaneBuilt;
    finishPlaneArraysMove(arrays);
    int written = writePlaneWords();
    if (written != planeMoveWriteCount) {
        for (int j = 0; j < planeMoveWriteCount; j++) writeGameCode(planeMoveWrites[j].at, &planeMoveWrites[j].old, 4);
        writeLog("plane_table_not_moved", "reason=code_unwritable va=0x%llx sites=%d of %d",
                 (unsigned long long)(planeMoveWrites[written].at - gameBase) + 0x100000000ULL, matched, PLANE_MOVE_SITE_COUNT);
        movedPlaneTable = moved[0], movedPlaneCameraX = moved[1], movedPlaneCameraY = moved[2], movedPlaneBuilt = movedBuilt;
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)memory, total);
        return 0;
    }
    planeTableMoved = 1;
    writeLog("plane_table_moved", "from=%p to=%p va=0x%llx records=%d sites=%d of %d", (void *)(gameBase + RVA_PLANES), (void *)table,
             (unsigned long long)(table - gameBase) + 0x100000000ULL, PLANE_RECORDS_MOVED, matched, PLANE_MOVE_SITE_COUNT);
    return 1;
}
#endif

// Moves the table once per launch, at the first plane a mod declares.
static int ensurePlaneTableMoved(void) {
    static int tried;
    if (planeTableMoved || tried) return planeTableMoved;
    tried = 1;
    return installPlaneTableMove();
}
