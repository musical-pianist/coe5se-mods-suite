// Mods' own spells: the .c5m words newspell, path, level, weapon, and the spell flags.
#include <limits.h>

#define NEWSPELL_WORD         "newspell"
#define SPELL_TABLE_SLOTS     SPELL_TABLE_RECORDS_GAME
#define MOD_SPELL_CAP         (SPELL_TABLE_SLOTS - 1 - VANILLA_SPELL_COUNT)
#define MOD_SPELL_CAP_MAX     (SPELL_TABLE_RECORDS_MOVED - 1 - VANILLA_SPELL_COUNT)
_Static_assert(MOD_SPELL_CAP_MAX == MOD_SPELL_TARGET, "the moved table has room for exactly the target");
#define SPELL_END_PATH        (-999)
#define SPELL_NAME_MAX        47
#define SPELL_LEVEL_MAX       3
#define WEAPON_NAME_MAX       63

// The game's flags, one bit each.
#define SPELL_FLAG_BURNFOREST      0x01ULL
#define SPELL_FLAG_PRIORITYLEARN   0x02ULL
#define SPELL_FLAG_SLOWCAST        0x04ULL
#define SPELL_FLAG_NOTSEA          0x08ULL
#define SPELL_FLAG_HEALLIVINGARMY  0x10ULL
#define SPELL_FLAG_HEALHUMANARMY   0x20ULL
#define SPELL_FLAG_NOTUNDERGROUND  0x40ULL

// One record of the game's spell table.
typedef struct {
    const char *name;
    int path, level;
    int weapon;
    int unused;
    long long dmg;
    unsigned long long flags;
} SpellRecord;
_Static_assert(sizeof(SpellRecord) == SPELL_SIZE, "a spell record is SPELL_SIZE bytes");

// The game's words that start a block.
static const char *const SPELL_BLOCK_ENDERS[] = {
    "newmonster", "selectmonster", "newclass", "selectclass", "newweapon", "selectweapon", "newritpow",
    "newritual", "selectritual", "newitem", "selectitem", "selectterr", "selectterrgroup",
    "newspellpath", "selectspellpath",
    SETTING_WORD, SETTING_PAGE_WORD, SETTING_HEADER_WORD,
};

static void resetSpellPaths(const char *reason);
static int spellPathByName(const char *name);

static struct {
    int state;
    SpellRecord shippedEnd;
    int modSpells;
    int spellMods[MOD_SPELL_CAP_MAX];
    int writable;
    int lastLevel, lastMod;
    unsigned int passes;
    char **names;
    int nameCount, nameCapacity;
    int *nameHash;
    int nameHashSize;
} spellTable = {.lastMod = -1};

// The mods' spells the table has room for now.
static int modSpellCap(void) { return spellTableRecords() - 1 - VANILLA_SPELL_COUNT; }

// The open newspell block.
static struct {
    int open;
    int refused;
    int mod, line;
    int slot;
    int hasPath, hasLevel, hasWeapon;
    int badPath, badLevel, badWeapon;
    char name[SPELL_NAME_MAX + 1];
    SpellRecord record;
} spellBlock;

// Every spell line the latest mod pass refused.
#define SPELL_PROBLEM_TEXT  140
#define SPELL_FILE_C5M      ".c5m"
#define SPELL_FILE_SIDECAR  SIDECAR_SUFFIX
typedef struct {
    char mod[MOD_NAME_MAX];
    int line;
    const char *file;
    char text[SPELL_PROBLEM_TEXT];
    int slotRun;
    char spell[SPELL_NAME_MAX + 1];
} SpellProblem;
static SpellProblem *spellProblems;
static int spellProblemCount, spellProblemCapacity;

static const char *spellModName(int mod);

// Adds a refused line of mod `mod` to the list.
static PRINTF_FORMAT(3, 4) void noteSpellProblem(int mod, int line, const char *fmt, ...) {
    if (mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return;
    if (!growList((void **)&spellProblems, &spellProblemCapacity, spellProblemCount + 1, sizeof *spellProblems))
        return;
    SpellProblem *p = &spellProblems[spellProblemCount++];
    snprintf(p->mod, sizeof p->mod, "%s", spellModName(mod));
    p->line = line;
    p->file = readingSidecarOf(mod) ? SPELL_FILE_SIDECAR : SPELL_FILE_C5M;
    p->slotRun = 0;
    p->spell[0] = '\0';
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(p->text, sizeof p->text, fmt, ap);
    va_end(ap);
}

// A spell with no free slot.
static void noteNoFreeSlot(int mod, int line, const char *name) {
    SpellProblem *last = spellProblemCount ? &spellProblems[spellProblemCount - 1] : NULL;
    if (last && last->slotRun && !strcmp(last->mod, spellModName(mod))) {
        last->slotRun++;
        snprintf(last->text, sizeof last->text, "no free slot for %s and the %d after it (the most is %d)", last->spell,
                 last->slotRun - 1, modSpellCap());
        return;
    }
    int before = spellProblemCount;
    noteSpellProblem(mod, line, "no free slot for %s (the most is %d)", name, modSpellCap());
    if (spellProblemCount == before) return;
    last = &spellProblems[spellProblemCount - 1];
    last->slotRun = 1;
    snprintf(last->spell, sizeof last->spell, "%s", name);
}

// Finds a weapon by name the way the game's meleeweapon does, or -1.
static int (*spellFindWeapon)(const char *name);

static SpellRecord *spellSlot(int slot) {
    return (SpellRecord *)spellTableAddress() + slot;
}

// Whether the table past the vanilla spells is as the game ships it.
static int shippedSpellTableReady(void) {
    if (spellTable.state) return spellTable.state > 0;
    static const SpellRecord empty;
    const SpellRecord *end = spellSlot(VANILLA_SPELL_COUNT);
    int ok = spellSlot(VANILLA_SPELL_COUNT - 1)->path != SPELL_END_PATH && end->path == SPELL_END_PATH;
    int bad = ok ? 0 : VANILLA_SPELL_COUNT;
    for (int s = VANILLA_SPELL_COUNT + 1; ok && s < spellTableRecords(); s++)
        if (memcmp(spellSlot(s), &empty, sizeof empty)) ok = 0, bad = s;
    if (!ok) {
        spellTable.state = -1;
        writeLog("spell_table_unexpected", "slot=%d end_path=%d", bad, end->path);
        return 0;
    }
    spellTable.shippedEnd = *end;
    spellTable.state = 1;
    writeLog("spell_table_ready", "records=%d moved=%d mod_spell_cap=%d last_slot=%d", spellTableRecords(), spellTableMoved, modSpellCap(),
             VANILLA_SPELL_COUNT + modSpellCap() - 1);
    return 1;
}

// Whether CoE5SE can write the records past the vanilla spells.
static int spellTableWritable(void) {
#ifdef __APPLE__
    if (spellTableMoved) return 1;
    if (!spellTable.writable)
        spellTable.writable = makeGameDataWritable(spellSlot(VANILLA_SPELL_COUNT), (size_t)(SPELL_TABLE_SLOTS - VANILLA_SPELL_COUNT) * sizeof(SpellRecord))
                              ? 1 : -1;
    return spellTable.writable > 0;
#else
    return 1;
#endif
}

static unsigned int spellNameHash(const char *name) {
    unsigned int hash = 2166136261u;
    for (; *name; name++) hash = (hash ^ (unsigned char)*name) * 16777619u;
    return hash;
}

// Puts names[index] into the hash, growing it to stay at most half full.
static int hashSpellName(int index) {
    if ((spellTable.nameCount + 1) * 2 > spellTable.nameHashSize) {
        int size = spellTable.nameHashSize ? spellTable.nameHashSize * 2 : 1024;
        int *grown = calloc((size_t)size, sizeof *grown);
        if (!grown) return 0;
        for (int i = 0; i < spellTable.nameHashSize; i++) {
            if (!spellTable.nameHash[i]) continue;
            unsigned int at = spellNameHash(spellTable.names[spellTable.nameHash[i] - 1]) & (unsigned)(size - 1);
            while (grown[at]) at = (at + 1) & (unsigned)(size - 1);
            grown[at] = spellTable.nameHash[i];
        }
        free(spellTable.nameHash);
        spellTable.nameHash = grown;
        spellTable.nameHashSize = size;
    }
    unsigned int at = spellNameHash(spellTable.names[index]) & (unsigned)(spellTable.nameHashSize - 1);
    while (spellTable.nameHash[at]) at = (at + 1) & (unsigned)(spellTable.nameHashSize - 1);
    spellTable.nameHash[at] = index + 1;
    return 1;
}

// The launch's copy of `name`.
static const char *keptSpellName(const char *name) {
    if (spellTable.nameHashSize) {
        unsigned int at = spellNameHash(name) & (unsigned)(spellTable.nameHashSize - 1);
        for (; spellTable.nameHash[at]; at = (at + 1) & (unsigned)(spellTable.nameHashSize - 1))
            if (!strcmp(spellTable.names[spellTable.nameHash[at] - 1], name)) return spellTable.names[spellTable.nameHash[at] - 1];
    }
    if (!growList((void **)&spellTable.names, &spellTable.nameCapacity, spellTable.nameCount + 1, sizeof *spellTable.names))
        return NULL;
    char *copy = malloc(strlen(name) + 1);
    if (!copy) return NULL;
    strcpy(copy, name);
    spellTable.names[spellTable.nameCount] = copy;
    if (!hashSpellName(spellTable.nameCount)) {
        free(copy);
        return NULL;
    }
    return spellTable.names[spellTable.nameCount++];
}

static const char *spellModName(int mod) {
    return mod >= 0 && mod <= G32(RVA_MOD_LAST_INDEX) ? modName(mod) : "-";
}

// Ends the open block.
static void finishSpellBlock(void) {
    if (!spellBlock.open) return;
    spellBlock.open = 0;
    if (spellBlock.refused || spellBlock.slot) return;
    const char *missing = !spellBlock.hasPath ? "no_path" : !spellBlock.hasLevel ? "no_level" : "no_weapon";
    refuseC5mLine(NEWSPELL_WORD, spellModName(spellBlock.mod), missing, "line=%d name=%s", spellBlock.line, spellBlock.name);
    int causeListed = !spellBlock.hasPath ? spellBlock.badPath : !spellBlock.hasLevel ? spellBlock.badLevel : spellBlock.badWeapon;
    if (!causeListed)
        noteSpellProblem(spellBlock.mod, spellBlock.line, "no %s line in %s",
                         !spellBlock.hasPath ? "path" : !spellBlock.hasLevel ? "level" : "weapon", spellBlock.name);
}

// Puts back the records past the vanilla spells as the game ships them, removing every mod's spell.
static void restoreShippedSpells(const char *reason) {
    finishSpellBlock();
    resetSpellPaths(reason);
    spellProblemCount = 0;
    if (!spellTable.modSpells) return;
    *spellSlot(VANILLA_SPELL_COUNT) = spellTable.shippedEnd;
    memset(spellSlot(VANILLA_SPELL_COUNT + 1), 0, (size_t)spellTable.modSpells * sizeof(SpellRecord));
    writeLog("spells_reset", "removed=%d reason=%s", spellTable.modSpells, reason);
    spellTable.modSpells = 0;
}

// Writes the open block into the table.
static void storeSpell(void) {
    if (spellBlock.slot) {
        *spellSlot(spellBlock.slot) = spellBlock.record;
        return;
    }
    if (!spellBlock.hasPath || !spellBlock.hasLevel || !spellBlock.hasWeapon) return;
    const char *mod = spellModName(spellBlock.mod);
    const char *name = keptSpellName(spellBlock.name);
    const char *reason = !name ? "out_of_memory" : !spellTableWritable() ? "table_not_writable" : NULL;
    if (reason) {
        spellBlock.refused = 1;
        refuseC5mLine(NEWSPELL_WORD, mod, reason, "line=%d name=%s", spellBlock.line, spellBlock.name);
        noteSpellProblem(spellBlock.mod, spellBlock.line, "CoE5SE couldn't add %s to the table", spellBlock.name);
        return;
    }
    int slot = VANILLA_SPELL_COUNT + spellTable.modSpells;
    spellBlock.record.name = name;
    *spellSlot(slot + 1) = spellTable.shippedEnd;
    *spellSlot(slot) = spellBlock.record;
    spellTable.spellMods[spellTable.modSpells++] = spellBlock.mod;
    spellBlock.slot = slot;
    writeLog("spell_added", "mod=%s slot=%d name=%s path=%d level=%d weapon=%d dmg=%lld flags=0x%llx line=%d", mod, slot,
             name, spellBlock.record.path, spellBlock.record.level, spellBlock.record.weapon, spellBlock.record.dmg,
             spellBlock.record.flags, spellBlock.line);
}

// ---- Reading the lines ---------------------------------------------------------------------------
// Reads a whole number from *p and moves past it.
static int readSpellNumber(const char **p, long long *out) {
    skipSpaces(p);
    char *end;
    long long v = strtoll(*p, &end, 10);
    if (end == *p || (*end && *end != ' ' && *end != '\t' && *end != '"' && *end != '\r' && *end != '\n')) return 0;
    *out = v;
    *p = end;
    return 1;
}

// Reads "quoted text" from *p into `out` (cut to fit) and moves past it.
static int readSpellQuoted(const char **p, char *out, size_t size) {
    skipSpaces(p);
    if (**p != '"') return -1;
    const char *start = *p + 1, *end = strchr(start, '"');
    if (!end) return -1;
    size_t len = (size_t)(end - start);
    snprintf(out, size, "%.*s", (int)(len < size ? len : size - 1), start);
    *p = end + 1;
    return len > INT_MAX ? INT_MAX : (int)len;
}

static int spellWordIs(const char *line, size_t len, const char *word) {
    return strlen(word) == len && !strncmp(line, word, len);
}

static int isSpellBlockEnder(const char *line, size_t len) {
    for (size_t i = 0; i < sizeof SPELL_BLOCK_ENDERS / sizeof SPELL_BLOCK_ENDERS[0]; i++)
        if (spellWordIs(line, len, SPELL_BLOCK_ENDERS[i])) return 1;
    return 0;
}

// Whether a line of a spell block's word applies.
static int spellBlockTakes(const char *word, int mod, int level) {
    if (level != 2) return 0;
    if (!spellBlock.open) {
        refuseC5mLine(word, spellModName(mod), "no_spell_block", "line=%d", G32(RVA_MOD_LINE));
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "%s outside a newspell block", word);
        return 0;
    }
    return !spellBlock.refused;
}

// Ends the parser's current records, as the game's own new and select words do.
static void clearParserCurrentRecords(void) {
    static const unsigned int CURRENT[] = {RVA_MONSTER_CURRENT, RVA_WEAPON_CURRENT, RVA_RITUAL_CURRENT, RVA_ITEM_CURRENT,
                                           RVA_CLASS_CURRENT, RVA_TERRAIN_CURRENT, RVA_TERRAIN_GROUP_CURRENT};
    for (size_t i = 0; i < sizeof CURRENT / sizeof CURRENT[0]; i++) G32(CURRENT[i]) = -1;
}

// Why a newspell line of name length `len` can't open a block, or NULL.
static const char *newspellRefusal(int mod, int len) {
    if (mod < 0 || mod > G32(RVA_MOD_LAST_INDEX)) return "not_in_a_mod";
    if (len <= 0) return "no_name";
    if (len > SPELL_NAME_MAX) return "name_too_long";
    if (!shippedSpellTableReady()) return "table_unexpected";
    if (spellTable.modSpells >= modSpellCap()) return "no_free_slot";
    return NULL;
}

// newspell "<name>": opens a spell block, and ends the parser's current records.
static void newspellLine(const char *args, int mod, int level) {
    if (level != 2) return;
    finishSpellBlock();
    clearParserCurrentRecords();
    memset(&spellBlock, 0, sizeof spellBlock);
    spellBlock.open = 1;
    spellBlock.mod = mod;
    spellBlock.line = G32(RVA_MOD_LINE);
    const char *p = args;
    int len = readSpellQuoted(&p, spellBlock.name, sizeof spellBlock.name);
    const char *reason = newspellRefusal(mod, len);
    if (reason) {
        spellBlock.refused = 1;
        refuseC5mLine(NEWSPELL_WORD, spellModName(mod), reason, "line=%d text=%.80s spells=%d cap=%d", spellBlock.line, args,
                      spellTable.modSpells, modSpellCap());
        if (!strcmp(reason, "no_free_slot"))
            noteNoFreeSlot(mod, spellBlock.line, spellBlock.name);
        else if (!strcmp(reason, "table_unexpected"))
            noteSpellProblem(mod, spellBlock.line, "the game's spell table isn't as CoE5SE expects");
        else if (!strcmp(reason, "name_too_long"))
            noteSpellProblem(mod, spellBlock.line, "a spell name longer than the limit of %d", SPELL_NAME_MAX);
        else
            noteSpellProblem(mod, spellBlock.line, "newspell needs a name in quotes");
    }
}

// path <nbr> or path "<name>": the spell's magic path.
static void pathLine(const char *args, int mod, int level) {
    if (!spellBlockTakes("path", mod, level)) return;
    long long path;
    const char *p = args;
    char name[SPELL_NAME_MAX + 1];
    skipSpaces(&p);
    if (*p == '"') {
        int found = readSpellQuoted(&p, name, sizeof name) > 0 ? spellPathByName(name) : -1;
        if (found < 0) {
            refuseC5mLine("path", spellModName(mod), "unknown_path", "line=%d text=%.40s", G32(RVA_MOD_LINE), args);
            noteSpellProblem(mod, G32(RVA_MOD_LINE), "no spell path named %.40s", name);
            spellBlock.badPath = 1;
            return;
        }
        spellBlock.record.path = found;
        spellBlock.hasPath = 1;
        storeSpell();
        return;
    }
    if (!readSpellNumber(&p, &path) || path < 0 || path >= spellPathCount()) {
        refuseC5mLine("path", spellModName(mod), "bad_path", "line=%d text=%.40s", G32(RVA_MOD_LINE), args);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "path must be a number from 0 to %d", spellPathCount() - 1);
        spellBlock.badPath = 1;
        return;
    }
    spellBlock.record.path = (int)path;
    spellBlock.hasPath = 1;
    storeSpell();
}

// level <1-3>, inside a spell block.
static void spellLevelLine(const char *args, int mod, int level) {
    if (!spellBlockTakes("level", mod, level)) return;
    long long spellLevel;
    const char *p = args;
    if (!readSpellNumber(&p, &spellLevel) || spellLevel < 1 || spellLevel > SPELL_LEVEL_MAX) {
        refuseC5mLine("level", spellModName(mod), "bad_level", "line=%d text=%.40s", G32(RVA_MOD_LINE), args);
        noteSpellProblem(mod, G32(RVA_MOD_LINE), "level must be 1, 2 or 3");
        spellBlock.badLevel = 1;
        return;
    }
    spellBlock.record.level = (int)spellLevel;
    spellBlock.hasLevel = 1;
    storeSpell();
}

// weapon <dmg> "<weapon>" or weapon <dmg> <nbr>: the spell's effect.
static void weaponLine(const char *args, int mod, int level) {
    if (!spellBlockTakes("weapon", mod, level)) return;
    long long dmg, number = 0;
    char name[WEAPON_NAME_MAX + 1] = "";
    const char *p = args;
    int weapon = -1, len = 0;
    const char *reason = NULL;
    if (!readSpellNumber(&p, &dmg)) reason = "bad_weapon";
    else if ((len = readSpellQuoted(&p, name, sizeof name)) >= 0) {
        if (!len) reason = "bad_weapon";
        else if (len > WEAPON_NAME_MAX || (weapon = spellFindWeapon(name)) < 0) reason = "unknown_weapon";
    } else if (readSpellNumber(&p, &number)) {
        if (number < 0 || number >= tableRecords(TABLE_WEAPON) || !*(const char *)(tableAddress(TABLE_WEAPON) + (size_t)number * WEAPON_SIZE))
            reason = "unknown_weapon";
        else
            weapon = (int)number;
    } else {
        reason = "bad_weapon";
    }
    if (reason) {
        refuseC5mLine("weapon", spellModName(mod), reason, "line=%d text=%.80s", G32(RVA_MOD_LINE), args);
        if (!strcmp(reason, "bad_weapon"))
            noteSpellProblem(mod, G32(RVA_MOD_LINE), "weapon needs a damage number, then a weapon");
        else if (name[0])
            noteSpellProblem(mod, G32(RVA_MOD_LINE), "no weapon named %.40s", name);
        else
            noteSpellProblem(mod, G32(RVA_MOD_LINE), "no weapon number %lld", number);
        spellBlock.badWeapon = 1;
        return;
    }
    spellBlock.record.weapon = weapon;
    spellBlock.record.dmg = dmg;
    spellBlock.hasWeapon = 1;
    storeSpell();
}

// The flag words take nothing after them, like the game's own flag words.
static void spellFlagLine(const char *word, unsigned long long bit, int mod, int level) {
    if (!spellBlockTakes(word, mod, level)) return;
    spellBlock.record.flags |= bit;
    storeSpell();
}

static void slowcastLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("slowcast", SPELL_FLAG_SLOWCAST, mod, level);
}
static void notseaLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("notsea", SPELL_FLAG_NOTSEA, mod, level);
}
static void notundergroundLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("notunderground", SPELL_FLAG_NOTUNDERGROUND, mod, level);
}
static void prioritylearnLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("prioritylearn", SPELL_FLAG_PRIORITYLEARN, mod, level);
}
static void heallivingarmyLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("heallivingarmy", SPELL_FLAG_HEALLIVINGARMY, mod, level);
}
static void healhumanarmyLine(const char *args, int mod, int level) {
    (void)args;
    spellFlagLine("healhumanarmy", SPELL_FLAG_HEALHUMANARMY, mod, level);
}

// Every .c5m line outside events comes here first.
static int spellLineHook(const char *line, size_t len, int mod, int level) {
    if (level != spellTable.lastLevel || mod != spellTable.lastMod) finishSpellBlock();
    if (level == 2 && spellTable.lastLevel != 2) {
        spellTable.passes++;
        restoreShippedSpells("mod_pass");
    }
    spellTable.lastLevel = level;
    spellTable.lastMod = mod;
    if (!spellBlock.open) return 0;
    const char *args = line + len;
    while (*args == ' ' || *args == '\t') args++;
    if (spellWordIs(line, len, "level")) {
        spellLevelLine(args, mod, level);
        return 1;
    }
    if (spellWordIs(line, len, "burnforest")) {
        spellFlagLine("burnforest", SPELL_FLAG_BURNFOREST, mod, level);
        return 1;
    }
    if (isSpellBlockEnder(line, len)) finishSpellBlock();
    return 0;
}

// The same for declinedModCommandHook, when no plugin loaded.
static int declinedSpellLine(const char *word, int mod, int level) {
    static int inBlock, blockMod, blockLevel;
    size_t len = strlen(word);
    if (mod != blockMod || level != blockLevel) inBlock = 0;
    blockMod = mod;
    blockLevel = level;
    if (spellWordIs(word, len, NEWSPELL_WORD)) inBlock = 1;
    else if (isSpellBlockEnder(word, len)) inBlock = 0;
    return inBlock && (spellWordIs(word, len, "level") || spellWordIs(word, len, "burnforest"));
}

// The number of level-2 readings started, for spellsAfterPass.
static unsigned int spellPassCount(void) { return spellTable.passes; }

static void dropSpellsOfDisabledMods(void);

// After the Mods screen.
static void spellsAfterModsScreen(unsigned int passesBefore) {
#ifdef __APPLE__
    (void)passesBefore;
    dropSpellsOfDisabledMods();
#else
    spellsAfterPass(2, passesBefore);
#endif
}

// After a mod pass at `level`, or the Mods screen.
static void spellsAfterPass(int level, unsigned int passesBefore) {
    finishSpellBlock();
    if (level == 2 && spellTable.passes == passesBefore) restoreShippedSpells("no_mods");
}

// Before a game starts from the main menu: drops the spells of mods turned off.
static void dropSpellsOfDisabledMods(void) {
    finishSpellBlock();
    for (int i = 0; i < spellTable.modSpells; i++)
        if (!modEnabled(spellTable.spellMods[i])) {
            restoreShippedSpells("mod_off");
            return;
        }
}

// ---- Showing the lines left out ------------------------------------------------------------------
#define SPELL_PROBLEM_ROWS    10
#define SPELL_POPUP_LINES     16

// The CoE5SE page's "Spell lines left out" box, drawn only when a line was left out.
static void drawSpellProblems(void) {
    finishSpellBlock();
    if (!spellProblemCount) return;
    sectionBegin();
    sectionHeader("Spell lines left out");
    char row[256], tip[512], label[80];
    for (int i = 0; i < spellProblemCount && i < SPELL_PROBLEM_ROWS; i++) {
        const SpellProblem *p = &spellProblems[i];
        modDisplayName(p->mod, label, sizeof label);
        snprintf(row, sizeof row, "%.79s%s line %d: %s", p->mod, p->file, p->line, p->text);
        textButton(0, row);
        snprintf(tip, sizeof tip, "%s (%.79s%s), line %d: %s. CoE5SE left this line out, and every other line of the mod "
                 "loads. coe5mod.log has it as c5m_refused.", label, p->mod, p->file, p->line, p->text);
        tooltip(tip);
    }
    if (spellProblemCount > SPELL_PROBLEM_ROWS) {
        snprintf(row, sizeof row, "...and %d more. coe5mod.log has each as c5m_refused.", spellProblemCount - SPELL_PROBLEM_ROWS);
        textButton(0, row);
    }
    sectionEnd();
}

// The pop-up, between main menu frames, once for each new set of lines left out.
static void popupSpellProblems(void) {
    static unsigned int shownHash;
    static char text[POPUP_TEXT_SIZE];
    finishSpellBlock();
    if (!spellProblemCount) {
        shownHash = 0;
        return;
    }
    size_t n = 0;
    char label[80];
    const char *lastMod = "";
    appendText(text, sizeof text, &n, "Spell lines left out:\nOther lines loaded.\n");
    for (int i = 0; i < spellProblemCount && i < SPELL_POPUP_LINES; i++) {
        const SpellProblem *p = &spellProblems[i];
        if (strcmp(lastMod, p->mod)) {
            modDisplayName(p->mod, label, sizeof label);
            appendText(text, sizeof text, &n, "\n%s\n", label);
            lastMod = p->mod;
        }
        appendText(text, sizeof text, &n, "%s%s line %d: %s\n", p->mod, p->file, p->line, p->text);
    }
    if (spellProblemCount > SPELL_POPUP_LINES) appendText(text, sizeof text, &n, "...and %d more.\n", spellProblemCount - SPELL_POPUP_LINES);
    appendText(text, sizeof text, &n, "\nFull list: Mod Settings, CoE5SE.");
    unsigned int hash = 2166136261u;
    for (const char *c = text; *c; c++) hash = (hash ^ (unsigned char)*c) * 16777619u;
    if (hash == shownHash) return;
    shownHash = hash;
    writeLog("spells_left_out_popup", "lines=%d", spellProblemCount);
    okBox("CoE5SE: Spells left out", text);
}

// Adds the words and the weapon lookup.
static void spellWordsInit(void) {
    spellFindWeapon = GAME_FN(int (*)(const char *), RVA_FIND_WEAPON);
    int words = apiAddModCommand("newspell", newspellLine) + apiAddModCommand("path", pathLine) +
                apiAddModCommand("weapon", weaponLine) + apiAddModCommand("slowcast", slowcastLine) +
                apiAddModCommand("notsea", notseaLine) + apiAddModCommand("notunderground", notundergroundLine) +
                apiAddModCommand("prioritylearn", prioritylearnLine) + apiAddModCommand("heallivingarmy", heallivingarmyLine) +
                apiAddModCommand("healhumanarmy", healhumanarmyLine);
    writeLog("spell_words_installed", "words=%d of 9", words);
}
