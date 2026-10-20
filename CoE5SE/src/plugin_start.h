// Starting plugin DLLs: the checks before any of a plugin's code runs.
#include "address_ids.h"
#include "address_table.h"
#ifdef __APPLE__
#include <mach-o/getsect.h>
#endif

_Static_assert(ADDRESS_TABLE_FORMAT == ADDRESS_ID_FORMAT, "address_table.h is from another format; generate it again from game_addresses.h");

#define GAME_VERSION_WORD   "coeversion"
#define GAME_VERSION_ANY    "any"

typedef struct {
    char file[MAX_PATH];
    char path[MAX_PATH];
    char mod[MOD_NAME_MAX];
    HMODULE module;
    int versionChecked, addressesChecked, entryCalled;
    const char *reason;
    char detail[240];
    char declared[120];
} PluginRecord;
static PluginRecord **pluginRecords;
static int pluginRecordCount, pluginRecordCapacity;

static const char *fileNameOf(const char *path) {
    const char *slash = strrchr(path, '\\'), *other = strrchr(path, '/');
    if (other > slash) slash = other;
    return slash ? slash + 1 : path;
}

// The record for the DLL at `path`, made on first use.
static PluginRecord *pluginRecordFor(const char *path) {
    const char *file = fileNameOf(path);
    for (int i = 0; i < pluginRecordCount; i++)
        if (!_stricmp(pluginRecords[i]->file, file)) return pluginRecords[i];
    PluginRecord *r = calloc(1, sizeof *r);
    if (!r || !growList((void **)&pluginRecords, &pluginRecordCapacity, pluginRecordCount + 1, sizeof r)) {
        free(r);
        writeLog("plugin_record_failed", "reason=out_of_memory path=%s", path);
        return NULL;
    }
    snprintf(r->file, sizeof r->file, "%s", file);
    snprintf(r->path, sizeof r->path, "%s", path);
    pluginRecords[pluginRecordCount++] = r;
    return r;
}

// The record of a loaded module, or NULL for a DLL the loader didn't load.
static PluginRecord *pluginRecordOfModule(HMODULE m) {
    char path[MAX_PATH];
    if (!m || !GetModuleFileNameA(m, path, sizeof path)) return NULL;
    const char *file = fileNameOf(path);
    for (int i = 0; i < pluginRecordCount; i++)
        if (!_stricmp(pluginRecords[i]->file, file)) {
            if (!pluginRecords[i]->module) pluginRecords[i]->module = m;
            return pluginRecords[i];
        }
    return NULL;
}

// Holds the DLL back: logs plugin_refused and keeps the reason for the page.
static PRINTF_FORMAT(3, 4) void refusePlugin(PluginRecord *r, const char *reason, const char *fmt, ...) {
    if (r->reason) return;
    r->reason = reason;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r->detail, sizeof r->detail, fmt, ap);
    va_end(ap);
    writeLog("plugin_refused", "dll=%s reason=%s %s", r->file, reason, r->detail);
}

// The refusal reason for a plugin built for API version `version`, which isn't CoE5SE's.
static const char *apiVersionReason(int version) {
    return version > PLUGIN_API_VERSION ? "newer_api_version" : "older_api_version";
}

// ---- 1. Game versions --------------------------------------------------------------------------

// The game version CoE5SE counts the running game as.
static const char *runningGameVersion(void) {
    if (gameBuild) return gameBuild->gameVersion;
    return pluginsDeclined ? NULL : KNOWN_BUILDS[KNOWN_BUILD_COUNT - 1].gameVersion;
}

// Reads the coeversion lines of `mod`'s .c5m, and refuses the DLL when none matches.
static void checkPluginGameVersion(PluginRecord *r, const char *mod) {
    r->versionChecked = 1;
    char dir[MAX_PATH], name[MAX_PATH], file[MAX_PATH], line[1024];
    if (!buildPath(dir, sizeof dir, __func__, "%s", r->path)) return;
    char *slash = strrchr(dir, PATH_SEP[0]);
    if (slash) *slash = '\0';
    if (mod && mod[0]) snprintf(name, sizeof name, "%s", mod);
    else if (!c5mNameInFolder(dir, name, sizeof name)) name[0] = '\0';
    int matched = 0, lines = 0;
    size_t used = 0;
    r->declared[0] = '\0';
    FILE *f = name[0] && buildPath(file, sizeof file, __func__, "%s" PATH_SEP "%s.c5m", dir, name) ? fopen(file, "r") : NULL;
    while (f && fgets(line, sizeof line, f)) {
        line[strcspn(line, "#\r\n")] = '\0';
        char *word = line + strspn(line, " \t");
        size_t len = strlen(GAME_VERSION_WORD);
        if (strncmp(word, GAME_VERSION_WORD, len) || (word[len] && word[len] != ' ' && word[len] != '\t')) continue;
        for (char *value = strtok(word + len, " \t\""); value; value = strtok(NULL, " \t\"")) {
            lines++;
            const char *running = runningGameVersion();
            if (!_stricmp(value, GAME_VERSION_ANY) || (running && !strcmp(value, running))) matched = 1;
            if (used + strlen(value) + 2 < sizeof r->declared)
                used += (size_t)snprintf(r->declared + used, sizeof r->declared - used, "%s%s", used ? "," : "", value);
        }
    }
    if (f) fclose(f);
    const char *running = runningGameVersion();
    writeLog("plugin_checked", "dll=%s mod=%s declared=%s game_version=%s ok=%d", r->file, name[0] ? name : "-",
             lines ? r->declared : "none", running ? running : "declined", matched);
    if (!lines)
        refusePlugin(r, "no_game_version", "mod=%s word=%s game_version=%s", name[0] ? name : "-", GAME_VERSION_WORD,
                     running ? running : "declined");
    else if (!matched)
        refusePlugin(r, "built_for_other_game_version", "mod=%s declared=%s game_version=%s", name[0] ? name : "-",
                     r->declared, running ? running : "declined");
}

// The loader calls this for each plugin before it hashes or loads it.
__declspec(dllexport) const char *coe5seCheckPlugin(const char *path, const char *mod) {
    PluginRecord *r = path ? pluginRecordFor(path) : NULL;
    if (!r) return NULL;
    if (mod) snprintf(r->mod, sizeof r->mod, "%s", mod);
    if (!r->versionChecked) checkPluginGameVersion(r, mod);
    return r->reason;
}

// ---- 2. Address ids ----------------------------------------------------------------------------

// The DLL's AddressSlots, or NULL when it has none.
static AddressSlot *moduleAddressSlots(HMODULE m, size_t *count) {
    *count = 0;
#ifdef __APPLE__
    unsigned long size = 0;
    uint8_t *data = getsectiondata((const struct mach_header_64 *)m, "__DATA", "__c5addr", &size);
    *count = data ? size / sizeof(AddressSlot) : 0;
    return (AddressSlot *)data;
#else
    const unsigned char *base = (const unsigned char *)m;
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
    const IMAGE_SECTION_HEADER *s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
        if (!memcmp(s->Name, ".c5addr", 8)) {
            *count = s->Misc.VirtualSize / sizeof(AddressSlot);
            return (AddressSlot *)(base + s->VirtualAddress);
        }
    return NULL;
#endif
}

static int compareAddressId(const void *key, const void *entry) {
    uint32_t id = *(const uint32_t *)key, other = ((const AddressEntry *)entry)->id;
    return id < other ? -1 : id > other;
}

static const AddressEntry *addressEntryById(uint32_t id) {
    return bsearch(&id, ADDRESS_TABLE, ADDRESS_TABLE_COUNT, sizeof ADDRESS_TABLE[0], compareAddressId);
}

// An entry's id: the 32-bit FNV-1a hash of its name.
static uint32_t addressIdOf(const char *name) {
    uint32_t h = 2166136261u;
    for (const char *c = name; *c; c++) h = (h ^ (unsigned char)*c) * 16777619u;
    return h;
}

// Checks every slot of the DLL, then fills them all, or refuses the DLL and fills none.
static void resolvePluginAddresses(PluginRecord *r) {
    if (r->addressesChecked || !r->module) return;
    r->addressesChecked = 1;
    size_t count;
    AddressSlot *slots = moduleAddressSlots(r->module, &count);
    int used = 0;
    for (size_t i = 0; i < count && !r->reason; i++) {
        const AddressSlot *s = &slots[i];
        if (s->magic != ADDRESS_SLOT_MAGIC) continue;
        used++;
        const char *name = s->name ? s->name : "?";
        if (s->format > ADDRESS_ID_FORMAT) {
            refusePlugin(r, "newer_address_format", "format=%d supported=%d name=%s", s->format, ADDRESS_ID_FORMAT, name);
            break;
        }
        const AddressEntry *e = addressEntryById(s->id);
        if (!e || !s->name || strcmp(e->name, s->name))
            refusePlugin(r, "unknown_address", "name=%s id=0x%08x", name, (unsigned)s->id);
        else if (e->kind != s->kind)
            refusePlugin(r, "address_kind_mismatch", "name=%s kind=%d table_kind=%d", name, s->kind, e->kind);
        else if (e->missing)
            refusePlugin(r, "address_missing", "name=%s missing=%s", name, e->missing);
    }
    if (r->reason) return;
    int distinct = 0;
    for (size_t i = 0; i < count; i++) {
        AddressSlot *s = &slots[i];
        if (s->magic != ADDRESS_SLOT_MAGIC) continue;
        const AddressEntry *e = addressEntryById(s->id);
        s->value = e->kind == ADDRESS_KIND_OFFSET ? (uintptr_t)(gameBase + e->value) : (uintptr_t)e->value;
        int first = 1;
        for (size_t j = 0; j < i && first; j++) first = slots[j].magic != ADDRESS_SLOT_MAGIC || slots[j].id != s->id;
        distinct += first;
    }
    if (used) writeLog("plugin_addresses_resolved", "dll=%s names=%d uses=%d format=%d", r->file, distinct, used, ADDRESS_ID_FORMAT);
}

// ---- 3. The entry ------------------------------------------------------------------------------

// coe5sePluginLoading(NULL): the loader has loaded the DLL at `path`, or failed to (it logs that).
static void pluginLoadingDone(const char *path) {
    PluginRecord *r = pluginRecordFor(path);
    if (!r || !(r->module = GetModuleHandleA(r->file))) return;
    resolvePluginAddresses(r);
    typedef int (*PluginLoadFn)(const PluginApi *);
    PluginLoadFn entry = (PluginLoadFn)(void *)GetProcAddress(r->module, "coe5sePluginLoad");
    const int *version = (const int *)(void *)GetProcAddress(r->module, "coe5sePluginApiVersion");
    if (r->reason) {
        writeLog("plugin_not_started", "dll=%s reason=%s", r->file, r->reason);
        return;
    }
    if (!entry || !version || *version != PLUGIN_API_VERSION) {
        if (!entry) refusePlugin(r, "no_entry", "export=coe5sePluginLoad");
        else if (!version) refusePlugin(r, "no_api_version", "entry=coe5sePluginLoad");
        else refusePlugin(r, apiVersionReason(*version), "version=%d supported=%d", *version, PLUGIN_API_VERSION);
        writeLog("plugin_not_started", "dll=%s reason=%s", r->file, r->reason);
        return;
    }
    writeLog("plugin_starting", "dll=%s entry=coe5sePluginLoad api=%d", r->file, *version);
    r->entryCalled = 1;
    int result = entry(&API);
    writeLog("plugin_started", "dll=%s entry=coe5sePluginLoad api=%d result=%d", r->file, *version, result);
}

// For jazzyGetApi: why CoE5SE holds back the DLL that holds `caller`, or NULL.
static const char *pluginRefusalFor(void *caller) {
    PluginRecord *r = pluginRecordOfModule(callingPlugin(caller));
    if (!r) return NULL;
    return r->reason ? r->reason : r->entryCalled ? NULL : "not_started";
}

// ---- Stack frame locals (frameVariable) --------------------------------------------------------
// The sites each game function calls from its own frame.
static const CallSite NEW_WORLD_FRAME_SITES[] = {SITE_NEW_WORLD_MAP_SCREEN, SITE_NEW_WORLD_INIT_PARTICIPANTS,
                                                 SITE_NEW_WORLD_CREATING_WORLD, SITE_PARTICIPANTS_WAIT,
                                                 SITE_PARTICIPANTS_BUTTON_ROW};
static const struct { const char *prefix; const CallSite *sites; size_t count; } FRAME_SITES[] = {
    {"FRAME_NEW_WORLD_", NEW_WORLD_FRAME_SITES, sizeof NEW_WORLD_FRAME_SITES / sizeof NEW_WORLD_FRAME_SITES[0]},
};

// PluginApi.frameVariable.
static void *apiFrameVariable(CallSite site, const char *name) {
    if ((unsigned)site >= SITE_COUNT) return REFUSE_API(NULL, "frameVariable", "unknown_site", "site=%d", (int)site);
    if (!name) return REFUSE_API(NULL, "frameVariable", "no_name", "site=%s", SITES[site].name);
    const AddressEntry *e = addressEntryById(addressIdOf(name));
    if (!e || strcmp(e->name, name) || e->kind != ADDRESS_KIND_FRAME)
        return REFUSE_API(NULL, "frameVariable", "not_a_frame_variable", "site=%s name=%s", SITES[site].name, name);
    if (e->missing) return REFUSE_API(NULL, "frameVariable", "no_offset_on_this_build", "site=%s name=%s missing=%s", SITES[site].name, name, e->missing);
    int reaches = 0;
    for (size_t i = 0; i < sizeof FRAME_SITES / sizeof FRAME_SITES[0]; i++)
        if (!strncmp(name, FRAME_SITES[i].prefix, strlen(FRAME_SITES[i].prefix)))
            for (size_t k = 0; k < FRAME_SITES[i].count; k++) reaches |= FRAME_SITES[i].sites[k] == site;
    if (!reaches) return REFUSE_API(NULL, "frameVariable", "site_not_in_that_frame", "site=%s name=%s", SITES[site].name, name);
    if (!siteInstalled[site]) return REFUSE_API(NULL, "frameVariable", "site_not_wrapped", "site=%s name=%s", SITES[site].name, name);
#ifdef __aarch64__
    uintptr_t sp = siteCallerSp(site);
#else
    uintptr_t sp = *(uintptr_t *)(siteStub(site) + STUB_RSP) + 8;
#endif
    return (void *)(sp + (uintptr_t)e->value);
}

// ---- The CoE5SE page ---------------------------------------------------------------------------

// The row suffix and the tooltip text for a DLL CoE5SE held back, or 0 for one it didn't.
static int pluginRefusalText(const char *file, char *row, size_t rowSize, char *tip, size_t tipSize) {
    PluginRecord *r = NULL;
    for (int i = 0; i < pluginRecordCount && !r; i++)
        if (!_stricmp(pluginRecords[i]->file, file)) r = pluginRecords[i];
    if (!r || !r->reason) return 0;
    const char *running = runningGameVersion();
    if (!strcmp(r->reason, "built_for_other_game_version")) {
        snprintf(row, rowSize, " (not loaded: made for another game version)");
        snprintf(tip, tipSize, "Not loaded: its .c5m says its DLLs were built for CoE5 %s (coeversion), and this is "
                 "CoE5 %s. A version of the mod made for this one fixes it. ", r->declared, running ? running : "?");
    } else if (!strcmp(r->reason, "no_game_version")) {
        snprintf(row, rowSize, " (not loaded: no coeversion line)");
        snprintf(tip, tipSize, "Not loaded: its .c5m has no coeversion line, which names the game versions its DLLs were "
                 "built for. CoE5SE loads a mod's DLLs only when the line names this version (CoE5 %s) or any. ",
                 running ? running : "?");
    } else if (!strcmp(r->reason, "no_entry") || !strcmp(r->reason, "no_api_version")) {
        snprintf(row, rowSize, " (not started: no plugin entry)");
        snprintf(tip, tipSize, "Not started: the DLL doesn't export coe5sePluginLoad and its API version, which CoE5SE "
                 "starts a plugin through (COE5SE_PLUGIN_LOAD in jazzy_api.h). ");
    } else if (!strcmp(r->reason, "older_api_version")) {
        snprintf(row, rowSize, " (not started: made for an older CoE5SE)");
        snprintf(tip, tipSize, "Not started: it was built for an older CoE5SE than this one (%.150s). A version of the "
                 "mod built for this CoE5SE fixes it. ", r->detail);
    } else if (!strcmp(r->reason, "address_missing")) {
        snprintf(row, rowSize, " (not started: needs what this game version lacks)");
        snprintf(tip, tipSize, "Not started: it uses a game address CoE5SE has none for on this game version (%.150s). ", r->detail);
    } else {
        snprintf(row, rowSize, " (not started: needs a newer CoE5SE)");
        snprintf(tip, tipSize, "Not started: it was built for a newer CoE5SE than this one (%s: %.150s). Update CoE5SE. ",
                 r->reason, r->detail);
    }
    return 1;
}

// Logs the address table this CoE5SE resolves plugins' ids with.
static void pluginStartInit(void) {
    writeLog("plugin_checks_installed", "address_table=%d format=%d", ADDRESS_TABLE_COUNT, ADDRESS_ID_FORMAT);
}
