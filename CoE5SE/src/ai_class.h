// aiclass 2: a class no player gets, neither by the random roll nor by Choose Class.
#pragma once

#define AICLASS_WORD          "aiclass"
#define AI_CLASS_NEITHER_LEVEL 1000

#define AI_CLASS_SKIP_NEVER    100
#define AI_CLASS_ROLL_SLOTS    32

static short aiClassLineMods[LAST_MOD_CLASS_SLOT + 1];
static int aiClassesKept;
static int (*aiClassRandom)(int n);
static void (*nextAiClassCreatingWorld)(int kind, const char *text);

// Starts a level-2 pass: no aiclass line read yet.
static void aiClassBeforePass(int level) {
    if (level != 2) return;
    memset(aiClassLineMods, 0, sizeof aiClassLineMods);
}

// Notes the mod of an aiclass line, for the refusals after the pass; the game reads the line.
static void aiClassLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2 || !spellWordIs(line, len, AICLASS_WORD)) return;
    int c = G32(RVA_CLASS_CURRENT);
    if (c >= 0 && c <= LAST_MOD_CLASS_SLOT) aiClassLineMods[c] = (short)(mod + 1);
}

// Class `c`'s pair for `ability`, or NULL when it has none: the first match, as the game's lookup.
static int *aiClassPair(int c, int ability) {
    for (int slot = 0; slot < CLASS_ABILITY_PAIRS; slot++) {
        int *pair = classAbilityPairAt(c, slot);
        if (!pair[0]) return NULL;
        if (pair[0] == ability) return pair;
    }
    return NULL;
}

// Sets class `c`'s `ability` to `value`, in its pair or the first free one; 0 when all are taken.
static int setAiClassPair(int c, int ability, int value) {
    int *pair = aiClassPair(c, ability);
    for (int slot = 0; !pair && slot < CLASS_ABILITY_PAIRS; slot++)
        if (!classAbilityPairAt(c, slot)[0]) pair = classAbilityPairAt(c, slot);
    if (!pair) return 0;
    pair[0] = ability, pair[1] = value;
    return 1;
}

// Class `c`'s aiclass, 0 when it has none.
static int aiClassValue(int c) {
    const int *pair = aiClassPair(c, CLASS_ABILITY_AI_CLASS);
    return pair ? pair[1] : AI_CLASS_ANY;
}

// After a level-2 mod pass: keeps each aiclass 2 class from every player, or from no side left without a class.
static void aiClassAfterPass(int level) {
    if (level != 2) return;
    int last = G32(RVA_CLASS_COUNT), cap = tableRecords(TABLE_CLASS) - 1;
    if (cap > LAST_MOD_CLASS_SLOT) cap = LAST_MOD_CLASS_SLOT;
    if (last > cap) last = cap;
    int neither = 0, forHumans = 0, forAis = 0;
    for (int c = 1; c <= last; c++) {
        int value = aiClassValue(c);
        if (value == AI_CLASS_NEITHER) neither++;
        else forHumans += value <= 0, forAis += value >= 0;
    }
    aiClassesKept = 0;
    if (!neither) return;
    int instead = !forHumans && !forAis ? AI_CLASS_ANY : !forHumans ? AI_CLASS_HUMAN_ONLY : !forAis ? AI_CLASS_AI_ONLY : AI_CLASS_NEITHER;
    const char *reason = !forHumans && !forAis ? "no_other_class" : !forHumans ? "no_other_class_for_humans" : "no_other_class_for_ais";
    int kept = 0;
    for (int c = 1; c <= last; c++) {
        if (aiClassValue(c) != AI_CLASS_NEITHER) continue;
        const char *name = (const char *)(tableAddress(TABLE_CLASS) + (size_t)c * CLASS_SIZE);
        const char *mod = aiClassLineMods[c] ? spellModName(aiClassLineMods[c] - 1) : "";
        if (instead != AI_CLASS_NEITHER) {
            setAiClassPair(c, CLASS_ABILITY_AI_CLASS, instead);
            refuseC5mLine(AICLASS_WORD, mod, reason, "class=%.48s aiclass=%d", name, instead);
        } else if (!setAiClassPair(c, CLASS_ABILITY_AI_LEVEL_MIN, AI_CLASS_NEITHER_LEVEL)) {
            refuseC5mLine(AICLASS_WORD, mod, "class_abilities_full", "class=%.48s aiclass=%d", name, AI_CLASS_AI_ONLY);
        } else {
            kept++;
        }
    }
    aiClassesKept = kept;
    writeLog("aiclass_neither", "classes=%d kept=%d humans=%d ais=%d", neither, kept, forHumans, forAis);
}

// ---- The roll -----------------------------------------------------------------------------------------
// Slot `slot`'s int16 at `rva` (a per-slot array, `stride` 2, or a per-player-block field).
static short *aiClassSlotField(unsigned int rva, int slot, unsigned int stride) {
    return (short *)playerTableAddress(rva + (unsigned int)slot * stride);
}

// Class `c`'s value of `ability`, 0 when it has none.
static int aiClassAbility(int c, int ability) {
    const int *pair = aiClassPair(c, ability);
    return pair ? pair[1] : 0;
}

// Whether class `c` passes slot `slot`'s roll by the game's rules but the skip draw; `letNeither` lets aiclass 2 through.
static int aiClassFits(int c, int slot, int unique, int letNeither) {
    int value = aiClassValue(c), controller = *aiClassSlotField(RVA_SLOT_CONTROLLER, slot, 2);
    int min = aiClassAbility(c, CLASS_ABILITY_AI_LEVEL_MIN), max = aiClassAbility(c, CLASS_ABILITY_AI_LEVEL_MAX);
    if (value == AI_CLASS_NEITHER && letNeither) value = AI_CLASS_ANY, min = 0;
    if (value > 0 && controller != CONTROLLER_AI) return 0;
    if (value < 0 && controller != CONTROLLER_HUMAN) return 0;
    if (controller == CONTROLLER_AI) {
        int level = *aiClassSlotField(RVA_SLOT_AI, slot, PLAYER_BLOCK_SIZE);
        if ((max > 0 && level > max) || (min > 0 && level < min)) return 0;
    }
    for (int other = 0; unique && other < AI_CLASS_ROLL_SLOTS; other++) {
        unsigned short holder = (unsigned short)*aiClassSlotField(RVA_SLOT_CONTROLLER, other, 2);
        if (other != slot && holder <= CONTROLLER_AI && *aiClassSlotField(RVA_SLOT_CLASS, other, 2) == c) return 0;
    }
    return 1;
}

// Whether any class can come out of slot `slot`'s roll: one that fits and doesn't skip every draw.
static int aiClassAnyFits(int slot, int unique, int letNeither, int last) {
    for (int c = 1; c <= last; c++)
        if (aiClassAbility(c, CLASS_ABILITY_RANDOM_SKIP) < AI_CLASS_SKIP_NEVER && aiClassFits(c, slot, unique, letNeither)) return 1;
    return 0;
}

// Slot `slot`'s class as the game's roll gives it, letting aiclass 2 through only when no other class can come; 0 when none can.
static int rollAiClass(int slot, int unique, int *letNeither) {
    int last = G32(RVA_CLASS_COUNT), cap = tableRecords(TABLE_CLASS) - 1;
    if (last > cap) last = cap;
    *letNeither = 0;
    if (last < 1) return 0;
    if (!aiClassAnyFits(slot, unique, 0, last)) {
        if (!aiClassAnyFits(slot, unique, 1, last)) return 0;
        *letNeither = 1;
    }
    for (;;) {
        int c = aiClassRandom(last) + 1;
        int skip = aiClassAbility(c, CLASS_ABILITY_RANDOM_SKIP);
        if (skip > 0 && aiClassRandom(100) < skip) continue;
        if (aiClassFits(c, slot, unique, *letNeither)) return c;
    }
}

// Rolls each Random slot's class after the Creating World text, as the game would next, while any class has aiclass 2.
static void aiClassCreatingWorldHook(int kind, const char *text) {
    nextAiClassCreatingWorld(kind, text);
    if (!aiClassesKept) return;
    int unique = *(const int *)(siteCallerSp(SITE_NEW_WORLD_CREATING_WORLD) + FRAME_NEW_WORLD_UNIQUE_RANDOM);
    for (int slot = 0; slot < AI_CLASS_ROLL_SLOTS; slot++) {
        unsigned short controller = (unsigned short)*aiClassSlotField(RVA_SLOT_CONTROLLER, slot, 2);
        short *cls = aiClassSlotField(RVA_SLOT_CLASS, slot, 2);
        if (controller > CONTROLLER_AI || *cls > 0) continue;
        int letNeither, c = rollAiClass(slot, unique, &letNeither);
        if (!c) {
            writeLog("aiclass_roll_left_to_game", "slot=%d controller=%d unique=%d", slot, controller, unique);
            continue;
        }
        *cls = (short)c;
        if (letNeither) refuseC5mLine(AICLASS_WORD, aiClassLineMods[c] ? spellModName(aiClassLineMods[c] - 1) : "", "no_other_class_for_slot",
                                      "class=%.48s slot=%d controller=%d unique=%d", (const char *)(tableAddress(TABLE_CLASS) + (size_t)c * CLASS_SIZE), slot, controller, unique);
    }
}

// Wraps the Creating World text, for the roll.
static void aiClassInit(void) {
    aiClassRandom = GAME_FN(int (*)(int), RVA_RANDOM);
    writeLog("aiclass_installed", "ok=%d", wrapSite(SITE_NEW_WORLD_CREATING_WORLD, (void *)aiClassCreatingWorldHook, (void **)&nextAiClassCreatingWorld));
}
