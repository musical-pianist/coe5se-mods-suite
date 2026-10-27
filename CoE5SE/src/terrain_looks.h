// Mods' terrains drawn on the world map like the game's own: a vanilla terrain's ground and decorations.
#pragma once

#define TERRAINLOOKS_WORD          "terrainlooks"
#define TERRAIN_LOOKS_SPRITES_MAX  64
#define TERRAIN_PICTURES_MAX       (SPRITES_PER_KIND - TREE_TRS_SPRITES)
#define TERRAIN_VANILLA_LAST       (TERRAIN_TRS_SPRITES - 1)
#define TERRAIN_LOOKS_FROM_WORDS   1
#define TERRAIN_LOOKS_EARLIER_PASS 2
#define SPRITE_MODE_MAP            4

// A picture a terrain's looks draw: a ground picture, or a decoration sprite's.
typedef struct {
    char path[MAX_PATH];
    int registered;
    int w, h;
    int loaded;
    unsigned char *rgba;
} TerrainPicture;
static TerrainPicture *terrainPictures;
static int terrainPictureCount, terrainPictureCapacity;

typedef struct {
    char mod[MOD_NAME_MAX];
    char file[MAX_PATH];
    int line, script;
    int from;
    int terrain;
    int ground, decor;
    int groundPicture;
    int spriteCount;
    short spriteFrom[TERRAIN_LOOKS_SPRITES_MAX];
    int spritePicture[TERRAIN_LOOKS_SPRITES_MAX];
    unsigned short spriteMap[TREE_TRS_SPRITES];
} TerrainLooks;
static TerrainLooks *terrainLooks;
static int terrainLooksCount, terrainLooksCapacity;

// What the hooks read, by terrain number, rebuilt from the declarations (rebuildTerrainLooks).
static int terrainGroundAs[TERRAIN_TABLE_MAX];
static short terrainLooksAt[TERRAIN_TABLE_MAX];
static int terrainGroundHook, terrainDecorHook;

// A test's stand-ins for the game's functions.
typedef void (*MapDecorationsFn)(int plane, int x, int y, int terrain);
static MapDecorationsFn terrainDecorationsForTests;
static int (*terrainSpriteReplaceForTests)(int kind, int sprite, const char *path);

// ---- Pictures ----------------------------------------------------------------------------------
// A .tga's or .png's width and height from its header: 1, or 0 when the file isn't one of those.
static int readImageSize(const char *path, int *w, int *h) {
    unsigned char head[24];
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t got = fread(head, 1, sizeof head, f);
    fclose(f);
    static const unsigned char PNG[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (got >= 24 && !memcmp(head, PNG, sizeof PNG)) {
        *w = (int)((unsigned)head[16] << 24 | (unsigned)head[17] << 16 | (unsigned)head[18] << 8 | head[19]);
        *h = (int)((unsigned)head[20] << 24 | (unsigned)head[21] << 16 | (unsigned)head[22] << 8 | head[23]);
        return *w > 0 && *h > 0;
    }
    if (got >= 18 && (head[2] == 2 || head[2] == 10)) {
        *w = head[12] | head[13] << 8, *h = head[14] | head[15] << 8;
        return *w > 0 && *h > 0;
    }
    return 0;
}

static int isPowerOfTwo(int n) { return n > 0 && !(n & (n - 1)); }

// The index of picture `path` in the list, added when new; -1 when the list is full.
static int addTerrainPicture(const char *path) {
    for (int i = 0; i < terrainPictureCount; i++)
        if (!strcmp(terrainPictures[i].path, path)) return i;
    if (terrainPictureCount >= TERRAIN_PICTURES_MAX ||
        !growList((void **)&terrainPictures, &terrainPictureCapacity, terrainPictureCount + 1, sizeof *terrainPictures))
        return -1;
    TerrainPicture *t = &terrainPictures[terrainPictureCount];
    memset(t, 0, sizeof *t);
    snprintf(t->path, sizeof t->path, "%s", path);
    return terrainPictureCount++;
}

// Finds picture `spec` in mod folder `folder` and adds it: NULL and its index in *picture, or the reason.
static const char *findTerrainPicture(const char *folder, const char *spec, int ground, int *picture) {
    char path[MAX_PATH];
    const char *reason = strlen(spec) > PLANE_ICON_SPEC_MAX ? "bad_icon" : findModImage(folder, spec, path, sizeof path);
    int w = 0, h = 0;
    if (!reason && !readImageSize(path, &w, &h)) reason = "bad_picture";
    if (!reason && ground && !endsWith(path, ".tga")) reason = "not_a_tga";
    if (!reason && ground && (!isPowerOfTwo(w) || !isPowerOfTwo(h))) reason = "size_not_power_of_two";
    if (!reason && (*picture = addTerrainPicture(path)) < 0) reason = "too_many_pictures";
    if (!reason) terrainPictures[*picture].w = w, terrainPictures[*picture].h = h;
    return reason;
}

// The tree.trs number picture `pic` draws as; -1 when the game refused it.
static int terrainPictureSprite(int pic) {
    TerrainPicture *t = &terrainPictures[pic];
    int sprite = TREE_TRS_SPRITES + pic;
    if (t->registered < 0) return -1;
    int *const *replaced = (int *const *)GAME_PTR(RVA_SPRITE_REPLACED);
    const int *list = replaced[SPRITE_KIND_TREE];
    if (t->registered > 0 && list && list[sprite] >= 0) return sprite;
    int image;
    if (terrainSpriteReplaceForTests) image = terrainSpriteReplaceForTests(SPRITE_KIND_TREE, sprite, t->path);
#ifndef __APPLE__
    else image = GAME_FN(int (*)(int, int, int, const char *), RVA_REPLACE_SPRITE)(SPRITE_KIND_TREE, sprite, 0, t->path);
#else
    else image = GAME_FN(int (*)(int, int, const char *), RVA_REPLACE_SPRITE)(SPRITE_KIND_TREE, sprite, t->path);
#endif
    list = replaced[SPRITE_KIND_TREE];
    if (image < 0 || !list || list[sprite] < 0) {
        t->registered = -1;
        writeLog("terrain_sprite_refused", "sprite=%d path=%s", sprite, t->path);
        return -1;
    }
    t->registered = 1;
    writeLog("terrain_sprite_registered", "sprite=%d path=%s", sprite, t->path);
    return sprite;
}

// ---- Declarations: the words' and coe5.setTerrainLooks ----------------------------------------------
typedef struct {
    int terrain, ground, decor, groundPicture, spriteCount;
    short spriteFrom[TERRAIN_LOOKS_SPRITES_MAX];
    int spritePicture[TERRAIN_LOOKS_SPRITES_MAX];
} TerrainLooksFields;

static void clearTerrainLooksFields(TerrainLooksFields *f) {
    memset(f, 0, sizeof *f);
    f->terrain = f->ground = f->decor = f->groundPicture = -1;
}

static int installTerrainGroundHook(void);
static int installTerrainDecorHook(void);
static int installTerrainCompositeHook(void);
static int installBattleBackgroundHook(void);
static void rebuildTerrainLooks(void);

// The declaration for terrain `terrain` that the running pass or a script made, or NULL.
static TerrainLooks *findTerrainLooks(int terrain) {
    for (int i = 0; i < terrainLooksCount; i++)
        if (terrainLooks[i].terrain == terrain && terrainLooks[i].from != TERRAIN_LOOKS_EARLIER_PASS) return &terrainLooks[i];
    return NULL;
}

// Adds a terrain's looks from a mod's block or script; NULL, or the refusal's reason.
static const char *declareTerrainLooks(const char *mod, const TerrainLooksFields *f, int script, const char *file, int line) {
    if (f->terrain <= TERRAIN_VANILLA_LAST || f->terrain >= TERRAIN_TABLE_MAX) return "not_a_mod_terrain";
    if (f->ground > TERRAIN_VANILLA_LAST || f->decor > TERRAIN_VANILLA_LAST) return "not_a_game_terrain";
    if (f->groundPicture >= 0 && f->ground < 0) return "no_ground";
    if (f->spriteCount && f->decor < 0) return "no_decor";
    if (f->ground < 0 && f->decor < 0) return "no_looks";
    if (findTerrainLooks(f->terrain)) return "looks_taken";
    if (f->ground >= 0 && !installTerrainGroundHook()) return "hook_not_installed";
    if (f->groundPicture >= 0 && (!installTerrainCompositeHook() || !installBattleBackgroundHook())) return "hook_not_installed";
    if (f->decor >= 0 && !installTerrainDecorHook()) return "hook_not_installed";
    int again = -1;
    for (int i = 0; i < terrainLooksCount && again < 0; i++)
        if (terrainLooks[i].terrain == f->terrain && terrainLooks[i].from == TERRAIN_LOOKS_EARLIER_PASS && !strcmp(terrainLooks[i].mod, mod)) again = i;
    if (again < 0 && !growList((void **)&terrainLooks, &terrainLooksCapacity, terrainLooksCount + 1, sizeof *terrainLooks)) return "out_of_memory";
    TerrainLooks *t = &terrainLooks[again >= 0 ? again : terrainLooksCount++];
    memset(t, 0, sizeof *t);
    snprintf(t->mod, sizeof t->mod, "%s", mod);
    snprintf(t->file, sizeof t->file, "%s", file);
    t->line = line, t->script = script, t->from = script ? 0 : TERRAIN_LOOKS_FROM_WORDS;
    t->terrain = f->terrain, t->ground = f->ground, t->decor = f->decor, t->groundPicture = f->groundPicture;
    t->spriteCount = f->spriteCount;
    memcpy(t->spriteFrom, f->spriteFrom, sizeof t->spriteFrom);
    memcpy(t->spritePicture, f->spritePicture, sizeof t->spritePicture);
    for (int i = 0; i < t->spriteCount; i++) t->spriteMap[t->spriteFrom[i]] = (unsigned short)(t->spritePicture[i] + 1);
    writeLog("terrain_looks_declared", "mod=%s terrain=%d ground=%d picture=%s decor=%d sprites=%d script=%d", mod, t->terrain, t->ground,
             t->groundPicture >= 0 ? terrainPictures[t->groundPicture].path : "-", t->decor, t->spriteCount, script);
    rebuildTerrainLooks();
    return NULL;
}

// Rebuilds the tables the hooks read from the declarations.
static void rebuildTerrainLooks(void) {
    for (int t = 0; t < TERRAIN_TABLE_MAX; t++) terrainGroundAs[t] = t;
    memset(terrainLooksAt, 0, sizeof terrainLooksAt);
    for (int i = 0; i < terrainLooksCount; i++) {
        const TerrainLooks *t = &terrainLooks[i];
        if (t->from == TERRAIN_LOOKS_EARLIER_PASS) continue;
        terrainLooksAt[t->terrain] = (short)(i + 1);
        if (t->ground >= 0) terrainGroundAs[t->terrain] = t->ground;
    }
}

// ---- The terrainlooks block --------------------------------------------------------------------
static struct {
    int open, refused, mod, line;
    unsigned int fileId;
    char file[MAX_PATH];
    TerrainLooksFields f;
} looksBlock;

static const char *const TERRAIN_LOOKS_WORDS[] = {"terrainground", "terraindecor", "terrainsprite"};

static int isTerrainLooksWord(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof TERRAIN_LOOKS_WORDS / sizeof TERRAIN_LOOKS_WORDS[0]; i++)
        if (spellWordIs(line, len, TERRAIN_LOOKS_WORDS[i])) return 1;
    return 0;
}

// Refuses a block word's line, logged with its line number.
static void refuseLooksLine(const char *word, int mod, const char *reason, const char *what, const char *args) {
    refuseC5mLine(word, spellModName(mod), reason, "line=%d terrain=%d text=%.60s", G32(RVA_MOD_LINE), looksBlock.f.terrain, args);
    if (!strcmp(reason, "unknown_terrain")) addPlaneProblem(0, spellModName(mod), (const char *)GAME_PTR(RVA_EVENT_FILE), G32(RVA_MOD_LINE), "No terrain called %.60s", what);
    else if (!strcmp(reason, "icon_not_found") || !strcmp(reason, "icon_wrong_case"))
        addPlaneProblem(0, spellModName(mod), (const char *)GAME_PTR(RVA_EVENT_FILE), G32(RVA_MOD_LINE), "No picture '%.60s'", what);
}

// Whether a block word's line applies: level 2, inside an open block that wasn't refused.
static int looksBlockTakes(const char *word, int mod, int level, const char *args) {
    if (level != 2) return 0;
    if (!looksBlock.open || looksBlock.mod != mod) {
        refuseLooksLine(word, mod, "no_looks_block", "", args);
        return 0;
    }
    return !looksBlock.refused;
}

// Ends the open block: declares its looks, or refuses the block.
static void finishLooksBlock(void) {
    if (!looksBlock.open) return;
    looksBlock.open = 0;
    if (looksBlock.refused) return;
    const char *mod = spellModName(looksBlock.mod);
    const char *reason = declareTerrainLooks(mod, &looksBlock.f, 0, looksBlock.file, looksBlock.line);
    if (reason) refuseC5mLine(TERRAINLOOKS_WORD, mod, reason, "line=%d terrain=%d", looksBlock.line, looksBlock.f.terrain);
}

// terrainlooks <terrain>: opens a block for a mod's terrain, and ends the game's blocks and the one before.
static void terrainlooksLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    finishLooksBlock();
    clearParserCurrentRecords();
    memset(&looksBlock, 0, sizeof looksBlock);
    clearTerrainLooksFields(&looksBlock.f);
    looksBlock.open = 1, looksBlock.mod = mod, looksBlock.line = G32(RVA_MOD_LINE), looksBlock.fileId = modFileNow();
    snprintf(looksBlock.file, sizeof looksBlock.file, "%s", (const char *)GAME_PTR(RVA_EVENT_FILE));
    const char *p = args, *why = NULL;
    looksBlock.f.terrain = readPlaneTerrain(&p, &why);
    const char *reason = mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)                  ? "not_in_a_mod"
                       : looksBlock.f.terrain < 0                                 ? why
                       : looksBlock.f.terrain <= TERRAIN_VANILLA_LAST || looksBlock.f.terrain >= TERRAIN_TABLE_MAX ? "not_a_mod_terrain"
                       : NULL;
    if (reason) {
        char what[TERRAIN_NAME_SIZE + 4];
        snprintf(what, sizeof what, "%.*s", (int)strcspn(args, "\r\n"), args);
        looksBlock.refused = 1;
        refuseLooksLine(TERRAINLOOKS_WORD, mod, reason, what, args);
    }
}

// Reads a vanilla terrain for terrainground or terraindecor: its number, or -1 having refused the line.
static int readLooksVanillaTerrain(const char *word, const char **p, int mod, const char *args) {
    const char *start = *p, *why = NULL;
    int terrain = readPlaneTerrain(p, &why);
    if (terrain > TERRAIN_VANILLA_LAST) why = "not_a_game_terrain", terrain = -1;
    if (terrain < 0) {
        char what[TERRAIN_NAME_SIZE + 4];
        snprintf(what, sizeof what, "%.*s", (int)strcspn(start, "\r\n"), start);
        refuseLooksLine(word, mod, why, what, args);
    }
    return terrain;
}

// terrainground <vanilla terrain> ["./picture.tga"]
static void terraingroundLine(const char *args, int mod, int level) {
    if (!looksBlockTakes("terrainground", mod, level, args)) return;
    const char *p = args;
    int ground = readLooksVanillaTerrain("terrainground", &p, mod, args);
    if (ground < 0) return;
    char spec[PLANE_ICON_SPEC_MAX + 2];
    int picture = -1;
    skipSpaces(&p);
    if (*p == '"') {
        int len = readSpellQuoted(&p, spec, sizeof spec);
        const char *reason = len <= 0 ? "no_name" : findTerrainPicture(modFolder(mod), spec, 1, &picture);
        if (reason) {
            refuseLooksLine("terrainground", mod, reason, spec, args);
            return;
        }
    }
    looksBlock.f.ground = ground, looksBlock.f.groundPicture = picture;
}

// terraindecor <vanilla terrain>
static void terraindecorLine(const char *args, int mod, int level) {
    if (!looksBlockTakes("terraindecor", mod, level, args)) return;
    const char *p = args;
    int decor = readLooksVanillaTerrain("terraindecor", &p, mod, args);
    if (decor >= 0) looksBlock.f.decor = decor;
}

// terrainsprite <tree.trs number> "./picture.tga"
static void terrainspriteLine(const char *args, int mod, int level) {
    if (!looksBlockTakes("terrainsprite", mod, level, args)) return;
    const char *p = args;
    long long sprite;
    char spec[PLANE_ICON_SPEC_MAX + 2] = "";
    int picture = -1;
    const char *reason = !readSpellNumber(&p, &sprite) || sprite < 0 || sprite >= TREE_TRS_SPRITES ? "bad_sprite"
                       : looksBlock.f.spriteCount >= TERRAIN_LOOKS_SPRITES_MAX                  ? "too_many_sprites"
                       : readSpellQuoted(&p, spec, sizeof spec) <= 0                            ? "no_name"
                       : findTerrainPicture(modFolder(mod), spec, 0, &picture);
    if (reason) {
        refuseLooksLine("terrainsprite", mod, reason, spec, args);
        return;
    }
    looksBlock.f.spriteFrom[looksBlock.f.spriteCount] = (short)sprite, looksBlock.f.spritePicture[looksBlock.f.spriteCount] = picture;
    looksBlock.f.spriteCount++;
}

// ---- The line hook and the passes -----------------------------------------------------------------
// Reads a terrainlooks block's lines; returns the line to go on with, or NULL when it took it.
static const char *terrainLooksLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2 || !looksBlock.open) return line;
    if (looksBlock.mod != mod || looksBlock.fileId != modFileNow()) {
        finishLooksBlock();
        return line;
    }
    if (spellWordIs(line, len, "end")) {
        finishLooksBlock();
        return NULL;
    }
    if (!isTerrainLooksWord(line, len)) finishLooksBlock();
    return line;
}

// The same when no plugin loaded; returns 1 to skip the line.
static int declinedTerrainLooksLine(const char *line, size_t len, int mod, int level) {
    static int inBlock, blockMod, blockLevel;
    if (mod != blockMod || level != blockLevel) inBlock = 0;
    blockMod = mod, blockLevel = level;
    if (spellWordIs(line, len, TERRAINLOOKS_WORD)) inBlock = 1;
    else if (inBlock && spellWordIs(line, len, "end")) {
        inBlock = 0;
        return 1;
    } else if (!isTerrainLooksWord(line, len)) inBlock = 0;
    return 0;
}

// Before a level-2 mod pass: the pass reads every block again.
static void terrainLooksBeforePass(int level) {
    if (level != 2) return;
    finishLooksBlock();
    for (int i = 0; i < terrainLooksCount; i++)
        if (terrainLooks[i].from) terrainLooks[i].from = TERRAIN_LOOKS_EARLIER_PASS;
}

// After a level-2 mod pass: the last block ends, and the blocks the pass didn't name again go.
static void terrainLooksAfterPass(int level) {
    if (level != 2) return;
    finishLooksBlock();
    int kept = 0;
    for (int i = 0; i < terrainLooksCount; i++) {
        if (terrainLooks[i].from == TERRAIN_LOOKS_EARLIER_PASS) {
            writeLog("terrain_looks_dropped", "mod=%s terrain=%d reason=not_declared_this_pass", terrainLooks[i].mod, terrainLooks[i].terrain);
            continue;
        }
        if (kept != i) terrainLooks[kept] = terrainLooks[i];
        kept++;
    }
    terrainLooksCount = kept;
    rebuildTerrainLooks();
}

// ---- The ground pass ---------------------------------------------------------------------------
// The ground pass reads a terrain with terrainground as its vanilla terrain.
#ifndef __APPLE__
#define GROUND_TERRAIN_SIZE  12
#define GROUND_TERRAIN_FIRST 9

// Writes the Windows stub for the read at `at` into `stub`; returns its length.
static size_t writeTerrainGroundStubWin(unsigned char *stub, const unsigned char *at) {
    unsigned char *p = stub;
    memcpy(p, at, GROUND_TERRAIN_FIRST);
    p += GROUND_TERRAIN_FIRST;
    *p++ = 0x41; *p++ = 0x81; *p++ = 0xf9;
    int max = TERRAIN_TABLE_MAX;
    memcpy(p, &max, 4);
    p += 4;
    *p++ = 0x73; *p++ = 0x0e;
    *p++ = 0x48; *p++ = 0xb8;
    uintptr_t table = (uintptr_t)terrainGroundAs;
    memcpy(p, &table, 8);
    p += 8;
    *p++ = 0x46; *p++ = 0x8b; *p++ = 0x0c; *p++ = 0x88;
    memcpy(p, at + GROUND_TERRAIN_FIRST, GROUND_TERRAIN_SIZE - GROUND_TERRAIN_FIRST);
    p += GROUND_TERRAIN_SIZE - GROUND_TERRAIN_FIRST;
    *p++ = 0xe9;
    int rel = (int)((at + GROUND_TERRAIN_SIZE) - (p + 4));
    memcpy(p, &rel, 4);
    p += 4;
    return (size_t)(p - stub);
}

static int patchTerrainGround(void) {
    unsigned char *at = GAME_PTR(RVA_SET_GROUND_TERRAIN_READ);
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=terrain_ground reason=game_code_differs");
        return 0;
    }
    unsigned char *stub = allocNear(at);
    if (!stub) {
        writeLog("hook_skipped", "site=terrain_ground reason=no_near_page");
        return 0;
    }
    writeTerrainGroundStubWin(stub, at);
    makePagesReadExecute(stub, 0x1000);
    unsigned char code[GROUND_TERRAIN_SIZE];
    memset(code, 0x90, sizeof code);
    code[0] = 0xe9;
    int rel = (int)(stub - (at + 5));
    memcpy(code + 1, &rel, 4);
    if (!writeGameCode(at, code, sizeof code)) {
        writeLog("hook_skipped", "site=terrain_ground reason=code_unwritable");
        return 0;
    }
    return 1;
}
#else
#define TERRAIN_GROUND_STUB_WORDS 10

// Fills the Mac stub for the read at `at`, placed at `stub`.
static void writeTerrainGroundStubMac(uint32_t code[TERRAIN_GROUND_STUB_WORDS], const unsigned char *stub, const unsigned char *at, uint64_t table) {
    code[0] = 0x37f800abu;
    code[1] = 0x7100001fu | ((uint32_t)TERRAIN_TABLE_MAX << 10) | (11u << 5);
    code[2] = 0x54000062u;
    code[3] = 0x580000acu;
    code[4] = 0xb86b598bu;
    memcpy(&code[5], at, 4);
    code[6] = 0x14000000u | ((uint32_t)(((at + 4) - (stub + 24)) >> 2) & 0x3ffffffu);
    code[7] = 0xd503201fu;
    memcpy(&code[8], &table, 8);
}

static int patchTerrainGround(void) {
    unsigned char *at = GAME_PTR(RVA_SET_GROUND_TERRAIN_READ);
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=terrain_ground reason=game_code_differs");
        return 0;
    }
    unsigned char *stub = takePadding(sizeof(uint32_t) * TERRAIN_GROUND_STUB_WORDS);
    if (!stub) {
        writeLog("hook_skipped", "site=terrain_ground reason=no_padding");
        return 0;
    }
    uint32_t code[TERRAIN_GROUND_STUB_WORDS];
    writeTerrainGroundStubMac(code, stub, at, (uint64_t)(uintptr_t)terrainGroundAs);
    uint32_t branch = 0x14000000u | ((uint32_t)((stub - at) >> 2) & 0x3ffffffu);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(at, &branch, 4)) {
        writeLog("hook_skipped", "site=terrain_ground reason=code_unwritable");
        return 0;
    }
    return 1;
}
#endif

// Puts the ground pass's patch in, once, at the first terrainground.
// @platform-split terrain_ground: Windows jumps to a stub in a near page, Mac branches to one in the header padding
static int installTerrainGroundHook(void) {
    if (terrainGroundHook) return terrainGroundHook > 0;
    rebuildTerrainLooks();
    terrainGroundHook = patchTerrainGround() ? 1 : -1;
    writeLog("terrain_ground_hook_installed", "ok=%d of 1", terrainGroundHook > 0);
    return terrainGroundHook > 0;
}

// ---- The decorations ---------------------------------------------------------------------------
// Points the queue's entries first..last of tree.trs sprites that `t` replaces at their pictures.
static void remapTerrainDecor(const TerrainLooks *t, int first, int last) {
    unsigned char *queue = GAME_PTR(RVA_DECOR_QUEUE);
    for (int i = first; i <= last; i++) {
        unsigned char *e = queue + (size_t)i * DECOR_QUEUE_ENTRY_SIZE;
        short kind, sprite;
        memcpy(&kind, e + DECOR_ENTRY_KIND, 2);
        memcpy(&sprite, e + DECOR_ENTRY_SPRITE, 2);
        if (kind != SPRITE_KIND_TREE || sprite < 0 || sprite >= TREE_TRS_SPRITES || !t->spriteMap[sprite]) continue;
        int to = terrainPictureSprite(t->spriteMap[sprite] - 1);
        if (to < 0) continue;
        size_t noted = (size_t)SPRITE_KIND_TREE * SPRITES_PER_KIND + (size_t)to;
        short *widths = (short *)GAME_PTR(RVA_SPRITE_WIDTHS), *heights = (short *)GAME_PTR(RVA_SPRITE_HEIGHTS);
        if (!widths[noted] && !terrainSpriteReplaceForTests) GAME_FN(int (*)(int, int, int), RVA_LOAD_SPRITE)(SPRITE_KIND_TREE, to, SPRITE_MODE_MAP);
        short sixteen = (short)to;
        memcpy(e + DECOR_ENTRY_SPRITE, &sixteen, 2);
        if (widths[noted] <= 0 || heights[noted] <= 0) continue;
        float x, w, h;
        memcpy(&x, e + DECOR_ENTRY_X, 4);
        memcpy(&w, e + DECOR_ENTRY_W, 4);
        memcpy(&h, e + DECOR_ENTRY_H, 4);
        float width = h * (float)widths[noted] / (float)heights[noted];
        x += (w - width) * 0.5f;
        memcpy(e + DECOR_ENTRY_X, &x, 4);
        memcpy(e + DECOR_ENTRY_W, &width, 4);
    }
}

// The map draw's decorations call for one square.
static void terrainDecorationsHook(int plane, int x, int y, int terrain) {
    MapDecorationsFn game = terrainDecorationsForTests ? terrainDecorationsForTests : GAME_FN(MapDecorationsFn, RVA_MAP_DECORATIONS);
    int at = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? terrainLooksAt[terrain] : 0;
    const TerrainLooks *t = at ? &terrainLooks[at - 1] : NULL;
    if (!t || t->decor < 0) {
        game(plane, x, y, terrain);
        return;
    }
    int first = G32(RVA_DECOR_QUEUE_LAST) + 1;
    game(plane, x, y, t->decor);
    if (t->spriteCount) remapTerrainDecor(t, first, G32(RVA_DECOR_QUEUE_LAST));
}

// Points the map draw's decorations call at terrainDecorationsHook, once, at the first terraindecor.
static int installTerrainDecorHook(void) {
    if (terrainDecorHook) return terrainDecorHook > 0;
    rebuildTerrainLooks();
    terrainDecorHook = pointCustomPlaneCall(RVA_MAP_DECORATIONS_CALL, RVA_MAP_DECORATIONS, (void *)terrainDecorationsHook, "map_decorations") ? 1 : -1;
    writeLog("terrain_decor_hook_installed", "ok=%d of 1", terrainDecorHook > 0);
    return terrainDecorHook > 0;
}

// ---- The ground pictures -----------------------------------------------------------------------
// The game's ground painting, with terrainground pictures sampled on their terrains' squares.

static short terrainPaintTags[MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES];
static unsigned char terrainPaintNear[MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES];
static volatile int terrainPaintPlaneArg = -1;
static int terrainPaintAny;
static int terrainCompositeHook;

// Reads ground picture `pic` as the game holds its own; 1 when loaded.
static int loadTerrainGroundPicture(TerrainPicture *t) {
    if (t->loaded > 0) return 1;
    if (t->loaded < 0) return 0;
    t->loaded = -1;
    FILE *f = fopen(t->path, "rb");
    unsigned char head[18];
    if (!f || fread(head, 1, sizeof head, f) != sizeof head || head[1] || (head[2] != 2 && head[2] != 10) || (head[16] != 24 && head[16] != 32)) {
        if (f) fclose(f);
        writeLog("terrain_picture_refused", "path=%s reason=not_a_tga", t->path);
        return 0;
    }
    int w = head[12] | head[13] << 8, h = head[14] | head[15] << 8, n = head[16] / 8;
    size_t pixels = (size_t)w * (size_t)h;
    unsigned char *rgba = isPowerOfTwo(w) && isPowerOfTwo(h) ? malloc(pixels * 4) : NULL;
    int ok = rgba && fseek(f, head[0], SEEK_CUR) == 0;
    unsigned char px[4] = {0, 0, 0, 255};
    for (size_t o = 0; ok && o < pixels;) {
        int count = 1, packet = 0;
        if (head[2] == 10) {
            int c = fgetc(f);
            if (c < 0) ok = 0;
            count = (c & 0x7f) + 1, packet = c & 0x80;
        }
        for (int k = 0; ok && k < count && o < pixels; k++, o++) {
            if (!packet || !k) ok = fread(px, 1, (size_t)n, f) == (size_t)n;
            rgba[o * 4] = px[2], rgba[o * 4 + 1] = px[1], rgba[o * 4 + 2] = px[0], rgba[o * 4 + 3] = n == 4 ? px[3] : 255;
        }
    }
    fclose(f);
    if (!ok) {
        free(rgba);
        writeLog("terrain_picture_refused", "path=%s reason=%s", t->path, rgba ? "cut_short" : "size_not_power_of_two");
        return 0;
    }
    t->rgba = rgba, t->w = w, t->h = h, t->loaded = 1;
    writeLog("terrain_picture_loaded", "path=%s w=%d h=%d", t->path, w, h);
    return 1;
}

// Tags each square of plane `plane` that shows a terrainground picture.
static void tagTerrainPaintSquares(int plane) {
    terrainPaintAny = 0;
    if (plane < 0 || plane >= planeTableRecords()) return;
    int w = G32(RVA_MAP_BG_W_SQUARES), h = G32(RVA_MAP_BG_H_SQUARES), x0 = planeField(plane, PLANE_X), y0 = planeField(plane, PLANE_Y);
    if (w < 1 || h < 1 || w > MAP_COLUMNS_MAX || h > MAP_COLUMN_SQUARES) return;
    const short *grounds = (const short *)GAME_PTR(RVA_MAP_BG_GROUNDS);
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
            terrainPaintTags[at] = 0;
            if (x0 + x < 0 || x0 + x >= MAP_COLUMNS_MAX || y0 + y < 0 || y0 + y >= MAP_COLUMN_SQUARES) continue;
            const unsigned char *square = squareRecordAt(x0 + x, y0 + y);
            short terrain, ground;
            memcpy(&terrain, square + SQUARE_TERRAIN_TYPE, 2);
            memcpy(&ground, square + SQUARE_GROUND, 2);
            int looks = terrain >= 0 && terrain < TERRAIN_TABLE_MAX ? terrainLooksAt[terrain] : 0;
            if (!looks || grounds[at] != ground) continue;
            const TerrainLooks *t = &terrainLooks[looks - 1];
            if (t->groundPicture < 0 || !loadTerrainGroundPicture(&terrainPictures[t->groundPicture])) continue;
            terrainPaintTags[at] = (short)(t->groundPicture + 1);
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

// The picture ground k samples at a pixel of a square; -1 for the game's.
static int terrainSourcePicture(int sx, int sy, int w, int h, int k, int lx, int ly) {
    const short *grounds = (const short *)GAME_PTR(RVA_MAP_BG_GROUNDS);
    size_t self = (size_t)sx * MAP_COLUMN_SQUARES + (size_t)sy;
    if (grounds[self] == k) return terrainPaintTags[self] - 1;
    static const signed char ORDER[8][2] = {{1, 0}, {-1, 0}, {0, -1}, {0, 1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    int best = -1, bestDistance = INT_MAX;
    for (int i = 0; i < 8; i++) {
        int x = sx + ORDER[i][0], y = sy + ORDER[i][1];
        if (x < 0 || y < 0 || x >= w || y >= h) continue;
        size_t at = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
        if (grounds[at] != k) continue;
        int dx = ORDER[i][0] < 0 ? lx + 1 : ORDER[i][0] > 0 ? 64 - lx : 0, dy = ORDER[i][1] < 0 ? ly + 1 : ORDER[i][1] > 0 ? 64 - ly : 0;
        int d = dx > dy ? dx : dy;
        if (d < bestDistance) bestDistance = d, best = terrainPaintTags[at] - 1;
    }
    return best;
}

// The weight rules of the game's composite on w[0..28], in its order; f is the square's flooded bit.
static void groundWeightRules(int w[GROUND_COUNT], int f) {
    if (w[15] < 1 || w[16] < 1) {
        if (w[15] < 70) w[15] = 0;
        if (w[16] < 70) w[16] = 0;
    }
    if (w[13] >= 1 && w[13] <= 74) w[13] = w[13] * 2 - 75 > 0 ? w[13] * 2 - 75 : 0;
    if (w[4] > 0 && w[11] > 0 && w[4] > 35) w[4] = w[4] * 2 - 35 < 100 ? w[4] * 2 - 35 : 100;
    if (w[0] > 0 && w[25] + w[11] + w[13] > 0) {
        int u = w[0] < 100 ? w[0] : 100, to = w[11] < w[13] ? 13 : w[11] < w[25] ? 25 : 11;
        w[6] = u * 2;
        if (u < 50) w[0] = 0, w[11] = 100 - u * 2;
        else w[0] = w[6] - 100, w[6] = 200 - u * 2, w[11] = 0;
        w[13] = 0;
        if (to != 11) w[to] = w[11], w[11] = 0;
    }
    if (w[3] > 0 && w[11] + w[13] > 0) {
        int u = w[3] < 100 ? w[3] : 100, river = w[11] < w[13], v = 0;
        w[6] = u * 2;
        if (u < 50) w[3] = 0, v = 100 - u * 2;
        else w[3] = w[6] - 100, w[6] = 200 - u * 2;
        w[11] = v, w[13] = 0;
        if (river) w[13] = v, w[11] = 0;
    }
    if (w[2] > 0 && w[11] + w[13] > 0) {
        int u = w[2] < 100 ? w[2] : 100, river = w[11] < w[13];
        w[6] = u * 2, w[11] = 100 - u * 2;
        if (u < 50) w[2] = 0;
        else w[2] = w[6] - 100, w[6] = 200 - u * 2, w[11] = 0;
        w[13] = 0;
        if (river) w[13] = w[11], w[11] = 0;
    }
    if (f && w[15] < 1 && w[16] < 1) {
        for (int k = 0; k <= 9; k++) w[k] /= 2;
        w[12] += 50;
    }
    int total = 0;
    for (int k = 0; k < GROUND_COUNT; k++) total += w[k];
    if (total < 100) {
        for (int k = 0; k < GROUND_COUNT; k++)
            if (w[k] > 0 || k == GROUND_COUNT - 1) {
                w[k] += 100 - total;
                break;
            }
    } else if (total > 100) {
        int extra = total - 100;
        for (int k = GROUND_COUNT - 1; k >= 0 && extra >= 1; k--)
            if (w[k] > 0) {
                int d = extra < w[k] ? extra : w[k];
                w[k] -= d, extra -= d;
            }
    }
}

// The game's composite over pixel rows first..last of the image being painted.
static void terrainCompositeRows(int first, int last) {
    int w = G32(RVA_MAP_BG_W_SQUARES), h = G32(RVA_MAP_BG_H_SQUARES), wpx = w * 64, hpx = h * 64;
    unsigned char *image = *(unsigned char **)GAME_PTR(RVA_MAP_BG_IMAGE);
    const unsigned char *solid = GAME_PTR(RVA_MAP_IMAGE_SQUARE_FLAGS);
    const short *grounds = (const short *)GAME_PTR(RVA_MAP_BG_GROUNDS), *flooded = (const short *)GAME_PTR(RVA_MAP_BG_FLOODED);
    unsigned char *const *weights = (unsigned char *const *)GAME_PTR(RVA_GROUND_WEIGHTS);
    const unsigned char *textures = GAME_PTR(RVA_GROUND_TEXTURES);
    if (!image || wpx < 64 || hpx < 1) return;
    for (int py = first; py <= last; py++) {
        int sy = py >> 6, ly = py & 63, dy = py < hpx - 1 ? py : hpx - 1;
        if (dy < 0) dy = 0;
        for (int px = 0; px < wpx; px++) {
            int sx = px >> 6;
            size_t square = (size_t)sx * MAP_COLUMN_SQUARES + (size_t)sy;
            int tagged = terrainPaintAny && terrainPaintNear[square];
            if (solid[square]) {
                int g = grounds[square], pic = tagged ? terrainPaintTags[square] - 1 : -1;
                int dx = px + 64 <= wpx ? px : wpx - 64;
                unsigned char *out = image + ((size_t)dy * (size_t)wpx + (size_t)dx) * 4;
                if (pic >= 0) {
                    const TerrainPicture *t = &terrainPictures[pic];
                    const unsigned char *row = t->rgba + (size_t)(py & (t->h - 1)) * (size_t)t->w * 4;
                    for (int i = 0; i < 64; i++) memcpy(out + i * 4, row + (size_t)((px + i) & (t->w - 1)) * 4, 4);
                } else {
                    const unsigned char *slot = textures + (size_t)g * GROUND_TEXTURE_SIZE;
                    const unsigned char *rgba;
                    int tw, th;
                    memcpy(&rgba, slot, sizeof rgba);
                    memcpy(&tw, slot + GROUND_TEXTURE_W, 4);
                    memcpy(&th, slot + GROUND_TEXTURE_H, 4);
                    int tx = px & (tw - 1);
                    if (tx + 64 > tw) tx = 0;
                    memcpy(out, rgba + ((size_t)(py & (th - 1)) * (size_t)tw + (size_t)tx) * 4, 256);
                }
                px += 63;
                continue;
            }
            size_t i = (size_t)py * (size_t)wpx + (size_t)px;
            int wk[GROUND_COUNT];
            for (int k = 0; k < GROUND_COUNT; k++) wk[k] = weights[k] ? weights[k][i] : 0;
            groundWeightRules(wk, flooded[square] & 1);
            int r = 0, gg = 0, b = 0, a = 25500, acc = 0;
            for (int k = 0; k < GROUND_COUNT; k++) {
                int v = wk[k];
                if (v <= 0) continue;
                acc += v;
                int pic = tagged ? terrainSourcePicture(sx, sy, w, h, k, px & 63, ly) : -1;
                const unsigned char *s;
                if (pic >= 0) {
                    const TerrainPicture *t = &terrainPictures[pic];
                    s = t->rgba + ((size_t)t->w * (size_t)(py & (t->h - 1)) + (size_t)(px & (t->w - 1))) * 4;
                } else {
                    const unsigned char *slot = textures + (size_t)k * GROUND_TEXTURE_SIZE;
                    const unsigned char *rgba;
                    int tw, th;
                    memcpy(&rgba, slot, sizeof rgba);
                    memcpy(&tw, slot + GROUND_TEXTURE_W, 4);
                    memcpy(&th, slot + GROUND_TEXTURE_H, 4);
                    s = rgba + ((size_t)tw * (size_t)(py & (th - 1)) + (size_t)(px & (tw - 1))) * 4;
                }
                r += s[0] * v, gg += s[1] * v, b += s[2] * v, a -= (255 - s[3]) * v;
                if (acc > 99) break;
            }
            int ox = px < wpx - 1 ? px : wpx - 1;
            unsigned char *out = image + ((size_t)dy * (size_t)wpx + (size_t)ox) * 4;
            out[0] = (unsigned char)(r / 100), out[1] = (unsigned char)(gg / 100), out[2] = (unsigned char)(b / 100), out[3] = (unsigned char)(a / 100);
        }
    }
}

// The builder's call to the image: tags the plane's squares, paints, and forgets the plane.
static void terrainBackgroundHook(void) {
    tagTerrainPaintSquares(terrainPaintPlaneArg);
    GAME_FN(void (*)(void), RVA_CREATE_BACKGROUND_IMAGE)();
    terrainPaintAny = 0, terrainPaintPlaneArg = -1;
}

#ifndef __APPLE__
// Points the composite's first instruction and the builder's call at CoE5SE, through a near page.
static int patchTerrainComposite(void) {
    unsigned char *entry = GAME_PTR(RVA_GROUND_COMPOSITE_ROWS), *call = GAME_PTR(RVA_MAP_BG_BUILDER_IMAGE_CALL);
    int rel;
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=terrain_composite reason=game_code_differs");
        return 0;
    }
    unsigned char *stub = allocNear(entry);
    if (!stub) {
        writeLog("hook_skipped", "site=terrain_composite reason=no_near_page");
        return 0;
    }
    unsigned char *p = stub;
    *p++ = 0xff; *p++ = 0x25; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
    void *composite = (void *)terrainCompositeRows, *hook = (void *)terrainBackgroundHook;
    volatile int *plane = &terrainPaintPlaneArg;
    memcpy(p, &composite, 8);
    p = stub + 0x20;
    *p++ = 0x48; *p++ = 0xb8;
    memcpy(p, &plane, 8);
    p += 8;
    *p++ = 0x44; *p++ = 0x89; *p++ = 0x30;
    *p++ = 0x48; *p++ = 0xb8;
    memcpy(p, &hook, 8);
    p += 8;
    *p++ = 0xff; *p++ = 0xe0;
    makePagesReadExecute(stub, 0x1000);
    unsigned char jump[5] = {0xe9};
    rel = (int)(stub - (entry + 5));
    memcpy(jump + 1, &rel, 4);
    int callRel = (int)((stub + 0x20) - (call + 5));
    unsigned char oldCallRel[4];
    memcpy(oldCallRel, call + 1, 4);
    if (!writeGameCode(call + 1, &callRel, 4)) {
        writeLog("hook_skipped", "site=terrain_composite reason=code_unwritable");
        return 0;
    }
    if (!writeGameCode(entry, jump, sizeof jump)) {
        writeGameCode(call + 1, oldCallRel, 4);
        writeLog("hook_skipped", "site=terrain_composite reason=code_unwritable");
        return 0;
    }
    return 1;
}
#else
#define TERRAIN_BACKGROUND_STUB_WORDS 8

// Fills the Mac stub the builder's `bl` goes to, which stores the plane (w23) and branches to the hook.
static void writeTerrainBackgroundStubMac(uint32_t code[TERRAIN_BACKGROUND_STUB_WORDS], uint64_t plane, uint64_t hook) {
    code[0] = 0x58000090u;
    code[1] = 0xb9000217u;
    code[2] = 0x58000090u;
    code[3] = 0xd61f0200u;
    memcpy(&code[4], &plane, 8);
    memcpy(&code[6], &hook, 8);
}

static int patchTerrainComposite(void) {
    unsigned char *entry = GAME_PTR(RVA_GROUND_COMPOSITE_ROWS), *call = GAME_PTR(RVA_MAP_BG_BUILDER_IMAGE_CALL);
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=terrain_composite reason=game_code_differs");
        return 0;
    }
    unsigned char *trampoline = newTrampoline((void *)terrainCompositeRows);
    unsigned char *stub = trampoline ? takePadding(sizeof(uint32_t) * TERRAIN_BACKGROUND_STUB_WORDS) : NULL;
    if (!stub) {
        writeLog("hook_skipped", "site=terrain_composite reason=no_padding");
        return 0;
    }
    uint32_t code[TERRAIN_BACKGROUND_STUB_WORDS];
    writeTerrainBackgroundStubMac(code, (uint64_t)(uintptr_t)&terrainPaintPlaneArg, (uint64_t)(uintptr_t)terrainBackgroundHook);
    uint32_t branch = 0x14000000u | ((uint32_t)((trampoline - entry) >> 2) & 0x3ffffffu);
    uint32_t link = blTo(call, stub);
    if (!writeGameCode(stub, code, sizeof code) || !writeGameCode(call, &link, 4) || !writeGameCode(entry, &branch, 4)) {
        writeLog("hook_skipped", "site=terrain_composite reason=code_unwritable");
        return 0;
    }
    return 1;
}
#endif

// Puts CoE5SE's composite and the builder's hook in, once, at the first terrainground picture.
// @platform-split terrain_composite: Windows jumps through a near page, Mac branches to a trampoline and a stub in the header padding
static int installTerrainCompositeHook(void) {
    if (terrainCompositeHook) return terrainCompositeHook > 0;
    terrainCompositeHook = patchTerrainComposite() ? 1 : -1;
    writeLog("terrain_composite_hook_installed", "ok=%d of 1", terrainCompositeHook > 0);
    return terrainCompositeHook > 0;
}

// ---- coe5.setTerrainLooks ------------------------------------------------------------------------
// The terrain a script names by number or name; -1 for none.
static int luaTerrainArg(lua_State *L, int index) {
    if (lua_isnil(L, index)) return -1;
    if (lua_type(L, index) == LUA_TNUMBER) {
        lua_Integer n = luaL_checkinteger(L, index);
        return n >= 0 && n < TERRAIN_TABLE_MAX ? (int)n : TERRAIN_TABLE_MAX;
    }
    const char *name = luaL_checkstring(L, index);
    int last = G32(RVA_LAST_TERRAIN);
    for (int t = 0; t <= last && t < TERRAIN_TABLE_MAX; t++)
        if (strlen(name) < TERRAIN_NAME_SIZE && recordNameIs(GAME_PTR(RVA_TERRAIN_TABLE) + (size_t)t * TERRAIN_SIZE, TERRAIN_NAME_SIZE, name)) return t;
    return -2;
}

// coe5.setTerrainLooks{terrain =, ground =, picture =, decor =, sprites =}: true, or false when refused.
static int lSetTerrainLooks(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    lua_getfield(L, 1, "terrain");
    lua_getfield(L, 1, "ground");
    lua_getfield(L, 1, "picture");
    lua_getfield(L, 1, "decor");
    lua_getfield(L, 1, "sprites");
    TerrainLooksFields f;
    clearTerrainLooksFields(&f);
    f.terrain = luaTerrainArg(L, 2), f.ground = luaTerrainArg(L, 3), f.decor = luaTerrainArg(L, 5);
    const char *picture = luaL_optstring(L, 4, NULL);
    if (!lua_isnil(L, 6)) luaL_checktype(L, 6, LUA_TTABLE);
    const char *mod = runningLuaMod(), *folder = luaRunning ? luaRunning->folder : "";
    const char *reason = !luaScriptsLoading                         ? "not_at_load"
                       : f.terrain == -1                            ? "no_terrain"
                       : f.terrain == -2 || f.ground == -2 || f.decor == -2 ? "unknown_terrain"
                       : picture                                    ? findTerrainPicture(folder, picture, 1, &f.groundPicture)
                       : NULL;
    char spec[PLANE_ICON_SPEC_MAX + 1] = "";
    for (lua_Integer i = 0; !reason && !lua_isnil(L, 6) && i < TREE_TRS_SPRITES; i++) {
        lua_rawgeti(L, 6, i);
        if (!lua_isnil(L, -1)) {
            snprintf(spec, sizeof spec, "%s", luaL_checkstring(L, -1));
            reason = f.spriteCount >= TERRAIN_LOOKS_SPRITES_MAX ? "too_many_sprites" : findTerrainPicture(folder, spec, 0, &f.spritePicture[f.spriteCount]);
            if (!reason) f.spriteFrom[f.spriteCount++] = (short)i;
        }
        lua_pop(L, 1);
    }
    if (!reason && !lua_isnil(L, 6)) {
        lua_pushnil(L);
        while (!reason && lua_next(L, 6)) {
            if (!lua_isinteger(L, -2) || lua_tointeger(L, -2) < 0 || lua_tointeger(L, -2) >= TREE_TRS_SPRITES) reason = "bad_sprite";
            lua_pop(L, 1);
        }
        if (reason) lua_pop(L, 1);
    }
    lua_Debug caller;
    int line = lua_getstack(L, 1, &caller) && lua_getinfo(L, "l", &caller) ? caller.currentline : 0;
    if (!reason) reason = declareTerrainLooks(mod ? mod : "?", &f, 1, luaRunning ? luaRunning->path : "?", line);
    if (reason) {
        refuseLuaCall(mod, "coe5.setTerrainLooks", reason, "terrain=%.40s ground=%.40s decor=%.40s picture=%.80s sprite=%.80s", luaL_tolstring(L, 2, NULL),
                      luaL_tolstring(L, 3, NULL), luaL_tolstring(L, 5, NULL), picture ? picture : "-", spec[0] ? spec : "-");
        int unknown = f.terrain == -2 ? 2 : f.ground == -2 ? 3 : 5;
        if (!strcmp(reason, "unknown_terrain")) addPlaneProblem(1, mod ? mod : "?", luaRunning ? luaRunning->path : "?", line, "No terrain called '%.60s'", lua_tostring(L, unknown));
        else if (!strcmp(reason, "icon_not_found") || !strcmp(reason, "icon_wrong_case"))
            addPlaneProblem(1, mod ? mod : "?", luaRunning ? luaRunning->path : "?", line, "No picture '%.60s'", spec[0] ? spec : picture ? picture : "-");
    }
    lua_pushboolean(L, !reason);
    return 1;
}

// Adds the words.
static void terrainLooksWordsInit(void) {
    int words = apiAddModCommand("terrainlooks", terrainlooksLine) + apiAddModCommand("terrainground", terraingroundLine) +
                apiAddModCommand("terraindecor", terraindecorLine) + apiAddModCommand("terrainsprite", terrainspriteLine);
    writeLog("terrain_looks_words_installed", "words=%d of 4", words);
}
