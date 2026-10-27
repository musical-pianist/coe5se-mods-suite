// Corpse kinds: a kind after the game's corpse words, corpsekind, raisecorpses, and the calls that apply them.
#pragma once

#define CORPSE_MASK_HUMANOID       0xc7
#define CORPSE_MASK_ANIMAL         0x38
#define CORPSE_MASK_ALL            0xff
#define CORPSE_COUNT_MAX           32767
#define CORPSE_KIND_LEAVES_NONE    SQUARE_CORPSE_KINDS
#define RITUAL_FX_RAISE_DEAD       915
#define RITUAL_FX_RAISE_ANIMALS    967
#define RITUAL_FX_PUT_CORPSES      1070
#define PUT_CORPSES_SCALED         10000
#define RAISE_SUMMON_CONTROL       0x3e5
#define RAISE_SUMMON_FAIL_PLAYER   0x18
#define ABILITY_VOIDSANITY         99
#define INSANITY_HALVED            (-9998)
#define INSANITY_MAX               100
#define CONTROLLER_REMOTE_HUMAN    5
#define CORPSE_CASTS_MAX           4

// The kind and group names, each with its kinds' bits.
static const struct {
    const char *name;
    int mask;
} CORPSE_KIND_WORDS[] = {
    {"small", 1 << CORPSE_KIND_SMALL},           {"human", 1 << CORPSE_KIND_HUMAN},     {"giant", 1 << CORPSE_KIND_GIANT},
    {"smallanimal", 1 << CORPSE_KIND_SMALL_ANIMAL}, {"largeanimal", 1 << CORPSE_KIND_LARGE_ANIMAL},
    {"markata", 1 << CORPSE_KIND_MARKATA},       {"vanara", 1 << CORPSE_KIND_VANARA},   {"bandar", 1 << CORPSE_KIND_BANDAR},
    {"humanoid", CORPSE_MASK_HUMANOID},          {"animal", CORPSE_MASK_ANIMAL},        {"all", CORPSE_MASK_ALL},
};

enum { CORPSE_RULE_PUT, CORPSE_RULE_RAISE_DEAD, CORPSE_RULE_RAISE_ANIMALS, CORPSE_RULE_RAISE_CORPSES, CORPSE_RULE_DROPPED };

// One ritual line of this pass that names corpse kinds.
typedef struct {
    int ritual, type, occurrence, mask, line;
    char mod[64], monster[MONSTER_NAME_SIZE];
} CorpseRule;

static CorpseRule *corpseRules;
static int corpseRuleCount, corpseRuleCapacity;
static unsigned char *monsterCorpseKinds;
static int monsterCorpseKindCount;

// A cast in progress: what the hooks inside RVA_EXECUTE_RITUAL need.
typedef struct {
    int ritual, unit, human, percent, puts, raiseDeads, raiseAnimals;
} CorpseCast;

static CorpseCast corpseCasts[CORPSE_CASTS_MAX];
static int corpseCastDepth;

// Reads a kind or group name at *p into *mask and moves past it; 0 for any other text.
static int readCorpseKinds(const char **p, int *mask) {
    size_t len = strcspn(*p, " \t\r\n");
    for (size_t i = 0; i < sizeof CORPSE_KIND_WORDS / sizeof CORPSE_KIND_WORDS[0]; i++) {
        if (strlen(CORPSE_KIND_WORDS[i].name) != len || strncmp(CORPSE_KIND_WORDS[i].name, *p, len)) continue;
        *mask = CORPSE_KIND_WORDS[i].mask;
        *p += len;
        return 1;
    }
    return 0;
}

// The one kind in `mask`, or -1 when it holds more or none.
static int onlyCorpseKind(int mask) {
    for (int k = 0; k < SQUARE_CORPSE_KINDS; k++)
        if (mask == 1 << k) return k;
    return -1;
}

// The square's 8 corpse counts, or NULL outside the world.
static short *squareCorpses(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return NULL;
    return (short *)(gameBase + RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_CORPSES);
}

static int countCorpses(const short *corpses, int mask) {
    int count = 0;
    for (int k = 0; k < SQUARE_CORPSE_KINDS; k++)
        if (mask & 1 << k) count += corpses[k];
    return count;
}

// Adds n corpses of one kind, up to the count's top.
static void addCorpses(short *corpses, int kind, int n) {
    int total = corpses[kind] + n;
    corpses[kind] = (short)(total > CORPSE_COUNT_MAX ? CORPSE_COUNT_MAX : total);
}

// Takes up to n corpses of the kinds in `mask`, one of each kind in turn; returns how many it took.
static int takeCorpses(short *corpses, int mask, int n) {
    int taken = 0;
    for (int round = 0; round < n && taken < n; round++)
        for (int k = 0; k < SQUARE_CORPSE_KINDS && taken < n; k++)
            if (mask & 1 << k && corpses[k] > 0) corpses[k]--, taken++;
    return taken;
}

// How many effects `id` ritual `ritual` holds now.
static int ritualEffectCount(int ritual, int id) {
    unsigned char *record = ritualWordRecord(ritual);
    int count = 0;
    for (int s = 0; s < RITUAL_FX_MAX && ritualWordEffect(record, s)[0]; s++) count += ritualWordEffect(record, s)[0] == id;
    return count;
}

// The value of the ritual's nth effect `id`, from 0; 0 when it has fewer.
static long long ritualEffectValueAt(int ritual, int id, int nth) {
    unsigned char *record = ritualWordRecord(ritual);
    for (int s = 0; s < RITUAL_FX_MAX && ritualWordEffect(record, s)[0]; s++)
        if (ritualWordEffect(record, s)[0] == id && nth-- == 0) return ritualWordEffect(record, s)[1];
    return 0;
}

// The rule for the ritual's nth effect of a type, or NULL.
static const CorpseRule *corpseRuleFor(int ritual, int type, int occurrence) {
    for (int i = 0; i < corpseRuleCount; i++)
        if (corpseRules[i].ritual == ritual && corpseRules[i].type == type && corpseRules[i].occurrence == occurrence) return &corpseRules[i];
    return NULL;
}

// Adds a rule for the current ritual's line; 0 when it can't.
static int addCorpseRule(const char *word, int mod, int type, int occurrence, int mask, const char *monster) {
    if (!growList((void **)&corpseRules, &corpseRuleCapacity, corpseRuleCount + 1, sizeof corpseRules[0])) {
        refuseC5mLine(word, spellModName(mod), "out_of_memory", "line=%d", G32(RVA_MOD_LINE));
        return 0;
    }
    CorpseRule *r = &corpseRules[corpseRuleCount++];
    memset(r, 0, sizeof *r);
    r->ritual = G32(RVA_RITUAL_CURRENT);
    r->type = type;
    r->occurrence = occurrence;
    r->mask = mask;
    r->line = G32(RVA_MOD_LINE);
    snprintf(r->mod, sizeof r->mod, "%s", spellModName(mod));
    if (monster) snprintf(r->monster, sizeof r->monster, "%s", monster);
    return 1;
}

// The current ritual, or -1 after a refusal naming `word`.
static int currentCorpseRitual(const char *word, int mod) {
    int record = G32(RVA_RITUAL_CURRENT);
    if (record >= 0 && record < ritualSlotCount()) return record;
    refuseC5mLine(word, spellModName(mod), "no_ritual", "line=%d", G32(RVA_MOD_LINE));
    return -1;
}

// putcorpses, raisedead and raiseanimals with kinds after them, before the game's parser.
static const char *corpseWordLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2) return line;
    static char rewritten[64];
    int type;
    const char *word;
    if (len == 10 && !strncmp(line, "putcorpses", len)) type = CORPSE_RULE_PUT, word = "putcorpses";
    else if (len == 9 && !strncmp(line, "raisedead", len)) type = CORPSE_RULE_RAISE_DEAD, word = "raisedead";
    else if (len == 12 && !strncmp(line, "raiseanimals", len)) type = CORPSE_RULE_RAISE_ANIMALS, word = "raiseanimals";
    else return line;
    const char *p = line + len;
    long long n = 0;
    skipSpaces(&p);
    if (type == CORPSE_RULE_PUT && !readSpellNumber(&p, &n)) return line;
    skipSpaces(&p);
    if (isLineEnd(p)) return line;
    int mask = 0;
    if (!readCorpseKinds(&p, &mask) || (skipSpaces(&p), !isLineEnd(p))) {
        refuseC5mLine(word, spellModName(mod), "bad_kind", "line=%d text=%.40s", G32(RVA_MOD_LINE), line + len);
        return NULL;
    }
    int allowed = type == CORPSE_RULE_RAISE_DEAD ? CORPSE_MASK_HUMANOID : type == CORPSE_RULE_RAISE_ANIMALS ? CORPSE_MASK_ANIMAL : CORPSE_MASK_ALL;
    if (mask & ~allowed) {
        refuseC5mLine(word, spellModName(mod), type == CORPSE_RULE_RAISE_DEAD ? "not_humanoid" : "not_animal", "line=%d text=%.40s",
                      G32(RVA_MOD_LINE), line + len);
        return NULL;
    }
    if (type == CORPSE_RULE_PUT && onlyCorpseKind(mask) < 0) {
        refuseC5mLine(word, spellModName(mod), "not_one_kind", "line=%d text=%.40s", G32(RVA_MOD_LINE), line + len);
        return NULL;
    }
    if (type == CORPSE_RULE_PUT && n <= 0) {
        refuseC5mLine(word, spellModName(mod), "bad_value", "line=%d value=%lld min=1", G32(RVA_MOD_LINE), n);
        return NULL;
    }
    int ritual = currentCorpseRitual(word, mod);
    if (ritual < 0) return NULL;
    int id = type == CORPSE_RULE_PUT ? RITUAL_FX_PUT_CORPSES : type == CORPSE_RULE_RAISE_DEAD ? RITUAL_FX_RAISE_DEAD : RITUAL_FX_RAISE_ANIMALS;
    if (!addCorpseRule(word, mod, type, ritualEffectCount(ritual, id), mask, NULL)) return NULL;
    if (type == CORPSE_RULE_PUT) snprintf(rewritten, sizeof rewritten, "putcorpses %lld", n);
    else snprintf(rewritten, sizeof rewritten, "%s", word);
    return rewritten;
}

// corpsekind: the corpse a unit of the current monster leaves, or none.
static void corpsekindLine(const char *args, int mod, int level) {
    if (level != 2) return;
    const char *p = args;
    int mask = 0, kind = -1;
    skipSpaces(&p);
    size_t len = strcspn(p, " \t\r\n");
    if (len == 4 && !strncmp(p, "none", 4)) kind = CORPSE_KIND_LEAVES_NONE, p += 4;
    else if (readCorpseKinds(&p, &mask)) kind = onlyCorpseKind(mask);
    skipSpaces(&p);
    if (kind < 0 || !isLineEnd(p)) {
        refuseC5mLine("corpsekind", spellModName(mod), "bad_kind", "line=%d text=%.40s", G32(RVA_MOD_LINE), args);
        return;
    }
    int monster = G32(RVA_MONSTER_CURRENT);
    if (monster < 0 || monster >= tableRecords(TABLE_MONSTER)) {
        refuseC5mLine("corpsekind", spellModName(mod), "no_monster", "line=%d", G32(RVA_MOD_LINE));
        return;
    }
    if (monster >= monsterCorpseKindCount) {
        int count = tableRecords(TABLE_MONSTER);
        unsigned char *grown = realloc(monsterCorpseKinds, (size_t)count);
        if (!grown) {
            refuseC5mLine("corpsekind", spellModName(mod), "out_of_memory", "line=%d", G32(RVA_MOD_LINE));
            return;
        }
        memset(grown + monsterCorpseKindCount, 0, (size_t)(count - monsterCorpseKindCount));
        monsterCorpseKinds = grown;
        monsterCorpseKindCount = count;
    }
    monsterCorpseKinds[monster] = (unsigned char)(kind + 1);
}

// raisecorpses: raises a monster from the current ritual's caster's corpses of those kinds.
static void raisecorpsesLine(const char *args, int mod, int level) {
    if (level != 2) return;
    const char *p = args;
    int mask = 0;
    char name[MONSTER_NAME_SIZE] = "";
    skipSpaces(&p);
    int ok = readCorpseKinds(&p, &mask);
    skipSpaces(&p);
    if (ok && *p == '"') {
        const char *end = strchr(p + 1, '"');
        size_t n = end ? (size_t)(end - p - 1) : 0;
        ok = end && n > 0 && n < sizeof name;
        if (ok) memcpy(name, p + 1, n), name[n] = '\0', p = end + 1;
    } else ok = 0;
    skipSpaces(&p);
    if (!ok || !isLineEnd(p)) {
        refuseC5mLine("raisecorpses", spellModName(mod), "bad_value", "line=%d text=%.60s", G32(RVA_MOD_LINE), args);
        return;
    }
    if (currentCorpseRitual("raisecorpses", mod) < 0) return;
    addCorpseRule("raisecorpses", mod, CORPSE_RULE_RAISE_CORPSES, 0, mask, name);
}

// Before a level-2 mod pass: the pass reads every corpse word again.
static void corpseWordsBeforePass(int level) {
    if (level != 2) return;
    corpseRuleCount = 0;
    if (monsterCorpseKinds) memset(monsterCorpseKinds, 0, (size_t)monsterCorpseKindCount);
}

// After a level-2 mod pass: drops each raisecorpses whose monster no mod made.
static void corpseWordsAfterPass(int level) {
    if (level != 2) return;
    for (int i = 0; i < corpseRuleCount; i++) {
        CorpseRule *r = &corpseRules[i];
        if (r->type != CORPSE_RULE_RAISE_CORPSES || monsterByName(r->monster) >= 0) continue;
        refuseC5mLine("raisecorpses", r->mod, "unknown_monster", "line=%d monster=%.48s", r->line, r->monster);
        r->type = CORPSE_RULE_DROPPED;
    }
}

// ---- Events: +mincorpses and removecorpses with kinds ----------------------------------------------
// One event line on one square: a trigger's answer, or the action done.
static int corpseEventSquare(short *corpses, int mask, int n, int remove) {
    if (remove) {
        if (corpses) takeCorpses(corpses, mask, n);
        return 1;
    }
    return (corpses ? countCorpses(corpses, mask) : 0) >= n;
}

// +mincorpses, -mincorpses and removecorpses with kinds, before the game's event runner; 1 when it took the line.
static int corpseEventLine(const char *line, int level) {
    int sign = 0, remove = 0;
    const char *p;
    if ((line[0] == '+' || line[0] == '-') && !strncmp(line + 1, "mincorpses", 10) && (line[11] == ' ' || line[11] == '\t'))
        sign = line[0] == '+' ? 1 : -1, p = line + 11;
    else if (!strncmp(line, "removecorpses", 13) && (line[13] == ' ' || line[13] == '\t')) remove = 1, p = line + 13;
    else return 0;
    long long n = 0;
    skipSpaces(&p);
    if (!readSpellNumber(&p, &n)) return 0;
    skipSpaces(&p);
    if (isLineEnd(p)) return 0;
    if ((level != 2 && level != 3) || G32(RVA_EVENT_STATE) < 1) return 1;
    int mask = 0;
    if (!readCorpseKinds(&p, &mask) || (skipSpaces(&p), !isLineEnd(p))) {
        refuseC5mLine(remove ? "removecorpses" : sign > 0 ? "+mincorpses" : "-mincorpses", "-", "bad_kind", "line=%.60s", line);
        if (!remove) G32(RVA_EVENT_STATE) = EVENT_FAILED;
        return 1;
    }
    int count = n > CORPSE_COUNT_MAX * SQUARE_CORPSE_KINDS ? CORPSE_COUNT_MAX * SQUARE_CORPSE_KINDS : n < 0 ? 0 : (int)n;
    if (G32(RVA_EVENT_STATE) == PLAYER_EVENT) {
        int x = G32(RVA_EVENT_TARGET_X), y = G32(RVA_EVENT_TARGET_Y);
        if (x < 0 || y < 0) return 1;
        int holds = corpseEventSquare(squareCorpses(x, y), mask, count, remove);
        if (!remove && holds != (sign > 0)) G32(RVA_EVENT_STATE) = EVENT_FAILED;
        return 1;
    }
    unsigned char *marks = gameBase + RVA_SQUARE_EVENT_A;
    int any = 0;
    for (size_t i = 0; i < SQUARE_EVENT_A_SIZE; i++) {
        if (!marks[i]) continue;
        int holds = corpseEventSquare(squareCorpses((int)(i / MAP_COLUMN_SQUARES), (int)(i % MAP_COLUMN_SQUARES)), mask, count, remove);
        if (!remove && holds != (sign > 0)) marks[i] = 0;
        else any = 1;
    }
    if (!remove && !any) G32(RVA_EVENT_STATE) = EVENT_FAILED;
    return 1;
}

// ---- The calls: a cast, its raises and putcorpses, and a unit's corpse ------------------------------
typedef long long (*ExecuteRitualFn)(int, int, int, int, int, int);
typedef int (*RaiseFn)(int);
typedef const char *(*SquareTextFn)(int, int);
typedef void (*LeaveCorpseFn)(int);
static ExecuteRitualFn executeRitualGame;
static RaiseFn raiseDeadGame, raiseAnimalsGame;
static SquareTextFn squareTextGame;
static LeaveCorpseFn leaveCorpseGame;
static int (*corpseRandom)(int), (*corpseOpenDie)(int), (*corpseMindless)(int);
static int (*corpseAbilityTotal)(int, int, int, char *, int, int *);
static int (*corpseSummon)(int, const char *, int, int, int);
static void (*corpseSendSquare)(int, int, int);
static uint64_t (*corpseProgressText)(int, const char *, ...);

// 1 when the unit's owner is a human player.
static int unitOwnerIsHuman(int unit) {
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return 0;
    int owner = (short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
    if (owner < 0 || owner >= playerSlotCount()) return 0;
    short controller;
    memcpy(&controller, playerSlotAddress(RVA_SLOT_CONTROLLER, owner, 2), 2);
    return controller == CONTROLLER_HUMAN || controller == CONTROLLER_REMOTE_HUMAN;
}

static CorpseCast *currentCorpseCast(void) {
    return corpseCastDepth > 0 && corpseCastDepth <= CORPSE_CASTS_MAX ? &corpseCasts[corpseCastDepth - 1] : NULL;
}

// The game's square root (RVA_INT_SQRT): the largest i with i * i at most n, 40000 at most; 0 below 1.
static int gameSquareRoot(int n) {
    if (n < 1) return 0;
    int i = 1;
    while (i < 40000 && (long long)i * i <= n) i++;
    return i < 40000 || (long long)i * i > n ? i - 1 : 40000;
}

// Adds insanity to the unit's commander, as the game's RVA_INSANITY_ADD does (inlined on Mac).
static void addInsanity(int unit, int amount) {
    int last = G32(RVA_LAST_COMMANDER), commander = -1;
    for (int c = 0; c <= last && c < COMMANDER_TABLE_MAX && commander < 0; c++)
        if (G32(RVA_COMMANDERS + (size_t)c * COMMANDER_SIZE) == unit) commander = c;
    if (unit < 0 || commander < 0 || corpseMindless(unit)) return;
    int resist = corpseAbilityTotal(unit, ABILITY_VOIDSANITY, 1, NULL, 0, NULL);
    int add = amount > 0 && resist != 0 ? (amount - resist > 0 ? amount - resist : 0) : amount;
    short *insanity = (short *)(gameBase + RVA_COMMANDER_FIELD_10 + (size_t)commander * COMMANDER_SIZE);
    int value = amount == INSANITY_HALVED ? *insanity / 2 : (short)(*insanity + add);
    *insanity = (short)(value < 0 ? 0 : value > INSANITY_MAX ? INSANITY_MAX : value);
}

// raisecorpses for a cast that ended: each of the ritual's raises, at the caster's square.
static void runRaiseCorpses(const CorpseCast *cast) {
    for (int i = 0; i < corpseRuleCount; i++) {
        const CorpseRule *r = &corpseRules[i];
        if (r->type != CORPSE_RULE_RAISE_CORPSES || r->ritual != cast->ritual) continue;
        int unit = cast->unit;
        if (unit < 0 || unit >= UNIT_TABLE_MAX) return;
        int x = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE);
        short *corpses = squareCorpses(x, y);
        int mask = cast->human ? r->mask : CORPSE_MASK_ALL;
        int n = corpses ? countCorpses(corpses, mask) : 0;
        if (n < 1) {
            corpseProgressText(4000, "CoE5SE: Ritual failed due to no suitable corpses found.");
            continue;
        }
        int raised = corpseRandom(n + 1);
        if (n - raised > 0) raised += corpseRandom(n - raised) + 1;
        takeCorpses(corpses, mask, raised);
        corpseSendSquare((short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE), x, y);
        corpseSummon(unit, r->monster, RAISE_SUMMON_CONTROL, raised, RAISE_SUMMON_FAIL_PLAYER);
        int root = gameSquareRoot(raised);
        addInsanity(unit, root + 1 + corpseOpenDie(root * 2));
    }
}

// RVA_EXECUTE_RITUAL at each of its calls: notes the cast for the hooks inside it, then runs raisecorpses.
static long long corpseExecuteRitual(int unit, int ritual, int x, int y, int percent, int pay) {
    if (!corpseRuleCount) return executeRitualGame(unit, ritual, x, y, percent, pay);
    int depth = corpseCastDepth++;
    if (depth < CORPSE_CASTS_MAX) corpseCasts[depth] = (CorpseCast){ritual, unit, unitOwnerIsHuman(unit), percent, 0, 0, 0};
    long long result = executeRitualGame(unit, ritual, x, y, percent, pay);
    corpseCastDepth--;
    if (depth < CORPSE_CASTS_MAX && result != -1) runRaiseCorpses(&corpseCasts[depth]);
    return result;
}

// A raise with only the kinds in `mask`: the others are set aside for the game's call, then put back.
static int raiseWithKinds(RaiseFn game, int commander, int mask) {
    if (commander < 0 || commander > G32(RVA_LAST_COMMANDER) || commander >= COMMANDER_TABLE_MAX) return game(commander);
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return game(commander);
    int x = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE);
    short *corpses = squareCorpses(x, y), aside[SQUARE_CORPSE_KINDS] = {0};
    if (!corpses) return game(commander);
    for (int k = 0; k < SQUARE_CORPSE_KINDS; k++)
        if (!(mask & 1 << k)) aside[k] = corpses[k], corpses[k] = 0;
    int raised = game(commander), anyAside = 0;
    for (int k = 0; k < SQUARE_CORPSE_KINDS; k++) anyAside |= aside[k], addCorpses(corpses, k, aside[k]);
    if (anyAside) corpseSendSquare((short)G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE), x, y);
    return raised;
}

static int corpseRaise(RaiseFn game, int commander, int type, int *seen) {
    CorpseCast *cast = currentCorpseCast();
    int occurrence = cast ? (*seen)++ : -1;
    const CorpseRule *r = cast && cast->human ? corpseRuleFor(cast->ritual, type, occurrence) : NULL;
    return r ? raiseWithKinds(game, commander, r->mask) : game(commander);
}

static int corpseRaiseDead(int commander) {
    CorpseCast *cast = currentCorpseCast();
    return corpseRaise(raiseDeadGame, commander, CORPSE_RULE_RAISE_DEAD, cast ? &cast->raiseDeads : NULL);
}

static int corpseRaiseAnimals(int commander) {
    CorpseCast *cast = currentCorpseCast();
    return corpseRaise(raiseAnimalsGame, commander, CORPSE_RULE_RAISE_ANIMALS, cast ? &cast->raiseAnimals : NULL);
}

// putcorpses' trace call, right after the game added human corpses: moves them to the line's kind.
static const char *corpsePutText(int x, int y) {
    CorpseCast *cast = currentCorpseCast();
    int occurrence = cast ? cast->puts++ : -1;
    const CorpseRule *r = cast && cast->human ? corpseRuleFor(cast->ritual, CORPSE_RULE_PUT, occurrence) : NULL;
    short *corpses = r ? squareCorpses(x, y) : NULL;
    if (corpses) {
        long long value = ritualEffectValueAt(cast->ritual, RITUAL_FX_PUT_CORPSES, occurrence);
        long long n = value >= PUT_CORPSES_SCALED ? (value - PUT_CORPSES_SCALED) * cast->percent / 100 : value;
        int moved = n <= 0 ? 0 : n < corpses[CORPSE_KIND_HUMAN] ? (int)n : corpses[CORPSE_KIND_HUMAN];
        corpses[CORPSE_KIND_HUMAN] = (short)(corpses[CORPSE_KIND_HUMAN] - moved);
        addCorpses(corpses, onlyCorpseKind(r->mask), moved);
    }
    return squareTextGame(x, y);
}

// A dying unit's corpse: its monster's corpsekind when it has one, else the game's choice.
static void corpseLeave(int unit) {
    int monster = unit >= 0 && unit < UNIT_TABLE_MAX ? (short)G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) : -1;
    int kind = monster >= 0 && monster < monsterCorpseKindCount ? monsterCorpseKinds[monster] - 1 : -1;
    if (kind < 0) {
        leaveCorpseGame(unit);
        return;
    }
    if (kind == CORPSE_KIND_LEAVES_NONE) return;
    short *corpses = squareCorpses(G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE));
    if (corpses) addCorpses(corpses, kind, 1);
}

typedef struct {
    unsigned int callRva, targetRva;
    void *hook;
    const char *name;
} CorpseCall;

#ifndef __APPLE__
#define CORPSE_STUB_SIZE 0x20
static int pointCorpseCall(const CorpseCall *c, int index) {
    static unsigned char *stubs;
    if (!stubs) {
        if (!(stubs = allocNear(gameBase + RVA_TEXT_SECTION))) {
            writeLog("hook_skipped", "site=%s reason=no_near_page", c->name);
            return 0;
        }
        makePagesReadExecute(stubs, 0x1000);
    }
    return redirectCall(c->callRva, c->targetRva, stubs + CORPSE_STUB_SIZE * index, c->hook, c->name, NULL);
}
#else
static int pointCorpseCall(const CorpseCall *c, int index) {
    (void)index;
    if (blTarget(gameBase + c->callRva) != gameBase + c->targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", c->name, c->callRva + 0x100000000ULL);
        return 0;
    }
    static void *hooks[8];
    static unsigned char *trampolines[8];
    int t = 0;
    while (t < 8 && hooks[t] && hooks[t] != c->hook) t++;
    if (t == 8) {
        writeLog("hook_skipped", "site=%s reason=no_padding", c->name);
        return 0;
    }
    if (!hooks[t]) trampolines[t] = newTrampoline(c->hook), hooks[t] = trampolines[t] ? c->hook : NULL;
    uint32_t word = trampolines[t] ? blTo(gameBase + c->callRva, trampolines[t]) : 0;
    if (trampolines[t] && writeGameCode(gameBase + c->callRva, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=%s", c->name, trampolines[t] ? "code_unwritable" : "no_padding");
    return 0;
}
#endif

// Adds corpsekind and raisecorpses, and points the game's calls at the hooks above.
static void corpseKindsInit(void) {
    executeRitualGame = GAME_FN(ExecuteRitualFn, RVA_EXECUTE_RITUAL);
    raiseDeadGame = GAME_FN(RaiseFn, RVA_RAISE_DEAD);
    raiseAnimalsGame = GAME_FN(RaiseFn, RVA_RAISE_ANIMALS);
    squareTextGame = GAME_FN(SquareTextFn, RVA_SQUARE_TEXT);
    leaveCorpseGame = GAME_FN(LeaveCorpseFn, RVA_UNIT_LEAVE_CORPSE);
    corpseRandom = GAME_FN(int (*)(int), RVA_RANDOM);
    corpseOpenDie = GAME_FN(int (*)(int), RVA_RANDOM_OPEN_DIE);
    corpseMindless = GAME_FN(int (*)(int), RVA_UNIT_MINDLESS);
    corpseAbilityTotal = GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL);
    corpseSummon = GAME_FN(int (*)(int, const char *, int, int, int), RVA_RITUAL_SUMMON_UNITS);
    corpseSendSquare = GAME_FN(void (*)(int, int, int), RVA_NET_SEND_SQUARE);
    corpseProgressText = GAME_FN(uint64_t (*)(int, const char *, ...), RVA_PROGRESS_TEXT);
    int words = apiAddModCommand("corpsekind", corpsekindLine) + apiAddModCommand("raisecorpses", raisecorpsesLine);
    static const CorpseCall CALLS[] = {
        {RVA_DAMAGE_LEAVE_CORPSE_CALL, RVA_UNIT_LEAVE_CORPSE, (void *)corpseLeave, "damage_leave_corpse"},
        {RVA_HP_LOSS_LEAVE_CORPSE_CALL, RVA_UNIT_LEAVE_CORPSE, (void *)corpseLeave, "hp_loss_leave_corpse"},
        {RVA_RITUAL_RAISE_DEAD_CALL, RVA_RAISE_DEAD, (void *)corpseRaiseDead, "ritual_raise_dead"},
        {RVA_RITUAL_RAISE_ANIMALS_CALL, RVA_RAISE_ANIMALS, (void *)corpseRaiseAnimals, "ritual_raise_animals"},
        {RVA_RITUAL_PUT_CORPSES_TEXT_CALL, RVA_SQUARE_TEXT, (void *)corpsePutText, "ritual_put_corpses"},
        {RVA_AI_APPRSPAM_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "ai_apprspam_execute"},
        {RVA_AI_PLAN_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "ai_plan_execute"},
        {RVA_AI_ALWAYS_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "ai_always_execute"},
        {RVA_RITUAL_SCREEN_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "ritual_screen_execute"},
        {RVA_REPEAT_RITUAL_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "repeat_ritual_execute"},
#ifdef __APPLE__
        // @platform-split repeat_ritual_execute: Mac copies the repeat loop's call four times
        {RVA_REPEAT_RITUAL_EXECUTE_CALL_B, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "repeat_ritual_execute_b"},
        {RVA_REPEAT_RITUAL_EXECUTE_CALL_C, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "repeat_ritual_execute_c"},
        {RVA_REPEAT_RITUAL_EXECUTE_CALL_D, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "repeat_ritual_execute_d"},
#endif
        {RVA_CAST_BY_NAME_EXECUTE_CALL, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "cast_by_name_execute"},
        {RVA_ELYSIUM_EXECUTE_CALL_A, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "elysium_execute_a"},
        {RVA_ELYSIUM_EXECUTE_CALL_B, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "elysium_execute_b"},
        {RVA_ELYSIUM_EXECUTE_CALL_C, RVA_EXECUTE_RITUAL, (void *)corpseExecuteRitual, "elysium_execute_c"},
    };
    int count = (int)(sizeof CALLS / sizeof CALLS[0]), calls = 0;
    if (checkGameFunction("corpse_kinds_check", ""))
        for (int i = 0; i < count; i++) calls += pointCorpseCall(&CALLS[i], i);
    writeLog("corpse_kinds_installed", "words=%d of 2 calls=%d of %d", words, calls, count);
}
