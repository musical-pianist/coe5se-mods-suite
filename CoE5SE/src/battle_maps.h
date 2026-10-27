// Mods' battle maps: pools of the game's .coem battle map files, picked per battle by terrain, fort and conditions.
#pragma once

#define BATTLEMAP_WORD               "battlemap"
#define BATTLE_MAP_TERRAINS_MAX      32
#define BATTLE_MAP_FORTS_MAX         16
#define BATTLE_MAP_PLANES_MAX        8
#define BATTLE_MAP_FORT_NAME_MAX     32
#define BATTLE_MAP_WEIGHT_DEFAULT    10
#define BATTLE_MAP_WEIGHT_MAX        1000
#define BATTLE_MAP_PATH_SIZE         0x9c4
#define BATTLE_MAP_MAGIC             0xd8e7c
#define BATTLE_MAPS_FROM_WORDS       1
#define BATTLE_MAPS_EARLIER_PASS     2
#define BATTLE_SEED_SQUARES          1000
#define BATTLE_SCATTER_OBJECTS_MAX   16
#define BATTLE_SCATTER_CHANCE_MAX    1000
#define BATTLE_ROWS                  20
#define BATTLE_COLUMNS_MAX           0x600
#define BATTLE_CELL_SIZE             24
#define BATTLE_CELL_OBJECT           4
#define BATTLE_CELL_FLAGS            8
#define BATTLE_CELL_ROCK_PILLAR      0x100
#define BATTLE_CELL_NO_SCATTER       0x2000000
#define BATTLE_FORT_WALLS            0x200
#define BATTLE_TERRAIN_PLAYER        31

// The conditions battlewhen names, one bit each.
enum {
    BATTLE_WHEN_SIEGE = 1, BATTLE_WHEN_OPEN = 2, BATTLE_WHEN_AMBUSH = 4, BATTLE_WHEN_SEA = 8, BATTLE_WHEN_LAND = 16,
    BATTLE_WHEN_COLD = 32, BATTLE_WHEN_WARM = 64, BATTLE_WHEN_SUMMER = 128
};
static const char *const BATTLE_WHEN_WORDS[] = {"siege", "open", "ambush", "sea", "land", "cold", "warm", "summer", "autumn", "winter", "spring"};
#define BATTLE_WHEN_WORD_COUNT ((int)(sizeof BATTLE_WHEN_WORDS / sizeof BATTLE_WHEN_WORDS[0]))

typedef struct {
    char path[MAX_PATH];
    int terrainCount, fortCount, planeCount;
    int terrains[BATTLE_MAP_TERRAINS_MAX];
    char forts[BATTLE_MAP_FORTS_MAX][BATTLE_MAP_FORT_NAME_MAX];
    char planes[BATTLE_MAP_PLANES_MAX][PLANE_NAME_SIZE];
    int replace, weight, noScatter;
    int groundPictures[GROUND_COUNT];
    unsigned int when;
} BattleMapFields;

typedef struct {
    char mod[MOD_NAME_MAX];
    char file[MAX_PATH];
    int line, script, from;
    BattleMapFields f;
} BattleMap;
static BattleMap *battleMaps;
static int battleMapCount, battleMapCapacity;
static int battleMapHooks;

// A terrain's own battle scatter.
typedef struct {
    int terrain, count;
    char names[BATTLE_SCATTER_OBJECTS_MAX][MONSTER_NAME_SIZE];
    int chances[BATTLE_SCATTER_OBJECTS_MAX];
} BattleScatterFields;

typedef struct {
    char mod[MOD_NAME_MAX];
    char file[MAX_PATH];
    int line, script, from;
    BattleScatterFields f;
} BattleScatter;
static BattleScatter *battleScatters;
static int battleScatterCount, battleScatterCapacity;

// The field the last setup with a square built, for its background.
static struct {
    int valid, x, y, pick;
} battleShown;
static int battleBackgroundHook;

// The field the running setup builds, from its traces.
static struct {
    int active, x, y, pick;
    int snapped, width;
    int objects[BATTLE_COLUMNS_MAX * BATTLE_ROWS];
    unsigned int flags[BATTLE_COLUMNS_MAX * BATTLE_ROWS];
} battleField;

static void clearBattleMapFields(BattleMapFields *f) {
    memset(f, 0, sizeof *f);
    f->weight = BATTLE_MAP_WEIGHT_DEFAULT;
}

// ---- Reading the parts ----------------------------------------------------------------------------
// Whether `path` is a battle map file: its first u32 is the game's magic.
static int isBattleMapFile(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char head[4];
    int ok = fread(head, 1, 4, f) == 4 && (head[0] | head[1] << 8 | head[2] << 16 | (unsigned)head[3] << 24) == BATTLE_MAP_MAGIC;
    fclose(f);
    return ok;
}

// Finds battle map `spec` in mod folder `folder`: NULL and its path, or the reason.
static const char *findBattleMapFile(const char *folder, const char *spec, char *path, size_t size) {
    static const char *const ENDINGS[2] = {".coem", ".coem"};
    static const char *const REASONS[4] = {"bad_file", "file_not_found", "file_wrong_case", "file_link"};
    const char *reason = findModFile(folder, spec, ENDINGS, REASONS, path, size);
    if (!reason && strlen(path) >= BATTLE_MAP_PATH_SIZE) reason = "path_too_long";
    if (!reason && !isBattleMapFile(path)) reason = "not_a_battle_map";
    return reason;
}

// Whether the game has battle map `name` ("citadel"): <data>/batmaps/<name>.coem is there.
static int isGameBattleMap(const char *name) {
    size_t n = strlen(name);
    if (!n || n >= BATTLE_MAP_FORT_NAME_MAX) return 0;
    for (size_t i = 0; i < n; i++)
        if (!(name[i] >= 'a' && name[i] <= 'z') && !(name[i] >= '0' && name[i] <= '9')) return 0;
    char path[MAX_PATH];
    if (!buildPath(path, sizeof path, __func__, "%s/%s.coem", (const char *)GAME_PTR(RVA_BATMAPS_DIR), name)) return 0;
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

// The battlewhen bit of word `word`; 0 for none.
static unsigned int battleWhenBit(const char *word) {
    for (int i = 0; i < BATTLE_WHEN_WORD_COUNT; i++)
        if (!strcmp(word, BATTLE_WHEN_WORDS[i])) return 1u << i;
    return 0;
}

// ---- Declarations: the words' and coe5.addBattleMap -------------------------------------------------
static int installBattleMapHooks(void);

// Adds a battle map from a mod's block or script; NULL, or the refusal's reason.
static const char *declareBattleMap(const char *mod, const BattleMapFields *f, int script, const char *file, int line) {
    if (!f->path[0]) return "no_file";
    if (!f->terrainCount && !f->fortCount) return "no_place";
    if (f->replace && !f->fortCount) return "replace_without_fort";
    if ((f->when & BATTLE_WHEN_SIEGE) && (f->when & BATTLE_WHEN_OPEN)) return "conditions_never_met";
    if ((f->when & BATTLE_WHEN_SEA) && (f->when & BATTLE_WHEN_LAND)) return "conditions_never_met";
    if ((f->when & BATTLE_WHEN_COLD) && (f->when & BATTLE_WHEN_WARM)) return "conditions_never_met";
    unsigned int seasons = f->when / BATTLE_WHEN_SUMMER;
    if (seasons & (seasons - 1)) return "conditions_never_met";
    if (!installBattleMapHooks()) return "hook_not_installed";
    int again = -1;
    for (int i = 0; i < battleMapCount && again < 0; i++)
        if (battleMaps[i].from == BATTLE_MAPS_EARLIER_PASS && !strcmp(battleMaps[i].mod, mod) && !strcmp(battleMaps[i].f.path, f->path)) again = i;
    if (again < 0 && !growList((void **)&battleMaps, &battleMapCapacity, battleMapCount + 1, sizeof *battleMaps)) return "out_of_memory";
    BattleMap *m = &battleMaps[again >= 0 ? again : battleMapCount++];
    memset(m, 0, sizeof *m);
    snprintf(m->mod, sizeof m->mod, "%s", mod);
    snprintf(m->file, sizeof m->file, "%s", file);
    m->line = line, m->script = script, m->from = script ? 0 : BATTLE_MAPS_FROM_WORDS;
    m->f = *f;
    writeLog("battle_map_declared", "mod=%s path=%s terrains=%d forts=%d replace=%d weight=%d when=0x%x planes=%d noscatter=%d script=%d", mod, f->path,
             f->terrainCount, f->fortCount, f->replace, f->weight, f->when, f->planeCount, f->noScatter, script);
    return NULL;
}

// ---- The battlemap block ---------------------------------------------------------------------------
static struct {
    int open, refused, mod, line;
    unsigned int fileId;
    char file[MAX_PATH];
    BattleMapFields f;
} battleBlock;

static const char *const BATTLE_MAP_WORDS[] = {"battleterrain", "battlefort", "battlereplace", "battleweight", "battlewhen", "battleplane", "battlenoscatter", "battleground"};

static int isBattleMapWord(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof BATTLE_MAP_WORDS / sizeof BATTLE_MAP_WORDS[0]; i++)
        if (spellWordIs(line, len, BATTLE_MAP_WORDS[i])) return 1;
    return 0;
}

// Refuses a block word's line, logged with its line number.
static void refuseBattleLine(const char *word, int mod, const char *reason, const char *what, const char *args) {
    refuseC5mLine(word, spellModName(mod), reason, "line=%d text=%.60s", G32(RVA_MOD_LINE), args);
    const char *file = (const char *)GAME_PTR(RVA_EVENT_FILE);
    if (!strcmp(reason, "unknown_terrain")) addPlaneProblem(0, spellModName(mod), file, G32(RVA_MOD_LINE), "No terrain called %.60s", what);
    else if (!strcmp(reason, "file_not_found") || !strcmp(reason, "file_wrong_case"))
        addPlaneProblem(0, spellModName(mod), file, G32(RVA_MOD_LINE), "No file '%.60s'", what);
    else if (!strcmp(reason, "unknown_battle_map")) addPlaneProblem(0, spellModName(mod), file, G32(RVA_MOD_LINE), "No battle map called '%.60s'", what);
}

// Whether a block word's line applies: level 2, inside an open block that wasn't refused.
static int battleBlockTakes(const char *word, int mod, int level, const char *args) {
    if (level != 2) return 0;
    if (!battleBlock.open || battleBlock.mod != mod) {
        refuseBattleLine(word, mod, "no_battlemap_block", "", args);
        return 0;
    }
    return !battleBlock.refused;
}

// Ends the open block: declares its map, or refuses the block.
static void finishBattleBlock(void) {
    if (!battleBlock.open) return;
    battleBlock.open = 0;
    if (battleBlock.refused) return;
    const char *mod = spellModName(battleBlock.mod);
    const char *reason = declareBattleMap(mod, &battleBlock.f, 0, battleBlock.file, battleBlock.line);
    if (reason) refuseC5mLine(BATTLEMAP_WORD, mod, reason, "line=%d path=%s", battleBlock.line, battleBlock.f.path);
}

// battlemap "./file.coem": opens a block, and ends the game's blocks and the one before.
static void battlemapLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    finishBattleBlock();
    clearParserCurrentRecords();
    memset(&battleBlock, 0, sizeof battleBlock);
    clearBattleMapFields(&battleBlock.f);
    battleBlock.open = 1, battleBlock.mod = mod, battleBlock.line = G32(RVA_MOD_LINE), battleBlock.fileId = modFileNow();
    snprintf(battleBlock.file, sizeof battleBlock.file, "%s", (const char *)GAME_PTR(RVA_EVENT_FILE));
    const char *p = args;
    char spec[PLANE_ICON_SPEC_MAX + 2] = "";
    int len = readSpellQuoted(&p, spec, sizeof spec);
    const char *reason = mod < 0 || mod > G32(RVA_MOD_LAST_INDEX) ? "not_in_a_mod"
                       : len <= 0                                 ? "no_name"
                       : findBattleMapFile(modFolder(mod), spec, battleBlock.f.path, sizeof battleBlock.f.path);
    if (reason) {
        battleBlock.refused = 1;
        refuseBattleLine(BATTLEMAP_WORD, mod, reason, spec, args);
    }
}

// battleterrain <terrain>
static void battleterrainLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battleterrain", mod, level, args)) return;
    const char *p = args, *why = NULL;
    int terrain = readPlaneTerrain(&p, &why);
    if (terrain >= TERRAIN_TABLE_MAX) terrain = -1, why = "bad_terrain";
    if (terrain >= 0 && battleBlock.f.terrainCount >= BATTLE_MAP_TERRAINS_MAX) terrain = -1, why = "too_many_terrains";
    if (terrain < 0) {
        char what[TERRAIN_NAME_SIZE + 4];
        snprintf(what, sizeof what, "%.*s", (int)strcspn(args, "\r\n"), args);
        refuseBattleLine("battleterrain", mod, why, what, args);
        return;
    }
    battleBlock.f.terrains[battleBlock.f.terrainCount++] = terrain;
}

// battlefort "<game battle map>"
static void battlefortLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battlefort", mod, level, args)) return;
    const char *p = args;
    char name[BATTLE_MAP_FORT_NAME_MAX + 2] = "";
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = len <= 0                                              ? "no_name"
                       : !isGameBattleMap(name)                                ? "unknown_battle_map"
                       : battleBlock.f.fortCount >= BATTLE_MAP_FORTS_MAX       ? "too_many_forts"
                       : NULL;
    if (reason) {
        refuseBattleLine("battlefort", mod, reason, name, args);
        return;
    }
    snprintf(battleBlock.f.forts[battleBlock.f.fortCount++], BATTLE_MAP_FORT_NAME_MAX, "%s", name);
}

// battlereplace
static void battlereplaceLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battlereplace", mod, level, args)) return;
    battleBlock.f.replace = 1;
}

// battlenoscatter
static void battlenoscatterLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battlenoscatter", mod, level, args)) return;
    battleBlock.f.noScatter = 1;
}

static int installBattleBackgroundHook(void);

// battleground <ground 0 to 28> "./picture.tga"
static void battlegroundLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battleground", mod, level, args)) return;
    const char *p = args;
    long long ground;
    char spec[PLANE_ICON_SPEC_MAX + 2] = "";
    int picture = -1;
    const char *reason = !readSpellNumber(&p, &ground) || ground < 0 || ground >= GROUND_COUNT ? "bad_ground"
                       : readSpellQuoted(&p, spec, sizeof spec) <= 0                           ? "no_name"
                       : findTerrainPicture(modFolder(mod), spec, 1, &picture);
    if (!reason && !installBattleBackgroundHook()) reason = "hook_not_installed";
    if (reason) {
        refuseBattleLine("battleground", mod, !strcmp(reason, "icon_not_found") ? "file_not_found" : reason, spec, args);
        return;
    }
    battleBlock.f.groundPictures[ground] = picture + 1;
}

// battleweight <1 to 1000>
static void battleweightLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battleweight", mod, level, args)) return;
    const char *p = args;
    long long weight;
    if (!readSpellNumber(&p, &weight) || weight < 1 || weight > BATTLE_MAP_WEIGHT_MAX) {
        refuseBattleLine("battleweight", mod, "bad_weight", "", args);
        return;
    }
    battleBlock.f.weight = (int)weight;
}

// battlewhen <condition>
static void battlewhenLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battlewhen", mod, level, args)) return;
    char word[16];
    snprintf(word, sizeof word, "%.*s", (int)strcspn(args, " \t\r\n"), args);
    unsigned int bit = battleWhenBit(word);
    if (!bit) {
        refuseBattleLine("battlewhen", mod, "unknown_condition", word, args);
        return;
    }
    battleBlock.f.when |= bit;
}

// battleplane "<plane>"
static void battleplaneLine(const char *args, int mod, int level) {
    if (!battleBlockTakes("battleplane", mod, level, args)) return;
    const char *p = args;
    char name[PLANE_NAME_SIZE + 2] = "";
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = len <= 0 || len >= PLANE_NAME_SIZE                        ? "no_name"
                       : battleBlock.f.planeCount >= BATTLE_MAP_PLANES_MAX         ? "too_many_planes"
                       : NULL;
    if (reason) {
        refuseBattleLine("battleplane", mod, reason, name, args);
        return;
    }
    snprintf(battleBlock.f.planes[battleBlock.f.planeCount++], PLANE_NAME_SIZE, "%s", name);
    notePlaneName(name, spellModName(mod));
}


// ---- Scatter declarations: the words' and coe5.setBattleScatter -------------------------------------
// The declaration for terrain `terrain` that the running pass or a script made, or NULL.
static BattleScatter *findBattleScatter(int terrain) {
    for (int i = 0; i < battleScatterCount; i++)
        if (battleScatters[i].f.terrain == terrain && battleScatters[i].from != BATTLE_MAPS_EARLIER_PASS) return &battleScatters[i];
    return NULL;
}

// Adds a terrain's battle scatter from a mod's block or script; NULL, or the refusal's reason.
static const char *declareBattleScatter(const char *mod, const BattleScatterFields *f, int script, const char *file, int line) {
    if (f->terrain < 0 || f->terrain >= TERRAIN_TABLE_MAX) return "bad_terrain";
    if (findBattleScatter(f->terrain)) return "scatter_taken";
    if (!installBattleMapHooks()) return "hook_not_installed";
    int again = -1;
    for (int i = 0; i < battleScatterCount && again < 0; i++)
        if (battleScatters[i].from == BATTLE_MAPS_EARLIER_PASS && !strcmp(battleScatters[i].mod, mod) && battleScatters[i].f.terrain == f->terrain) again = i;
    if (again < 0 && !growList((void **)&battleScatters, &battleScatterCapacity, battleScatterCount + 1, sizeof *battleScatters)) return "out_of_memory";
    BattleScatter *b = &battleScatters[again >= 0 ? again : battleScatterCount++];
    memset(b, 0, sizeof *b);
    snprintf(b->mod, sizeof b->mod, "%s", mod);
    snprintf(b->file, sizeof b->file, "%s", file);
    b->line = line, b->script = script, b->from = script ? 0 : BATTLE_MAPS_FROM_WORDS;
    b->f = *f;
    writeLog("battle_scatter_declared", "mod=%s terrain=%d objects=%d script=%d", mod, f->terrain, f->count, script);
    return NULL;
}

// Adds object `name` at `chance` per 1000 cells to the fields; NULL, or the refusal's reason.
static const char *addBattleScatterObject(BattleScatterFields *f, const char *name, long long chance) {
    if (!name[0] || strlen(name) >= MONSTER_NAME_SIZE) return "no_name";
    if (chance < 1 || chance > BATTLE_SCATTER_CHANCE_MAX) return "bad_chance";
    if (f->count >= BATTLE_SCATTER_OBJECTS_MAX) return "too_many_objects";
    snprintf(f->names[f->count], MONSTER_NAME_SIZE, "%s", name);
    f->chances[f->count++] = (int)chance;
    return NULL;
}

// ---- The battlescatter block ------------------------------------------------------------------------
static struct {
    int open, refused, mod, line;
    unsigned int fileId;
    char file[MAX_PATH];
    BattleScatterFields f;
} scatterBlock;

// Ends the open scatter block: declares its scatter, or refuses the block.
static void finishScatterBlock(void) {
    if (!scatterBlock.open) return;
    scatterBlock.open = 0;
    if (scatterBlock.refused) return;
    const char *mod = spellModName(scatterBlock.mod);
    const char *reason = declareBattleScatter(mod, &scatterBlock.f, 0, scatterBlock.file, scatterBlock.line);
    if (reason) refuseC5mLine("battlescatter", mod, reason, "line=%d terrain=%d", scatterBlock.line, scatterBlock.f.terrain);
}

// battlescatter <terrain>: opens a block, and ends the game's blocks and the one before.
static void battlescatterLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    finishBattleBlock();
    finishScatterBlock();
    clearParserCurrentRecords();
    memset(&scatterBlock, 0, sizeof scatterBlock);
    scatterBlock.open = 1, scatterBlock.mod = mod, scatterBlock.line = G32(RVA_MOD_LINE), scatterBlock.fileId = modFileNow();
    snprintf(scatterBlock.file, sizeof scatterBlock.file, "%s", (const char *)GAME_PTR(RVA_EVENT_FILE));
    const char *p = args, *why = NULL;
    scatterBlock.f.terrain = readPlaneTerrain(&p, &why);
    if (scatterBlock.f.terrain >= TERRAIN_TABLE_MAX) scatterBlock.f.terrain = -1, why = "bad_terrain";
    const char *reason = mod < 0 || mod > G32(RVA_MOD_LAST_INDEX) ? "not_in_a_mod" : scatterBlock.f.terrain < 0 ? why : NULL;
    if (reason) {
        char what[TERRAIN_NAME_SIZE + 4];
        snprintf(what, sizeof what, "%.*s", (int)strcspn(args, "\r\n"), args);
        scatterBlock.refused = 1;
        refuseBattleLine("battlescatter", mod, reason, what, args);
    }
}

// battleobject "<monster>" <chance per 1000 cells>
static void battleobjectLine(const char *args, int mod, int level) {
    if (level != 2) return;
    if (!scatterBlock.open || scatterBlock.mod != mod) {
        refuseBattleLine("battleobject", mod, "no_battlescatter_block", "", args);
        return;
    }
    if (scatterBlock.refused) return;
    const char *p = args;
    char name[MONSTER_NAME_SIZE + 2] = "";
    long long chance = 0;
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = len <= 0 ? "no_name" : !readSpellNumber(&p, &chance) ? "bad_chance" : addBattleScatterObject(&scatterBlock.f, name, chance);
    if (reason) refuseBattleLine("battleobject", mod, reason, name, args);
}

// ---- The line hook and the passes -----------------------------------------------------------------
// Reads a battlemap block's lines; returns the line to go on with, or NULL when it took it.
static const char *battleMapLineHook(const char *line, size_t len, int mod, int level) {
    if (level == 2 && scatterBlock.open) {
        if (scatterBlock.mod != mod || scatterBlock.fileId != modFileNow()) finishScatterBlock();
        else if (spellWordIs(line, len, "end")) {
            finishScatterBlock();
            return NULL;
        } else if (!spellWordIs(line, len, "battleobject")) finishScatterBlock();
        return line;
    }
    if (level != 2 || !battleBlock.open) return line;
    if (battleBlock.mod != mod || battleBlock.fileId != modFileNow()) {
        finishBattleBlock();
        return line;
    }
    if (spellWordIs(line, len, "end")) {
        finishBattleBlock();
        return NULL;
    }
    if (!isBattleMapWord(line, len)) finishBattleBlock();
    return line;
}

// The same when no plugin loaded; returns 1 to skip the line.
static int declinedBattleMapLine(const char *line, size_t len, int mod, int level) {
    static int inBlock, blockMod, blockLevel;
    if (mod != blockMod || level != blockLevel) inBlock = 0;
    blockMod = mod, blockLevel = level;
    if (spellWordIs(line, len, BATTLEMAP_WORD) || spellWordIs(line, len, "battlescatter")) inBlock = 1;
    else if (inBlock && spellWordIs(line, len, "end")) {
        inBlock = 0;
        return 1;
    } else if (!isBattleMapWord(line, len) && !spellWordIs(line, len, "battleobject")) inBlock = 0;
    return 0;
}

// Before a level-2 mod pass: the pass reads every block again.
static void battleMapsBeforePass(int level) {
    if (level != 2) return;
    finishBattleBlock();
    finishScatterBlock();
    for (int i = 0; i < battleMapCount; i++)
        if (battleMaps[i].from) battleMaps[i].from = BATTLE_MAPS_EARLIER_PASS;
    for (int i = 0; i < battleScatterCount; i++)
        if (battleScatters[i].from) battleScatters[i].from = BATTLE_MAPS_EARLIER_PASS;
}

// After a level-2 mod pass: the last block ends, and the blocks the pass didn't name again go.
static void battleMapsAfterPass(int level) {
    if (level != 2) return;
    finishBattleBlock();
    int kept = 0;
    for (int i = 0; i < battleMapCount; i++) {
        if (battleMaps[i].from == BATTLE_MAPS_EARLIER_PASS) {
            writeLog("battle_map_dropped", "mod=%s path=%s reason=not_declared_this_pass", battleMaps[i].mod, battleMaps[i].f.path);
            continue;
        }
        if (kept != i) battleMaps[kept] = battleMaps[i];
        kept++;
    }
    battleMapCount = kept;
    finishScatterBlock();
    kept = 0;
    for (int i = 0; i < battleScatterCount; i++) {
        if (battleScatters[i].from == BATTLE_MAPS_EARLIER_PASS) {
            writeLog("battle_scatter_dropped", "mod=%s terrain=%d reason=not_declared_this_pass", battleScatters[i].mod, battleScatters[i].f.terrain);
            continue;
        }
        for (int k = 0; k < battleScatters[i].f.count; k++)
            if (monsterByName(battleScatters[i].f.names[k]) < 0) {
                writeLog("battle_scatter_object_missing", "mod=%s terrain=%d name=%s", battleScatters[i].mod, battleScatters[i].f.terrain, battleScatters[i].f.names[k]);
                addPlaneProblem(battleScatters[i].script, battleScatters[i].mod, battleScatters[i].file, battleScatters[i].line, "No monster called '%.60s'",
                                battleScatters[i].f.names[k]);
            }
        if (kept != i) battleScatters[kept] = battleScatters[i];
        kept++;
    }
    battleScatterCount = kept;
}

// ---- The pick ------------------------------------------------------------------------------------
// The square's mix of x and y, the same on every machine.
static unsigned int battleSquareHash(int x, int y) {
    unsigned int h = (unsigned int)x * BATTLE_SEED_SQUARES + (unsigned int)y;
    h ^= h >> 16, h *= 0x7feb352du, h ^= h >> 15, h *= 0x846ca68bu, h ^= h >> 16;
    return h;
}

// Whether map `m` may serve a battle with conditions `when` on plane `plane`.
static int battleMapFits(const BattleMap *m, unsigned int when, int plane) {
    if ((m->f.when & when) != m->f.when) return 0;
    if (!m->f.planeCount) return 1;
    for (int i = 0; i < m->f.planeCount; i++)
        if (plane >= 0 && planeIndexNamed(m->f.planes[i]) == plane) return 1;
    return 0;
}

static int battleMapNamesTerrain(const BattleMap *m, int terrain) {
    for (int i = 0; i < m->f.terrainCount; i++)
        if (m->f.terrains[i] == terrain) return 1;
    return 0;
}

static int battleMapNamesFort(const BattleMap *m, const char *game) {
    for (int i = 0; i < m->f.fortCount; i++)
        if (!strcmp(m->f.forts[i], game)) return 1;
    return 0;
}

// The map for a battle on square (x, y): a battleMaps index, or -1 for the game's own.
static int pickBattleMap(int x, int y, int terrain, const char *game, unsigned int when, int plane, const char **pool) {
    int total = 0, gameWeight = 0, replaced = 0;
    for (int pass = 0; pass < 2 && !total; pass++) {
        if (pass == 1 && !game[0]) break;
        for (int i = 0; i < battleMapCount; i++) {
            const BattleMap *m = &battleMaps[i];
            int named = pass == 0 ? battleMapNamesTerrain(m, terrain) : battleMapNamesFort(m, game);
            if (!named || !battleMapFits(m, when, plane)) continue;
            total += m->f.weight;
            replaced |= pass == 1 && m->f.replace;
        }
        *pool = pass == 0 ? "terrain" : "fort";
        if (pass == 1 && total && !replaced) gameWeight = BATTLE_MAP_WEIGHT_DEFAULT;
    }
    if (!total) return -1;
    int roll = (int)(battleSquareHash(x, y) % (unsigned int)(total + gameWeight));
    if (roll >= total) return -1;
    int fortPool = !strcmp(*pool, "fort");
    for (int i = 0; i < battleMapCount; i++) {
        const BattleMap *m = &battleMaps[i];
        int named = fortPool ? battleMapNamesFort(m, game) : battleMapNamesTerrain(m, terrain);
        if (!named || !battleMapFits(m, when, plane)) continue;
        if (roll < m->f.weight) return i;
        roll -= m->f.weight;
    }
    return -1;
}

// The game's own pick in path `path` ("<data>/batmaps/citadel.coem"): its name in out[size], "" for none.
static void gameBattleMapName(const char *path, char *out, size_t size) {
    out[0] = '\0';
    const char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
    const char *name = back && (!slash || back > slash) ? back + 1 : slash ? slash + 1 : path;
    size_t n = strlen(name);
    if (n > 5 && !strcmp(name + n - 5, ".coem")) snprintf(out, size, "%.*s", (int)(n - 5), name);
}

// The battle's BATTLEMAP_WHEN bits on square (x, y), from what the setup has set so far.
static unsigned int battleConditions(int x, int y) {
    unsigned int when = 0;
    when |= *(const unsigned char *)GAME_PTR(RVA_BATTLE_IS_SIEGE) ? BATTLE_WHEN_SIEGE : BATTLE_WHEN_OPEN;
    when |= *(const unsigned char *)GAME_PTR(RVA_BATTLE_IS_AMBUSH) ? BATTLE_WHEN_AMBUSH : 0;
    when |= *(const unsigned char *)GAME_PTR(RVA_BATTLE_AT_SEA) ? BATTLE_WHEN_SEA : BATTLE_WHEN_LAND;
    short temperature;
    memcpy(&temperature, squareRecordAt(x, y) + SQUARE_TEMPERATURE, 2);
    when |= temperature < 1 ? BATTLE_WHEN_COLD : BATTLE_WHEN_WARM;
    short season;
    memcpy(&season, GAME_PTR(RVA_SEASON), 2);
    if (season >= 0 && season <= 3) when |= (unsigned int)BATTLE_WHEN_SUMMER << season;
    return when;
}

// ---- The hooks -----------------------------------------------------------------------------------
static const char *battleMapCopyFor(const char *path);
typedef void (*GameTraceFn)(const char *format, ...);
static GameTraceFn battleTraceForTests;
static int (*battlePlaneAtForTests)(int x, int y);
static unsigned int (*battleFortPartsForTests)(int x, int y);

static GameTraceFn battleTrace(void) { return battleTraceForTests ? battleTraceForTests : GAME_FN(GameTraceFn, RVA_TRACE); }

typedef void (*PlaceTerrainUnitFn)(const char *name, int x, int y, int side, unsigned long long flags, int a, int b);
static PlaceTerrainUnitFn battlePlaceForTests;
static void (*battleRemoveForTests)(int unit);

// Cell (x, y) of the battle grid.
static unsigned char *battleCell(int x, int y) {
    return GAME_PTR(RVA_BATTLE_GRID) + ((size_t)x * BATTLE_ROWS + (size_t)y) * BATTLE_CELL_SIZE;
}

// The setup's "newseed" trace: notes the square, then traces as the game does.
static void battleSeedTraceHook(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int seed = va_arg(ap, int);
    va_end(ap);
    battleTrace()(format, seed);
    int x = seed / BATTLE_SEED_SQUARES, y = seed % BATTLE_SEED_SQUARES;
    battleField.active = battleField.snapped = 0, battleField.pick = -1;
    battleShown.valid = 0;
    if (seed < 0 || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return;
    if (*(const unsigned char *)GAME_PTR(RVA_BATTLE_EDITOR_ACTIVE)) return;
    battleField.active = 1, battleField.x = x, battleField.y = y;
    battleShown.valid = 1, battleShown.x = x, battleShown.y = y, battleShown.pick = -1;
}

// The setup's "bfsize" trace: traces as the game does, then puts the picked map's path in the setup's buffer.
static void battleSizeTraceHook(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int width = va_arg(ap, int), attackers = va_arg(ap, int), defenders = va_arg(ap, int), starve = va_arg(ap, int), charge = va_arg(ap, int);
    char *path = va_arg(ap, char *);
    va_end(ap);
    battleTrace()(format, width, attackers, defenders, starve, charge, path);
    if (!battleField.active || !battleMapCount || !path) return;
    int x = battleField.x, y = battleField.y;
    short terrain;
    memcpy(&terrain, squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE, 2);
    char game[BATTLE_MAP_FORT_NAME_MAX];
    gameBattleMapName(path, game, sizeof game);
    const char *pool = "";
    int plane = battlePlaneAtForTests ? battlePlaneAtForTests(x, y) : GAME_FN(int (*)(int, int), RVA_PLANE_AT)(x, y);
    int pick = pickBattleMap(x, y, terrain, game, battleConditions(x, y), plane, &pool);
    battleField.pick = battleShown.pick = pick;
    if (pick < 0) return;
    const char *copy = battleMapCopyFor(battleMaps[pick].f.path);
    snprintf(path, BATTLE_MAP_PATH_SIZE, "%s", copy ? copy : battleMaps[pick].f.path);
    writeLog("battle_map_picked", "mod=%s path=%s x=%d y=%d terrain=%d game=%s pool=%s", battleMaps[pick].mod, battleMaps[pick].f.path, x, y, terrain,
             game[0] ? game : "-", pool);
}

// The scatter's "createtrees" trace: traces as the game does, then notes which cells hold objects.
static void battleTreesTraceHook(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    const char *terrainName = va_arg(ap, const char *);
    va_end(ap);
    battleTrace()(format, terrainName);
    if (!battleField.active) return;
    int width = G32(RVA_BATTLE_WIDTH);
    if (width < 1 || width > BATTLE_COLUMNS_MAX) return;
    for (int x = 0; x < width; x++)
        for (int y = 0; y < BATTLE_ROWS; y++) {
            const unsigned char *cell = battleCell(x, y);
            memcpy(&battleField.objects[x * BATTLE_ROWS + y], cell + BATTLE_CELL_OBJECT, 4);
            memcpy(&battleField.flags[x * BATTLE_ROWS + y], cell + BATTLE_CELL_FLAGS, 4);
        }
    battleField.width = width, battleField.snapped = 1;
}

// Takes the scatter off: each object placed since the snapshot, and the rock pillars' marks; returns how many.
static int clearBattleScatter(void) {
    int removed = 0;
    for (int x = 0; x < battleField.width; x++)
        for (int y = 0; y < BATTLE_ROWS; y++) {
            unsigned char *cell = battleCell(x, y);
            int object, was = battleField.objects[x * BATTLE_ROWS + y];
            unsigned int flags, wasFlags = battleField.flags[x * BATTLE_ROWS + y];
            memcpy(&object, cell + BATTLE_CELL_OBJECT, 4);
            memcpy(&flags, cell + BATTLE_CELL_FLAGS, 4);
            if (object >= 0 && object != was) {
                int none = -1;
                memcpy(cell + BATTLE_CELL_OBJECT, &none, 4);
                if (battleRemoveForTests) battleRemoveForTests(object);
                else GAME_FN(void (*)(int), RVA_REMOVE_UNIT)(object);
                removed++;
            }
            flags = (flags & ~(unsigned int)BATTLE_CELL_ROCK_PILLAR) | (wasFlags & BATTLE_CELL_ROCK_PILLAR);
            memcpy(cell + BATTLE_CELL_FLAGS, &flags, 4);
        }
    return removed;
}

// Places terrain scatter `b` on the field of square (x, y); returns how many objects went in.
static int placeBattleScatter(const BattleScatter *b, int sx, int sy) {
    int width = battleField.width, placed = 0;
    unsigned int parts = battleFortPartsForTests ? battleFortPartsForTests(sx, sy) : (unsigned int)GAME_FN(int (*)(int, int), RVA_SQUARE_FORT_PARTS)(sx, sy);
    int limit = parts & BATTLE_FORT_WALLS ? width / 2 - 2 : width;
    for (int x = 0; x < limit; x++)
        for (int y = 0; y < BATTLE_ROWS; y++) {
            unsigned int flags;
            memcpy(&flags, battleCell(x, y) + BATTLE_CELL_FLAGS, 4);
            if (flags & BATTLE_CELL_NO_SCATTER) continue;
            for (int k = 0; k < b->f.count; k++) {
                unsigned int roll = battleSquareHash(sx * 7919 + x * 31 + k, sy * 1543 + y) % BATTLE_SCATTER_CHANCE_MAX;
                if ((int)roll >= b->f.chances[k]) continue;
                PlaceTerrainUnitFn place = battlePlaceForTests ? battlePlaceForTests : GAME_FN(PlaceTerrainUnitFn, RVA_BATTLE_PLACE_TERRAIN_UNIT);
                int before, after;
                memcpy(&before, battleCell(x, y) + BATTLE_CELL_OBJECT, 4);
                place(b->f.names[k], x, y, -1, 0, 0, 0);
                memcpy(&after, battleCell(x, y) + BATTLE_CELL_OBJECT, 4);
                placed += after >= 0 && after != before;
                break;
            }
        }
    return placed;
}

// The setup's "calculatefort" trace: traces as the game does, then swaps the scatter for the terrain's own or none.
static void battleFortTraceHook(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int units = va_arg(ap, int), width = va_arg(ap, int), max = va_arg(ap, int), indoors = va_arg(ap, int), siege = va_arg(ap, int);
    const char *text = va_arg(ap, const char *);
    va_end(ap);
    battleTrace()(format, units, width, max, indoors, siege, text);
    int snapped = battleField.active && battleField.snapped;
    battleField.active = battleField.snapped = 0;
    if (!snapped) return;
    short terrain;
    memcpy(&terrain, squareRecordAt(battleField.x, battleField.y) + SQUARE_TERRAIN_TYPE, 2);
    const BattleScatter *own = findBattleScatter(terrain);
    int off = battleField.pick >= 0 && battleMaps[battleField.pick].f.noScatter;
    if (!own && !off) return;
    int removed = clearBattleScatter(), placed = own && !off ? placeBattleScatter(own, battleField.x, battleField.y) : 0;
    writeLog("battle_scatter_replaced", "x=%d y=%d terrain=%d removed=%d placed=%d mod=%s", battleField.x, battleField.y, terrain, removed, placed,
             off ? battleMaps[battleField.pick].mod : own->mod);
}

// Points the four trace calls at the hooks, once, at the first battle map or scatter a mod declares.
static int installBattleMapHooks(void) {
    if (battleMapHooks) return battleMapHooks > 0;
    int in = pointCustomPlaneCall(RVA_BATTLE_SEED_TRACE_CALL, RVA_TRACE, (void *)battleSeedTraceHook, "battle_seed_trace") &&
             pointCustomPlaneCall(RVA_BATTLE_SIZE_TRACE_CALL, RVA_TRACE, (void *)battleSizeTraceHook, "battle_size_trace") &&
             pointCustomPlaneCall(RVA_BATTLE_TREES_TRACE_CALL, RVA_TRACE, (void *)battleTreesTraceHook, "battle_trees_trace") &&
             pointCustomPlaneCall(RVA_BATTLE_FORT_TRACE_CALL, RVA_TRACE, (void *)battleFortTraceHook, "battle_fort_trace");
    battleMapHooks = in ? 1 : -1;
    writeLog("battle_map_hooks_installed", "ok=%d of 1", in);
    return in;
}

// ---- Mods' monsters in map files, by name --------------------------------------------------------------
#define BATTLE_NAMES_MAGIC   0x45533543u
#define BATTLE_NAMES_VERSION 1
#define BATTLE_MAP_FILE_MAX  (64 * 1024)
#define BATTLE_OBJECTS_MAX   4000

typedef struct {
    unsigned char *data;
    size_t size, objects, objectsEnd, end;
    int version, ctm, count;
} BattleMapFile;

// Reads battle map file `path` and finds its parts; 1, or 0 when it isn't one CoE5SE can read.
static int readBattleMapFile(const char *path, BattleMapFile *m) {
    memset(m, 0, sizeof *m);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    m->data = malloc(BATTLE_MAP_FILE_MAX);
    m->size = m->data ? fread(m->data, 1, BATTLE_MAP_FILE_MAX, f) : 0;
    fclose(f);
    const unsigned char *d = m->data;
    int u32[5] = {0};
    if (m->size < 20) return 0;
    memcpy(u32, d, 20);
    if ((unsigned int)u32[0] != BATTLE_MAP_MAGIC || u32[1] < 2) return 0;
    m->version = u32[1], m->ctm = u32[2];
    int width = u32[3], height = u32[4];
    size_t at = 20;
    m->objects = at;
    for (;;) {
        short x;
        if (at + 2 > m->size) return 0;
        memcpy(&x, d + at, 2);
        if (x == -1) break;
        if (at + 6 > m->size || m->count >= BATTLE_OBJECTS_MAX) return 0;
        at += 6, m->count++;
    }
    m->objectsEnd = at;
    at += 2;
    if (width < 1 || height < 1 || width > BATTLE_COLUMNS_MAX || height > BATTLE_ROWS) return 0;
    at += (size_t)width * (size_t)height * (m->version > 3 ? 6 : 2);
    short end;
    if (at + 2 > m->size) return 0;
    memcpy(&end, d + at, 2);
    if (end != -555) return 0;
    m->end = at + 2;
    return 1;
}

// The object list's names after the end marker: 1 with the names in names[count][MONSTER_NAME_SIZE], or 0.
static int battleMapNames(const BattleMapFile *m, const char **names) {
    unsigned int head[3];
    if (m->end + sizeof head > m->size) return 0;
    memcpy(head, m->data + m->end, sizeof head);
    if (head[0] != BATTLE_NAMES_MAGIC || head[1] != BATTLE_NAMES_VERSION || (int)head[2] != m->count) return 0;
    if (m->end + sizeof head + (size_t)m->count * MONSTER_NAME_SIZE > m->size) return 0;
    *names = (const char *)(m->data + m->end + sizeof head);
    return 1;
}

static int battleTreeNow(void) { return monsterByName("Tree"); }

// After the editor saves `path`: adds the names of its mods' monsters, when it holds one.
static void addBattleMapNames(const char *path) {
    BattleMapFile m;
    if (!readBattleMapFile(path, &m)) {
        free(m.data);
        writeLog("battle_map_names_skipped", "path=%s reason=not_a_battle_map", path);
        return;
    }
    int tree = battleTreeNow(), mods = 0;
    char (*names)[MONSTER_NAME_SIZE] = calloc((size_t)(m.count ? m.count : 1), MONSTER_NAME_SIZE);
    for (int i = 0; names && i < m.count; i++) {
        short stored;
        memcpy(&stored, m.data + m.objects + (size_t)i * 6 + 4, 2);
        int monster = stored - m.ctm + tree;
        if (monster < FIRST_MOD_MONSTER_SLOT || monster >= tableRecords(TABLE_MONSTER)) continue;
        snprintf(names[i], MONSTER_NAME_SIZE, "%s", (const char *)monsterRecord(monster));
        mods++;
    }
    FILE *f = mods && names && m.end == m.size ? fopen(path, "ab") : NULL;
    if (f) {
        unsigned int head[3] = {BATTLE_NAMES_MAGIC, BATTLE_NAMES_VERSION, (unsigned int)m.count};
        fwrite(head, sizeof head, 1, f);
        fwrite(names, MONSTER_NAME_SIZE, (size_t)m.count, f);
        fclose(f);
    }
    writeLog("battle_map_names_saved", "path=%s objects=%d mod_monsters=%d written=%d", path, m.count, mods, f != NULL);
    free(names);
    free(m.data);
}

// The copy of a battle map to read `path` through, or NULL.
static const char *battleMapCopyFor(const char *path) {
    static char copy[BATTLE_MAP_PATH_SIZE];
    static unsigned int made;
    BattleMapFile m;
    const char *names = NULL;
    if (!readBattleMapFile(path, &m) || !battleMapNames(&m, &names)) {
        free(m.data);
        return NULL;
    }
    int tree = battleTreeNow(), dropped = 0;
    size_t kept = m.objects;
    for (int i = 0; i < m.count; i++) {
        unsigned char *object = m.data + m.objects + (size_t)i * 6;
        const char *name = names + (size_t)i * MONSTER_NAME_SIZE;
        if (name[0]) {
            char text[MONSTER_NAME_SIZE];
            snprintf(text, sizeof text, "%.*s", MONSTER_NAME_SIZE - 1, name);
            int monster = monsterByName(text);
            if (monster < 0) {
                writeLog("battle_map_object_missing", "path=%s name=%s", path, text);
                dropped++;
                continue;
            }
            short number = (short)(monster - tree + m.ctm);
            memcpy(object + 4, &number, 2);
        }
        memmove(m.data + kept, object, 6);
        kept += 6;
    }
    memmove(m.data + kept, m.data + m.objectsEnd, m.end - m.objectsEnd);
    size_t size = kept + (m.end - m.objectsEnd);
    int ok = buildPath(copy, sizeof copy, __func__, "%s" PATH_SEP "coe5se_battle_map_%u.coem", (const char *)GAME_PTR(RVA_TEMP_DIR), made++ % 16);
    FILE *f = ok ? fopen(copy, "wb") : NULL;
    ok = f && fwrite(m.data, 1, size, f) == size;
    if (f) fclose(f);
    free(m.data);
    if (!ok) {
        writeLog("battle_map_copy_skipped", "path=%s reason=temp_not_writable", path);
        return NULL;
    }
    writeLog("battle_map_copy_made", "path=%s copy=%s objects=%d dropped=%d", path, copy, m.count, dropped);
    return copy;
}

#define BACKSLASH 0x5c

// Where the battle map editor's save of `path` goes.
static const char *battleEditorSavePath(const char *path, char *out, size_t size) {
    const char *dir = (const char *)GAME_PTR(RVA_BATMAPS_DIR);
    size_t n = strlen(dir);
    if (strncmp(path, dir, n) || (path[n] != '/' && path[n] != BACKSLASH)) return path;
    const char *rest = path + n + 1, *slash = strchr(rest, '/');
    if (!slash) slash = strchr(rest, BACKSLASH);
    if (!slash) return path;
    const char *file = slash + 1;
    char mod[MOD_NAME_MAX];
    snprintf(mod, sizeof mod, "%.*s", (int)(slash - rest), rest);
    int ok = file[0] && file[0] != '.' && !strchr(file, '/') && !strchr(file, BACKSLASH) && !strstr(file, "..");
    for (const char *c = file; ok && *c; c++)
        ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_' || *c == '-' || *c == '.';
    int found = -1;
    for (int m = 0; ok && found < 0 && m <= G32(RVA_MOD_LAST_INDEX); m++)
        if (!strcmp(modName(m), mod)) found = m;
    const char *reason = !ok ? "bad_file" : found < 0 ? "unknown_mod" : NULL;
    char maps[MAX_PATH];
    if (!reason && !buildPath(maps, sizeof maps, __func__, "%s" PATH_SEP "maps", modFolder(found))) reason = "path_too_long";
    if (!reason) CreateDirectoryA(maps, NULL);
    if (!reason && !buildPath(out, size, __func__, "%s" PATH_SEP "%s", maps, file)) reason = "path_too_long";
    if (reason) {
        writeLog("battle_editor_save_refused", "name=%.*s reason=%s", (int)strlen(rest), rest, reason);
        return NULL;
    }
    writeLog("battle_editor_save_to_mod", "mod=%s path=%s", mod, out);
    return out;
}

// The editor's save: opens the file where its name says, and notes it.
static void *battleSaveFile;
static char battleSavePath[BATTLE_MAP_PATH_SIZE];
typedef void *(*GameFopenFn)(const char *path, const char *mode);
static void *battleEditorSaveOpenHook(const char *path, const char *mode) {
    static char inMod[BATTLE_MAP_PATH_SIZE];
    const char *where = battleEditorSavePath(path, inMod, sizeof inMod);
    void *f = where ? GAME_FN(GameFopenFn, RVA_FOPEN)(where, mode) : NULL;
    battleSaveFile = f;
    snprintf(battleSavePath, sizeof battleSavePath, "%s", where ? where : path);
    return f;
}

// The editor's save closing its file: closes it, then adds the mods' monsters' names.
static int battleEditorSaveCloseHook(void *file) {
    int r = GAME_FN(int (*)(void *), RVA_FCLOSE)(file);
    if (file && file == battleSaveFile) addBattleMapNames(battleSavePath);
    battleSaveFile = NULL;
    return r;
}

#ifndef __APPLE__
// The editor's load opening a file: a map with names through its copy.
static void *battleEditorLoadOpenHook(const char *path, const char *mode) {
    const char *copy = battleMapCopyFor(path);
    return GAME_FN(GameFopenFn, RVA_FOPEN)(copy ? copy : path, mode);
}
#else
// The editor's load: a map with names through its copy (Mac's loader opens the file itself).
static int battleEditorLoadHook(const char *path, int centre, long x, long y) {
    const char *copy = battleMapCopyFor(path);
    return GAME_FN(int (*)(const char *, int, long, long), RVA_LOAD_BATMAP)(copy ? copy : path, centre, x, y);
}
#endif

// The editor's Load Map list: opens in the mods folder instead of the game's batmaps.
typedef int (*BattleBrowserFn)(const char *title, int a, int mode, int b, int c, const char *folder, const char *const *skips, const char *name,
                               char *path, int pathSize);
static int battleEditorBrowseHook(const char *title, int a, int mode, int b, int c, const char *folder, const char *const *skips, const char *name,
                                  char *path, int pathSize) {
    static char start[BATTLE_MAP_PATH_SIZE];
    (void)folder;
    snprintf(start, sizeof start, "%s", (const char *)GAME_PTR(RVA_MODS_DIR));
    return GAME_FN(BattleBrowserFn, RVA_FILE_BROWSER)(title, a, mode, b, c, start, skips, name, path, pathSize);
}

// Points the editor's save and load calls at CoE5SE, at the start.
// @platform-split battle_editor_load: Windows wraps the editor's fopen before RVA_LOAD_BATMAP, Mac the editor's RVA_LOAD_BATMAP call, which opens the file itself
static void installBattleEditorHooks(void) {
    int in = pointCustomPlaneCall(RVA_BATTLE_EDITOR_SAVE_OPEN_CALL, RVA_FOPEN, (void *)battleEditorSaveOpenHook, "battle_editor_save_open") +
             pointCustomPlaneCall(RVA_BATTLE_EDITOR_SAVE_CLOSE_CALL, RVA_FCLOSE, (void *)battleEditorSaveCloseHook, "battle_editor_save_close") +
             pointCustomPlaneCall(RVA_BATTLE_EDITOR_BROWSE_CALL, RVA_FILE_BROWSER, (void *)battleEditorBrowseHook, "battle_editor_browse");
#ifndef __APPLE__
    in += pointCustomPlaneCall(RVA_BATTLE_EDITOR_LOAD_CALL, RVA_FOPEN, (void *)battleEditorLoadOpenHook, "battle_editor_load");
#else
    in += pointCustomPlaneCall(RVA_BATTLE_EDITOR_LOAD_CALL, RVA_LOAD_BATMAP, (void *)battleEditorLoadHook, "battle_editor_load");
#endif
    writeLog("battle_editor_hooks_installed", "ok=%d of 4", in);
}

// ---- The battle's background ----------------------------------------------------------------------
// Tags the battle's cells for CoE5SE's ground pictures.
static void tagBattleCells(void) {
    terrainPaintAny = 0;
    if (!battleShown.valid) return;
    int w = G32(RVA_MAP_BG_W_SQUARES), h = G32(RVA_MAP_BG_H_SQUARES);
    if (w < 1 || h < 1 || w > MAP_COLUMNS_MAX || h > MAP_COLUMN_SQUARES) return;
    const short *grounds = (const short *)GAME_PTR(RVA_MAP_BG_GROUNDS);
    const unsigned char *square = squareRecordAt(battleShown.x, battleShown.y);
    short terrain, own;
    memcpy(&terrain, square + SQUARE_TERRAIN_TYPE, 2);
    memcpy(&own, square + SQUARE_GROUND, 2);
    int looks = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? terrainLooksAt[terrain] : 0;
    int terrainPicture = looks ? terrainLooks[looks - 1].groundPicture : -1;
    const int *mapPictures = battleShown.pick >= 0 && battleShown.pick < battleMapCount ? battleMaps[battleShown.pick].f.groundPictures : NULL;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            int g = grounds[at], pic = -1;
            if (mapPictures && g >= 0 && g < GROUND_COUNT && mapPictures[g]) pic = mapPictures[g] - 1;
            else if (terrainPicture >= 0 && g == own) pic = terrainPicture;
            terrainPaintTags[at] = 0;
            if (pic < 0 || !loadTerrainGroundPicture(&terrainPictures[pic])) continue;
            terrainPaintTags[at] = (short)(pic + 1);
            terrainPaintAny = 1;
        }
    for (int x = 0; x < w && terrainPaintAny; x++)
        for (int y = 0; y < h; y++) {
            int tagged = 0;
            for (int i = x - 1; i <= x + 1 && !tagged; i++)
                for (int j = y - 1; j <= y + 1 && !tagged; j++)
                    tagged = i >= 0 && j >= 0 && i < w && j < h && terrainPaintTags[(size_t)i * MAP_COLUMN_SQUARES + (size_t)j];
            terrainPaintNear[(size_t)x * MAP_COLUMN_SQUARES + (size_t)y] = (unsigned char)tagged;
        }
}

// The battle background's call to the image: tags the cells, paints, and forgets the tags.
static void battleBackgroundImageHook(void) {
    tagBattleCells();
    GAME_FN(void (*)(void), RVA_CREATE_BACKGROUND_IMAGE)();
    terrainPaintAny = 0;
}

// Points the battle background's call to the image at CoE5SE, once, with CoE5SE's composite.
static int installBattleBackgroundHook(void) {
    if (battleBackgroundHook) return battleBackgroundHook > 0;
    int in = installTerrainCompositeHook() &&
             pointCustomPlaneCall(RVA_BATTLE_BG_IMAGE_CALL, RVA_CREATE_BACKGROUND_IMAGE, (void *)battleBackgroundImageHook, "battle_bg_image");
    battleBackgroundHook = in ? 1 : -1;
    writeLog("battle_background_hook_installed", "ok=%d of 1", in);
    return in;
}

// ---- coe5.addBattleMap ---------------------------------------------------------------------------
// Reads list field `field` of the table at 1 into the fields; NULL, or the refusal's reason.
static const char *luaBattleMapList(lua_State *L, const char *field, BattleMapFields *f, const char *folder) {
    lua_getfield(L, 1, field);
    const char *reason = NULL;
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }
    if (!lua_istable(L, -1)) reason = "bad_list";
    lua_Integer n = reason ? 0 : (lua_Integer)lua_rawlen(L, -1);
    for (lua_Integer i = 1; !reason && i <= n; i++) {
        lua_rawgeti(L, -1, i);
        if (!strcmp(field, "terrains")) {
            int t = luaTerrainArg(L, -1);
            reason = t == -2 ? "unknown_terrain" : t < 0 || t >= TERRAIN_TABLE_MAX ? "bad_terrain" : f->terrainCount >= BATTLE_MAP_TERRAINS_MAX ? "too_many_terrains" : NULL;
            if (!reason) f->terrains[f->terrainCount++] = t;
        } else {
            const char *s = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : NULL;
            if (!s) reason = "bad_list";
            else if (!strcmp(field, "forts")) {
                reason = !isGameBattleMap(s) ? "unknown_battle_map" : f->fortCount >= BATTLE_MAP_FORTS_MAX ? "too_many_forts" : NULL;
                if (!reason) snprintf(f->forts[f->fortCount++], BATTLE_MAP_FORT_NAME_MAX, "%s", s);
            } else if (!strcmp(field, "planes")) {
                reason = strlen(s) >= PLANE_NAME_SIZE ? "no_name" : f->planeCount >= BATTLE_MAP_PLANES_MAX ? "too_many_planes" : NULL;
                if (!reason) snprintf(f->planes[f->planeCount++], PLANE_NAME_SIZE, "%s", s);
            } else {
                unsigned int bit = battleWhenBit(s);
                reason = bit ? NULL : "unknown_condition";
                f->when |= bit;
            }
        }
        lua_pop(L, 1);
    }
    (void)folder;
    lua_pop(L, 1);
    return reason;
}

// coe5.addBattleMap{file =, terrains =, forts =, replace =, weight =, when =, planes =, noscatter =, grounds =}: true, or false when refused.
static int lAddBattleMap(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    BattleMapFields f;
    clearBattleMapFields(&f);
    const char *mod = runningLuaMod(), *folder = luaRunning ? luaRunning->folder : "";
    lua_getfield(L, 1, "file");
    const char *spec = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : NULL;
    char specCopy[PLANE_ICON_SPEC_MAX + 1] = "";
    if (spec) snprintf(specCopy, sizeof specCopy, "%s", spec);
    lua_pop(L, 1);
    lua_getfield(L, 1, "replace");
    f.replace = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 1, "noscatter");
    f.noScatter = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, 1, "weight");
    int weightGiven = !lua_isnil(L, -1);
    lua_Integer weight = lua_isinteger(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    const char *reason = !luaScriptsLoading                                             ? "not_at_load"
                       : !spec                                                          ? "no_file"
                       : weightGiven && (weight < 1 || weight > BATTLE_MAP_WEIGHT_MAX)  ? "bad_weight"
                       : findBattleMapFile(folder, specCopy, f.path, sizeof f.path);
    if (weightGiven && !reason) f.weight = (int)weight;
    static const char *const LISTS[] = {"terrains", "forts", "when", "planes"};
    for (size_t i = 0; !reason && i < sizeof LISTS / sizeof LISTS[0]; i++) reason = luaBattleMapList(L, LISTS[i], &f, folder);
    lua_getfield(L, 1, "grounds");
    if (!reason && !lua_isnil(L, -1) && !lua_istable(L, -1)) reason = "bad_list";
    for (int g = 0; !reason && lua_istable(L, -1) && g < GROUND_COUNT; g++) {
        lua_rawgeti(L, -1, g);
        if (!lua_isnil(L, -1)) {
            int picture = -1;
            reason = lua_type(L, -1) != LUA_TSTRING ? "bad_list" : findTerrainPicture(folder, lua_tostring(L, -1), 1, &picture);
            if (!reason && !installBattleBackgroundHook()) reason = "hook_not_installed";
            if (!reason) f.groundPictures[g] = picture + 1;
        }
        lua_pop(L, 1);
    }
    if (!reason && lua_istable(L, -1)) {
        lua_pushnil(L);
        while (!reason && lua_next(L, -2)) {
            if (!lua_isinteger(L, -2) || lua_tointeger(L, -2) < 0 || lua_tointeger(L, -2) >= GROUND_COUNT) reason = "bad_ground";
            lua_pop(L, 1);
        }
        if (reason) lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_Debug caller;
    int line = lua_getstack(L, 1, &caller) && lua_getinfo(L, "l", &caller) ? caller.currentline : 0;
    if (!reason) {
        for (int i = 0; i < f.planeCount; i++) notePlaneNameAt(f.planes[i], mod ? mod : "?", luaRunning ? luaRunning->path : "?", line, 1);
        reason = declareBattleMap(mod ? mod : "?", &f, 1, luaRunning ? luaRunning->path : "?", line);
    }
    if (reason) {
        refuseLuaCall(mod, "coe5.addBattleMap", reason, "file=%.80s", specCopy[0] ? specCopy : "-");
        if (!strcmp(reason, "file_not_found") || !strcmp(reason, "file_wrong_case"))
            addPlaneProblem(1, mod ? mod : "?", luaRunning ? luaRunning->path : "?", line, "No file '%.60s'", specCopy);
    }
    lua_pushboolean(L, !reason);
    return 1;
}

// coe5.setBattleScatter{terrain =, objects = {{"<monster>", chance}, ...}}: true, or false when refused.
static int lSetBattleScatter(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    BattleScatterFields f;
    memset(&f, 0, sizeof f);
    lua_getfield(L, 1, "terrain");
    lua_getfield(L, 1, "objects");
    f.terrain = luaTerrainArg(L, 2);
    const char *mod = runningLuaMod();
    const char *reason = !luaScriptsLoading  ? "not_at_load"
                       : f.terrain == -1     ? "no_terrain"
                       : f.terrain == -2     ? "unknown_terrain"
                       : !lua_isnil(L, 3) && !lua_istable(L, 3) ? "bad_list"
                       : NULL;
    lua_Integer n = !reason && lua_istable(L, 3) ? (lua_Integer)lua_rawlen(L, 3) : 0;
    for (lua_Integer i = 1; !reason && i <= n; i++) {
        lua_rawgeti(L, 3, i);
        if (!lua_istable(L, -1)) reason = "bad_list";
        else {
            lua_rawgeti(L, -1, 1);
            lua_rawgeti(L, -2, 2);
            const char *name = lua_type(L, -2) == LUA_TSTRING ? lua_tostring(L, -2) : NULL;
            reason = !name || !lua_isinteger(L, -1) ? "bad_list" : addBattleScatterObject(&f, name, (long long)lua_tointeger(L, -1));
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
    }
    lua_Debug caller;
    int line = lua_getstack(L, 1, &caller) && lua_getinfo(L, "l", &caller) ? caller.currentline : 0;
    if (!reason) reason = declareBattleScatter(mod ? mod : "?", &f, 1, luaRunning ? luaRunning->path : "?", line);
    if (reason) {
        refuseLuaCall(mod, "coe5.setBattleScatter", reason, "terrain=%.40s", luaL_tolstring(L, 2, NULL));
        if (!strcmp(reason, "unknown_terrain")) addPlaneProblem(1, mod ? mod : "?", luaRunning ? luaRunning->path : "?", line, "No terrain called '%.60s'", lua_tostring(L, 2));
    }
    lua_pushboolean(L, !reason);
    return 1;
}

// Adds the words.
static void battleMapWordsInit(void) {
    int words = apiAddModCommand("battlemap", battlemapLine) + apiAddModCommand("battleterrain", battleterrainLine) +
                apiAddModCommand("battlefort", battlefortLine) + apiAddModCommand("battlereplace", battlereplaceLine) +
                apiAddModCommand("battleweight", battleweightLine) + apiAddModCommand("battlewhen", battlewhenLine) +
                apiAddModCommand("battleplane", battleplaneLine) + apiAddModCommand("battlenoscatter", battlenoscatterLine) +
                apiAddModCommand("battlescatter", battlescatterLine) + apiAddModCommand("battleobject", battleobjectLine) +
                apiAddModCommand("battleground", battlegroundLine);
    writeLog("battle_map_words_installed", "words=%d of 11", words);
    installBattleEditorHooks();
}
