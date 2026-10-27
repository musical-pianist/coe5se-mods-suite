// coe5.get and coe5.set: the values they work out (move flags, incomes, garrisons, ritual rules, ceremonies), and the calls.

// ---- Move flags, flag words, ritual and spell levels, library levels ----------------------------
// A unit's move flags or flag words, into *out.
static int unitFlagsOf(int unit, int words, unsigned long long *out) {
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) < 0) return -1;
    *out = GAME_FN(unsigned long long (*)(int), words ? RVA_UNIT_RESISTANCES : RVA_UNIT_FLAGS)(unit);
    return 0;
}

// The commander's unit when it is in the table and alive, else -1.
static int livingCommanderUnit(int commander) {
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) return -1;
    return unit;
}

// The move flags of the commander's army as the AI's routes test them.
static unsigned long long commanderMoveFlags(int commander) {
    return GAME_FN(unsigned long long (*)(int), RVA_ARMY_MOVE_FLAGS)(commander);
}

// The commander's school and level, as RVA_COMMANDER_RITPOW works them out.
static int commanderRitpowOf(int commander, int *level) {
    *level = 0;
    int unit = livingCommanderUnit(commander);
    if (unit < 0 || (*(const unsigned long long *)GAME_PTR(RVA_UNITS + (size_t)unit * UNIT_SIZE + 8) >> 28) & 1) return 0;
    int last = G32(RVA_RITPOW_COUNT);
    for (int school = 501; school <= last && school < RITUAL_EFFECT_ID_COUNT; school++) {
        int total = unitAbilityTotal(unit, school, 1);
        if (total > 0) {
            *level = total;
            return school;
        }
    }
    return 0;
}

// Square (x, y)'s library level as the AI's conquest scores it.
#define LIBRARY_ENCHANT_KIND 0x484
static int squareLibraryLevel(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= G32(RVA_MAP_W)) return 0;
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    int terrain = (short)G16(RVA_SQUARE_RECORDS + square + SQUARE_TERRAIN_TYPE);
    if (terrain < 0 || terrain >= TERRAIN_TABLE_MAX) return 0;
    int owner = (signed char)G8(RVA_SQUARE_RECORDS + square + SQUARE_OWNER);
    unsigned long long flags = *(const unsigned long long *)GAME_PTR(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_FLAGS);
    int base = (int)((flags >> 34) & 1);
    int enchant = GAME_FN(int (*)(int, int, int, int), RVA_SQUARE_ENCHANT_VALUE)(owner, x, y, LIBRARY_ENCHANT_KIND);
    return enchant + ((flags >> 35) & 1 ? base + 2 : base);
}

// Whether square (x, y) has a fort, as the game's AI buy asks it for an aihold monster.
#define FORT_PART_FORT 0x200
#define FORT_PARTS_LEVEL_2 0x11800
static int squareHasFort(int x, int y) {
    if (x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    return (GAME_FN(unsigned (*)(int, int), RVA_SQUARE_FORT_PARTS)(x, y) & FORT_PART_FORT) != 0;
}

// ---- Incomes: slotIncome and terrainIncome ---------------------------------------------------------
// The field of a terrain record that holds its income of `resource`, or -1.
static int terrainIncomeField(int resource) {
    static const int FIELDS[RESOURCE_COUNT] = {TERRAIN_GOLD,  TERRAIN_IRON, TERRAIN_HERBS, TERRAIN_FUNGUS, TERRAIN_SACRIFICES,
                                               TERRAIN_HANDS, TERRAIN_WEED, -1, -1, -1, -1, TERRAIN_TRADE, -1, -1};
    return resource >= 0 && resource < RESOURCE_COUNT ? FIELDS[resource] : -1;
}

// The percent player_turn_start pays a player's income of `resource` at.
static int incomePercent(int controller, int bonus, int resource, int anySpecial) {
    if (controller == CONTROLLER_HUMAN) return 100;
    if (controller != CONTROLLER_AI) return 0;
    int quarter = bonus > 0 ? bonus / 4 : 0;
    if (resource != 0 && resource != 1 && resource != 11) return 100 + bonus + quarter;
    return 100 + bonus - (anySpecial ? quarter : 0);
}

// An income paid at `percent`, rounded as player_turn_start rounds it.
static int scaledIncome(int raw, int percent) {
    return (int)(((long long)raw * percent + 50) / 100);
}

// What player `slot` gets of `resource` at its next turn start.
static int anyCommanderGathers(int slot, unsigned int check) {
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX; c++) {
        int unit = G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE);
        int owner = unit >= 0 && unit < UNIT_TABLE_MAX ? (short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE) : -1;
        if (owner == slot && GAME_FN(int (*)(int), check)(c)) return 1;
    }
    return 0;
}
static int playerGathersSpecial(int slot, int resource) {
    switch (resource) {
    case 2: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_HERBS)(slot) != 0;
    case 3: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_FUNGUS)(slot) != 0;
    case 4: return anyCommanderGathers(slot, RVA_COMMANDER_GATHERS_SACRIFICES);
    case 5: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_HANDS)(slot) != 0;
    case 6: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_WEED)(slot) != 0;
    case 7: case 8: case 9: case 10:
        return anyCommanderGathers(slot, RVA_COMMANDER_GATHERS_GEMS) || GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_ANY_GEMS)(slot) != 0;
    case 12: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_RELICS)(slot) != 0;
    case 13: return GAME_FN(int (*)(int), RVA_PLAYER_GATHERS_LIFEFORCE)(slot) != 0;
    default: return 1;
    }
}
static int rawIncome(int slot, int resource) {
    switch (resource) {
    case 0: return GAME_FN(int (*)(int, int *, int), RVA_PLAYER_GOLD_INCOME)(slot, NULL, 0);
    case 1: return GAME_FN(int (*)(int), RVA_PLAYER_IRON_INCOME)(slot);
    case 2: return GAME_FN(int (*)(int), RVA_PLAYER_HERBS_INCOME)(slot);
    case 3: return GAME_FN(int (*)(int), RVA_PLAYER_FUNGUS_INCOME)(slot);
    case 4: return GAME_FN(int (*)(int), RVA_PLAYER_SACRIFICES_INCOME)(slot);
    case 5: return GAME_FN(int (*)(int), RVA_PLAYER_HANDS_INCOME)(slot);
    case 6: return GAME_FN(int (*)(int), RVA_PLAYER_WEED_INCOME)(slot);
    case 11: return GAME_FN(int (*)(int), RVA_PLAYER_TRADE_INCOME)(slot);
    case 12: return GAME_FN(int (*)(int), RVA_PLAYER_RELICS_INCOME)(slot);
    case 13: return GAME_FN(int (*)(int, int), RVA_PLAYER_LIFEFORCE_INCOME)(slot, 0);
    default: return GAME_FN(int (*)(int, int), RVA_PLAYER_GEM_INCOME)(resource - 7, slot);
    }
}
static int slotIncome(int slot, int resource) {
    int controller = (*(short *)playerSlotAddress(RVA_SLOT_CONTROLLER, slot, 2));
    int level = (*(short *)playerSlotAddress(RVA_SLOT_AI, slot, PLAYER_BLOCK_SIZE));
    int bonus = level >= 0 && level < AI_LEVEL_TABLE_MAX ? G32(RVA_AI_LEVEL_BONUS + (size_t)level * 4) : 0;
    int percent = incomePercent(controller, bonus, resource, 0);
    if (percent == 0 || !playerGathersSpecial(slot, resource)) return 0;
    if (controller == CONTROLLER_AI && (resource == 0 || resource == 1 || resource == 11)) {
        static const int SPECIAL_CHECKS[] = {3, 2, 6, 13, 4, 12, 5, 7};
        int anySpecial = 0;
        for (size_t i = 0; i < sizeof SPECIAL_CHECKS / sizeof SPECIAL_CHECKS[0] && !anySpecial; i++)
            anySpecial = playerGathersSpecial(slot, SPECIAL_CHECKS[i]);
        percent = incomePercent(controller, bonus, resource, anySpecial);
    }
    return scaledIncome(rawIncome(slot, resource), percent);
}

// ---- What stays home: commanderGarrisonLeft --------------------------------------------------------
// The strength that stays on a commander's square when it leaves with its army.
enum { GARRISON_GAME, GARRISON_JOINED, GARRISON_DROPPED };
static unsigned char garrisonState[UNIT_TABLE_MAX];
static unsigned long long garrisonUnitFlags[UNIT_TABLE_MAX];

// Unit u's leader after the mirrored joins and drops of commander c's gather.
static int garrisonLeader(int c, int u) {
    if (garrisonState[u] == GARRISON_JOINED) return c;
    if (garrisonState[u] == GARRISON_DROPPED) return -1;
    return G32(RVA_UNIT_LEADER + (size_t)u * UNIT_SIZE);
}
static unsigned long long garrisonUnitFlagsOf(int u) { return GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(u); }

// The game's small unit and square reads the gather makes, as the game works them out.
static int garrisonUnitStrength(int u) {
    if (u < 0 || u >= UNIT_TABLE_MAX) return 0;
    return GAME_FN(int (*)(int, int, int), RVA_UNIT_STRENGTH_CORE)(u, -1, (short)G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE));
}
static int garrisonUnitX(int u) { return u < 0 || u >= UNIT_TABLE_MAX ? -1 : G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE); }
static int garrisonUnitY(int u) { return u < 0 || u >= UNIT_TABLE_MAX ? -1 : G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE); }
static int garrisonUnitHp(int u) { return u < 0 || u >= UNIT_TABLE_MAX ? 0 : (short)G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE); }
static int garrisonUnitOwner(int u) {
    if (u < 0 || u >= UNIT_TABLE_MAX || u > G32(RVA_UNIT_COUNT)) return -1;
    return (short)G16(RVA_UNIT_PLAYER + (size_t)u * UNIT_SIZE);
}
static int garrisonCommanderOf(int u) {
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX; c++)
        if (G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) == u) return c;
    return -1;
}
static int garrisonSameTeam(int a, int b) { return playerSameTeam(a, b); }
static int garrisonSquareTerrain(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_W) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return -1;
    return (short)G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE);
}
static int garrisonFortLevel(int x, int y) {
    if (!squareHasFort(x, y)) return 0;
    return (GAME_FN(unsigned (*)(int, int), RVA_SQUARE_FORT_PARTS)(x, y) & FORT_PARTS_LEVEL_2) ? 2 : 1;
}

// RVA_COMMANDER_STRENGTH (flags 0) or RVA_COMMANDER_FLAGGED_STRENGTH, on the mirrored leaders.
static int garrisonArmyStrength(int c, int last, unsigned long long flags) {
    int sum = 0;
    for (int u = 0; u <= last; u++)
        if (garrisonLeader(c, u) == c && (garrisonUnitFlagsOf(u) & flags) == flags) sum += garrisonUnitStrength(u);
    int unit = c <= G32(RVA_LAST_COMMANDER) ? G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) : -1;
    return sum + garrisonUnitStrength(unit);
}

// RVA_AI_REFORMS_HERE with what it changes put back: the random sequence and each unit's int64 at +8.
static int garrisonReformsHere(int player, int x, int y, int last) {
    int randomIndex = G32(RVA_RANDOM_INDEX), randomShift = G32(RVA_RANDOM_SHIFT), randomXor = G32(RVA_RANDOM_XOR);
    for (int u = 0; u <= last; u++) garrisonUnitFlags[u] = *(unsigned long long *)(gameBase + RVA_UNITS + (size_t)u * UNIT_SIZE + 8);
    int here = GAME_FN(int (*)(int, int, int), RVA_AI_REFORMS_HERE)(player, x, y);
    for (int u = 0; u <= last; u++) *(unsigned long long *)(gameBase + RVA_UNITS + (size_t)u * UNIT_SIZE + 8) = garrisonUnitFlags[u];
    G32(RVA_RANDOM_INDEX) = randomIndex, G32(RVA_RANDOM_SHIFT) = randomShift, G32(RVA_RANDOM_XOR) = randomXor;
    return here;
}

// Whether unit u may join in the gather's pick-up passes.
static int garrisonMayJoin(int c, int u, int player, int x, int y, int kind) {
    if (garrisonUnitX(u) != x || garrisonUnitY(u) != y) return 0;
    if (garrisonUnitHp(u) <= 0 || garrisonLeader(c, u) != -1) return 0;
    if (garrisonUnitOwner(u) != player || garrisonCommanderOf(u) != -1) return 0;
    return GAME_FN(int (*)(int, int), RVA_AI_UNIT_JOIN_KIND)(c, u) >= kind;
}

// Drops every follower of c for which `drop` holds.
static void garrisonDropFollowers(int c, int last, int (*drop)(int u)) {
    for (int u = 0; u <= last; u++)
        if (garrisonLeader(c, u) == c && drop(u)) garrisonState[u] = GARRISON_DROPPED;
}
static int garrisonHolds(int u) { return unitAbilityTotal(u, 0x34, 0) == 1; }
static int garrisonLacks10(int u) { return !(garrisonUnitFlagsOf(u) & 0x10); }
static int garrisonLacks2(int u) { return !(garrisonUnitFlagsOf(u) & 2); }
static int garrisonHas8(int u) { return (garrisonUnitFlagsOf(u) & 8) != 0; }

// RVA_AI_GATHER_TROOPS for commander c, on the mirrored leaders.
static void garrisonGather(int c, int unit, int player, int x, int y, int last) {
    if ((garrisonUnitFlagsOf(unit) >> MONSTER_FLAG_NO_COMMANDER) & 1) return;
    if ((GAME_FN(unsigned long long (*)(int), RVA_UNIT_RESISTANCES)(unit) >> UNIT_RESISTANCE_NOLEADER) & 1) return;
    if (*(unsigned long long *)(gameBase + RVA_UNITS + (size_t)unit * UNIT_SIZE) & UNIT_AI_SAILOR) return;
    int terrain = garrisonSquareTerrain(x, y);
    if (terrain < 0 || terrain >= TERRAIN_TABLE_MAX) return;
    int gold = G16(RVA_TERRAIN_TABLE + (size_t)terrain * TERRAIN_SIZE + TERRAIN_GOLD);
    int citadel = GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y) != 0;
    int keep = 0;
    if (citadel) keep = GAME_FN(int (*)(int), RVA_AI_COUNT_CITADELS)(player) <= 1 ? 60 : 20;
    if (gold >= 4 && keep < 10) keep = 10;
    if (garrisonReformsHere(player, x, y, last) && keep < 50) keep = 50;
    int own = garrisonUnitStrength(unit) + GAME_FN(int (*)(int), RVA_COMMANDER_CASTER_BONUS)(c);
    int savedCount = G32(RVA_FREE_TROOPS_COUNT);
    int free = GAME_FN(int (*)(int, int), RVA_FREE_TROOPS_STRENGTH)(x, y);
    int count = G32(RVA_FREE_TROOPS_COUNT);
    G32(RVA_FREE_TROOPS_COUNT) = savedCount;
    int army = garrisonArmyStrength(c, last, 0);
    if (free > keep && count > 1) {
        int take = (int)((unsigned)(100 - keep) * (unsigned)free) / 100;
        if (take > 0) {
            for (int u = 0; u <= last; u++)
                if (garrisonMayJoin(c, u, player, x, y, 2)) garrisonState[u] = GARRISON_JOINED, take -= garrisonUnitStrength(u), count--;
            for (int r = 0; r <= 3 && r <= last; r++)
                for (int u = r; u <= last && take > 0 && (count > 1 || keep == 0) && count > 0; u += 4)
                    if (garrisonMayJoin(c, u, player, x, y, 1)) garrisonState[u] = GARRISON_JOINED, take -= garrisonUnitStrength(u), count--;
        }
    } else if (count == 0 && army > own && keep) {
        int t = (int)((unsigned)army * (unsigned)keep) / 100;
        for (int start = 0; start <= 1; start++)
            for (int u = start; u <= last && t > own; u += 2) {
                if (garrisonLeader(c, u) != c || garrisonUnitHp(u) <= 0) continue;
                int strength = garrisonUnitStrength(u);
                if (t - strength < own || !GAME_FN(int (*)(int), RVA_AI_UNIT_MAY_GUARD)(u)) continue;
                garrisonState[u] = GARRISON_DROPPED;
                t -= strength;
            }
    }
    if (squareHasFort(x, y)) garrisonDropFollowers(c, last, garrisonHolds);
    if (gold <= 1 && !citadel) return;
    unsigned long long f = garrisonUnitFlagsOf(unit);
    if (f & 0x10) {
        army = garrisonArmyStrength(c, last, 0);
        if (army - garrisonArmyStrength(c, last, 0x10) < army / 2) garrisonDropFollowers(c, last, garrisonLacks10);
    }
    if (f & 2) {
        army = garrisonArmyStrength(c, last, 0);
        if (army - garrisonArmyStrength(c, last, 2) < army / 4) garrisonDropFollowers(c, last, garrisonLacks2);
    }
    if (!(f & 8)) {
        army = garrisonArmyStrength(c, last, 0);
        if (garrisonArmyStrength(c, last, 8) < army / 10) garrisonDropFollowers(c, last, garrisonHas8);
    }
}

// Whether unit u leaves with commander c.
static int garrisonLeaves(int c, int cUnit, int u) {
    if (u == cUnit) return 1;
    int leader = garrisonLeader(c, u);
    for (int depth = 0; depth < 8 && leader >= 0 && leader < COMMANDER_TABLE_MAX; depth++) {
        if (leader == c) return 1;
        int leaderUnit = G32(RVA_COMMANDERS + (size_t)leader * COMMANDER_SIZE);
        if (leaderUnit < 0 || leaderUnit >= UNIT_TABLE_MAX) return 0;
        leader = garrisonLeader(c, leaderUnit);
    }
    return 0;
}

// The strength that stays on commander c's square when c leaves, or -1.
static int commanderGarrisonLeft(int c) {
    int unit = G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) return -1;
    int player = garrisonUnitOwner(unit);
    int x = garrisonUnitX(unit), y = garrisonUnitY(unit);
    if (player < 0 || player >= playerSlotCount() || x < 0 || y < 0) return -1;
    int last = G32(RVA_UNIT_COUNT);
    if (last >= UNIT_TABLE_MAX) last = UNIT_TABLE_MAX - 1;
    memset(garrisonState, GARRISON_GAME, sizeof garrisonState);
    garrisonGather(c, unit, player, x, y, last);
    long long sum = 0;
    int count = 0, ranged = 0;
    for (int u = 0; u <= last; u++) {
        if (!unitAliveAt(u, x, y)) continue;
        if (!garrisonSameTeam(player, garrisonUnitOwner(u))) continue;
        if (garrisonLeaves(c, unit, u)) continue;
        sum += unitStrengthFor(player, u);
        count++;
        ranged += GAME_FN(int (*)(int, int, int), RVA_UNIT_HAS_RANGED)(u, 3, 1) != 0;
    }
    if (ranged > count / 10) sum = sum * (100 + 60 * garrisonFortLevel(x, y)) / 100;
    return sum < INT_MAX ? (int)sum : INT_MAX;
}

// A player's remembered strength of square (x, y).
static int rememberedStrength(const int *memo, int x, int y, int (*measure)(int, int, int)) {
    if (x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    return *memo >= 0 ? *memo : measure(-1, x, y);
}

// Where a ritual's rules are judged.
typedef struct {
    const unsigned char *record;
    int commander, unit, player, x, y;
} RitualCaster;

// Effect i's value as the game's effect lookups read it: the int64 cut to an int.
static int ritualFxValue(const unsigned char *record, int i) {
    return (int)*(const long long *)(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE + RITUAL_FX_VALUE);
}
static int ritualFxId(const unsigned char *record, int i) { return *(const int *)(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE); }

// The value of the record's first effect `id`, or `none` without one.
static int ritualFirstFx(const unsigned char *record, int id, int none) {
    for (int i = 0; i < RITUAL_FX_MAX && ritualFxId(record, i) >= 1; i++)
        if (ritualFxId(record, i) == id) return ritualFxValue(record, i);
    return none;
}

// One RITUAL_STRINGS line as the game reads it.
static void ritualStringDecode(const char *line, char *name, size_t size, int *kind) {
    char text[RITUAL_STRING_SIZE + 1];
    size_t len = strnlen(line, RITUAL_STRING_SIZE);
    memcpy(text, line, len);
    text[len] = 0;
    *kind = 0;
    if (text[0] == '|') return;
    if (text[0] == '(') {
        static const char MARKS[] = "-+!&|";
        const char *mark = text[1] ? strchr(MARKS, text[1]) : NULL;
        if (mark && text[2] == ')') *kind = (int)(mark - MARKS) + 1;
        char *close = strchr(text, ')');
        if (close) memmove(text, close + 1, strlen(close + 1) + 1);
    }
    char *amp = strstr(text, " & ");
    if (amp) *amp = 0;
    const char *star = strchr(text, '*');
    snprintf(name, size, "%.*s", (int)(size - 1), star ? star + 1 : text);
}

// How many living units named `name` have their home at (x, y).
static int monstersHomeAt(int x, int y, const char *name) {
    int count = 0, last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        int monster = G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE);
        if (G16(RVA_UNIT_HOME_X + (size_t)u * UNIT_SIZE) != x || G16(RVA_UNIT_HOME_X + 2 + (size_t)u * UNIT_SIZE) != y) continue;
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) < 1 || !monsterRecord(monster)) continue;
        count += !strncmp((const char *)monsterRecord(monster), name, MONSTER_NAME_SIZE);
    }
    return count;
}

// How the unit rules count a line's monster.
enum { COUNT_HERE, COUNT_HOME_HERE, COUNT_WORLD, COUNT_PLAYER, COUNT_ELYSIUM };
static int ritualMonsterCount(const RitualCaster *c, int how, const char *name) {
    int (*at)(int, int, const char *) = GAME_FN(int (*)(int, int, const char *), RVA_COUNT_MONSTERS_AT);
    int (*plane)(int, const char *) = GAME_FN(int (*)(int, const char *), RVA_COUNT_MONSTERS_ON_PLANE);
    switch (how) {
    case COUNT_HERE: return at(c->x, c->y, name);
    case COUNT_HOME_HERE: return monstersHomeAt(c->x, c->y, name);
    case COUNT_WORLD: return at(-1, -1, name);
    case COUNT_PLAYER: return GAME_FN(int (*)(int, const char *), RVA_COUNT_PLAYER_MONSTERS)(c->player, name);
    default: return plane(0, name) + plane(2, name) + plane(3, name);
    }
}

// The unit rules.
static int ritualUnitRuleMet(const RitualCaster *c, int id, int value) {
    const char *strings = (const char *)c->record + RITUAL_STRINGS;
    char name[100] = "";
    int how, count = 0;
    int casterMonster = G16(RVA_UNIT_MONSTER + (size_t)c->unit * UNIT_SIZE);
    const char *casterName = (const char *)monsterRecord(casterMonster);
    switch (id) {
    case RITUAL_FX_NOMONREQ: case RITUAL_FX_FEWMONREQ: {
        int limit = id == RITUAL_FX_NOMONREQ ? value - 1 : value;
        if (id == RITUAL_FX_FEWMONREQ && ritualFirstFx(c->record, RITUAL_FX_NOMONREQ, 0) != 0) return 1;
        if (value == 0 || limit < 0) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind != 1 && kind != 2) continue;
            count += ritualMonsterCount(c, COUNT_HERE, name);
            if (count > limit) return kind == 2;
        }
        return 1;
    }
    case RITUAL_FX_MINMONREQ: case RITUAL_FX_MONWORLDREQ: case RITUAL_FX_MONPLAYERREQ:
        how = id == RITUAL_FX_MINMONREQ ? COUNT_HERE : id == RITUAL_FX_MONWORLDREQ ? COUNT_WORLD : COUNT_PLAYER;
        if (value < 1) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind == 4 && (count += ritualMonsterCount(c, how, name)) >= value) return 1;
        }
        return 0;
    case RITUAL_FX_NOMONMASTERYREQ:
        if (value < 1 || unitAbilityTotal(c->unit, *(const int *)(c->record + RITUAL_RITPOW), 0) > *(const int *)(c->record + RITUAL_LEVEL))
            return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (kind == 5 && casterName && !GAME_FN(int (*)(const char *, const char *), RVA_NAME_COMPARE)(casterName, name)) return 1;
            if ((kind == 1 || kind == 2) && ritualMonsterCount(c, COUNT_PLAYER, name) > 0) return kind == 2;
        }
        return 1;
    default:
        how = id == RITUAL_FX_NOMONHOMEREQ ? COUNT_HOME_HERE : id == RITUAL_FX_NOMONWORLDREQ ? COUNT_WORLD
            : id == RITUAL_FX_NOMONPLAYERREQ ? COUNT_PLAYER : COUNT_ELYSIUM;
        if (value < 1) return 1;
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            int kind;
            ritualStringDecode(strings + (size_t)i * RITUAL_STRING_SIZE, name, sizeof name, &kind);
            if (how == COUNT_ELYSIUM) count += ritualMonsterCount(c, how, name);
            if (kind == 5 && casterName && !GAME_FN(int (*)(const char *, const char *), RVA_NAME_COMPARE)(casterName, name)) return 1;
            if (kind != 1 && kind != 2) continue;
            if (how != COUNT_ELYSIUM) count += ritualMonsterCount(c, how, name);
            if (count >= value) return kind == 2;
        }
        return 1;
    }
}

// Whether a unit on (x, y) has an ability total of 1..999999.
static int unitWithAbilityAt(int x, int y, int ability) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    if (!G16(RVA_SQUARE_ARMY + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * 2)) return 0;
    int last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++) {
        if (G32(RVA_UNIT_X + (size_t)u * UNIT_SIZE) != x || G32(RVA_UNIT_X + 4 + (size_t)u * UNIT_SIZE) != y) continue;
        if (G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) < 1) continue;
        int total = unitAbilityTotal(u, ability, 1);
        if (total >= 1 && total <= 999999) return 1;
    }
    return 0;
}

// The square's SQUARE_PORTAL, 0 off the world.
static int portalAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    return G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_PORTAL);
}

// The caster's and its square's rules.
static int ritualCasterRuleMet(const RitualCaster *c, int id, int v) {
    switch (id) {
    case RITUAL_FX_LEVELREQ: {
        if (v < 1) return 1;
        if (unitAbilityTotal(c->unit, *(const int *)(c->record + RITUAL_RITPOW), 0) != v) return 0;
        int last = G32(RVA_RITPOW_COUNT);
        for (int a = 501; a <= last && a < RITUAL_EFFECT_ID_COUNT; a++)
            if (unitAbilityTotal(c->unit, a, 0) > v) return 0;
        return 1;
    }
    case RITUAL_FX_RESRESTRICT: case RITUAL_FX_RESREQ: case RITUAL_FX_BENRESTRICT: {
        if (v == 0) return 1;
        unsigned long long bits = GAME_FN(unsigned long long (*)(int), id == RITUAL_FX_BENRESTRICT ? RVA_UNIT_BENEFITS : RVA_UNIT_RESISTANCES)(c->unit);
        unsigned long long want = (unsigned long long)(long long)v;
        return id == RITUAL_FX_RESREQ ? (want & ~bits) == 0 : (bits & want) == 0;
    }
    case RITUAL_FX_FXREQ:
        return v == 0 || GAME_FN(int (*)(int, int, int), RVA_COMMANDER_ABILITY_TOTAL)(c->commander, v, 1) > 0;
    case RITUAL_FX_NOPLANEREQ: return v == 999 || GAME_FN(int (*)(int, int), RVA_PLANE_AT)(c->x, c->y) != v;
    case RITUAL_FX_SEASONREQ: return v < 0 || G16(RVA_SEASON) == v;
    case RITUAL_FX_EVENTVARREQ: return v < 1 || v >= EVENT_VAR_COUNT || G32(RVA_EVENT_VARS + (size_t)v * 4) >= 1;
    case RITUAL_FX_NOEVENTVARREQ: return v < 1 || v >= EVENT_VAR_COUNT || G32(RVA_EVENT_VARS + (size_t)v * 4) <= 0;
    case RITUAL_FX_HASPORTALREQ: return v == 0 || GAME_FN(int (*)(int, int, int), RVA_COUNT_PLAYER_PORTALS)(c->player, -1, 2) > 0;
    case RITUAL_FX_NOPORTALREQ: return v < 1 || portalAt(c->x, c->y) <= 0;
    case RITUAL_FX_ENCHHEREREQ: {
        if (v < 1) return 1;
        int onMap = c->x >= 0 && c->y >= 0 && c->x < MAP_COLUMNS_MAX && c->y < MAP_COLUMN_SQUARES;
        int terrain = onMap ? G16(RVA_SQUARE_RECORDS + ((size_t)c->x * MAP_COLUMN_SQUARES + (size_t)c->y) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE) : -1;
        return (v == 19 && terrain == 282) || GAME_FN(int (*)(int, int, int, int), RVA_ENCHANT_COUNT)(c->player, v, c->x, c->y) > 0;
    }
    case RITUAL_FX_NOFXHEREREQ: return v < 1 || !unitWithAbilityAt(c->x, c->y, v);
    case RITUAL_FX_FORTREQ: case RITUAL_FX_NOFORTREQ: {
        if (v == 0) return 1;
        unsigned parts = GAME_FN(unsigned (*)(int, int), RVA_SQUARE_FORT_PARTS)(c->x, c->y);
        return id == RITUAL_FX_FORTREQ ? (parts & (unsigned)v) == (unsigned)v : (parts & (unsigned)v) == 0;
    }
    case RITUAL_FX_HOMEREQ: {
        int home = G16(RVA_UNIT_HOME_X + (size_t)c->unit * UNIT_SIZE) == c->x && G16(RVA_UNIT_HOME_X + 2 + (size_t)c->unit * UNIT_SIZE) == c->y;
        return v == 0 || (v > 0 ? home : !home);
    }
    case RITUAL_FX_HOMECITREQ: {
        if (v == 0 || c->player < 0 || c->player >= playerSlotCount()) return 1;
        int hx = (*(short *)playerSlotAddress(RVA_PLAYER_HOME_X, c->player, 2)), hy = (*(short *)playerSlotAddress(RVA_PLAYER_HOME_Y, c->player, 2));
        return v < 0 ? !(c->x == hx && c->y == hy) : c->x == hx && c->y != hy;
    }
    case RITUAL_FX_NOMONREQ: case RITUAL_FX_FEWMONREQ: case RITUAL_FX_MINMONREQ: case RITUAL_FX_MONWORLDREQ:
    case RITUAL_FX_MONPLAYERREQ: case RITUAL_FX_NOMONMASTERYREQ: case RITUAL_FX_NOMONHOMEREQ:
    case RITUAL_FX_NOMONWORLDREQ: case RITUAL_FX_NOMONPLAYERREQ: case RITUAL_FX_NOMONELYSIUMREQ:
        return ritualUnitRuleMet(c, id, v);
    case RITUAL_FX_PROMOTION: {
        if (v == 0) return 1;
        const char *strings = (const char *)c->record + RITUAL_STRINGS;
        int (*named)(int, int, int, const char *) = GAME_FN(int (*)(int, int, int, const char *), RVA_COUNT_UNITS_NAMED);
        char line[RITUAL_STRING_SIZE + 1];
        for (int i = 0; i < RITUAL_STRINGS_MAX && strings[(size_t)i * RITUAL_STRING_SIZE]; i++) {
            const char *at = strings + (size_t)i * RITUAL_STRING_SIZE;
            if (!strncmp(at, "(-)", 3)) continue;
            snprintf(line, sizeof line, "%.*s", RITUAL_STRING_SIZE, at);
            if (named(c->player, c->x, c->y, line) > 0) return 1;
            i++;
        }
        return 0;
    }
    default: return -1;
    }
}

// Whether the rule of effect i of a ritual record holds for commander `commander`.
static int ritualEffectMetAt(const unsigned char *record, int i, int commander) {
    static const int RADII[] = {1, 3, 5, 7, 99};
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return -1;
    RitualCaster c = {record, commander, unit, G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE),
                      G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE)};
    if (c.player < 0) return -1;
    int id = ritualFxId(record, i);
    for (int j = 0; j < i; j++)
        if (ritualFxId(record, j) == id) return 1;
    int code, rule = ritualNearbyRule(record + RITUAL_FX + (size_t)i * RITUAL_FX_SIZE, &code);
    if (rule < 0) return ritualCasterRuleMet(&c, id, ritualFxValue(record, i));
    if (c.x < 0 || c.x >= MAP_COLUMNS_MAX || c.y < 0 || c.y >= MAP_COLUMN_SQUARES) return -1;
    int count = GAME_FN(int (*)(int, int, int, int), RVA_TERRAIN_NEAR_COUNT)(c.x, c.y, code, RADII[rule % 5]);
    return rule < 5 ? count == 0 : count > 0;
}

// Fort part words, read once from the game's terrain reader.
static const char *fortPartWords[FORT_PART_BITS];
static int fortPartWordsRead;

// The bit number of a value with one bit set, or -1.
static int singleBitIndex(uint32_t value) {
    if (!value || (value & (value - 1))) return -1;
    int n = 0;
    while (!(value & 1)) value >>= 1, n++;
    return n;
}

// Windows: reads the fort part words from the terrain reader's code; returns how many.
static __attribute__((unused)) int readFortWordsX64(const unsigned char *code, size_t size, uintptr_t at, uintptr_t match, const char **words) {
    int found = 0;
    for (size_t i = 20; i + 5 <= size; i++) {
        if (code[i] != 0xe8 || code[i - 20] != 0xba || memcmp(code + i - 15, "\x48\x8d\x0d", 3) || code[i - 8] != 0x41 || code[i - 7] != 0xb8)
            continue;
        int32_t rel, disp;
        uint32_t bit;
        memcpy(&rel, code + i + 1, 4);
        if (at + i + 5 + (intptr_t)rel != match) continue;
        memcpy(&bit, code + i - 19, 4);
        memcpy(&disp, code + i - 12, 4);
        int n = singleBitIndex(bit);
        if (n < 0) continue;
        words[n] = (const char *)(at + i - 8 + (intptr_t)disp);
        found++;
    }
    return found;
}

// Mac: each `adrp x0, page / add x0, x0, #low / mov w1, #bit / bl match` in code[0..size), at `at`.
static __attribute__((unused)) int readFortWordsArm64(const unsigned char *code, size_t size, uintptr_t at, uintptr_t match, const char **words) {
    int found = 0;
    for (size_t i = 12; i + 4 <= size; i += 4) {
        uint32_t adrp, add, mov, bl;
        memcpy(&adrp, code + i - 12, 4);
        memcpy(&add, code + i - 8, 4);
        memcpy(&mov, code + i - 4, 4);
        memcpy(&bl, code + i, 4);
        if ((bl & 0xfc000000) != 0x94000000 || (adrp & 0x9f00001f) != 0x90000000 || (add & 0xffc003ff) != 0x91000000 ||
            (mov & 0xff80001f) != 0x52800001) continue;
        int32_t offset = (int32_t)(bl << 6) >> 4;
        if (at + i + (intptr_t)offset != match) continue;
        int64_t pageImm = (int64_t)(((uint64_t)((adrp >> 5) & 0x7ffff) << 2 | ((adrp >> 29) & 3)) << 43) >> 31;
        uintptr_t page = ((at + i - 12) & ~(uintptr_t)0xfff) + (intptr_t)pageImm;
        int n = singleBitIndex(((mov >> 5) & 0xffff) << (16 * ((mov >> 21) & 3)));
        if (n < 0) continue;
        words[n] = (const char *)(page + ((add >> 10) & 0xfff));
        found++;
    }
    return found;
}

// The terrain word for fort part bit `bit`, or NULL for a bit no word sets.
static const char *fortPartWord(int bit) {
    if (!fortPartWordsRead) {
        fortPartWordsRead = 1;
        const unsigned char *code = GAME_PTR(RVA_FORT_WORD_CALLS);
#ifdef __APPLE__
        int found = readFortWordsArm64(code, FORT_WORD_CALLS_SIZE, (uintptr_t)code, (uintptr_t)GAME_PTR(RVA_FORT_WORD_MATCH), fortPartWords);
#else
        int found = readFortWordsX64(code, FORT_WORD_CALLS_SIZE, (uintptr_t)code, (uintptr_t)GAME_PTR(RVA_FORT_WORD_MATCH), fortPartWords);
#endif
        writeLog("fort_part_words_read", "found=%d", found);
    }
    return bit >= 0 && bit < FORT_PART_BITS ? fortPartWords[bit] : NULL;
}

// The game's own name for Voice of El ceremony `ceremony`.
static unsigned int ceremonyNameRva(int ceremony) {
    switch (ceremony) {
    case CEREMONY_CONVERT: return RVA_STR_CEREMONY_CONVERT;
    case CEREMONY_BREAK_SEAL: return RVA_STR_CEREMONY_BREAK_SEAL;
    case CEREMONY_BOLSTER_DEVOTION: return RVA_STR_CEREMONY_BOLSTER;
    case CEREMONY_START_CRUSADE: return RVA_STR_CEREMONY_CRUSADE;
    case CEREMONY_CALL_INQUISITION: return RVA_STR_CEREMONY_INQUISITION;
    case CEREMONY_MASTERY_ARCH_BISHOP:
    case CEREMONY_MASTERY_CARDINAL:
    case CEREMONY_MASTERY_PONTIFF: return RVA_STR_CEREMONY_MASTERY;
    case CEREMONY_INCREASE_FAVOR: return RVA_STR_CEREMONY_FAVOR;
    default: return 0;
    }
}

// Where ceremony `ceremony`'s cost is, as an offset from the row's address.
#define CEREMONY_COST_ENTRY(name) (int)RVA_CEREMONY_COST_##name
#ifdef __APPLE__
#define CEREMONY_MASTERY_ENTRY(name) (int)RVA_CEREMONY_COST_##name##_SET
#else
#define CEREMONY_MASTERY_ENTRY(name) (int)RVA_CEREMONY_COST_##name
#endif
static int ceremonyCostAt(int ceremony, int *also) {
    *also = -1;
    switch (ceremony) {
    case CEREMONY_CONVERT: return CEREMONY_COST_ENTRY(CONVERT);
    case CEREMONY_BREAK_SEAL: *also = CEREMONY_COST_ENTRY(SEAL_BASE); return CEREMONY_COST_ENTRY(SEAL_STEP);
    case CEREMONY_BOLSTER_DEVOTION: *also = CEREMONY_COST_ENTRY(BOLSTER_BASE); return CEREMONY_COST_ENTRY(BOLSTER_STEP);
    case CEREMONY_START_CRUSADE: return CEREMONY_COST_ENTRY(CRUSADE);
    case CEREMONY_CALL_INQUISITION: return CEREMONY_COST_ENTRY(INQUISITION);
    case CEREMONY_MASTERY_ARCH_BISHOP: return CEREMONY_MASTERY_ENTRY(ARCH_BISHOP);
    case CEREMONY_MASTERY_CARDINAL: return CEREMONY_MASTERY_ENTRY(CARDINAL);
    case CEREMONY_MASTERY_PONTIFF: return CEREMONY_MASTERY_ENTRY(PONTIFF);
    case CEREMONY_INCREASE_FAVOR: return CEREMONY_COST_ENTRY(FAVOR_STEP);
    default: return -1;
    }
}
// The most a script may set a ceremony's cost to.
#define CEREMONY_COST_MAX 100000

// coe5.set("ceremonyCost", ceremony, value).
static int setCeremonyCost(lua_State *L, unsigned char *table, int arg) {
    lua_Integer ceremony = lua_tointeger(L, 2), value = luaL_checkinteger(L, arg);
    int also, at = ceremonyCostAt((int)ceremony, &also);
    LuaScript *s = luaScriptOf(L);
    if (at < 0) {
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "no_cost", "name=ceremonyCost ceremony=%lld", (long long)ceremony);
        return 0;
    }
    if (value < 0 || value > CEREMONY_COST_MAX) {
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "out_of_range", "name=ceremonyCost ceremony=%lld value=%lld min=0 max=%d",
                      (long long)ceremony, (long long)value, CEREMONY_COST_MAX);
        return 0;
    }
    *(int *)(table + at) = (int)value;
    if (also >= 0) *(int *)(table + also) = (int)value;
    return 0;
}

// ---- Content records by their .c5m words: coe5.get("monster", m, "hp") -----------------------------
// The record_words.h row of `word` in a block of kind `record`, or NULL.
static const RecordWord *recordWordRow(int record, const char *word) {
    for (size_t i = 0; i < sizeof RECORD_WORDS / sizeof RECORD_WORDS[0]; i++)
        if (RECORD_WORDS[i].record == record && !strcmp(RECORD_WORDS[i].word, word)) return &RECORD_WORDS[i];
    return NULL;
}

// The value of ability `id` among a record's (ability, value) pairs: the first match, 0 for none.
static long long abilityPairValue(const unsigned char *pairs, int count, int width, int id) {
    for (int i = 0; i < count; i++) {
        const unsigned char *pair = pairs + (size_t)i * 2 * width;
        long long ability = width == 8 ? *(const long long *)pair : *(const int *)pair;
        if (!ability) break;
        if (ability == id) return width == 8 ? *(const long long *)(pair + 8) : *(const int *)(pair + 4);
    }
    return 0;
}

// Pushes record `record`'s value of .c5m word `word` of its block (kind `kind`).
static void pushRecordWord(lua_State *L, const char *name, int kind, const unsigned char *record, const char *word) {
    const RecordWord *row = recordWordRow(kind, word);
    long long ability = row && row->kind == WORD_ABILITY ? (long long)row->value : -1;
    if (!row && (kind == RECORD_MONSTER || kind == RECORD_ITEM || kind == RECORD_RITUAL)) ability = abilityIdOfWord(word);
    if (!row && ability < 0) { luaL_error(L, "%s: unknown word '%s'", name, word); return; }
    if (row && row->kind == WORD_FIELD) {
        const unsigned char *at = record + row->offset;
        lua_pushinteger(L, row->size == 1 ? *at : row->size == 2 ? *(const short *)at : row->size == 4 ? *(const int *)at
                                                                                                    : *(const long long *)at);
        return;
    }
    if (row && row->kind == WORD_FLAG) {
        unsigned long long bits = row->size == 4 ? *(const unsigned int *)(record + row->offset) : *(const unsigned long long *)(record + row->offset);
        lua_pushboolean(L, (bits & row->value) == row->value);
        return;
    }
    switch (kind) {
    case RECORD_MONSTER: lua_pushinteger(L, abilityPairValue(record + MONSTER_ABILITIES, MONSTER_ABILITY_PAIRS, 4, (int)ability)); break;
    case RECORD_ITEM: lua_pushinteger(L, abilityPairValue(record + ITEM_ABILITIES, ITEM_ABILITY_PAIRS, 4, (int)ability)); break;
    case RECORD_CLASS: lua_pushinteger(L, abilityPairValue(record + CLASS_ABILITIES, CLASS_ABILITY_PAIRS, 4, (int)ability)); break;
    case RECORD_TERRAIN: lua_pushinteger(L, abilityPairValue(record + TERRAIN_ABILITIES, TERRAIN_ABILITY_PAIRS, 8, (int)ability)); break;
    default: lua_pushinteger(L, ritualFirstFx(record, (int)ability, 0));
    }
}

// coe5.get(name[, index]).
static int lGet(lua_State *L) {
    const GameVar *v;
    int arg = 2;
    unsigned char *p = gameVarAddress(L, &v, &arg);
    if (!p) { lua_pushnil(L); return 1; }
    switch (v->type) {
    case T_ARMY_OWNER: lua_pushinteger(L, armyOwnerAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_RITUAL_SUMMONS: {
        int summons = 0;
        for (int i = 0; i < RITUAL_FX_MAX; i++) summons |= *(const int *)(p + (size_t)i * RITUAL_FX_SIZE) == RITUAL_FX_SUMMONING;
        lua_pushboolean(L, summons);
        break;
    }
    case T_I64: lua_pushinteger(L, (lua_Integer)*(const long long *)p); break;
    case T_TICKS_MS: lua_pushinteger(L, GAME_FN(int (*)(void), RVA_TICKS_MS)()); break;
    case T_EFFECT_NAME: {
        const char *word = ritualEffectWord((int)lua_tointeger(L, 2));
        if (word) lua_pushstring(L, word);
        else lua_pushnil(L);
        break;
    }
    case T_RITUAL_TERRAIN_NAME: pushGameTerrainName(L, *(const int *)p); break;
    case T_RITUAL_TERRAIN_AT: {
        int at = ritualTerrainAt(*(const int *)p, (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4));
        if (at < 0) lua_pushnil(L);
        else lua_pushboolean(L, at);
        break;
    }
    case T_RITUAL_EFFECT_MET: {
        int i = (int)lua_tointeger(L, 3);
        int met = ritualEffectMetAt(p - RITUAL_FX - (size_t)i * RITUAL_FX_SIZE, i, (int)lua_tointeger(L, 4));
        if (met < 0) lua_pushnil(L);
        else lua_pushboolean(L, met);
        break;
    }
    case T_RITUAL_EFFECT_TERRAIN_NAME: {
        int code;
        if (ritualNearbyRule(p, &code) < 0) lua_pushnil(L);
        else pushGameTerrainName(L, code);
        break;
    }
    case T_CAST_COUNT: lua_pushinteger(L, *(const short *)p); break;
    case T_UNIT_AP_LEFT: lua_pushinteger(L, unitApLeft((int)lua_tointeger(L, 2))); break;
    case T_UNIT_STATIONARY: lua_pushboolean(L, unitStationary((int)lua_tointeger(L, 2))); break;
    case T_COMMANDER_COST: {
        int have;
        if (commanderCostAvailable((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), &have) < 0) lua_pushnil(L);
        else lua_pushinteger(L, have);
        break;
    }
    case T_SLOT_INCOME: lua_pushinteger(L, slotIncome((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_TERRAIN_INCOME: {
        int field = terrainIncomeField((int)lua_tointeger(L, 3));
        if (field < 0) lua_pushnil(L);
        else lua_pushinteger(L, *(const short *)(p + field));
        break;
    }
    case T_UNIT_ABILITY: lua_pushinteger(L, unitAbilityTotal((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), 0)); break;
    case T_MONSTER_ABILITY: lua_pushinteger(L, monsterAbility((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_CLASS_STDTROOPS:
        lua_pushboolean(L, GAME_FN(int (*)(int, int, int), RVA_CLASS_ABILITY)((int)lua_tointeger(L, 2), CLASS_ABILITY_STDTROOPS, 0) > 0);
        break;
    case T_CLASS_ABILITY:
        lua_pushinteger(L, GAME_FN(int (*)(int, int, int), RVA_CLASS_ABILITY)((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), 0));
        break;
    case T_U8: lua_pushinteger(L, *p); break;
    case T_I8: lua_pushinteger(L, *(const signed char *)p); break;
    case T_PLANE_AT:
        lua_pushinteger(L, GAME_FN(int (*)(int, int), RVA_PLANE_AT)((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)));
        break;
    case T_COMMANDER_STRENGTH: {
        int unit = *(const int *)p;
        if (unit < 0 || unit >= UNIT_TABLE_MAX || G16(RVA_UNIT_HP + (size_t)unit * UNIT_SIZE) <= 0) lua_pushnil(L);
        else lua_pushinteger(L, commanderStrength((int)lua_tointeger(L, 2)));
        break;
    }
    case T_SQUARE_STRENGTH:
        lua_pushinteger(L, squareStrength((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4)));
        break;
    case T_GARRISON_LEFT: {
        int left = commanderGarrisonLeft((int)lua_tointeger(L, 2));
        if (left < 0) lua_pushnil(L);
        else lua_pushinteger(L, left);
        break;
    }
    case T_ARMY_STATIONARY: lua_pushboolean(L, armyStationaryAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)) != 0); break;
    case T_ARMY_HOLDS: lua_pushboolean(L, armyHoldsAt((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3)) != 0); break;
    case T_UNIT_STRENGTH: lua_pushinteger(L, unitStrengthFor((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_UNIT_VISIBLE: lua_pushboolean(L, unitVisibleToNow((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_UNIT_ARMOR: {
        int armor = unitArmorNow((int)lua_tointeger(L, 2));
        if (armor < 0) lua_pushnil(L);
        else lua_pushinteger(L, armor);
        break;
    }
    case T_MONSTER_ATTACK: {
        int row = (int)lua_tointeger(L, 3), field = (int)(v->addr - (RVA_MONSTER_TABLE + MONSTER_ATTACKS)), value;
        if (monsterAttackField(p - field - (size_t)row * MONSTER_ATTACK_SIZE, row, field, &value) < 0) lua_pushnil(L);
        else lua_pushinteger(L, value);
        break;
    }
    case T_SQUARE_CITADEL:
        lua_pushboolean(L, squareCitadelFor((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4)) != 0);
        break;
    case T_REMEMBERED_STRENGTH:
        lua_pushinteger(L, rememberedStrength((const int *)p, (int)lua_tointeger(L, 3), (int)lua_tointeger(L, 4), squareStrength));
        break;
    case T_WINDOWS_ONLY: lua_pushnil(L); break;
    case T_GAME_CHANGED: lua_pushnil(L); break;
    case T_CEREMONY_NAME: {
        unsigned int rva = ceremonyNameRva((int)lua_tointeger(L, 2));
        if (!rva) lua_pushnil(L);
        else lua_pushlstring(L, (const char *)GAME_PTR(rva), strnlen((const char *)GAME_PTR(rva), NAME_SIZE));
        break;
    }
    case T_CEREMONY_COST: {
        int also, at = ceremonyCostAt((int)lua_tointeger(L, 2), &also);
        if (at < 0) lua_pushnil(L);
        else lua_pushinteger(L, *(const int *)(p + at));
        break;
    }
    case T_CEREMONY_LAST: lua_pushinteger(L, CEREMONY_LAST); break;
    case T_FORT_PART_WORD: {
        const char *word = fortPartWord((int)lua_tointeger(L, 2));
        if (!word) lua_pushnil(L);
        else lua_pushstring(L, word);
        break;
    }
    case T_UNIT_MOVE_FLAGS:
    case T_UNIT_FLAG_WORDS: {
        unsigned long long flags;
        if (unitFlagsOf((int)lua_tointeger(L, 2), v->type == T_UNIT_FLAG_WORDS, &flags) < 0) lua_pushnil(L);
        else lua_pushinteger(L, (lua_Integer)flags);
        break;
    }
    case T_COMMANDER_MOVE_FLAGS:
    case T_COMMANDER_RITPOW:
    case T_COMMANDER_RITUAL_LEVEL:
    case T_COMMANDER_SPELL_LEVEL: {
        int commander = (int)lua_tointeger(L, 2), unit = livingCommanderUnit(commander), level;
        if (unit < 0) lua_pushnil(L);
        else if (v->type == T_COMMANDER_MOVE_FLAGS) lua_pushinteger(L, (lua_Integer)commanderMoveFlags(commander));
        else if (v->type == T_COMMANDER_SPELL_LEVEL) lua_pushinteger(L, GAME_FN(int (*)(int, int), RVA_UNIT_PATH_LEVEL)(unit, -1));
        else {
            int school = commanderRitpowOf(commander, &level);
            lua_pushinteger(L, v->type == T_COMMANDER_RITPOW ? school : level);
        }
        break;
    }
    case T_SQUARE_LIBRARY_LEVEL: lua_pushinteger(L, squareLibraryLevel((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_SQUARE_HAS_FORT: lua_pushboolean(L, squareHasFort((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3))); break;
    case T_RECORD_WORD: pushRecordWord(L, v->name, v->size, p, luaL_checkstring(L, arg)); break;
    case T_AI_ATTACK_MARGIN: lua_pushinteger(L, AI_ATTACK_MARGIN); break;
    case T_SLOT_ABILITY:
        lua_pushinteger(L, GAME_FN(int (*)(int, int, int), RVA_PLAYER_ABILITY)((int)lua_tointeger(L, 2), (int)lua_tointeger(L, 3), v->size));
        break;
    case T_UNIT_STATE_FLAGS: {
        unsigned long long flags = 0;
        if (!unitStateFlagsNow((int)lua_tointeger(L, 2), &flags)) lua_pushnil(L);
        else lua_pushinteger(L, (lua_Integer)flags);
        break;
    }
    case T_UNIT_RETURNS: {
        int returns = unitReturnsNow((int)lua_tointeger(L, 2));
        if (returns < 0) lua_pushnil(L);
        else lua_pushboolean(L, returns);
        break;
    }
    case T_I16: lua_pushinteger(L, *(short *)p); break;
    case T_FLAG_U8: lua_pushboolean(L, *p != 0); break;
    case T_FLAG_POSITIVE: lua_pushboolean(L, *(int *)p > 0); break;
    case T_FLAG_I32: lua_pushboolean(L, *(int *)p != 0); break;
    case T_FLAG_I16: lua_pushboolean(L, *(short *)p != 0); break;
    case T_FLAG_TWO: lua_pushboolean(L, *(int *)p == 2); break;
    case T_F32: lua_pushnumber(L, *(float *)p); break;
    case T_STR: lua_pushlstring(L, (const char *)p, strnlen((const char *)p, v->size)); break;
    case T_PTR_STR: {
        const char *s = *(const char *const *)p;
        if (s) lua_pushlstring(L, s, strnlen(s, v->size));
        else lua_pushnil(L);
        break;
    }
    case T_PLANE_TINT: {
        float rgba[PLANE_TINT_CHANNELS];
        memcpy(rgba, p + PLANE_TINT, sizeof rgba);
        for (int c = 0; c < PLANE_TINT_CHANNELS; c++) lua_pushnumber(L, c == PLANE_TINT_CHANNELS - 1 && !p[PLANE_TINT_ON] ? 0.0 : rgba[c]);
        return PLANE_TINT_CHANNELS;
    }
    default: lua_pushinteger(L, *(int *)p);
    }
    return 1;
}

// The values coe5.set takes for a variable the game indexes a fixed table by.
static const struct { const char *name; lua_Integer min, max; } GAME_VAR_RANGES[] = {
    {"mapW", INT_MIN, MAP_COLUMNS_MAX}, {"mapH", INT_MIN, MAP_COLUMN_SQUARES}, {"mapDepth", INT_MIN, MAP_COLUMNS_MAX},
    {"cliMapW", INT_MIN, MAP_COLUMNS_MAX}, {"cliMapH", INT_MIN, MAP_COLUMN_SQUARES},
    {"society", 0, SOCIETY_TABLE_MAX - 1},
    {"slotClass", INT_MIN, LAST_MOD_CLASS_SLOT},
    {"slotAi", INT_MIN, AI_LEVEL_TABLE_MAX - 1},
    {"startDefault", 0, 2}, {"northPart", -1, 100}, {"southPart", -1, 100}, {"wilder", 0, 2},
    {"mouseButtons", 0, 7},
    {"traceLevel", 0, 3},
    {"slotTradeType", 0, 24},
    {"squareCorpses", 0, SHRT_MAX},
};

// A slot's colour range.
#define SLOT_COLOUR_MIN -8421505.0f
#define SLOT_COLOUR_MAX 8421504.0f

// Returns 1 for a float variable the game keeps within the slot colour's range.
static int isSlotColour(const GameVar *v) {
    return v->type == T_F32 && v->addr >= RVA_SLOT_COLOUR && v->addr <= RVA_SLOT_COLOUR + 8;
}

// The values coe5.set takes for integer variable v.
static void gameVarRange(const GameVar *v, lua_Integer *min, lua_Integer *max) {
    if (v->type == T_U8) *min = SCHAR_MIN, *max = UCHAR_MAX;
    else if (v->type == T_I16) *min = SHRT_MIN, *max = USHRT_MAX;
    else *min = INT_MIN, *max = UINT_MAX;
    for (size_t i = 0; i < sizeof GAME_VAR_RANGES / sizeof GAME_VAR_RANGES[0]; i++) {
        if (strcmp(GAME_VAR_RANGES[i].name, v->name)) continue;
        if (GAME_VAR_RANGES[i].min > *min) *min = GAME_VAR_RANGES[i].min;
        if (GAME_VAR_RANGES[i].max < *max) *max = GAME_VAR_RANGES[i].max;
    }
    if (!strcmp(v->name, "slotClass") && tableRecords(TABLE_CLASS) - 1 < *max) *max = tableRecords(TABLE_CLASS) - 1;
}

// Resets a commander's cached army action points.
static void flushCommanderCache(int commander) {
    if (commander < 0 || commander >= COMMANDER_TABLE_MAX) return;
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit >= 0 && unit < UNIT_TABLE_MAX) {
        int leader = G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER);
        if (leader >= 0 && leader < COMMANDER_TABLE_MAX) {
            *(signed char *)GAME_PTR(RVA_COMMANDERS + (size_t)leader * COMMANDER_SIZE + COMMANDER_AP_CACHE) = -99;
            G32(RVA_COMMANDER_CACHE_INTS + (size_t)leader * 4) = -1;
        }
    }
    *(signed char *)GAME_PTR(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE + COMMANDER_AP_CACHE) = -99;
    G32(RVA_COMMANDER_CACHE_INTS + (size_t)commander * 4) = -1;
}

// After a write of unitApUsed: resets the commander caches the game resets.
static void flushArmyApCaches(int unit) {
    int last = G32(RVA_LAST_COMMANDER);
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX; c++) {
        if (G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) == unit) {
            flushCommanderCache(c);
            break;
        }
    }
    flushCommanderCache(G32(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_COMMANDER));
}

// World building: the squares a script may write.
typedef struct { int x, y, w, h; } SquareRect;
static int worldBuildingDepth;
static SquareRect worldBuildingArea;

// Marks world building in progress on `area`; returns the area it replaces.
static __attribute__((unused)) SquareRect startWorldBuilding(SquareRect area) {
    SquareRect previous = worldBuildingArea;
    worldBuildingArea = area;
    worldBuildingDepth++;
    return previous;
}

// Ends the startWorldBuilding call that returned `previous`.
static __attribute__((unused)) void stopWorldBuilding(SquareRect previous) {
    worldBuildingArea = previous;
    if (worldBuildingDepth > 0) worldBuildingDepth--;
}

// 1 and the area being built while the world is built, else 0.
static int worldBuildingAreaNow(SquareRect *area) {
    if (worldBuildingDepth > 0) {
        *area = worldBuildingArea;
        return 1;
    }
    if (!luaSiteDepth[SITE_PLAY_GAME_NEW_WORLD]) return 0;
    int w = G32(RVA_MAP_W), h = G32(RVA_MAP_H);
    *area = (SquareRect){0, 0, w < MAP_COLUMNS_MAX ? w : MAP_COLUMNS_MAX, h < MAP_COLUMN_SQUARES ? h : MAP_COLUMN_SQUARES};
    return 1;
}

// Why a write to square (x, y) is refused, or NULL.
static const char *squareWriteRefusal(int building, SquareRect area, lua_Integer x, lua_Integer y) {
    if (!building) return "not_world_building";
    if (x < area.x || y < area.y || x >= (lua_Integer)area.x + area.w || y >= (lua_Integer)area.y + area.h) return "outside_plane";
    return NULL;
}

// The values a square write takes: what the game itself stores there.
static void squareValueRange(const char *name, int lastTerrain, lua_Integer *min, lua_Integer *max) {
    *min = SHRT_MIN, *max = SHRT_MAX;
    if (!strcmp(name, "squareTerrain")) *min = 0, *max = lastTerrain;
    else if (!strcmp(name, "squareGround")) *min = 0, *max = SQUARE_GROUND_LAST;
}

// coe5.set for a square value, only inside the area being built.
static int setSquareValue(lua_State *L, const GameVar *v, unsigned char *p, int arg) {
    LuaScript *s = luaScriptOf(L);
    const char *mod = s ? s->mod : "?";
    lua_Integer x = lua_tointeger(L, 2), y = lua_tointeger(L, 3);
    SquareRect area = {0, 0, 0, 0};
    int building = worldBuildingAreaNow(&area);
    const char *reason = squareWriteRefusal(building, area, x, y);
    if (reason) {
        refuseLuaCall(mod, "coe5.set", reason, "name=%s x=%lld y=%lld", v->name, (long long)x, (long long)y);
        return 0;
    }
    if (v->type == T_STR) {
        snprintf((char *)p, v->size, "%s", luaL_checkstring(L, arg));
        return 0;
    }
    lua_Integer value = luaL_checkinteger(L, arg), min, max;
    squareValueRange(v->name, G32(RVA_LAST_TERRAIN), &min, &max);
    if (value < min || value > max) {
        refuseLuaCall(mod, "coe5.set", "out_of_range", "name=%s value=%lld min=%lld max=%lld", v->name, (long long)value,
                      (long long)min, (long long)max);
        return 0;
    }
    if (!strcmp(v->name, "squareTerrain")) GAME_FN(void (*)(int, int, int), RVA_CHANGE_SQUARE_TERRAIN)((int)x, (int)y, (int)value);
    else *(short *)p = (short)value;
    return 0;
}

// coe5.set(name, [index,] value).
static int lSet(lua_State *L) {
    const GameVar *v;
    int arg = 2;
    unsigned char *p = gameVarAddress(L, &v, &arg);
    if (!v->writable) return luaL_error(L, "game variable '%s' is read-only", v->name);
    if (!p) return luaL_error(L, "%s can only be set while a wrap of its site runs", v->name);
    if (v->writable == WRITABLE_WHILE_BUILDING) return setSquareValue(L, v, p, arg);
    if (v->type == T_WINDOWS_ONLY) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "windows_only", "name=%s", v->name);
        return 0;
    }
    if (v->type == T_GAME_CHANGED) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "game_changed", "name=%s", v->name);
        return 0;
    }
    if (v->type == T_CEREMONY_COST) return setCeremonyCost(L, p, arg);
    if (v->type == T_U8 || v->type == T_I16 || v->type == T_I32 || v->type == T_FRAME_I32) {
        lua_Integer value = luaL_checkinteger(L, arg), min, max;
        gameVarRange(v, &min, &max);
        if (value < min || value > max) {
            LuaScript *s = luaScriptOf(L);
            refuseLuaCall(s ? s->mod : "?", "coe5.set", "out_of_range", "name=%s value=%lld min=%lld max=%lld", v->name,
                          (long long)value, (long long)min, (long long)max);
            return 0;
        }
    }
    int isFlag = v->type == T_FLAG_U8 || v->type == T_FLAG_I32 || v->type == T_FLAG_I16 || v->type == T_FLAG_TWO;
    if (isFlag && lua_type(L, arg) != LUA_TBOOLEAN) {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "needs_boolean", "name=%s value=%s", v->name, luaL_typename(L, arg));
        return 0;
    }
    if (isSlotColour(v)) {
        lua_Number value = luaL_checknumber(L, arg);
        const char *reason = !isfinite(value) ? "not_finite"
                           : value < SLOT_COLOUR_MIN - 1.0 || value > SLOT_COLOUR_MAX + 1.0 || (float)value < SLOT_COLOUR_MIN ||
                                     (float)value > SLOT_COLOUR_MAX
                               ? "out_of_range"
                               : NULL;
        if (reason) {
            LuaScript *s = luaScriptOf(L);
            refuseLuaCall(s ? s->mod : "?", "coe5.set", reason, "name=%s value=%.14g min=%.14g max=%.14g", v->name, (double)value,
                          (double)SLOT_COLOUR_MIN, (double)SLOT_COLOUR_MAX);
            return 0;
        }
    }
    switch (v->type) {
    case T_U8: *p = (unsigned char)luaL_checkinteger(L, arg); break;
    case T_I16: *(short *)p = (short)luaL_checkinteger(L, arg); break;
    case T_FLAG_U8: *p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_I32: *(int *)p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_I16: *(short *)p = lua_toboolean(L, arg) ? 1 : 0; break;
    case T_FLAG_TWO: *(int *)p = lua_toboolean(L, arg) ? 2 : 1; break;
    case T_F32: {
        lua_Number value = luaL_checknumber(L, arg);
        *(float *)p = value > FLT_MAX ? INFINITY : value < -FLT_MAX ? -INFINITY : (float)value;
        break;
    }
    case T_STR: snprintf((char *)p, v->size, "%s", luaL_checkstring(L, arg)); break;
    case T_I32:
    case T_FRAME_I32: *(int *)p = (int)luaL_checkinteger(L, arg); break;
    default: {
        LuaScript *s = luaScriptOf(L);
        refuseLuaCall(s ? s->mod : "?", "coe5.set", "not_writable", "name=%s", v->name);
    }
    }
    if (!strcmp(v->name, "unitApUsed")) flushArmyApCaches((int)((p - GAME_PTR(v->addr)) / UNIT_SIZE));
    if (!strcmp(v->name, "loadedGame") && lua_tointeger(L, arg)) {
        LuaScript *s = luaScriptOf(L);
        writeLog("lua_loaded_game_set", "mod=%s value=%lld", s ? s->mod : "?", (long long)lua_tointeger(L, arg));
    }
    return 0;
}
