// ---- The shop's built-in offers, and mods' offer rows ------------------------------------------
// The offers the game's shop build hardcodes, readable by scripts, and the rows scripts add or swap in.
#define SHOP_ROWS_MAX           50
#define SHOP_ROW_LIMITER_SIZE   SHOP_OFFER_TEXT_SIZE
#define SHOP_TEAM_SQUARES       -997
#define SHOP_OWN_SQUARES        -998
#define SHOP_FX_GUILD_KIND      1091
#define SHOP_FX_ITEM_CHANCE     1159
#define SHOP_SKY_PLANE_GROUP    -106
#define SHOP_PORTS_GROUP        -93
#define SHOP_ITEM_EXTRACOST     0x4f7
#define SHOP_ABILITY_NO_SHIP    0xf1
#define SHOP_ABILITY_RECWIZ     0xdf
#define SHOP_ABILITY_RECHERO    0xe0
#define SHOP_ABILITY_MERCPRICE  0xe9
#define SHOP_ABILITY_MERCBOOST  0xea

// How a built-in line's chance is worked out.
enum {
    RULE_STDTROOPS,
    RULE_STDTROOPS_ROLL,
    RULE_SHIP,
    RULE_MERCENARY,
    RULE_HERO,
    RULE_WIZARD,
    RULE_ASSASSIN,
    RULE_GUILD,
    RULE_ENGINEER,
    RULE_HAS_GUILD,
    RULE_LIBRARY,
    RULE_MAGE_PILOT,
    RULE_PEGASUS_RIDER,
    RULE_PEGASUS_LORD,
    RULE_GRYPHON,
    RULE_ITEM,
};
// How a built-in line's price is worked out.
enum {
    PRICE_PLAIN,
    PRICE_MERCENARY,
    PRICE_HERO,
    PRICE_ASSASSIN,
    PRICE_STALKER,
    PRICE_ITEM,
};
// How a line's gold die is rolled.
enum { DIE_EXPLODES, DIE_PLAIN };

typedef struct {
    const char *name;
    int count;
    int gold, goldDie, dieKind, iron;
    int xcostType, xcostAmount;
    int terrain;
    int kind;
    int named;
    int importance;
    int message;
    int rule, base, per, cap, divisor, outOf;
    int kinds[3];
    int price;
} BuiltinOffer;

#define GK(k) ENCHANT_KIND_GUILD_##k
#define SHIP_GUILDS GK(SHIPWRIGHTS), GK(CARTOGRAPHERS), GK(VOID_PILOTS)
#define STD(n, g, i)            {n, 5, g, 0, DIE_EXPLODES, i, -1, 0, 0, 0, 0, 0, 0, RULE_STDTROOPS, 0, 0, 0, 1, 100, {0}, PRICE_PLAIN}
#define MERC(c, n, g, d, div)   {n, c, g, d, DIE_EXPLODES, 0, -1, 0, 0, 2, 0, 1, 1, RULE_MERCENARY, 0, 0, 0, div, 1000, {0}, PRICE_MERCENARY}
#define WIZ(n, g, d, out, less, imp) {n, 1, g, d, DIE_EXPLODES, 0, -1, 0, 0, 2, 1, imp, 1, RULE_WIZARD, less, 0, 0, 1, out, {0}, PRICE_PLAIN}
#define COM(n, g, d, iron, rule, base, per, cap, msg, ...) {n, 1, g, d, DIE_EXPLODES, iron, -1, 0, 0, 2, 1, 0, msg, rule, base, per, cap, 1, 100, {__VA_ARGS__}, PRICE_PLAIN}
// The built-in offers, in the order the build adds them.
static const BuiltinOffer BUILTIN_OFFERS[] = {
    STD("Spearman", 50, 0), STD("Swordsman", 50, 10), STD("Archer", 50, 0), STD("Crossbowman", 50, 5),
    STD("Heavy Infantry", 50, 25),
    {"Catapult", 1, 25, 0, DIE_EXPLODES, 50, -1, 0, 0, 0, 0, 0, 0, RULE_STDTROOPS, 0, 0, 0, 1, 100, {0}, PRICE_PLAIN},
    {"Captain", 1, 40, 10, DIE_PLAIN, 0, -1, 0, 0, 2, 1, 0, 1, RULE_STDTROOPS_ROLL, 20, 0, 0, 1, 100, {0}, PRICE_PLAIN},
    {"Scout", 1, 15, 10, DIE_EXPLODES, 0, -1, 0, 0, 2, 0, 0, 1, RULE_STDTROOPS_ROLL, 10, 0, 0, 1, 100, {0}, PRICE_PLAIN},
    {"Ship", 1, 10, 0, DIE_EXPLODES, 25, -1, 0, SHOP_PORTS_GROUP, 0, 0, 0, 0, RULE_SHIP, 0, 0, 0, 1, 100, {0}, PRICE_PLAIN},
    MERC(5, "Spearman", 25, 30, 1), MERC(10, "Spearman", 50, 60, 1), MERC(10, "Pikeneer", 50, 60, 1),
    MERC(5, "Swordsman", 30, 30, 1), MERC(10, "Swordsman", 50, 60, 1), MERC(10, "Archer", 50, 60, 1),
    MERC(10, "Crossbowman", 50, 60, 1), MERC(20, "Pikeneer", 100, 120, 2), MERC(40, "Pikeneer", 200, 240, 4),
    MERC(10, "Halberdier", 50, 60, 2), MERC(10, "Zweihander", 75, 60, 2), MERC(1, "Catapult", 30, 50, 2),
    {"Hero", 1, 50, 20, DIE_EXPLODES, 5, -1, 0, 0, 2, 1, 0, 1, RULE_HERO, 0, 0, 0, 1, 150, {0}, PRICE_HERO},
    WIZ("Old Wizard", 250, 200, 1000, 1, 1), WIZ("Dark Wizard", 85, 20, 1000, 0, 1), WIZ("Pyromancer", 85, 20, 1000, 0, 1),
    WIZ("Wizard of the Golden Dawn", 85, 20, 1000, 0, 1), WIZ("White Wizard", 85, 20, 1000, 0, 1),
    WIZ("Sea Father", 85, 20, 1000, 0, 1), WIZ("Moon Mage", 85, 20, 1000, 0, 1), WIZ("Astrologer", 85, 20, 1000, 0, 1),
    WIZ("Ice Witch", 85, 20, 1000, 0, 1), WIZ("Ice Druid", 85, 20, 1000, 0, 1), WIZ("Animist", 85, 20, 1000, 0, 1),
    WIZ("Sorceress", 85, 20, 1000, 0, 1), WIZ("Black Sorceress", 100, 20, 1000, 0, 1),
    WIZ("Green Enchantress", 85, 20, 1000, 0, 1), WIZ("Initiate of the Metal Orders", 40, 15, 500, 0, 1),
    WIZ("Hedge Wizard", 40, 20, 500, 0, 0), WIZ("Scarlet Sister", 85, 20, 1000, 0, 1),
    {"Assassin", 1, 0, 20, DIE_EXPLODES, 0, -1, 0, 0, 2, 1, 0, 1, RULE_ASSASSIN, 0, 0, 0, 1, 100, {0}, PRICE_ASSASSIN},
    COM("Apothecary", 30, 20, 0, RULE_GUILD, 2, 2, 100, 1, GK(APOTHECARIES), GK(ALCHEMISTS), GK(ALCHEMIST_MAGUS)),
    COM("Guild Alchemist", 40, 20, 0, RULE_GUILD, 2, 2, 100, 1, GK(ALCHEMISTS), GK(ALCHEMIST_MAGUS)),
    COM("Alchemist Magus", 100, 20, 0, RULE_GUILD, 2, 2, 100, 1, GK(ALCHEMIST_MAGUS)),
    COM("Sage", 30, 20, 0, RULE_GUILD, 2, 1, 15, 1, GK(SAGES), GK(MAGES), GK(ARCANE)),
    COM("Guild Mage", 30, 20, 0, RULE_GUILD, 2, 1, 15, 1, GK(MAGES), GK(ARCANE)),
    COM("Engineer Magus", 100, 20, 0, RULE_ENGINEER, 0, 0, 0, 1, 0),
    {"Galleon", 1, 50, 0, DIE_EXPLODES, 50, -1, 0, SHOP_PORTS_GROUP, 0, 0, 0, 0, RULE_HAS_GUILD, 100, 0, 0, 1, 100, {SHIP_GUILDS}, PRICE_PLAIN},
    {"Mariner", 5, 50, 0, DIE_EXPLODES, 10, -1, 0, SHOP_PORTS_GROUP, 0, 0, 0, 0, RULE_HAS_GUILD, 100, 0, 0, 1, 100, {SHIP_GUILDS}, PRICE_PLAIN},
    COM("Admiral", 40, 10, 0, RULE_HAS_GUILD, 10, 0, 0, 1, SHIP_GUILDS),
    {"Caravan Guard", 7, 50, 0, DIE_EXPLODES, 0, -1, 0, 0, 0, 0, 0, 0, RULE_HAS_GUILD, 100, 0, 0, 1, 100,
     {GK(TRADERS), GK(MERCHANTS), GK(BANKERS)}, PRICE_PLAIN},
    COM("Navigator", 40, 20, 0, RULE_LIBRARY, 6, 0, 0, 1, SHIP_GUILDS),
    COM("Cartographer", 50, 20, 0, RULE_LIBRARY, 6, 0, 0, 1, GK(CARTOGRAPHERS), GK(VOID_PILOTS)),
    {"Void Ship", 1, 75, 0, DIE_EXPLODES, 50, 15, 100, SHOP_PORTS_GROUP, 0, 0, 0, 0, RULE_HAS_GUILD, 100, 0, 0, 1, 100,
     {GK(VOID_PILOTS)}, PRICE_PLAIN},
    COM("Mage Pilot", 40, 20, 0, RULE_MAGE_PILOT, 2, 0, 0, 1, SHIP_GUILDS),
    COM("Grave Robber", 40, 20, 0, RULE_HAS_GUILD, 5, 0, 0, 1, GK(ADVENTURERS), GK(BEASTMASTERS)),
    {"Shadow Stalker", 1, 0, 20, DIE_EXPLODES, 0, -1, 0, 0, 2, 1, 0, 1, RULE_HAS_GUILD, 5, 0, 0, 1, 100,
     {GK(SHADOW_STALKERS)}, PRICE_STALKER},
    {"Pegasus Rider", 5, 100, 0, DIE_EXPLODES, 25, -1, 0, SHOP_SKY_PLANE_GROUP, 1, 0, 0, 0, RULE_PEGASUS_RIDER, 0, 0, 0, 1, 100,
     {GK(BEASTMASTERS)}, PRICE_PLAIN},
    {"Pegasus Lord", 1, 40, 20, DIE_EXPLODES, 5, -1, 0, SHOP_SKY_PLANE_GROUP, 2, 1, 0, 1, RULE_PEGASUS_LORD, 0, 0, 0, 1, 100,
     {GK(BEASTMASTERS)}, PRICE_PLAIN},
    COM("Gryphon Lord", 75, 20, 15, RULE_GRYPHON, 0, 0, 0, 0, GK(BEASTMASTERS)),
    {"Gryphon Rider", 5, 250, 0, DIE_EXPLODES, 75, -1, 0, 0, 1, 0, 0, 1, RULE_GRYPHON, 0, 0, 0, 1, 100, {GK(BEASTMASTERS)}, PRICE_PLAIN},
    {"Magic item", -1, 20, 40, DIE_EXPLODES, 0, -1, 0, 0, 2, 0, 1, 1, RULE_ITEM, 0, 0, 0, 1, 1000, {0}, PRICE_ITEM},
};
#undef STD
#undef MERC
#undef WIZ
#undef COM
#undef SHIP_GUILDS
#undef GK
#define BUILTIN_OFFER_COUNT ((int)(sizeof BUILTIN_OFFERS / sizeof BUILTIN_OFFERS[0]))

// The flags RVA_SHOP_ADD writes for these arguments.
static int shopAddFlags(int kind, int importance) {
    int flags = (kind & 3) ? SHOP_FLAG_ONCE : 0;
    if (kind & 2) flags |= SHOP_FLAG_EXTRA;
    if (importance > 0) flags |= SHOP_FLAG_AI_IMPORTANT;
    else if (importance < 0) flags |= SHOP_FLAG_AI_NEVER;
    return flags;
}

// ---- The game's numbers the chance rules read ---------------------------------------------------
// The player's ability `ability`: its class's value plus its units' (RVA_PLAYER_ABILITY with sum 1).
static int shopPlayerAbility(int player, int ability) {
    return GAME_FN(int (*)(int, int, int), RVA_PLAYER_ABILITY)(player, ability, 1);
}
// The player's guilds of `kind` on squares it (SHOP_OWN_SQUARES) or its team (SHOP_TEAM_SQUARES) holds.
static int shopGuildCount(int player, int kind, int where) {
    return GAME_FN(int (*)(int, int, int, int), RVA_ENCHANT_COUNT)(player, kind, where, where);
}
// 1 when the player has a guild of one of `kinds` on its team's squares.
static int shopHasGuild(int player, const int *kinds) {
    for (int k = 0; k < 3 && kinds[k]; k++)
        if (GAME_FN(int (*)(int, int, int, int), RVA_FIND_ENCHANT)(player, kinds[k], SHOP_TEAM_SQUARES, SHOP_TEAM_SQUARES) >= 0) return 1;
    return 0;
}
// The player's guilds of all of `kinds` on its team's squares.
static int shopGuildsOf(int player, const int *kinds) {
    int n = 0;
    for (int k = 0; k < 3 && kinds[k]; k++) n += shopGuildCount(player, kinds[k], SHOP_TEAM_SQUARES);
    return n;
}
// The player's library levels, max 10.
static int shopLibraryLevels(int player) {
    int n = GAME_FN(int (*)(int), RVA_PLAYER_LIBRARY_TOTAL)(player);
    return n < 10 ? n : 10;
}
// The terrain flags of the square (x, y), 0 for a square with no terrain.
static unsigned long long shopSquareTerrainFlags(int x, int y) {
    short t = *(const short *)(squareRecordAt(x, y) + SQUARE_TERRAIN_TYPE);
    if (t < 0) return 0;
    return *(const unsigned long long *)(gameBase + RVA_TERRAIN_TABLE + (size_t)t * TERRAIN_SIZE + TERRAIN_FLAGS);
}
// The player's town points: town1 squares + 2 x town2 squares it owns, max 10.
static int shopTownPoints(int player) {
    int w = G32(RVA_MAP_W), h = G32(RVA_MAP_H), towns = 0, cities = 0;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++) {
            if (*(const signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) != player) continue;
            unsigned long long f = shopSquareTerrainFlags(x, y);
            towns += (f & TERRAIN_FLAG_TOWN1) != 0;
            cities += (f & TERRAIN_FLAG_TOWN2) != 0;
        }
    int n = towns + 2 * cities;
    return n < 10 ? n : 10;
}
// 1 when the player owns a citadel in terrain group `group`.
static int shopHasCitadelIn(int player, int group) {
    int w = G32(RVA_MAP_W), h = G32(RVA_MAP_H);
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++)
            if (*(const signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) == player &&
                GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y) &&
                GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH)(group, x, y))
                return 1;
    return 0;
}
// The Assassin's chance, which the Shadow Stalker's price also reads.
static int shopAssassinChance(int player) {
    int c = shopPlayerAbility(player, CLASS_ABILITY_ASSASSIN_CHANCE);
    if (shopGuildCount(player, ENCHANT_KIND_GUILD_ASSASSINS, SHOP_OWN_SQUARES) > 0 ||
        shopGuildCount(player, ENCHANT_KIND_GUILD_SHADOW_STALKERS, SHOP_OWN_SQUARES) > 0)
        c += 7;
    return c;
}
// Adventurers and Beastmasters guilds on the player's own squares, the Hero's count.
static int shopAdventurerGuilds(int player) {
    return shopGuildCount(player, ENCHANT_KIND_GUILD_ADVENTURERS, SHOP_OWN_SQUARES) +
           shopGuildCount(player, ENCHANT_KIND_GUILD_BEASTMASTERS, SHOP_OWN_SQUARES);
}

// Built-in line `o`'s chance for `player` this month, in `o->outOf`; -1 when the line isn't offered at all.
static int builtinOfferChance(const BuiltinOffer *o, int player) {
    static const int SAGE_GUILDS[3] = {ENCHANT_KIND_GUILD_SAGES, ENCHANT_KIND_GUILD_MAGES, ENCHANT_KIND_GUILD_ARCANE};
    static const int MAGE_GUILDS[3] = {ENCHANT_KIND_GUILD_MAGES, ENCHANT_KIND_GUILD_ARCANE};
    switch (o->rule) {
    case RULE_STDTROOPS: return shopPlayerAbility(player, CLASS_ABILITY_STDTROOPS) > 0 ? 100 : -1;
    case RULE_STDTROOPS_ROLL: return shopPlayerAbility(player, CLASS_ABILITY_STDTROOPS) > 0 ? o->base : 0;
    case RULE_SHIP: return shopPlayerAbility(player, SHOP_ABILITY_NO_SHIP) < 1 ? 100 : -1;
    case RULE_MERCENARY: {
        int m = (shopGuildCount(player, ENCHANT_KIND_GUILD_MERCENARIES, SHOP_OWN_SQUARES) + shopAdventurerGuilds(player) +
                 shopTownPoints(player)) * 15;
        int boost = shopPlayerAbility(player, SHOP_ABILITY_MERCBOOST);
        if (boost) m = (boost + 100) * m / 100;
        return m / o->divisor;
    }
    case RULE_HERO: {
        int byGuilds = shopAdventurerGuilds(player);
        byGuilds = 5 * (byGuilds < 5 ? byGuilds : 5);
        int own = shopPlayerAbility(player, SHOP_ABILITY_RECHERO);
        return own > byGuilds ? own : byGuilds;
    }
    case RULE_WIZARD: {
        int l = shopLibraryLevels(player);
        return shopPlayerAbility(player, SHOP_ABILITY_RECWIZ) + l + (l >= 1) - o->base;
    }
    case RULE_ASSASSIN: return shopAssassinChance(player);
    case RULE_GUILD: {
        int n = shopGuildsOf(player, o->kinds);
        if (n < 1) return 0;
        int c = o->base + o->per * n;
        return c < o->cap ? c : o->cap;
    }
    case RULE_ENGINEER: {
        int e = shopGuildCount(player, ENCHANT_KIND_GUILD_ENGINEERS, SHOP_TEAM_SQUARES);
        return e > 0 && shopGuildsOf(player, MAGE_GUILDS) > 0 ? 2 * e + 2 : 0;
    }
    case RULE_HAS_GUILD:
        if (!shopHasGuild(player, o->kinds)) return o->base >= 100 ? -1 : 0;
        return o->base;
    case RULE_LIBRARY: return shopHasGuild(player, o->kinds) ? shopLibraryLevels(player) + o->base : 0;
    case RULE_MAGE_PILOT:
        return shopHasGuild(player, o->kinds) && shopHasGuild(player, SAGE_GUILDS) ? shopLibraryLevels(player) + o->base : 0;
    case RULE_PEGASUS_RIDER: return shopGuildCount(player, ENCHANT_KIND_GUILD_BEASTMASTERS, SHOP_TEAM_SQUARES) > 0 ? 100 : -1;
    case RULE_PEGASUS_LORD: {
        int b = shopGuildCount(player, ENCHANT_KIND_GUILD_BEASTMASTERS, SHOP_TEAM_SQUARES);
        return b > 0 && shopHasCitadelIn(player, SHOP_SKY_PLANE_GROUP) ? 15 * b : 0;
    }
    case RULE_GRYPHON: return 10 * shopGuildCount(player, ENCHANT_KIND_GUILD_BEASTMASTERS, SHOP_TEAM_SQUARES);
    case RULE_ITEM: {
        long long fx = GAME_FN(long long (*)(int, int, int, int), RVA_ENCHANT_FX_SUM)(player, SHOP_TEAM_SQUARES, SHOP_TEAM_SQUARES,
                                                                                    SHOP_FX_ITEM_CHANCE);
        return (int)(fx + 2 * shopTownPoints(player)) * 10;
    }
    }
    return 0;   // No log: every rule is handled above.
}

// ---- Mods' rows -------------------------------------------------------------------------------
typedef struct {
    LuaScript *owner;
    int replaces;
    char name[MONSTER_NAME_SIZE];
    int count, gold, goldDie, dieKind, iron, xcostType, xcostAmount, terrain, kind, named, importance, message;
    int chance, outOf;
    int chanceLua;
    int price;
    char limiter[SHOP_ROW_LIMITER_SIZE];
} ShopRow;
static ShopRow shopRows[SHOP_ROWS_MAX];
static int shopRowCount;
static int shopHooksInstalled;

// 1 when row r's script is alive and may run.
static int shopRowLive(const ShopRow *r) { return r->owner && !r->owner->dead; }
// 1 when a live row replaces built-in line `line`.
static int builtinOfferReplaced(int line) {
    for (int i = 0; i < shopRowCount; i++)
        if (shopRows[i].replaces == line && shopRowLive(&shopRows[i])) return 1;
    return 0;
}

// What the build did, for the hooks.
static struct {
    int active, player;
    unsigned char entryFromRecords[SHOP_OFFERS_MAX];
} shopBuild;

// callLuaProtected's push for a row's chance function: the function, then the player.
typedef struct { int ref, player; } ShopChanceCall;
static int pushShopChanceCall(lua_State *L, void *ctx) {
    const ShopChanceCall *c = ctx;
    lua_rawgeti(L, LUA_REGISTRYINDEX, c->ref);
    lua_pushinteger(L, c->player);
    return 1;
}
// Row r's chance for `player`, in its outOf.
static int shopRowChance(const ShopRow *r, int player) {
    if (r->chanceLua >= 0) {
        LuaCallback c = luaCallbacks[r->chanceLua];
        if (!c.s || c.s->dead) return 0;   // No log: a stopped script's row doesn't roll
        ShopChanceCall call = {c.ref, player};
        if (!callLuaProtected(c.s, pushShopChanceCall, &call, 1)) return 0;   // Logged by callLuaProtected
        int chance = lua_isinteger(c.s->L, -1) ? (int)lua_tointeger(c.s->L, -1) : 0;
        if (!lua_isinteger(c.s->L, -1)) refuseLuaCall(c.s->mod, "addShopOffer.chance", "not_an_integer", "offer=%s", r->name);
        lua_pop(c.s->L, 1);
        closeDeadLuaScript(c.s);
        return chance;
    }
    if (r->chance >= 0 || r->replaces < 0) return r->chance;
    int c = builtinOfferChance(&BUILTIN_OFFERS[r->replaces], player);
    return c < 0 ? r->outOf : c;
}
// Row r's gold for `player` this time, its die rolled with the game's own random numbers.
static int shopRowGold(const ShopRow *r, int player, int chance) {
    int die = r->goldDie < 1 ? 0 : r->dieKind == DIE_PLAIN ? GAME_FN(int (*)(int), RVA_RANDOM)(r->goldDie) + 1
                                                         : GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED)(r->goldDie);
    switch (r->price) {
    case PRICE_MERCENARY: {
        int mult = shopPlayerAbility(player, SHOP_ABILITY_MERCPRICE);
        int pct = mult ? mult * 100 : 100;
        if (pct < 50) pct = 50;
        return (die + r->gold) * pct / 100;
    }
    case PRICE_HERO: {
        int n = shopAdventurerGuilds(player);
        return die + r->gold - 5 * (n < 5 ? n : 5);
    }
    case PRICE_ASSASSIN: {
        int g = die + (11 - chance) * 10;
        return g > 50 ? g : 50;
    }
    case PRICE_STALKER: {
        int g = die + (12 - shopAssassinChance(player)) * 10;
        return g > 60 ? g : 60;
    }
    default: return r->gold + die;
    }
}

// Rolls row r for `player` and adds its offer when the roll comes up.
static void rollShopRow(const ShopRow *r, int player) {
    int chance = shopRowChance(r, player);
    if (chance < 1) return;   // No log: no offer this month
    if (chance < r->outOf && GAME_FN(int (*)(int), RVA_RANDOM)(r->outOf) >= chance) return;   // No log: no offer this month
    short xcost[3] = {(short)r->xcostType, (short)r->xcostAmount, -1};
    const char *name = r->name;
    int gold;
    if (r->count == -1) {
        int item = GAME_FN(int (*)(int, int, int, int, int), RVA_RANDOM_ITEM_PICK)(-1, -1, 0, 0, 0);
        if (item < 0) return;   // No log: the game's pick found none, as the build's own does
        int dice = GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED)(r->goldDie) + GAME_FN(int (*)(int), RVA_DIE_OPEN_ENDED)(r->goldDie);
        int mult = shopPlayerAbility(player, SHOP_ABILITY_MERCPRICE);
        int pct = mult ? (mult + 1) * 50 : 100;
        int extracost = GAME_FN(int (*)(int, int, int), RVA_ITEM_ABILITY_NTH)(item, SHOP_ITEM_EXTRACOST, 0);
        gold = r->price == PRICE_ITEM ? (extracost + r->gold + dice) * pct / 100 : r->gold + dice;
        name = (const char *)tableAddress(TABLE_ITEM) + (size_t)item * ITEM_SIZE;
    } else {
        gold = shopRowGold(r, player, chance);
    }
    typedef int (*ShopAddFn)(int, const char *, int, int, int, int, int, int, short *, int, const char *);
    int entry = GAME_FN(ShopAddFn, RVA_SHOP_ADD)(r->count, name, gold, r->iron, r->named, r->kind, player, r->importance,
                                                r->xcostType >= 0 ? xcost : NULL, r->terrain, r->limiter);
    if (entry < 0) return;   // Logged by the game ("out of shop space", "failed to add")
    if (r->message) GAME_FN(int (*)(int), RVA_SHOP_OFFER_MESSAGE)(player);
    writeLog("shop_row_offered", "mod=%s offer=%s player=%d gold=%d", r->owner->mod, name, player, gold);
}

// ---- After the build: the replaced lines' entries and their message lines ----------------------
// 1 when shop entry `e` of the player's block is built-in line `o`'s.
static int entryIsBuiltinLine(size_t block, int e, const BuiltinOffer *o) {
    int count = PG32(RVA_SHOP_OFFER_COUNTS + block + (size_t)e * 4);
    if (count != o->count) return 0;
    if (PG32(RVA_SHOP_OFFER_TERRAIN + block + (size_t)e * 4) != o->terrain) return 0;
    if (PG32(RVA_SHOP_OFFER_FLAGS + block + (size_t)e * 4) != shopAddFlags(o->kind, o->importance)) return 0;
    if (count == -1) return 1;
    const unsigned char *m = monsterRecord(PG32(RVA_SHOP_OFFERS + block + (size_t)e * 4));
    return m && recordNameIs(m, MONSTER_NAME_SIZE, o->name);
}
static unsigned char *shopMessage(int m) { return GAME_PTR(RVA_MESSAGE_TABLE) + (size_t)m * MESSAGE_SIZE; }

// The line RVA_SHOP_OFFER_MESSAGE wrote for shop entry `e`, and its message's title.
static const char *shopOfferLine(size_t block, int e, char *line, size_t size) {
    int count = PG32(RVA_SHOP_OFFER_COUNTS + block + (size_t)e * 4), what = PG32(RVA_SHOP_OFFERS + block + (size_t)e * 4);
    int gold = PG32(RVA_SHOP_COSTS + block + (size_t)e * SHOP_COST_COUNT * 4);
    if (count == -1) {
        snprintf(line, size, "%.40s for %d gold.", (const char *)tableAddress(TABLE_ITEM) + (size_t)what * ITEM_SIZE, gold);
        return "Magic Item for Sale";
    }
    char name[112] = "";
    const unsigned char *m = monsterRecord(what);
    if (m) GAME_FN(const char *(*)(int, const char *, char *), RVA_COUNT_NAME_TEXT)(count, (const char *)m, name);
    snprintf(line, size, "%d %.80s for %d gold.", count, name, gold);
    return "Recruitment Offer";
}

// Removes `line` from the player's first message titled `title` that holds it; 1 when it did.
static int dropShopOfferLine(int player, const char *title, const char *line) {
    size_t n = strlen(line);
    for (int m = 0; m <= G32(RVA_MESSAGE_LAST); m++) {
        unsigned char *msg = shopMessage(m);
        if (*(const short *)(msg + MESSAGE_TO) != player || strcmp((const char *)msg + MESSAGE_TITLE, title)) continue;
        char *text = (char *)msg + MESSAGE_TEXT;
        for (char *at = text; *at;) {
            char *end = strchr(at, '\n');
            size_t len = end ? (size_t)(end - at) : strlen(at);
            if (len == n && !memcmp(at, line, n)) {
                if (end) memmove(at, end + 1, strlen(end + 1) + 1);
                else if (at > text) at[-1] = 0;
                else at[0] = 0;
                if (!text[0]) *(short *)(msg + MESSAGE_TO) = -1;
                return 1;
            }
            if (!end) break;
            at = end + 1;
        }
    }
    return 0;
}

// Merges the player's Recruitment Offer messages into its first, as the build's last step does.
static void mergeShopMessages(int player) {
    int last = G32(RVA_MESSAGE_LAST), first = -1;
    for (int m = 0; m <= last; m++) {
        unsigned char *msg = shopMessage(m);
        if (*(const short *)(msg + MESSAGE_TO) != player || strcmp((const char *)msg + MESSAGE_TITLE, "Recruitment Offer")) continue;
        if (first < 0) {
            first = m;
            continue;
        }
        char *into = (char *)shopMessage(first) + MESSAGE_TEXT;
        const char *from = (const char *)msg + MESSAGE_TEXT;
        size_t have = strnlen(into, ADD_MESSAGE_TEXT_SIZE - 1);
        if (have + 1 < ADD_MESSAGE_TEXT_SIZE) {
            into[have++] = '\n';
            size_t n = strnlen(from, ADD_MESSAGE_TEXT_SIZE), room = ADD_MESSAGE_TEXT_SIZE - 1 - have;
            memmove(into + have, from, n < room ? n : room);
            into[have + (n < room ? n : room)] = 0;
        }
        *(short *)(msg + MESSAGE_TO) = -1;
    }
}

// Empties the entries of replaced built-in lines that the build made for `player`, and drops their lines.
static void emptyReplacedOffers(int player) {
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int last = PG32(RVA_SHOP_LAST + block), line = 0;
    for (int e = 0; e <= last && e < SHOP_OFFERS_MAX; e++) {
        if (shopBuild.entryFromRecords[e]) continue;
        int l = line;
        while (l < BUILTIN_OFFER_COUNT && !entryIsBuiltinLine(block, e, &BUILTIN_OFFERS[l])) l++;
        if (l == BUILTIN_OFFER_COUNT) {
            writeLog("shop_offer_unmatched", "player=%d entry=%d", player, e);
            return;   // Logged above; the rest stays as the game made it
        }
        line = l + 1;
        const BuiltinOffer *o = &BUILTIN_OFFERS[l];
        if (!builtinOfferReplaced(l)) continue;
        char text[160];
        const char *title = shopOfferLine(block, e, text, sizeof text);
        int dropped = o->message && dropShopOfferLine(player, title, text);
        PG32(RVA_SHOP_OFFER_COUNTS + block + (size_t)e * 4) = 0;
        writeLog("shop_offer_replaced", "player=%d offer=%s entry=%d message_line=%d", player, o->name, e, dropped);
    }
}

// ---- The hooks --------------------------------------------------------------------------------
// The class list's and special locations' calls: a mod's recruitment bonuses, and marks what each record added.
#ifndef __APPLE__
typedef uintptr_t (*ShopProcessOfferFn)(int, void *, int, int, int);
static uintptr_t shopProcessOfferHook(int player, void *record, int temples, int libraries, int terrain) {
#else
// @platform-split shop_process_offer: Mac's RVA_SHOP_PROCESS_OFFER returns nothing
typedef void (*ShopProcessOfferFn)(int, void *, int, int, int);
static void shopProcessOfferHook(int player, void *record, int temples, int libraries, int terrain) {
#endif
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int entry = PG32(RVA_SHOP_LAST + block);
    unsigned char copy[CLASS_RECRUIT_SIZE];
    int bonus = recBonusOfferCopy(player, record, copy);
    recBonusRoll.record = record, recBonusRoll.rolled = bonus ? copy : record;
#ifndef __APPLE__
    uintptr_t result =
#endif
        GAME_FN(ShopProcessOfferFn, RVA_SHOP_PROCESS_OFFER)(player, bonus ? copy : record, temples, libraries, terrain);
    recBonusRoll.record = recBonusRoll.rolled = NULL;
    if (bonus) recBonusAfterRoll(player, record, copy, entry);
    if (shopBuild.active && shopBuild.player == player) {
        for (int e = entry + 1; e <= PG32(RVA_SHOP_LAST + block) && e < SHOP_OFFERS_MAX; e++) shopBuild.entryFromRecords[e] = 1;
    }
#ifndef __APPLE__
    return result;
#endif
}

// player_turn_start's call of the shop build: the game's build, then the replaced lines and mods' rows.
static void shopBuildHook(int player) {
    int any = 0;
    for (int i = 0; i < shopRowCount && !any; i++) any = shopRowLive(&shopRows[i]);
    if (!any || isNetworkGame() || player < 0 || player >= PLAYER_SLOTS) {
        GAME_FN(void (*)(int), RVA_SHOP_BUILD)(player);
        return;   // No log: with no live row, in a network game, or for a slot outside 0-31 the build runs untouched
    }
    memset(shopBuild.entryFromRecords, 0, sizeof shopBuild.entryFromRecords);
    shopBuild.active = 1, shopBuild.player = player;
    GAME_FN(void (*)(int), RVA_SHOP_BUILD)(player);
    shopBuild.active = 0;
    emptyReplacedOffers(player);
    for (int i = 0; i < shopRowCount; i++)
        if (shopRowLive(&shopRows[i])) rollShopRow(&shopRows[i], player);
    mergeShopMessages(player);
}

// Points the shop build's call and its two record calls at CoE5SE, once; 1 when all three are in.
static int installShopOfferHooks(void) {
    if (shopHooksInstalled) return shopHooksInstalled > 0;
    int ok = pointCustomPlaneCall(RVA_SHOP_CLASS_OFFER_CALL, RVA_SHOP_PROCESS_OFFER, (void *)shopProcessOfferHook, "shop_class_offer") &&
             pointCustomPlaneCall(RVA_SHOP_LOCATION_OFFER_CALL, RVA_SHOP_PROCESS_OFFER, (void *)shopProcessOfferHook,
                                  "shop_location_offer") &&
             pointCustomPlaneCall(RVA_SHOP_BUILD_CALL, RVA_SHOP_BUILD, (void *)shopBuildHook, "shop_build");
    shopHooksInstalled = ok ? 1 : -1;
    writeLog("shop_offer_hooks_installed", "ok=%d", ok);
    return ok;
}

// ---- Lua: coe5.builtinOffers, builtinOfferChance, replaceBuiltinOffer, addShopOffer, guildCount ----
// The fields of one built-in line or row, as the Lua functions give and take them.
static void pushBuiltinOffer(lua_State *L, int line) {
    const BuiltinOffer *o = &BUILTIN_OFFERS[line];
    lua_createtable(L, 0, 20);
    setIntField(L, "id", line + 1);
    lua_pushstring(L, o->name);
    lua_setfield(L, -2, "name");
    setIntField(L, "count", o->count);
    setBoolField(L, "item", o->count == -1);
    setBoolField(L, "commander", o->named);
    setIntField(L, "gold", o->gold);
    setIntField(L, "goldDie", o->goldDie);
    setBoolField(L, "goldDieExplodes", o->dieKind == DIE_EXPLODES);
    setIntField(L, "iron", o->iron);
    lua_createtable(L, 0, 1);
    if (o->xcostType >= 0) {
        lua_pushinteger(L, o->xcostAmount);
        lua_rawseti(L, -2, o->xcostType);
    }
    lua_setfield(L, -2, "costs");
    setIntField(L, "terrain", o->terrain);
    setBoolField(L, "oncePerOffer", (o->kind & 3) != 0);
    setBoolField(L, "extra", (o->kind & 2) != 0);
    setBoolField(L, "aiImportant", o->importance > 0);
    setBoolField(L, "aiNever", o->importance < 0);
    setBoolField(L, "message", o->message);
    setIntField(L, "outOf", o->outOf);
    static const char *const PRICES[] = {"plain", "mercenary", "hero", "assassin", "shadow_stalker", "item"};
    lua_pushstring(L, PRICES[o->price]);
    lua_setfield(L, -2, "price");
}

// coe5.builtinOffers(): the 61 offers the shop build adds from its own code, in its order.
static int lBuiltinOffers(lua_State *L) {
    lua_createtable(L, BUILTIN_OFFER_COUNT, 0);
    for (int i = 0; i < BUILTIN_OFFER_COUNT; i++) {
        pushBuiltinOffer(L, i);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

// coe5.builtinOfferChance(id, player): the line's chance this month for the player, and what it's out of.
static int lBuiltinOfferChance(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1), player = luaCheckPlayer(L, 2);
    if (id < 1 || id > BUILTIN_OFFER_COUNT || player < 0 || player >= PLAYER_SLOTS) {
        refuseLuaCall(runningLuaMod(), "coe5.builtinOfferChance", id < 1 || id > BUILTIN_OFFER_COUNT ? "bad_id" : "bad_player",
                      "id=%lld player=%lld", (long long)id, (long long)player);
        lua_pushnil(L);
        return 1;
    }
    const BuiltinOffer *o = &BUILTIN_OFFERS[id - 1];
    lua_pushinteger(L, builtinOfferChance(o, (int)player));
    lua_pushinteger(L, o->outOf);
    return 2;
}

// The guild kind ritual `name` sets up (its effect 1091), or -1.
static int guildKindOfRitual(const char *name) {
    int r = ritualByName(name, 0);
    if (r < 0) return -1;   // No log: lGuildCount refuses it
    return ritualFirstFx(gameBase + ritualTableRva() + (size_t)r * RITUAL_SIZE, SHOP_FX_GUILD_KIND, -1);
}

// coe5.guildCount(player, guild[, ownSquares]): the player's guilds of that guild ritual, as the shop counts them.
static int lGuildCount(lua_State *L) {
    lua_Integer player = luaCheckPlayer(L, 1);
    const char *guild = luaL_checkstring(L, 2);
    int own = lua_toboolean(L, 3);
    int kind = guildKindOfRitual(guild);
    if (player < 0 || player >= PLAYER_SLOTS || kind < 1) {
        refuseLuaCall(runningLuaMod(), "coe5.guildCount", kind < 1 ? "not_a_guild" : "bad_player", "player=%lld guild=%s",
                      (long long)player, guild);
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, shopGuildCount((int)player, kind, own ? SHOP_OWN_SQUARES : SHOP_TEAM_SQUARES));
    return 1;
}

// Reads the optional integer field `key` of the table at 1 into *out; 0 with lua_refused when it isn't one.
static int shopRowIntField(lua_State *L, const char *call, const char *key, int *out, int lo, int hi) {
    int t = lua_getfield(L, 1, key);
    int ok = 1;
    if (t != LUA_TNIL) {
        if (!lua_isinteger(L, -1) || lua_tointeger(L, -1) < lo || lua_tointeger(L, -1) > hi) {
            refuseLuaCall(runningLuaMod(), call, "bad_field", "field=%s", key);
            ok = 0;
        } else *out = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return ok;
}
// Reads the optional boolean field `key` of the table at 1 into *out.
static void shopRowBoolField(lua_State *L, const char *key, int *out) {
    if (lua_getfield(L, 1, key) != LUA_TNIL) *out = lua_toboolean(L, -1);
    lua_pop(L, 1);
}

// Fills row r from the fields of the table at 1; 0 with the refusal logged.
static int readShopRowFields(lua_State *L, const char *call, ShopRow *r) {
    if (lua_getfield(L, 1, "name") != LUA_TNIL) {
        const char *name = lua_tostring(L, -1);
        if (!name || monsterByName(name) < 0) {
            refuseLuaCall(runningLuaMod(), call, "no_monster", "name=%s", name ? name : "(not a string)");
            lua_pop(L, 1);
            return 0;
        }
        snprintf(r->name, sizeof r->name, "%.*s", (int)sizeof r->name - 1, name);
    }
    lua_pop(L, 1);
    if (!r->name[0]) {
        refuseLuaCall(runningLuaMod(), call, "no_name", "%s", "");
        return 0;
    }
    int goldBefore = r->gold, dieBefore = r->goldDie, once = (r->kind & 3) != 0, extra = (r->kind & 2) != 0;
    int aiImportant = r->importance > 0, aiNever = r->importance < 0, explodes = r->dieKind == DIE_EXPLODES;
    if (!shopRowIntField(L, call, "count", &r->count, 1, 1000) || !shopRowIntField(L, call, "gold", &r->gold, 0, 1000000) ||
        !shopRowIntField(L, call, "goldDie", &r->goldDie, 0, 100000) || !shopRowIntField(L, call, "iron", &r->iron, 0, 1000000) ||
        !shopRowIntField(L, call, "terrain", &r->terrain, TERRAIN_MATCH_CODE_MIN, 11999) ||
        !shopRowIntField(L, call, "outOf", &r->outOf, 1, 1000000))
        return 0;   // Logged by shopRowIntField
    shopRowBoolField(L, "commander", &r->named);
    shopRowBoolField(L, "oncePerOffer", &once);
    shopRowBoolField(L, "extra", &extra);
    shopRowBoolField(L, "aiImportant", &aiImportant);
    shopRowBoolField(L, "aiNever", &aiNever);
    shopRowBoolField(L, "goldDieExplodes", &explodes);
    shopRowBoolField(L, "message", &r->message);
    r->kind = extra ? 2 : once ? 1 : 0;
    r->importance = aiNever ? -1 : aiImportant ? 1 : 0;
    r->dieKind = explodes ? DIE_EXPLODES : DIE_PLAIN;
    if (r->gold != goldBefore || r->goldDie != dieBefore) r->price = PRICE_PLAIN;
    if (lua_getfield(L, 1, "costs") == LUA_TTABLE) {
        r->xcostType = -1;
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            if (!lua_isinteger(L, -2) || !lua_isinteger(L, -1) || lua_tointeger(L, -2) < 2 || lua_tointeger(L, -2) >= SHOP_COST_COUNT ||
                lua_tointeger(L, -1) < 0 || lua_tointeger(L, -1) > 32767 || r->xcostType >= 0) {
                refuseLuaCall(runningLuaMod(), call, "bad_costs", "%s", "one extra cost, type 2 to 18, amount 0 to 32767");
                lua_pop(L, 3);
                return 0;
            }
            r->xcostType = (int)lua_tointeger(L, -2), r->xcostAmount = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    if (lua_getfield(L, 1, "limiter") != LUA_TNIL) snprintf(r->limiter, sizeof r->limiter, "%.47s", lua_tostring(L, -1) ? lua_tostring(L, -1) : "");
    lua_pop(L, 1);
    int t = lua_getfield(L, 1, "chance");
    if (t == LUA_TFUNCTION) {
        r->chanceLua = newLuaCallback(L, lua_gettop(L), 0);
        if (r->chanceLua < 0) {
            refuseLuaCall(runningLuaMod(), call, "out_of_memory", "name=%s", r->name);
            lua_pop(L, 1);
            return 0;
        }
    } else if (t != LUA_TNIL) {
        if (!lua_isinteger(L, -1) || lua_tointeger(L, -1) < 0) {
            refuseLuaCall(runningLuaMod(), call, "bad_field", "field=%s", "chance");
            lua_pop(L, 1);
            return 0;
        }
        r->chance = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return 1;
}

// Adds row r for the running script; its number, or -1 with the refusal logged.
static int addShopRow(lua_State *L, const char *call, ShopRow *r) {
    if (shopRowCount >= SHOP_ROWS_MAX) {
        refuseLuaCall(runningLuaMod(), call, "too_many_rows", "rows=%d max=%d", shopRowCount, SHOP_ROWS_MAX);
        return -1;
    }
    if (!installShopOfferHooks()) {
        refuseLuaCall(runningLuaMod(), call, "site_unavailable", "name=%s", r->name);
        return -1;
    }
    r->owner = luaScriptOf(L);
    shopRows[shopRowCount] = *r;
    writeLog("shop_row_added", "mod=%s row=%d name=%s replaces=%d", r->owner->mod, shopRowCount + 1, r->name, r->replaces + 1);
    return ++shopRowCount;
}

// coe5.replaceBuiltinOffer(id[, fields]): turns built-in line `id` off and adds a row in its place; the row's number or false.
static int lReplaceBuiltinOffer(lua_State *L) {
    lua_Integer id = luaL_checkinteger(L, 1);
    if (lua_isnoneornil(L, 2)) {
        lua_settop(L, 1);
        lua_newtable(L);
    }
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_remove(L, 1);
    if (refuseFromDeadLuaScript(L, "coe5.replaceBuiltinOffer")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    if (id < 1 || id > BUILTIN_OFFER_COUNT) {
        refuseLuaCall(runningLuaMod(), "coe5.replaceBuiltinOffer", "bad_id", "id=%lld", (long long)id);
        lua_pushboolean(L, 0);
        return 1;
    }
    const BuiltinOffer *o = &BUILTIN_OFFERS[id - 1];
    ShopRow r = {.replaces = (int)id - 1, .count = o->count, .gold = o->gold, .goldDie = o->goldDie, .dieKind = o->dieKind,
                 .iron = o->iron, .xcostType = o->xcostType, .xcostAmount = o->xcostAmount, .terrain = o->terrain, .kind = o->kind,
                 .named = o->named, .importance = o->importance, .message = o->message, .chance = -1, .outOf = o->outOf,
                 .chanceLua = -1, .price = o->price};
    snprintf(r.name, sizeof r.name, "%s", o->name);
    int row = readShopRowFields(L, "coe5.replaceBuiltinOffer", &r) ? addShopRow(L, "coe5.replaceBuiltinOffer", &r) : -1;
    if (row < 0) lua_pushboolean(L, 0);
    else lua_pushinteger(L, row);
    return 1;
}

// coe5.addShopOffer{name=, chance=, ...}: a new offer every player's shop rolls each month; the row's number or false.
static int lAddShopOffer(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    if (refuseFromDeadLuaScript(L, "coe5.addShopOffer")) {
        lua_pushboolean(L, 0);
        return 1;
    }
    ShopRow r = {.replaces = -1, .count = 1, .dieKind = DIE_EXPLODES, .xcostType = -1, .message = 1, .chance = 0, .outOf = 100,
                 .chanceLua = -1, .price = PRICE_PLAIN};
    int row = readShopRowFields(L, "coe5.addShopOffer", &r) ? addShopRow(L, "coe5.addShopOffer", &r) : -1;
    if (row < 0) lua_pushboolean(L, 0);
    else lua_pushinteger(L, row);
    return 1;
}
