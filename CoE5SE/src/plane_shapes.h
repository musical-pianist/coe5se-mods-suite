// Monster shapes on mods' planes: coe5.setPlaneShape, and the game's plane shape step for planes 11 and up.

#define PLANE_SHAPE_NAME_SIZE  (MONSTER_NAME_SIZE + 3)
#define PLANE_SHAPE_ABILITY_MOD 0x10000

typedef struct {
    char mod[MOD_NAME_MAX];
    char monster[PLANE_SHAPE_NAME_SIZE];
    char plane[PLANE_NAME_SIZE];
    char shape[PLANE_SHAPE_NAME_SIZE];
    int monsterIndex;
    int line;
} PlaneShapeDeclaration;

static PlaneShapeDeclaration *planeShapeDeclarations;
static int planeShapeDeclarationCount, planeShapeDeclarationRoom;
static short planeShapeOffsets[CUSTOM_PLANE_TARGET][MONSTER_RECORDS_MOVED];

static PRINTF_FORMAT(4, 5) void refuseLuaCall(const char *mod, const char *call, const char *reason, const char *fmt, ...);

// Adds a declaration: a monster by name (monsterIndex -1) or a monster block's record; NULL or the reason word.
static const char *addPlaneShape(const char *mod, const char *monster, int monsterIndex, const char *plane, const char *shape, int line) {
    if (!shape) shape = "";
    if ((monsterIndex < 0 && !monster[0]) || strlen(monster) >= PLANE_SHAPE_NAME_SIZE || strlen(shape) >= PLANE_SHAPE_NAME_SIZE) return "bad_name";
    if (!plane[0] || strlen(plane) >= PLANE_NAME_SIZE) return "bad_plane";
    for (int i = 0; i < PLANE_RECORDS_GAME; i++)
        if (!strcmp(VANILLA_PLANE_NAMES[i], plane)) return "game_plane";
    if (vanillaPlaneNamed(plane) >= 0) return "game_plane";
    if (planeShapeDeclarationCount == planeShapeDeclarationRoom) {
        int room = planeShapeDeclarationRoom ? 2 * planeShapeDeclarationRoom : 64;
        PlaneShapeDeclaration *more = realloc(planeShapeDeclarations, (size_t)room * sizeof *more);
        if (!more) return "out_of_memory";
        planeShapeDeclarations = more, planeShapeDeclarationRoom = room;
    }
    PlaneShapeDeclaration *d = &planeShapeDeclarations[planeShapeDeclarationCount++];
    snprintf(d->mod, sizeof d->mod, "%s", mod);
    snprintf(d->monster, sizeof d->monster, "%s", monster);
    snprintf(d->plane, sizeof d->plane, "%s", plane);
    snprintf(d->shape, sizeof d->shape, "%s", shape);
    d->monsterIndex = monsterIndex, d->line = line;
    return NULL;
}

// Declares a monster's shape on a mod's plane (coe5.setPlaneShape); NULL when it is taken, or else the reason word.
static const char *declarePlaneShape(const char *mod, const char *monster, const char *plane, const char *shape, int line) {
    return addPlaneShape(mod, monster, -1, plane, shape, line);
}

// Drops the declarations planeshape lines made, at a mod pass's start: the pass reads them again.
static void dropPlaneShapeWords(void) {
    int kept = 0;
    for (int i = 0; i < planeShapeDeclarationCount; i++)
        if (planeShapeDeclarations[i].monsterIndex < 0) planeShapeDeclarations[kept++] = planeShapeDeclarations[i];
    planeShapeDeclarationCount = kept;
}

// Refuses declaration d at the world build: lua_refused for coe5.setPlaneShape's, c5m_refused for a planeshape line's.
static PRINTF_FORMAT(3, 4) void refusePlaneShape(const PlaneShapeDeclaration *d, const char *reason, const char *fmt, ...) {
    char details[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    if (d->monsterIndex < 0) refuseLuaCall(d->mod, "coe5.setPlaneShape", reason, "%s", details);
    else refuseC5mLine("planeshape", d->mod, reason, "line=%d %s", d->line, details);
}

// Fills the table from the declarations, for the planes' indices in this world.
static void resolvePlaneShapes(void) {
    memset(planeShapeOffsets, 0, sizeof planeShapeOffsets);
    if (!planeShapeDeclarationCount) return;
    int set = 0;
    for (int i = 0; i < planeShapeDeclarationCount; i++) {
        const PlaneShapeDeclaration *d = &planeShapeDeclarations[i];
        const CustomPlane *p = findCustomPlane(d->plane);
        if (!p) {
            refusePlaneShape(d, "no_such_plane", "monster=%s plane=%s", d->monster, d->plane);
            continue;
        }
        if (p->index <= PLANE_ELYSIUM2 || p->index > PLANE_INDEX_LAST_MOVED) continue;
        int monster = d->monsterIndex >= 0 ? d->monsterIndex : monsterByName(d->monster);
        if (!monsterRecord(monster)) {
            refusePlaneShape(d, "no_such_monster", "monster=%s plane=%s", d->monster, d->plane);
            continue;
        }
        int shape = d->shape[0] ? monsterByName(d->shape) : monster;
        if (shape < 0) {
            refusePlaneShape(d, "no_such_shape", "monster=%s plane=%s shape=%s", d->monster, d->plane, d->shape);
            continue;
        }
        planeShapeOffsets[p->index - PLANE_ELYSIUM2 - 1][monster] = (short)(shape - monster);
        set++;
    }
    writeLog("plane_shapes_resolved", "declared=%d set=%d", planeShapeDeclarationCount, set);
}

// The offset from `monster` to its shape on plane `plane`, 0 for none or a plane not a mod's.
static int modPlaneShapeOffset(int monster, int plane) {
    if (plane <= PLANE_ELYSIUM2 || plane > PLANE_INDEX_LAST_MOVED || !monsterRecord(monster)) return 0;
    return planeShapeOffsets[plane - PLANE_ELYSIUM2 - 1][monster];
}

#ifndef __APPLE__
static int planeShapeAbilityIds[PLANE_RECORDS_MOVED];

// The step's monster read: a mod's plane's id from the table, the rest from the game.
typedef int (*MonsterAbilityFn)(int monster, int ability);
static int monsterPlaneShapeHook(int monster, int ability) {
    if (ability >= PLANE_SHAPE_ABILITY_MOD) return modPlaneShapeOffset(monster, ability - PLANE_SHAPE_ABILITY_MOD);
    return GAME_FN(MonsterAbilityFn, RVA_MONSTER_ABILITY)(monster, ability);
}

// Points the step's ability id at planeShapeAbilityIds.
static int patchPlaneShapeId(void) {
    static const unsigned char SHIPPED[] = {0x8d, 0xa8, 0xd8, 0x04, 0x00, 0x00};
    unsigned char *at = GAME_PTR(RVA_UNIT_PLANE_SHAPE_ID);
    if (memcmp(at, SHIPPED, sizeof SHIPPED)) {
        writeLog("hook_skipped", "site=plane_shape_id reason=code_differs");
        return 0;
    }
    for (int plane = 0; plane < PLANE_RECORDS_MOVED; plane++)
        planeShapeAbilityIds[plane] = plane <= PLANE_ELYSIUM2 ? MONSTER_ABILITY_ELYSIUMSHAPE + plane : PLANE_SHAPE_ABILITY_MOD + plane;
    unsigned char *stub = allocNear(at);
    if (!stub) {
        writeLog("hook_skipped", "site=plane_shape_id reason=no_near_page");
        return 0;
    }
    unsigned char *p = stub;
    *p++ = 0x44; *p++ = 0x8b; *p++ = 0xd0;
    *p++ = 0x49; *p++ = 0xbb;
    uintptr_t table = (uintptr_t)planeShapeAbilityIds;
    memcpy(p, &table, 8);
    p += 8;
    *p++ = 0x43; *p++ = 0x8b; *p++ = 0x2c; *p++ = 0x93;
    *p++ = 0xc3;
    FlushInstructionCache(GetCurrentProcess(), stub, (size_t)(p - stub));
    unsigned char code[6] = {0xe8, 0, 0, 0, 0, 0x90};
    int rel = (int)(stub - (at + 5));
    memcpy(code + 1, &rel, 4);
    DWORD old;
    if (!VirtualProtect(at, sizeof code, PAGE_EXECUTE_READWRITE, &old)) {
        writeLog("hook_skipped", "site=plane_shape_id reason=code_unwritable");
        return 0;
    }
    memcpy(at, code, sizeof code);
    VirtualProtect(at, sizeof code, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, sizeof code);
    return 1;
}

// Installs the shape step's patch and wrap, with the planes' other hooks; 1 when both are in.
static int installPlaneShapeHooks(void) {
    int ok = patchPlaneShapeId();
    ok += pointCustomPlaneCall(RVA_UNIT_PLANE_SHAPE_CALL, RVA_MONSTER_ABILITY, (void *)monsterPlaneShapeHook, "plane_shape");
    writeLog("plane_shape_hooks_installed", "ok=%d of 2", ok);
    return ok == 2;
}
#else
// The step's ability total: a mod's plane's shape from the table, the rest from the game.
typedef int (*UnitAbilityTotalFn)(int unit, int ability, int add, char *out, int outFlag, int *outAt);
static int unitPlaneShapeHook(int unit, int ability, int add, char *out, int outFlag, int *outAt) {
    int plane = ability - MONSTER_ABILITY_ELYSIUMSHAPE;
    if (plane <= PLANE_ELYSIUM2) return GAME_FN(UnitAbilityTotalFn, RVA_UNIT_ABILITY_TOTAL)(unit, ability, add, out, outFlag, outAt);
    if (unit < 0 || unit > G32(RVA_UNIT_COUNT)) return 0;
    return modPlaneShapeOffset((short)G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE), plane);
}

// Installs the shape step's wrap, with the planes' other hooks; 1 when it is in.
static int installPlaneShapeHooks(void) {
    int ok = pointCustomPlaneCall(RVA_UNIT_PLANE_SHAPE_CALL, RVA_UNIT_ABILITY_TOTAL, (void *)unitPlaneShapeHook, "plane_shape");
    writeLog("plane_shape_hooks_installed", "ok=%d of 1", ok);
    return ok;
}
#endif
