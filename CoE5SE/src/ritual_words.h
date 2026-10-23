// Ritual effect words: CoE5SE's words for effects the game's rituals carry but no game word sets.
#pragma once

#define RITUAL_FX_RANDOM_EQUIP        1093
#define RITUAL_FX_RANDOM_EQUIP_VALUE  1
#define RANDOMEQUIP_WORD              "randomequip"
#define RITUAL_STRING_PICK            '|'
#define RITUAL_FX_MAKE_ITEM           1033
#define RANDARTIFACT_WORD             "randartifact"
#define ITEM_GROUP_MASK_MAX           31

// The item groups' names randartifact takes, each for its bit.
static const struct {
    const char *name;
    int bit;
} ITEM_GROUP_NAMES[] = {{"fire", 1}, {"water", 2}, {"air", 4}, {"earth", 8}, {"books", 16}};

// One randomequip line of this pass: its ritual, to check once every mod has added its strings.
typedef struct {
    int record, line;
    char mod[64];
} RandomEquipLine;

static RandomEquipLine *randomEquipLines;
static int randomEquipLineCount, randomEquipLineCapacity;

static unsigned char *ritualWordRecord(int record) {
    return gameBase + ritualTableRva() + (size_t)record * RITUAL_SIZE;
}

// The effect at `slot` of the ritual record: its id, then its int64 value.
static long long *ritualWordEffect(unsigned char *record, int slot) {
    return (long long *)(record + RITUAL_FX + (size_t)slot * RITUAL_FX_SIZE);
}

// Adds the effect to the ritual, as the game's words do.
static int addRitualWordEffect(int record, int id, long long value) {
    unsigned char *r = ritualWordRecord(record);
    for (int s = 0; s < RITUAL_FX_MAX; s++) {
        long long *fx = ritualWordEffect(r, s);
        if (fx[0]) continue;
        fx[0] = id;
        fx[1] = value;
        if (s + 1 < RITUAL_FX_MAX) ritualWordEffect(r, s + 1)[0] = 0;
        return 1;
    }
    return 0;
}

// Takes every effect `id` off the ritual, moving the later ones up.
static int dropRitualWordEffect(int record, int id) {
    unsigned char *r = ritualWordRecord(record);
    int kept = 0, dropped = 0;
    for (int s = 0; s < RITUAL_FX_MAX; s++) {
        long long *fx = ritualWordEffect(r, s);
        if (!fx[0]) break;
        if (fx[0] == id) {
            dropped++;
            continue;
        }
        if (kept != s) memcpy(ritualWordEffect(r, kept), fx, RITUAL_FX_SIZE);
        kept++;
    }
    for (int s = kept; s < kept + dropped && s < RITUAL_FX_MAX; s++) memset(ritualWordEffect(r, s), 0, RITUAL_FX_SIZE);
    return dropped;
}

// What is wrong with the ritual's "|" lines for randomequip, or NULL.
static const char *randomEquipProblem(int record, int *at) {
    const char *strings = (const char *)ritualWordRecord(record) + RITUAL_STRINGS;
    int picks = 0, firstOther = -1;
    *at = -1;
    for (int i = 0; i < RITUAL_STRINGS_MAX; i++) {
        const char *s = strings + (size_t)i * RITUAL_STRING_SIZE;
        int pick = s[0] == RITUAL_STRING_PICK && s[1];
        if (!pick) {
            if (firstOther < 0) firstOther = i;
            continue;
        }
        if (firstOther >= 0) {
            *at = firstOther;
            return "pick_lines_not_first";
        }
        picks++;
    }
    return picks ? NULL : "no_pick_lines";
}

// randomequip: adds the effect to the current ritual, and keeps the line to check after the pass.
static void randomequipLine(const char *args, int mod, int level) {
    if (level != 2) return;
    const char *p = args;
    skipSpaces(&p);
    if (!isLineEnd(p)) {
        refuseC5mLine(RANDOMEQUIP_WORD, spellModName(mod), "bad_value", "line=%d text=%.40s", G32(RVA_MOD_LINE), args);
        return;
    }
    int record = G32(RVA_RITUAL_CURRENT);
    if (record < 0 || record >= ritualSlotCount()) {
        refuseC5mLine(RANDOMEQUIP_WORD, spellModName(mod), "no_ritual", "line=%d", G32(RVA_MOD_LINE));
        return;
    }
    if (!growList((void **)&randomEquipLines, &randomEquipLineCapacity, randomEquipLineCount + 1, sizeof randomEquipLines[0])) {
        refuseC5mLine(RANDOMEQUIP_WORD, spellModName(mod), "out_of_memory", "line=%d", G32(RVA_MOD_LINE));
        return;
    }
    if (!addRitualWordEffect(record, RITUAL_FX_RANDOM_EQUIP, RITUAL_FX_RANDOM_EQUIP_VALUE)) {
        refuseC5mLine(RANDOMEQUIP_WORD, spellModName(mod), "too_many_effects", "line=%d ritual=%.48s max=%d", G32(RVA_MOD_LINE),
                      (const char *)ritualWordRecord(record), RITUAL_FX_MAX);
        return;
    }
    RandomEquipLine *e = &randomEquipLines[randomEquipLineCount++];
    e->record = record;
    e->line = G32(RVA_MOD_LINE);
    snprintf(e->mod, sizeof e->mod, "%s", spellModName(mod));
}

// Reads one item group's name from *p into *out, its bit, and moves past it; 0 for any other text.
static int readItemGroupName(const char **p, long long *out) {
    size_t len = strcspn(*p, " \t\r\n");
    for (size_t i = 0; i < sizeof ITEM_GROUP_NAMES / sizeof ITEM_GROUP_NAMES[0]; i++) {
        if (strlen(ITEM_GROUP_NAMES[i].name) != len || strncmp(ITEM_GROUP_NAMES[i].name, *p, len)) continue;
        *out = ITEM_GROUP_NAMES[i].bit;
        *p += len;
        return 1;
    }
    return 0;
}

// randartifact: adds the effect to the current ritual, with the item group number, a group's bit, or 0.
static void randartifactLine(const char *args, int mod, int level) {
    if (level != 2) return;
    const char *p = args;
    long long groups = 0;
    skipSpaces(&p);
    if (!isLineEnd(p) && (!(readItemGroupName(&p, &groups) || readSpellNumber(&p, &groups)) || (skipSpaces(&p), !isLineEnd(p)) || groups < 0 || groups > ITEM_GROUP_MASK_MAX)) {
        refuseC5mLine(RANDARTIFACT_WORD, spellModName(mod), "bad_value", "line=%d text=%.40s min=0 max=%d", G32(RVA_MOD_LINE), args,
                      ITEM_GROUP_MASK_MAX);
        return;
    }
    int record = G32(RVA_RITUAL_CURRENT);
    if (record < 0 || record >= ritualSlotCount()) {
        refuseC5mLine(RANDARTIFACT_WORD, spellModName(mod), "no_ritual", "line=%d", G32(RVA_MOD_LINE));
        return;
    }
    if (!addRitualWordEffect(record, RITUAL_FX_MAKE_ITEM, groups))
        refuseC5mLine(RANDARTIFACT_WORD, spellModName(mod), "too_many_effects", "line=%d ritual=%.48s max=%d", G32(RVA_MOD_LINE),
                      (const char *)ritualWordRecord(record), RITUAL_FX_MAX);
}

// After a level-2 mod pass: takes randomequip off each ritual whose "|" lines don't come first.
static void ritualWordsAfterPass(int level) {
    if (level != 2) return;
    for (int i = 0; i < randomEquipLineCount; i++) {
        const RandomEquipLine *e = &randomEquipLines[i];
        int at;
        const char *problem = randomEquipProblem(e->record, &at);
        if (!problem) continue;
        if (!dropRitualWordEffect(e->record, RITUAL_FX_RANDOM_EQUIP)) continue;
        const char *name = (const char *)ritualWordRecord(e->record);
        if (at >= 0)
            refuseC5mLine(RANDOMEQUIP_WORD, e->mod, problem, "line=%d ritual=%.48s string=%d text=%.60s", e->line, name, at + 1,
                          name + RITUAL_STRINGS + (size_t)at * RITUAL_STRING_SIZE);
        else refuseC5mLine(RANDOMEQUIP_WORD, e->mod, problem, "line=%d ritual=%.48s", e->line, name);
    }
    randomEquipLineCount = 0;
}

// Adds randomequip and randartifact.
static void ritualWordsInit(void) {
    int words = apiAddModCommand("randomequip", randomequipLine) + apiAddModCommand("randartifact", randartifactLine);
    writeLog("ritual_words_installed", "words=%d of 2", words);
}
