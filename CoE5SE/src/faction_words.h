// Mods' factions from .c5se words: the newfaction block, and each faction's slot in a game.
#pragma once

#define NEWFACTION_WORD      "newfaction"
#define FACTION_NAME_SIZE    32
#define FACTION_LIKE_DEFAULT 24
#define FACTION_EARLIER_PASS 2
#define FACTION_FROM_SCRIPT  3
#define FACTION_MOD_SCRIPT   -2
#define FACTION_ACTIVE_ALWAYS 0
#define FACTION_ACTIVE_NEVER  -1
#define FACTION_SPAWN_MAX     8
#define FACTION_GROUP_SIZE    160
#define FACTION_FUEL_SIZE     MONSTER_NAME_SIZE
#define FACTION_NEWS_SIZE     400
#define FACTION_AVOID_MAX     8
#define FACTION_UNITS_MAX     16
#define FACTION_TAKES_MAX     8
#define FACTION_UNIT_COUNT_MAX 1000
enum { FACTION_SITE_NONE, FACTION_SITE_OWN_CITADEL, FACTION_SITE_MONUMENT };
enum { FACTION_TERRAIN_ANY, FACTION_TERRAIN_MAGMA, FACTION_TERRAIN_ICE, FACTION_TERRAIN_WASTE };

// One factionspawn line: with at least `min` fuel (and the terrain), a draw below `chance` makes `group`, using `use`.
typedef struct {
    int min, use, chance, terrain;
    char group[FACTION_GROUP_SIZE];
} FactionSpawn;

// One factionunits line.
typedef struct {
    char monster[MONSTER_NAME_SIZE], plane[PLANE_NAME_SIZE];
    int count, chance, terrain;
} FactionUnits;

// One factionpopulateterrain line: the Independents' stock on the terrain and plane ("" any) becomes the faction's.
typedef struct {
    char plane[PLANE_NAME_SIZE];
    int terrain;
} FactionTake;

typedef struct {
    char name[FACTION_NAME_SIZE];
    char mod[MOD_NAME_MAX];
    int like;
    float colour[3];
    int hasColour;
    char home[PLANE_NAME_SIZE];
    int hasHome;
    int active, hasActive;
    int populate, hasPopulate;
    int wanderAs, hasWanderAs, wanderGate, hasWanderGate, tithe, hasTithe, worldEvents, hasWorldEvents;
    int conquestIndependent, hasConquest, statue, hasStatue;
    int spawnSite, hasSpawnSite;
    char spawnPlane[PLANE_NAME_SIZE];
    char fuel[FACTION_FUEL_SIZE];
    int hasFuel;
    FactionSpawn spawns[FACTION_SPAWN_MAX];
    int spawnCount, hasSpawns;
    char monument[MONSTER_NAME_SIZE];
    int hasMonument, monumentRule, hasMonumentRule;
    int eventTurn, eventChance, eventTerrain, hasEvent;
    char shiftMonster[MONSTER_NAME_SIZE], loyaltyMonster[MONSTER_NAME_SIZE];
    int shiftPercent, shiftEach, hasShift, hasLoyalty;
    int gatherAbility, hasGather;
    int gatewayFuel, gatewayRuin, gatewayGate, gatewayExit, hasGateway;
    char gatewayNews[FACTION_NEWS_SIZE];
    int horrors, hasHorrors;
    char avoidName[FACTION_AVOID_MAX][FACTION_NAME_SIZE];
    int avoidPercent[FACTION_AVOID_MAX], avoidCount, hasAvoid;
    int caravanTurn, caravanChance, caravanTerrain, hasCaravan;
    char caravanPlane[PLANE_NAME_SIZE], caravanGroup[FACTION_GROUP_SIZE], caravanNews[FACTION_NEWS_SIZE];
    char eventPlane[PLANE_NAME_SIZE], eventUnfinished[MONSTER_NAME_SIZE], eventNews[FACTION_NEWS_SIZE];
    FactionUnits units[FACTION_UNITS_MAX];
    int unitCount;
    FactionTake takes[FACTION_TAKES_MAX];
    int takeCount;
    int fromPass;
} Faction;

static Faction factions[FACTION_SLOT_TARGET];
static int factionCount;

// The game's names of slots 24 to 30, the presets factionlike takes.
static const char *const FACTION_PRESET_NAMES[] = {"Independents", "Special Monsters", "Horrors", "Hades", "Inferno", "Celestial", "Empire"};

// The vanilla slot whose preset `name` names, or -1.
static int factionPresetNamed(const char *name) {
    for (int i = 0; i < (int)(sizeof FACTION_PRESET_NAMES / sizeof FACTION_PRESET_NAMES[0]); i++)
        if (!strcasecmp(name, FACTION_PRESET_NAMES[i])) return 24 + i;
    return !strcasecmp(name, "Kingdom") ? 30 : -1;
}

// A preset's home plane name ("" for none) and active rule: the vanilla movers' planes and gates.
static const char *factionPresetHome(int like) { return like == 28 ? "Inferno" : like == 27 ? "Hades" : like == 29 ? "Celestial Plane" : ""; }
static int factionPresetActive(int like) { return like == 28 ? 1 : like == 29 ? 2 : FACTION_ACTIVE_ALWAYS; }
static int factionPresetPopulate(int like) { return like == 24 || like == 26 || like == 27 || like == 28; }
static int factionPresetWander(int like) { return like == 24 || like == 27 || like == 28 || like == 29 ? like : -1; }

// A preset's spawns, as the movers' tables hold them.
static const FactionSpawn INFERNO_SPAWNS[] = {
    {40, 40, 100, FACTION_TERRAIN_ANY, "c*Greater Devil & 1d3*Devil & 2d6*Hellhound & 2d6*Fiery Imp"},
    {10, 10, 25, FACTION_TERRAIN_ANY, "c*Bone Devil & 1d4*Bone Imp & 1d6*Longdead"},
    {10, 10, 100, FACTION_TERRAIN_MAGMA, "c*Devil & 1d6+1*Fiery Imp"},
    {10, 10, 100, FACTION_TERRAIN_ICE, "c*Frost Fiend & 1d6+1*Ice Imp"},
    {10, 10, 50, FACTION_TERRAIN_ANY, "c*Demon Knight & 1d4+1*Bloodsworn"},
    {10, 10, 100, FACTION_TERRAIN_ANY, "c*Disease Demon & 1d3*Spine Devil"},
};
static const FactionSpawn HADES_SPAWNS[] = {
    {30, 30, 100, FACTION_TERRAIN_ANY, "c*Wraith & 1d2*Ghost & 3d6*Shade & 2d4*Shade Beast & 3d6*Ghost Warrior"},
    {20, 20, 25, FACTION_TERRAIN_ANY, "c*Tartarian Spirit & 1d4*Shade Beast"},
    {10, 10, 50, FACTION_TERRAIN_ANY, "c*Bane Lord & 1d4*Bane"},
    {10, 10, 25, FACTION_TERRAIN_ANY, "c*Wraith & 1d4*Umbral"},
    {10, 10, 50, FACTION_TERRAIN_ANY, "c*Shade Mage & 2d4*Shade"},
};
// Hades' founding news, as the game's event sends it.
static const char *const HADES_MONUMENT_NEWS = "It is said that %s has started to construct the Monument of Hades. A construction that will draw "
                                               "the spirits of Hades to it and empower them to kill the living beings of Elysium. This "
                                               "construction must be stopped if Elysium is to survive.";
// The Inferno's gateway news, as the game sends it.
static const char *const INFERNO_GATEWAY_NEWS =
    "Infernal forces has managed to restore the gateway to Inferno by sacrificing a huge number of sinners there. Elysium risks getting "
    "overrun by devils and demons if this gateway isn't closed once again.";
// The Merchants' caravan news, as the game sends it.
static const char *const MERCHANT_CARAVAN_NEWS = "A wealthy merchant from a land across the sea is travelling through the country. He has recently "
                                                 "come ashore in %s.";
// A preset's caravan: the Special Monsters' (RVA_CARAVAN_EVENT_TURN_TEST's numbers, the Merchants' crew), none for the others.
static void factionCaravanDefaults(Faction *f) {
    int merchants = f->like == 25;
    f->caravanTurn = merchants ? 12 : -1, f->caravanChance = 2, f->caravanTerrain = -93;
    f->caravanPlane[0] = f->caravanGroup[0] = 0;
    snprintf(f->caravanNews, sizeof f->caravanNews, "%s", merchants ? MERCHANT_CARAVAN_NEWS : "");
}

// A preset's monument founding: Hades' event (RVA_MONUMENT_EVENT_TURN_TEST), none for the others.
static void factionPresetMonumentEvent(Faction *f) {
    int hades = f->like == 27;
    f->eventTurn = hades ? 48 : -1, f->eventChance = 8, f->eventTerrain = 38;
    f->eventPlane[0] = 0;
    snprintf(f->eventUnfinished, sizeof f->eventUnfinished, "%s", hades ? "Unfinished Monument" : "");
    snprintf(f->eventNews, sizeof f->eventNews, "%s", hades ? HADES_MONUMENT_NEWS : "");
}

static void factionPresetSpawns(Faction *f) {
    const FactionSpawn *list = f->like == 28 ? INFERNO_SPAWNS : f->like == 27 ? HADES_SPAWNS : NULL;
    int count = f->like == 28 ? (int)(sizeof INFERNO_SPAWNS / sizeof INFERNO_SPAWNS[0])
              : f->like == 27 ? (int)(sizeof HADES_SPAWNS / sizeof HADES_SPAWNS[0]) : 0;
    for (int i = 0; i < count; i++) f->spawns[i] = list[i];
    f->spawnCount = count;
}

// The declared faction of that name, or NULL.
static Faction *findFaction(const char *name) {
    for (int i = 0; i < factionCount; i++)
        if (!strcasecmp(factions[i].name, name)) return &factions[i];
    return NULL;
}

// A faction's slot in a game: PLAYER_SLOTS_GAME + its place in the list.
static int factionSlotOf(const Faction *f) { return PLAYER_SLOTS_GAME + (int)(f - factions); }

// The slot of the faction named `name` in this game, or -1.
__attribute__((unused)) static int factionSlotNamed(const char *name) {
    const Faction *f = findFaction(name);
    return f ? factionSlotOf(f) : -1;
}

// ---- Each game's faction slots -------------------------------------------------------------------
// After the game's reset, the declared factions take their slots: independent, named, coloured.
static void resetFactionTurnState(void);
static void setUpFactionSlots(void) {
    resetFactionTurnState();
#ifdef __APPLE__
    // @platform-split faction_slots_reset: Mac's reset writes its 32 controllers, homes and plane caches with stores of 64 bytes; CoE5SE does the faction slots'
    for (int f = PLAYER_SLOTS_GAME; f < PLAYER_SLOTS_MOVED; f++) {
        short none = -1;
        memcpy(playerSlotAddress(RVA_SLOT_CONTROLLER, f, 2), &none, 2);
        memcpy(playerSlotAddress(RVA_PLAYER_HOME_X, f, 2), &none, 2);
        memcpy(playerSlotAddress(RVA_PLAYER_HOME_Y, f, 2), &none, 2);
        memset(playerSlotAddress(RVA_PLAYER_PLANE_SEEN, f, PLANE_RECORDS_GAME), 0xff, PLANE_RECORDS_GAME);
    }
    if (factionSquareSeen) {
        size_t entries = (size_t)FACTION_SQUARES * FACTION_SLOT_TARGET;
        memset(factionSquareSeen, 0, entries);
        memset(factionSquareTrueSeen, 0, entries);
        memset(factionSquareSeenMark, 0xff, entries);
        memset(factionSquareAiStrength, 0xff, entries * sizeof(int));
    }
#endif
    for (int i = 0; i < factionCount; i++) {
        const Faction *f = &factions[i];
        int slot = factionSlotOf(f);
        short controller = CONTROLLER_INDEPENDENT;
        memcpy(playerSlotAddress(RVA_SLOT_CONTROLLER, slot, 2), &controller, 2);
        unsigned char *block = playerSlotAddress(RVA_SLOT_NAME, slot, PLAYER_BLOCK_SIZE);
        memset(block, 0, FACTION_NAME_SIZE);
        memcpy(block, f->name, strlen(f->name));
        float colour[3];
        if (f->hasColour) memcpy(colour, f->colour, sizeof colour);
        else memcpy(colour, playerSlotAddress(RVA_PLAYER_COLOURS, f->like, sizeof colour), sizeof colour);
        memcpy(playerSlotAddress(RVA_PLAYER_COLOURS, slot, sizeof colour), colour, sizeof colour);
        memcpy(block + (RVA_SLOT_COLOUR - RVA_SLOT_NAME), colour, sizeof colour);
    }
    writeLog("faction_slots_set_up", "factions=%d", factionCount);
}

typedef void (*ResetGameStateFn)(void);
static ResetGameStateFn gameResetGameState;

static void resetGameStateHook(void) {
    gameResetGameState();
    setUpFactionSlots();
}

// Hooks the game's reset, once; 1 when it is in.
static int factionResetHookState;
static int installFactionResetHook(void) {
    if (!factionResetHookState) {
        void *target = gameBase + RVA_RESET_GAME_STATE;
        factionResetHookState = fanpatchHookCreate(target, (void *)resetGameStateHook, (void **)&gameResetGameState) && fanpatchHookEnable(target, 1) ? 1 : -1;
        if (factionResetHookState < 0) writeLog("faction_reset_hook_not_installed", "reason=entry_hook");
    }
    return factionResetHookState > 0;
}

// Declares a faction for this pass; NULL, or the reason it was refused.
static const char *declareFaction(const char *mod, const Faction *draft) {
    const char *name = draft->name;
    if (!name[0] || strlen(name) >= FACTION_NAME_SIZE) return "bad_name";
    if (factionPresetNamed(name) >= 0) return "name_taken";
    Faction *again = findFaction(name);
    if (again && (again->fromPass != FACTION_EARLIER_PASS || strcmp(again->mod, mod))) return "name_taken";
    if (!again && factionCount >= FACTION_SLOT_TARGET) return "too_many_factions";
    if (!ensureFactionSlots()) return "faction_slots_not_ready";
    if (!installFactionResetHook() || !installSaveMagicHooks()) return "hooks_not_installed";
    Faction *f = again ? again : &factions[factionCount++];
    *f = *draft;
    snprintf(f->mod, sizeof f->mod, "%s", mod);
    if (!f->hasHome) snprintf(f->home, sizeof f->home, "%s", factionPresetHome(f->like));
    if (!f->hasActive) f->active = factionPresetActive(f->like);
    if (!f->hasPopulate) f->populate = factionPresetPopulate(f->like);
    if (!f->hasWanderAs) f->wanderAs = factionPresetWander(f->like);
    if (!f->hasWanderGate) f->wanderGate = f->like == 24 || f->like == 28 || f->like == 29;
    if (!f->hasTithe) f->tithe = f->like != 29;
    if (!f->hasWorldEvents) f->worldEvents = f->like == 24;
    if (!f->hasConquest) f->conquestIndependent = f->like == 24;
    if (!f->hasStatue) f->statue = f->like == 30;
    if (!f->hasSpawnSite) f->spawnSite = f->like == 28 ? FACTION_SITE_OWN_CITADEL : f->like == 27 ? FACTION_SITE_MONUMENT : FACTION_SITE_NONE;
    if (!f->hasFuel) snprintf(f->fuel, sizeof f->fuel, "%s", f->like == 28 ? "Sinner" : f->like == 27 ? "Dispossessed Spirit" : "");
    if (!f->hasSpawns) factionPresetSpawns(f);
    if (!f->hasMonument) snprintf(f->monument, sizeof f->monument, "%s", f->like == 27 ? "Monument of Hades" : "");
    if (!f->hasMonumentRule) f->monumentRule = f->like == 27;
    if (!f->hasEvent) factionPresetMonumentEvent(f);
    if (!f->hasShift) {
        snprintf(f->shiftMonster, sizeof f->shiftMonster, "%s", f->like == 27 ? "Dispossessed Spirit" : "");
        f->shiftPercent = 8, f->shiftEach = 50;
    }
    if (!f->hasLoyalty) snprintf(f->loyaltyMonster, sizeof f->loyaltyMonster, "%s", f->like == 27 ? "Dispossessed Spirit" : "");
    if (!f->hasGather) f->gatherAbility = f->like == 28 ? ABILITY_SINNER_CARRIER : 0;
    if (!f->hasCaravan) factionCaravanDefaults(f);
    if (!f->hasHorrors) f->horrors = f->like == 26;
    if (!f->hasAvoid) {
        f->avoidCount = f->like == 24;
        if (f->avoidCount) snprintf(f->avoidName[0], sizeof f->avoidName[0], "Kingdom"), f->avoidPercent[0] = 25;
    }
    if (!f->hasGateway) {
        int inferno = f->like == 28;
        f->gatewayFuel = inferno ? 99 : 0, f->gatewayRuin = TERRAIN_DESTROYED_GATEWAY, f->gatewayGate = TERRAIN_INFERNO_GATEWAY;
        f->gatewayExit = TERRAIN_INFERNO_GATEWAY_EXIT;
        snprintf(f->gatewayNews, sizeof f->gatewayNews, "%s", inferno ? INFERNO_GATEWAY_NEWS : "");
    }
    f->fromPass = 1;
    writeLog("faction_declared", "mod=%s name=%s slot=%d like=%s colour=%d home=%s active=%d populate=%d", mod, name, factionSlotOf(f),
             FACTION_PRESET_NAMES[f->like - 24], f->hasColour, f->home[0] ? f->home : "-", f->active, f->populate);
    return NULL;
}

// ---- The newfaction block ------------------------------------------------------------------------
static struct {
    int open, refused, mod, line;
    unsigned int fileId;
    Faction draft;
} factionBlock;

// The block words, which a newfaction block takes; any other word ends it.
static const char *const FACTION_BLOCK_WORDS[] = {"factionlike", "factioncolor", "factionhome", "factionactive", "factionpopulate", "factionwander",
                                                   "factionwandergate", "factiontithe", "factionworldevents", "factionconquest", "factionstatue",
                                                   "factionspawnsite", "factionfuel", "factionspawn", "factionmonument",
                                                   "factionmonumentrule", "factionmonumentevent", "factionplaneshift", "factionloyalty",
                                                   "factiongather", "factiongateway", "factioncaravan",
                                                   "factionhorrors", "factionavoid", "factionunits", "factionpopulateterrain"};

static int isFactionBlockWord(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof FACTION_BLOCK_WORDS / sizeof FACTION_BLOCK_WORDS[0]; i++)
        if (spellWordIs(line, len, FACTION_BLOCK_WORDS[i])) return 1;
    return 0;
}

static const char *addFactionMod, *addFactionFieldNow;

// Refuses a block word's line: logged with its line number, or as the coe5.addFaction field it came from.
static PRINTF_FORMAT(4, 5) void refuseFactionLine(const char *word, int mod, const char *reason, const char *fmt, ...) {
    char details[300];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    if (mod == FACTION_MOD_SCRIPT && addFactionMod) {
        factionBlock.refused = 1;
        refuseLuaCall(addFactionMod, "coe5.addFaction", reason, "field=%s %s", addFactionFieldNow, details);
        return;
    }
    refuseC5mLine(word, spellModName(mod), reason, "line=%d %s", G32(RVA_MOD_LINE), details);
}

// Whether a block word's line applies: level 2, inside an open block that wasn't refused.
static int factionBlockTakes(const char *word, int mod, int level) {
    if (level != 2) return 0;
    if (!factionBlock.open || factionBlock.mod != mod) {
        refuseFactionLine(word, mod, "no_faction_block", "faction=-");
        return 0;
    }
    return !factionBlock.refused;
}

// Whether a draft's own factionpopulate on names a game plane's population, which stays the game's faction's.
static int factionPopulatesGamePlane(const Faction *d) {
    return d->hasPopulate && d->populate && vanillaPlaneNamed(d->hasHome ? d->home : factionPresetHome(d->like)) >= 0;
}

// Ends the open block: declares its faction, or refuses the block.
static void finishFactionBlock(void) {
    if (!factionBlock.open) return;
    factionBlock.open = 0;
    if (factionBlock.refused) return;
    const char *mod = spellModName(factionBlock.mod);
    if (factionPopulatesGamePlane(&factionBlock.draft)) {
        refuseC5mLine("factionpopulate", mod, "game_plane", "line=%d faction=%s home=%s", factionBlock.line, factionBlock.draft.name,
                      factionBlock.draft.hasHome ? factionBlock.draft.home : factionPresetHome(factionBlock.draft.like));
        factionBlock.draft.populate = 0;
    }
    const char *reason = declareFaction(mod, &factionBlock.draft);
    if (reason) refuseC5mLine(NEWFACTION_WORD, mod, reason, "line=%d name=%s", factionBlock.line, factionBlock.draft.name);
}

// newfaction "<name>": opens a faction's block, and ends the game's blocks and the one before.
static void newfactionLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    finishPlaneBlock();
    finishFactionBlock();
    clearParserCurrentRecords();
    memset(&factionBlock, 0, sizeof factionBlock);
    factionBlock.open = 1, factionBlock.mod = mod, factionBlock.line = G32(RVA_MOD_LINE), factionBlock.fileId = modFileNow();
    factionBlock.draft.like = FACTION_LIKE_DEFAULT;
    const char *p = args;
    char name[FACTION_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = mod < 0 || mod > G32(RVA_MOD_LAST_INDEX) ? "not_in_a_mod"
                       : len <= 0                                 ? "no_name"
                       : len >= FACTION_NAME_SIZE                 ? "name_too_long"
                       : NULL;
    if (reason) {
        factionBlock.refused = 1;
        refuseFactionLine(NEWFACTION_WORD, mod, reason, "text=%.60s", args);
        return;
    }
    snprintf(factionBlock.draft.name, sizeof factionBlock.draft.name, "%.*s", FACTION_NAME_SIZE - 1, name);
}

static void factionlikeLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionlike", mod, level)) return;
    const char *p = args;
    char name[FACTION_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    int like = len > 0 && len < FACTION_NAME_SIZE ? factionPresetNamed(name) : -1;
    if (like < 0) {
        refuseFactionLine("factionlike", mod, len <= 0 ? "no_name" : "unknown_preset", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.like = like;
}

static void factioncolorLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factioncolor", mod, level)) return;
    const char *p = args;
    long long c[3];
    for (int i = 0; i < 3; i++)
        if (!readSpellNumber(&p, &c[i]) || c[i] < 0 || c[i] > 255) {
            refuseFactionLine("factioncolor", mod, "bad_colour", "faction=%s text=%.40s", factionBlock.draft.name, args);
            return;
        }
    for (int i = 0; i < 3; i++) factionBlock.draft.colour[i] = (float)c[i] / 255.0f;
    factionBlock.draft.hasColour = 1;
}

// factionhome "<plane>": the faction's home plane.
static void factionhomeLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionhome", mod, level)) return;
    const char *p = args;
    char name[PLANE_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    if (len <= 0 || len >= PLANE_NAME_SIZE) {
        refuseFactionLine("factionhome", mod, len <= 0 ? "no_name" : "name_too_long", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    snprintf(factionBlock.draft.home, sizeof factionBlock.draft.home, "%.*s", (int)(sizeof factionBlock.draft.home) - 1, name);
    factionBlock.draft.hasHome = 1;
}

// factionactive portals <n> | always | never: when the faction's turn spawns.
static void factionactiveLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionactive", mod, level)) return;
    const char *p = args;
    while (*p == ' ' || *p == '\t') p++;
    long long n = 0;
    int active;
    size_t word = strcspn(p, " \t\r\n");
    if (word == 6 && !strncasecmp(p, "always", 6)) active = FACTION_ACTIVE_ALWAYS;
    else if (word == 5 && !strncasecmp(p, "never", 5)) active = FACTION_ACTIVE_NEVER;
    else if (word == 7 && !strncasecmp(p, "portals", 7) && (p += 7, readSpellNumber(&p, &n)) && n >= 1 && n <= 1000) active = (int)n;
    else {
        refuseFactionLine("factionactive", mod, "bad_value", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.active = active, factionBlock.draft.hasActive = 1;
}

// The index of the bare word `args` starts with among `words` (case ignored), or -1.
static int factionWordChoice(const char *args, const char *const *words, int count) {
    const char *p = args;
    while (*p == ' ' || *p == '\t') p++;
    size_t length = strcspn(p, " \t\r\n");
    for (int i = 0; i < count; i++)
        if (strlen(words[i]) == length && !strncasecmp(p, words[i], length)) return i;
    return -1;
}

// A block word taking off or on: the value, or -1 refused (logged).
static int factionOnOffLine(const char *word, const char *args, int mod, int level) {
    static const char *const OFF_ON[] = {"off", "on"};
    if (!factionBlockTakes(word, mod, level)) return -1;
    int on = factionWordChoice(args, OFF_ON, 2);
    if (on < 0) refuseFactionLine(word, mod, "bad_value", "faction=%s text=%.40s", factionBlock.draft.name, args);
    return on;
}

// factionpopulate on|off: whether its home's population is its own.
static void factionpopulateLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionpopulate", args, mod, level);
    if (on >= 0) factionBlock.draft.populate = on, factionBlock.draft.hasPopulate = 1;
}

// factionwander "<word>": the wander word its commanders take, by the game's names.
static void factionwanderLine(const char *args, int mod, int level) {
    static const char *const WORDS[] = {"wander", "indwander", "infwander", "hadeswander", "celwander"};
    static const int SLOTS[] = {-1, 24, 28, 27, 29};
    if (!factionBlockTakes("factionwander", mod, level)) return;
    const char *p = args;
    char word[16];
    int len = readSpellQuoted(&p, word, sizeof word), i = len > 0 ? factionWordChoice(word, WORDS, 5) : -1;
    if (i < 0) {
        refuseFactionLine("factionwander", mod, len <= 0 ? "no_name" : "unknown_word", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.wanderAs = SLOTS[i], factionBlock.draft.hasWanderAs = 1;
}

static void factionwandergateLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionwandergate", args, mod, level);
    if (on >= 0) factionBlock.draft.wanderGate = on, factionBlock.draft.hasWanderGate = 1;
}

static void factiontitheLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factiontithe", args, mod, level);
    if (on >= 0) factionBlock.draft.tithe = on, factionBlock.draft.hasTithe = 1;
}

static void factionworldeventsLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionworldevents", args, mod, level);
    if (on >= 0) factionBlock.draft.worldEvents = on, factionBlock.draft.hasWorldEvents = 1;
}

// factionconquest independent|normal: whether its squares are conquered as the Independents' are.
static void factionconquestLine(const char *args, int mod, int level) {
    static const char *const WORDS[] = {"normal", "independent"};
    if (!factionBlockTakes("factionconquest", mod, level)) return;
    int i = factionWordChoice(args, WORDS, 2);
    if (i < 0) {
        refuseFactionLine("factionconquest", mod, "bad_value", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.conquestIndependent = i, factionBlock.draft.hasConquest = 1;
}

static void factionstatueLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionstatue", args, mod, level);
    if (on >= 0) factionBlock.draft.statue = on, factionBlock.draft.hasStatue = 1;
}

// factionspawnsite owncitadel | monument ["<plane>"] | none: where the faction's turn spawns.
static void factionspawnsiteLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionspawnsite", mod, level)) return;
    static const char *const SITES[] = {"none", "owncitadel", "monument"};
    int site = factionWordChoice(args, SITES, 3);
    char plane[PLANE_NAME_SIZE + 2] = "";
    if (site == FACTION_SITE_MONUMENT) {
        const char *p = args;
        while (*p == ' ' || *p == '\t') p++;
        p += strlen("monument");
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"') {
            int len = readSpellQuoted(&p, plane, sizeof plane);
            if (len <= 0 || len >= PLANE_NAME_SIZE) site = -1;
        }
    }
    if (site < 0) {
        refuseFactionLine("factionspawnsite", mod, "bad_value", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.spawnSite = site, factionBlock.draft.hasSpawnSite = 1;
    snprintf(factionBlock.draft.spawnPlane, sizeof factionBlock.draft.spawnPlane, "%s", plane);
}

// factionfuel "<monster>": the units the faction's spawns use up.
static void factionfuelLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionfuel", mod, level)) return;
    const char *p = args;
    char name[FACTION_FUEL_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    if (len <= 0 || len >= FACTION_FUEL_SIZE) {
        refuseFactionLine("factionfuel", mod, len <= 0 ? "no_name" : "name_too_long", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    snprintf(factionBlock.draft.fuel, sizeof factionBlock.draft.fuel, "%.*s", (int)(sizeof factionBlock.draft.fuel) - 1, name);
    factionBlock.draft.hasFuel = 1;
}

// factionspawn <min> <use> <chance> "<group>" [magma|ice|waste]: one of the faction's spawns, after those before it.
static void factionspawnLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionspawn", mod, level)) return;
    Faction *f = &factionBlock.draft;
    const char *p = args;
    long long min, use, chance;
    FactionSpawn spawn = {0};
    const char *reason = NULL;
    if (!readSpellNumber(&p, &min) || !readSpellNumber(&p, &use) || !readSpellNumber(&p, &chance)) reason = "bad_number";
    else if (min < 0 || min > 10000 || use < 0 || use > min || chance < 1 || chance > 100) reason = "bad_number";
    else {
        int len = readSpellQuoted(&p, spawn.group, sizeof spawn.group);
        if (len <= 0) reason = "no_group";
        else if (len >= FACTION_GROUP_SIZE) reason = "group_too_long";
    }
    if (!reason) {
        static const char *const TERRAINS[] = {"magma", "ice", "waste"};
        while (*p == ' ' || *p == '\t') p++;
        int terrain = *p && *p != '\r' && *p != '\n' && *p != '#' ? factionWordChoice(p, TERRAINS, 3) : -1;
        if (terrain >= 0) spawn.terrain = terrain + FACTION_TERRAIN_MAGMA;
        else if (*p && *p != '\r' && *p != '\n' && *p != '#') reason = "bad_terrain";
    }
    if (!reason && f->hasSpawns && f->spawnCount >= FACTION_SPAWN_MAX) reason = "too_many_spawns";
    if (reason) {
        refuseFactionLine("factionspawn", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    if (!f->hasSpawns) f->spawnCount = 0, f->hasSpawns = 1;
    spawn.min = (int)min, spawn.use = (int)use, spawn.chance = (int)chance;
    f->spawns[f->spawnCount++] = spawn;
}

// The quoted name `args` starts with, or none (the bare word): 1 read, 0 none, -1 refused (logged).
static int factionNameOrNone(const char *word, const char *args, int mod, char *out, size_t size) {
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) return out[0] = 0, 0;
    const char *p = args;
    char name[MONSTER_NAME_SIZE + 2];
    int len = readSpellQuoted(&p, name, sizeof name);
    if (len <= 0 || (size_t)len >= size) {
        refuseFactionLine(word, mod, len <= 0 ? "no_name" : "name_too_long", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return -1;
    }
    snprintf(out, size, "%.*s", (int)(size) - 1, name);
    return 1;
}

// factionmonument "<monster>" | none: the faction's monument.
static void factionmonumentLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionmonument", mod, level)) return;
    if (factionNameOrNone("factionmonument", args, mod, factionBlock.draft.monument, sizeof factionBlock.draft.monument) >= 0)
        factionBlock.draft.hasMonument = 1;
}

static void factionmonumentruleLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionmonumentrule", args, mod, level);
    if (on >= 0) factionBlock.draft.monumentRule = on, factionBlock.draft.hasMonumentRule = 1;
}

// Whether `news` is safe as the game message's format: each % starts %% or the one %s (the commander's name).
static int factionNewsFormatOk(const char *news) {
    int names = 0;
    for (const char *c = news; *c; c++) {
        if (*c != '%') continue;
        if (c[1] == '%') c++;
        else if (c[1] == 's' && !names++) c++;
        else return 0;
    }
    return 1;
}

// factionplaneshift "<monster>" <percent> <each> | none: the spirits its monument shifts off its home.
static void factionplaneshiftLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionplaneshift", mod, level)) return;
    Faction *f = &factionBlock.draft;
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) {
        f->shiftMonster[0] = 0, f->hasShift = 1;
        return;
    }
    const char *p = args;
    char name[MONSTER_NAME_SIZE + 2];
    long long percent, each;
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = len <= 0 ? "no_name" : len >= MONSTER_NAME_SIZE ? "name_too_long"
                       : !readSpellNumber(&p, &percent) || !readSpellNumber(&p, &each) || percent < 1 || percent > 100 || each < 1 || each > 100 ? "bad_number"
                       : NULL;
    if (reason) {
        refuseFactionLine("factionplaneshift", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    snprintf(f->shiftMonster, sizeof f->shiftMonster, "%.*s", (int)(sizeof f->shiftMonster) - 1, name);
    f->shiftPercent = (int)percent, f->shiftEach = (int)each, f->hasShift = 1;
}

// factionloyalty "<monster>" | none: the units on its monument plane that become its own.
static void factionloyaltyLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionloyalty", mod, level)) return;
    if (factionNameOrNone("factionloyalty", args, mod, factionBlock.draft.loyaltyMonster, sizeof factionBlock.draft.loyaltyMonster) >= 0)
        factionBlock.draft.hasLoyalty = 1;
}

// factiongather <ability> | none: the ability that makes its units carriers.
static void factiongatherLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factiongather", mod, level)) return;
    static const char *const NONE[] = {"none"};
    const char *p = args;
    long long ability;
    if (factionWordChoice(args, NONE, 1) == 0) ability = 0;
    else if (!readSpellNumber(&p, &ability) || ability < 1 || ability > 0xffff) {
        refuseFactionLine("factiongather", mod, "bad_number", "faction=%s text=%.40s", factionBlock.draft.name, args);
        return;
    }
    factionBlock.draft.gatherAbility = (int)ability, factionBlock.draft.hasGather = 1;
}

// factiongateway <fuel> <ruin> <gate> <exit> ["<news>"] | none: the gateway its carriers restore.
static void factiongatewayLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factiongateway", mod, level)) return;
    Faction *f = &factionBlock.draft;
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) {
        f->gatewayFuel = 0, f->hasGateway = 1;
        return;
    }
    const char *p = args;
    long long fuel, ruin, gate, exitTerrain;
    char news[FACTION_NEWS_SIZE + 2] = "";
    int newsLen = 0;
    const char *reason = NULL;
    if (!readSpellNumber(&p, &fuel) || !readSpellNumber(&p, &ruin) || !readSpellNumber(&p, &gate) || !readSpellNumber(&p, &exitTerrain)
        || fuel < 1 || fuel > 10000 || ruin < 0 || ruin >= TERRAIN_TABLE_MAX || gate < 0 || gate >= TERRAIN_TABLE_MAX || exitTerrain < 0 || exitTerrain >= TERRAIN_TABLE_MAX)
        reason = "bad_number";
    else {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"' && ((newsLen = readSpellQuoted(&p, news, sizeof news)) <= 0 || newsLen >= FACTION_NEWS_SIZE)) reason = "bad_news";
        else if (newsLen > 0 && (!factionNewsFormatOk(news) || strstr(news, "%s"))) reason = "bad_news";
    }
    if (reason) {
        refuseFactionLine("factiongateway", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    f->gatewayFuel = (int)fuel, f->gatewayRuin = (int)ruin, f->gatewayGate = (int)gate, f->gatewayExit = (int)exitTerrain, f->hasGateway = 1;
    snprintf(f->gatewayNews, sizeof f->gatewayNews, "%s", news);
}

// factionavoid "<faction>" <percent> | none: a faction whose armies its own step onto only at that chance.
static void factionavoidLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionavoid", mod, level)) return;
    Faction *f = &factionBlock.draft;
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) {
        f->avoidCount = 0, f->hasAvoid = 1;
        return;
    }
    const char *p = args;
    char name[FACTION_NAME_SIZE + 2];
    long long percent;
    int len = readSpellQuoted(&p, name, sizeof name);
    const char *reason = len <= 0 ? "no_name" : len >= FACTION_NAME_SIZE ? "name_too_long"
                       : !readSpellNumber(&p, &percent) || percent < 0 || percent > 100 ? "bad_number"
                       : f->hasAvoid && f->avoidCount >= FACTION_AVOID_MAX ? "too_many_avoids"
                       : NULL;
    if (reason) {
        refuseFactionLine("factionavoid", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    if (!f->hasAvoid) f->avoidCount = 0, f->hasAvoid = 1;
    snprintf(f->avoidName[f->avoidCount], sizeof f->avoidName[0], "%.*s", (int)(sizeof f->avoidName[0]) - 1, name);
    f->avoidPercent[f->avoidCount++] = (int)percent;
}

// The optional terrain number and quoted plane at `p`: 1 read (terrain -1 and plane "" for each left out), 0 refused.
static int readFactionTerrainPlane(const char *p, int *terrain, char *plane, size_t size) {
    while (*p == ' ' || *p == '\t') p++;
    long long code = -1;
    if ((*p == '-' || (*p >= '0' && *p <= '9')) && (!readSpellNumber(&p, &code) || code < TERRAIN_MATCH_CODE_MIN || code > 1999)) return 0;
    while (*p == ' ' || *p == '\t') p++;
    plane[0] = 0;
    if (*p == '"') {
        char name[PLANE_NAME_SIZE + 2];
        int len = readSpellQuoted(&p, name, sizeof name);
        if (len <= 0 || (size_t)len >= size) return 0;
        snprintf(plane, size, "%.*s", (int)(size) - 1, name);
    }
    *terrain = (int)code;
    return 1;
}

// factionunits "<monster>" <count> <chance> [terrain] ["<plane>"]: its own units on free squares, as planeunits'.
static void factionunitsLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionunits", mod, level)) return;
    Faction *f = &factionBlock.draft;
    FactionUnits u = {0};
    const char *p = args;
    long long count, chance;
    int len = readSpellQuoted(&p, u.monster, sizeof u.monster);
    const char *reason = len <= 0 ? "no_name" : len >= MONSTER_NAME_SIZE ? "name_too_long"
                       : !readSpellNumber(&p, &count) || count < 1 || count > FACTION_UNIT_COUNT_MAX ? "bad_count"
                       : !readSpellNumber(&p, &chance) || chance < 1 || chance > 100 ? "bad_chance"
                       : !readFactionTerrainPlane(p, &u.terrain, u.plane, sizeof u.plane) ? "bad_value"
                       : f->unitCount >= FACTION_UNITS_MAX ? "too_many_units"
                       : NULL;
    if (reason) {
        refuseFactionLine("factionunits", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    u.count = (int)count, u.chance = (int)chance;
    f->units[f->unitCount++] = u;
}

// factionpopulateterrain <terrain> ["<plane>"]: the Independents' stock on that terrain becomes its own.
static void factionpopulateterrainLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionpopulateterrain", mod, level)) return;
    Faction *f = &factionBlock.draft;
    FactionTake t = {0};
    const char *p = args;
    while (*p == ' ' || *p == '\t') p++;
    const char *reason = *p != '-' && (*p < '0' || *p > '9') ? "bad_terrain"
                       : !readFactionTerrainPlane(p, &t.terrain, t.plane, sizeof t.plane) || t.terrain == -1 ? "bad_terrain"
                       : f->takeCount >= FACTION_TAKES_MAX ? "too_many_terrains"
                       : NULL;
    if (reason) {
        refuseFactionLine("factionpopulateterrain", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    f->takes[f->takeCount++] = t;
}

static void factionhorrorsLine(const char *args, int mod, int level) {
    int on = factionOnOffLine("factionhorrors", args, mod, level);
    if (on >= 0) factionBlock.draft.horrors = on, factionBlock.draft.hasHorrors = 1;
}

// factioncaravan <turn> <chance> "<plane>" <terrain> ["<group>"] ["<news>"] | none: its caravan event.
static void factioncaravanLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factioncaravan", mod, level)) return;
    Faction *f = &factionBlock.draft;
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) {
        f->caravanTurn = -1, f->hasCaravan = 1;
        return;
    }
    const char *p = args;
    long long turn, chance, terrain;
    char plane[PLANE_NAME_SIZE + 2], group[FACTION_GROUP_SIZE + 2] = "", news[FACTION_NEWS_SIZE + 2] = "";
    int planeLen = 0, groupLen = 0, newsLen = 0;
    const char *reason = NULL;
    if (!readSpellNumber(&p, &turn) || !readSpellNumber(&p, &chance) || turn < 0 || turn > 100000 || chance < 1 || chance > 350) reason = "bad_number";
    else if ((planeLen = readSpellQuoted(&p, plane, sizeof plane)) <= 0 || planeLen >= PLANE_NAME_SIZE) reason = "bad_plane";
    else if (!readSpellNumber(&p, &terrain) || terrain < TERRAIN_MATCH_CODE_MIN || terrain > 1999) reason = "bad_terrain";
    else {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"' && ((groupLen = readSpellQuoted(&p, group, sizeof group)) < 0 || groupLen >= FACTION_GROUP_SIZE)) reason = "bad_group";
        while (!reason && (*p == ' ' || *p == '\t')) p++;
        if (!reason && *p == '"' && ((newsLen = readSpellQuoted(&p, news, sizeof news)) <= 0 || newsLen >= FACTION_NEWS_SIZE)) reason = "bad_news";
        else if (!reason && newsLen > 0 && !factionNewsFormatOk(news)) reason = "bad_news";
    }
    if (reason) {
        refuseFactionLine("factioncaravan", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    f->caravanTurn = (int)turn, f->caravanChance = (int)chance, f->caravanTerrain = (int)terrain, f->hasCaravan = 1;
    snprintf(f->caravanPlane, sizeof f->caravanPlane, "%.*s", (int)(sizeof f->caravanPlane) - 1, plane);
    snprintf(f->caravanGroup, sizeof f->caravanGroup, "%s", group);
    snprintf(f->caravanNews, sizeof f->caravanNews, "%s", news);
}

// factionmonumentevent <turn> <chance> "<plane>" <terrain> "<unfinished monster>" ["<news>"] | none: its founding.
static void factionmonumenteventLine(const char *args, int mod, int level) {
    if (!factionBlockTakes("factionmonumentevent", mod, level)) return;
    Faction *f = &factionBlock.draft;
    static const char *const NONE[] = {"none"};
    if (factionWordChoice(args, NONE, 1) == 0) {
        f->eventTurn = -1, f->hasEvent = 1;
        return;
    }
    const char *p = args;
    long long turn, chance, terrain;
    char plane[PLANE_NAME_SIZE + 2], unfinished[MONSTER_NAME_SIZE + 2], news[FACTION_NEWS_SIZE + 2] = "";
    const char *reason = NULL;
    int planeLen = 0, unfinishedLen = 0, newsLen = 0;
    if (!readSpellNumber(&p, &turn) || !readSpellNumber(&p, &chance) || turn < 0 || turn > 100000 || chance < 1 || chance > 1000) reason = "bad_number";
    else if ((planeLen = readSpellQuoted(&p, plane, sizeof plane)) <= 0 || planeLen >= PLANE_NAME_SIZE) reason = "bad_plane";
    else if (!readSpellNumber(&p, &terrain) || terrain < TERRAIN_MATCH_CODE_MIN || terrain > 1999) reason = "bad_terrain";
    else if ((unfinishedLen = readSpellQuoted(&p, unfinished, sizeof unfinished)) <= 0 || unfinishedLen >= MONSTER_NAME_SIZE) reason = "bad_monster";
    else {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '"' && ((newsLen = readSpellQuoted(&p, news, sizeof news)) <= 0 || newsLen >= FACTION_NEWS_SIZE)) reason = "bad_news";
        else if (newsLen > 0 && !factionNewsFormatOk(news)) reason = "bad_news";
    }
    if (reason) {
        refuseFactionLine("factionmonumentevent", mod, reason, "faction=%s text=%.60s", f->name, args);
        return;
    }
    f->eventTurn = (int)turn, f->eventChance = (int)chance, f->eventTerrain = (int)terrain, f->hasEvent = 1;
    snprintf(f->eventPlane, sizeof f->eventPlane, "%.*s", (int)(sizeof f->eventPlane) - 1, plane);
    snprintf(f->eventUnfinished, sizeof f->eventUnfinished, "%.*s", (int)(sizeof f->eventUnfinished) - 1, unfinished);
    snprintf(f->eventNews, sizeof f->eventNews, "%s", news);
}

// ---- coe5.addFaction -----------------------------------------------------------------------------

// One value of a field's table, and its kind.
typedef struct {
    const char *key;
    int place;
    char kind;
} AddFactionPart;

enum { ADD_FACTION_TEXT, ADD_FACTION_WORD, ADD_FACTION_ON_OFF, ADD_FACTION_COLOUR, ADD_FACTION_ACTIVE, ADD_FACTION_SITE, ADD_FACTION_NAME_OR_NONE,
       ADD_FACTION_GATHER, ADD_FACTION_PARTS, ADD_FACTION_LIST };

static const AddFactionPart ADD_FACTION_SPAWN[] = {{"min", 0, 'n'}, {"use", 0, 'n'}, {"chance", 0, 'n'}, {"group", 0, 'q'}, {"terrain", 0, 'W'}};
static const AddFactionPart ADD_FACTION_EVENT[] = {{"turn", 0, 'n'}, {"chance", 0, 'n'}, {"plane", 0, 'q'}, {"terrain", 0, 'n'},
                                                   {"unfinished", 0, 'q'}, {"news", 0, 'Q'}};
static const AddFactionPart ADD_FACTION_SHIFT[] = {{"monster", 0, 'q'}, {"percent", 0, 'n'}, {"each", 0, 'n'}};
static const AddFactionPart ADD_FACTION_GATEWAY[] = {{"fuel", 0, 'n'}, {"ruin", 0, 'n'}, {"gate", 0, 'n'}, {"exit", 0, 'n'}, {"news", 0, 'Q'}};
static const AddFactionPart ADD_FACTION_CARAVAN[] = {{"turn", 0, 'n'}, {"chance", 0, 'n'}, {"plane", 0, 'q'}, {"terrain", 0, 'n'},
                                                     {"group", 0, 'g'}, {"news", 0, 'Q'}};
static const AddFactionPart ADD_FACTION_AVOID[] = {{"name", 0, 'q'}, {"percent", 0, 'n'}};
static const AddFactionPart ADD_FACTION_UNITS[] = {{"monster", 0, 'q'}, {"count", 0, 'n'}, {"chance", 0, 'n'}, {"terrain", 0, 'N'}, {"plane", 0, 'Q'}};
static const AddFactionPart ADD_FACTION_TAKE[] = {{"terrain", 0, 'n'}, {"plane", 0, 'Q'}};

#define ADD_FACTION_PARTS_OF(table) table, (int)(sizeof table / sizeof table[0])

// coe5.addFaction's fields, each with its block word's handler, in the order the handlers read them.
static const struct {
    const char *field;
    void (*line)(const char *args, int mod, int level);
    int kind;
    const AddFactionPart *parts;
    int partCount;
} ADD_FACTION_FIELDS[] = {
    {"like", factionlikeLine, ADD_FACTION_TEXT, NULL, 0},
    {"color", NULL, ADD_FACTION_COLOUR, NULL, 0},
    {"home", factionhomeLine, ADD_FACTION_TEXT, NULL, 0},
    {"active", factionactiveLine, ADD_FACTION_ACTIVE, NULL, 0},
    {"populate", factionpopulateLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"wander", factionwanderLine, ADD_FACTION_TEXT, NULL, 0},
    {"wanderGate", factionwandergateLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"tithe", factiontitheLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"worldEvents", factionworldeventsLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"conquest", factionconquestLine, ADD_FACTION_WORD, NULL, 0},
    {"statue", factionstatueLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"spawnSite", factionspawnsiteLine, ADD_FACTION_SITE, NULL, 0},
    {"spawnPlane", NULL, ADD_FACTION_SITE, NULL, 0},
    {"fuel", factionfuelLine, ADD_FACTION_TEXT, NULL, 0},
    {"spawns", factionspawnLine, ADD_FACTION_LIST, ADD_FACTION_PARTS_OF(ADD_FACTION_SPAWN)},
    {"monument", factionmonumentLine, ADD_FACTION_NAME_OR_NONE, NULL, 0},
    {"monumentRule", factionmonumentruleLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"monumentEvent", factionmonumenteventLine, ADD_FACTION_PARTS, ADD_FACTION_PARTS_OF(ADD_FACTION_EVENT)},
    {"planeShift", factionplaneshiftLine, ADD_FACTION_PARTS, ADD_FACTION_PARTS_OF(ADD_FACTION_SHIFT)},
    {"loyalty", factionloyaltyLine, ADD_FACTION_NAME_OR_NONE, NULL, 0},
    {"gather", factiongatherLine, ADD_FACTION_GATHER, NULL, 0},
    {"gateway", factiongatewayLine, ADD_FACTION_PARTS, ADD_FACTION_PARTS_OF(ADD_FACTION_GATEWAY)},
    {"caravan", factioncaravanLine, ADD_FACTION_PARTS, ADD_FACTION_PARTS_OF(ADD_FACTION_CARAVAN)},
    {"horrors", factionhorrorsLine, ADD_FACTION_ON_OFF, NULL, 0},
    {"avoid", factionavoidLine, ADD_FACTION_LIST, ADD_FACTION_PARTS_OF(ADD_FACTION_AVOID)},
    {"units", factionunitsLine, ADD_FACTION_LIST, ADD_FACTION_PARTS_OF(ADD_FACTION_UNITS)},
    {"populateTerrain", factionpopulateterrainLine, ADD_FACTION_LIST, ADD_FACTION_PARTS_OF(ADD_FACTION_TAKE)},
};

#define ADD_FACTION_LINE_SIZE 1024

// Appends ` <value>` to `out`, the value at `index` read as `kind`; NULL, or the reason.
static const char *addFactionValue(lua_State *L, int index, char kind, char *out, size_t size) {
    size_t used = strlen(out);
    int n;
    if (kind == 'n' || kind == 'N') {
        int whole = 0;
        lua_Integer value = lua_type(L, index) == LUA_TNUMBER ? lua_tointegerx(L, index, &whole) : 0;
        if (!whole) return "bad_number";
        n = snprintf(out + used, size - used, " %lld", (long long)value);
    } else {
        size_t length;
        const char *text = lua_type(L, index) == LUA_TSTRING ? lua_tolstring(L, index, &length) : NULL;
        if (!text || strlen(text) != length || strpbrk(text, "\"\r\n")) return "bad_text";
        n = snprintf(out + used, size - used, kind == 'w' || kind == 'W' ? " %s" : " \"%s\"", text);
    }
    return n < 0 || (size_t)n >= size - used ? "too_long" : NULL;
}

// Appends the values of the table at `index` (an absolute index), as `parts` reads them; NULL, or the reason.
static const char *addFactionParts(lua_State *L, int index, const AddFactionPart *parts, int count, char *out, size_t size) {
    if (lua_type(L, index) != LUA_TTABLE) return "bad_field";
    const char *reason = NULL;
    int top = lua_gettop(L);
    for (int i = 0; i < count && !reason; i++) {
        if (parts[i].key) lua_getfield(L, index, parts[i].key);
        else lua_rawgeti(L, index, parts[i].place);
        if (!lua_isnil(L, -1)) reason = addFactionValue(L, -1, parts[i].kind, out, size);
        else if (parts[i].kind == 'g') {
            lua_pushliteral(L, "");
            reason = addFactionValue(L, -1, 'q', out, size);
        } else if (parts[i].kind != 'Q' && parts[i].kind != 'W' && parts[i].kind != 'N') reason = "missing_value";
        lua_settop(L, top);
    }
    return reason;
}

// Feeds the field at `index` to its word's handler as one line or more; NULL, or the reason it was refused before.
static const char *feedAddFactionField(lua_State *L, int index, int field) {
    char line[ADD_FACTION_LINE_SIZE] = "";
    int kind = ADD_FACTION_FIELDS[field].kind, type = lua_type(L, index);
    void (*handler)(const char *, int, int) = ADD_FACTION_FIELDS[field].line;
    const char *reason = NULL;
    switch (kind) {
    case ADD_FACTION_TEXT:
    case ADD_FACTION_WORD: reason = addFactionValue(L, index, kind == ADD_FACTION_TEXT ? 'q' : 'w', line, sizeof line); break;
    case ADD_FACTION_ON_OFF:
        if (type != LUA_TBOOLEAN) return "bad_field";
        snprintf(line, sizeof line, " %s", lua_toboolean(L, index) ? "on" : "off");
        break;
    case ADD_FACTION_COLOUR: {
        float colour[3];
        for (int i = 0; i < 3; i++) {
            int number = 0;
            if (type == LUA_TTABLE) lua_rawgeti(L, index, i + 1), number = lua_type(L, -1) == LUA_TNUMBER;
            lua_Number value = number ? lua_tonumber(L, -1) : -1;
            if (type == LUA_TTABLE) lua_pop(L, 1);
            if (!(value >= 0 && value <= 1)) return "bad_color";
            colour[i] = (float)value;
        }
        if (type == LUA_TTABLE && lua_rawlen(L, index) != 3) return "bad_color";
        memcpy(factionBlock.draft.colour, colour, sizeof colour);
        factionBlock.draft.hasColour = 1;
        return NULL;
    }
    case ADD_FACTION_ACTIVE:
        if (type == LUA_TNUMBER) {
            snprintf(line, sizeof line, " portals");
            reason = addFactionValue(L, index, 'n', line, sizeof line);
        } else reason = addFactionValue(L, index, 'w', line, sizeof line);
        break;
    case ADD_FACTION_SITE:
        if (!handler) {
            lua_getfield(L, 1, "spawnSite");
            int site = !lua_isnil(L, -1);
            lua_pop(L, 1);
            return site ? NULL : "bad_field";
        }
        reason = addFactionValue(L, index, 'w', line, sizeof line);
        lua_getfield(L, 1, "spawnPlane");
        if (!reason && !lua_isnil(L, -1)) reason = addFactionValue(L, lua_gettop(L), 'q', line, sizeof line);
        lua_pop(L, 1);
        break;
    case ADD_FACTION_NAME_OR_NONE:
    case ADD_FACTION_GATHER:
        if (type == LUA_TBOOLEAN && !lua_toboolean(L, index)) snprintf(line, sizeof line, " none");
        else reason = addFactionValue(L, index, kind == ADD_FACTION_GATHER ? 'n' : 'q', line, sizeof line);
        break;
    case ADD_FACTION_PARTS:
        if (type == LUA_TBOOLEAN && !lua_toboolean(L, index)) snprintf(line, sizeof line, " none");
        else reason = addFactionParts(L, index, ADD_FACTION_FIELDS[field].parts, ADD_FACTION_FIELDS[field].partCount, line, sizeof line);
        break;
    case ADD_FACTION_LIST: {
        size_t count = type == LUA_TTABLE ? lua_rawlen(L, index) : 0;
        if (type != LUA_TTABLE && !(type == LUA_TBOOLEAN && !lua_toboolean(L, index))) return "bad_field";
        if (!count) {
            if (handler == factionspawnLine) factionBlock.draft.spawnCount = 0, factionBlock.draft.hasSpawns = 1;
            else if (handler == factionavoidLine) handler(" none", FACTION_MOD_SCRIPT, 2);
            return NULL;
        }
        for (size_t i = 1; i <= count && !factionBlock.refused; i++) {
            line[0] = '\0';
            lua_rawgeti(L, index, (lua_Integer)i);
            reason = addFactionParts(L, lua_gettop(L), ADD_FACTION_FIELDS[field].parts, ADD_FACTION_FIELDS[field].partCount, line, sizeof line);
            lua_pop(L, 1);
            if (reason) return reason;
            handler(line, FACTION_MOD_SCRIPT, 2);
        }
        return NULL;
    }
    }
    if (reason) return reason;
    handler(line, FACTION_MOD_SCRIPT, 2);
    return NULL;
}

// Whether every key of the table at 1 is one of coe5.addFaction's fields; the first that isn't in `bad`.
static int addFactionKeysKnown(lua_State *L, char *bad, size_t size) {
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        lua_pop(L, 1);
        int known = 0;
        if (lua_type(L, -1) == LUA_TSTRING) {
            const char *key = lua_tostring(L, -1);
            known = !strcmp(key, "name");
            for (size_t i = 0; !known && i < sizeof ADD_FACTION_FIELDS / sizeof ADD_FACTION_FIELDS[0]; i++) known = !strcmp(key, ADD_FACTION_FIELDS[i].field);
        }
        if (!known) {
            snprintf(bad, size, "%.40s", lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "(not a name)");
            lua_pop(L, 1);
            return 0;
        }
    }
    return 1;
}

// coe5.addFaction{name =, like =, ...}: declares a faction of the mod's, at script load; true, or false when refused.
static int lAddFaction(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    const char *mod = runningLuaMod();
    lua_getfield(L, 1, "name");
    const char *name = lua_type(L, 2) == LUA_TSTRING ? lua_tostring(L, 2) : NULL;
    char bad[48] = "name";
    const char *reason = !luaScriptsLoading ? "not_at_load"
                       : !name || !name[0] ? "no_name"
                       : strlen(name) >= FACTION_NAME_SIZE ? "name_too_long"
                       : !addFactionKeysKnown(L, bad, sizeof bad) ? "unknown_field"
                       : NULL;
    if (reason) {
        refuseLuaCall(mod, "coe5.addFaction", reason, "field=%s name=%.40s", bad, name ? name : "-");
        lua_pushboolean(L, 0);
        return 1;
    }
    finishFactionBlock();
    memset(&factionBlock, 0, sizeof factionBlock);
    factionBlock.open = 1, factionBlock.mod = FACTION_MOD_SCRIPT, factionBlock.draft.like = FACTION_LIKE_DEFAULT;
    snprintf(factionBlock.draft.name, sizeof factionBlock.draft.name, "%s", name);
    addFactionMod = mod ? mod : "?";
    for (int i = 0; i < (int)(sizeof ADD_FACTION_FIELDS / sizeof ADD_FACTION_FIELDS[0]) && !factionBlock.refused; i++) {
        addFactionFieldNow = ADD_FACTION_FIELDS[i].field;
        lua_getfield(L, 1, addFactionFieldNow);
        const char *fieldReason = lua_isnil(L, 3) ? NULL : feedAddFactionField(L, 3, i);
        lua_settop(L, 2);
        if (fieldReason) {
            factionBlock.refused = 1;
            refuseLuaCall(addFactionMod, "coe5.addFaction", fieldReason, "field=%s faction=%s", addFactionFieldNow, name);
        }
    }
    if (!factionBlock.refused && factionPopulatesGamePlane(&factionBlock.draft)) {
        factionBlock.refused = 1;
        refuseLuaCall(addFactionMod, "coe5.addFaction", "game_plane", "field=populate faction=%s", name);
    }
    int refused = factionBlock.refused;
    factionBlock.open = 0, addFactionMod = NULL;
    if (!refused) {
        reason = declareFaction(mod ? mod : "?", &factionBlock.draft);
        if (reason) refuseLuaCall(mod, "coe5.addFaction", reason, "field=name name=%.40s", name);
        else findFaction(name)->fromPass = FACTION_FROM_SCRIPT;
    }
    lua_pushboolean(L, !refused && !reason);
    return 1;
}

// ---- The line hook and the passes -----------------------------------------------------------------
// Ends the open newfaction block at its file's end or another word's line; returns the line.
static const char *factionWordLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2 || !factionBlock.open) return line;
    if (factionBlock.mod != mod || factionBlock.fileId != modFileNow() || !isFactionBlockWord(line, len)) finishFactionBlock();
    return line;
}

// Before a level-2 mod pass: the pass reads every block again.
static void placeSavedFactions(void);
static void factionWordsBeforePass(int level) {
    if (level != 2) return;
    finishFactionBlock();
    for (int i = 0; i < factionCount; i++)
        if (factions[i].fromPass != FACTION_FROM_SCRIPT) factions[i].fromPass = FACTION_EARLIER_PASS;
    placeSavedFactions();
}

// After a level-2 mod pass: the last block ends, and factions no block named again go.
static void factionWordsAfterPass(int level) {
    if (level != 2) return;
    finishFactionBlock();
    int kept = 0;
    for (int i = 0; i < factionCount; i++) {
        if (factions[i].fromPass == FACTION_EARLIER_PASS) {
            writeLog("faction_dropped", "mod=%s name=%s reason=not_declared_this_pass", factions[i].mod, factions[i].name);
            continue;
        }
        if (kept != i) factions[kept] = factions[i];
        kept++;
    }
    factionCount = kept;
}

// Adds the words.
static void factionWordsInit(void) {
    int words = apiAddModCommand("newfaction", newfactionLine) + apiAddModCommand("factionlike", factionlikeLine) +
                apiAddModCommand("factioncolor", factioncolorLine) + apiAddModCommand("factionhome", factionhomeLine) +
                apiAddModCommand("factionactive", factionactiveLine) + apiAddModCommand("factionpopulate", factionpopulateLine) +
                apiAddModCommand("factionwander", factionwanderLine) + apiAddModCommand("factionwandergate", factionwandergateLine) +
                apiAddModCommand("factiontithe", factiontitheLine) + apiAddModCommand("factionworldevents", factionworldeventsLine) +
                apiAddModCommand("factionconquest", factionconquestLine) + apiAddModCommand("factionstatue", factionstatueLine) +
                apiAddModCommand("factionspawnsite", factionspawnsiteLine) + apiAddModCommand("factionfuel", factionfuelLine) +
                apiAddModCommand("factionspawn", factionspawnLine) + apiAddModCommand("factionmonument", factionmonumentLine) +
                apiAddModCommand("factionmonumentrule", factionmonumentruleLine) +
                apiAddModCommand("factionmonumentevent", factionmonumenteventLine) +
                apiAddModCommand("factionplaneshift", factionplaneshiftLine) + apiAddModCommand("factionloyalty", factionloyaltyLine) +
                apiAddModCommand("factiongather", factiongatherLine) + apiAddModCommand("factiongateway", factiongatewayLine) +
                apiAddModCommand("factioncaravan", factioncaravanLine) + apiAddModCommand("factionhorrors", factionhorrorsLine) +
                apiAddModCommand("factionavoid", factionavoidLine) + apiAddModCommand("factionunits", factionunitsLine) +
                apiAddModCommand("factionpopulateterrain", factionpopulateterrainLine);
    writeLog("faction_words_installed", "words=%d of 27", words);
}
