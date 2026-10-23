// Mods' planes: up to CUSTOM_PLANE_TARGET new planes beside the game's, at indices CoE5SE hands out.

#define PLANE_FILES_DIR           "planes"
#define SAVE_MAGIC_GAME           0x6eb65
#define SAVE_MAGIC_CUSTOM_PLANES  SAVE_TAG('C', '5', 'P', 'L')
#define SAVE_PLNS_VERSION         1
#define PLANE_ROWS_MAX            MAP_COLUMN_SQUARES
#define PLANE_FIRST_HOST          1
#define PLANE_LAST_HOST           9

#define PLANE_SWAP_MAX            64

typedef struct {
    char name[PLANE_NAME_SIZE];
    char mod[MOD_NAME_MAX];
    int w, h, fill;
    int sizeFromLike;
    int index;
    int skipped;
    char icon[MAX_PATH];
    int recipe, populate;
    int swapCount;
    short swapFrom[PLANE_SWAP_MAX], swapTo[PLANE_SWAP_MAX];
    void *script;
    int generateRef;
    int fromWords;
    char file[MAX_PATH];
    PlaneLayout layout;
} CustomPlane;

#define PLANE_FROM_WORDS        1
#define PLANE_FROM_EARLIER_PASS 2

static CustomPlane customPlanes[CUSTOM_PLANE_TARGET];
static int customPlaneCount;
static int customPlaneHooksInstalled;
static int installCustomPlaneHooks(void);
static int installPlaneShapeHooks(void);
static void resolvePlaneShapes(void);
static void setPlaneGroundLikes(void);
static int installPlaneGroundHooks(void);
static int installCustomPlaneViewHooks(int *wanted);
static int installPlaneTintHooks(void);
static void buildCustomPlane(CustomPlane *p);
static void addCustomPlanesSpecialSquares(void);
static void populateCustomPlanes(void);
static void loadCustomPlaneFile(const CustomPlane *p);
static void resolvePlaneNames(int planesKnown);
static int vanillaPlaneNamed(const char *name);

static const char *const VANILLA_PLANE_NAMES[PLANE_RECORDS_GAME] = {
    "Elysium", "Celestial Plane", "Sky", "Agartha", "Inferno", "Hades", "Aztlan", "Primal Plane", "Elemental Planes", "Void", "Elysium2",
};

static CustomPlane *findCustomPlane(const char *name) {
    for (int i = 0; i < customPlaneCount; i++)
        if (!strcmp(customPlanes[i].name, name)) return &customPlanes[i];
    return NULL;
}

// The number of planes declared now.
static int declaredCustomPlaneCount(void) {
    int count = 0;
    for (int i = 0; i < customPlaneCount; i++) count += customPlanes[i].fromWords != PLANE_FROM_EARLIER_PASS;
    return count;
}

// Makes room for a new plane when the earlier pass's blocks fill the list.
static void dropEarlierPassPlane(void) {
    for (int i = customPlaneCount - 1; i >= 0; i--) {
        if (customPlanes[i].fromWords != PLANE_FROM_EARLIER_PASS) continue;
        writeLog("plane_dropped", "mod=%s name=%s reason=room_for_new_plane", customPlanes[i].mod, customPlanes[i].name);
        memmove(&customPlanes[i], &customPlanes[i + 1], (size_t)(customPlaneCount - i - 1) * sizeof customPlanes[0]);
        customPlaneCount--;
        resetCustomPlaneImages();
        return;
    }
}

// Declares a mod's plane; NULL when it is taken, or else the refusal's reason word.
static const char *declareCustomPlane(const char *mod, const char *name, int w, int h, int fill, const char *icon) {
    if (!name[0] || strlen(name) >= PLANE_NAME_SIZE) return "bad_name";
    for (int i = 0; i < PLANE_RECORDS_GAME; i++)
        if (!strcmp(VANILLA_PLANE_NAMES[i], name)) return "name_taken";
    if (vanillaPlaneNamed(name) >= 0) return "name_taken";
    CustomPlane *again = findCustomPlane(name);
    if (again && (again->fromWords != PLANE_FROM_EARLIER_PASS || strcmp(again->mod, mod))) return "name_taken";
    if (w < 1 || w >= MAP_COLUMNS_MAX || h < 1 || h >= PLANE_ROWS_MAX) return "bad_size";
    if (fill < 0) return "bad_terrain";
    if (!again && declaredCustomPlaneCount() >= CUSTOM_PLANE_TARGET) return "too_many_planes";
    if (!ensurePlaneTableMoved()) return "plane_table_not_moved";
    if (!customPlaneHooksInstalled && !(customPlaneHooksInstalled = installCustomPlaneHooks())) return "hooks_not_installed";
    if (!again && customPlaneCount >= CUSTOM_PLANE_TARGET) dropEarlierPassPlane();
    int index = again ? again->index : -1, skipped = again ? again->skipped : 0;
    void *script = again ? again->script : NULL;
    CustomPlane *p = again ? again : &customPlanes[customPlaneCount++];
    memset(p, 0, sizeof *p);
    snprintf(p->name, sizeof p->name, "%s", name);
    snprintf(p->mod, sizeof p->mod, "%s", mod);
    p->w = w, p->h = h, p->fill = fill, p->index = index, p->skipped = skipped, p->script = script;
    if (icon && strlen(icon) < sizeof p->icon) snprintf(p->icon, sizeof p->icon, "%s", icon);
    p->recipe = p->populate = -1;
    writeLog("plane_declared", "mod=%s name=%s w=%d h=%d fill=%d icon=%s", mod, name, w, h, fill, p->icon[0] ? p->icon : "-");
    return NULL;
}

// The index of a declared plane in this world, or -1.
static int customPlaneIndex(const char *name) {
    const CustomPlane *p = findCustomPlane(name);
    return p ? p->index : -1;
}

// ---- Placement ----------------------------------------------------------------------------------
// Fills a placed plane's squares with its terrain, then the game's ground and temperature passes.
typedef void (*SetRectFn)(int x, int y, int w, int h);
typedef void (*SetGroundRectFn)(int x, int y, int w, int h, int terrain);
static void fillCustomPlane(const CustomPlane *p, int x, int y) {
    int fill = p->fill;
    if (fill > G32(RVA_LAST_TERRAIN)) {
        writeLog("plane_fill_refused", "mod=%s name=%s fill=%d reason=no_such_terrain last_terrain=%d", p->mod, p->name, fill, G32(RVA_LAST_TERRAIN));
        fill = 0;
    }
    short terrain = (short)fill;
    for (int sx = x; sx < x + p->w; sx++)
        for (int sy = y; sy < y + p->h; sy++)
            memcpy(GAME_PTR(RVA_SQUARE_RECORDS) + ((size_t)sx * MAP_COLUMN_SQUARES + (size_t)sy) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE, &terrain, 2);
    GAME_FN(SetGroundRectFn, RVA_SET_GROUND_RECT)(x, y, p->w, p->h, -1);
    GAME_FN(SetRectFn, RVA_SET_TEMPERATURE_RECT)(x, y, p->w, p->h);
}

// Each mod's plane's first camera: its centre, as RVA_VIEW_PLANE fills planes 0 to 10.
static void setCustomPlaneCameras(void) {
    if (!planeTableMoved) return;
    for (int index = PLANE_ELYSIUM2 + 1; index <= G32(RVA_LAST_PLANE) && index < PLANE_RECORDS_MOVED; index++) {
        float x = (float)(planeField(index, PLANE_X) + planeField(index, PLANE_W) / 2);
        float y = (float)(planeField(index, PLANE_Y) + planeField(index, PLANE_H) / 2);
        memcpy(movedPlaneCameraX + 4 * index, &x, 4);
        memcpy(movedPlaneCameraY + 4 * index, &y, 4);
    }
}

// Writes record `index`: the plane's name and rectangle, the rest zero, as addplane leaves a record.
static void writeCustomPlaneRecord(int index, const char *name, int x, int y, int w, int h) {
    unsigned char *record = planeRecord(index);
    memset(record, 0, PLANE_SIZE);
    snprintf((char *)record, PLANE_NAME_SIZE, "%.31s", name);
    setPlaneField(index, PLANE_X, x);
    setPlaneField(index, PLANE_Y, y);
    setPlaneField(index, PLANE_W, w);
    setPlaneField(index, PLANE_H, h);
}

// Decides where each declared plane goes; returns the width of the planes beside the others.
static int planCustomPlanes(int placeX[], int placeY[], int host[]) {
    int elysium2X = planeField(PLANE_ELYSIUM2, PLANE_X), elysium2W = planeField(PLANE_ELYSIUM2, PLANE_W);
    int freeColumns = MAP_COLUMNS_MAX - (elysium2X + elysium2W), besideWidth = 0;
    int hostBottom[PLANE_LAST_HOST + 1] = {0};
    for (int k = PLANE_FIRST_HOST; k <= PLANE_LAST_HOST; k++) hostBottom[k] = planeField(k, PLANE_Y) + planeField(k, PLANE_H);
    for (int i = 0; i < customPlaneCount; i++) {
        CustomPlane *p = &customPlanes[i];
        host[i] = -1;
        if (p->w <= freeColumns - besideWidth) {
            placeX[i] = elysium2X + besideWidth, placeY[i] = 0;
            besideWidth += p->w;
            continue;
        }
        int best = -1;
        for (int k = PLANE_FIRST_HOST; k <= PLANE_LAST_HOST; k++) {
            int rows = PLANE_ROWS_MAX - hostBottom[k];
            if (planeField(k, PLANE_W) >= p->w && rows >= p->h && (best < 0 || rows < PLANE_ROWS_MAX - hostBottom[best])) best = k;
        }
        if (best < 0) {
            p->skipped = 1;
            placeX[i] = placeY[i] = -1;
            writeLog("plane_skipped", "mod=%s name=%s w=%d h=%d reason=no_room free_columns=%d", p->mod, p->name, p->w, p->h,
                     freeColumns - besideWidth);
            continue;
        }
        host[i] = best;
        placeX[i] = planeField(best, PLANE_X), placeY[i] = hostBottom[best];
        hostBottom[best] += p->h;
    }
    return besideWidth;
}

// The vanilla planes the game builds at Elysium's size.
#define PLANE_SKY      2
#define PLANE_AGARTHA  3
#define PLANE_INFERNO  4
#define PLANE_HADES    5
#define PLANE_INFERNO_FRAME 4
// The smallest size a plane declared without one gets.
#define PLANE_LIKE_SIZE_MIN 32

// What vanilla plane `like` adds to Elysium's size, or -1.
static int likePlaneSizeExtra(int like) {
    return like == PLANE_INFERNO ? PLANE_INFERNO_FRAME : like == PLANE_SKY || like == PLANE_AGARTHA || like == PLANE_HADES ? 0 : -1;
}

// Gives each plane declared without a size its like plane's size in this world.
static void sizeLikePlanes(void) {
    int elysiumW = planeField(0, PLANE_W), elysiumH = planeField(0, PLANE_H);
    for (int i = 0; i < customPlaneCount; i++) {
        CustomPlane *p = &customPlanes[i];
        int extra = p->sizeFromLike ? likePlaneSizeExtra(p->recipe) : -1;
        if (extra < 0) continue;
        int w = elysiumW + extra, h = elysiumH + extra;
        p->w = w < PLANE_LIKE_SIZE_MIN ? PLANE_LIKE_SIZE_MIN : w >= MAP_COLUMNS_MAX ? MAP_COLUMNS_MAX - 1 : w;
        p->h = h < PLANE_LIKE_SIZE_MIN ? PLANE_LIKE_SIZE_MIN : h >= PLANE_ROWS_MAX ? PLANE_ROWS_MAX - 1 : h;
        writeLog("plane_sized", "mod=%s name=%s like=%d w=%d h=%d elysium_w=%d elysium_h=%d", p->mod, p->name, p->recipe, p->w, p->h, elysiumW, elysiumH);
    }
}

// Adds every declared plane to the world the game just built.
static void placeCustomPlanes(void) {
    for (int i = 0; i < customPlaneCount; i++) customPlanes[i].index = -1, customPlanes[i].skipped = 0;
    if (!customPlaneCount) return;
    int last = G32(RVA_LAST_PLANE);
    if (last != PLANE_ELYSIUM2) {
        writeLog("planes_not_placed", "reason=elysium2_not_last last_plane=%d", last);
        for (int i = 0; i < customPlaneCount; i++) customPlanes[i].skipped = 1;
        return;
    }
    int elysium2X = planeField(PLANE_ELYSIUM2, PLANE_X);
    sizeLikePlanes();
    int placeX[CUSTOM_PLANE_TARGET], placeY[CUSTOM_PLANE_TARGET], host[CUSTOM_PLANE_TARGET];
    int besideWidth = planCustomPlanes(placeX, placeY, host);
    setPlaneField(PLANE_ELYSIUM2, PLANE_X, elysium2X + besideWidth);
    int mapH = G32(RVA_MAP_H);
    for (int i = 0; i < customPlaneCount; i++) {
        CustomPlane *p = &customPlanes[i];
        if (p->skipped) continue;
        int index = ++last;
        writeCustomPlaneRecord(index, p->name, placeX[i], placeY[i], p->w, p->h);
        p->index = index;
        if (placeY[i] + p->h > mapH) mapH = placeY[i] + p->h;
        writeLog("plane_placed", "mod=%s name=%s index=%d x=%d y=%d w=%d h=%d stacked_under=%d", p->mod, p->name, index, placeX[i],
                 placeY[i], p->w, p->h, host[i]);
    }
    G32(RVA_LAST_PLANE) = last;
    G32(RVA_MAP_W) += besideWidth;
    G32(RVA_MAP_H) = mapH;
    G32(RVA_MAP_DEPTH) = elysium2X + besideWidth;
    resolvePlaneShapes();
    setPlaneGroundLikes();
    resolvePlaneNames(1);
    for (int i = 0; i < customPlaneCount; i++) {
        if (customPlanes[i].skipped) continue;
        fillCustomPlane(&customPlanes[i], placeX[i], placeY[i]);
        if (customPlanes[i].file[0]) loadCustomPlaneFile(&customPlanes[i]);
        buildCustomPlane(&customPlanes[i]);
    }
    setCustomPlaneCameras();
    resetCustomPlaneImages();
}

// RVA_CREATE_EXTRA_PLANES(w, h), then the mods' planes.
typedef void (*CreateExtraPlanesFn)(int w, int h);
static void createExtraPlanesHook(int w, int h) {
    GAME_FN(CreateExtraPlanesFn, RVA_CREATE_EXTRA_PLANES)(w, h);
    placeCustomPlanes();
}

// RVA_PLANE_SPECIAL_TERRAIN(no_planes), then each mod's plane's special squares and its way in.
typedef void (*SpecialTerrainFn)(int noPlanes);
static void specialTerrainHook(int noPlanes) {
    GAME_FN(SpecialTerrainFn, RVA_PLANE_SPECIAL_TERRAIN)(noPlanes);
    if (!noPlanes) addCustomPlanesSpecialSquares();
}

// new_world's population, then the mods' planes': right after the Void's, the game's last.
#ifndef __APPLE__
#define POPULATE_AFTER_VOID_TARGET RVA_POPULATE_VOID
static void populateAfterVoidHook(void) {
    GAME_FN(void (*)(void), RVA_POPULATE_VOID)();
    populateCustomPlanes();
}
#else
#define POPULATE_AFTER_VOID_TARGET RVA_SPELL_BUG_CHECK
static void populateAfterVoidHook(const char *text, int level) {
    GAME_FN(void (*)(const char *, int), RVA_SPELL_BUG_CHECK)(text, level);
    populateCustomPlanes();
}
#endif

// setplane -1's draw: RVA_RANDOM(11) in the game, every plane of this world here (mods' planes included).
typedef int (*RandomFn)(int n);
static int setplaneRandomHook(int n) {
    (void)n;
    return GAME_FN(RandomFn, RVA_RANDOM)(G32(RVA_LAST_PLANE) + 1);
}

// The CoE5SE page's "Planes" box: a mod's plane the last world had no room for.
static void drawCustomPlaneProblems(void) {
    int skipped = 0;
    for (int i = 0; i < customPlaneCount; i++) skipped += customPlanes[i].skipped;
    if (!skipped) return;
    sectionBegin();
    sectionHeader("Planes");
    char line[512];
    for (int i = 0; i < customPlaneCount; i++) {
        if (!customPlanes[i].skipped) continue;
        snprintf(line, sizeof line, "%.31s (%.200s): no room in the last world", customPlanes[i].name, customPlanes[i].mod);
        textButton(0, line);
        tooltip("A plane goes beside the others up to 1000 columns, then under a smaller plane. A smaller map or fewer planes leave room.");
    }
    sectionEnd();
}

// ---- Saves --------------------------------------------------------------------------------------
// Adds PLNS: each mod's plane in this world, by index, with its name, mod, and rectangle.
static void addCustomPlanesChunk(SaveList *list) {
    int last = G32(RVA_LAST_PLANE);
    if (!planeTableMoved || last <= PLANE_ELYSIUM2) return;
    SaveBuffer b = {0};
    saveBufferInt(&b, SAVE_PLNS_VERSION);
    saveBufferInt(&b, last - PLANE_ELYSIUM2);
    for (int index = PLANE_ELYSIUM2 + 1; index <= last; index++) {
        char name[PLANE_NAME_SIZE];
        snprintf(name, sizeof name, "%s", (const char *)planeRecord(index));
        const CustomPlane *p = findCustomPlane(name);
        saveBufferInt(&b, index);
        saveBufferString(&b, name);
        saveBufferString(&b, p ? p->mod : "");
        saveBufferInt(&b, planeField(index, PLANE_X));
        saveBufferInt(&b, planeField(index, PLANE_Y));
        saveBufferInt(&b, planeField(index, PLANE_W));
        saveBufferInt(&b, planeField(index, PLANE_H));
    }
    saveBufferInt(&b, (int)b.size + 8);
    saveBufferInt(&b, SAVE_TAG_PLNS);
    saveListAdd(list, SAVE_TAG_PLNS, &b);
}

// Whether the save being loaded may pass the magic check.
static int loadedSavePlanesAccepted;

// The accepted save's planes by name and index.
static struct {
    int pending, count;
    char names[CUSTOM_PLANE_TARGET][PLANE_NAME_SIZE];
    char mods[CUSTOM_PLANE_TARGET][MOD_NAME_MAX];
    int indices[CUSTOM_PLANE_TARGET];
} loadedSavePlanes;

// Whether loading this save turns on mod `mod`, so the load's mod pass reads its planes' blocks.
static int loadTurnsModOn(const char *mod) {
    int m = mod[0] ? modIndexExact(mod) : -1;
    if (m < 0 || modEnabled(m) || strcmp(savedMods.result, "found")) return 0;
    const SavedModEntry *entry = NULL;
    for (int i = 0; i < savedMods.count && !entry; i++)
        if (!strcmp(savedMods.mods[i].name, mod)) entry = &savedMods.mods[i];
    ModFileScan scan;
    return entry && !(entry->flags & MOD_ADDED_NO_CONTENT) && scanMod(m, &scan) && !scan.contentFree;
}

// Adds "name(mod)" to a comma list, kept only while it fits.
static void addMissingPlaneName(char *list, size_t size, const char *name, const char *mod) {
    char one[80];
    snprintf(one, sizeof one, "%.31s(%.40s)", name, mod[0] ? mod : "?");
    size_t used = strlen(list), length = strlen(one);
    if (used + length + 2 >= size) return;
    if (used) list[used++] = ',';
    memcpy(list + used, one, length + 1);
}

// Reads PLNS from the end of the save at `path`, when it has one, and decides whether it loads.
static void readLoadedSavePlanes(const char *path) {
    loadedSavePlanesAccepted = 0;
    loadedSavePlanes.pending = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return;
    int ints[3] = {0}, payloadSize = 0;
    long end = !fseek(f, 0, SEEK_END) ? ftell(f) : -1;
    unsigned char *payload = NULL;
    const char *result = "none";
    if (end >= 28 && !fseek(f, end - 12, SEEK_SET)) {
        for (int i = 0; i < 3; i++) readFileInt(f, &ints[i]);
        if (ints[0] == SAVE_TAG_END && !ints[1] && ints[2] == SAVE_LIST_MAGIC) {
            long at = end - 12;
            for (int step = 0; step < 3 && !payload; step++) {
                int trailer[2] = {0};
                if (at < 16 || fseek(f, at - 8, SEEK_SET) || !readFileInt(f, &trailer[0]) || !readFileInt(f, &trailer[1])) break;
                if (trailer[1] == SAVE_TAG_PLNS) payload = readTrailedChunk(f, at, SAVE_TAG_PLNS, &payloadSize, &result);
                else if (trailer[1] == SAVE_TAG_MODC || trailer[1] == SAVE_TAG_MODH) at -= trailer[0] + 8;
                else break;
            }
        }
    }
    fclose(f);
    if (!payload) {
        if (strcmp(result, "none")) writeLog("save_planes_read", "result=%s", result);
        return;
    }
    SaveCursor c = {payload, payload + payloadSize, 0};
    int version = saveCursorInt(&c), count = saveCursorInt(&c), missing = 0, waiting = 0;
    char missingNames[512] = "";
    int readBack = !c.failed && version == SAVE_PLNS_VERSION && count > 0 && count <= CUSTOM_PLANE_TARGET;
    for (int k = 0; k < count && readBack; k++) {
        char name[SAVE_STRING_MAX + 1], mod[SAVE_STRING_MAX + 1];
        int index = saveCursorInt(&c);
        saveCursorString(&c, name, sizeof name);
        saveCursorString(&c, mod, sizeof mod);
        for (int f4 = 0; f4 < 4; f4++) saveCursorInt(&c);
        if (c.failed) {
            readBack = 0;
            break;
        }
        int fits = index > PLANE_ELYSIUM2 && index <= PLANE_INDEX_LAST_MOVED && strlen(name) < PLANE_NAME_SIZE && strlen(mod) < MOD_NAME_MAX;
        if (fits && !findCustomPlane(name) && loadTurnsModOn(mod)) waiting++;
        else if (!fits || !findCustomPlane(name)) {
            missing++;
            addMissingPlaneName(missingNames, sizeof missingNames, name, mod);
        }
        if (!fits) continue;
        snprintf(loadedSavePlanes.names[k], sizeof loadedSavePlanes.names[k], "%s", name);
        snprintf(loadedSavePlanes.mods[k], sizeof loadedSavePlanes.mods[k], "%s", mod);
        loadedSavePlanes.indices[k] = index;
    }
    if (readBack && !missing && waiting && ensurePlaneTableMoved() && !customPlaneHooksInstalled)
        customPlaneHooksInstalled = installCustomPlaneHooks();
    loadedSavePlanesAccepted = readBack && !missing && planeTableMoved && customPlaneHooksInstalled;
    if (loadedSavePlanesAccepted) {
        loadedSavePlanes.pending = 1, loadedSavePlanes.count = count;
        for (int i = 0; i < customPlaneCount; i++) customPlanes[i].index = -1, customPlanes[i].skipped = 0;
        for (int k = 0; k < count && !waiting; k++) findCustomPlane(loadedSavePlanes.names[k])->index = loadedSavePlanes.indices[k];
        resetCustomPlaneImages();
        writeLog("save_planes_read", "result=accepted planes=%d waiting=%d", count, waiting);
        if (!waiting) {
            resolvePlaneShapes();
            setPlaneGroundLikes();
            resolvePlaneNames(1);
        }
    } else
        writeLog("save_planes_refused", "reason=%s planes=%d missing=%s", !readBack ? "bad_chunk" : missing ? "plane_not_declared" : "table_not_moved",
                 count, missingNames[0] ? missingNames : "-");
    free(payload);
}

static void finishPlaneBlock(void);

// After the load's mod pass: every plane of the save takes its saved index.
static void applyLoadedSavePlanes(int level) {
    if (level != 2 || !loadedSavePlanes.pending) return;
    loadedSavePlanes.pending = 0;
    finishPlaneBlock();
    for (int i = 0; i < customPlaneCount; i++) customPlanes[i].index = -1, customPlanes[i].skipped = 0;
    int missing = 0;
    char missingNames[512] = "";
    for (int k = 0; k < loadedSavePlanes.count; k++) {
        CustomPlane *p = findCustomPlane(loadedSavePlanes.names[k]);
        if (p && p->fromWords != PLANE_FROM_EARLIER_PASS) {
            p->index = loadedSavePlanes.indices[k];
            continue;
        }
        missing++;
        addMissingPlaneName(missingNames, sizeof missingNames, loadedSavePlanes.names[k], loadedSavePlanes.mods[k]);
    }
    resetCustomPlaneImages();
    if (!missing) {
        writeLog("save_planes_given", "planes=%d", loadedSavePlanes.count);
        return;
    }
    writeLog("save_planes_refused", "reason=plane_not_declared_after_pass planes=%d missing=%s", loadedSavePlanes.count, missingNames);
    static char text[POPUP_TEXT_SIZE];
    snprintf(text, sizeof text, "Planes missing from this save's mods:\n%s\n", missingNames);
    okBox("CoE5SE: Not a proper save file", text);
    GAME_FN(void (*)(const char *), RVA_EXIT_GAME)("");
}

// ---- The hooks ----------------------------------------------------------------------------------
// Whether this world holds a mod's plane, so its save takes SAVE_MAGIC_CUSTOM_PLANES.
static int customPlanesInWorld(void) { return planeTableMoved && G32(RVA_LAST_PLANE) > PLANE_ELYSIUM2; }

// save_game's fwrite of the magic.
typedef size_t (*GameFileFn)(void *data, size_t size, size_t count, void *file);
static size_t writeSaveMagicHook(void *data, size_t size, size_t count, void *file) {
    int magic;
    memcpy(&magic, data, 4);
    if (customPlanesInWorld() && magic == SAVE_MAGIC_GAME) {
        magic = SAVE_MAGIC_CUSTOM_PLANES;
        return GAME_FN(GameFileFn, RVA_FWRITE)(&magic, size, count, file);
    }
    return GAME_FN(GameFileFn, RVA_FWRITE)(data, size, count, file);
}

// load_game's fread of the magic: a save with a mod's plane passes only when readLoadedSavePlanes accepted it.
static size_t readSaveMagicHook(void *data, size_t size, size_t count, void *file) {
    size_t read = GAME_FN(GameFileFn, RVA_FREAD)(data, size, count, file);
    int magic, game = SAVE_MAGIC_GAME;
    memcpy(&magic, data, 4);
    if (magic == SAVE_MAGIC_CUSTOM_PLANES && loadedSavePlanesAccepted) memcpy(data, &game, 4);
    return read;
}

// The Load Game list's count of saves: a save with a mod's plane counts.
static size_t countSaveMagicHook(void *data, size_t size, size_t count, void *file) {
    size_t read = GAME_FN(GameFileFn, RVA_FREAD)(data, size, count, file);
    int magic, game = SAVE_MAGIC_GAME;
    memcpy(&magic, data, 4);
    if (magic == SAVE_MAGIC_CUSTOM_PLANES) memcpy(data, &game, 4);
    return read;
}

#ifndef __APPLE__
// The server screen's copy of save_game writes the magic with write_int(0x6eb65, file).
typedef int (*GameWriteIntFn)(int value, void *file);
static int writeServerSaveMagicHook(int value, void *file) {
    if (customPlanesInWorld() && value == SAVE_MAGIC_GAME) value = SAVE_MAGIC_CUSTOM_PLANES;
    return GAME_FN(GameWriteIntFn, RVA_FILE_WRITE_INT)(value, file);
}

#define CUSTOM_PLANE_STUB_SIZE 0x20
static int pointCustomPlaneCall(unsigned int callRva, unsigned int targetRva, void *hook, const char *name) {
    static unsigned char *stubs;
    static int used;
    if (!stubs) {
        if (!(stubs = allocNear(gameBase + RVA_TEXT_SECTION))) {
            writeLog("hook_skipped", "site=%s reason=no_near_page", name);
            return 0;
        }
        makePagesReadExecute(stubs, 0x1000);
    }
    if ((used + 1) * CUSTOM_PLANE_STUB_SIZE > 0x1000) {
        writeLog("hook_skipped", "site=%s reason=no_stub_room", name);
        return 0;
    }
    return redirectCall(callRva, targetRva, stubs + CUSTOM_PLANE_STUB_SIZE * used++, hook, name, NULL);
}
#else
static int pointCustomPlaneCall(unsigned int callRva, unsigned int targetRva, void *hook, const char *name) {
    if (blTarget(gameBase + callRva) != gameBase + targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", name, callRva + 0x100000000ULL);
        return 0;
    }
    unsigned char *trampoline = newTrampoline(hook);
    uint32_t word = trampoline ? blTo(gameBase + callRva, trampoline) : 0;
    if (trampoline && writeGameCode(gameBase + callRva, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=%s", name, trampoline ? "code_unwritable" : "no_padding");
    return 0;
}
#endif

// Installs the world build, setplane and save hooks, at the first plane a mod declares.
static int installCustomPlaneHooks(void) {
    static const struct { unsigned int callRva, targetRva; void *hook; const char *name; } CALLS[] = {
        {RVA_NEW_GAME_EXTRA_PLANES_CALL,  RVA_CREATE_EXTRA_PLANES, (void *)createExtraPlanesHook, "new_game_extra_planes"},
        {RVA_NEW_WORLD_EXTRA_PLANES_CALL, RVA_CREATE_EXTRA_PLANES, (void *)createExtraPlanesHook, "new_world_extra_planes"},
        {RVA_RITUAL_SETPLANE_RANDOM_CALL, RVA_RANDOM,              (void *)setplaneRandomHook,    "setplane_random"},
        {RVA_NEW_GAME_SPECIAL_TERRAIN_CALL, RVA_PLANE_SPECIAL_TERRAIN, (void *)specialTerrainHook, "new_game_special_terrain"},
        {RVA_NEW_WORLD_SPECIAL_TERRAIN_CALL, RVA_PLANE_SPECIAL_TERRAIN, (void *)specialTerrainHook, "new_world_special_terrain"},
        {RVA_NEW_WORLD_POP_AFTER_VOID_CALL, POPULATE_AFTER_VOID_TARGET, (void *)populateAfterVoidHook, "new_world_populate_custom"},
        {RVA_SAVE_GAME_MAGIC_WRITE,       RVA_FWRITE,              (void *)writeSaveMagicHook,    "save_game_magic"},
        {RVA_LOAD_GAME_MAGIC_READ,        RVA_FREAD,               (void *)readSaveMagicHook,     "load_game_magic"},
#ifndef __APPLE__
        {RVA_SERVER_SAVE_MAGIC_WRITE,     RVA_FILE_WRITE_INT,      (void *)writeServerSaveMagicHook, "server_save_magic"},
#endif
    };
    int ok = 0, wanted = (int)(sizeof CALLS / sizeof CALLS[0]), viewWanted = 0;
    for (int i = 0; i < wanted; i++) ok += pointCustomPlaneCall(CALLS[i].callRva, CALLS[i].targetRva, CALLS[i].hook, CALLS[i].name);
    ok += installPlaneShapeHooks(), wanted++;
    int viewOk = installCustomPlaneViewHooks(&viewWanted);
    installPlaneGroundHooks();
    installPlaneTintHooks();
    writeLog("custom_plane_hooks_installed", "ok=%d of %d buttons=%d of %d", ok, wanted, viewOk, viewWanted);
    return ok == wanted;
}

// Installs the Load Game count's hook at startup, before the main menu first counts the saves.
static void installSaveCountHook(void) {
    writeLog("save_count_hook_installed", "ok=%d of 1",
             pointCustomPlaneCall(RVA_COUNT_SAVES_MAGIC_READ, RVA_FREAD, (void *)countSaveMagicHook, "count_saves_magic"));
}
