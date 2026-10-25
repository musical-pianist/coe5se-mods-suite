// coe5.onPortalCrossing: scripts may refuse an army's way through a portal.

// One crossing, as the handlers see it. A field below 0 is nil to them.
typedef struct {
    int x, y;
    int toX, toY;
    int portal;
    int player, commander;
    int planning;
} PortalCrossing;

static int *portalHandlers;
static int portalHandlerCount, portalHandlerCapacity;
// 1 while a handler runs: the crossings it causes aren't asked, and coe5.route refuses.
static int portalHandlerRunning;
static int portalHandlerPlanning;
static int portalHooksState;

// The game's functions the hooks call on, set by installPortalHooks (the unit tests set their own).
typedef int (*PortalMoveStepFn)(int commander, int x, int y);
typedef int (*PortalExitFn)(int portal, int *x, int *y, int x0, int y0);
typedef int (*PortalFarSquareFn)(int commander, int x, int y, int flag);
static PortalMoveStepFn portalGameMoveStep;
static PortalExitFn portalGameExit;
static PortalFarSquareFn portalGameFarSquare;

// Set by the stubs below, read by the hooks they lead to.
__attribute__((used)) int coe5sePortalSpreadFrom[2];
__attribute__((used)) int coe5sePortalRouteCommander = -1;
__attribute__((used)) void *coe5sePortalGameWaytogo;
__attribute__((used)) void *coe5sePortalTraceB;

// ---- The game's records ------------------------------------------------------------------------
// The SQUARE_PORTAL of square (x, y), 0 off the map.
static int portalNumberAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    return G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_PORTAL);
}

// The unit of commander c, or -1.
static int portalCommanderUnit(int commander) {
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX || commander > G32(RVA_LAST_COMMANDER)) return -1;
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    return unit >= 0 && unit < UNIT_TABLE_MAX ? unit : -1;
}

// The player commander c's unit belongs to, or -1.
static int portalCommanderPlayer(int commander) {
    int unit = portalCommanderUnit(commander);
    return unit < 0 ? -1 : G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
}

// Whether the game's step would take commander c to (x, y) through a portal; fills `out` if so.
static int portalStepCrossing(int commander, int x, int y, PortalCrossing *out) {
    int unit = portalCommanderUnit(commander);
    if (unit < 0) return 0;
    int owner = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
    int fromX = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), fromY = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
    if (owner < 0 || owner >= PLAYER_SLOTS || (abs(x - fromX) <= 1 && abs(y - fromY) <= 1)) return 0;
    int from = portalNumberAt(fromX, fromY), to = portalNumberAt(x, y);
    if (from <= 0 || from != abs(to)) return 0;
    if (to > 0 && (signed char)G8(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_SEEN + (size_t)owner) <= 0)
        return 0;
    *out = (PortalCrossing){fromX, fromY, x, y, from, owner, commander, 0};
    return 1;
}

// ---- Asking the handlers -----------------------------------------------------------------------
typedef struct { int ref; const PortalCrossing *crossing; } PortalHandlerCall;

static void setPortalField(lua_State *L, const char *name, int value) {
    if (value < 0) return;
    lua_pushinteger(L, value);
    lua_setfield(L, -2, name);
}

// callLuaProtected's push: the handler and its crossing table.
static int pushPortalHandlerCall(lua_State *L, void *ctx) {
    const PortalHandlerCall *call = ctx;
    const PortalCrossing *c = call->crossing;
    lua_rawgeti(L, LUA_REGISTRYINDEX, call->ref);
    lua_createtable(L, 0, 8);
    setPortalField(L, "x", c->x);
    setPortalField(L, "y", c->y);
    setPortalField(L, "toX", c->toX);
    setPortalField(L, "toY", c->toY);
    setPortalField(L, "portal", c->portal);
    setPortalField(L, "player", c->player);
    setPortalField(L, "commander", c->commander);
    lua_pushboolean(L, c->planning);
    lua_setfield(L, -2, "planning");
    return 1;
}

// Asks each handler in turn; 0 once one returns false.
static int portalCrossingAllowed(const PortalCrossing *c) {
    if (!portalHandlerCount || portalHandlerRunning || isNetworkGame()) return 1;
    int allowed = 1;
    portalHandlerRunning = 1;
    portalHandlerPlanning = c->planning;
    for (int i = 0; allowed && i < portalHandlerCount; i++) {
        LuaCallback cb = luaCallbacks[portalHandlers[i]];
        if (!cb.s || cb.s->dead) continue;
        PortalHandlerCall call = {cb.ref, c};
        if (!callLuaProtected(cb.s, pushPortalHandlerCall, &call, 1)) continue;
        allowed = !(lua_type(cb.s->L, -1) == LUA_TBOOLEAN && !lua_toboolean(cb.s->L, -1));
        lua_pop(cb.s->L, 1);
        if (!allowed && !c->planning)
            writeLog("portal_crossing_refused", "mod=%s x=%d y=%d to_x=%d to_y=%d portal=%d player=%d commander=%d", cb.s->mod, c->x,
                     c->y, c->toX, c->toY, c->portal, c->player, c->commander);
        closeDeadLuaScript(cb.s);
    }
    portalHandlerRunning = portalHandlerPlanning = 0;
    return allowed;
}

// ---- The hooks ---------------------------------------------------------------------------------
// The step's four callers: the game's step, unless it is a crossing a handler refuses.
static int portalMoveStepHook(int commander, int x, int y) {
    PortalCrossing c;
    if (portalHandlerCount && portalStepCrossing(commander, x, y, &c) && !portalCrossingAllowed(&c)) return MOVE_STEP_REFUSED;
    return portalGameMoveStep(commander, x, y);
}

// Enter Gateway's exit: asked before it is drawn; refused, no exit, as when the game finds none.
static int portalGatewayExitHook(int portal, int *x, int *y, int x0, int y0) {
    if (portalHandlerCount) {
        int commander = G32(RVA_SELECTED_COMMANDER);
        PortalCrossing c = {x0, y0, -1, -1, portal, portalCommanderPlayer(commander), commander, 0};
        if (!portalCrossingAllowed(&c)) {
            *x = *y = -1;
            return 0;
        }
    }
    return portalGameExit(portal, x, y, x0, y0);
}

// An independent army's random exit: refused, no exit, and the army doesn't move on that draw.
static int portalIndepExitHook(int portal, int *x, int *y, int x0, int y0) {
    int found = portalGameExit(portal, x, y, x0, y0);
    if (!portalHandlerCount || *x < 0 || *y < 0) return found;
    int player = G16(RVA_SQUARE_RECORDS + ((size_t)x0 * MAP_COLUMN_SQUARES + (size_t)y0) * SQUARE_SIZE + SQUARE_ARMY_OWNER);
    PortalCrossing c = {x0, y0, *x, *y, portal, player, -1, 0};
    if (portalCrossingAllowed(&c)) return found;
    *x = *y = -1;
    return 0;
}

// The spread's exit check (from coe5sePortalSpreadStub).
int coe5sePortalSpreadCheck(int commander, int x, int y, int flag);
__attribute__((used)) int coe5sePortalSpreadCheck(int commander, int x, int y, int flag) {
    int r = portalGameFarSquare(commander, x, y, flag);
    if (r <= 0 || !portalHandlerCount) return r;
    int fromX = coe5sePortalSpreadFrom[0], fromY = coe5sePortalSpreadFrom[1];
    PortalCrossing c = {fromX, fromY, x, y, portalNumberAt(fromX, fromY), portalCommanderPlayer(commander), commander, 1};
    return portalCrossingAllowed(&c) ? r : 0;
}

// The walk back's portal predecessor (from coe5sePortalBackStub): 0 skips it.
int coe5sePortalBackCheck(int px, int py, int sx, int sy);
__attribute__((used)) int coe5sePortalBackCheck(int px, int py, int sx, int sy) {
    if (!portalHandlerCount) return 1;
    int commander = coe5sePortalRouteCommander;
    PortalCrossing c = {px, py, sx, sy, portalNumberAt(px, py), portalCommanderPlayer(commander), commander, 1};
    return portalCrossingAllowed(&c);
}

// ---- The stubs ---------------------------------------------------------------------------------
// The three calls whose hooks need a register the call doesn't pass.
void coe5sePortalRouteStub(void);
void coe5sePortalSpreadStub(void);
void coe5sePortalBackStub(void);
#ifdef __APPLE__
// @platform-split portal_spread: Mac's spread is inlined into waytogo, with two copies of its exit loop and of the walk back's
__asm__(".text\n.p2align 2\n"
        "_coe5sePortalRouteStub:\n"
        "  adrp x16, _coe5sePortalRouteCommander@PAGE\n"
        "  str w0, [x16, _coe5sePortalRouteCommander@PAGEOFF]\n"
        "  adrp x16, _coe5sePortalGameWaytogo@PAGE\n"
        "  ldr x16, [x16, _coe5sePortalGameWaytogo@PAGEOFF]\n"
        "  br x16\n"
        "_coe5sePortalSpreadStub:\n"
        "  adrp x16, _coe5sePortalSpreadFrom@PAGE\n"
        "  add x16, x16, _coe5sePortalSpreadFrom@PAGEOFF\n"
        "  str w19, [x16]\n"
        "  ldur w17, [x29, #-0xa8]\n"
        "  str w17, [x16, #4]\n"
        "  b _coe5sePortalSpreadCheck\n"
        "_coe5sePortalBackStub:\n"
        "  sub sp, sp, #0x60\n"
        "  stp x29, x30, [sp, #0x50]\n"
        "  stp x0, x1, [sp]\n"
        "  stp x2, x3, [sp, #0x10]\n"
        "  stp x4, x5, [sp, #0x20]\n"
        "  stp x6, x7, [sp, #0x30]\n"
        "  str x8, [sp, #0x40]\n"
        "  ldr w0, [sp, #0x60]\n"
        "  ldr w1, [sp, #0x68]\n"
        "  ldur w2, [x29, #-0x60]\n"
        "  ldur w3, [x29, #-0x80]\n"
        "  add x29, sp, #0x50\n"
        "  bl _coe5sePortalBackCheck\n"
        "  cbnz w0, 1f\n"
        "  mov w25, #-1\n"
        "1:\n"
        "  ldp x0, x1, [sp]\n"
        "  ldp x2, x3, [sp, #0x10]\n"
        "  ldp x4, x5, [sp, #0x20]\n"
        "  ldp x6, x7, [sp, #0x30]\n"
        "  ldr x8, [sp, #0x40]\n"
        "  ldp x29, x30, [sp, #0x50]\n"
        "  add sp, sp, #0x60\n"
        "  adrp x16, _coe5sePortalTraceB@PAGE\n"
        "  ldr x16, [x16, _coe5sePortalTraceB@PAGEOFF]\n"
        "  br x16\n");
#else
__asm__(".text\n"
        ".globl coe5sePortalRouteStub\n"
        ".def coe5sePortalRouteStub; .scl 2; .type 32; .endef\n"
        ".p2align 4\n"
        "coe5sePortalRouteStub:\n"
        "    movl %ecx, coe5sePortalRouteCommander(%rip)\n"
        "    jmpq *coe5sePortalGameWaytogo(%rip)\n"
        ".globl coe5sePortalSpreadStub\n"
        ".def coe5sePortalSpreadStub; .scl 2; .type 32; .endef\n"
        ".p2align 4\n"
        "coe5sePortalSpreadStub:\n"
        "    movl %r12d, coe5sePortalSpreadFrom(%rip)\n"
        "    movl %r13d, coe5sePortalSpreadFrom+4(%rip)\n"
        "    jmp coe5sePortalSpreadCheck\n"
        ".globl coe5sePortalBackStub\n"
        ".def coe5sePortalBackStub; .scl 2; .type 32; .endef\n"
        ".p2align 4\n"
        "coe5sePortalBackStub:\n"
        ".seh_proc coe5sePortalBackStub\n"
        "    movq %rcx, 8(%rsp)\n"
        "    movq %rdx, 16(%rsp)\n"
        "    movq %r8, 24(%rsp)\n"
        "    movq %r9, 32(%rsp)\n"
        "    subq $40, %rsp\n"
        "    .seh_stackalloc 40\n"
        "    .seh_endprologue\n"
        "    movl %edx, %ecx\n"
        "    movl %r8d, %edx\n"
        "    movl 0x150(%rsp), %r8d\n"
        "    movl %r13d, %r9d\n"
        "    callq coe5sePortalBackCheck\n"
        "    testl %eax, %eax\n"
        "    jnz 1f\n"
        "    movl $-1, %ebx\n"
        "1:  movq 48(%rsp), %rcx\n"
        "    movq 56(%rsp), %rdx\n"
        "    movq 64(%rsp), %r8\n"
        "    movq 72(%rsp), %r9\n"
        "    addq $40, %rsp\n"
        "    jmpq *coe5sePortalTraceB(%rip)\n"
        ".seh_endproc\n");
#endif

// ---- Installing --------------------------------------------------------------------------------
typedef struct {
    unsigned int callRva, targetRva;
    void *hook;
    const char *name;
} PortalCall;

#ifndef __APPLE__
#define PORTAL_STUB_SIZE 0x20
static int pointPortalCall(const PortalCall *c, unsigned char *stubs, int index) {
    return redirectCall(c->callRva, c->targetRva, stubs + PORTAL_STUB_SIZE * index, c->hook, c->name, NULL);
}
#else
static int pointPortalCall(const PortalCall *c, unsigned char *stubs, int index) {
    (void)stubs, (void)index;
    if (blTarget(gameBase + c->callRva) != gameBase + c->targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", c->name, c->callRva + 0x100000000ULL);
        return 0;
    }
    unsigned char *trampoline = newTrampoline(c->hook);
    uint32_t word = trampoline ? blTo(gameBase + c->callRva, trampoline) : 0;
    if (trampoline && writeGameCode(gameBase + c->callRva, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=%s", c->name, trampoline ? "code_unwritable" : "no_padding");
    return 0;
}
#endif

// Points every portal path at the hooks above, once. Returns 1 when all are.
static int installPortalHooks(void) {
    if (portalHooksState) return portalHooksState > 0;
    portalHooksState = -1;
    static const PortalCall CALLS[] = {
        {RVA_GROUP_MOVE_STEP_CALL, RVA_MOVE_COMMANDER_STEP, (void *)portalMoveStepHook, "portal_group_move_step"},
        {RVA_AI_MOVE_STEP_CALL, RVA_MOVE_COMMANDER_STEP, (void *)portalMoveStepHook, "portal_ai_move_step"},
        {RVA_WANDER_MOVE_STEP_CALL, RVA_MOVE_COMMANDER_STEP, (void *)portalMoveStepHook, "portal_wander_move_step"},
        {RVA_WANDER_MOVE_STEP_CALL_2, RVA_MOVE_COMMANDER_STEP, (void *)portalMoveStepHook, "portal_wander_move_step_2"},
        {RVA_GATEWAY_EXIT_CALL, RVA_RANDOM_PORTAL_EXIT, (void *)portalGatewayExitHook, "portal_gateway_exit"},
        {RVA_INDEP_EXIT_CALL, RVA_RANDOM_PORTAL_EXIT, (void *)portalIndepExitHook, "portal_indep_exit"},
        {RVA_WAYTOGO_CALL, RVA_WAYTOGO, (void *)coe5sePortalRouteStub, "portal_waytogo"},
        {RVA_WAYTOGO_CALL_2, RVA_WAYTOGO, (void *)coe5sePortalRouteStub, "portal_waytogo_2"},
        {RVA_SPREAD_EXIT_CHECK_CALL, RVA_WAYTOGO_FAR_SQUARE_CHECK, (void *)coe5sePortalSpreadStub, "portal_spread_exit"},
        {RVA_WAYTOGO_BACK_TRACE_CALL, RVA_TRACE_B, (void *)coe5sePortalBackStub, "portal_walk_back"},
#ifdef __APPLE__
        {RVA_SPREAD_EXIT_CHECK_CALL_2, RVA_WAYTOGO_FAR_SQUARE_CHECK, (void *)coe5sePortalSpreadStub, "portal_spread_exit_2"},
        {RVA_WAYTOGO_BACK_TRACE_CALL_2, RVA_TRACE_B, (void *)coe5sePortalBackStub, "portal_walk_back_2"},
#endif
    };
    enum { COUNT = sizeof CALLS / sizeof CALLS[0] };
    if (!gameBuild) {
        writeLog("portal_hooks_installed", "ok=0 of %d reason=unknown_game_build", COUNT);
        return 0;
    }
    if (!checkGameFunction("portal_crossing_check", "")) {
        writeLog("portal_hooks_installed", "ok=0 of %d reason=game_code_changed", COUNT);
        return 0;
    }
    unsigned char *stubs = NULL;
#ifndef __APPLE__
    if (!(stubs = allocNear(gameBase + RVA_TEXT_SECTION))) {
        writeLog("portal_hooks_installed", "ok=0 of %d reason=no_near_page", COUNT);
        return 0;
    }
    makePagesReadExecute(stubs, 0x1000);
#endif
    portalGameMoveStep = GAME_FN(PortalMoveStepFn, RVA_MOVE_COMMANDER_STEP);
    portalGameExit = GAME_FN(PortalExitFn, RVA_RANDOM_PORTAL_EXIT);
    portalGameFarSquare = GAME_FN(PortalFarSquareFn, RVA_WAYTOGO_FAR_SQUARE_CHECK);
    coe5sePortalGameWaytogo = GAME_PTR(RVA_WAYTOGO);
    coe5sePortalTraceB = GAME_PTR(RVA_TRACE_B);
    int ok = 0;
    for (int i = 0; i < COUNT; i++) ok += pointPortalCall(&CALLS[i], stubs, i);
    portalHooksState = ok == COUNT ? 1 : -1;
    writeLog("portal_hooks_installed", "ok=%d of %d", ok, COUNT);
    return ok == COUNT;
}

// ---- coe5.onPortalCrossing ---------------------------------------------------------------------
static int lOnPortalCrossing(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    if (refuseFromDeadLuaScript(L, "coe5.onPortalCrossing")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (!installPortalHooks()) {
        refuseLuaCall(mod, "coe5.onPortalCrossing", "site_unavailable", "handlers=%d", portalHandlerCount);
        lua_pushboolean(L, 0);
        return 1;
    }
    int callback = newLuaCallback(L, 1, 0);
    if (callback < 0 || !growList((void **)&portalHandlers, &portalHandlerCapacity, portalHandlerCount + 1, sizeof portalHandlers[0])) {
        refuseLuaCall(mod, "coe5.onPortalCrossing", "out_of_memory", "handlers=%d", portalHandlerCount);
        lua_pushboolean(L, 0);
        return 1;
    }
    portalHandlers[portalHandlerCount++] = callback;
    writeLog("portal_crossing_handler_added", "mod=%s handlers=%d", mod, portalHandlerCount);
    lua_pushboolean(L, 1);
    return 1;
}
