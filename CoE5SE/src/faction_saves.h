// Mods' factions in saves: which factions a save holds, their slots' data, and loads without them refused.
#pragma once

#define SAVE_FCTN_VERSION   1
#define SAVE_FCDT_VERSION   1
#define FACTION_SQUARE_BYTES 7

// The moved per-player tables, each slot's entry in them, as FCDT holds them.
static const struct { unsigned int rva; int entry; } FACTION_SLOT_TABLES[] = {
    {RVA_SLOT_NAME, PLAYER_BLOCK_SIZE}, {RVA_SLOT_CONTROLLER, 2}, {RVA_SLOT_CLASS, 2}, {RVA_PLAYER_HOME_X, 2},
    {RVA_PLAYER_HOME_Y, 2}, {RVA_PLAYER_COLOURS, 12}, {RVA_PLAYER_PLANE_SEEN, 11},
};
#define FACTION_SLOT_TABLE_COUNT (int)(sizeof FACTION_SLOT_TABLES / sizeof FACTION_SLOT_TABLES[0])

// Whether this world holds a faction, so its saves carry them.
static int factionsInWorld(void) { return factionCount > 0 && factionSlotsInUse(); }

// The save magic's hooks, which a faction's save needs as a plane's does (custom_planes.h); 1 when in.
static int installSaveMagicHooks(void);
static int ensureFactionSaveHooks(void) { return installSaveMagicHooks(); }

// The world's squares the per-square arrays are saved over: RVA_MAP_DEPTH by RVA_MAP_H, within the arrays.
static void factionSaveArea(int *w, int *h) {
    *w = G32(RVA_MAP_DEPTH) < MAP_COLUMNS_MAX ? G32(RVA_MAP_DEPTH) : MAP_COLUMNS_MAX;
    *h = G32(RVA_MAP_H) < MAP_COLUMN_SQUARES ? G32(RVA_MAP_H) : MAP_COLUMN_SQUARES;
    if (*w < 0) *w = 0;
    if (*h < 0) *h = 0;
}

// ---- Writing ------------------------------------------------------------------------------------
// Adds FCDT for each faction, then FCTN.
static void addFactionChunks(SaveList *list) {
    if (!factionsInWorld()) return;
    int w, h;
    factionSaveArea(&w, &h);
    for (int i = 0; i < factionCount; i++) {
        int slot = factionSlotOf(&factions[i]);
        SaveBuffer b = {0};
        saveBufferInt(&b, SAVE_FCDT_VERSION);
        saveBufferInt(&b, slot);
        saveBufferString(&b, factions[i].name);
        for (int t = 0; t < FACTION_SLOT_TABLE_COUNT; t++) {
            saveBufferInt(&b, FACTION_SLOT_TABLES[t].entry);
            saveBufferAdd(&b, playerSlotAddress(FACTION_SLOT_TABLES[t].rva, slot, FACTION_SLOT_TABLES[t].entry), (size_t)FACTION_SLOT_TABLES[t].entry);
        }
        saveFactionTurnState(&b, slot);
        int squares = factionSquareSeen ? w * h : 0;
        saveBufferInt(&b, squares ? w : 0);
        saveBufferInt(&b, squares ? h : 0);
        for (int x = 0; x < w && squares; x++)
            for (int y = 0; y < h; y++) {
                size_t at = factionSquareIndex(x, y, slot);
                unsigned char bytes[3] = {factionSquareSeen[at], factionSquareSeenMark[at], factionSquareTrueSeen[at]};
                saveBufferAdd(&b, bytes, 3);
                saveBufferInt(&b, factionSquareAiStrength[at]);
            }
        saveListAdd(list, SAVE_TAG_FCDT, &b);
    }
    SaveBuffer b = {0};
    saveBufferInt(&b, SAVE_FCTN_VERSION);
    saveBufferInt(&b, factionCount);
    for (int i = 0; i < factionCount; i++) {
        saveBufferInt(&b, factionSlotOf(&factions[i]));
        saveBufferString(&b, factions[i].name);
        saveBufferString(&b, factions[i].mod);
    }
    saveBufferInt(&b, (int)b.size + 8);
    saveBufferInt(&b, SAVE_TAG_FCTN);
    saveListAdd(list, SAVE_TAG_FCTN, &b);
}

// ---- Loading ------------------------------------------------------------------------------------
// The save being loaded: its factions by slot, until the load's mod pass gives them their slots.
static struct {
    int present, accepted, pending, count;
    int slots[FACTION_SLOT_TARGET];
    char names[FACTION_SLOT_TARGET][FACTION_NAME_SIZE];
    char mods[FACTION_SLOT_TARGET][MOD_NAME_MAX];
} loadedSaveFactions;

// Reads an FCTN payload into loadedSaveFactions and decides whether the load may go on; the result word.
static const char *useFactionIndex(const unsigned char *payload, int size, char *missing, size_t missingSize) {
    SaveCursor c = {payload, payload + size, 0};
    int version = saveCursorInt(&c), count = saveCursorInt(&c), absent = 0, waiting = 0;
    missing[0] = '\0';
    if (c.failed || version != SAVE_FCTN_VERSION || count < 1 || count > FACTION_SLOT_TARGET) return "bad_chunk";
    for (int k = 0; k < count; k++) {
        char name[SAVE_STRING_MAX + 1], mod[SAVE_STRING_MAX + 1];
        int slot = saveCursorInt(&c);
        saveCursorString(&c, name, sizeof name);
        saveCursorString(&c, mod, sizeof mod);
        if (c.failed || slot != PLAYER_SLOTS_GAME + k || strlen(name) >= FACTION_NAME_SIZE || strlen(mod) >= MOD_NAME_MAX) return "bad_chunk";
        loadedSaveFactions.slots[k] = slot;
        snprintf(loadedSaveFactions.names[k], sizeof loadedSaveFactions.names[k], "%s", name);
        snprintf(loadedSaveFactions.mods[k], sizeof loadedSaveFactions.mods[k], "%s", mod);
        if (findFaction(name)) continue;
        if (loadTurnsModOn(mod)) waiting++;
        else absent++, addMissingPlaneName(missing, missingSize, name, mod);
    }
    loadedSaveFactions.count = count;
    if (absent) return "faction_not_declared";
    if (!ensureFactionSlots()) return "slots_not_ready";
    if (!ensureFactionSaveHooks()) return "hooks_not_installed";
    (void)waiting;
    return "accepted";
}

// Reads FCTN from the end of the save at `path`, when it has one, and decides whether it loads.
static void readLoadedSaveFactions(const char *path) {
    memset(&loadedSaveFactions, 0, sizeof loadedSaveFactions);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    int ints[3] = {0}, size = 0;
    long end = !fseek(f, 0, SEEK_END) ? ftell(f) : -1;
    unsigned char *payload = NULL;
    const char *result = "none";
    if (end >= 28 && !fseek(f, end - 12, SEEK_SET)) {
        for (int i = 0; i < 3; i++) readFileInt(f, &ints[i]);
        if (ints[0] == SAVE_TAG_END && !ints[1] && ints[2] == SAVE_LIST_MAGIC) {
            long at = end - 12;
            for (int step = 0; step < 4 && !payload; step++) {
                int trailer[2] = {0};
                if (at < 16 || fseek(f, at - 8, SEEK_SET) || !readFileInt(f, &trailer[0]) || !readFileInt(f, &trailer[1])) break;
                if (trailer[1] == SAVE_TAG_FCTN) payload = readTrailedChunk(f, at, SAVE_TAG_FCTN, &size, &result);
                else if (trailer[1] == SAVE_TAG_MODC || trailer[1] == SAVE_TAG_MODH || trailer[1] == SAVE_TAG_PLNS) at -= trailer[0] + 8;
                else break;
            }
        }
    }
    fclose(f);
    if (!payload) {
        if (strcmp(result, "none")) writeLog("save_factions_read", "result=%s", result);
        return;
    }
    char missing[512];
    const char *word = useFactionIndex(payload, size, missing, sizeof missing);
    free(payload);
    loadedSaveFactions.present = 1;
    loadedSaveFactions.accepted = !strcmp(word, "accepted");
    loadedSaveFactions.pending = loadedSaveFactions.accepted;
    if (loadedSaveFactions.accepted) writeLog("save_factions_read", "result=accepted factions=%d", loadedSaveFactions.count);
    else writeLog("save_factions_refused", "reason=%s factions=%d missing=%s", word, loadedSaveFactions.count, missing[0] ? missing : "-");
}

// A load's pass: the save's factions at their saved places first.
static void placeSavedFactions(void) {
    if (!loadedSaveFactions.pending) return;
    Faction placed[FACTION_SLOT_TARGET];
    int used[FACTION_SLOT_TARGET] = {0}, n = 0;
    for (int k = 0; k < loadedSaveFactions.count; k++) {
        Faction *f = findFaction(loadedSaveFactions.names[k]);
        if (f) used[f - factions] = 1, placed[n] = *f;
        else {
            memset(&placed[n], 0, sizeof placed[n]);
            snprintf(placed[n].name, sizeof placed[n].name, "%s", loadedSaveFactions.names[k]);
            placed[n].like = FACTION_LIKE_DEFAULT, placed[n].fromPass = FACTION_EARLIER_PASS;
        }
        snprintf(placed[n++].mod, sizeof placed[0].mod, "%s", loadedSaveFactions.mods[k]);
    }
    for (int i = 0; i < factionCount && n < FACTION_SLOT_TARGET; i++)
        if (!used[i]) placed[n++] = factions[i];
    memcpy(factions, placed, (size_t)n * sizeof factions[0]);
    factionCount = n;
}

// What the magic check may do with the save being loaded: -1 it has no FCTN, 0 refused, 1 accepted.
static int loadedSaveFactionsDecision(void) { return !loadedSaveFactions.present ? -1 : loadedSaveFactions.accepted; }

// Puts the factions in the order their saved slots give: the save's first, each at its slot, then the rest.
static int orderFactionsAsSaved(char *missing, size_t missingSize) {
    Faction ordered[FACTION_SLOT_TARGET];
    int used[FACTION_SLOT_TARGET] = {0}, n = 0, absent = 0;
    missing[0] = '\0';
    for (int k = 0; k < loadedSaveFactions.count; k++) {
        Faction *f = findFaction(loadedSaveFactions.names[k]);
        if (!f || f->fromPass == FACTION_EARLIER_PASS) {
            absent++;
            addMissingPlaneName(missing, missingSize, loadedSaveFactions.names[k], loadedSaveFactions.mods[k]);
            continue;
        }
        used[f - factions] = 1;
        ordered[n++] = *f;
    }
    if (absent) return 0;
    for (int i = 0; i < factionCount; i++)
        if (!used[i]) ordered[n++] = factions[i];
    memcpy(factions, ordered, (size_t)n * sizeof factions[0]);
    factionCount = n;
    return 1;
}

// After the load's mod pass: each saved faction takes its saved slot.
static void applyLoadedSaveFactions(int level) {
    if (level != 2 || !loadedSaveFactions.pending) return;
    loadedSaveFactions.pending = 0;
    char missing[512];
    if (orderFactionsAsSaved(missing, sizeof missing)) {
        if (playerTableMoved) setUpFactionSlots();
        writeLog("save_factions_given", "factions=%d declared=%d", loadedSaveFactions.count, factionCount);
        return;
    }
    writeLog("save_factions_refused", "reason=faction_not_declared_after_pass factions=%d missing=%s", loadedSaveFactions.count, missing);
    static char text[POPUP_TEXT_SIZE];
    snprintf(text, sizeof text, "Factions missing from this save's mods:\n%s\n", missing);
    okBox("CoE5SE: Not a proper save file", text);
    GAME_FN(void (*)(const char *), RVA_EXIT_GAME)("");
}

// Puts one FCDT back: the slot's table entries, its turn state and its per-square arrays; the result word.
static const char *useFactionDataChunk(const SaveChunk *chunk) {
    SaveCursor c = {chunk->payload, chunk->payload + chunk->size, 0};
    char name[SAVE_STRING_MAX + 1];
    int version = saveCursorInt(&c), slot = saveCursorInt(&c);
    saveCursorString(&c, name, sizeof name);
    if (c.failed || version != SAVE_FCDT_VERSION) return "bad_chunk";
    Faction *f = findFaction(name);
    if (!f || factionSlotOf(f) != slot || !factionSlot(slot)) return "slot_differs";
    for (int t = 0; t < FACTION_SLOT_TABLE_COUNT; t++) {
        int entry = saveCursorInt(&c);
        if (c.failed || entry != FACTION_SLOT_TABLES[t].entry || c.end - c.at < entry) return "bad_chunk";
        memcpy(playerSlotAddress(FACTION_SLOT_TABLES[t].rva, slot, entry), c.at, (size_t)entry);
        c.at += entry;
    }
    if (!readFactionTurnState(&c, slot)) return "bad_chunk";
    int w = saveCursorInt(&c), h = saveCursorInt(&c);
    if (c.failed || w < 0 || h < 0 || w > MAP_COLUMNS_MAX || h > MAP_COLUMN_SQUARES ||
        c.end - c.at != (ptrdiff_t)w * h * FACTION_SQUARE_BYTES || (w > 0 && h > 0 && !factionSquareSeen))
        return "bad_chunk";
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            size_t at = factionSquareIndex(x, y, slot);
            factionSquareSeen[at] = c.at[0], factionSquareSeenMark[at] = c.at[1], factionSquareTrueSeen[at] = c.at[2];
            c.at += 3;
            factionSquareAiStrength[at] = saveCursorInt(&c);
        }
    return c.failed ? "bad_chunk" : "given";
}

// The tail's FCDT chunks, read at a save's load; logs one line.
static void useFactionDataChunks(const SaveList *list) {
    int given = 0, refused = 0;
    for (int i = 0; i < list->count; i++) {
        if (list->chunks[i].tag != SAVE_TAG_FCDT) continue;
        const char *word = useFactionDataChunk(&list->chunks[i]);
        if (!strcmp(word, "given")) given++;
        else refused++, writeLog("save_faction_data_refused", "reason=%s", word);
    }
    if (given || refused) writeLog("save_faction_data_read", "given=%d refused=%d", given, refused);
}
