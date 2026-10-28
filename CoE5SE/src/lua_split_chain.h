// Lua wraps of a step the Mac build copied into its caller: the chain split at next().
#ifdef __APPLE__

#define SPLIT_LEVELS_MAX 64
enum { SPLIT_PASS, SPLIT_WAITING, SPLIT_DONE };

// One wrap of a split call.
typedef struct {
    LuaWrap *w;
    lua_State *co;
    int coRef;
    int state;
    LuaFrame f;
    LuaArgs in;
    LuaArgs out;
    int refusedTwice;
} SplitLevel;

// One running call of a split site.
typedef struct SplitCall {
    struct SplitCall *up;
    CallSite site;
    int luaSite;
    int count;
    int skipped;
    u64 result;
    int squareX, squareY;
    uint64_t frameSp;
    u64 kept[2];
    u64 passed[2];
    LuaFrame *framesBelow;
    SplitLevel levels[SPLIT_LEVELS_MAX];
} SplitCall;
static SplitCall *splitChainCalls;

// The innermost function of a split site's chain: the game's step, copied into its caller.
static u64 splitStepGame(void) {
    writeLog("split_step_called", "reason=unexpected");
    return 0;
}

// next() inside a split wrap: yields the arguments to the driver, which runs the rest.
static int lNextSplit(lua_State *L) {
    SplitLevel *level = lua_touserdata(L, lua_upvalueindex(1));
    lua_Integer serial = lua_tointeger(L, lua_upvalueindex(2));
    int found = 0;
    for (SplitCall *c = splitChainCalls; c && !found; c = c->up)
        found = level >= c->levels && level < c->levels + c->count;
    if (!found || level->f.serial != serial || level->co != L) return luaL_error(L, "next() called after its wrap returned");
    const LuaSite *ls = &luaSites[level->w->site];
    if (level->f.nextCalled) {
        if (!level->refusedTwice) refuseLuaCall(level->w->s->mod, ls->name, "next_once", "site=%s", ls->name);
        level->refusedTwice = 1;
        return pushLuaResult(L, ls, &level->out, level->f.result);
    }
    LuaArgs a = level->in;
    a.changed = NULL;
    readNextArgs(L, ls, &a);
    checkNextArgs(ls, &a, &level->in, level->w->s->mod);
    level->out = a;
    level->f.nextCalled = 1;
    return lua_yield(L, 0);
}

// callLuaProtected's push for a split wrap's coroutine, left on the script's stack.
static int makeSplitThread(lua_State *L) {
    SplitLevel *level = lua_touserdata(L, 1);
    lua_State *co = lua_newthread(L);
    luaL_checkstack(co, 2 + LUA_CALL_ARGS, "a wrap's arguments");
    lua_rawgeti(co, LUA_REGISTRYINDEX, level->w->ref);
    lua_pushlightuserdata(co, level);
    lua_pushinteger(co, level->f.serial);
    lua_pushcclosure(co, lNextSplit, 2);
    pushLuaArgs(co, &luaSites[level->w->site], &level->in);
    return 1;
}
static int pushMakeSplitThread(lua_State *L, void *level) {
    lua_pushcfunction(L, makeSplitThread);
    lua_pushlightuserdata(L, level);
    return 1;
}

// Logs a split wrap's error and turns its script off, as callLuaProtected does.
static void stopSplitScript(LuaScript *s, lua_State *co) {
    char line[600];
    int type = lua_type(co, -1);
    if (type == LUA_TSTRING) snprintf(line, sizeof line, "%s", lua_tostring(co, -1));
    else snprintf(line, sizeof line, "(error object is a %s value)", lua_typename(co, type));
    writeLog("lua_error", "mod=%s error=%s", s->mod, line);
    lua_settop(co, 0);
    s->dead = 1;
    if (!s->stopLogged) writeLog("lua_script_stopped", "mod=%s", s->mod);
    s->stopLogged = 1;
}

// Resumes a split wrap's coroutine with `nargs` values on it, as callLuaProtected runs a call.
static int resumeSplitLevel(SplitLevel *level, int nargs, int *nresults) {
    LuaScript *s = level->w->s;
    if (s->stepsFrame != gameFrames) startLuaSteps(s);
    LuaScript *outer = luaRunning;
    luaRunning = s;
    luaCodeRunning++;
    int rc = lua_resume(level->co, NULL, nargs, nresults);
    luaCodeRunning--;
    luaRunning = outer;
    return rc;
}

// What a split wrap returned, with its results on its coroutine: the call's result, as runLuaWrap takes it.
static u64 takeSplitResult(SplitCall *c, SplitLevel *level, int nresults) {
    const LuaSite *ls = &luaSites[level->w->site];
    LuaScript *s = level->w->s;
    lua_State *co = level->co;
    u64 result;
    int base = lua_gettop(co) - nresults;
    if (ls->kind == K_TARGET_SQUARE) {
        lua_settop(co, base + 3);
        result = takeTargetSquare(co, ls, &level->in, &level->f, s->mod);
    } else {
        result = nresults && lua_isinteger(co, base + 1) ? (u64)lua_tointeger(co, base + 1) : level->f.result;
    }
    lua_settop(co, 0);
    if (!level->f.nextCalled || (siteResult(ls) == 'i' ? (int)result != (int)level->f.result : result != level->f.result))
        siteResultMod[c->site] = s->mod;
    if (!level->f.nextCalled && !level->w->noNextLogged) {
        level->w->noNextLogged = 1;
        writeLog("lua_wrap_no_next", "mod=%s site=%s priority=%d result=%lld", s->mod, ls->name, level->w->priority, (long long)result);
    }
    return result;
}

// Ends level `i`, which waits at its next(), with the rest's `result`: the wrap's own result.
static u64 finishSplitLevel(SplitCall *c, int i, u64 result) {
    SplitLevel *level = &c->levels[i];
    if (level->state != SPLIT_WAITING) return result;
    LuaScript *s = level->w->s;
    level->state = SPLIT_DONE;
    level->f.result = result;
    if (s->dead) return result;
    const LuaSite *ls = &luaSites[level->w->site];
    int pushed = pushLuaResult(level->co, ls, &level->out, result), nresults = 0;
    luaFrames = &level->f;
    int rc = resumeSplitLevel(level, pushed, &nresults);
    luaFrames = level->f.up;
    if (rc == LUA_OK) return takeSplitResult(c, level, nresults);
    if (rc == LUA_YIELD) lua_pushstring(level->co, "a wrap can't yield");
    stopSplitScript(s, level->co);
    return result;
}

// Drops the call's coroutines and lets their scripts close if they were turned off.
static void releaseSplitCall(SplitCall *c) {
    for (int i = 0; i < c->count; i++) {
        SplitLevel *level = &c->levels[i];
        if (level->co) {
            LuaScript *s = level->w->s;
            if (s->L) luaL_unref(s->L, LUA_REGISTRYINDEX, level->coRef);
            s->depth--;
            closeDeadLuaScript(s);
        }
    }
    luaSiteDepth[c->site]--;
    luaFrames = c->framesBelow;
    splitChainCalls = c->up;
    free(c);
}

// Starts a split site's call: runs its wraps up to their next(), innermost last.
static SplitCall *startSplitChain(CallSite site, const LuaArgs *in, LuaArgs *out) {
    if (isNetworkGame()) return NULL;
    int luaSite = -1;
    for (int i = 0; i < LUA_SITE_COUNT && luaSite < 0; i++)
        if (luaSites[i].site == site) luaSite = i;
    if (luaSite < 0) return NULL;
    void *g = *siteHead(site);
    if (g == (void *)splitStepGame) return NULL;
    SplitCall *c = calloc(1, sizeof *c);
    if (!c) {
        writeLog("split_chain_skipped", "site=%s reason=out_of_memory", SITES[site].name);
        return NULL;
    }
    c->site = site, c->luaSite = luaSite, c->up = splitChainCalls, c->framesBelow = luaFrames;
    c->squareX = c->squareY = AI_NO_SQUARE;
    splitChainCalls = c;
    luaSiteDepth[site]++;
    LuaArgs args = *in;
    if (luaSites[luaSite].kind == K_TARGET_SQUARE) {
        args.a[2] = (u64)(uintptr_t)&c->squareX;
        args.a[3] = (u64)(uintptr_t)&c->squareY;
    }
    for (LuaWrap *w; g != (void *)splitStepGame; g = w->next) {
        w = luaWrapOfGate(g);
        if (!w) {
            writeLog("split_chain_skipped", "site=%s reason=not_a_lua_wrap", SITES[site].name);
            c->count = 0;
            releaseSplitCall(c);
            return NULL;
        }
        if (c->count == SPLIT_LEVELS_MAX) {
            refuseLuaCall(w->s ? w->s->mod : "?", luaSites[luaSite].name, "split_chain_full", "max=%d", SPLIT_LEVELS_MAX);
            break;
        }
        SplitLevel *level = &c->levels[c->count++];
        level->w = w;
        level->in = args;
        level->f = (LuaFrame){luaFrames, w, &level->in, 0, 0, ++luaFrameSerial, 0, 0, 0, luaChangeCount};
        LuaScript *s = w->s;
        if (!s || s->dead) continue;
        if (!s->depth++) startLuaSteps(s);
        if (!callLuaProtected(s, pushMakeSplitThread, level, 1)) {
            s->depth--;
            closeDeadLuaScript(s);
            continue;
        }
        level->co = lua_tothread(s->L, -1);
        level->coRef = luaL_ref(s->L, LUA_REGISTRYINDEX);
        luaFrames = &level->f;
        int nresults = 0, rc = resumeSplitLevel(level, lua_gettop(level->co) - 1, &nresults);
        if (rc != LUA_YIELD) luaFrames = level->f.up;
        if (rc == LUA_YIELD) {
            level->state = SPLIT_WAITING;
            args = level->out;
            continue;
        }
        if (rc == LUA_OK) {
            level->state = SPLIT_DONE;
            c->skipped = 1;
            c->result = takeSplitResult(c, level, nresults);
            for (int i = c->count - 2; i >= 0; i--) c->result = finishSplitLevel(c, i, c->result);
            break;
        }
        stopSplitScript(s, level->co);
    }
    *out = args;
    return c;
}

// Ends a split call that wasn't skipped: resumes the wraps, innermost first, with the step's `result`.
static u64 endSplitChain(SplitCall *c, u64 result) {
    for (int i = c->count - 1; i >= 0; i--) result = finishSplitLevel(c, i, result);
    return result;
}

// ---- The hooks at each copied step's ends ---------------------------------------------------------
enum { PLACE_NONE, PLACE_REG, PLACE_SP, PLACE_X29, PLACE_CONST };
typedef struct { unsigned char in; int at; } SplitPlace;
enum {
    SKIP_NONE,
    SKIP_REG,
    SKIP_REG_GAME,
    SKIP_REG_SP64,
    SKIP_SP32_REG,
    SKIP_SP32_COMSTR,
    SKIP_SP64_RECORD,
    SKIP_X29_PAIR,
};
typedef struct { unsigned char op, reg; short at; int64_t value; int stride; } SplitSkip;
enum { VALUE_MOV, VALUE_CMP };
typedef struct { unsigned int rva; unsigned char op, reg, arg; } SplitValueHook;
#define SPLIT_SKIPS_MAX 6
#define SPLIT_VALUES_MAX 2
typedef struct {
    CallSite site;
    unsigned int startRva, endRva, noneRva;
    SplitPlace args[2];
    SplitPlace score, x, y;
    SplitSkip skip[SPLIT_SKIPS_MAX];
    SplitValueHook values[SPLIT_VALUES_MAX];
} SplitStepMac;
#define MOVE_LOOP_ARGS {{PLACE_SP, AI_MOVE_MAC_COMMANDER_AT}, {PLACE_SP, AI_MOVE_MAC_COMSTR_AT}}
static const SplitStepMac SPLIT_STEPS_MAC[] = {
    {SITE_AI_RALLY, RVA_AI_RALLY_COPY_START, RVA_AI_RALLY_COPY_END, RVA_AI_RALLY_COPY_NONE, MOVE_LOOP_ARGS, {PLACE_SP, 0x44},
     {PLACE_X29, -0xe0}, {PLACE_X29, -0xd8}, {{SKIP_REG_GAME, 13, 0, RVA_UNITS & ~0xfffu, 0}}, {{0}}},
    {SITE_AI_FRIENDLY_MOVE, RVA_AI_FRIENDLY_COPY_START, RVA_AI_FRIENDLY_COPY_END, 0, MOVE_LOOP_ARGS, {PLACE_SP, 0xdc}, {PLACE_SP, 0x68},
     {PLACE_SP, 0x6c},
     {{SKIP_REG, 21, 0, 0x158, 0}, {SKIP_REG, 24, 0, 0x13c, 0}, {SKIP_REG_SP64, 12, 0xc8, 0, 0}, {SKIP_SP32_COMSTR, 0, 0x34, 4, 0},
      {SKIP_X29_PAIR, 0, -0x78, -1, 0}},
     {{0}}},
    {SITE_AI_CONQUER, RVA_AI_CONQUER_COPY_START, RVA_AI_CONQUER_COPY_END, 0, MOVE_LOOP_ARGS, {PLACE_REG, 20}, {PLACE_REG, 23},
     {PLACE_REG, 19}, {{SKIP_REG, 26, 0, 0xc580, 0}, {SKIP_X29_PAIR, 0, -0x78, -1, 0}, {SKIP_SP32_REG, 8, 0x44, 0, 0}}, {{0}}},
    {SITE_AI_EXPLORE, RVA_AI_EXPLORE_COPY_START, RVA_AI_EXPLORE_COPY_END, 0, MOVE_LOOP_ARGS, {PLACE_REG, 25}, {PLACE_REG, 24},
     {PLACE_REG, 23}, {{SKIP_REG, 21, 0, 0x158, 0}, {SKIP_REG, 26, 0, 0xc580, 0}, {SKIP_X29_PAIR, 0, -0x78, -1, 0}, {SKIP_SP32_REG, 8, 0x44, 0, 0}},
     {{0}}},
    {SITE_AI_GATHER_TROOPS, RVA_AI_GATHER_COPY_START, RVA_AI_GATHER_COPY_END, 0, {{PLACE_SP, AI_MOVE_MAC_COMMANDER_AT}, {PLACE_NONE, 0}},
     {0}, {0}, {0}, {{0}}, {{0}}},
    {SITE_AI_SPLIT_ARMY, RVA_AI_SPLIT_COPY_START, RVA_AI_SPLIT_COPY_END, 0, {{PLACE_REG, 19}, {PLACE_CONST, AI_SPLIT_MIN_UNITS_MAC}},
     {0}, {0}, {0}, {{0}}, {{RVA_AI_SPLIT_MIN_UNITS_TRACE, VALUE_MOV, 9, 1}, {RVA_AI_SPLIT_MIN_UNITS_TEST, VALUE_CMP, 21, 1}}},
    {SITE_AI_RECRUIT, RVA_AI_RECRUIT_COPY_START, RVA_AI_RECRUIT_COPY_END, 0, {{PLACE_REG, 19}, {PLACE_NONE, 0}}, {0}, {0}, {0},
     {{SKIP_SP64_RECORD, 22, 0x100, RVA_SLOT_NAME + 0x1930, PLAYER_BLOCK_SIZE}, {SKIP_REG_GAME, 20, 0, RVA_SLOT_NAME, 0}}, {{0}}},
};
#define SPLIT_STEP_COUNT ((int)(sizeof SPLIT_STEPS_MAC / sizeof SPLIT_STEPS_MAC[0]))
static StepHook splitStartHooksMac[SPLIT_STEP_COUNT], splitEndHooksMac[SPLIT_STEP_COUNT], splitNoneHooksMac[SPLIT_STEP_COUNT],
    splitValueHooksMac[SPLIT_STEP_COUNT][SPLIT_VALUES_MAX];

static int splitStepIndex(CallSite site) {
    for (int i = 0; i < SPLIT_STEP_COUNT; i++)
        if (SPLIT_STEPS_MAC[i].site == site) return i;
    return -1;
}

// Where a place's 32-bit value is (PLACE_REG: the register's low half; arm64 is little-endian).
static uint32_t *splitPlace(StepContext *ctx, SplitPlace p) {
    if (p.in == PLACE_REG) return (uint32_t *)&ctx->x[p.at];
    return (uint32_t *)(uintptr_t)((p.in == PLACE_SP ? ctx->sp : ctx->x[29]) + (int64_t)p.at);
}
// An argument as the game has it at the start.
static u64 splitArg(StepContext *ctx, SplitPlace p) {
    if (p.in == PLACE_REG) return ctx->x[p.at];
    if (p.in == PLACE_SP) return *(u64 *)(uintptr_t)(ctx->sp + (int64_t)p.at);
    if (p.in == PLACE_CONST) return (u64)(int64_t)p.at;
    return 0;
}
static void putSplitArg(StepContext *ctx, SplitPlace p, u64 value) {
    if (p.in == PLACE_REG) ctx->x[p.at] = value;
    else if (p.in == PLACE_SP) *(u64 *)(uintptr_t)(ctx->sp + (int64_t)p.at) = value;
}
// Puts a step's result where the step leaves its own (a register's high half cleared, as a w write does).
static void putSplitResult(StepContext *ctx, const SplitStepMac *m, u64 score, int x, int y) {
    if (m->score.in == PLACE_NONE) return;
    const SplitPlace places[3] = {m->score, m->x, m->y};
    const uint32_t values[3] = {(uint32_t)score, (uint32_t)x, (uint32_t)y};
    for (int k = 0; k < 3; k++) {
        if (places[k].in == PLACE_REG) ctx->x[places[k].at] = values[k];
        else *splitPlace(ctx, places[k]) = values[k];
    }
}
// The running call of `site` started at this frame, or NULL.
static SplitCall *splitCallHere(StepContext *ctx, CallSite site) {
    SplitCall *c = splitChainCalls;
    return c && c->site == site && c->frameSp == ctx->sp ? c : NULL;
}

// The start hook: runs the site's wraps up to their next(), then the game's step, or skips it.
static void splitStepStart(StepContext *ctx, const StepHook *hook) {
    int i = splitStepIndex(hook->site);
    if (i < 0) return;
    const SplitStepMac *m = &SPLIT_STEPS_MAC[i];
    for (SplitCall *stale; (stale = splitCallHere(ctx, m->site));) {
        writeLog("split_call_dropped", "site=%s reason=no_end", SITES[m->site].name);
        releaseSplitCall(stale);
    }
    LuaArgs in = {{0}, {0}, NULL, NULL}, out;
    for (int k = 0; k < 2; k++) in.a[k] = splitArg(ctx, m->args[k]);
    SplitCall *c = startSplitChain(m->site, &in, &out);
    if (!c) return;
    c->frameSp = ctx->sp;
    for (int k = 0; k < 2; k++) c->kept[k] = in.a[k], c->passed[k] = out.a[k];
    if (!c->skipped) {
        for (int k = 0; k < 2; k++)
            if (out.a[k] != in.a[k]) putSplitArg(ctx, m->args[k], out.a[k]);
        return;
    }
    unsigned char *sp = (unsigned char *)(uintptr_t)ctx->sp;
    for (int k = 0; k < SPLIT_SKIPS_MAX && m->skip[k].op != SKIP_NONE; k++) {
        const SplitSkip *s = &m->skip[k];
        switch (s->op) {
        case SKIP_REG: ctx->x[s->reg] = (uint64_t)s->value; break;
        case SKIP_REG_GAME: ctx->x[s->reg] = (uint64_t)(uintptr_t)(gameBase + s->value); break;
        case SKIP_REG_SP64: ctx->x[s->reg] = *(uint64_t *)(sp + s->at); break;
        case SKIP_SP32_REG: *(uint32_t *)(sp + s->at) = (uint32_t)ctx->x[s->reg]; break;
        case SKIP_SP32_COMSTR: *(uint32_t *)(sp + s->at) = (uint32_t)in.a[1] << s->value; break;
        case SKIP_SP64_RECORD: *(uint64_t *)(sp + s->at) = (uint64_t)(uintptr_t)(gameBase + s->value) + ctx->x[s->reg] * (uint64_t)s->stride; break;
        case SKIP_X29_PAIR: {
            int *pair = (int *)(uintptr_t)(ctx->x[29] + (int64_t)s->at);
            pair[0] = pair[1] = (int)s->value;
            break;
        }
        default: break;
        }
    }
    putSplitResult(ctx, m, c->result, c->squareX, c->squareY);
    ctx->pc = splitEndHooksMac[i].resume;
    releaseSplitCall(c);
}

// Ends this hook's call with the step's score and square; returns 1 if a call ran.
static int finishSplitStep(StepContext *ctx, const StepHook *hook, u64 score, int x, int y) {
    int i = splitStepIndex(hook->site);
    SplitCall *c = splitCallHere(ctx, hook->site);
    if (i < 0 || !c) return 0;
    const SplitStepMac *m = &SPLIT_STEPS_MAC[i];
    for (int k = 0; k < 2; k++)
        if (c->passed[k] != c->kept[k]) putSplitArg(ctx, m->args[k], c->kept[k]);
    c->squareX = x;
    c->squareY = y;
    u64 result = endSplitChain(c, score);
    putSplitResult(ctx, m, result, c->squareX, c->squareY);
    releaseSplitCall(c);
    return 1;
}

// The end hook: resumes the wraps with the step's result and puts the outermost's back.
static void splitStepEnd(StepContext *ctx, const StepHook *hook) {
    int i = splitStepIndex(hook->site);
    if (i < 0) return;
    const SplitStepMac *m = &SPLIT_STEPS_MAC[i];
    if (m->score.in == PLACE_NONE) {
        finishSplitStep(ctx, hook, 0, AI_NO_SQUARE, AI_NO_SQUARE);
        return;
    }
    finishSplitStep(ctx, hook, (u64)(int64_t)(int)*splitPlace(ctx, m->score), (int)*splitPlace(ctx, m->x), (int)*splitPlace(ctx, m->y));
}

// The second end of a step whose early way out goes past its end, where the step's score is 0.
static void splitStepNone(StepContext *ctx, const StepHook *hook) {
    int i = splitStepIndex(hook->site);
    if (i < 0) return;
    if (finishSplitStep(ctx, hook, 0, AI_NO_SQUARE, AI_NO_SQUARE)) ctx->pc = splitEndHooksMac[i].resume;
}

// A value hook: the instruction's work with the argument the innermost next() passed on.
static void splitStepValue(StepContext *ctx, const StepHook *hook) {
    int i = splitStepIndex(hook->site);
    if (i < 0) return;
    const SplitStepMac *m = &SPLIT_STEPS_MAC[i];
    const SplitValueHook *v = &m->values[hook - splitValueHooksMac[i]];
    SplitCall *c = splitCallHere(ctx, m->site);
    uint32_t value = (uint32_t)(c ? c->passed[v->arg] : splitArg(ctx, m->args[v->arg]));
    if (v->op == VALUE_MOV) {
        ctx->x[v->reg] = value;
        return;
    }
    uint32_t a = (uint32_t)ctx->x[v->reg], r = a - value;
    uint64_t n = r >> 31, z = r == 0, carry = a >= value, overflow = ((a ^ value) & (a ^ r)) >> 31;
    ctx->nzcv = (n << 31) | (z << 30) | (carry << 29) | (overflow << 28);
}

// installSite for a split site: its hooks.
static int installSplitSiteMac(CallSite site) {
    int i = splitStepIndex(site);
    if (i < 0) {
        writeLog("site_skipped", "site=%s reason=not_on_this_build", SITES[site].name);
        return 0;
    }
    const SplitStepMac *m = &SPLIT_STEPS_MAC[i];
    if (!gameCodeMatches) {
        writeLog("site_skipped", "site=%s reason=not_the_expected_code", SITES[site].name);
        return 0;
    }
    *siteHead(site) = siteGameFunction(site);
    const char *name = SITES[site].name;
    splitStartHooksMac[i] = (StepHook){0, splitStepStart, site};
    splitEndHooksMac[i] = (StepHook){0, splitStepEnd, site};
    splitNoneHooksMac[i] = (StepHook){0, splitStepNone, site};
    if (!putStepHookMac(m->endRva, &splitEndHooksMac[i], name, 0)) return 0;
    if (m->noneRva && !putStepHookMac(m->noneRva, &splitNoneHooksMac[i], name, 0)) return 0;
    for (int k = 0; k < SPLIT_VALUES_MAX && m->values[k].rva; k++) {
        splitValueHooksMac[i][k] = (StepHook){0, splitStepValue, site};
        if (!putStepHookMac(m->values[k].rva, &splitValueHooksMac[i][k], name, 1)) return 0;
    }
    if (!putStepHookMac(m->startRva, &splitStartHooksMac[i], name, 0)) return 0;
    siteInstalled[site] = 1;
    return 1;
}
#endif
