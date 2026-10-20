// The names chunks of the save tail: what each of mods' spell slots held.

#define SAVE_TAG_MODS        SAVE_TAG('M', 'O', 'D', 'S')
#define SAVE_TAG_SPEL        SAVE_TAG('S', 'P', 'E', 'L')
#define SAVE_NAMES_VERSION   1
#define SAVE_SLOTS_MAX       SPELL_SLOT_BOUND_MOVED
#define SAVE_MODS_MAX        4096
#define UNIT_KNOWN_SPELLS_MAX      60
#define UNIT_MEMORIZED_SPELLS_MAX  30

static int isSaveNameTag(int tag) { return tag == SAVE_TAG_MODS || tag == SAVE_TAG_SPEL; }

static unsigned char lowerAscii(unsigned char ch) { return ch >= 'A' && ch <= 'Z' ? (unsigned char)(ch + 32) : ch; }

static int isSameNameIgnoringCase(const char *a, const char *b) {
    for (; *a && lowerAscii((unsigned char)*a) == lowerAscii((unsigned char)*b); a++, b++) {}
    return lowerAscii((unsigned char)*a) == lowerAscii((unsigned char)*b);
}

// ---- The spells now ---------------------------------------------------------------------------------
// The spell in slot `slot` now.
static int currentModSpell(int slot, const char **mod, const char **name) {
    int index = slot - VANILLA_SPELL_COUNT;
    if (index < 0 || index >= spellTable.modSpells) return 0;
    *mod = spellModName(spellTable.spellMods[index]);
    *name = spellSlot(slot)->name ? spellSlot(slot)->name : "";
    return 1;
}

// One run of the spells now: the slots one mod's spells fill in a row.
typedef struct { const char *mod; int first, count; } SpellRun;

// The runs of mods' spells now, in slot order, into `runs` (room for MOD_SPELL_CAP_MAX).
static int currentSpellRuns(SpellRun *runs) {
    int count = 0;
    for (int i = 0; i < spellTable.modSpells; i++) {
        const char *mod = spellModName(spellTable.spellMods[i]);
        if (count && !strcmp(runs[count - 1].mod, mod)) runs[count - 1].count++;
        else runs[count++] = (SpellRun){mod, VANILLA_SPELL_COUNT + i, 1};
    }
    return count;
}

// ---- Writing ------------------------------------------------------------------------------------
static void addSaveNameChunks(SaveList *list, SaveNamesWritten *written) {
    static SpellRun runs[MOD_SPELL_CAP_MAX];
    int runCount = currentSpellRuns(runs);
    if (!runCount) return;
    const char **mods = NULL;
    int modCount = 0, modCapacity = 0;
    int *refs = malloc((size_t)runCount * sizeof *refs);
    for (int r = 0; refs && r < runCount; r++) {
        refs[r] = -1;
        if (!strcmp(runs[r].mod, "-")) continue;
        for (int m = 0; m < modCount; m++)
            if (!strcmp(mods[m], runs[r].mod)) refs[r] = m;
        if (refs[r] >= 0) continue;
        if (!growList((void **)&mods, &modCapacity, modCount + 1, sizeof *mods)) {
            free(refs);
            refs = NULL;
            break;
        }
        mods[modCount] = runs[r].mod;
        refs[r] = modCount++;
    }
    if (!refs) {
        SaveBuffer none = {.failed = 1};
        saveListAdd(list, SAVE_TAG_MODS, &none);
        free(mods);
        return;
    }
    SaveBuffer b = {0};
    saveBufferInt(&b, SAVE_NAMES_VERSION);
    saveBufferInt(&b, modCount);
    for (int m = 0; m < modCount; m++) saveBufferString(&b, mods[m]);
    saveListAdd(list, SAVE_TAG_MODS, &b);
    written->mods = modCount;
    saveBufferInt(&b, SAVE_NAMES_VERSION);
    saveBufferInt(&b, VANILLA_SPELL_COUNT);
    saveBufferInt(&b, VANILLA_SPELL_COUNT + spellTable.modSpells - 1);
    saveBufferInt(&b, runCount);
    for (int r = 0; r < runCount; r++) {
        saveBufferInt(&b, refs[r]);
        saveBufferInt(&b, runs[r].first);
        saveBufferInt(&b, runs[r].count);
        for (int s = runs[r].first; s < runs[r].first + runs[r].count; s++) {
            const char *mod, *name = "";
            currentModSpell(s, &mod, &name);
            saveBufferString(&b, name);
        }
    }
    saveListAdd(list, SAVE_TAG_SPEL, &b);
    written->spells = spellTable.modSpells;
    free(refs);
    free(mods);
}

// ---- Reading ------------------------------------------------------------------------------------
// A run of a saved SPEL chunk.
typedef struct {
    int mod, first, count;
    char **names;
} SavedSpellRun;

// What a save or turn file said about its mods' spell slots.
typedef struct {
    char **mods;
    int modCount;
    int read;
    int first, last;
    SavedSpellRun *runs;
    int runCount;
} SavedSpells;

static void freeSavedSpells(SavedSpells *saved) {
    for (int i = 0; i < saved->modCount; i++) free(saved->mods[i]);
    free(saved->mods);
    for (int r = 0; r < saved->runCount; r++) free(saved->runs[r].names);
    free(saved->runs);
    *saved = (SavedSpells){0};
}

// Reads the MODS chunk.
static int readSavedMods(const SaveChunk *chunk, SavedSpells *saved) {
    SaveCursor c = {chunk->payload, chunk->payload + chunk->size, 0};
    int version = saveCursorInt(&c), count = saveCursorInt(&c);
    if (c.failed || version != SAVE_NAMES_VERSION || count < 0 || count > SAVE_MODS_MAX) return 0;
    saved->mods = calloc((size_t)count + 1, sizeof *saved->mods);
    if (!saved->mods) return 0;
    for (int i = 0; i < count; i++) {
        char name[SAVE_STRING_MAX + 1];
        saveCursorString(&c, name, sizeof name);
        if (c.failed || !(saved->mods[i] = malloc(strlen(name) + 1))) return 0;
        memcpy(saved->mods[i], name, strlen(name) + 1);
        saved->modCount = i + 1;
    }
    return 1;
}

// Reads `count` strings into run->names (one block).
static int readSavedNames(SaveCursor *c, SavedSpellRun *run) {
    SaveCursor scan = *c;
    size_t text = 0;
    for (int i = 0; i < run->count; i++) {
        if (scan.at >= scan.end || scan.end - scan.at - 1 < scan.at[0]) return 0;
        text += (size_t)scan.at[0] + 1;
        scan.at += 1 + scan.at[0];
    }
    run->names = malloc((size_t)run->count * sizeof *run->names + text + 1);
    if (!run->names) return 0;
    char *at = (char *)(run->names + run->count);
    for (int i = 0; i < run->count; i++) {
        size_t length = c->at[0];
        saveCursorString(c, at, length + 1);
        run->names[i] = at;
        at += length + 1;
    }
    return !c->failed;
}

// Reads the SPEL chunk.
static int readSavedSpells(const SaveChunk *chunk, SavedSpells *saved) {
    SaveCursor c = {chunk->payload, chunk->payload + chunk->size, 0};
    if (saveCursorInt(&c) != SAVE_NAMES_VERSION) return 0;
    saved->first = saveCursorInt(&c);
    saved->last = saveCursorInt(&c);
    int runCount = saveCursorInt(&c);
    if (c.failed || saved->first < 0 || saved->last < saved->first - 1 || saved->last >= SAVE_SLOTS_MAX || runCount < 0 ||
        runCount > saved->last - saved->first + 1)
        return 0;
    if (!(saved->runs = calloc((size_t)runCount + 1, sizeof *saved->runs))) return 0;
    int next = saved->first;
    for (int r = 0; r < runCount; r++) {
        SavedSpellRun *run = &saved->runs[r];
        run->mod = saveCursorInt(&c);
        run->first = saveCursorInt(&c);
        run->count = saveCursorInt(&c);
        if (c.failed || run->mod < -1 || run->mod >= saved->modCount || run->first < next || run->count < 1 ||
            run->first + (long long)run->count - 1 > saved->last)
            return 0;
        saved->runCount = r + 1;
        if (!readSavedNames(&c, run)) return 0;
        next = run->first + run->count;
    }
    return saved->read = 1;
}

// What the file said slot `slot` held.
static int savedSpellAt(const SavedSpells *saved, int slot, int *modRef, const char **name) {
    for (int r = 0; r < saved->runCount; r++) {
        const SavedSpellRun *run = &saved->runs[r];
        if (slot < run->first || slot >= run->first + run->count) continue;
        *modRef = run->mod;
        *name = run->names[slot - run->first];
        return 1;
    }
    return 0;
}

static const char *savedModName(const SavedSpells *saved, int modRef) { return modRef < 0 ? "-" : saved->mods[modRef]; }

// Whether the spell the file named in a slot (its mod and name) is the one there now.
static int isSameSpellNow(const char *savedMod, const char *savedName, int slot) {
    const char *mod, *name;
    return currentModSpell(slot, &mod, &name) && !strcmp(mod, savedMod) && isSameNameIgnoringCase(name, savedName);
}

// ---- The changed spells -------------------------------------------------------------------------
// A slot that units of the last save read hold, whose spell changed since the save was made.
typedef struct {
    int slot;
    int modOrder;
    char mod[MOD_NAME_MAX];
    char oldName[SAVE_STRING_MAX + 1];
    char newMod[MOD_NAME_MAX];
    char newName[SAVE_STRING_MAX + 1];
    int units, lastUnit;
} ChangedSpell;

static ChangedSpell *changedSpells;
static int changedSpellCount, changedSpellCapacity;

// Forgets the list: at the start of every save's read hook, and after the pop-up.
static void clearChangedSpells(void) { changedSpellCount = 0; }

static int changedSpellTotal(void) { return changedSpellCount; }

// Entry `index` of the list.
static const ChangedSpell *changedSpellAt(int index) {
    if (index < 0 || index >= changedSpellCount) return NULL;
    return &changedSpells[index];
}

static int isChangedSpellBefore(const ChangedSpell *a, const ChangedSpell *b) {
    return a->modOrder != b->modOrder ? a->modOrder < b->modOrder : a->slot < b->slot;
}

// Counts unit `unit` holding slot `slot` when its spell changed.
static void noteUnitSpell(const SavedSpells *saved, int unit, int slot, int modRef, const char *name) {
    const char *mod = savedModName(saved, modRef);
    if (isSameSpellNow(mod, name, slot)) return;
    for (int i = 0; i < changedSpellCount; i++) {
        ChangedSpell *c = &changedSpells[i];
        if (c->slot != slot) continue;
        if (c->lastUnit != unit) c->units++, c->lastUnit = unit;
        return;
    }
    ChangedSpell entry = {.slot = slot, .modOrder = modRef < 0 ? saved->modCount : modRef, .units = 1, .lastUnit = unit};
    snprintf(entry.mod, sizeof entry.mod, "%s", mod);
    snprintf(entry.oldName, sizeof entry.oldName, "%s", name);
    const char *newMod, *newName;
    if (currentModSpell(slot, &newMod, &newName)) {
        snprintf(entry.newMod, sizeof entry.newMod, "%s", newMod);
        snprintf(entry.newName, sizeof entry.newName, "%s", newName);
    }
    if (!growList((void **)&changedSpells, &changedSpellCapacity, changedSpellCount + 1, sizeof entry)) {
        writeLog("save_spells_changed_out_of_memory", "mod=%s slot=%d name=%s", entry.mod, slot, name);
        return;
    }
    int at = changedSpellCount;
    while (at > 0 && isChangedSpellBefore(&entry, &changedSpells[at - 1])) at--;
    memmove(&changedSpells[at + 1], &changedSpells[at], (size_t)(changedSpellCount - at) * sizeof entry);
    changedSpells[at] = entry;
    changedSpellCount++;
}

// Notes every mod spell slot in one of a unit's lists (`max` entries, ended at the first < 0).
static void noteUnitSpellList(const SavedSpells *saved, int unit, const short *list, int max) {
    for (int i = 0; i < max && list[i] >= 0; i++) {
        int modRef;
        const char *name;
        if (list[i] < VANILLA_SPELL_COUNT) continue;
        if (!savedSpellAt(saved, list[i], &modRef, &name)) continue;
        noteUnitSpell(saved, unit, list[i], modRef, name);
    }
}

// Collects the changed spells that the units of the save just read hold, reading memory only.
static void collectChangedSpells(const SavedSpells *saved) {
    for (int u = 0; u <= G32(RVA_UNIT_COUNT); u++) {
        size_t at = (size_t)u * UNIT_SIZE;
        if (*(const short *)GAME_PTR(RVA_UNIT_MONSTER + at) < 0) continue;
        noteUnitSpellList(saved, u, (const short *)GAME_PTR(RVA_UNIT_KNOWN_SPELLS + at), UNIT_KNOWN_SPELLS_MAX);
        noteUnitSpellList(saved, u, (const short *)GAME_PTR(RVA_UNIT_MEMORIZED_SPELLS + at), UNIT_MEMORIZED_SPELLS_MAX);
    }
}

static void logChangedSpells(void) {
    for (int i = 0; i < changedSpellCount; i++) {
        const ChangedSpell *c = &changedSpells[i];
        writeLog("save_spells_changed", "mod=%s old=%s new=%s new_mod=%s slot=%d units=%d", c->mod, c->oldName,
                 c->newName[0] ? c->newName : "-", c->newMod[0] ? c->newMod : "-", c->slot, c->units);
    }
}

// ---- Using a read list --------------------------------------------------------------------------
// The first chunk with tag `tag` in `list`, or NULL.
static const SaveChunk *findSaveChunk(const SaveList *list, int tag) {
    for (int i = 0; i < list->count; i++)
        if (list->chunks[i].tag == tag) return &list->chunks[i];
    return NULL;
}

// The slots of the file's runs whose spell isn't the one there now.
static int countChangedSlots(const SavedSpells *saved) {
    int changed = 0;
    for (int r = 0; r < saved->runCount; r++)
        for (int i = 0; i < saved->runs[r].count; i++)
            changed += !isSameSpellNow(savedModName(saved, saved->runs[r].mod), saved->runs[r].names[i], saved->runs[r].first + i);
    return changed;
}

// Reads MODS and SPEL and compares: "same" or "differs".
static const char *useSaveNameChunks(const SaveList *list, SaveFile file) {
    SavedSpells saved = {0};
    const SaveChunk *mods = findSaveChunk(list, SAVE_TAG_MODS), *spells = findSaveChunk(list, SAVE_TAG_SPEL);
    if (mods && !readSavedMods(mods, &saved)) skipSaveChunk(&lastSaveListRead, SAVE_TAG_MODS);
    if (spells && !readSavedSpells(spells, &saved)) skipSaveChunk(&lastSaveListRead, SAVE_TAG_SPEL);
    int changed = saved.read ? countChangedSlots(&saved) : 0;
    if (saved.read && file == SAVE_FILE_SAVE) {
        collectChangedSpells(&saved);
        logChangedSpells();
    }
    freeSavedSpells(&saved);
    return changed ? "differs" : "same";
}

// ---- DevTools ----------------------------------------------------------------------------------
// For DevTools' contentledger command: run `index` of the spell runs a save would name now.
__declspec(dllexport) int coe5seContentLedgerRun(int kind, int index, const char **kindName, const char **mod, int *first,
                                                 int *count, const char *const **names) {
    static SpellRun runs[MOD_SPELL_CAP_MAX];
    static const char *runNames[MOD_SPELL_CAP_MAX];
    static int built;
    if (kind != 0 || index < 0) return -1;
    if (!index) built = currentSpellRuns(runs);
    if (index >= built) return 0;
    const SpellRun *run = &runs[index];
    for (int i = 0; i < run->count; i++) {
        const char *runMod;
        runNames[i] = "";
        currentModSpell(run->first + i, &runMod, &runNames[i]);
    }
    *kindName = "spells";
    *mod = run->mod;
    *first = run->first;
    *count = run->count;
    *names = runNames;
    return 1;
}
