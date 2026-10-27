// Mods' planes from .c5se words: the newplane block, planeshape, and plane names in the game's plane words.
#pragma once

#define NEWPLANE_WORD          "newplane"
#define PLANE_LINE_MAX         1024
#define PLANE_REF_PLACEHOLDER  1000000
#define PLANE_NOT_IN_WORLD     (-3)
#define PLANE_AI_NEVER         999
#define PLANE_FILE_LINE_MAX    5000

static void finishPlaneBlock(void);
static void notePlaneName(const char *name, const char *mod);
static void noteRefusedPlane(const char *name, const char *mod);

// ---- Reading the words ----------------------------------------------------------------------------
// Reads a terrain by number or quoted name; -1, with *why set, when there is none.
static int readPlaneTerrain(const char **p, const char **why) {
    skipSpaces(p);
    if (**p == '"') {
        char name[TERRAIN_NAME_SIZE + 2];
        int len = readSpellQuoted(p, name, sizeof name);
        if (len <= 0 || len >= TERRAIN_NAME_SIZE) {
            *why = "bad_terrain";
            return -1;
        }
        int last = G32(RVA_LAST_TERRAIN);
        for (int t = 0; t <= last && t < TERRAIN_TABLE_MAX; t++)
            if (recordNameIs(GAME_PTR(RVA_TERRAIN_TABLE) + (size_t)t * TERRAIN_SIZE, TERRAIN_NAME_SIZE, name)) return t;
        *why = "unknown_terrain";
        return -1;
    }
    long long n;
    if (!readSpellNumber(p, &n) || n < 0 || n > SHRT_MAX) {
        *why = "bad_terrain";
        return -1;
    }
    return (int)n;
}

// Finds planefile `spec` in the mod's folder (findModFile; "x.coem" is read as "./x.coem").
static const char *findPlaneFile(int mod, const char *spec, char *path, size_t size) {
    static const char *const ENDINGS[2] = {".coem", ".coem"};
    static const char *const REASONS[4] = {"bad_file", "file_not_found", "file_wrong_case", "file_link"};
    char dotted[PLANE_ICON_SPEC_MAX + 3];
    snprintf(dotted, sizeof dotted, "%s%.200s", strncmp(spec, "./", 2) ? "./" : "", spec);
    return findModFile(modFolder(mod), dotted, ENDINGS, REASONS, path, size);
}

// The first `mapsize <w> <h>` line of map file `path`: 1 and the size, or 0.
static int planeFileMapsize(const char *path, int *w, int *h) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    static char line[PLANE_FILE_LINE_MAX];
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        const char *p = line;
        skipSpaces(&p);
        if (strncmp(p, "mapsize", 7) || (p[7] != ' ' && p[7] != '\t')) continue;
        p += 7;
        long long a, b;
        found = readSpellNumber(&p, &a) && readSpellNumber(&p, &b) && a > 0 && a < MAP_COLUMNS_MAX && b > 0 && b < PLANE_ROWS_MAX;
        if (found) *w = (int)a, *h = (int)b;
        else break;
    }
    fclose(f);
    return found;
}

// ---- The newplane block ---------------------------------------------------------------------------
static struct {
    int open, refused, mod, line;
    unsigned int fileId;
    char name[PLANE_NAME_SIZE];
    int w, h, hasSize, fill, hasFill, recipe, populate, populateSet;
    int swapCount;
    short swapFrom[PLANE_SWAP_MAX], swapTo[PLANE_SWAP_MAX];
    char icon[MAX_PATH], file[MAX_PATH];
    int fileW, fileH;
    PlaneLayout layout;
    int zoneOpen;
} planeBlock;

// The block words, which a newplane block takes; any other word ends it.
static const char *const PLANE_BLOCK_WORDS[] = {
    "planesize", "planefill", "planelike", "planepopulate", "planeswap", "planeicon", "planefile",
    "planepiece", "planezone", "planezoneswap", "planezoneunits", "planezonepopulate", "planeunits",
};

static void planepieceLine(const char *args, int mod, int level);
static void planezoneLine(const char *args, int mod, int level);
static void planezoneswapLine(const char *args, int mod, int level);
static void planezoneunitsLine(const char *args, int mod, int level);
static void planezonepopulateLine(const char *args, int mod, int level);
static void planeunitsLine(const char *args, int mod, int level);
static void finishPlaneLayout(PlaneLayout *layout, int w, int h, const char *mod);
static void notePlaneLayoutProblems(const PlaneLayout *layout, const char *mod);
static void appendPlaneProblems(char *text, size_t size, size_t *n, int *count);
static void dropPassPlaneProblems(void);

static int isPlaneBlockWord(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof PLANE_BLOCK_WORDS / sizeof PLANE_BLOCK_WORDS[0]; i++)
        if (spellWordIs(line, len, PLANE_BLOCK_WORDS[i])) return 1;
    return 0;
}

// Refuses a block word's line: logged with its line number.
static PRINTF_FORMAT(4, 5) void refusePlaneLine(const char *word, int mod, const char *reason, const char *fmt, ...) {
    char details[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    refuseC5mLine(word, spellModName(mod), reason, "line=%d %s", G32(RVA_MOD_LINE), details);
}

// Whether a block word's line applies: level 2, inside an open block that wasn't refused.
static int planeBlockTakes(const char *word, int mod, int level) {
    if (level != 2) return 0;
    if (!planeBlock.open || planeBlock.mod != mod) {
        refusePlaneLine(word, mod, "no_plane_block", "plane=-");
        return 0;
    }
    return !planeBlock.refused;
}

// newplane "<name>": opens a plane's block, and ends the game's blocks and the one before.
static void newplaneLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    finishPlaneBlock();
    clearParserCurrentRecords();
    memset(&planeBlock, 0, sizeof planeBlock);
    planeBlock.open = 1, planeBlock.mod = mod, planeBlock.line = G32(RVA_MOD_LINE), planeBlock.fileId = modFileNow();
    planeBlock.recipe = planeBlock.populate = -1;
    snprintf(planeBlock.layout.source, sizeof planeBlock.layout.source, "%s", (const char *)GAME_PTR(RVA_EVENT_FILE));
    const char *p = args;
    char name[PLANE_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = mod < 0 || mod > G32(RVA_MOD_LAST_INDEX) ? "not_in_a_mod"
                       : len <= 0                                 ? "no_name"
                       : len >= PLANE_NAME_SIZE                   ? "name_too_long"
                       : NULL;
    if (reason) {
        planeBlock.refused = 1;
        refusePlaneLine(NEWPLANE_WORD, mod, reason, "text=%.60s", args);
        return;
    }
    snprintf(planeBlock.name, sizeof planeBlock.name, "%.*s", PLANE_NAME_SIZE - 1, name);
}

static void planesizeLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planesize", mod, level)) return;
    const char *p = args;
    long long w, h;
    if (!readSpellNumber(&p, &w) || !readSpellNumber(&p, &h) || w < 1 || w >= MAP_COLUMNS_MAX || h < 1 || h >= PLANE_ROWS_MAX) {
        refusePlaneLine("planesize", mod, "bad_size", "plane=%s text=%.40s", planeBlock.name, args);
        return;
    }
    planeBlock.w = (int)w, planeBlock.h = (int)h, planeBlock.hasSize = 1;
}

static void planefillLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planefill", mod, level)) return;
    const char *p = args, *why = NULL;
    int terrain = readPlaneTerrain(&p, &why);
    if (terrain < 0) {
        refusePlaneLine("planefill", mod, why, "plane=%s text=%.40s", planeBlock.name, args);
        return;
    }
    planeBlock.fill = terrain, planeBlock.hasFill = 1;
}

// Reads a vanilla plane's name into *plane: 1, or 0 having refused the line.
static int readVanillaPlane(const char *word, const char *args, int mod, int *plane) {
    const char *p = args;
    char name[PLANE_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    *plane = len > 0 && len < PLANE_NAME_SIZE ? vanillaPlaneNamed(name) : -1;
    if (*plane >= 0) return 1;
    refusePlaneLine(word, mod, len <= 0 ? "no_name" : "unknown_plane", "plane=%s text=%.40s", planeBlock.name, args);
    return 0;
}

static void planelikeLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planelike", mod, level)) return;
    int like;
    if (!readVanillaPlane("planelike", args, mod, &like)) return;
    if (!PLANE_HAS_RECIPE[like]) {
        refusePlaneLine("planelike", mod, "no_recipe", "plane=%s like=%s", planeBlock.name, VANILLA_PLANE_SHORT_NAMES[like]);
        return;
    }
    planeBlock.recipe = like;
}

static void planepopulateLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planepopulate", mod, level)) return;
    const char *p = args;
    skipSpaces(&p);
    if (!strncmp(p, "off", 3) && (!p[3] || p[3] == ' ' || p[3] == '\t' || p[3] == '\r' || p[3] == '\n')) {
        planeBlock.populate = -1, planeBlock.populateSet = 1;
        return;
    }
    int populate;
    if (!readVanillaPlane("planepopulate", args, mod, &populate)) return;
    planeBlock.populate = populate, planeBlock.populateSet = 1;
}

// planeswap <vanilla terrain> <terrain> inside a block (planeWordLineHook takes it before the game's word).
static void planeswapBlockLine(const char *args, int mod) {
    if (planeBlock.refused) return;
    const char *p = args, *why = NULL;
    int from = readPlaneTerrain(&p, &why), to = from >= 0 ? readPlaneTerrain(&p, &why) : -1;
    if (to < 0) {
        refusePlaneLine("planeswap", mod, why ? why : "bad_terrain", "plane=%s text=%.40s", planeBlock.name, args);
        return;
    }
    if (planeBlock.swapCount >= PLANE_SWAP_MAX) {
        refusePlaneLine("planeswap", mod, "too_many_swaps", "plane=%s max=%d", planeBlock.name, PLANE_SWAP_MAX);
        return;
    }
    planeBlock.swapFrom[planeBlock.swapCount] = (short)from, planeBlock.swapTo[planeBlock.swapCount] = (short)to;
    planeBlock.swapCount++;
}

static void planeiconLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planeicon", mod, level)) return;
    const char *p = args;
    char spec[PLANE_ICON_SPEC_MAX + 2];
    int len = readSpellQuoted(&p, spec, sizeof spec);
    const char *reason = len <= 0 ? "no_name" : len > PLANE_ICON_SPEC_MAX ? "bad_icon" : findModImage(modFolder(mod), spec, planeBlock.icon, sizeof planeBlock.icon);
    if (reason) {
        planeBlock.icon[0] = '\0';
        refusePlaneLine("planeicon", mod, reason, "plane=%s text=%.60s", planeBlock.name, args);
    }
}

static int installPlaneFileHook(void);

static void planefileLine(const char *args, int mod, int level) {
    if (!planeBlockTakes("planefile", mod, level)) return;
    const char *p = args;
    char spec[PLANE_ICON_SPEC_MAX + 2];
    int len = readSpellQuoted(&p, spec, sizeof spec);
    const char *reason = len <= 0 ? "no_name" : len > PLANE_ICON_SPEC_MAX ? "bad_file" : findPlaneFile(mod, spec, planeBlock.file, sizeof planeBlock.file);
    if (!reason && !planeFileMapsize(planeBlock.file, &planeBlock.fileW, &planeBlock.fileH)) reason = "no_mapsize";
    if (!reason && !installPlaneFileHook()) reason = "hook_not_installed";
    if (reason) {
        planeBlock.file[0] = '\0';
        refusePlaneLine("planefile", mod, reason, "plane=%s text=%.60s", planeBlock.name, args);
    }
}

// Ends the open block: declares its plane through declareCustomPlane, or refuses the block.
static void finishPlaneBlock(void) {
    if (!planeBlock.open) return;
    planeBlock.open = 0;
    if (planeBlock.refused) return;
    int hasFile = planeBlock.file[0] != '\0';
    int sizeFromLike = !planeBlock.hasSize && !hasFile && likePlaneSizeExtra(planeBlock.recipe) >= 0;
    int w = planeBlock.hasSize ? planeBlock.w : sizeFromLike ? PLANE_LIKE_SIZE_MIN : planeBlock.fileW;
    int h = planeBlock.hasSize ? planeBlock.h : sizeFromLike ? PLANE_LIKE_SIZE_MIN : planeBlock.fileH;
    int fill = planeBlock.hasFill ? planeBlock.fill : 0;
    const char *mod = spellModName(planeBlock.mod);
    const char *reason = !planeBlock.hasSize && !hasFile && !sizeFromLike                        ? "no_size"
                       : hasFile && (w != planeBlock.fileW || h != planeBlock.fileH)              ? "file_size_differs"
                       : !planeBlock.hasFill && planeBlock.recipe < 0 && !hasFile                 ? "no_fill"
                       : NULL;
    if (!reason) finishPlaneLayout(&planeBlock.layout, w, h, mod);
    if (!reason) reason = declareCustomPlane(mod, planeBlock.name, w, h, fill, planeBlock.icon);
    if (reason) {
        refuseC5mLine(NEWPLANE_WORD, mod, reason, "line=%d name=%s w=%d h=%d file_w=%d file_h=%d", planeBlock.line, planeBlock.name, w, h,
                      planeBlock.fileW, planeBlock.fileH);
        noteRefusedPlane(planeBlock.name, mod);
        return;
    }
    CustomPlane *p = findCustomPlane(planeBlock.name);
    p->fromWords = PLANE_FROM_WORDS;
    p->recipe = planeBlock.recipe, p->sizeFromLike = sizeFromLike;
    p->populate = planeBlock.populateSet ? planeBlock.populate : planeBlock.recipe;
    p->swapCount = planeBlock.swapCount;
    memcpy(p->swapFrom, planeBlock.swapFrom, sizeof p->swapFrom);
    memcpy(p->swapTo, planeBlock.swapTo, sizeof p->swapTo);
    snprintf(p->file, sizeof p->file, "%s", planeBlock.file);
    p->layout = planeBlock.layout;
    writeLog("plane_recipe", "mod=%s name=%s like=%s populate=%s swaps=%d generate=0 file=%s pieces=%d zones=%d units=%d", mod, p->name,
             p->recipe >= 0 ? VANILLA_PLANE_SHORT_NAMES[p->recipe] : "-", p->populate >= 0 ? VANILLA_PLANE_SHORT_NAMES[p->populate] : "-",
             p->swapCount, hasFile ? p->file : "-", p->layout.pieceCount, p->layout.zoneCount, p->layout.units.count);
}

// planeshape "<plane>" "<monster>": the current monster's shape on a mod's plane ("" for none).
static void planeshapeLine(const char *args, int mod, int level) {
    if (level != 2) return;
    int monster = G32(RVA_MONSTER_CURRENT);
    const char *p = args;
    char plane[PLANE_NAME_SIZE + 2], shape[PLANE_SHAPE_NAME_SIZE + 2];
    int planeLen = readSpellQuoted(&p, plane, sizeof plane), shapeLen = planeLen > 0 ? readSpellQuoted(&p, shape, sizeof shape) : -1;
    const unsigned char *record = monsterRecord(monster);
    const char *reason = !record                                   ? "no_monster"
                       : planeLen <= 0                             ? "no_plane"
                       : shapeLen < 0                              ? "no_shape"
                       : planeLen >= PLANE_NAME_SIZE               ? "bad_plane"
                       : shapeLen >= PLANE_SHAPE_NAME_SIZE         ? "bad_name"
                       : NULL;
    char name[MONSTER_NAME_SIZE + 1] = "";
    if (!reason) {
        snprintf(name, sizeof name, "%.*s", MONSTER_NAME_SIZE, (const char *)record);
        reason = addPlaneShape(spellModName(mod), name, monster, plane, shape, G32(RVA_MOD_LINE));
        if (!reason) notePlaneName(plane, spellModName(mod));
    }
    if (reason) refusePlaneLine("planeshape", mod, reason, "monster=%d text=%.60s", monster, args);
}

// ---- Plane names in the game's plane words ----------------------------------------------------------
enum { PLANE_REF_RITUAL, PLANE_REF_CLASS };

// The game's words outside events whose first number is a plane: a ritual's effect, or the class's startplane.
static const struct { const char *word; int kind, id; } PLANE_NUMBER_WORDS[] = {
    {"planereq", PLANE_REF_RITUAL, RITUAL_FX_PLANEREQ},     {"noplanereq", PLANE_REF_RITUAL, RITUAL_FX_NOPLANEREQ},
    {"setplane", PLANE_REF_RITUAL, RITUAL_FX_SETPLANE},     {"planeloc", PLANE_REF_RITUAL, RITUAL_FX_PLANELOC},
    {"planeswap", PLANE_REF_RITUAL, RITUAL_FX_PLANESWAP},   {"aionlyplane", PLANE_REF_RITUAL, RITUAL_FX_AIONLYPLANE},
    {"startplane", PLANE_REF_CLASS, CLASS_ABILITY_STARTPLANE},
};

// The game's event words with a plane number, and which number it is (from 0).
static const struct { const char *word; int arg; } PLANE_EVENT_WORDS[] = {
    {"+plane", 0}, {"-plane", 0}, {"randloc", 0}, {"+minterrains", 2}, {"-minterrains", 2}, {"+minunits", 2}, {"-minunits", 2},
};

// A plane name read on a pass, and where its number went: a ritual's effect or a class's startplane pair.
typedef struct {
    int kind, record, id;
    int slot;
    int line;
    char name[PLANE_NAME_SIZE], mod[MOD_NAME_MAX], word[16];
} PlaneNameRef;

static PlaneNameRef *planeRefs;
static int planeRefCount, planeRefCapacity;
static char planeNameLine[PLANE_LINE_MAX];
static char planeEventLine[PLANE_LINE_MAX];

// The index plane `name` has in this world: a vanilla plane's, or a mod's that this world holds; else -1.
static int planeIndexNamed(const char *name) {
    int vanilla = vanillaPlaneNamed(name);
    if (vanilla >= 0) return vanilla;
    const CustomPlane *p = findCustomPlane(name);
    return p && !p->skipped && p->index > PLANE_ELYSIUM2 && p->index <= PLANE_INDEX_LAST_MOVED ? p->index : -1;
}

// Whether this world's planes have their indices: a world built, or a save with mods' planes loaded.
static int planeIndicesKnown(void) {
    for (int i = 0; i < customPlaneCount; i++)
        if (customPlanes[i].index >= 0) return 1;
    return 0;
}

// Writes `number` in place of the quoted name at `at` in `line` (`len` bytes of word first), into out.
static int putPlaneNumber(const char *line, const char *at, const char *rest, long long number, char *out, const char *word, const char *mod) {
    int n = snprintf(out, PLANE_LINE_MAX, "%.*s%lld%s", (int)(at - line), line, number, rest);
    if (n >= 0 && n < PLANE_LINE_MAX) return 1;
    refuseC5mLine(word, mod, "line_too_long", "line=%d", G32(RVA_MOD_LINE));
    return 0;
}

// A ritual's or class's plane word with a quoted name; NULL when refused.
static const char *planeNumberWordLine(const char *line, size_t len, const char *args, int mod, int kind, int id) {
    char word[16], name[PLANE_NAME_SIZE + 2];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    const char *rest = args;
    int nameLen = readSpellQuoted(&rest, name, sizeof name);
    if (nameLen <= 0 || nameLen >= PLANE_NAME_SIZE) {
        refusePlaneLine(word, mod, nameLen <= 0 ? "no_name" : "bad_name", "text=%.60s", args);
        return NULL;
    }
    int vanilla = vanillaPlaneNamed(name);
    notePlaneName(name, spellModName(mod));
    int record = G32(kind == PLANE_REF_RITUAL ? RVA_RITUAL_CURRENT : RVA_CLASS_CURRENT);
    long long number = vanilla >= 0 ? vanilla : PLANE_NOT_IN_WORLD;
    if (vanilla < 0 && record >= 0 && growList((void **)&planeRefs, &planeRefCapacity, planeRefCount + 1, sizeof planeRefs[0])) {
        PlaneNameRef *r = &planeRefs[planeRefCount];
        memset(r, 0, sizeof *r);
        r->kind = kind, r->record = record, r->id = id, r->slot = -1, r->line = G32(RVA_MOD_LINE);
        snprintf(r->name, sizeof r->name, "%.*s", PLANE_NAME_SIZE - 1, name);
        snprintf(r->mod, sizeof r->mod, "%s", spellModName(mod));
        snprintf(r->word, sizeof r->word, "%s", word);
        number = PLANE_REF_PLACEHOLDER + planeRefCount++;
    }
    return putPlaneNumber(line, args, rest, number, planeNameLine, word, spellModName(mod)) ? planeNameLine : NULL;
}

// The effect at `slot` of ritual `record`: its int id, then its int64 value at RITUAL_FX_VALUE.
static unsigned char *ritualEffectAt(int record, int slot) {
    return GAME_PTR(ritualTableRva()) + (size_t)record * RITUAL_SIZE + RITUAL_FX + (size_t)slot * RITUAL_FX_SIZE;
}
// The ability pair at `slot` of class `record`.
static int *classAbilityPairAt(int record, int slot) {
    return (int *)(tableAddress(TABLE_CLASS) + (size_t)record * CLASS_SIZE + CLASS_ABILITIES) + 2 * slot;
}

// Finds where each reference of the pass landed: the effect or pair holding its placeholder.
static void findPlaneRefSlots(void) {
    for (int i = 0; i < planeRefCount; i++) {
        PlaneNameRef *r = &planeRefs[i];
        if (r->slot != -1) continue;
        r->slot = -2;
        long long placeholder = PLANE_REF_PLACEHOLDER + i;
        if (r->kind == PLANE_REF_RITUAL && r->record < ritualSlotCount()) {
            for (int s = 0; s < RITUAL_FX_MAX; s++) {
                const unsigned char *fx = ritualEffectAt(r->record, s);
                int fxId;
                long long value;
                memcpy(&fxId, fx, 4);
                memcpy(&value, fx + RITUAL_FX_VALUE, 8);
                if (fxId <= 0) break;
                if (fxId == r->id && value == placeholder) r->slot = s;
            }
        } else if (r->kind == PLANE_REF_CLASS && r->record < tableRecords(TABLE_CLASS)) {
            for (int s = 0; s < CLASS_ABILITY_PAIRS; s++) {
                const int *pair = classAbilityPairAt(r->record, s);
                if (!pair[0]) break;
                if (pair[0] == r->id && pair[1] == placeholder) r->slot = s;
            }
        }
    }
}

// Writes each reference's plane number for this world's indices.
static void resolvePlaneNames(int planesKnown) {
    int resolved = 0, missing = 0;
    for (int i = 0; i < planeRefCount; i++) {
        const PlaneNameRef *r = &planeRefs[i];
        if (r->slot < 0) continue;
        int index = planeIndexNamed(r->name);
        if (index < 0) {
            missing++;
            if (planesKnown && !findCustomPlane(r->name))
                refuseC5mLine(r->word, r->mod, "unknown_plane", "line=%d plane=%s", r->line, r->name);
        } else resolved++;
        if (r->kind == PLANE_REF_CLASS) {
            classAbilityPairAt(r->record, r->slot)[1] = index >= 0 ? index : 0;
            continue;
        }
        int id = r->id;
        long long value = index;
        if (index < 0) {
            value = r->id == RITUAL_FX_AIONLYPLANE ? PLANE_AI_NEVER : PLANE_NOT_IN_WORLD;
            if (r->id == RITUAL_FX_PLANELOC || r->id == RITUAL_FX_PLANESWAP) id = RITUAL_FX_PLANEREQ;
        }
        unsigned char *fx = ritualEffectAt(r->record, r->slot);
        memcpy(fx, &id, 4);
        memcpy(fx + RITUAL_FX_VALUE, &value, 8);
    }
    if (planeRefCount) writeLog("plane_names_resolved", "names=%d resolved=%d not_in_world=%d planes_known=%d", planeRefCount, resolved, missing, planesKnown);
}

// An event line with a quoted plane name: the line with the plane's index, or NULL when it fails the event.
static const char *planeNameEventLine(const char *line, int level, const char *mod) {
    if (line[0] != '+' && line[0] != '-' && line[0] != 'r') return line;
    size_t len = strcspn(line, " \t");
    int arg = -1;
    for (size_t i = 0; i < sizeof PLANE_EVENT_WORDS / sizeof PLANE_EVENT_WORDS[0]; i++)
        if (spellWordIs(line, len, PLANE_EVENT_WORDS[i].word)) arg = PLANE_EVENT_WORDS[i].arg;
    if (arg < 0 || level == 4) return line;
    const char *at = line + len;
    for (int n = 0; n < arg; n++) {
        while (*at == ' ' || *at == '\t') at++;
        at += strcspn(at, " \t");
    }
    while (*at == ' ' || *at == '\t') at++;
    if (*at != '"') return line;
    char word[16], name[PLANE_NAME_SIZE + 2];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    const char *rest = at;
    int nameLen = readSpellQuoted(&rest, name, sizeof name);
    if (level == 2 && mod[0] && nameLen > 0 && nameLen < PLANE_NAME_SIZE) notePlaneName(name, mod);
    if (G32(RVA_EVENT_STATE) < 1) return line;
    int index = nameLen > 0 && nameLen < PLANE_NAME_SIZE ? planeIndexNamed(name) : -1;
    if (index < 0 && (nameLen <= 0 || nameLen >= PLANE_NAME_SIZE || (planeIndicesKnown() && !findCustomPlane(name))))
        refuseC5mLine(word, mod[0] ? mod : "-", nameLen <= 0 || nameLen >= PLANE_NAME_SIZE ? "bad_name" : "unknown_plane", "plane=%.40s line=%d", name,
                      G32(RVA_MOD_LINE));
    if (index < 0 && !strcmp(word, "randloc")) {
        G32(RVA_EVENT_STATE) = EVENT_FAILED;
        return NULL;
    }
    long long number = index >= 0 ? index : PLANE_NOT_IN_WORLD;
    return putPlaneNumber(line, at, rest, number, planeEventLine, word, mod[0] ? mod : "-") ? planeEventLine : NULL;
}

// ---- The missing plane pop-up -------------------------------------------------------------------------
// The plane names mods used, for the missing plane pop-up.
#define MISSING_PLANE_ENTRIES 6
typedef struct {
    char name[PLANE_NAME_SIZE], mod[MOD_NAME_MAX];
    char file[MAX_PATH];
    int line;
    int script;
} NamedPlane;
static NamedPlane *namedPlanes;
static int namedPlaneCount, namedPlaneCapacity;

// Adds plane `name`, used by mod `mod` at `line` of `file`, to the list: once per mod, its first use.
static void notePlaneNameAt(const char *name, const char *mod, const char *file, int line, int script) {
    if (!name[0] || !mod[0] || vanillaPlaneNamed(name) >= 0) return;
    for (int i = 0; i < namedPlaneCount; i++)
        if (!strcmp(namedPlanes[i].name, name) && !strcmp(namedPlanes[i].mod, mod)) return;
    if (!growList((void **)&namedPlanes, &namedPlaneCapacity, namedPlaneCount + 1, sizeof *namedPlanes))
        return;
    NamedPlane *p = &namedPlanes[namedPlaneCount++];
    memset(p, 0, sizeof *p);
    snprintf(p->name, sizeof p->name, "%.*s", PLANE_NAME_SIZE - 1, name);
    snprintf(p->mod, sizeof p->mod, "%s", mod);
    snprintf(p->file, sizeof p->file, "%s", file);
    p->line = line, p->script = script;
}

// The same for the line the game is reading now: its file (RVA_EVENT_FILE) and line (RVA_MOD_LINE).
static void notePlaneName(const char *name, const char *mod) {
    notePlaneNameAt(name, mod, (const char *)GAME_PTR(RVA_EVENT_FILE), G32(RVA_MOD_LINE), 0);
}

// The scripts' coe5.setPlaneShape planes join the list, with each script's mod, file and line.
static void noteScriptPlaneNames(void) {
    for (int i = 0; i < planeShapeDeclarationCount; i++) {
        const PlaneShapeDeclaration *d = &planeShapeDeclarations[i];
        if (d->monsterIndex >= 0) continue;
        int mod = modIndexByName(d->mod);
        char file[MAX_PATH];
        if (mod >= 0) snprintf(file, sizeof file, "%.200s/%.48s.lua", modFolder(mod), d->mod);
        else snprintf(file, sizeof file, "%.48s.lua", d->mod);
        notePlaneNameAt(d->plane, d->mod, file, d->line, 1);
    }
}

// The newplane blocks the latest pass refused, by name.
typedef struct {
    char name[PLANE_NAME_SIZE], mod[MOD_NAME_MAX];
} RefusedPlane;
static RefusedPlane *refusedPlanes;
static int refusedPlaneCount, refusedPlaneCapacity;

static void noteRefusedPlane(const char *name, const char *mod) {
    if (!name[0]) return;
    for (int i = 0; i < refusedPlaneCount; i++)
        if (!strcmp(refusedPlanes[i].name, name)) return;
    if (!growList((void **)&refusedPlanes, &refusedPlaneCapacity, refusedPlaneCount + 1, sizeof *refusedPlanes))
        return;
    snprintf(refusedPlanes[refusedPlaneCount].name, PLANE_NAME_SIZE, "%s", name);
    snprintf(refusedPlanes[refusedPlaneCount].mod, MOD_NAME_MAX, "%s", mod);
    refusedPlaneCount++;
}

// The mod whose newplane block of that name was refused on the latest pass, or NULL.
static const char *refusedPlaneMod(const char *name) {
    for (int i = 0; i < refusedPlaneCount; i++)
        if (!strcmp(refusedPlanes[i].name, name)) return refusedPlanes[i].mod;
    return NULL;
}

// Whether no game plane and no declared plane has the name.
static int isPlaneNameMissing(const char *name) {
    return vanillaPlaneNamed(name) < 0 && !findCustomPlane(name);
}

// The missing plane pop-up's text; *missing gets the count of entries.
static size_t missingPlanesText(char *text, size_t size, int *missing) {
    size_t n = 0;
    int count = 0;
    text[0] = '\0';
    for (int i = 0; i < namedPlaneCount; i++) {
        const NamedPlane *p = &namedPlanes[i];
        if (!isPlaneNameMissing(p->name)) continue;
        if (count++ == MISSING_PLANE_ENTRIES) appendText(text, size, &n, "\n\n...and others.");
        if (count > MISSING_PLANE_ENTRIES) continue;
        appendText(text, size, &n, "%sError in %s '%s' at line %d\n\n", n ? "\n\n" : "", p->script ? "script" : "mod", p->file, p->line);
        if (refusedPlaneMod(p->name)) appendText(text, size, &n, "Plane '%s' refused, see coe5mod.log", p->name);
        else appendText(text, size, &n, "No plane called '%s'", p->name);
    }
    appendPlaneProblems(text, size, &n, &count);
    *missing = count;
    return n;
}

// The pop-up, between main menu frames, once for each new set of missing planes (as popupRitualsLeftOut).
static void popupMissingPlanes(void) {
    static unsigned int shownHash;
    static char text[POPUP_TEXT_SIZE];
    int missing;
    noteScriptPlaneNames();
    missingPlanesText(text, sizeof text, &missing);
    if (!missing) {
        shownHash = 0;
        return;
    }
    unsigned int hash = 2166136261u;
    for (const char *c = text; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
    if (hash == shownHash) return;
    shownHash = hash;
    for (int i = 0; i < namedPlaneCount; i++) {
        const NamedPlane *p = &namedPlanes[i];
        if (!isPlaneNameMissing(p->name)) continue;
        const char *refusedIn = refusedPlaneMod(p->name);
        writeLog("plane_missing", "name=%s mod=%s line=%d script=%d refused_in=%s", p->name, p->mod, p->line, p->script, refusedIn ? refusedIn : "-");
    }
    writeLog("missing_planes_popup", "entries=%d", missing);
    okBox("CoE5SE: Mod Error", text);
}

// ---- The line hook and the passes -----------------------------------------------------------------
// Every .c5m and .c5se line outside events comes here after nameListLineHook (modCommandHook).
static const char *planeWordLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2) return line;
    if (planeBlock.open && (planeBlock.mod != mod || planeBlock.fileId != modFileNow())) finishPlaneBlock();
    const char *args = line + len;
    while (*args == ' ' || *args == '\t') args++;
    if (planeBlock.open) {
        if (spellWordIs(line, len, "planeswap")) {
            planeswapBlockLine(args, mod);
            return NULL;
        }
        if (!isPlaneBlockWord(line, len)) finishPlaneBlock();
    }
    if (*args != '"') return line;
    for (size_t i = 0; i < sizeof PLANE_NUMBER_WORDS / sizeof PLANE_NUMBER_WORDS[0]; i++)
        if (spellWordIs(line, len, PLANE_NUMBER_WORDS[i].word))
            return planeNumberWordLine(line, len, args, mod, PLANE_NUMBER_WORDS[i].kind, PLANE_NUMBER_WORDS[i].id);
    return line;
}

// The same when no plugin loaded; returns 1 to skip the line.
static int declinedPlaneLine(const char *line, size_t len, int mod, int level) {
    static int inBlock, blockMod, blockLevel;
    if (mod != blockMod || level != blockLevel) inBlock = 0;
    blockMod = mod, blockLevel = level;
    if (spellWordIs(line, len, NEWPLANE_WORD)) inBlock = 1;
    else if (!isPlaneBlockWord(line, len)) inBlock = 0;
    if (inBlock && spellWordIs(line, len, "planeswap")) return 1;
    const char *args = line + len;
    while (*args == ' ' || *args == '\t') args++;
    if (*args != '"') return 0;
    for (size_t i = 0; i < sizeof PLANE_NUMBER_WORDS / sizeof PLANE_NUMBER_WORDS[0]; i++)
        if (spellWordIs(line, len, PLANE_NUMBER_WORDS[i].word)) return 1;
    return 0;
}

// An event line with a quoted plane name, for declinedEventLineHook: 1 to skip it.
static int declinedPlaneEventLine(const char *line) {
    size_t len = strcspn(line, " \t");
    for (size_t i = 0; i < sizeof PLANE_EVENT_WORDS / sizeof PLANE_EVENT_WORDS[0]; i++)
        if (spellWordIs(line, len, PLANE_EVENT_WORDS[i].word) && strchr(line, '"')) return 1;
    return 0;
}

// Before a level-2 mod pass: the pass reads every block, planeshape and plane name again.
static void planeWordsBeforePass(int level) {
    if (level != 2) return;
    finishPlaneBlock();
    for (int i = 0; i < customPlaneCount; i++)
        if (customPlanes[i].fromWords) customPlanes[i].fromWords = PLANE_FROM_EARLIER_PASS;
    dropPlaneShapeWords();
    planeRefCount = 0;
    namedPlaneCount = refusedPlaneCount = 0;
    dropPassPlaneProblems();
}

// After a level-2 mod pass: ends the last block, drops planes not named again, numbers plane names.
static void planeWordsAfterPass(int level) {
    if (level != 2) return;
    finishPlaneBlock();
    int kept = 0, dropped = 0;
    for (int i = 0; i < customPlaneCount; i++) {
        if (customPlanes[i].fromWords == PLANE_FROM_EARLIER_PASS) {
            writeLog("plane_dropped", "mod=%s name=%s reason=not_declared_this_pass", customPlanes[i].mod, customPlanes[i].name);
            dropped++;
            continue;
        }
        if (kept != i) customPlanes[kept] = customPlanes[i];
        kept++;
    }
    customPlaneCount = kept;
    if (dropped) resetCustomPlaneImages();
    for (int i = 0; i < customPlaneCount; i++)
        if (customPlanes[i].fromWords) notePlaneLayoutProblems(&customPlanes[i].layout, customPlanes[i].mod);
    findPlaneRefSlots();
    int known = planeIndicesKnown();
    resolvePlaneNames(known);
    if (known) resolvePlaneShapes(), setPlaneGroundLikes();
}

// ---- planefile: a plane's squares from a map file ---------------------------------------------------
// The game's map reader reads the file into the placed plane; only square words pass.
static const char *const PLANE_FILE_WORDS[] = {"terrainrow", "groundrow", "temprow", "squarename", "squarespec", "portal"};

static struct {
    int active, index, stopped, lines, leftOut;
    int yShift, rows;
    int groundRows;
    const CustomPlane *p;
} planeFileLoad;

typedef void (*MapLineFn)(const char *line, int decorate, const char *name, int plane);
typedef void (*LoadMapPlaneFn)(const char *path, const char *name, int plane);
static MapLineFn planeFileMapLineForTests;
static LoadMapPlaneFn planeFileLoadForTests;
static void passMapLine(const char *line, int decorate, const char *name, int plane) {
    if (planeFileMapLineForTests) planeFileMapLineForTests(line, decorate, name, plane);
    else GAME_FN(MapLineFn, RVA_MAP_LINE)(line, decorate, name, plane);
}

// A square word's line with its y moved down by `shift`, into `out`; NULL when out of range.
static const char *shiftMapLineY(const char *p, size_t len, int arg, int shift, int rows, char *out, size_t size) {
    const char *at = p + len;
    for (int n = 0; n < arg; n++) {
        while (*at == ' ' || *at == '\t') at++;
        at += strcspn(at, " \t\r\n");
    }
    while (*at == ' ' || *at == '\t') at++;
    char *end;
    long y = strtol(at, &end, 10);
    if (end == at || y < 0 || (rows > 0 && y >= rows)) return NULL;
    int n = snprintf(out, size, "%.*s%ld%s", (int)(at - p), p, y + shift, end);
    return n >= 0 && (size_t)n < size ? out : NULL;
}

static void planeFileLineHook(const char *line, int decorate, const char *name, int plane) {
    if (!planeFileLoad.active) {
        passMapLine(line, decorate, name, plane);
        return;
    }
    const char *p = line;
    skipSpaces(&p);
    size_t len = strcspn(p, " \t\r\n");
    if (!len || p[0] == '#' || planeFileLoad.stopped) return;
    const char *args = p + len;
    long long n;
    if (spellWordIs(p, len, "version")) {
        if (readSpellNumber(&args, &n)) G32(RVA_MAP_VERSION) = (int)n;
        return;
    }
    if (spellWordIs(p, len, "plane")) {
        if (readSpellNumber(&args, &n) && n != 0) planeFileLoad.stopped = 1;
        return;
    }
    if (spellWordIs(p, len, "mapsize")) return;
    for (size_t i = 0; i < sizeof PLANE_FILE_WORDS / sizeof PLANE_FILE_WORDS[0]; i++)
        if (spellWordIs(p, len, PLANE_FILE_WORDS[i])) {
            static char shifted[PLANE_FILE_LINE_MAX + 16];
            const char *passed = line;
            if (planeFileLoad.yShift || planeFileLoad.rows) {
                passed = shiftMapLineY(p, len, i < 3 ? 0 : 1, planeFileLoad.yShift, planeFileLoad.rows, shifted, sizeof shifted);
                if (!passed) return;
            }
            planeFileLoad.groundRows += i == 1;
            G32(RVA_MAP_PLANE_INDEX) = planeFileLoad.index;
            passMapLine(passed, decorate, name, plane);
            planeFileLoad.lines++;
            return;
        }
    char word[32];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), p);
    planeFileLoad.leftOut++;
    refuseC5mLine(planeFileLoad.rows ? "planepiece" : "planefile", planeFileLoad.p->mod, "map_word_left_out", "plane=%s word=%s", planeFileLoad.p->name, word);
}

// Installs the hook on the map reader's line call, once; 1 when it is in.
static int planeFileHookState;
static int installPlaneFileHook(void) {
    if (!planeFileHookState)
        planeFileHookState = pointCustomPlaneCall(RVA_LOAD_MAP_PLANE_LINE_CALL, RVA_MAP_LINE, (void *)planeFileLineHook, "load_map_plane_line") ? 1 : -1;
    return planeFileHookState > 0;
}

// Copies the w x h squares at (x, y) to or from `buffer`, a column at a time.
static void copyPlaneSquares(int x, int y, int w, int h, unsigned char *buffer, int toBuffer) {
    size_t column = (size_t)h * SQUARE_SIZE;
    for (int c = 0; c < w; c++) {
        unsigned char *square = squareRecordAt(x + c, y);
        if (toBuffer) memcpy(buffer + column * (size_t)c, square, column);
        else memcpy(square, buffer + column * (size_t)c, column);
    }
}

// Reads map file `path` through the game's map reader into record `index`'s rectangle.
static void readMapFileThroughGame(const CustomPlane *p, const char *path, int index, int yShift, int rows) {
    unsigned char hasPlane[PLANE_RECORDS_GAME];
    memcpy(hasPlane, GAME_PTR(RVA_MAP_FILE_HAS_PLANE), sizeof hasPlane);
    int filePlane = G32(RVA_MAP_FILE_PLANE), version = G32(RVA_MAP_VERSION), planeIndex = G32(RVA_MAP_PLANE_INDEX);
    memset(&planeFileLoad, 0, sizeof planeFileLoad);
    planeFileLoad.active = 1, planeFileLoad.index = index, planeFileLoad.p = p, planeFileLoad.yShift = yShift, planeFileLoad.rows = rows;
    if (planeFileLoadForTests) planeFileLoadForTests(path, p->name, -2);
    else GAME_FN(LoadMapPlaneFn, RVA_LOAD_MAP_PLANE)(path, p->name, -2);
    planeFileLoad.active = 0;
    memcpy(GAME_PTR(RVA_MAP_FILE_HAS_PLANE), hasPlane, sizeof hasPlane);
    G32(RVA_MAP_FILE_PLANE) = filePlane, G32(RVA_MAP_VERSION) = version, G32(RVA_MAP_PLANE_INDEX) = planeIndex;
}

// Sets the cold flag of each square of the rectangle from its temperature.
static void setColdFlagsFromTemperatures(int x0, int y0, int w, int h) {
    for (int x = x0; x < x0 + w; x++)
        for (int y = y0; y < y0 + h; y++) {
            size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
            short temperature = (short)G16(RVA_SQUARE_RECORDS + square + SQUARE_TEMPERATURE);
            if (temperature <= SQUARE_COLD_TEMPERATURE_MAX) G32(RVA_SQUARE_FLAGS + square) |= SQUARE_FLAG_COLD;
            else G32(RVA_SQUARE_FLAGS + square) &= ~SQUARE_FLAG_COLD;
        }
}

// Loads plane p's planefile over its fill, then sets its cold squares from the file's temperatures.
static void loadCustomPlaneFile(const CustomPlane *p) {
    int x0 = planeField(p->index, PLANE_X), y0 = planeField(p->index, PLANE_Y);
    if (!installPlaneFileHook()) {
        writeLog("plane_file_skipped", "mod=%s name=%s reason=hook_not_installed", p->mod, p->name);
        return;
    }
    size_t bytes = (size_t)p->w * (size_t)p->h * SQUARE_SIZE;
    unsigned char *host = NULL, *ours = NULL;
    if (y0 > 0) {
        host = malloc(bytes), ours = malloc(bytes);
        if (!host || !ours) {
            free(host), free(ours);
            writeLog("plane_file_skipped", "mod=%s name=%s reason=out_of_memory", p->mod, p->name);
            return;
        }
        copyPlaneSquares(x0, 0, p->w, p->h, host, 1);
        copyPlaneSquares(x0, y0, p->w, p->h, ours, 1);
        copyPlaneSquares(x0, 0, p->w, p->h, ours, 0);
    }
    readMapFileThroughGame(p, p->file, p->index, 0, 0);
    if (y0 > 0) {
        copyPlaneSquares(x0, 0, p->w, p->h, ours, 1);
        copyPlaneSquares(x0, 0, p->w, p->h, host, 0);
        copyPlaneSquares(x0, y0, p->w, p->h, ours, 0);
        free(host), free(ours);
    }
    setColdFlagsFromTemperatures(x0, y0, p->w, p->h);
    writeLog("plane_file_loaded", "mod=%s name=%s index=%d lines=%d left_out=%d stacked=%d file=%s", p->mod, p->name, p->index, planeFileLoad.lines,
             planeFileLoad.leftOut, y0 > 0, p->file);
}

// ---- The recipes' script state ----------------------------------------------------------------------
// A newplane block's recipe runs in a script state CoE5SE makes for the block's mod.
static LuaScript *wordPlaneScripts[CUSTOM_PLANE_TARGET];

static LuaScript *wordPlaneScript(const CustomPlane *p) {
    int open = -1;
    for (int i = 0; i < CUSTOM_PLANE_TARGET; i++) {
        if (wordPlaneScripts[i] && !strcmp(wordPlaneScripts[i]->mod, p->mod)) return wordPlaneScripts[i];
        if (!wordPlaneScripts[i] && open < 0) open = i;
    }
    LuaScript *s = open >= 0 ? calloc(1, sizeof *s) : NULL;
    if (s) snprintf(s->mod, sizeof s->mod, "%s", p->mod);
    if (!s || !(s->L = newLuaSandbox(s))) {
        free(s);
        writeLog("plane_step_skipped", "mod=%s name=%s reason=no_script_state", p->mod, p->name);
        return NULL;
    }
    *(LuaScript **)lua_getextraspace(s->L) = s;
    wordPlaneScripts[open] = s;
    if (!planeRecipesLoaded(s->L, p->mod, "planelike")) s->dead = 1;
    return s;
}

// Adds the words.
static void planeWordsInit(void) {
    int words = apiAddModCommand("newplane", newplaneLine) + apiAddModCommand("planesize", planesizeLine) +
                apiAddModCommand("planefill", planefillLine) + apiAddModCommand("planelike", planelikeLine) +
                apiAddModCommand("planepopulate", planepopulateLine) + apiAddModCommand("planeicon", planeiconLine) +
                apiAddModCommand("planefile", planefileLine) + apiAddModCommand("planeshape", planeshapeLine) +
                apiAddModCommand("planepiece", planepieceLine) + apiAddModCommand("planezone", planezoneLine) +
                apiAddModCommand("planezoneswap", planezoneswapLine) + apiAddModCommand("planezoneunits", planezoneunitsLine) +
                apiAddModCommand("planezonepopulate", planezonepopulateLine) + apiAddModCommand("planeunits", planeunitsLine);
    writeLog("plane_words_installed", "words=%d of 14", words);
}
