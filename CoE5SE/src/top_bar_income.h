// TOP_BAR_INCOME: the income number the map's top bar draws beside a resource, as a script's wraps answer it.

// The resource each covered call draws, by the call's address.
typedef struct { unsigned rva; int resource; } TopBarIncomeCall;
static const TopBarIncomeCall TOP_BAR_INCOME_CALLS[] = {
    {0x09c9f1, 0}, {0x09cad0, 1}, {0x09ce18, 3}, {0x09cef5, 2}, {0x09cfc9, 6},
    {0x09d09d, 12}, {0x09d171, 4}, {0x09d260, 5}, {0x09d34a, 13},
};
#define TOP_BAR_INCOME_CALL_COUNT ((int)(sizeof TOP_BAR_INCOME_CALLS / sizeof TOP_BAR_INCOME_CALLS[0]))
#define CALL_SIZE 5

// The resource the call returning to `returnTo` draws, or -1 for none of them.
static UNUSED int topBarIncomeResource(uintptr_t returnTo, uintptr_t base) {
    for (int i = 0; i < TOP_BAR_INCOME_CALL_COUNT; i++)
        if (returnTo == base + TOP_BAR_INCOME_CALLS[i].rva + CALL_SIZE) return TOP_BAR_INCOME_CALLS[i].resource;
    return -1;
}

// TOP_BAR_INCOME's end, the game's own answer: the income the bar would draw.
static UNUSED int topBarIncomeAsGame(int player, int resource, int income) {
    (void)player, (void)resource;
    return income;
}

#ifndef __aarch64__
static void (*nextTopBar)(int, int);
static u64 (*nextTopBarIncomeDraw)(int, float, float, float, const char *, int, int);
static int topBarPlayer = -1;

// CoE5SE's wrapper of map_screen's top bar call: notes the bar's player.
static void noteTopBarPlayer(int player, int flag) {
    int outer = topBarPlayer;
    topBarPlayer = player;
    nextTopBar(player, flag);
    topBarPlayer = outer;
}

// CoE5SE's wrapper of the bar's income draws: the number TOP_BAR_INCOME's chain answers, the rest as the game's.
static u64 drawTopBarIncome(int id, float x, float y, float size, const char *format, int stock, int income) {
    uintptr_t returnTo = *(const uintptr_t *)(siteCallerSp(SITE_TOP_BAR_INCOME_DRAW) - sizeof(uintptr_t));
    int resource = topBarIncomeResource(returnTo, (uintptr_t)gameBase);
    int drawn = income;
    if (resource >= 0 && topBarPlayer >= 0 && siteInstalled[SITE_TOP_BAR_INCOME])
        drawn = ((int (*)(int, int, int))siteEntry(SITE_TOP_BAR_INCOME, (void *)topBarIncomeAsGame))(topBarPlayer, resource, income);
    return ((u64 (*)(int, float, float, float, const char *, ...))nextTopBarIncomeDraw)(id, x, y, size, format, stock, drawn);
}

// Installs the two internal wraps, once, when TOP_BAR_INCOME is first wrapped (prepareSiteCode).
static int installTopBarIncomeDraws(void) {
    static int done;
    if (done) return done > 0;
    char owner[64];
    moduleFileName((void *)drawTopBarIncome, owner, sizeof owner);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int bar = insertWrap(SITE_TOP_BAR, (void *)noteTopBarPlayer, (void **)&nextTopBar, outermost, owner, NULL);
    int draws = bar && insertWrap(SITE_TOP_BAR_INCOME_DRAW, (void *)drawTopBarIncome, (void **)&nextTopBarIncomeDraw, outermost, owner, NULL);
    done = bar && draws ? 1 : -1;
    writeLog("top_bar_income_installed", "bar=%d draws=%d calls=%d", bar, draws, TOP_BAR_INCOME_CALL_COUNT);
    return done > 0;
}
#endif
