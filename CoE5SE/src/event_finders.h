// +hasunithere and +hasitemhere, answered from a list of each square's units.
#pragma once
#include "event_finder_shadow.h"

#define FINDER_MATCHES_MAX 10000
#define FINDER_ANY_PLAYER  (-1)
#define FINDER_NO_UNIT     (-1)

static struct {
    int lineDepth;
    int built;
    int width, height;
    int installed;
    EventFinderCounts counts;
} eventFinders;

// The square lists: the units on each square of the map.
static int finderHead[MAP_COLUMNS_MAX * MAP_COLUMN_SQUARES];
static int finderNext[UNIT_TABLE_MAX];

// The game's code this calls.
static EventFinderFn finderGame[EVENT_FINDER_KINDS];
static int (*finderRandom)(int n);
static int (*finderArmyHere)(int player, int x, int y);
static EventFinderShadowFn finderShadow;

// Called by eventLineHook around each line it passes to the game's event runner.
static void eventFindersLineBegin(void) {
    eventFinders.lineDepth++;
    eventFinders.built = 0;
}

static void eventFindersLineEnd(void) {
    eventFinders.lineDepth--;
    eventFinders.built = 0;
}

// Builds the square lists from the units now.
static int buildFinderLists(void) {
    int width = G32(RVA_MAP_DEPTH), height = G32(RVA_MAP_H), last = G32(RVA_UNIT_COUNT);
    if (last >= UNIT_TABLE_MAX) return 0;
    if (width > MAP_COLUMNS_MAX) width = MAP_COLUMNS_MAX;
    if (height > MAP_COLUMN_SQUARES) height = MAP_COLUMN_SQUARES;
    if (width < 0) width = 0;
    if (height < 0) height = 0;
    memset(finderHead, 0xff, (size_t)width * MAP_COLUMN_SQUARES * sizeof finderHead[0]);
    for (int u = last; u >= 0; u--) {
        size_t at = (size_t)u * UNIT_SIZE;
        if ((short)G16(RVA_UNIT_HP + at) <= 0) continue;
        int x = (int)G32(RVA_UNIT_X + at), y = (int)G32(RVA_UNIT_X + at + 4);
        if (x < 0 || y < 0 || x >= width || y >= height) continue;
        size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
        finderNext[u] = finderHead[square];
        finderHead[square] = u;
    }
    eventFinders.width = width;
    eventFinders.height = height;
    eventFinders.built = 1;
    eventFinders.counts.builds++;
    return 1;
}

// 1 if unit u is the player's and has the monster's name, or holds an item with the item's name.
static int finderUnitMatches(int kind, int u, int player, const char *name) {
    size_t at = (size_t)u * UNIT_SIZE;
    if (player != FINDER_ANY_PLAYER && (short)G16(RVA_UNIT_PLAYER + at) != player) return 0;
    if (kind == EVENT_FINDER_UNIT) {
        if (!name[0]) return 1;
        long monster = (short)G16(RVA_UNIT_MONSTER + at);
        return !strcmp((const char *)(tableAddress(TABLE_MONSTER) + monster * MONSTER_SIZE), name);
    }
    for (int i = 0; i < UNIT_ITEM_SLOTS; i++) {
        short item = (short)G16(RVA_UNIT_ITEMS + at + (size_t)i * 2);
        if (item >= 0 && !strcmp((const char *)(tableAddress(TABLE_ITEM) + (size_t)item * ITEM_SIZE), name)) return 1;
    }
    return 0;
}

// The game's finder of that kind, answered from the square lists when it can be.
static int answerEventFinder(int kind, int player, int x, int y, const char *name) {
    if (x < 0 || eventFinders.lineDepth <= 0) {
        eventFinders.counts.passed++;
        return finderGame[kind](player, x, y, name);
    }
    if (!finderArmyHere(player, x, y)) {
        eventFinders.counts.answered++;
        return FINDER_NO_UNIT;
    }
    if ((!eventFinders.built && !buildFinderLists()) || y < 0 || x >= eventFinders.width || y >= eventFinders.height) {
        eventFinders.counts.passed++;
        return finderGame[kind](player, x, y, name);
    }
    eventFinders.counts.answered++;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    int n = 0;
    for (int u = finderHead[square]; u != FINDER_NO_UNIT && n < FINDER_MATCHES_MAX; u = finderNext[u])
        if (finderUnitMatches(kind, u, player, name)) n++;
    if (!n) return FINDER_NO_UNIT;
    int pick = finderRandom(n);
    for (int u = finderHead[square]; u != FINDER_NO_UNIT; u = finderNext[u])
        if (finderUnitMatches(kind, u, player, name) && pick-- == 0) return u;
    return FINDER_NO_UNIT;
}

static int findUnitFromLists(int player, int x, int y, const char *name) {
    return answerEventFinder(EVENT_FINDER_UNIT, player, x, y, name);
}

static int findItemHolderFromLists(int player, int x, int y, const char *name) {
    return answerEventFinder(EVENT_FINDER_ITEM, player, x, y, name);
}

// Where the event runner's four calls go.
static int findUnitAtHook(int player, int x, int y, const char *name) {
    EventFinderShadowFn shadow = finderShadow;
    if (shadow) return shadow(EVENT_FINDER_UNIT, player, x, y, name, finderGame[EVENT_FINDER_UNIT], findUnitFromLists);
    return findUnitFromLists(player, x, y, name);
}

static int findItemHolderAtHook(int player, int x, int y, const char *name) {
    EventFinderShadowFn shadow = finderShadow;
    if (shadow) return shadow(EVENT_FINDER_ITEM, player, x, y, name, finderGame[EVENT_FINDER_ITEM], findItemHolderFromLists);
    return findItemHolderFromLists(player, x, y, name);
}

// DevTools' findershadow.
__declspec(dllexport) const EventFinderCounts *coe5seEventFinderShadow(EventFinderShadowFn shadow) {
    if (!eventFinders.installed) return NULL;
    finderShadow = shadow;
    writeLog("event_finders_shadow", "on=%d", shadow != NULL);
    return &eventFinders.counts;
}

#ifdef __APPLE__
// @platform-split event_finder_army_test: Mac runs CoE5SE's copy of the army test its finders inline
// RVA_ARMY_AT_FOR_PLAYER and RVA_SAME_TEAM as the game's code has them.
static int armyHereForPlayer(int player, int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    short army = (short)G16(RVA_SQUARE_ARMY + square * 2);
    if (!army) return 0;
    if (player < 0 || army < 0) return 1;
    int owner = *(const short *)(gameBase + RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_ARMY_OWNER);
    if (owner < 0 || owner >= PLAYER_SLOTS || player >= PLAYER_SLOTS) return 0;
    if (owner == player) return 1;
    short ownerTeam = (short)G16(RVA_SLOT_TEAM + (size_t)owner * PLAYER_BLOCK_SIZE);
    short playerTeam = (short)G16(RVA_SLOT_TEAM + (size_t)player * PLAYER_BLOCK_SIZE);
    return ownerTeam > 0 && playerTeam > 0 && ownerTeam == playerTeam;
}
#endif

// The four calls: the event runner's +hasunit, +hasunithere, +hasitem, +hasitemhere.
typedef struct {
    unsigned int callRva, targetRva;
    void *hook;
    const char *name;
} EventFinderCall;

#ifndef __APPLE__
#define EVENT_FINDERS_STUB_SIZE 0x20
static int pointEventFinderCall(const EventFinderCall *c, int index) {
    static unsigned char *stubs;
    if (!stubs && !(stubs = allocNear(gameBase + RVA_TEXT_SECTION))) {
        writeLog("hook_skipped", "site=%s reason=no_near_page", c->name);
        return 0;
    }
    return redirectCall(c->callRva, c->targetRva, stubs + EVENT_FINDERS_STUB_SIZE * index, c->hook, c->name, NULL);
}
#else
static int pointEventFinderCall(const EventFinderCall *c, int index) {
    (void)index;
    if (blTarget(gameBase + c->callRva) != gameBase + c->targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call va=0x%llx", c->name, c->callRva + 0x100000000ULL);
        return 0;
    }
    unsigned char *trampoline = newTrampoline(c->hook);
    uint32_t word = trampoline ? blTo(gameBase + c->callRva, trampoline) : 0;
    if (trampoline && writeGameCode(gameBase + c->callRva, &word, 4)) return 1;
    writeLog("hook_skipped", "site=%s reason=%s", c->name, trampoline ? "code_unwritable" : "no_padding");
    return 0;
}
#endif

// Points the event runner's finder calls at the answers above, and logs event_finders_installed.
static void eventFindersInit(void) {
    finderGame[EVENT_FINDER_UNIT] = GAME_FN(EventFinderFn, RVA_FIND_UNIT_AT);
    finderGame[EVENT_FINDER_ITEM] = GAME_FN(EventFinderFn, RVA_FIND_ITEM_HOLDER_AT);
    finderRandom = GAME_FN(int (*)(int), RVA_RANDOM);
#ifdef __APPLE__
    finderArmyHere = armyHereForPlayer;
#else
    finderArmyHere = GAME_FN(int (*)(int, int, int), RVA_ARMY_AT_FOR_PLAYER);
#endif
    if (!gameBuild) {
        writeLog("event_finders_installed", "ok=0 of 4 reason=unknown_game_build");
        return;
    }
    if (!checkEventFinderCode()) {
        writeLog("event_finders_installed", "ok=0 of 4 reason=game_code_changed");
        return;
    }
    static const EventFinderCall CALLS[] = {
        {RVA_EVENT_HASUNIT_CALL, RVA_FIND_UNIT_AT, (void *)findUnitAtHook, "event_hasunit"},
        {RVA_EVENT_HASUNITHERE_CALL, RVA_FIND_UNIT_AT, (void *)findUnitAtHook, "event_hasunithere"},
        {RVA_EVENT_HASITEM_CALL, RVA_FIND_ITEM_HOLDER_AT, (void *)findItemHolderAtHook, "event_hasitem"},
        {RVA_EVENT_HASITEMHERE_CALL, RVA_FIND_ITEM_HOLDER_AT, (void *)findItemHolderAtHook, "event_hasitemhere"},
    };
    int ok = 0;
    for (int i = 0; i < (int)(sizeof CALLS / sizeof CALLS[0]); i++) ok += pointEventFinderCall(&CALLS[i], i);
    eventFinders.installed = ok > 0;
    writeLog("event_finders_installed", "ok=%d of 4", ok);
}
