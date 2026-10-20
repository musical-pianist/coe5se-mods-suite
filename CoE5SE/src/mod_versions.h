// Mods whose version differs from the one that made the current turn.

#define MOD_VERSIONS_TITLE  "CoE5SE: Mod versions differ"
#define MOD_VERSIONS_LINES  16
static struct {
    char (*mods)[MOD_NAME_MAX];
    int count, capacity;
} modVersionsDiffer;

// Adds mod `name` to the list, once.
static void noteModVersionDiffers(const char *name) {
    for (int i = 0; i < modVersionsDiffer.count; i++)
        if (!strcmp(modVersionsDiffer.mods[i], name)) return;
    writeLog("mod_version_differs", "mod=%s", name);
    if (!growList((void **)&modVersionsDiffer.mods, &modVersionsDiffer.capacity, modVersionsDiffer.count + 1,
                  sizeof *modVersionsDiffer.mods))
        return;
    snprintf(modVersionsDiffer.mods[modVersionsDiffer.count++], MOD_NAME_MAX, "%s", name);
}

// The box's text: one mod per line, by its display name.
static size_t modVersionsDifferText(char *text, size_t size) {
    size_t n = 0;
    char label[80];
    text[0] = '\0';
    appendText(text, size, &n, "Mod versions differ from this turn's:\n");
    for (int i = 0; i < modVersionsDiffer.count && i < MOD_VERSIONS_LINES; i++) {
        modDisplayName(modVersionsDiffer.mods[i], label, sizeof label);
        appendText(text, size, &n, "\n%s", label);
    }
    if (modVersionsDiffer.count > MOD_VERSIONS_LINES) appendText(text, size, &n, "\n...and others.");
    appendText(text, size, &n, "\n\nSame version required for all players.");
    return n;
}

// The game's checksum differs for mod `name` on a later turn.
static void modChecksumDiffers(const char *name) {
    if (findSavedModHash(name)) {
        writeLog("mod_checksum_differs", "mod=%s decided_by=content_hash", name);
        return;
    }
    noteModVersionDiffers(name);
}

// Compares the save's content hashes with each installed mod's files now.
static int compareModContentHashes(void) {
    if (strcmp(savedModHashes.result, "found")) return -1;
    if (G32(RVA_TURN) <= G32(RVA_MOD_CRC_FIRST_TURN)) return -1;
    int compared = 0, differ = 0;
    for (int i = 0; i < savedModHashes.count; i++) {
        const SavedModHash *saved = &savedModHashes.mods[i];
        int mod = modIndexExact(saved->name);
        if (mod < 0) continue;
        ModFileScan scan;
        compared++;
        if (scanMod(mod, &scan) && scan.hash == saved->hash) continue;
        differ++;
        noteModVersionDiffers(saved->name);
    }
    writeLog("mod_content_compared", "mods=%d differ=%d turn=%d", compared, differ, G32(RVA_TURN));
    return compared;
}

// After the mod list: with a differing mod, shows the box and ends the game.
static void modVersionsCheckDone(void) {
    compareModContentHashes();
    if (modVersionsDiffer.count) {
        static char text[POPUP_TEXT_SIZE];
        modVersionsDifferText(text, sizeof text);
        writeLog("mod_versions_differ_exit", "mods=%d", modVersionsDiffer.count);
        okBox(MOD_VERSIONS_TITLE, text);
        GAME_FN(void (*)(const char *), RVA_EXIT_GAME)("");
    }
}

static int modVersionsChecked;

// Where load_game turns to resetmods after the mod list.
static void finishLoadedModList(void) {
    keepPlayerModStates();
    if (modVersionsChecked) modVersionsCheckDone();
    GAME_FN(void (*)(void), RVA_RESET_MODS)();
}

#ifndef __APPLE__
// load_game's RVA_WARN_FORMAT(RVA_STR_MOD_DIFFERS, name).
static void modDiffersWarningHook(const char *format, const char *name) {
    (void)format;
    modChecksumDiffers(name);
}

#define MOD_VERSIONS_STUB_SIZE 0x20
#define WARNED_SET_REGISTER    2
#define WARNED_SET_FROM_R12D   0x25
static int installModVersionHooks(int *wanted) {
    static const unsigned char WARNED_SET[] = {0x44, 0x89, 0x3d, 0x33, 0xa5, 0x69, 0x05};
    *wanted = 3;
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!stubs) {
        writeLog("mod_version_hooks_skipped", "reason=no_near_page");
        return 0;
    }
    if (!redirectCall(RVA_LOAD_GAME_RESET_MODS_CALL, RVA_RESET_MODS, stubs, (void *)finishLoadedModList, "load_game_reset_mods", NULL))
        return 0;
    modListEndHooked = 1;
    unsigned char *flag = gameBase + RVA_LOAD_GAME_MOD_WARNED_SET;
    if (memcmp(flag, WARNED_SET, sizeof WARNED_SET)) {
        writeLog("hook_skipped", "site=load_game_mod_warned reason=not_the_expected_bytes");
        return 1;
    }
    modVersionsChecked = 1;
    if (!redirectCall(RVA_LOAD_GAME_MOD_DIFFERS_CALL, RVA_WARN_FORMAT, stubs + MOD_VERSIONS_STUB_SIZE, (void *)modDiffersWarningHook,
                      "load_game_mod_differs", NULL))
        return 1;
    DWORD old;
    VirtualProtect(flag + WARNED_SET_REGISTER, 1, PAGE_EXECUTE_READWRITE, &old);
    flag[WARNED_SET_REGISTER] = WARNED_SET_FROM_R12D;
    VirtualProtect(flag + WARNED_SET_REGISTER, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), flag, sizeof WARNED_SET);
    return 3;
}
#else
// load_game's RVA_WARN_FORMAT(RVA_STR_MOD_DIFFERS, name) on Mac.
static void modDiffersWarningHook(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    modChecksumDiffers(va_arg(ap, const char *));
    va_end(ap);
}

static int installModVersionHooks(int *wanted) {
    const uint32_t WARNED_ONE = 0x52800029, WARNED_ZERO = 0x52800009;
    *wanted = 3;
    uint32_t word;
    if (!pointMissingModsCall(RVA_LOAD_GAME_RESET_MODS_CALL, RVA_RESET_MODS, (void *)finishLoadedModList, "load_game_reset_mods"))
        return 0;
    modListEndHooked = 1;
    memcpy(&word, gameBase + RVA_LOAD_GAME_MOD_WARNED_SET, 4);
    if (word != WARNED_ONE) {
        writeLog("hook_skipped", "site=load_game_mod_warned reason=not_the_expected_bytes");
        return 1;
    }
    modVersionsChecked = 1;
    if (!pointMissingModsCall(RVA_LOAD_GAME_MOD_DIFFERS_CALL, RVA_WARN_FORMAT, (void *)modDiffersWarningHook, "load_game_mod_differs"))
        return 1;
    if (!writeGameCode(gameBase + RVA_LOAD_GAME_MOD_WARNED_SET, &WARNED_ZERO, 4)) {
        writeLog("hook_skipped", "site=load_game_mod_warned reason=code_unwritable");
        return 2;
    }
    return 3;
}
#endif

// Installs the hooks.
static void modVersionsInit(void) {
    int wanted = 0, ok = installModVersionHooks(&wanted);
    writeLog("mod_version_hooks_installed", "ok=%d of %d", ok, wanted);
}
