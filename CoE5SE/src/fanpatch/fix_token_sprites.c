/*
 * fix_token_sprites.c - Fix #5: distorted item sprites on battle tokens.
 *
 * The bug
 *   A unit's battle token shows its items. The game sizes each item icon from
 *   the width and height stored in the base sprite file (item.trs) for that
 *   sprite number. Item sprites added by mods (spr / copyspr) live in slots
 *   past the end of item.trs, so the game reads whatever bytes follow the
 *   sprite table and gets a random aspect ratio. The inventory screen does not
 *   have this problem because it uses the real size of the mod image.
 *
 * In v5.41 the icon code is built into the token-drawing function, so the
 * fix hooks the sprite draw it ends in (draw_sprite) and acts only on the
 * call coming from that one spot (checked by its return address, game.h).
 *
 * The fix
 *   For that call, when the sprite is a mod image or its number is past the
 *   end of the sprite file, the width comes from the game's own sprite-size
 *   cache: the same width/height the inventory screen uses, filled in when the
 *   image is first loaded. If that cache has no entry yet, the icon is drawn
 *   square for that frame. The icon stays centred the way the game centres
 *   it. Every other sprite draw goes straight to the original, unchanged.
 *
 * CoE5SE: this file is Alex the Beetrayer's fan patch fix, built into CoE5SE; each change is marked
 *   "CoE5SE:". The Mac build inlines replaced_spr into the sprite loader, so there CoE5SE reads the
 *   same list (fanpatchReplacedSprite), and the token's call is a bl, checked by its target. In a
 *   network game the fix steps aside (CoE5SE's ADR 0004).
 */

#include "fanpatch.h"
#include "minhook/include/MinHook.h"
#ifdef _MSC_VER   /* CoE5SE: built with clang, which fanpatch.h gives _ReturnAddress */
#include <intrin.h>

#pragma intrinsic(_ReturnAddress)
#endif

#define NUM_SPRFILES      0x26
#define MAX_SPRITES       0x7fbc
#define ITEM_SPRFILE      2
#define CENTRE            0.5f

#ifdef __APPLE__
/* CoE5SE: the Mac build's first bytes of draw_sprite and the sprite file loader. */
static const BYTE SIG_DRAW_SPRITE[]   = { 0xFF,0x43,0x02,0xD1,0xEF,0x3B,0x02,0x6D,0xED,0x33,0x03,0x6D,0xEB,0x2B,0x04,0x6D };
static const BYTE SIG_LOAD_SPRFILE[]  = { 0xFC,0x6F,0xBA,0xA9,0xFA,0x67,0x01,0xA9,0xF8,0x5F,0x02,0xA9 };
#else
static const BYTE SIG_DRAW_SPRITE[]   = { 0x48,0x8B,0xC4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x57,0x48,0x81,0xEC,0xA0 };
static const BYTE SIG_LOAD_SPRFILE[]  = { 0x40,0x55,0x57,0x41,0x54,0x41,0x57,0x48,0x81,0xEC,0x18,0x0A };
static const BYTE SIG_REPLACED_SPR[]  = { 0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7C,0x24,0x18,0x41 };
/* The token's icon draw: "addss xmm3,xmm6 / call draw_sprite / cmp esi,r15d" */
static const BYTE SIG_TOKEN_CALL[]    = { 0xF3,0x0F,0x58,0xDE,0xE8 };
static const BYTE SIG_TOKEN_RET[]     = { 0x41,0x3B,0xF7 };
#endif

/* ----------------------------------------------------- function types */

typedef void (*DrawSprite_t)(unsigned file, unsigned spr, int flags, float x,
                             float y, float width, float height, float scale);
typedef void (*LoadSprFile_t)(unsigned file);
typedef int  (*ReplacedSpr_t)(unsigned file, unsigned spr);

static DrawSprite_t   o_draw_sprite;     /* trampoline to the original */
static LoadSprFile_t  load_sprfile;
static ReplacedSpr_t  replaced_spr;
static void          *s_token_ret;      /* live return address of the token call */

/* Number of sprites in a loaded .trs file: big-endian u16 at +4 ("TCS" header). */
static int sprfile_count(const unsigned char *f)
{
    if (!f || f[0] != 'T' || f[1] != 'C' || f[2] != 'S')
        return 0;
    return (f[4] << 8) | f[5];
}

/* --------------------------------------------------------------- stats */

static volatile LONG s_fixed_cached;   /* drawn using the cached real size  */
static volatile LONG s_fixed_square;   /* drawn square, size not cached yet */
static volatile LONG s_logged;         /* detail lines written so far       */
static int s_log_limit;

/* One bit per (file, sprite): each sprite is described once with its real
 * size, plus once more if it was first drawn before the size was cached. */
static unsigned char s_seen_sized[(NUM_SPRFILES * MAX_SPRITES + 7) / 8];
static unsigned char s_seen_square[(NUM_SPRFILES * MAX_SPRITES + 7) / 8];

static int first_time(unsigned char *bits, size_t idx)
{
    unsigned char m = (unsigned char)(1u << (idx & 7));
    if (bits[idx >> 3] & m)
        return 0;
    bits[idx >> 3] |= m;
    return 1;
}

static void log_detail(unsigned file, unsigned spr, int rep, int count,
                       int w, int h)
{
    size_t idx = (size_t)file * MAX_SPRITES + spr;

    if (!first_time((w > 0 && h > 0) ? s_seen_sized : s_seen_square, idx))
        return;
    if (InterlockedIncrement(&s_logged) <= s_log_limit)
        fp_log("token sprite: file %u spr %u (mod image %d, file has %d) "
               "-> %s %dx%d", file, spr, rep, count,
               (w > 0 && h > 0) ? "real size" : "square, size not cached",
               w, h);
}

/* ---------------------------------------------------------------- hook */

static void hk_draw_sprite(unsigned file, unsigned spr, int flags, float x,
                           float y, float width, float height, float scale)
{
    void **files;
    const unsigned char *sprfile;
    int rep, count, w, h;
    float new_width;
    size_t idx;

    if (_ReturnAddress() != s_token_ret || file != ITEM_SPRFILE || spr >= MAX_SPRITES
        || fanpatchNetworkGame()) {   /* CoE5SE: the game's own in a network game */
        o_draw_sprite(file, spr, flags, x, y, width, height, scale);
        return;
    }

    load_sprfile(file);
    files = GAME_PTR(void **, VA_SPRFILE_TABLE);
    sprfile = (const unsigned char *)files[file];
    rep = replaced_spr(file, spr);
    count = sprfile_count(sprfile);
    if (rep < 0 && (int)spr < count) {
        /* Normal sprite inside the file: the game's size is correct. */
        o_draw_sprite(file, spr, flags, x, y, width, height, scale);
        return;
    }

    idx = (size_t)file * MAX_SPRITES + spr;
    w = GAME_PTR(short *, VA_SPR_WIDTH)[idx];
    h = GAME_PTR(short *, VA_SPR_HEIGHT)[idx];
    if (w > 0 && h > 0) {
        new_width = ((float)w / (float)h) * height;
        InterlockedIncrement(&s_fixed_cached);
    } else {
        new_width = height;
        InterlockedIncrement(&s_fixed_square);
    }
    log_detail(file, spr, rep, count, w, h);

    /* The game placed the icon at base + (height - width) * 0.5; move it so
     * the corrected width is centred the same way. */
    x += (width - new_width) * CENTRE;
    o_draw_sprite(file, spr, flags, x, y, new_width, height, scale);
}

/* --------------------------------------------------------------- setup */

int fix_token_sprites_install(void)
{
    MH_STATUS st;
    void *target;

    if (!fp_config_int("fixes", "token_sprites", 1)) {
        fp_log("fix token_sprites: off in fanpatch.ini");
        return 0;
    }
    s_log_limit = fp_config_int("debug", "token_sprite_log_lines", 20);

    if (
#ifdef __APPLE__   /* CoE5SE: the token's bl goes to draw_sprite */
        fanpatchCallTarget(VA_TOKEN_ICON_RET - 4) != GAME_PTR(void *, VA_DRAW_SPRITE) ||
#else
        !fp_check_bytes(VA_TOKEN_ICON_RET - 9, SIG_TOKEN_CALL, sizeof SIG_TOKEN_CALL, "token icon call") ||
        !fp_check_bytes(VA_TOKEN_ICON_RET, SIG_TOKEN_RET, sizeof SIG_TOKEN_RET, "token icon return") ||
        *GAME_PTR(int *, VA_TOKEN_ICON_RET - 4) != (int)(VA_DRAW_SPRITE - VA_TOKEN_ICON_RET) ||
        !fp_check_bytes(VA_REPLACED_SPR, SIG_REPLACED_SPR, sizeof SIG_REPLACED_SPR, "replaced sprite") ||
#endif
        !fp_check_bytes(VA_DRAW_SPRITE, SIG_DRAW_SPRITE, sizeof SIG_DRAW_SPRITE, "draw sprite") ||
        !fp_check_bytes(VA_LOAD_SPRFILE, SIG_LOAD_SPRFILE, sizeof SIG_LOAD_SPRFILE, "load sprite file")) {
        fp_log("fix token_sprites: NOT installed (code does not match " GAME_VERSION_TEXT ")");
        return 0;
    }

    load_sprfile  = GAME_PTR(LoadSprFile_t,  VA_LOAD_SPRFILE);
#ifdef __APPLE__
    replaced_spr  = (ReplacedSpr_t)fanpatchReplacedSprite;   /* CoE5SE: Mac inlines replaced_spr */
#else
    replaced_spr  = GAME_PTR(ReplacedSpr_t,  VA_REPLACED_SPR);
#endif
    s_token_ret   = GAME_PTR(void *, VA_TOKEN_ICON_RET);

    target = GAME_PTR(void *, VA_DRAW_SPRITE);
    st = MH_CreateHook(target, (void *)hk_draw_sprite, (void **)&o_draw_sprite);
    if (st != MH_OK) {
        fp_log("fix token_sprites: MH_CreateHook failed (%s)", MH_StatusToString(st));
        return 0;
    }
    st = MH_EnableHook(target);
    if (st != MH_OK) {
        fp_log("fix token_sprites: MH_EnableHook failed (%s)", MH_StatusToString(st));
        MH_RemoveHook(target);
        return 0;
    }
    fp_log("fix token_sprites: installed at %p", target);
    return 1;
}

void fix_token_sprites_report(void)
{
    fp_log("fix token_sprites: corrected %ld icon draws using the real size, "
           "%ld drawn square before the size was cached",
           (long)s_fixed_cached, (long)s_fixed_square);
}
