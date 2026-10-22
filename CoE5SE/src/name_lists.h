// Mods' own commander name lists, and fixed names.
#pragma once

#define NEWNAMETYPE_WORD   "newnametype"
#define ADDNAME_WORD       "addname"
#define FIXEDNAME_WORD     "fixedname"
#define NAMETYPE_WORD      "nametype"
#define NAME_LIST_NAME_MAX 64
#define NAME_TEXT_MAX      (COMMANDER_NAME_SIZE - 1)

typedef struct {
    char list[NAME_LIST_NAME_MAX];
    char **names;
    int count, capacity;
} NameList;

static struct {
    NameList *lists;
    int count, capacity;
    int open;
    int lastLevel, lastMod;
    int passes;
} nameLists = {NULL, 0, 0, -1, -1, -1, 0};

static char nameListLine[OFFSET_NAME_LINE_MAX];

static void clearNameLists(void) {
    for (int i = 0; i < nameLists.count; i++) {
        for (int n = 0; n < nameLists.lists[i].count; n++) free(nameLists.lists[i].names[n]);
        free(nameLists.lists[i].names);
    }
    nameLists.count = 0;
    nameLists.open = -1;
}

static void closeNameList(void) {
    if (nameLists.open >= 0 && !nameLists.lists[nameLists.open].count)
        writeLog("name_list_empty", "list=%s line=%d", nameLists.lists[nameLists.open].list, G32(RVA_MOD_LINE));
    nameLists.open = -1;
}

// The list called `list`, ignoring case, as the game compares names; -1 when there is none.
static int nameListByName(const char *list) {
    for (int i = 0; i < nameLists.count; i++)
        if (nameLists.lists[i].list[0] && !_stricmp(nameLists.lists[i].list, list)) return i;
    return -1;
}

// A new empty list; -1 when out of memory (logged).
static int addNameList(const char *list) {
    if (!growList((void **)&nameLists.lists, &nameLists.capacity, nameLists.count + 1, sizeof nameLists.lists[0])) {
        writeLog("name_list_skipped", "list=%s reason=out_of_memory", list);
        return -1;
    }
    NameList *l = &nameLists.lists[nameLists.count];
    memset(l, 0, sizeof *l);
    snprintf(l->list, sizeof l->list, "%s", list);
    return nameLists.count++;
}

static int addNameToList(int index, const char *name) {
    NameList *l = &nameLists.lists[index];
    char *copy = strdup(name);
    if (!copy || !growList((void **)&l->names, &l->capacity, l->count + 1, sizeof l->names[0])) {
        free(copy);
        writeLog("name_skipped", "name=%s reason=out_of_memory", name);
        return 0;
    }
    l->names[l->count++] = copy;
    return 1;
}

// Reads a word's quoted text into `out`; returns its length, or -1 when refused.
static int readNameText(const char *word, const char *args, char *out, size_t size, size_t max, int mod) {
    const char *p = args;
    int len = readSpellQuoted(&p, out, size);
    if (len <= 0) {
        refuseC5mLine(word, spellModName(mod), "no_name", "line=%d text=%.80s", G32(RVA_MOD_LINE), args);
        return -1;
    }
    if ((size_t)len > max) {
        refuseC5mLine(word, spellModName(mod), "name_too_long", "line=%d text=%.80s max=%zu", G32(RVA_MOD_LINE), args, max);
        return -1;
    }
    return len;
}

// newnametype "<list>": opens a name list.
static void newnametypeLine(const char *args, int mod, int level) {
    if (level != 2) return;
    closeNameList();
    char list[NAME_LIST_NAME_MAX];
    if (readNameText(NEWNAMETYPE_WORD, args, list, sizeof list, sizeof list - 1, mod) < 0) return;
    int index = nameListByName(list);
    if (index < 0) index = addNameList(list);
    else writeLog("name_list_reopened", "list=%s mod=%s line=%d", list, spellModName(mod), G32(RVA_MOD_LINE));
    nameLists.open = index;
}

// addname "<name>": adds a name to the open list.
static void addnameLine(const char *args, int mod, int level) {
    if (level != 2) return;
    char name[COMMANDER_NAME_SIZE + 1];
    if (nameLists.open < 0) {
        refuseC5mLine(ADDNAME_WORD, spellModName(mod), "no_name_list", "line=%d text=%.80s", G32(RVA_MOD_LINE), args);
        return;
    }
    if (readNameText(ADDNAME_WORD, args, name, sizeof name, NAME_TEXT_MAX, mod) < 0) return;
    addNameToList(nameLists.open, name);
}

// A list's type number: what nametype stores for it.
static int nameListType(int index) { return AUTO_NAME_TYPES + index; }

// `nametype <type>` in nameListLine for the game to read; NULL when it doesn't fit (logged).
static const char *nametypeLineFor(const char *word, int type, int mod, const char *text) {
    int n = snprintf(nameListLine, sizeof nameListLine, NAMETYPE_WORD " %d", type);
    if (n < 0 || (size_t)n >= sizeof nameListLine) {
        refuseC5mLine(word, spellModName(mod), "line_too_long", "line=%d name=%s", G32(RVA_MOD_LINE), text);
        return NULL;
    }
    return nameListLine;
}

// Every .c5m and .c5se line outside events comes here after offsetNameLineHook.
static const char *nameListLineHook(const char *line, size_t len, int mod, int level) {
    if (level == 2 && nameLists.lastLevel != 2) {
        clearNameLists();
        nameLists.passes++;
    }
    if (level != nameLists.lastLevel || mod != nameLists.lastMod) closeNameList();
    nameLists.lastLevel = level;
    nameLists.lastMod = mod;
    if (level != 2) return line;
    if (nameLists.open >= 0 && !spellWordIs(line, len, ADDNAME_WORD)) {
        closeNameList();
        if (spellWordIs(line, len, "end")) return NULL;
    }
    int fixed = spellWordIs(line, len, FIXEDNAME_WORD);
    if (!fixed && !spellWordIs(line, len, NAMETYPE_WORD)) return line;
    const char *args = line + len;
    skipSpaces(&args);
    if (!fixed && args[0] != '"') return line;
    const char *word = fixed ? FIXEDNAME_WORD : NAMETYPE_WORD;
    char text[COMMANDER_NAME_SIZE + NAME_LIST_NAME_MAX];
    if (readNameText(word, args, text, sizeof text, fixed ? NAME_TEXT_MAX : NAME_LIST_NAME_MAX - 1, mod) < 0) return NULL;
    if (G32(RVA_MONSTER_CURRENT) < 0) {
        refuseC5mLine(word, spellModName(mod), "no_monster", "line=%d name=%s", G32(RVA_MOD_LINE), text);
        return NULL;
    }
    int index;
    if (fixed) {
        index = -1;
        for (int i = 0; i < nameLists.count && index < 0; i++)
            if (!nameLists.lists[i].list[0] && nameLists.lists[i].count == 1 && !strcmp(nameLists.lists[i].names[0], text)) index = i;
        if (index < 0 && (index = addNameList("")) >= 0 && !addNameToList(index, text)) index = -1;
        if (index < 0) return NULL;
    } else if ((index = nameListByName(text)) < 0) {
        refuseC5mLine(word, spellModName(mod), "unknown_name_list", "line=%d list=%s", G32(RVA_MOD_LINE), text);
        return NULL;
    }
    writeLog("name_list_used", "mod=%s word=%s name=%s monster=%d type=%d line=%d", spellModName(mod), word, text,
             G32(RVA_MONSTER_CURRENT), nameListType(index), G32(RVA_MOD_LINE));
    return nametypeLineFor(word, nameListType(index), mod, text);
}

// The same when no plugin loaded; returns 1 to skip the line.
static int declinedNameListLine(const char *line, size_t len, int level) {
    if (level != 2 || !spellWordIs(line, len, NAMETYPE_WORD)) return 0;
    const char *args = line + len;
    skipSpaces(&args);
    return args[0] == '"';
}

// ---- The game's auto_name --------------------------------------------------------------------------
// auto_name(type, out) for a mod's list.
typedef void (*AutoNameFn)(int type, char *out);
static int (*nameListRandom)(int n);
static AutoNameFn gameAutoName;
static void autoNameHook(int type, char *out) {
    int index = type - AUTO_NAME_TYPES;
    if (index >= 0 && index < nameLists.count && nameLists.lists[index].count > 0) {
        const NameList *l = &nameLists.lists[index];
        int pick = nameListRandom(l->count);
        if (pick < 0 || pick >= l->count) pick = 0;
        snprintf(out, COMMANDER_NAME_SIZE, "%s", l->names[pick]);
        return;
    }
    gameAutoName(type, out);
}

#ifndef __aarch64__
#define NAME_LIST_HOOKS     6
#define NAME_LIST_STUB_SIZE 0x20
// Points the tail jump at `siteRva` to `hook`.
static int redirectTailJump(unsigned int siteRva, unsigned int targetRva, unsigned char *stub, void *hook, const char *label) {
    unsigned char *site = gameBase + siteRva;
    int rel;
    memcpy(&rel, site + 1, 4);
    if (site[0] != 0xe9 || site + 5 + rel != gameBase + targetRva) {
        writeLog("hook_skipped", "site=%s reason=not_the_expected_call address=%p", label, site);
        return 0;
    }
    static const unsigned char JMP_RIP[6] = {0xff, 0x25, 0, 0, 0, 0};
    memcpy(stub, JMP_RIP, sizeof JMP_RIP);
    memcpy(stub + sizeof JMP_RIP, &hook, 8);
    rel = (int)(stub - (site + 5));
    DWORD old;
    VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old);
    memcpy(site + 1, &rel, 4);
    VirtualProtect(site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    return 1;
}

static int installNameListHooks(void) {
    static const struct { unsigned int callRva; const char *name; int tail; } CALLS[NAME_LIST_HOOKS] = {
        {RVA_AUTO_NAME_CALL_COSTLIST, "auto_name_costlist", 0},       {RVA_AUTO_NAME_CALL_PLAYER, "auto_name_player", 1},
        {RVA_AUTO_NAME_CALL_HELPER, "auto_name_helper", 0},           {RVA_AUTO_NAME_CALL_NEW_COMMANDER, "auto_name_new_commander", 0},
        {RVA_AUTO_NAME_CALL_NET_SETUP, "auto_name_net_setup", 0},     {RVA_AUTO_NAME_CALL_SETUP, "auto_name_setup", 0},
    };
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!stubs) {
        writeLog("name_list_hooks_skipped", "reason=no_near_page");
        return 0;
    }
    int ok = 0;
    for (int i = 0; i < NAME_LIST_HOOKS; i++) {
        unsigned char *stub = stubs + NAME_LIST_STUB_SIZE * i;
        if (CALLS[i].tail) ok += redirectTailJump(CALLS[i].callRva, RVA_AUTO_NAME, stub, (void *)autoNameHook, CALLS[i].name);
        else ok += redirectCall(CALLS[i].callRva, RVA_AUTO_NAME, stub, (void *)autoNameHook, CALLS[i].name, NULL);
    }
    return ok;
}
#else
#define NAME_LIST_HOOKS 7
// Where the `b` at `at` goes, or NULL if the instruction isn't a `b`.
static unsigned char *bTarget(const unsigned char *at) {
    uint32_t insn;
    memcpy(&insn, at, 4);
    if ((insn & 0xfc000000u) != 0x14000000u) return NULL;
    int32_t imm = (int32_t)(insn << 6) >> 6;
    return (unsigned char *)at + (intptr_t)imm * 4;
}

static int installNameListHooks(void) {
    static const struct { unsigned int callRva; const char *name; int tail; } CALLS[NAME_LIST_HOOKS] = {
        {RVA_AUTO_NAME_CALL_COSTLIST, "auto_name_costlist", 0},       {RVA_AUTO_NAME_CALL_PLAYER, "auto_name_player", 1},
        {RVA_AUTO_NAME_CALL_HELPER, "auto_name_helper", 0},           {RVA_AUTO_NAME_CALL_NEW_COMMANDER, "auto_name_new_commander", 0},
        {RVA_AUTO_NAME_CALL_NET_SETUP, "auto_name_net_setup", 0},     {RVA_AUTO_NAME_CALL_SETUP, "auto_name_setup", 0},
        {RVA_AUTO_NAME_CALL_SERVER, "auto_name_server", 0},
    };
    unsigned char *trampoline = newTrampoline((void *)autoNameHook);
    if (!trampoline) {
        writeLog("name_list_hooks_skipped", "reason=no_padding");
        return 0;
    }
    int ok = 0;
    for (int i = 0; i < NAME_LIST_HOOKS; i++) {
        unsigned char *call = gameBase + CALLS[i].callRva;
        unsigned char *target = CALLS[i].tail ? bTarget(call) : blTarget(call);
        if (target != gameBase + RVA_AUTO_NAME) {
            writeLog("hook_skipped", "name=%s va=0x%llx reason=not_the_expected_call", CALLS[i].name, CALLS[i].callRva + 0x100000000ULL);
            continue;
        }
        uint32_t insn = CALLS[i].tail ? 0x14000000u | ((uint32_t)((trampoline - call) >> 2) & 0x3ffffffu) : blTo(call, trampoline);
        if (writeGameCode(call, &insn, 4)) ok++;
        else writeLog("hook_skipped", "name=%s va=0x%llx reason=code_unwritable", CALLS[i].name, CALLS[i].callRva + 0x100000000ULL);
    }
    return ok;
}
#endif

// fixedname at a level the game reads no content: nothing to do.
static void fixednameLine(const char *args, int mod, int level) {
    (void)args; (void)mod; (void)level;
}

// Adds the words; returns how many were added.
static int nameListWordsInit(void) {
    return apiAddModCommand("newnametype", newnametypeLine) + apiAddModCommand("addname", addnameLine) +
           apiAddModCommand("fixedname", fixednameLine);
}

// Adds the words and hooks every call of auto_name.
static void nameListsInit(void) {
    nameListRandom = GAME_FN(int (*)(int), RVA_RANDOM);
    gameAutoName = GAME_FN(AutoNameFn, RVA_AUTO_NAME);
    int words = nameListWordsInit();
    writeLog("name_list_words_installed", "words=%d of 3 hooks=%d of %d", words, installNameListHooks(), NAME_LIST_HOOKS);
}
