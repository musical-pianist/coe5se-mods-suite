// Weapon trait words: onlyaffects, doesntaffect, doubledmgvs.
#pragma once
#include "move_words.h"
#include "ritual_effect_names.h"

#define WEAPON_TRAIT_WORD_SIZE 32

enum { WEAPON_TRAIT_FLAG, WEAPON_TRAIT_MOVE, WEAPON_TRAIT_ABILITY };
enum { WEAPON_RULE_ONLY, WEAPON_RULE_NOT, WEAPON_RULE_DOUBLE };
static const char *const WEAPON_RULE_WORDS[] = {"onlyaffects", "doesntaffect", "doubledmgvs"};
static const char *const WEAPON_RULE_TEXT[] = {"(only affects %s)", "(doesn't affect %s)", "(double damage vs %s)"};

typedef struct {
    int weapon, rule, kind;
    unsigned long long value;
    char word[WEAPON_TRAIT_WORD_SIZE];
} WeaponTraitRule;

static struct {
    WeaponTraitRule *rules;
    int count, capacity;
    unsigned char *named;
    int namedSize;
} weaponTraits;

// How a unit has a trait, as the game counts it.
static unsigned long long (*weaponTraitUnitFlags)(int unit);
static unsigned long long (*weaponTraitUnitMoves)(int unit);
static int (*weaponTraitUnitAbility)(int unit, int ability);
static void (*weaponTraitBattleLog)(int attacker, int target, const char *text);

// The trait the word names, into *kind and *value; 0 when the game has no such monster word.
static int findWeaponTrait(const char *word, int *kind, unsigned long long *value) {
    static const MonsterFlagWord OWN[] = {{"small", MONSTER_FLAG_SMALL}, {"combatcaster", MONSTER_FLAG_COMBAT_CASTER}};
    size_t len = strlen(word);
    const MonsterFlagWord *flag = gameFlagWord(word, len);
    for (size_t i = 0; !flag && i < sizeof OWN / sizeof OWN[0]; i++)
        if (!strcmp(OWN[i].word, word)) flag = &OWN[i];
    if (flag) {
        *kind = WEAPON_TRAIT_FLAG;
        *value = flag->bit;
        return 1;
    }
    for (size_t i = 0; i < sizeof GAME_MOVE_WORDS / sizeof GAME_MOVE_WORDS[0]; i++)
        if (!strcmp(GAME_MOVE_WORDS[i].word, word)) {
            *kind = WEAPON_TRAIT_MOVE;
            *value = GAME_MOVE_WORDS[i].mask;
            return 1;
        }
    for (size_t i = 0; i < sizeof RITUAL_EFFECT_NAMES / sizeof RITUAL_EFFECT_NAMES[0]; i++)
        for (const char *name = RITUAL_EFFECT_NAMES[i].name; name; name = strchr(name, '/') ? strchr(name, '/') + 1 : NULL) {
            size_t n = strcspn(name, "/");
            if (n == len && !strncmp(name, word, len)) {
                *kind = WEAPON_TRAIT_ABILITY;
                *value = (unsigned long long)RITUAL_EFFECT_NAMES[i].id;
                return 1;
            }
        }
    return 0;
}

// 1 when the unit has the rule's trait.
static int unitHasWeaponTrait(int unit, const WeaponTraitRule *r) {
    if (r->kind == WEAPON_TRAIT_FLAG) return (weaponTraitUnitFlags(unit) & r->value) != 0;
    if (r->kind == WEAPON_TRAIT_MOVE) return (weaponTraitUnitMoves(unit) & r->value) == r->value;
    return weaponTraitUnitAbility(unit, (int)r->value) > 0;
}

// What the weapon's rules make of a hit on `target`: 0 unaffected, 1 as the game has it, 2 double.
static int weaponTraitVerdict(int weapon, int target, const char **word) {
    if (weapon < 0 || weapon >= weaponTraits.namedSize || !weaponTraits.named[weapon] || target < 0 || target >= UNIT_TABLE_MAX ||
        G16(RVA_UNIT_MONSTER + (size_t)target * UNIT_SIZE) < 0)
        return 1;
    int only = 0, onlyMatched = 0;
    const char *doubled = NULL;
    for (int i = 0; i < weaponTraits.count; i++) {
        const WeaponTraitRule *r = &weaponTraits.rules[i];
        if (r->weapon != weapon) continue;
        if (r->rule == WEAPON_RULE_ONLY) only = 1;
        if (r->rule == WEAPON_RULE_DOUBLE && doubled) continue;
        if (r->rule == WEAPON_RULE_ONLY && onlyMatched) continue;
        if (!unitHasWeaponTrait(target, r)) continue;
        if (r->rule == WEAPON_RULE_NOT) return 0;
        if (r->rule == WEAPON_RULE_ONLY) onlyMatched = 1;
        else doubled = r->word;
    }
    if (only && !onlyMatched) return 0;
    if (word) *word = doubled;
    return doubled ? 2 : 1;
}

// The hit's damage after the weapon's rules, at the game's multipliers (RVA_WEAPON_HIT_DAMAGE_CHECK).
static int weaponTraitDamage(int attacker, int target, int weapon, int damage) {
    if (damage < 1) return damage;
    const char *word = NULL;
    int verdict = weaponTraitVerdict(weapon, target, &word);
    if (verdict == 0) return 0;
    if (verdict == 2) {
        char text[64];
        snprintf(text, sizeof text, "double damage due to %s", word);
        weaponTraitBattleLog(attacker, target, text);
        return damage * 2;
    }
    return damage;
}

// Takes every rule off the weapon.
static void clearWeaponTraitRules(int weapon) {
    int kept = 0;
    for (int i = 0; i < weaponTraits.count; i++)
        if (weaponTraits.rules[i].weapon != weapon) weaponTraits.rules[kept++] = weaponTraits.rules[i];
    weaponTraits.count = kept;
    if (weapon >= 0 && weapon < weaponTraits.namedSize) weaponTraits.named[weapon] = 0;
}

// Adds a rule to the weapon; 0 when out of memory.
static int addWeaponTraitRule(int weapon, int rule, int kind, unsigned long long value, const char *word) {
    int records = tableRecords(TABLE_WEAPON);
    if (weaponTraits.namedSize != records) {
        unsigned char *named = realloc(weaponTraits.named, (size_t)records);
        if (!named) return 0;
        if (records > weaponTraits.namedSize) memset(named + weaponTraits.namedSize, 0, (size_t)(records - weaponTraits.namedSize));
        weaponTraits.named = named;
        weaponTraits.namedSize = records;
    }
    for (int i = 0; i < weaponTraits.count; i++) {
        const WeaponTraitRule *r = &weaponTraits.rules[i];
        if (r->weapon == weapon && r->rule == rule && r->kind == kind && r->value == value) return 1;
    }
    if (!growList((void **)&weaponTraits.rules, &weaponTraits.capacity, weaponTraits.count + 1, sizeof *weaponTraits.rules)) return 0;
    WeaponTraitRule *r = &weaponTraits.rules[weaponTraits.count++];
    r->weapon = weapon;
    r->rule = rule;
    r->kind = kind;
    r->value = value;
    snprintf(r->word, sizeof r->word, "%s", word);
    weaponTraits.named[weapon] = 1;
    return 1;
}

// One onlyaffects, doesntaffect or doubledmgvs line at level 2: adds the rule, or refuses the line.
static void weaponTraitLine(int rule, const char *args, int mod, int level) {
    if (level != 2) return;
    const char *word = WEAPON_RULE_WORDS[rule], *p = args;
    char trait[WEAPON_TRAIT_WORD_SIZE];
    int len = readWord(&p, trait, sizeof trait), weapon = G32(RVA_WEAPON_CURRENT), kind;
    unsigned long long value;
    if (weapon < 0 || weapon >= tableRecords(TABLE_WEAPON))
        refuseC5mLine(word, spellModName(mod), "no_weapon", "line=%d", G32(RVA_MOD_LINE));
    else if (!len)
        refuseC5mLine(word, spellModName(mod), "no_trait", "line=%d", G32(RVA_MOD_LINE));
    else if ((size_t)len >= sizeof trait || !findWeaponTrait(trait, &kind, &value))
        refuseC5mLine(word, spellModName(mod), "unknown_trait", "line=%d trait=%.40s", G32(RVA_MOD_LINE), trait);
    else if (!addWeaponTraitRule(weapon, rule, kind, value, trait))
        refuseC5mLine(word, spellModName(mod), "out_of_memory", "line=%d", G32(RVA_MOD_LINE));
}

static void onlyaffectsLine(const char *args, int mod, int level) {
    weaponTraitLine(WEAPON_RULE_ONLY, args, mod, level);
}
static void doesntaffectLine(const char *args, int mod, int level) {
    weaponTraitLine(WEAPON_RULE_NOT, args, mod, level);
}
static void doubledmgvsLine(const char *args, int mod, int level) {
    weaponTraitLine(WEAPON_RULE_DOUBLE, args, mod, level);
}

// Every .c5m line outside events comes here before the game's parser.
static int weaponTraitLineHook(const char *line, size_t len, int level) {
    if (level != 2 || len != sizeof "clearwspec" - 1 || strncmp(line, "clearwspec", len)) return 0;
    clearWeaponTraitRules(G32(RVA_WEAPON_CURRENT));
    return 0;
}

// Before a mod pass: a level-2 pass reads every weapon's rules again.
static void weaponTraitsBeforePass(int level) {
    if (level != 2) return;
    weaponTraits.count = 0;
    if (weaponTraits.named) memset(weaponTraits.named, 0, (size_t)weaponTraits.namedSize);
}

// ---- The game's side -----------------------------------------------------------------------------
static unsigned long long gameUnitResistances(int unit) {
    return GAME_FN(unsigned long long (*)(int), RVA_UNIT_RESISTANCES)(unit);
}
static unsigned long long gameUnitMoveFlags(int unit) {
    return GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(unit);
}
static int gameUnitAbility(int unit, int ability) {
    return GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL)(unit, ability, 1, NULL, 0, NULL);
}

// One line of the battle report, kind 2 as the game's own doublings.
static void gameBattleLog(int attacker, int target, const char *text) {
    GAME_FN(void (*)(int, int, int, const char *, ...), RVA_BATTLE_LOG)(attacker, target, 2, text);
}

typedef int (*TargetUsefulFn)(int unit, unsigned long long type, int weapon, unsigned long long value, int target);
typedef void (*WeaponTextFn)(int weapon, unsigned long long dmg, int chained);

// The game's RVA_BATTLE_TARGET_USEFUL, then the weapon's rules on the target the caller chose.
static int targetUsefulHook(int unit, unsigned long long type, int weapon, unsigned long long value, int target) {
    int result = GAME_FN(TargetUsefulFn, RVA_BATTLE_TARGET_USEFUL)(unit, type, weapon, value, target);
    if (result == 1 && target >= 0 && weaponTraitVerdict(weapon, target, NULL) == 0) return BATTLE_TARGET_NOT_AFFECTED;
    return result;
}

// The game's lines for a weapon on an attack box, then a line for each of its rules.
static void weaponTextHook(int weapon, unsigned long long dmg, int chained) {
    GAME_FN(WeaponTextFn, RVA_WEAPON_TEXT)(weapon, dmg, chained);
    if (weapon < 0 || weapon >= weaponTraits.namedSize || !weaponTraits.named[weapon]) return;
    char line[80];
    for (int i = 0; i < weaponTraits.count; i++)
        if (weaponTraits.rules[i].weapon == weapon) {
            snprintf(line, sizeof line, WEAPON_RULE_TEXT[weaponTraits.rules[i].rule], weaponTraits.rules[i].word);
            textButton(0, line);
        }
}

#ifndef __aarch64__
// The hit's stub, in a page near the game: RVA_WEAPON_HIT_DAMAGE_CHECK's 9 bytes become a jump to it.
#define WEAPON_HIT_STUB          0x000
#define WEAPON_HIT_FUNCTION      0x080
#define WEAPON_HIT_UNWIND        0x090
#define WEAPON_HIT_UNWIND_CODES  64
#define WEAPON_HIT_REDIRECTS     0x200
#define WEAPON_HIT_CHAIN_MAX     16
#define UNWIND_FLAG_CHAININFO    4

// Copies the hit's unwind info for the stub, its chain of entries made one, with no prolog.
static size_t copyWeaponHitUnwind(unsigned char *out) {
    DWORD64 imageBase = 0;
    PRUNTIME_FUNCTION f = RtlLookupFunctionEntry((DWORD64)(uintptr_t)GAME_PTR(RVA_WEAPON_HIT_DAMAGE_CHECK), &imageBase, NULL);
    if (!f || imageBase != (DWORD64)(uintptr_t)gameBase) return 0;
    int codes = 0;
    for (int link = 0; link < WEAPON_HIT_CHAIN_MAX; link++) {
        const unsigned char *info = gameBase + f->UnwindData;
        int flags = info[0] >> 3, count = info[2];
        if ((info[0] & 7) != 1 || (flags & ~UNWIND_FLAG_CHAININFO) || codes + count > WEAPON_HIT_UNWIND_CODES) return 0;
        memcpy(out + 4 + codes * 2, info + 4, (size_t)count * 2);
        codes += count;
        if (!(flags & UNWIND_FLAG_CHAININFO)) {
            out[0] = 1;
            out[1] = 0;
            out[2] = (unsigned char)codes;
            out[3] = info[3];
            return 4 + (size_t)((codes + 1) & ~1) * 2;
        }
        f = (PRUNTIME_FUNCTION)(info + 4 + ((count + 1) & ~1) * 2);
    }
    return 0;
}

static void emitWeaponHitRel32(unsigned char **p, unsigned char *target) {
    int rel = (int)(target - (*p + 4));
    memcpy(*p, &rel, 4);
    *p += 4;
}

// Writes the hit's stub and jump, once. Returns 1 when hits go through it.
static int patchWeaponHit(unsigned char *page) {
    if (!copyWeaponHitUnwind(page + WEAPON_HIT_UNWIND)) {
        writeLog("weapon_hit_patched", "ok=0 reason=unwind_info");
        return 0;
    }
    static const unsigned char ARGS[] = {0x44, 0x89, 0xf1, 0x89, 0xf2, 0x44, 0x8b, 0x45, 0x60, 0x45, 0x89, 0xf9};
    static const unsigned char AFTER[] = {0x41, 0x89, 0xc7, 0x89, 0x44, 0x24, 0x40, 0x4c, 0x8b, 0x44, 0x24, 0x58, 0x45, 0x85, 0xff};
    unsigned char *stub = page + WEAPON_HIT_STUB, *p = stub;
    void *hook = (void *)weaponTraitDamage;
    memcpy(p, ARGS, sizeof ARGS);
    p += sizeof ARGS;
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &hook, 8); p += 8;
    *p++ = 0xff; *p++ = 0xd0;
    memcpy(p, AFTER, sizeof AFTER);
    p += sizeof AFTER;
    *p++ = 0x0f; *p++ = 0x8e; emitWeaponHitRel32(&p, GAME_PTR(RVA_WEAPON_HIT_NO_DAMAGE));
    *p++ = 0xe9; emitWeaponHitRel32(&p, GAME_PTR(RVA_WEAPON_HIT_DAMAGE_GO_ON));
    RUNTIME_FUNCTION *function = (RUNTIME_FUNCTION *)(page + WEAPON_HIT_FUNCTION);
    function->BeginAddress = WEAPON_HIT_STUB;
    function->EndAddress = (DWORD)(p - page);
    function->UnwindData = WEAPON_HIT_UNWIND;
    if (!RtlAddFunctionTable(function, 1, (DWORD64)(uintptr_t)page)) {
        writeLog("weapon_hit_patched", "ok=0 reason=unwind_not_added");
        return 0;
    }
    FlushInstructionCache(GetCurrentProcess(), page, WEAPON_HIT_REDIRECTS);
    unsigned char *at = GAME_PTR(RVA_WEAPON_HIT_DAMAGE_CHECK), code[9] = {0xe9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90};
    int rel = (int)(stub - (at + 5));
    memcpy(code + 1, &rel, 4);
    DWORD old;
    if (!VirtualProtect(at, sizeof code, PAGE_EXECUTE_READWRITE, &old)) {
        writeLog("weapon_hit_patched", "ok=0 reason=code_unwritable");
        return 0;
    }
    memcpy(at, code, sizeof code);
    VirtualProtect(at, sizeof code, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, sizeof code);
    writeLog("weapon_hit_patched", "ok=1 address=%p stub=%p", (void *)at, (void *)stub);
    return 1;
}

// Points the attack box's and the AI's calls at the hooks. Returns how many it pointed.
static int pointWeaponTraitCalls(unsigned char *page) {
    unsigned char *stubs = page + WEAPON_HIT_REDIRECTS;
    return redirectCall(RVA_WEAPON_TEXT_CALL, RVA_WEAPON_TEXT, stubs, (void *)weaponTextHook, "weapon_text", NULL) +
           redirectCall(RVA_WEAPON_TEXT_NEXT_CALL, RVA_WEAPON_TEXT, stubs + 0x20, (void *)weaponTextHook, "weapon_text_next", NULL) +
           redirectCall(RVA_BATTLE_TARGET_USEFUL_CALL, RVA_BATTLE_TARGET_USEFUL, stubs + 0x40, (void *)targetUsefulHook,
                        "battle_target_useful", NULL) +
           redirectCall(RVA_SPELL_USEFUL_TARGET_CALL, RVA_BATTLE_TARGET_USEFUL, stubs + 0x60, (void *)targetUsefulHook,
                        "spell_useful_target", NULL);
}

static int installWeaponTraitHooks(int *hit) {
    unsigned char *page = allocNear(GAME_PTR(RVA_WEAPON_HIT_DAMAGE_CHECK));
    if (!page) {
        writeLog("weapon_hit_patched", "ok=0 reason=no_near_page");
        return 0;
    }
    *hit = patchWeaponHit(page);
    return pointWeaponTraitCalls(page);
}
#else
// @platform-split weapon_hit: Mac's stub is fixed code here, reached through the header padding
// The hit's stub: RVA_WEAPON_HIT_DAMAGE_CHECK's cmp becomes a b to a trampoline to it.
int coe5seWeaponTraitDamage(int attacker, int target, int weapon, int damage);
__attribute__((used)) int coe5seWeaponTraitDamage(int attacker, int target, int weapon, int damage) {
    return weaponTraitDamage(attacker, target, weapon, damage);
}
__attribute__((used)) void *coe5seWeaponHitGoOn;
__asm__(
    ".text\n.p2align 2\n_coe5seWeaponHitStub:\n"
    "  sub sp, sp, #0x220\n"
    "  stp x0, x1, [sp, #0x00]\n"
    "  stp x2, x3, [sp, #0x10]\n"
    "  stp x4, x5, [sp, #0x20]\n"
    "  stp x6, x7, [sp, #0x30]\n"
    "  stp x8, x9, [sp, #0x40]\n"
    "  stp x10, x11, [sp, #0x50]\n"
    "  stp x12, x13, [sp, #0x60]\n"
    "  stp x14, x15, [sp, #0x70]\n"
    "  stp x16, x17, [sp, #0x80]\n"
    "  stp x29, x30, [sp, #0x90]\n"
    "  stp q0, q1, [sp, #0xa0]\n"
    "  stp q2, q3, [sp, #0xc0]\n"
    "  stp q4, q5, [sp, #0xe0]\n"
    "  stp q6, q7, [sp, #0x100]\n"
    "  stp q16, q17, [sp, #0x120]\n"
    "  stp q18, q19, [sp, #0x140]\n"
    "  stp q20, q21, [sp, #0x160]\n"
    "  stp q22, q23, [sp, #0x180]\n"
    "  stp q24, q25, [sp, #0x1a0]\n"
    "  stp q26, q27, [sp, #0x1c0]\n"
    "  stp q28, q29, [sp, #0x1e0]\n"
    "  stp q30, q31, [sp, #0x200]\n"
    "  ldur w2, [x29, #-0x90]\n"
    "  add x29, sp, #0x90\n"
    "  mov w0, w21\n"
    "  mov w1, w20\n"
    "  mov w3, w23\n"
    "  bl _coe5seWeaponTraitDamage\n"
    "  mov w23, w0\n"
    "  ldp q30, q31, [sp, #0x200]\n"
    "  ldp q28, q29, [sp, #0x1e0]\n"
    "  ldp q26, q27, [sp, #0x1c0]\n"
    "  ldp q24, q25, [sp, #0x1a0]\n"
    "  ldp q22, q23, [sp, #0x180]\n"
    "  ldp q20, q21, [sp, #0x160]\n"
    "  ldp q18, q19, [sp, #0x140]\n"
    "  ldp q16, q17, [sp, #0x120]\n"
    "  ldp q6, q7, [sp, #0x100]\n"
    "  ldp q4, q5, [sp, #0xe0]\n"
    "  ldp q2, q3, [sp, #0xc0]\n"
    "  ldp q0, q1, [sp, #0xa0]\n"
    "  ldp x29, x30, [sp, #0x90]\n"
    "  ldp x16, x17, [sp, #0x80]\n"
    "  ldp x14, x15, [sp, #0x70]\n"
    "  ldp x12, x13, [sp, #0x60]\n"
    "  ldp x10, x11, [sp, #0x50]\n"
    "  ldp x8, x9, [sp, #0x40]\n"
    "  ldp x6, x7, [sp, #0x30]\n"
    "  ldp x4, x5, [sp, #0x20]\n"
    "  ldp x2, x3, [sp, #0x10]\n"
    "  ldp x0, x1, [sp, #0x00]\n"
    "  add sp, sp, #0x220\n"
    "  cmp w23, #1\n"
    "  adrp x16, _coe5seWeaponHitGoOn@PAGE\n"
    "  ldr x16, [x16, _coe5seWeaponHitGoOn@PAGEOFF]\n"
    "  br x16\n");
extern const unsigned char coe5seWeaponHitStub[] __asm__("_coe5seWeaponHitStub");

#define ARM64_B_MASK  0xfc000000u
#define ARM64_B       0x14000000u
#define ARM64_BL      0x94000000u

// Points the bl or b at `callRva` at a trampoline to `hook`.
static int pointWeaponTraitBranch(unsigned int callRva, unsigned int targetRva, void *hook, const char *name) {
    unsigned char *at = gameBase + callRva;
    uint32_t insn;
    memcpy(&insn, at, 4);
    uint32_t op = insn & ARM64_B_MASK;
    int32_t imm = (int32_t)(insn << 6) >> 6;
    if ((op != ARM64_B && op != ARM64_BL) || at + (intptr_t)imm * 4 != gameBase + targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", name, callRva + 0x100000000ULL);
        return 0;
    }
    unsigned char *trampoline = newTrampoline(hook);
    uint32_t word = trampoline ? op | ((uint32_t)((trampoline - at) >> 2) & 0x3ffffffu) : 0;
    if (trampoline && writeGameCode(at, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=%s", name, trampoline ? "code_unwritable" : "no_padding");
    return 0;
}

// Writes the hit's b, once. Returns 1 when hits go through the stub.
static int patchWeaponHit(void) {
    coe5seWeaponHitGoOn = GAME_PTR(RVA_WEAPON_HIT_DAMAGE_GO_ON);
    unsigned char *at = GAME_PTR(RVA_WEAPON_HIT_DAMAGE_CHECK), *trampoline = newTrampoline((void *)coe5seWeaponHitStub);
    uint32_t word = trampoline ? ARM64_B | ((uint32_t)((trampoline - at) >> 2) & 0x3ffffffu) : 0;
    if (!trampoline || !writeGameCode(at, &word, 4)) {
        writeLog("weapon_hit_patched", "ok=0 reason=%s", trampoline ? "code_unwritable" : "no_padding");
        return 0;
    }
    writeLog("weapon_hit_patched", "ok=1 address=%p stub=%p", (void *)at, (void *)coe5seWeaponHitStub);
    return 1;
}

static int installWeaponTraitHooks(int *hit) {
    *hit = patchWeaponHit();
    return pointWeaponTraitBranch(RVA_WEAPON_TEXT_CALL, RVA_WEAPON_TEXT, (void *)weaponTextHook, "weapon_text") +
           pointWeaponTraitBranch(RVA_WEAPON_TEXT_NEXT_CALL, RVA_WEAPON_TEXT, (void *)weaponTextHook, "weapon_text_next") +
           pointWeaponTraitBranch(RVA_BATTLE_TARGET_USEFUL_CALL, RVA_BATTLE_TARGET_USEFUL, (void *)targetUsefulHook,
                                  "battle_target_useful") +
           pointWeaponTraitBranch(RVA_SPELL_USEFUL_TARGET_CALL, RVA_BATTLE_TARGET_USEFUL, (void *)targetUsefulHook,
                                  "spell_useful_target");
}
#endif

// Adds the words, and patches the hit, the attack box and the AI's target check.
static void weaponTraitsInit(void) {
    weaponTraitUnitFlags = gameUnitResistances;
    weaponTraitUnitMoves = gameUnitMoveFlags;
    weaponTraitUnitAbility = gameUnitAbility;
    weaponTraitBattleLog = gameBattleLog;
    int words = apiAddModCommand("onlyaffects", onlyaffectsLine) + apiAddModCommand("doesntaffect", doesntaffectLine) +
                apiAddModCommand("doubledmgvs", doubledmgvsLine);
    int hit = 0, calls = 0;
    if (checkGameFunction("weapon_hit_check", "part=prologue ", RVA_WEAPON_HIT, RVA_WEAPON_HIT_PROLOGUE_END, WEAPON_HIT_PROLOGUE_CODE,
                          WEAPON_HIT_PROLOGUE_CALLS, WEAPON_HIT_PROLOGUE_CALL_COUNT) &&
        checkGameFunction("weapon_hit_check", "part=multiply ", RVA_WEAPON_HIT_MULTIPLY, RVA_WEAPON_HIT_MULTIPLY_END,
                          WEAPON_HIT_MULTIPLY_CODE, WEAPON_HIT_MULTIPLY_CALLS,
                          (int)(sizeof WEAPON_HIT_MULTIPLY_CALLS / sizeof WEAPON_HIT_MULTIPLY_CALLS[0])))
        calls = installWeaponTraitHooks(&hit);
    writeLog("weapon_trait_words_installed", "words=%d of 3 hit=%d calls=%d of 4", words, hit, calls);
}
