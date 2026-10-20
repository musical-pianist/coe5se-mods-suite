// The spells changed pop-up.

// The text, one line per string, so each is easy to change.
#define SPELLS_CHANGED_TITLE              "CoE5SE: Spells changed"
#define SPELLS_CHANGED_INTRO_1            "Mod spells changed since this save."
#define SPELLS_CHANGED_INTRO_2            "Save may behave oddly."
#define SPELLS_CHANGED_MOD                "Spells from %s:"
#define SPELLS_CHANGED_PAIR               "%s is now %s"
#define SPELLS_CHANGED_PAIR_AFTER_NUMBER  "%s became %s"
#define SPELLS_CHANGED_NO_SPELL           "no spell"
#define SPELLS_CHANGED_OTHER_MOD          "%.47s (%.79s)"
#define SPELLS_CHANGED_UNKNOWN_MOD        "an unknown mod"
#define SPELLS_CHANGED_MORE               "And %d more."
#define SPELLS_CHANGED_END                "Previous mod version keeps compatibility."
#define SPELLS_CHANGED_LOAD               "Load anyway"
#define SPELLS_CHANGED_BACK               "Back to the menu"

// Lines of text the pop-up has at most, blank lines and mod headers included.
#define SPELLS_CHANGED_TEXT_LINES 30
// The lines that aren't the list.
#define SPELLS_CHANGED_FIXED_LINES 5

enum { SPELLS_CHANGED_CHOICE_LOAD_ANYWAY = 1 };

static int isNameEndingInDigit(const char *name) {
    size_t length = strlen(name);
    return length && name[length - 1] >= '0' && name[length - 1] <= '9';
}

// Writes one changed spell's line: "<old> is now <new>".
static void appendChangedSpellLine(char *text, size_t size, size_t *n, const ChangedSpell *c) {
    char now[SPELL_NAME_MAX + 96];
    if (!c->newName[0]) snprintf(now, sizeof now, SPELLS_CHANGED_NO_SPELL);
    else if (isSameNameIgnoringCase(c->newName, c->oldName)) {
        char label[80];
        modDisplayName(c->newMod, label, sizeof label);
        snprintf(now, sizeof now, SPELLS_CHANGED_OTHER_MOD, c->newName, label);
    } else snprintf(now, sizeof now, "%.47s", c->newName);
    char old[SPELL_NAME_MAX + 1];
    snprintf(old, sizeof old, "%.47s", c->oldName);
    appendText(text, size, n, isNameEndingInDigit(old) ? SPELLS_CHANGED_PAIR_AFTER_NUMBER "\n" : SPELLS_CHANGED_PAIR "\n", old, now);
}

// Writes the pop-up's text for the `count` changed spells.
static void writeChangedSpellsText(char *text, size_t size, int count, const ChangedSpell *(*entryAt)(int)) {
    size_t n = 0;
    text[0] = '\0';
    appendText(text, size, &n, SPELLS_CHANGED_INTRO_1 "\n" SPELLS_CHANGED_INTRO_2 "\n\n");
    const char *lastMod = NULL;
    int shown = 0, lines = 0;
    const int budget = SPELLS_CHANGED_TEXT_LINES - SPELLS_CHANGED_FIXED_LINES;
    for (int i = 0; i < count; i++) {
        const ChangedSpell *c = entryAt(i);
        if (!c) break;
        int newMod = !lastMod || strcmp(lastMod, c->mod);
        int cost = !newMod ? 1 : lastMod ? 3 : 2;
        int andLines = i + 1 < count ? 2 : 0;
        if (lines + cost + andLines > budget) break;
        lines += cost;
        if (newMod) {
            char label[80];
            if (strcmp(c->mod, "-")) modDisplayName(c->mod, label, sizeof label);
            else snprintf(label, sizeof label, SPELLS_CHANGED_UNKNOWN_MOD);
            appendText(text, size, &n, "%s" SPELLS_CHANGED_MOD "\n", lastMod ? "\n" : "", label);
            lastMod = c->mod;
        }
        appendChangedSpellLine(text, size, &n, c);
        shown++;
    }
    if (count > shown) appendText(text, size, &n, "\n" SPELLS_CHANGED_MORE "\n", count - shown);
    appendText(text, size, &n, "\n" SPELLS_CHANGED_END);
}

// Asks the player whether to play a loaded save whose units hold mods' spells that changed.
static int askAboutChangedSpells(void) {
    int count = changedSpellTotal();
    if (!count || !isLoadedGame()) {
        clearChangedSpells();
        return 1;
    }
    int mods = 0;
    for (int i = 0; i < count; i++)
        if (!i || strcmp(changedSpellAt(i)->mod, changedSpellAt(i - 1)->mod)) mods++;
    static char text[POPUP_TEXT_SIZE];
    writeChangedSpellsText(text, sizeof text, count, changedSpellAt);
    writeLog("spells_changed_popup", "mods=%d spells=%d", mods, count);
    int choice = messageBox(SPELLS_CHANGED_TITLE, text, SPELLS_CHANGED_LOAD, SPELLS_CHANGED_BACK, 0, 0, 0);
    clearChangedSpells();
    if (choice == SPELLS_CHANGED_CHOICE_LOAD_ANYWAY) {
        writeLog("spells_changed_choice", "choice=load_anyway");
        return 1;
    }
    writeLog("spells_changed_choice", "choice=back_to_menu");
    GAME_FN(void (*)(void), RVA_RESET_GAME_STATE)();
    clearLoadedGame();
    return 0;
}
