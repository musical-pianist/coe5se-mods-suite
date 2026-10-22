/*
 * fix_feeblemind.c - Feebleminded lowers casting level right away.
 *
 * The bug (still in v5.41)
 *   Feebleminded is affliction bit 0x2000000 (unit +0). The casting level
 *   already subtracts 1 for it, but a caster casts from its stored list of
 *   known spells (unit +0x98, up to 30), picked at random by VA_PICK_SPELL.
 *   That list is only rebuilt by VA_REBUILD_SPELLS, which the unit card and a
 *   few other game events call. Getting Feebleminded doesn't, so the unit
 *   keeps casting spells above its new level until someone opens its card.
 *
 * The fix
 *   - The add-afflictions function is hooked: when it makes a unit
 *     Feebleminded, the unit's spell list is rebuilt at once with the game's
 *     own rebuild function, exactly as viewing the card would.
 *   - Many effects set or clear the bit directly instead, so two more
 *     places check whether a unit's Feebleminded state
 *     has changed since its last rebuild and rebuild once if so: the battle
 *     spell picker, before a spell is chosen, and the unit card, before it is
 *     drawn, so the card shows the right spells the first time it's opened.
 *   - Recovery: no game function removes Feebleminded on its own, in v5.40 or
 *     v5.41. Healing ("cure one affliction", the cure-all on returning from
 *     death) subtracts the bit in place, so recovery is always caught by the
 *     two checks above. (v5.40's remove-afflictions function, hooked before,
 *     was only ever used for two other afflictions.)
 *
 * CoE5SE: this file is Alex the Beetrayer's fan patch fix, built into CoE5SE; each change is marked
 *   "CoE5SE:". A battle's replay plays the battle again from the units as they were when it began,
 *   so a unit this fix already rebuilt would cast its old spells in the replay: before casting, a
 *   Feebleminded unit whose spell list still holds a spell above its level is rebuilt whatever its
 *   state says. In a network game the fix steps aside (CoE5SE's ADR 0004).
 */

#include "fanpatch.h"
#include "minhook/include/MinHook.h"
#include <string.h>

#define VA_REBUILD       VA_REBUILD_SPELLS
#define VA_UNIT_AFFL     VA_UNITS        /* u64 afflictions at unit +0 */

#define FEEBLEMINDED     0x2000000ULL
#define MAX_TRACKED      65536

#ifdef __APPLE__
/* CoE5SE: the Mac build's first bytes of the same four functions. */
static const BYTE SIG_ADD_AFFL[]   = { 0xF6,0x57,0xBD,0xA9,0xF4,0x4F,0x01,0xA9,0xFD,0x7B,0x02,0xA9,0xFD,0x83,0x00,0x91 };
static const BYTE SIG_PICK_SPELL[] = { 0xFF,0xC3,0x00,0xD1,0xF4,0x4F,0x01,0xA9,0xFD,0x7B,0x02,0xA9,0xFD,0x83,0x00,0x91 };
static const BYTE SIG_UNIT_CARD[]  = { 0xEF,0x3B,0xB6,0x6D,0xED,0x33,0x01,0x6D,0xEB,0x2B,0x02,0x6D,0xE9,0x23,0x03,0x6D };
static const BYTE SIG_REBUILD[]    = { 0xFF,0x83,0x02,0xD1,0xFC,0x6F,0x04,0xA9,0xFA,0x67,0x05,0xA9,0xF8,0x5F,0x06,0xA9 };
#else
static const BYTE SIG_ADD_AFFL[]   = { 0x85,0xC9,0x0F,0x88,0x24,0x04,0x00,0x00,0x53,0x55,0x48,0x83,0xEC,0x38 };
static const BYTE SIG_PICK_SPELL[] = { 0x40,0x57,0x48,0x83,0xEC,0x20,0x45,0x33,0xDB,0x8B,0xFA,0x45,0x8B,0xCB,0x85,0xC9 };
static const BYTE SIG_UNIT_CARD[]  = { 0x48,0x89,0x5C,0x24,0x18,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57 };
static const BYTE SIG_REBUILD[]    = { 0x89,0x4C,0x24,0x08,0x4C,0x8B,0xDC,0x41,0x56,0x48,0x81,0xEC,0x80,0x00,0x00,0x00 };
#endif

typedef void (*Affl_t)(int unit, unsigned long long bits);
typedef int  (*Pick_t)(int unit, int path);
typedef int  (*Rebuild_t)(int unit);
typedef unsigned long long (*Card_t)(unsigned unit, int a, int b, int c, unsigned d);

static Affl_t    o_add;
static Pick_t    o_pick;
static Rebuild_t rebuild;
static Card_t    o_card;

/* 0 = not seen, 1 = rebuilt while not Feebleminded, 2 = rebuilt while Feebleminded */
static unsigned char s_state[MAX_TRACKED];
static long s_rebuilds;

static int valid_unit(int unit)
{
    return unit >= 0 && unit <= *GAME_PTR(int *, VA_UNIT_MAX);
}

static int is_feeble(int unit)
{
    return (*(GAME_PTR(unsigned long long *, VA_UNIT_AFFL) + (size_t)unit * (UNIT_STRIDE / 8))
            & FEEBLEMINDED) != 0;
}

static int s_in_rebuild;     /* the game's rebuild calls the spell picker in v5.41 */

/* Every rebuild, the game's own or ours, goes through here. The unit is
 * marked first and the picker check is off while it runs: in v5.41 the
 * rebuild function itself calls the spell picker (to drop spells when a unit
 * knows too many), and the picker hook must not start another rebuild in the
 * middle of one. */
static int hk_rebuild(int unit)
{
    int r;

    if (valid_unit(unit) && unit < MAX_TRACKED)
        s_state[unit] = is_feeble(unit) ? 2 : 1;
    s_in_rebuild++;
    r = rebuild(unit);
    s_in_rebuild--;
    return r;
}

static void do_rebuild(int unit, const char *why)
{
    if (s_in_rebuild)
        return;
    hk_rebuild(unit);
    s_rebuilds++;
    if (s_rebuilds <= 50)
        fp_log("feeblemind: unit %d spell list rebuilt (%s)", unit, why);
}

static void hk_add(int unit, unsigned long long bits)
{
    int before = valid_unit(unit) && is_feeble(unit);
    o_add(unit, bits);
    if (fanpatchNetworkGame())   /* CoE5SE: the game's own in a network game */
        return;
    if (valid_unit(unit) && !before && is_feeble(unit))
        do_rebuild(unit, "became Feebleminded");
}

/* Rebuild once if the unit's Feebleminded state changed since we last looked. */
static void check_unit(int unit, const char *why)
{
    unsigned char now;

    if (!valid_unit(unit) || unit >= MAX_TRACKED || fanpatchNetworkGame())   /* CoE5SE: not in a network game */
        return;
    now = is_feeble(unit) ? 2 : 1;
    /* CoE5SE: a Feebleminded unit still holding a spell above its level is rebuilt whatever its state
     * says: a battle's replay starts from the units as they were, before an earlier rebuild. */
    if (now == 2 && fanpatchSpellAboveLevel(unit)) {
        do_rebuild(unit, why);
        return;
    }
    if (s_state[unit] == now)
        return;
    if (s_state[unit] != 0 || now == 2)
        do_rebuild(unit, why);
    else
        s_state[unit] = now;
}

static int hk_pick(int unit, int path)
{
    if (!s_in_rebuild)
        check_unit(unit, "Feebleminded state changed, before casting");
    return o_pick(unit, path);
}

static unsigned long long hk_card(unsigned unit, int a, int b, int c, unsigned d)
{
    check_unit((int)unit, "Feebleminded state changed, before showing the unit card");
    return o_card(unit, a, b, c, d);
}

static int hook(ULONG_PTR va, void *detour, void **orig, const char *what)
{
    void *target = GAME_PTR(void *, va);
    MH_STATUS st = MH_CreateHook(target, detour, orig);

    if (st != MH_OK) {
        fp_log("fix feeblemind: MH_CreateHook(%s) failed (%s)", what, MH_StatusToString(st));
        return 0;
    }
    st = MH_EnableHook(target);
    if (st != MH_OK) {
        fp_log("fix feeblemind: MH_EnableHook(%s) failed (%s)", what, MH_StatusToString(st));
        MH_RemoveHook(target);
        return 0;
    }
    return 1;
}

int fix_feeblemind_install(void)
{
    int ok;

    if (!fp_config_int("fixes", "feeblemind", 1)) {
        fp_log("fix feeblemind: off in fanpatch.ini");
        return 0;
    }
    if (!fp_check_bytes(VA_ADD_AFFL, SIG_ADD_AFFL, sizeof SIG_ADD_AFFL, "add affliction") ||
        !fp_check_bytes(VA_PICK_SPELL, SIG_PICK_SPELL, sizeof SIG_PICK_SPELL, "pick spell") ||
        !fp_check_bytes(VA_REBUILD, SIG_REBUILD, sizeof SIG_REBUILD, "rebuild spells") ||
        !fp_check_bytes(VA_UNIT_CARD, SIG_UNIT_CARD, sizeof SIG_UNIT_CARD, "unit card")) {
        fp_log("fix feeblemind: NOT installed (code does not match " GAME_VERSION_TEXT ")");
        return 0;
    }
    memset(s_state, 0, sizeof s_state);

    if (!hook(VA_REBUILD, (void *)hk_rebuild, (void **)&rebuild, "rebuild spells")) {
        fp_log("fix feeblemind: NOT installed");
        return 0;
    }
    ok = hook(VA_ADD_AFFL, (void *)hk_add, (void **)&o_add, "add affliction");
    ok &= hook(VA_PICK_SPELL, (void *)hk_pick, (void **)&o_pick, "pick spell");
    ok &= hook(VA_UNIT_CARD, (void *)hk_card, (void **)&o_card, "unit card");
    fp_log("fix feeblemind: %s", ok ? "installed" : "partly installed (see above)");
    return ok;
}

void fix_feeblemind_report(void)
{
    fp_log("fix feeblemind: %ld spell list rebuild(s)", s_rebuilds);
}
