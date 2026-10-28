// Play as AI: coe5.runAiTurn, the game's AI moves for a player a script plays, and that player's Recruit screen buys.
#pragma once

// coe5.runAiTurn(player): the game's AI moves for the player, at its human turn's end.
static int lRunAiTurn(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1);
    const CallInfo *turn = luaObserving[SITE_HUMAN_TURN];
    const char *reason = isNetworkGame()                                     ? "network_game"
                         : !turn || !luaObservingAfter[SITE_HUMAN_TURN]      ? "outside_human_turn_end"
                         : player != (lua_Integer)(int)turn->args[0]         ? "not_this_turn"
#ifdef __APPLE__
                         : G8(RVA_RESIGN)                                    ? "resigning"
                                                                             : NULL;
#else
                         : G32(RVA_RESIGN)                                   ? "resigning"
                         : player != G32(RVA_ACTIVE_PLAYER)                  ? "not_this_turn"
                                                                             : NULL;
#endif
    LuaScript *s = luaScriptOf(L);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.runAiTurn", reason, "player=%lld", (long long)player);
        lua_pushboolean(L, 0);
        return 1;
    }
    short controller = PG16(RVA_SLOT_CONTROLLER + 2 * (size_t)player);
    PG16(RVA_SLOT_CONTROLLER + 2 * (size_t)player) = CONTROLLER_AI;
#ifdef __APPLE__
    G8(RVA_TEST_GAME_FLAG) = 0;
#else
    G32(RVA_TEST_GAME_FLAG) = 0;
    G32(RVA_AI_PROGRESS_MODE) = 1;   // @platform-split ai_turn_flags: Mac's RVA_AI_PROGRESS_MODE wasn't looked up
#endif
    writeLog("ai_turn_run", "mod=%s player=%lld turn=%d", s ? s->mod : "?", (long long)player, G32(RVA_TURN));
    void (*move)(int) = (void (*)(int))siteEntry(SITE_AI_TURN, GAME_PTR(RVA_AI_MOVE));
    move((int)player);
    if (PG16(RVA_SLOT_CONTROLLER + 2 * (size_t)player) == CONTROLLER_AI) PG16(RVA_SLOT_CONTROLLER + 2 * (size_t)player) = controller;
    lua_pushboolean(L, 1);
    return 1;
}

// ---- Play as AI's buys: coe5.recruitScreen, and coe5.aiBuyOffer on a human's Recruit screen -------
// A played AI's buys on its Recruit screen, through the game's AI buy.

// The Recruit screen while a wrap of its wait runs.
typedef struct {
    int player, x, y, ctrl, latch;
} RecruitScreen;

// Reads the Recruit screen; NULL on Mac.
static int (*recruitScreenRead)(RecruitScreen *out);
#ifndef __APPLE__
static int keyDown(int code) { return code >= 0 && code < KEY_STATE_SIZE && G8(RVA_KEY_STATE + (size_t)code) != 0; }
static int recruitScreenFromFrame(RecruitScreen *out) {
    uintptr_t sp = luaApi->callerRsp(SITE_RECRUIT_WAIT);
    if (!sp) return 0;   // Logged by callerRsp.
    out->player = *(const int *)(sp + FRAME_RECRUIT_SCREEN_PLAYER);
    out->x = *(const int *)(sp + FRAME_RECRUIT_SCREEN_X);
    out->y = *(const int *)(sp + FRAME_RECRUIT_SCREEN_Y);
    out->ctrl = keyDown(G16(RVA_KEY_CODE_LEFT_CTRL)) || keyDown(G16(RVA_KEY_CODE_RIGHT_CTRL));
    out->latch = *(const int *)(sp + FRAME_RECRUIT_SCREEN_CLICK_LATCH);
    return 1;
}
#endif

// The Recruit screen now: 1 with *out filled, else 0.
static int recruitScreenNow(RecruitScreen *out) {
#ifndef __APPLE__
    if (!recruitScreenRead) recruitScreenRead = recruitScreenFromFrame;
#endif
    if (!luaSiteDepth[SITE_RECRUIT_WAIT] || !recruitScreenRead) return 0;   // No log: the callers log why.
    return recruitScreenRead(out);
}

// coe5.recruitScreen(): inside a wrap of RECRUIT_WAIT, the Recruit screen as {player, x, y, ctrl, latch}.
static int lRecruitScreen(lua_State *L) {
    RecruitScreen r = {-1, -1, -1, 0, 0};
#ifndef __APPLE__
    if (!recruitScreenRead) recruitScreenRead = recruitScreenFromFrame;
#endif
    const char *reason = !luaSiteDepth[SITE_RECRUIT_WAIT] ? "outside_recruit_wait"
                         : !recruitScreenRead             ? "windows_only"
                         : !recruitScreenNow(&r)          ? "no_frame"
                                                          : NULL;
    if (reason) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.recruitScreen", reason, "depth=%d", luaSiteDepth[SITE_RECRUIT_WAIT]);
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 5);
    setIntField(L, "player", r.player);
    setIntField(L, "x", r.x);
    setIntField(L, "y", r.y);
    setBoolField(L, "ctrl", r.ctrl);
    setIntField(L, "latch", r.latch);
    return 1;
}

// The game's stock reads the Recruit screen's buy checks a cost against.
typedef struct {
    int (*resourceStock)(int player, int type);
    int (*resourceAvailable)(int player, int type, int x, int y);
    void (*updateSightAround)(int player, int x, int y);
    void (*resetSquareCache)(void);
} HumanBuyCalls;
static HumanBuyCalls humanBuyCalls;
#ifndef __APPLE__
// A player's stock of a resource, as the Recruit screen reads it.
static int resourceStockOf(int player, int type) {
    return GAME_FN(int (*)(int, int, int, int), RVA_RESOURCE_AVAILABLE)(player, type, -1, -1);
}
#endif
static const HumanBuyCalls *humanBuyGame(void) {
#ifndef __APPLE__
    if (!humanBuyCalls.resourceStock) {
        humanBuyCalls.resourceStock = resourceStockOf;
        humanBuyCalls.resourceAvailable = GAME_FN(int (*)(int, int, int, int), RVA_RESOURCE_AVAILABLE);
        humanBuyCalls.updateSightAround = GAME_FN(void (*)(int, int, int), RVA_UPDATE_SIGHT_AROUND);
        humanBuyCalls.resetSquareCache = GAME_FN(void (*)(void), RVA_RESET_SQUARE_CACHE);
    }
#endif
    return humanBuyCalls.resourceStock ? &humanBuyCalls : NULL;
}

// After a buy through the AI's functions on this screen, what the human buy does after its own.
static void humanBuyAfter(int player, int x, int y) {
    const HumanBuyCalls *h = humanBuyGame();
    if (!h || !h->updateSightAround || !h->resetSquareCache) return;   // No log: Mac never buys here (windows_only).
    h->updateSightAround(player, x, y);
    h->resetSquareCache();
}

#define SHOP_COST_EXPENDABLE 16

// Why the Recruit screen's own buy would refuse the player's offer at (x, y), or NULL when it would buy it.
static const char *humanBuyRefusal(const AiRecruitCalls *g, int player, int offer, int x, int y) {
    const HumanBuyCalls *h = humanBuyGame();
    if (!h) return "windows_only";
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int count = PG32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4);
    int flags = PG32(RVA_SHOP_OFFER_FLAGS + block + (size_t)offer * 4);
    const int *costs = (const int *)(gameBase + RVA_SHOP_COSTS + block + (size_t)offer * SHOP_COST_COUNT * 4);
    int available = g->offerAvailable(player, offer, x, y);
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    if ((G32(RVA_SQUARE_FLAGS + square) & SQUARE_FLAG_RECRUITED) && !(flags & SHOP_FLAG_EXTRA)) return "square_used";
    if (costs[0] > h->resourceStock(player, 0)) return "not_enough_gold";
    if (costs[1] > h->resourceStock(player, 1)) return "not_enough_iron";
    if (costs[SHOP_COST_EXPENDABLE] > h->resourceAvailable(player, SHOP_COST_EXPENDABLE, x, y)) return "not_enough_units";
    if (available < 0)
        return available == -1   ? "cannot_pay"
               : available == -6 ? "wrong_plane"
               : available == -7 ? "no_upgrade_units"
               : available == -3 ? "sold_out"
               : available == -5 ? "not_here"
                                 : "not_possible";
    if (count == -1) return "item";
    if (available == 1) return "too_few_to_upgrade";
    return NULL;
}

// Who coe5.aiBuyOffer may buy for outside AI_RECRUIT: the active human on its own Recruit screen, or -1.
static int aiBuyHumanTurn(int *x, int *y, AiBuySquareRule *rule, AiBuyAfter *after) {
    RecruitScreen r;
    if (isNetworkGame() || !recruitScreenNow(&r)) return -1;   // Logged by coe5.aiBuyOffer (outside_ai_recruit).
    if (r.player < 0 || r.player >= PLAYER_SLOTS || r.player != G32(RVA_ACTIVE_PLAYER)) return -1;   // Logged by coe5.aiBuyOffer.
    if (PG16(RVA_SLOT_CONTROLLER + 2 * (size_t)r.player) != CONTROLLER_HUMAN) return -1;   // Logged by coe5.aiBuyOffer.
    *x = r.x, *y = r.y;
    *rule = humanBuyRefusal;
    *after = humanBuyAfter;
    return r.player;
}
