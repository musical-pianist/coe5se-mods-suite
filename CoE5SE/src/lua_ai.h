// Scripts and the AI: coe5.route, the ritual plans, the armies, recruiting, and coe5.keepTraceLines.

// ---- AI routes: coe5.route --------------------------------------------------------------------
// The sites of an AI commander's move.
static const CallSite AI_MOVE_SITES[] = {SITE_AI_RALLY, SITE_AI_FRIENDLY_MOVE, SITE_AI_CONQUER, SITE_AI_EXPLORE, SITE_AI_FAR_EXPLORE,
                                         SITE_AI_MOVE_COMSTR};
static int aiMoveSiteRunning(void) {
    for (size_t i = 0; i < sizeof AI_MOVE_SITES / sizeof AI_MOVE_SITES[0]; i++)
        if (luaSiteDepth[AI_MOVE_SITES[i]]) return 1;
    return 0;
}

// The steps of the script's budget each coe5.route call costs.
#define ROUTE_CALL_STEPS 10000
// Why coe5.route refuses its call, or NULL when it can go to the game.
static const char *routeRefusal(int inAiSite, lua_Integer commander, lua_Integer x, lua_Integer y, lua_Integer comstr, int mapDepth,
                                int mapH) {
    if (!inAiSite) return "outside_ai_site";
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (x < 0 || y < 0 || x >= mapDepth || y >= mapH || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return "off_map";
    if (comstr < -1 || comstr > ROUTE_COMSTR_MAX) return "out_of_range";
    return NULL;
}

// coe5.route(commander, x, y[, comstr]): how far the commander's army has to go to reach (x, y).
static int lRoute(lua_State *L) {
    coe5seChargeLuaSteps(L, ROUTE_CALL_STEPS);
    lua_Integer commander = luaL_checkinteger(L, 1), x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    int ownStrength = lua_isnoneornil(L, 4);
    lua_Integer comstr = ownStrength ? 0 : luaL_checkinteger(L, 4);
    const char *reason = routeRefusal(aiMoveSiteRunning(), commander, x, y, comstr, G32(RVA_MAP_DEPTH), G32(RVA_MAP_H));
    if (reason) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.route", reason, "commander=%lld x=%lld y=%lld comstr=%lld", (long long)commander,
                      (long long)x, (long long)y, (long long)comstr);
        lua_pushnil(L);
        return 1;
    }
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    int fromX = -1, fromY = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0)
        fromX = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), fromY = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
    if (fromX < 0 || fromY < 0) {
        lua_pushnil(L);
        return 1;
    }
    // @platform-split ai_route: Mac takes no strength from the game (RVA_COMMANDER_STRENGTH not looked up)
    if (ownStrength) comstr = commanderStrength((int)commander);
    int randomIndex = G32(RVA_RANDOM_INDEX), randomShift = G32(RVA_RANDOM_SHIFT), randomXor = G32(RVA_RANDOM_XOR);
    int stepX = 0, stepY = 0;
#ifdef __APPLE__
    int length = GAME_FN(int (*)(int, int, int, int, int, int, int *, int *), RVA_WAYTOGO2)((int)commander, fromX, fromY, (int)x, (int)y,
                                                                                         (int)comstr, &stepX, &stepY);
#else
    int length = GAME_FN(int (*)(int, int, int, int, int, int, int, int *, int *), RVA_WAYTOGO2)(
        (int)commander, fromX, fromY, (int)x, (int)y, (int)comstr, 0, &stepX, &stepY);
#endif
    G32(RVA_RANDOM_INDEX) = randomIndex, G32(RVA_RANDOM_SHIFT) = randomShift, G32(RVA_RANDOM_XOR) = randomXor;
    if (length < 0) lua_pushnil(L);
    else lua_pushinteger(L, length);
    return 1;
}

// ---- AI ritual plans: coe5.scrapRitualPlan --------------------------------------------------------
// The AI sites.
static int aiSiteRunning(void) {
    return aiMoveSiteRunning() || luaSiteDepth[SITE_AI_RITUAL_CANDIDATE] || luaSiteDepth[SITE_AI_GATHER_TROOPS] ||
           luaSiteDepth[SITE_AI_PICK_UP_LONER] || luaSiteDepth[SITE_AI_SPLIT_ARMY] || luaObserving[SITE_AI_TURN] != NULL;
}

// Why coe5.scrapRitualPlan refuses its call, or NULL.
static const char *ritualPlanScrapRefusal(int inAiSite, lua_Integer commander, int owner, int controller, int plan) {
    if (!inAiSite) return "outside_ai_site";
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (owner < 0) return "no_unit";
    if (controller != CONTROLLER_AI) return "not_ai";
    if (plan < 0) return "not_ritual";
    return NULL;
}

// coe5.scrapRitualPlan(commander): drops an AI commander's planned ritual.
static int lScrapRitualPlan(lua_State *L) {
    lua_Integer commander = luaL_checkinteger(L, 1);
    int owner = -1, controller = CONTROLLER_REMOVED, plan = 0;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        }
        plan = G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE);
    }
    LuaScript *s = luaScriptOf(L);
    const char *reason = ritualPlanScrapRefusal(aiSiteRunning(), commander, owner, controller, plan);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.scrapRitualPlan", reason, "commander=%lld player=%d controller=%d plan=%d",
                      (long long)commander, owner, controller, plan);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (plan > 0) {
        G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE) = 0;
        writeLog("ai_plan_scrapped", "mod=%s commander=%lld player=%d ritual=%d", s ? s->mod : "?", (long long)commander, owner, plan);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// ---- AI armies: coe5.releaseFollower -------------------------------------------------------------
// Why coe5.releaseFollower refuses its call, or NULL.
static const char *followerReleaseRefusal(int inAiSite, int network, int windowsOnly, lua_Integer unit, int owner, int controller,
                                          int activePlayer) {
    if (windowsOnly) return "windows_only";
    if (!inAiSite) return "outside_ai_site";
    if (network) return "network_game";
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return "bad_unit";
    if (owner < 0) return "no_unit";
    if (controller != CONTROLLER_AI) return "not_ai";
    if (owner != activePlayer) return "not_moving_player";
    return NULL;
}

// @platform-split release_follower: Windows only, as the AI sites (Mac's AI_TURN observers refuse it)
// coe5.releaseFollower(unit): frees one unit of an AI player from the commander it follows.
#ifdef __APPLE__
static int releaseFollowerWindowsOnly = 1;
#else
static int releaseFollowerWindowsOnly = 0;
#endif
static int lReleaseFollower(lua_State *L) {
    lua_Integer unit = luaL_checkinteger(L, 1);
    int owner = -1, controller = CONTROLLER_REMOVED, leader = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
        int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
        if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
    }
    int active = G32(RVA_ACTIVE_PLAYER);
    LuaScript *s = luaScriptOf(L);
    const char *reason = followerReleaseRefusal(aiSiteRunning(), isNetworkGame(), releaseFollowerWindowsOnly, unit, owner, controller,
                                                active);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.releaseFollower", reason, "unit=%lld player=%d controller=%d active=%d commander=%d",
                      (long long)unit, owner, controller, active, leader);
        lua_pushboolean(L, 0);
        return 1;
    }
    if (leader >= 0) {
        G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER) = -1;
        flushCommanderCache(leader);
        writeLog("ai_follower_released", "mod=%s unit=%lld commander=%d player=%d", s ? s->mod : "?", (long long)unit, leader, owner);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// ---- AI armies: coe5.transferFollower --------------------------------------------------------------
// Why coe5.transferFollower refuses its call, or NULL.
static const char *followerTransferRefusal(int inAiSite, int network, int windowsOnly, lua_Integer unit, int owner, int controller,
                                           int activePlayer, lua_Integer commander, int commanderOwner, int commanderUnit, int leader,
                                           int sameSquare) {
    const char *reason = followerReleaseRefusal(inAiSite, network, windowsOnly, unit, owner, controller, activePlayer);
    if (reason) return reason;
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return "bad_commander";
    if (commanderOwner < 0) return "no_commander";
    if (commanderOwner != owner) return "other_player";
    if (commanderUnit == unit) return "same_unit";
    if (leader < 0) return "not_following";
    if (leader == commander) return "already_following";
    if (!sameSquare) return "not_same_square";
    return NULL;
}

// @platform-split transfer_follower: Windows only; RVA_TRANSFER_UNIT_TO_COMMANDER wasn't looked up on Mac
// The game's own one-unit transfer.
static int (*transferUnitGame)(int unit, int commander);

// coe5.transferFollower(unit, commander): moves one unit to another commander on the same square.
static int lTransferFollower(lua_State *L) {
    lua_Integer unit = luaL_checkinteger(L, 1), commander = luaL_checkinteger(L, 2);
    int owner = -1, controller = CONTROLLER_REMOVED, leader = -1, ux = -1, uy = -1;
    if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
        int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
        if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
        ux = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), uy = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE + 4);
    }
    int commanderOwner = -1, commanderUnit = -1, cx = -2, cy = -2;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        commanderUnit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (commanderUnit >= 0 && commanderUnit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)commanderUnit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)commanderUnit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) commanderOwner = player;
            cx = G32(RVA_UNIT_X + (size_t)commanderUnit * UNIT_SIZE), cy = G32(RVA_UNIT_X + (size_t)commanderUnit * UNIT_SIZE + 4);
        }
    }
#ifndef __APPLE__
    if (!transferUnitGame) transferUnitGame = GAME_FN(int (*)(int, int), RVA_TRANSFER_UNIT_TO_COMMANDER);
#endif
    int active = G32(RVA_ACTIVE_PLAYER);
    LuaScript *s = luaScriptOf(L);
    const char *reason = followerTransferRefusal(aiSiteRunning(), isNetworkGame(), transferUnitGame == NULL, unit, owner, controller,
                                                 active, commander, commanderOwner, commanderUnit, leader, ux == cx && uy == cy);
    if (!reason && !transferUnitGame((int)unit, (int)commander)) reason = "game_refused";
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.transferFollower", reason,
                      "unit=%lld commander=%lld player=%d commander_player=%d active=%d follows=%d", (long long)unit,
                      (long long)commander, owner, commanderOwner, active, leader);
        lua_pushboolean(L, 0);
        return 1;
    }
    int now = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
    writeLog("ai_follower_transferred", "mod=%s unit=%lld from=%d to=%lld player=%d follows=%d", s ? s->mod : "?", (long long)unit, leader,
             (long long)commander, owner, now);
    lua_pushboolean(L, now == commander);
    return 1;
}

// ---- AI recruiting: coe5.shopOffers, classRecruits, recruitSquares, aiBuyOffer -------------------
// A player's recruitment offers, its class's list, its recruit squares, and the AI's buy.

// @platform-split ai_recruit: Windows only; the Mac step is inlined and its helpers not looked up
// The game functions the square rules and the buy call.
typedef struct AiRecruitCalls {
    int (*offerAvailable)(int player, int offer, int x, int y);
    int (*citadelFor)(int player, int x, int y);
    int (*squareAbilityTotal)(int player, int x, int y, int ability, int sum);
    int (*countMonsterAt)(int x, int y, int monster, int player);
    int (*seaAreaSize)(int x, int y, int limit);
    int (*squareHasFort)(int x, int y);
    void (*buyTroops)(int player, int offer, int x, int y);
    int (*buyCommander)(int player, int offer, int x, int y);
    void (*makeSpawnbooster)(int commander);
} AiRecruitCalls;
static AiRecruitCalls aiRecruitCalls;

static const AiRecruitCalls *aiRecruitGame(void) {
#ifndef __APPLE__
    if (!aiRecruitCalls.offerAvailable) {
        aiRecruitCalls.offerAvailable = GAME_FN(int (*)(int, int, int, int), RVA_SHOP_OFFER_AVAILABLE);
        aiRecruitCalls.citadelFor = GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR);
        aiRecruitCalls.squareAbilityTotal = GAME_FN(int (*)(int, int, int, int, int), RVA_SQUARE_ABILITY_TOTAL);
        aiRecruitCalls.countMonsterAt = GAME_FN(int (*)(int, int, int, int), RVA_COUNT_MONSTER_AT);
        aiRecruitCalls.seaAreaSize = GAME_FN(int (*)(int, int, int), RVA_SEA_AREA_SIZE);
        aiRecruitCalls.squareHasFort = GAME_FN(int (*)(int, int), RVA_SQUARE_HAS_FORT);
        aiRecruitCalls.buyTroops = GAME_FN(void (*)(int, int, int, int), RVA_AI_BUY_TROOPS);
        aiRecruitCalls.buyCommander = GAME_FN(int (*)(int, int, int, int), RVA_AI_BUY_COMMANDER);
        aiRecruitCalls.makeSpawnbooster = GAME_FN(void (*)(int), RVA_AI_MAKE_SPAWNBOOSTER);
    }
#endif
    return aiRecruitCalls.offerAvailable ? &aiRecruitCalls : NULL;
}

#define SHOP_SQUARES_MAX 100
#define SHIP_SEA_SQUARES 50

// A monster's action points a turn.
static int monsterMoves(int monster) {
    const unsigned char *record = monsterRecord(monster);
    if (!record) return 0;
    unsigned long long flags = *(const unsigned long long *)(record + (RVA_MONSTER_FLAGS - RVA_MONSTER_TABLE));
    return flags & UNIT_FLAG_FAST ? 4 : flags & UNIT_FLAG_SLOW ? 2 : 3;
}

// The highest level of a monster's spell attacks, 0 for none.
static int monsterSpellLevel(int monster) {
    if (!monsterRecord(monster)) return 0;
    const unsigned char *rows = monsterRecord(monster) + MONSTER_ATTACKS;
    int best = 0;
    for (int i = 0; i < MONSTER_ATTACK_MAX; i++) {
        const int *row = (const int *)(rows + (size_t)i * MONSTER_ATTACK_SIZE);
        if (row[0] < 1 || row[2] < 0) break;
        if ((row[0] == 3 || row[0] == 6 || row[0] == 14 || row[0] == 15 || row[0] == 19) && row[4] > best) best = row[4];
    }
    return best;
}

static void setIntField(lua_State *L, const char *key, lua_Integer value) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}
static void setBoolField(lua_State *L, const char *key, int value) {
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

// A string cut to `size` bytes, its 0 or the end.
static void pushRecordText(lua_State *L, const char *text, size_t size) {
    size_t len = 0;
    while (len < size && text[len]) len++;
    lua_pushlstring(L, text, len);
}

// The monster fields every offer has.
static void setOfferMonster(lua_State *L, int monster, const char *name) {
    if (monster >= 0) setIntField(L, "monster", monster);
    pushRecordText(L, name, MONSTER_NAME_SIZE);
    lua_setfield(L, -2, "name");
    setIntField(L, "moves", monster >= 0 ? monsterMoves(monster) : 0);
    setIntField(L, "spellLevel", monster >= 0 ? monsterSpellLevel(monster) : 0);
}

// An offer's costs, by cost type.
static void setOfferCosts(lua_State *L, const int *costs) {
    lua_createtable(L, 0, SHOP_COST_COUNT);
    for (int t = 0; t < SHOP_COST_COUNT; t++) {
        lua_pushinteger(L, costs[t]);
        lua_rawseti(L, -2, t);
    }
    lua_setfield(L, -2, "costs");
}

// coe5.shopOffers(player): the player's recruitment offers this turn.
static int lShopOffers(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1);
    lua_createtable(L, 0, 0);
    if (player < 0 || player >= PLAYER_SLOTS) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.shopOffers", "bad_player", "player=%lld", (long long)player);
        return 1;
    }
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int last = G32(RVA_SHOP_LAST + block), n = 0;
    for (int i = 0; i <= last && i < SHOP_OFFERS_MAX; i++) {
        int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)i * 4);
        if (count <= 0) continue;
        int monster = G32(RVA_SHOP_OFFERS + block + (size_t)i * 4);
        int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)i * 4);
        const int *costs = (const int *)(gameBase + RVA_SHOP_COSTS + block + (size_t)i * SHOP_COST_COUNT * 4);
        const char *named = (const char *)(gameBase + RVA_SHOP_OFFER_NAMES + block + (size_t)i * SHOP_OFFER_TEXT_SIZE);
        const char *limiter = (const char *)(gameBase + RVA_SHOP_OFFER_LIMITS + block + (size_t)i * SHOP_OFFER_TEXT_SIZE);
        if (!monsterRecord(monster)) continue;
        lua_createtable(L, 0, 16);
        setIntField(L, "offer", i);
        setOfferMonster(L, monster, (const char *)monsterRecord(monster));
        setIntField(L, "count", count);
        setBoolField(L, "commander", named[0] != 0);
        setOfferCosts(L, costs);
        setIntField(L, "gold", costs[0]);
        setIntField(L, "iron", costs[1]);
        setIntField(L, "flags", flags);
        setBoolField(L, "oncePerOffer", (flags & SHOP_FLAG_ONCE) != 0);
        setBoolField(L, "extra", (flags & SHOP_FLAG_EXTRA) != 0);
        setBoolField(L, "aiNever", (flags & SHOP_FLAG_AI_NEVER) != 0);
        setIntField(L, "terrain", G32(RVA_SHOP_OFFER_TERRAIN + block + (size_t)i * 4));
        if (limiter[0]) {
            pushRecordText(L, limiter, SHOP_OFFER_TEXT_SIZE);
            lua_setfield(L, -2, "limiter");
        }
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// coe5.classRecruits(class): the class's recruitment list.
static int lClassRecruits(lua_State *L) {
    lua_Integer cls = luaL_checkinteger(L, 1);
    lua_createtable(L, 0, 0);
    if (cls < 0 || cls >= tableRecords(TABLE_CLASS_RECRUITS)) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.classRecruits", "bad_class", "class=%lld", (long long)cls);
        return 1;
    }
    const unsigned char *list = tableAddress(TABLE_CLASS_RECRUITS) + (size_t)cls * CLASS_RECRUITS_SIZE;
    for (int i = 0; i < CLASS_RECRUITS_MAX; i++) {
        const unsigned char *rec = list + (size_t)i * CLASS_RECRUIT_SIZE;
        if (!rec[RECRUIT_MONSTER_NAME]) break;
        char name[MONSTER_NAME_SIZE];
        memcpy(name, rec + RECRUIT_MONSTER_NAME, MONSTER_NAME_SIZE - 1);
        name[MONSTER_NAME_SIZE - 1] = 0;
        int count = *(const int *)(rec + RECRUIT_COUNT);
        int costs[SHOP_COST_COUNT] = {0};
        costs[0] = *(const int *)(rec + RECRUIT_GOLD);
        costs[1] = *(const int *)(rec + RECRUIT_IRON);
        const short *extra = (const short *)(rec + RECRUIT_EXTRA_COSTS);
        for (int k = 0; k < 2 && extra[2 * k] >= 0; k++)
            if (extra[2 * k] < SHOP_COST_COUNT) costs[extra[2 * k]] = extra[2 * k + 1];
        lua_createtable(L, 0, 14);
        setOfferMonster(L, monsterByName(name), name);
        setIntField(L, "chance", *(const int *)(rec + RECRUIT_CHANCE));
        setIntField(L, "count", count);
        setBoolField(L, "commander", count == -1);
        setOfferCosts(L, costs);
        setIntField(L, "gold", costs[0]);
        setIntField(L, "goldDie", *(const int *)(rec + RECRUIT_GOLD_DIE));
        setIntField(L, "iron", costs[1]);
        setIntField(L, "flags", *(const int *)(rec + RECRUIT_FLAGS));
        setIntField(L, "terrain", *(const int *)(rec + RECRUIT_TERRAIN));
        lua_createtable(L, 0, 0);
        for (int k = 0, n = 0; k < RECRUIT_LIMITERS_MAX; k++) {
            const char *line = (const char *)(rec + RECRUIT_LIMITERS + (size_t)k * MONSTER_NAME_SIZE);
            if (!line[0]) break;
            pushRecordText(L, line, MONSTER_NAME_SIZE);
            lua_rawseti(L, -2, ++n);
        }
        lua_setfield(L, -2, "limiters");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// The squares where the game's AI recruiting can buy for `player`.
static int recruitSquaresOf(const AiRecruitCalls *g, int player, int *xs, int *ys) {
    int depth = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H), n = 0;
    for (int y = 0; y < height && y < MAP_COLUMN_SQUARES; y++)
        for (int x = 0; x < depth && x < MAP_COLUMNS_MAX && n < SHOP_SQUARES_MAX; x++) {
            size_t rec = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
            if ((signed char)G8(RVA_SQUARE_RECORDS + rec + SQUARE_OWNER) != player) continue;
            if (G16(RVA_SQUARE_RECORDS + rec + SQUARE_TERRAIN_TYPE) < 0 || !g->citadelFor(player, x, y)) continue;
            xs[n] = x, ys[n] = y, n++;
        }
    return n;
}

// Why the game's AI recruiting wouldn't buy `player`'s offer on square (x, y), or NULL.
static const char *aiOfferSquareRefusal(const AiRecruitCalls *g, int player, int offer, int x, int y) {
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4);
    int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)offer * 4);
    int monster = G32(RVA_SHOP_OFFERS + block + (size_t)offer * 4);
    if (count < 0) return "item";
    if (count == 0) return "sold_out";
    if (flags & SHOP_FLAG_AI_NEVER) return "not_for_ai";
    int available = g->offerAvailable(player, offer, x, y);
    if (!(available > 1 || (available > 0 && (flags & SHOP_FLAG_AI_IMPORTANT))))
        return available == -1 ? "cannot_pay" : available == 1 ? "too_few_to_upgrade" : "not_here";
    if (G32(RVA_SQUARE_FLAGS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE) & SQUARE_FLAG_RECRUITED) return "square_used";
    for (int level = 1; level <= 3; level++) {
        int power = monsterAbility(monster, MONSTER_ABILITY_AIPOWCOM1 + level - 1);
        if (power > 0 && g->squareAbilityTotal(player, x, y, power, 0) < level) return "aipowcom";
    }
    int most = monsterAbility(monster, MONSTER_ABILITY_AIMAXSHOP);
    if (most == -1 || (most > 0 && g->countMonsterAt(x, y, monster, player) >= most)) return "aimaxshop";
    if (monsterAbility(monster, MONSTER_ABILITY_SHIP) > 0 && g->seaAreaSize(x, y, SHIP_SEA_SQUARES) < SHIP_SEA_SQUARES) return "no_sea";
    if (monsterAbility(monster, MONSTER_ABILITY_AIHOLD) == 1 && !g->squareHasFort(x, y)) return "aihold";
    return NULL;
}

// The player of the AI_RECRUIT call whose wrap is running now, -1 outside one.
static int aiRecruitPlayer(void) {
    for (const LuaFrame *f = luaFrames; f; f = f->up)
        if (f->wrap && luaSites[f->wrap->site].site == SITE_AI_RECRUIT) return (int)f->args->a[0];
    return -1;
}

// Why coe5.aiBuyOffer refuses its arguments, or NULL.
static const char *aiBuyRefusal(int sitePlayer, int humanTurn, lua_Integer player, int controller, lua_Integer offer, int shopLast) {
    if (sitePlayer < 0) return "outside_ai_recruit";
    if (player < 0 || player >= PLAYER_SLOTS) return "bad_player";
    if (player != sitePlayer) return "other_player";
    if (!humanTurn && controller != CONTROLLER_AI) return "not_ai";
    if (offer < 0 || offer > shopLast || offer >= SHOP_OFFERS_MAX) return "bad_offer";
    return NULL;
}

// coe5.recruitSquares(player[, offer]): where the game's AI recruiting can buy for the player.
static int lRecruitSquares(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1);
    int withOffer = !lua_isnoneornil(L, 2);
    lua_Integer offer = withOffer ? luaL_checkinteger(L, 2) : -1;
    lua_createtable(L, 0, 0);
    const AiRecruitCalls *g = aiRecruitGame();
    const char *reason = player < 0 || player >= PLAYER_SLOTS ? "bad_player" : NULL;
    if (!reason && withOffer && (offer < 0 || offer >= SHOP_OFFERS_MAX || offer > G32(RVA_SHOP_LAST + (size_t)player * PLAYER_BLOCK_SIZE)))
        reason = "bad_offer";
    if (!reason && !g) reason = "windows_only";
    if (reason) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.recruitSquares", reason, "player=%lld offer=%lld", (long long)player, (long long)offer);
        return 1;
    }
    int xs[SHOP_SQUARES_MAX], ys[SHOP_SQUARES_MAX], found = recruitSquaresOf(g, (int)player, xs, ys), n = 0;
    for (int i = 0; i < found; i++) {
        if (withOffer && aiOfferSquareRefusal(g, (int)player, (int)offer, xs[i], ys[i])) continue;
        size_t rec = ((size_t)xs[i] * MAP_COLUMN_SQUARES + (size_t)ys[i]) * SQUARE_SIZE;
        lua_createtable(L, 0, 3);
        setIntField(L, "x", xs[i]);
        setIntField(L, "y", ys[i]);
        setBoolField(L, "used", (G32(RVA_SQUARE_FLAGS + rec) & SQUARE_FLAG_RECRUITED) != 0);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// coe5.aiBuyOffer(player, offer, x, y): buys the AI player's offer on square (x, y).
typedef const char *(*AiBuySquareRule)(const AiRecruitCalls *g, int player, int offer, int x, int y);
typedef void (*AiBuyAfter)(int player, int x, int y);
static int lAiBuyOffer(lua_State *L) {
    lua_Integer player = luaL_checkinteger(L, 1), offer = luaL_checkinteger(L, 2);
    lua_Integer x = luaL_checkinteger(L, 3), y = luaL_checkinteger(L, 4);
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    int inRange = player >= 0 && player < PLAYER_SLOTS;
    int controller = inRange ? G16(RVA_SLOT_CONTROLLER + (size_t)player * 2) : CONTROLLER_REMOVED;
    int shopLast = inRange ? G32(RVA_SHOP_LAST + (size_t)player * PLAYER_BLOCK_SIZE) : -1;
    const AiRecruitCalls *g = aiRecruitGame();
    int sitePlayer = aiRecruitPlayer(), humanTurn = 0, onlyX = -1, onlyY = -1;
    AiBuySquareRule humanRule = NULL;
    AiBuyAfter humanAfter = NULL;
#ifdef COE5SE_AI_BUY_HOOK
    if (sitePlayer < 0) humanTurn = (sitePlayer = COE5SE_AI_BUY_HOOK(&onlyX, &onlyY, &humanRule, &humanAfter)) >= 0;
#endif
    const char *reason = aiBuyRefusal(sitePlayer, humanTurn, player, controller, offer, shopLast);
    if (!reason && !g) reason = "windows_only";
    if (!reason && (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES))
        reason = "off_map";
    if (!reason && humanTurn && (x != onlyX || y != onlyY)) reason = "not_screen_square";
    if (!reason && humanTurn && humanRule) {
        reason = humanRule(g, (int)player, (int)offer, (int)x, (int)y);
    } else if (!reason) {
        int xs[SHOP_SQUARES_MAX], ys[SHOP_SQUARES_MAX], found = recruitSquaresOf(g, (int)player, xs, ys), mine = 0;
        for (int i = 0; i < found && !mine; i++) mine = xs[i] == x && ys[i] == y;
        reason = mine ? aiOfferSquareRefusal(g, (int)player, (int)offer, (int)x, (int)y) : "not_recruit_square";
    }
    if (reason) {
        refuseLuaCall(mod, "coe5.aiBuyOffer", reason, "player=%lld offer=%lld x=%lld y=%lld", (long long)player, (long long)offer,
                      (long long)x, (long long)y);
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int flags = G32(RVA_SHOP_OFFER_FLAGS + block + (size_t)offer * 4);
    int monster = G32(RVA_SHOP_OFFERS + block + (size_t)offer * 4);
    int count = G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4);
    int commander = *(const char *)(gameBase + RVA_SHOP_OFFER_NAMES + block + (size_t)offer * SHOP_OFFER_TEXT_SIZE) != 0;
    int goldBefore = G32(RVA_PLAYER_INTS + block), made = -1;
    if (commander) {
        made = g->buyCommander((int)player, (int)offer, (int)x, (int)y);
        g->makeSpawnbooster(made);
    } else {
        g->buyTroops((int)player, (int)offer, (int)x, (int)y);
    }
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    if (!(flags & SHOP_FLAG_EXTRA)) G32(RVA_SQUARE_FLAGS + square) |= SQUARE_FLAG_RECRUITED;
    if (flags & SHOP_FLAG_ONCE) G32(RVA_SHOP_OFFER_COUNTS + block + (size_t)offer * 4) = 0;
    if (humanTurn && humanAfter) humanAfter((int)player, (int)x, (int)y);
    writeLog("ai_offer_bought", "mod=%s player=%lld offer=%lld monster=%d count=%d commander=%d new_commander=%d x=%lld y=%lld "
             "gold_before=%d gold_after=%d flags=0x%x", mod, (long long)player, (long long)offer, monster, count, commander, made,
             (long long)x, (long long)y, goldBefore, G32(RVA_PLAYER_INTS + block), flags);
    lua_pushboolean(L, !commander || made >= 0);
    return 1;
}

// Why coe5.planRitual refuses its call, or NULL.
static const char *ritualPlanRefusal(int inAiSite, lua_Integer commander, int owner, int controller, int plan, int known) {
    const char *reason = ritualPlanScrapRefusal(inAiSite, commander, owner, controller, 0);
    if (reason) return reason;
    if (plan != 0) return "has_plan";
    if (!known) return "not_known";
    return NULL;
}

// coe5.planRitual(commander, ritual): gives an AI commander with no plan a ritual to cast.
static int lPlanRitual(lua_State *L) {
    lua_Integer commander = luaL_checkinteger(L, 1), ritual = luaL_checkinteger(L, 2);
    int owner = -1, controller = CONTROLLER_REMOVED, plan = 0, known = 0;
    if (commander >= 0 && commander < COMMANDER_TABLE_MAX) {
        int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
        if (unit >= 0 && unit < UNIT_TABLE_MAX && G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) > 0) {
            int player = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
            if (player >= 0 && player < PLAYER_SLOTS) owner = player, controller = G16(RVA_SLOT_CONTROLLER + (size_t)player * 2);
        }
        plan = G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE);
        for (int i = 0; i < COMMANDER_KNOWN_RITUALS_MAX && ritual > 0; i++) {
            int slot = G16(RVA_COMMANDER_KNOWN_RITUALS + (size_t)commander * COMMANDER_SIZE + (size_t)i * 2);
            if (slot < 1) break;
            if (slot == ritual) { known = 1; break; }
        }
    }
    LuaScript *s = luaScriptOf(L);
    const char *reason = ritualPlanRefusal(aiSiteRunning(), commander, owner, controller, plan, known);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.planRitual", reason, "commander=%lld ritual=%lld player=%d controller=%d plan=%d",
                      (long long)commander, (long long)ritual, owner, controller, plan);
        lua_pushboolean(L, 0);
        return 1;
    }
    G16(RVA_COMMANDER_AI_PLAN + (size_t)commander * COMMANDER_SIZE) = (short)ritual;
    writeLog("ai_plan_set", "mod=%s commander=%lld player=%d ritual=%lld", s ? s->mod : "?", (long long)commander, owner, (long long)ritual);
    lua_pushboolean(L, 1);
    return 1;
}

// coe5.terrainMatch(code, x, y): whether square (x, y) counts for terrain code `code`.
static int lTerrainMatch(lua_State *L) {
    lua_Integer code = luaL_checkinteger(L, 1), x = luaL_checkinteger(L, 2), y = luaL_checkinteger(L, 3);
    if (code < TERRAIN_MATCH_CODE_MIN || code >= TERRAIN_TABLE_MAX) {
        lua_pushnil(L);
        return 1;
    }
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, ritualTerrainAt((int)code, (int)x, (int)y) > 0);
    return 1;
}

// ---- The trace: coe5.keepTraceLines -------------------------------------------------------------
// coe5.keepTraceLines(words): the game writes only trace lines that start with one of the words.
static int lKeepTraceLines(lua_State *L) {
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    if (lua_isnoneornil(L, 1)) {
        setTraceKeepList(NULL, NULL, 0);
        writeLog("trace_filter_set", "mod=%s words=0", mod);
        lua_pushboolean(L, 1);
        return 1;
    }
    if (!lua_istable(L, 1)) {
        refuseLuaCall(mod, "coe5.keepTraceLines", "not_a_list", "type=%s", luaL_typename(L, 1));
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_Unsigned n = lua_rawlen(L, 1);
    coe5seChargeLuaSteps(L, (size_t)n);
    if (n > INT_MAX) n = INT_MAX;
    const char **words = n ? calloc((size_t)n, sizeof *words) : NULL;
    size_t *lengths = n ? calloc((size_t)n, sizeof *lengths) : NULL;
    const char *reason = n && (!words || !lengths) ? "out_of_memory" : NULL;
    lua_Unsigned bad = 0;
    for (lua_Unsigned i = 1; !reason && i <= n; i++) {
        if (lua_rawgeti(L, 1, (lua_Integer)i) != LUA_TSTRING) reason = "not_a_string";
        else reason = traceKeepWordProblem(words[i - 1] = lua_tolstring(L, -1, &lengths[i - 1]), lengths[i - 1]);
        if (reason) bad = i;
        lua_pop(L, 1);
    }
#ifdef __APPLE__
    if (!reason && n) reason = "windows_only";   // @platform-split trace_filter: Windows only; the Mac trace functions weren't read for it
#else
    if (!reason && n && !installTraceFilter()) reason = "site_unavailable";
#endif
    if (!reason && !setTraceKeepList(words, lengths, (int)n)) reason = "out_of_memory";
    free(words);
    free(lengths);
    if (reason) {
        refuseLuaCall(mod, "coe5.keepTraceLines", reason, "words=%llu entry=%llu", (unsigned long long)n, (unsigned long long)bad);
        lua_pushboolean(L, 0);
        return 1;
    }
    writeLog("trace_filter_set", "mod=%s words=%llu", mod, (unsigned long long)n);
    lua_pushboolean(L, 1);
    return 1;
}
