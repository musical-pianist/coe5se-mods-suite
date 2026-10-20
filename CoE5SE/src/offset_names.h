// Names where the game wants an offset.
#pragma once

#define MONSTER_ABILITY_GROWHP     1
#define MONSTER_ABILITY_LOOKSLIKE  0x1e
#define FIND_MONSTER_MATCHES_MAX   100
#define OFFSET_NAME_TEXT_MAX       0x80
#define OFFSET_NAME_LINE_MAX       1024
#define GAINRIT_WORD               "gainrit"

// The game's words that take a monster offset.
static const char *const MONSTER_OFFSET_WORDS[] = {
    "spawnoffs",    "growoffs",       "lookslike",  "landshape",     "watershape",  "elysiumshape", "celestialshape", "skyshape",
    "agarthashape", "infernoshape",   "hadesshape", "aztlanshape",   "primalshape", "elementalshape", "voidshape",
};

static char offsetNameLine[OFFSET_NAME_LINE_MAX];

static int isMonsterOffsetWord(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof MONSTER_OFFSET_WORDS / sizeof MONSTER_OFFSET_WORDS[0]; i++)
        if (spellWordIs(line, len, MONSTER_OFFSET_WORDS[i])) return 1;
    return 0;
}

// A monster's value of an ability.
static int monsterAbility(int monster, int ability) {
    if (monster < 0 || monster >= MONSTER_TABLE_MAX) return 0;
    const int *pairs = (const int *)(gameBase + RVA_MONSTER_TABLE + (size_t)monster * MONSTER_SIZE + MONSTER_ABILITIES);
    for (int i = 0; i < MONSTER_ABILITY_PAIRS && pairs[2 * i]; i++)
        if (pairs[2 * i] == ability) return pairs[2 * i + 1];
    return 0;
}

// Whether record name `record` is `name`, ignoring case.
static int recordNameIs(const unsigned char *record, size_t size, const char *name) {
    return strlen(name) < size && !_strnicmp((const char *)record, name, size);
}

// The monster the game's find_monster(text, 0) finds.
static int monsterByName(const char *text) {
    int nth = -1;
    const char *name = text;
    if (isdigit((unsigned char)text[0]) && isdigit((unsigned char)text[1]) && text[2] == ':') {
        nth = (text[0] - '0') * 10 + (text[1] - '0');
        name = text + 3;
    } else if (isdigit((unsigned char)text[0]) && text[1] == ':') {
        nth = text[0] - '0';
        name = text + 2;
    }
    int found[FIND_MONSTER_MATCHES_MAX], count = 0;
    for (int pass = 0; pass < 2 && !count; pass++) {
        for (int m = 0; m < MONSTER_TABLE_MAX && count < FIND_MONSTER_MATCHES_MAX; m++) {
            if (G32(RVA_MONSTER_SPRITES + (size_t)m * MONSTER_SIZE) == -1) break;
            if (!recordNameIs(gameBase + RVA_MONSTER_TABLE + (size_t)m * MONSTER_SIZE, MONSTER_NAME_SIZE, name)) continue;
            if (pass == 0 && monsterAbility(m, MONSTER_ABILITY_GROWHP)) continue;
            if (monsterAbility(m - 1, MONSTER_ABILITY_LOOKSLIKE) == 1 || monsterAbility(m - 2, MONSTER_ABILITY_LOOKSLIKE) == 2) continue;
            found[count++] = m;
        }
    }
    if (!count) {
        if (!strcmp(text, "Golden Wizard")) return monsterByName("Wizard of the Golden Dawn");
        if (!strcmp(text, "Master Arch Wizard")) return monsterByName("Arch Wizard of the Golden Dawn");
        return -1;
    }
    if (nth < 0) return found[0];
    return nth < count ? found[nth] : -1;
}

// The slot selectritual's lookup finds.
static int ritualByName(const char *name, int nth) {
    int last = G32(RVA_RITUAL_COUNT);
    if (last >= ritualSlotCount()) last = ritualSlotCount() - 1;
    for (int r = 0; r <= last; r++)
        if (recordNameIs(gameBase + ritualTableRva() + (size_t)r * RITUAL_SIZE, RITUAL_NAME_SIZE, name) && nth-- == 0) return r;
    return -1;
}

// Where a quoted name starts in a monster word's arguments, or -1.
static int monsterNamePrefix(const char *args) {
    if (args[0] == '"') return 0;
    if (isdigit((unsigned char)args[0]) && args[1] == ':' && args[2] == '"') return 2;
    if (isdigit((unsigned char)args[0]) && isdigit((unsigned char)args[1]) && args[2] == ':' && args[3] == '"') return 3;
    return -1;
}

// Refuses the line, for offsetNameLineHook.
static PRINTF_FORMAT(4, 5) const char *refuseOffsetName(const char *word, int mod, const char *reason, const char *fmt, ...) {
    char details[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(details, sizeof details, fmt, ap);
    va_end(ap);
    refuseC5mLine(word, spellModName(mod), reason, "line=%d %s", G32(RVA_MOD_LINE), details);
    return NULL;
}

// Every .c5m and .c5se line outside events comes here after spellPathLineHook.
static const char *offsetNameLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2) return line;
    int ritualWord = spellWordIs(line, len, GAINRIT_WORD);
    if (!ritualWord && !isMonsterOffsetWord(line, len)) return line;
    const char *args = line + len;
    skipSpaces(&args);
    int prefix = ritualWord ? (args[0] == '"' ? 0 : -1) : monsterNamePrefix(args);
    if (prefix < 0) return line;
    char word[32], text[OFFSET_NAME_TEXT_MAX];
    snprintf(word, sizeof word, "%.*s", (int)(len < sizeof word ? len : sizeof word - 1), line);
    memcpy(text, args, (size_t)prefix);
    const char *rest = args + prefix;
    int nameLen = readSpellQuoted(&rest, text + prefix, sizeof text - (size_t)prefix);
    size_t textMax = ritualWord ? RITUAL_NAME_SIZE - 1 : MONSTER_NAME_SIZE - 1 + 3;
    if (nameLen <= 0) return refuseOffsetName(word, mod, "no_name", "text=%.60s", args);
    if ((size_t)nameLen > textMax) return refuseOffsetName(word, mod, "name_too_long", "text=%.60s", args);

    int current, target, nth = 0;
    const char *kind = ritualWord ? "ritual" : "monster";
    if (ritualWord) {
        long long n;
        const char *p = rest;
        if (readSpellNumber(&p, &n)) {
            nth = n < 0 ? 0 : n > INT_MAX ? INT_MAX : (int)n;
            rest = p;
        }
        current = G32(RVA_RITUAL_CURRENT);
        if (current < 0) return refuseOffsetName(word, mod, "no_ritual", "name=%s", text);
        target = ritualByName(text, nth);
        if (target < 0) return refuseOffsetName(word, mod, "unknown_ritual", "name=%s nth=%d", text, nth);
        if (target == current) return refuseOffsetName(word, mod, "same_ritual", "name=%s", text);
    } else {
        current = G32(RVA_MONSTER_CURRENT);
        if (current < 0) return refuseOffsetName(word, mod, "no_monster", "name=%s", text);
        target = monsterByName(text);
        if (target < 0) return refuseOffsetName(word, mod, "unknown_monster", "name=%s", text);
        if (target == current) return refuseOffsetName(word, mod, "same_monster", "name=%s", text);
    }
    int n = snprintf(offsetNameLine, sizeof offsetNameLine, "%s %d%s", word, target - current, rest);
    if (n < 0 || (size_t)n >= sizeof offsetNameLine) return refuseOffsetName(word, mod, "line_too_long", "name=%s", text);
    writeLog("offset_name_read", "mod=%s word=%s name=%s %s=%d current=%d offset=%d line=%d", spellModName(mod), word, text, kind,
             target, current, target - current, G32(RVA_MOD_LINE));
    return offsetNameLine;
}

// The same for declinedModCommandHook, when no plugin loaded.
static int declinedOffsetNameLine(const char *line, size_t len, int level) {
    if (level != 2) return 0;
    int ritualWord = spellWordIs(line, len, GAINRIT_WORD);
    if (!ritualWord && !isMonsterOffsetWord(line, len)) return 0;
    const char *args = line + len;
    skipSpaces(&args);
    return ritualWord ? args[0] == '"' : monsterNamePrefix(args) >= 0;
}
