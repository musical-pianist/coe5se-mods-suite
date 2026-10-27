// Recruitment bonuses: a number after templerec, libraryrec, atavirec, or a reclimiter "$" line.
#pragma once

#define REC_BONUS_AMOUNT_MAX   100
#define REC_BONUS_CAP_MAX      1000
#define REC_BONUS_NO_CAP       -1
#define REC_BONUS_MESSAGE_BELOW 41

enum { REC_BONUS_TEMPLES, REC_BONUS_LIBRARIES, REC_BONUS_ATAVI, REC_BONUS_UNITS, REC_BONUS_KINDS };

// A recruitment word, its bonus kind, and the record flag it sets.
static const struct {
    const char *word;
    int flag;
} REC_BONUS_WORDS[REC_BONUS_KINDS] = {
    {"templerec", 0x20},
    {"libraryrec", 0x40},
    {"atavirec", 0x40000},
    {"reclimiter", 0},
};

// One offer's bonus.
typedef struct {
    int cls, offer, kind;
    int slot;
    int amount, cap;
} RecBonus;

static struct {
    RecBonus *list;
    int count, capacity;
} recBonus;

// The record the shop is rolling: the class list's or a special location's, and the copy given to the game.
static struct RecBonusRoll recBonusRoll;

// What a player owns of a bonus kind; `name` is the monster for REC_BONUS_UNITS.
static int recBonusGameCount(int player, int kind, const char *name);
static int (*recBonusCountOf)(int player, int kind, const char *name) = recBonusGameCount;
static int installRecBonusUnitsHooks(void);

// ---- Reading the lines ---------------------------------------------------------------------------
// The bonus of kind and slot on the offer, or NULL.
static RecBonus *findRecBonus(int cls, int offer, int kind, int slot) {
    for (int i = 0; i < recBonus.count; i++) {
        RecBonus *b = &recBonus.list[i];
        if (b->cls == cls && b->offer == offer && b->kind == kind && b->slot == slot) return b;
    }
    return NULL;
}

static void dropRecBonus(RecBonus *b) {
    *b = recBonus.list[--recBonus.count];
}

// Drops every bonus of the class.
static void dropClassRecBonuses(int cls) {
    for (int i = recBonus.count - 1; i >= 0; i--)
        if (recBonus.list[i].cls == cls) dropRecBonus(&recBonus.list[i]);
}

// Sets the offer's bonus; 0 when there was no memory for it.
static int setRecBonus(int cls, int offer, int kind, int slot, int amount, int cap) {
    RecBonus *b = findRecBonus(cls, offer, kind, slot);
    if (!b) {
        if (recBonus.count == recBonus.capacity) {
            int capacity = recBonus.capacity ? recBonus.capacity * 2 : 64;
            RecBonus *list = realloc(recBonus.list, (size_t)capacity * sizeof *list);
            if (!list) return 0;   // Logged by the caller
            recBonus.list = list, recBonus.capacity = capacity;
        }
        b = &recBonus.list[recBonus.count++];
    }
    *b = (RecBonus){cls, offer, kind, slot, amount, cap};
    return 1;
}

// Reads "amount [cap]" at p into the two; 0 for no number at p, -1 for anything else after the word.
static int readRecBonusNumbers(const char *p, int *amount, int *cap) {
    skipSpaces(&p);
    if (!isdigit((unsigned char)*p) && !((*p == '-' || *p == '+') && isdigit((unsigned char)p[1]))) return 0;
    *cap = REC_BONUS_NO_CAP;
    if (!readInt(&p, amount)) return -1;
    skipSpaces(&p);
    if (!isLineEnd(p) && !readInt(&p, cap)) return -1;
    if (!isLineEnd(p)) return -1;
    if (*amount < 0 || *amount > REC_BONUS_AMOUNT_MAX) return -1;
    if (*cap != REC_BONUS_NO_CAP && (*cap < 0 || *cap > REC_BONUS_CAP_MAX)) return -1;
    return 1;
}

// The reclimiter line's quoted text into `text`, and where the line goes on after it; NULL with none.
static const char *readRecLimiterText(const char *args, char *text, size_t size) {
    const char *open = strchr(args, '"');
    const char *close = open ? strchr(open + 1, '"') : NULL;
    if (!close) return NULL;
    snprintf(text, size, "%.*s", (int)(close - open - 1), open + 1);
    return close + 1;
}

// Every .c5m line outside events comes here before the game's parser; the game always gets the line.
static void recBonusLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2) return;
    int cls = G32(RVA_CLASS_CURRENT), offer = G32(RVA_CLASS_LAST_RECRUIT);
    if (len == sizeof "clearrec" - 1 && !strncmp(line, "clearrec", len)) {
        dropClassRecBonuses(cls);
        return;
    }
    int kind = 0;
    while (kind < REC_BONUS_KINDS && (strlen(REC_BONUS_WORDS[kind].word) != len || strncmp(REC_BONUS_WORDS[kind].word, line, len))) kind++;
    if (kind == REC_BONUS_KINDS) return;
    if (cls < 0 || offer < 0 || cls >= tableRecords(TABLE_CLASS_RECRUITS) || offer >= CLASS_RECRUITS_MAX) return;
    const char *args = line + len, *word = REC_BONUS_WORDS[kind].word;
    int slot = -1;
    char text[MONSTER_NAME_SIZE + 1];
    if (kind == REC_BONUS_UNITS) {
        slot = G32(RVA_RECLIMITER_COUNT);
        if (slot < 0 || slot >= RECRUIT_LIMITERS_MAX) return;
        args = readRecLimiterText(args, text, sizeof text);
        if (!args) return;
    }
    int amount = 0, cap = 0, numbers = readRecBonusNumbers(args, &amount, &cap);
    RecBonus *old = findRecBonus(cls, offer, kind, slot);
    if (old) dropRecBonus(old);
    if (!numbers) return;
    if (numbers < 0 || (kind == REC_BONUS_UNITS && text[0] != '$')) {
        refuseC5mLine(word, spellModName(mod), "bad_value", "line=%d text=%.40s", G32(RVA_MOD_LINE), line + len);
        return;
    }
    if (!setRecBonus(cls, offer, kind, slot, amount, cap)) {
        refuseC5mLine(word, spellModName(mod), "no_memory", "line=%d", G32(RVA_MOD_LINE));
        return;
    }
    int hooked = kind == REC_BONUS_UNITS ? installRecBonusUnitsHooks() : installShopOfferHooks();
    if (!hooked) refuseC5mLine(word, spellModName(mod), "site_unavailable", "line=%d", G32(RVA_MOD_LINE));
}

// Before a mod pass: a level-2 pass reads every bonus again.
static void recBonusBeforePass(int level) {
    if (level != 2) return;
    recBonus.count = 0;
}

// ---- The shop's roll -----------------------------------------------------------------------------
static int recBonusGameCount(int player, int kind, const char *name) {
    if (kind == REC_BONUS_LIBRARIES) return GAME_FN(int (*)(int), RVA_PLAYER_LIBRARY_TOTAL)(player);
    if (kind == REC_BONUS_ATAVI)
        return GAME_FN(int (*)(int, int, int, int), RVA_COUNT_SQUARES_OF_TERRAIN)(player, TERRAIN_ATAVI_VILLAGE, -1, 0);
    if (kind == REC_BONUS_UNITS) return GAME_FN(int (*)(int, const char *), RVA_COUNT_PLAYER_MONSTERS)(player, name);
    int w = G32(RVA_MAP_W), h = G32(RVA_MAP_H), temples = 0;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++)
            temples += *(const signed char *)(squareRecordAt(x, y) + SQUARE_OWNER) == player &&
                       (shopSquareTerrainFlags(x, y) & TERRAIN_FLAG_TEMPLE);
    return temples;
}

// One bonus for the player: amount x min(count, cap).
static int recBonusFor(const RecBonus *b, int player, const char *name) {
    int n = recBonusCountOf(player, b->kind, name);
    if (b->cap != REC_BONUS_NO_CAP && n > b->cap) n = b->cap;
    return n > 0 ? b->amount * n : 0;
}

// The class and offer of a class list record, 0 for another record (a special location's).
static int recBonusOfferOf(const unsigned char *record, int *cls, int *offer) {
    const unsigned char *list = tableAddress(TABLE_CLASS_RECRUITS);
    size_t at = (size_t)(record - list);
    if (record < list || at >= (size_t)tableRecords(TABLE_CLASS_RECRUITS) * CLASS_RECRUITS_SIZE ||
        at % CLASS_RECRUITS_SIZE % CLASS_RECRUIT_SIZE)
        return 0;
    *cls = (int)(at / CLASS_RECRUITS_SIZE), *offer = (int)(at % CLASS_RECRUITS_SIZE / CLASS_RECRUIT_SIZE);
    return 1;
}

// Copies a class offer with flag word bonuses into `copy`, the bonuses in its chance; 0 for a record with none.
static int recBonusOfferCopy(int player, const unsigned char *record, unsigned char *copy) {
    int cls, offer;
    if (!recBonus.count || !recBonusOfferOf(record, &cls, &offer)) return 0;
    int any = 0;
    for (int kind = REC_BONUS_TEMPLES; kind < REC_BONUS_UNITS && !any; kind++) any = findRecBonus(cls, offer, kind, -1) != NULL;
    if (!any || *(const int *)(record + RECRUIT_CHANCE) == -1) return 0;
    memcpy(copy, record, CLASS_RECRUIT_SIZE);
    int bonus = 0, *flags = (int *)(copy + RECRUIT_FLAGS);
    for (int kind = REC_BONUS_TEMPLES; kind < REC_BONUS_UNITS; kind++) {
        const RecBonus *b = findRecBonus(cls, offer, kind, -1);
        if (!b) continue;
        *flags &= ~REC_BONUS_WORDS[kind].flag;
        bonus += recBonusFor(b, player, NULL);
    }
    *(int *)(copy + RECRUIT_CHANCE) += bonus;
    return 1;
}

// RVA_SHOP_PROCESS_OFFER's count for a "$" line: the line's bonus, or the game's own step.
static int recBonusUnitsCount(int player, const char *name) {
    int n = recBonusCountOf(player, REC_BONUS_UNITS, name);
    const unsigned char *rolled = recBonusRoll.rolled;
    if (!recBonusRoll.unitsHooked) return n;
    if (!rolled) return n;
    size_t at = (size_t)(name - (const char *)(rolled + RECRUIT_LIMITERS + 1));
    int slot = (int)(at / RECRUIT_LIMITER_SIZE), cls, offer;
    const RecBonus *b = name > (const char *)rolled && at % RECRUIT_LIMITER_SIZE == 0 && slot < RECRUIT_LIMITERS_MAX &&
                                recBonusOfferOf(recBonusRoll.record, &cls, &offer)
                            ? findRecBonus(cls, offer, REC_BONUS_UNITS, slot)
                            : NULL;
    if (b) return recBonusFor(b, player, name);
    int chance = *(const int *)(recBonusRoll.record + RECRUIT_CHANCE);
    return n * (1 + (chance >= 3) + (chance >= 9));
}

// After the game rolled a copy: the new entry keeps the record's flags and the game's message.
static void recBonusAfterRoll(int player, const unsigned char *record, const unsigned char *copy, int entryBefore) {
    size_t block = (size_t)player * PLAYER_BLOCK_SIZE;
    int entry = PG32(RVA_SHOP_LAST + block);
    if (entry <= entryBefore || entry >= SHOP_OFFERS_MAX) return;
    PG32(RVA_SHOP_OFFER_FLAGS + block + (size_t)entry * 4) = *(const int *)(record + RECRUIT_FLAGS);
    if (*(const int *)(record + RECRUIT_CHANCE) < REC_BONUS_MESSAGE_BELOW && *(const int *)(copy + RECRUIT_CHANCE) >= REC_BONUS_MESSAGE_BELOW)
        GAME_FN(void (*)(int), RVA_SHOP_OFFER_MESSAGE)(player);
}

// Points the "$" count call at recBonusUnitsCount and patches out the game's two steps, once; 1 when all are in.
#ifndef __APPLE__
#define REC_BONUS_SIB_NO_INDEX 0x20
#define REC_BONUS_STEP_3(live) (unsigned char)(REC_BONUS_SIB_NO_INDEX | (((live) >> 3) & 7))
#define REC_BONUS_STEP_9(live) (unsigned char)(REC_BONUS_SIB_NO_INDEX | ((live) & 7))
#define REC_BONUS_STEP_BYTE   2
#define REC_BONUS_STEP_SIZE   1
#else
#define REC_BONUS_STEP_3(live) patchedWord(live, 0x1fu << 5, 31u << 5)
#define REC_BONUS_STEP_9(live) REC_BONUS_STEP_3(live)
#define REC_BONUS_STEP_BYTE   0
#define REC_BONUS_STEP_SIZE   4
#endif
static int installRecBonusUnitsHooks(void) {
    if (recBonusRoll.unitsTried) return recBonusRoll.unitsHooked;
    recBonusRoll.unitsTried = 1;
    if (!gameCodeMatches) {
        writeLog("hook_skipped", "site=shop_units_count reason=game_code_differs");
        return 0;   // Logged
    }
    if (!installShopOfferHooks() ||
        !pointCustomPlaneCall(RVA_SHOP_UNITS_COUNT_CALL, RVA_COUNT_PLAYER_MONSTERS, (void *)recBonusUnitsCount, "shop_units_count"))
        return 0;
    unsigned char *steps[2] = {gameBase + RVA_SHOP_UNITS_STEP_3 + REC_BONUS_STEP_BYTE, gameBase + RVA_SHOP_UNITS_STEP_9 + REC_BONUS_STEP_BYTE};
#ifndef __APPLE__
    unsigned char live[2] = {steps[0][0], steps[1][0]};
#else
    uint32_t live[2];
    memcpy(&live[0], steps[0], 4);
    memcpy(&live[1], steps[1], 4);
#endif
    __typeof__(live[0]) patched[2] = {REC_BONUS_STEP_3(live[0]), REC_BONUS_STEP_9(live[1])};
    if (!writeGameCode(steps[0], &patched[0], REC_BONUS_STEP_SIZE)) {
        writeLog("hook_skipped", "site=shop_units_steps reason=code_unwritable");
        return 0;   // Logged
    }
    if (!writeGameCode(steps[1], &patched[1], REC_BONUS_STEP_SIZE)) {
        writeGameCode(steps[0], &live[0], REC_BONUS_STEP_SIZE);
        writeLog("hook_skipped", "site=shop_units_steps reason=code_unwritable");
        return 0;   // Logged
    }
    recBonusRoll.unitsHooked = 1;
    writeLog("rec_bonus_units_hooked", "ok=1");
    return 1;
}
