// Load order inside the game: each enabled mod's modprio, modversion, and requires.

#include "mod_order.h"

static ModInfo *orderedMods;
static int orderedModCount, orderedModCapacity;
static int *loadOrder;
static ModIssue *orderIssues;
static int orderIssueCount;

static void clearLoadOrder(void) {
    for (int i = 0; i < orderedModCount; i++) freeModInfo(&orderedMods[i]);
    orderedModCount = 0;
    free(loadOrder);
    loadOrder = NULL;
    free(orderIssues);
    orderIssues = NULL;
    orderIssueCount = 0;
}

// Reads every enabled mod's .c5m for modprio, modversion, and requires, and orders them.
static void readLoadOrder(int logIt) {
    clearLoadOrder();
    char path[MAX_PATH];
    for (int m = 0; m <= G32(RVA_MOD_LAST_INDEX); m++) {
        if (!modEnabled(m)) continue;
        int seen = 0;
        for (int i = 0; i < orderedModCount && !seen; i++) seen = !strcmp(orderedMods[i].name, modName(m));
        if (seen) continue;
        if (!growList((void **)&orderedMods, &orderedModCapacity, orderedModCount + 1, sizeof *orderedMods)) {
            writeLog("mod_order_cut", "reason=out_of_memory mods=%d", orderedModCount);
            break;
        }
        ModInfo *info = &orderedMods[orderedModCount++];
        initModInfo(info, modName(m), m);
        if (buildPath(path, sizeof path, __func__, "%s" PATH_SEP "%s.c5m", modFolder(m), modName(m))) readModOrderFile(info, path);
    }
    loadOrder = orderedModCount ? malloc((size_t)orderedModCount * sizeof *loadOrder) : NULL;
    int ordered = loadOrder && orderMods(orderedMods, orderedModCount, loadOrder, &orderIssues, &orderIssueCount);
    int enabledCount = orderedModCount;
    if (!loadOrder && orderedModCount) clearLoadOrder();
    if (!logIt) return;
    writeLog("mod_order_read", "enabled_mods=%d ordered=%d warnings=%d", enabledCount, ordered || !enabledCount, orderIssueCount);
    for (int i = 0; i < orderedModCount; i++)
        for (int k = 0; k < orderedMods[i].problemCount; k++) {
            const ModLineProblem *p = &orderedMods[i].problems[k];
            refuseC5mLine(p->word, orderedMods[i].name, p->reason, "line=%d text=%s", p->line, p->text);
        }
    static const char *const EVENTS[] = {"mod_requirement_missing", "mod_requirement_old", "mod_requirement_cycle"};
    for (int k = 0; k < orderIssueCount; k++) {
        const ModInfo *m = &orderedMods[orderIssues[k].mod];
        const ModRequirement *q = &m->requirements[orderIssues[k].requirement];
        const char *have = q->target >= 0 && orderedMods[q->target].version[0] ? orderedMods[q->target].version : "-";
        writeLog(EVENTS[orderIssues[k].kind], "mod=%s required=%s have=%s want=%s line=%d", m->name, q->mod, have,
                 q->want[0] ? q->want : "-", q->line);
    }
}

// The mod's modversion, or "-" when it has none or isn't enabled.
static const char *modVersionText(const char *mod) {
    for (int i = 0; i < orderedModCount; i++)
        if (!strcmp(orderedMods[i].name, mod)) return orderedMods[i].version[0] ? orderedMods[i].version : "-";
    return "-";
}

// The version of the mod whose folder holds the plugin at `pluginPath`, or "" for none.
static const char *pluginModVersion(const char *pluginPath) {
    char dir[MAX_PATH], mod[MOD_NAME_MAX];
    if (!buildPath(dir, sizeof dir, __func__, "%s", pluginPath)) return "";
    char *slash = strrchr(dir, PATH_SEP[0]);
    if (!slash) return "";
    *slash = '\0';
    if (!c5mNameInFolder(dir, mod, sizeof mod)) return "";
    const char *version = modVersionText(mod);
    return strcmp(version, "-") ? version : "";
}

// The CoE5SE page's "Mod requirements" box: one line per warning, drawn only when there is one.
static void drawModRequirements(void) {
    if (!orderIssueCount) return;
    sectionBegin();
    sectionHeader("Mod requirements");
    char line[1024], tip[2048];
    for (int k = 0; k < orderIssueCount; k++) {
        const ModInfo *m = &orderedMods[orderIssues[k].mod];
        const ModRequirement *q = &m->requirements[orderIssues[k].requirement];
        const char *have = q->target >= 0 ? orderedMods[q->target].version : "";
        if (orderIssues[k].kind == MOD_REQUIREMENT_MISSING) {
            snprintf(line, sizeof line, "%s needs %s%s%s, which is not enabled", m->name, q->mod, q->want[0] ? " " : "",
                     q->want[0] ? q->want : "");
            snprintf(tip, sizeof tip, "%s's .c5m, line %d, requires %s. %s must be installed and enabled on the Mods screen. "
                     "%s still loads, without what it uses from %s.", m->name, q->line, q->mod, q->mod, m->name, q->mod);
        } else if (orderIssues[k].kind == MOD_REQUIREMENT_OLD) {
            snprintf(line, sizeof line, "%s needs %s %s or later; %s is enabled", m->name, q->mod, q->want, have);
            snprintf(tip, sizeof tip, "%s's .c5m, line %d, requires %s %s or later, and the enabled copy is %s. Update %s. "
                     "%s still loads.", m->name, q->line, q->mod, q->want, have, q->mod, m->name);
        } else {
            snprintf(line, sizeof line, "%s needs %s, in a requirement cycle", m->name, q->mod);
            snprintf(tip, sizeof tip, "%s's .c5m, line %d, requires %s, and %s requires %s, directly or through other "
                     "mods. No order puts each after what it requires, so these mods load in name order.",
                     m->name, q->line, q->mod, q->mod, m->name);
        }
        textButton(0, line);
        tooltip(tip);
    }
    sectionEnd();
}

// The `requires <ModName> [minimum version]` .c5m word.
static void requiresLine(const char *args, int mod, int level) {
    (void)args; (void)mod; (void)level;
}

// Adds the requires word.
static void loadOrderInit(void) {
    writeLog("load_order_installed", "requires=%d", api->addModCommand("requires", requiresLine));
}
