// The order CoE5SE loads mods' code in, and the mods' versions and requirements.
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOD_NAME_MAX      256
#define MOD_VERSION_MAX   32
#define MOD_PRIO_DEFAULT  5
#define MOD_LINE_MAX      50000

// One `requires` line.
typedef struct {
    char mod[MOD_NAME_MAX];
    char want[MOD_VERSION_MAX];
    int line;
    int target;
} ModRequirement;

// A modversion or requires line CoE5SE couldn't read.
typedef struct {
    char word[16];
    const char *reason;
    int line;
    char text[160];
} ModLineProblem;

typedef struct {
    char name[MOD_NAME_MAX];
    int tag;
    int prio;
    char version[MOD_VERSION_MAX];
    ModRequirement *requirements;
    int requirementCount, requirementCapacity;
    ModLineProblem *problems;
    int problemCount, problemCapacity;
    int cycle;
} ModInfo;

enum { MOD_REQUIREMENT_MISSING, MOD_REQUIREMENT_OLD, MOD_REQUIREMENT_CYCLE };

// One warning from orderMods.
typedef struct {
    int kind;
    int mod;
    int requirement;
} ModIssue;

// The game's mod name comparison: < 0 if a sorts first, 0 if equal, > 0 if b sorts first.
static int compareModNames(const char *a, const char *b) {
    for (;; a++, b++) {
        if (*a != *b) return (signed char)*a < (signed char)*b ? -1 : 1;
        if (!*a) return 0;
    }
}

// 1 if `text` is a version: digits, then optionally a dot and more digits.
static int isModVersion(const char *text) {
    const char *c = text;
    if (*c < '0' || *c > '9') return 0;
    while (*c >= '0' && *c <= '9') c++;
    if (*c == '.') {
        c++;
        if (*c < '0' || *c > '9') return 0;
        while (*c >= '0' && *c <= '9') c++;
    }
    return *c == '\0';
}

// Compares two versions (isModVersion) as numbers, exactly: < 0, 0, or > 0.
static int compareModVersions(const char *a, const char *b) {
    while (*a == '0' && a[1] >= '0' && a[1] <= '9') a++;
    while (*b == '0' && b[1] >= '0' && b[1] <= '9') b++;
    size_t ia = strcspn(a, "."), ib = strcspn(b, ".");
    if (ia != ib) return ia < ib ? -1 : 1;
    int c = strncmp(a, b, ia);
    if (c) return c < 0 ? -1 : 1;
    a += ia; b += ib;
    if (*a == '.') a++;
    if (*b == '.') b++;
    for (; *a || *b; a += *a != '\0', b += *b != '\0') {
        char da = *a ? *a : '0', db = *b ? *b : '0';
        if (da != db) return da < db ? -1 : 1;
    }
    return 0;
}

// The game's number reader for modprio.
static long long gameFirstNumber(const char *line) {
    int inQuotes = 0;
    const char *start = NULL;
    for (const char *c = line;; c++) {
        if (*c == '"') inQuotes = !inQuotes;
        if (!inQuotes) {
            if ((*c >= '0' && *c <= '9') || *c == '-') {
                if (!start) start = c;
            } else if (start) {
                return atoll(start);
            }
        }
        if (!*c) return -1;
    }
}

// Grows a list to hold at least `count` items of `size` bytes.
static int growList(void **list, int *capacity, int count, size_t size) {
    if (count <= *capacity) return 1;
    int newCapacity = *capacity ? *capacity * 2 : 16;
    while (newCapacity < count) newCapacity *= 2;
    void *grown = realloc(*list, (size_t)newCapacity * size);
    if (!grown) return 0;
    *list = grown;
    *capacity = newCapacity;
    return 1;
}

static void addModLineProblem(ModInfo *m, const char *word, const char *reason, int line, const char *text) {
    if (!growList((void **)&m->problems, &m->problemCapacity, m->problemCount + 1, sizeof *m->problems)) return;
    ModLineProblem *p = &m->problems[m->problemCount++];
    snprintf(p->word, sizeof p->word, "%s", word);
    p->reason = reason;
    p->line = line;
    snprintf(p->text, sizeof p->text, "%.159s", text);
}

// Copies the next argument of `*args` into out: a "quoted" one without its quotes, or one word.
static int nextModArg(const char **args, char *out, size_t size) {
    const char *c = *args, *start = c, *end;
    if (*c == '"') {
        start = ++c;
        end = strchr(c, '"');
        if (!end) end = c + strlen(c);
        c = *end ? end + 1 : end;
    } else {
        end = c + strcspn(c, " \t");
        c = end;
    }
    while (*c == ' ' || *c == '\t') c++;
    *args = c;
    size_t len = (size_t)(end - start);
    if (len >= size) return -1;
    memcpy(out, start, len);
    out[len] = '\0';
    return (int)len;
}

// Reads one .c5m line for modprio, modversion, and requires.
static void readModOrderLine(ModInfo *m, const char *text, int line) {
    while (*text == ' ' || *text == '\t') text++;
    size_t wordLen = strcspn(text, " \t");
    const char *args = text + wordLen;
    while (*args == ' ' || *args == '\t') args++;
    size_t argsLen = strlen(args);
    while (argsLen && (args[argsLen - 1] == ' ' || args[argsLen - 1] == '\t')) argsLen--;

    if (wordLen == 7 && !strncmp(text, "modprio", 7)) {
        long long prio = gameFirstNumber(text);
        m->prio = prio < 1 ? 1 : prio > 9 ? 9 : (int)prio;
    } else if (wordLen == 10 && !strncmp(text, "modversion", 10)) {
        if (!argsLen) addModLineProblem(m, "modversion", "no_version", line, text);
        else if (argsLen >= MOD_VERSION_MAX) addModLineProblem(m, "modversion", "version_too_long", line, text);
        else {
            char version[MOD_VERSION_MAX];
            memcpy(version, args, argsLen);
            version[argsLen] = '\0';
            if (!isModVersion(version)) addModLineProblem(m, "modversion", "not_a_number", line, text);
            else memcpy(m->version, version, sizeof version);
        }
    } else if (wordLen == 8 && !strncmp(text, "requires", 8)) {
        ModRequirement r;
        memset(&r, 0, sizeof r);
        r.line = line;
        r.target = -1;
        int len = nextModArg(&args, r.mod, sizeof r.mod);
        if (len < 0) { addModLineProblem(m, "requires", "name_too_long", line, text); return; }
        if (len == 0) { addModLineProblem(m, "requires", "no_mod_name", line, text); return; }
        if (*args) {
            len = nextModArg(&args, r.want, sizeof r.want);
            if (len < 0 || !isModVersion(r.want)) { addModLineProblem(m, "requires", "not_a_number", line, text); return; }
        }
        if (*args) { addModLineProblem(m, "requires", "trailing_text", line, text); return; }
        if (!growList((void **)&m->requirements, &m->requirementCapacity, m->requirementCount + 1, sizeof r)) {
            addModLineProblem(m, "requires", "out_of_memory", line, text);
            return;
        }
        m->requirements[m->requirementCount++] = r;
    }
}

// Starts a mod's info: its name, the default modprio, no version, no requirements.
static void initModInfo(ModInfo *m, const char *name, int tag) {
    memset(m, 0, sizeof *m);
    snprintf(m->name, sizeof m->name, "%s", name);
    m->tag = tag;
    m->prio = MOD_PRIO_DEFAULT;
    m->cycle = -1;
}

static void freeModInfo(ModInfo *m) {
    free(m->requirements);
    free(m->problems);
    m->requirements = NULL;
    m->problems = NULL;
    m->requirementCount = m->requirementCapacity = m->problemCount = m->problemCapacity = 0;
}

// Reads a whole .c5m text for modprio, modversion, and requires.
static void readModOrderText(ModInfo *m, const char *text) {
    static char buf[MOD_LINE_MAX + 1];
    int line = 0, inEvent = 0;
    while (*text) {
        size_t len = strcspn(text, "\n");
        line++;
        size_t keep = len < MOD_LINE_MAX ? len : MOD_LINE_MAX;
        memcpy(buf, text, keep);
        buf[keep] = '\0';
        buf[strcspn(buf, "#\r")] = '\0';
        const char *w = buf;
        while (*w == ' ' || *w == '\t') w++;
        size_t wordLen = strcspn(w, " \t");
        if ((wordLen == 11 && !strncmp(w, "playerevent", 11)) || (wordLen == 11 && !strncmp(w, "squareevent", 11))) inEvent = 1;
        else if (wordLen == 8 && !strncmp(w, "endevent", 8)) inEvent = 0;
        else if (!inEvent) readModOrderLine(m, buf, line);
        text += len;
        if (*text == '\n') text++;
    }
}

// Reads the .c5m file at `path` into m (readModOrderText).
static int readModOrderFile(ModInfo *m, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *text = size >= 0 ? malloc((size_t)size + 1) : NULL;
    if (!text) { fclose(f); return 0; }
    size_t got = fread(text, 1, (size_t)size, f);
    fclose(f);
    text[got] = '\0';
    readModOrderText(m, text);
    free(text);
    return 1;
}

// 1 if mod a comes before mod b by rules 1 and 2 (modprio, then name).
static int modBefore(const ModInfo *a, const ModInfo *b) {
    if (a->prio != b->prio) return a->prio < b->prio;
    return compareModNames(a->name, b->name) < 0;
}

// The working state of orderMods.
typedef struct {
    ModInfo *mods;
    int count;
    int *base;
    int *rank;
    int *component;
    int *index, *low, *stack, stackSize, nextIndex, componentCount;
    char *onStack, *placed;
    int *order, placedCount;
    int *members, *groupStart;
} ModOrderState;

// Tarjan's algorithm: finds the groups of mods that require each other.
static void findModGroups(ModOrderState *s, int v) {
    s->index[v] = s->low[v] = s->nextIndex++;
    s->stack[s->stackSize++] = v;
    s->onStack[v] = 1;
    const ModInfo *m = &s->mods[v];
    for (int r = 0; r < m->requirementCount; r++) {
        int w = m->requirements[r].target;
        if (w < 0) continue;
        if (s->index[w] < 0) {
            findModGroups(s, w);
            if (s->low[w] < s->low[v]) s->low[v] = s->low[w];
        } else if (s->onStack[w] && s->index[w] < s->low[v]) {
            s->low[v] = s->index[w];
        }
    }
    if (s->low[v] != s->index[v]) return;
    int w;
    do {
        w = s->stack[--s->stackSize];
        s->onStack[w] = 0;
        s->component[w] = s->componentCount;
    } while (w != v);
    s->componentCount++;
}

// Places the group of mod v.
static int placeModGroup(ModOrderState *s, int v) {
    int c = s->component[v];
    if (s->placed[c]) return 1;
    s->placed[c] = 1;
    int n = 0;
    for (int k = s->groupStart[c]; k < s->groupStart[c + 1]; k++) n += s->mods[s->members[k]].requirementCount;
    int *targets = n ? malloc((size_t)n * sizeof(int)) : NULL;
    if (n && !targets) return 0;
    n = 0;
    for (int k = s->groupStart[c]; k < s->groupStart[c + 1]; k++) {
        const ModInfo *m = &s->mods[s->members[k]];
        for (int r = 0; r < m->requirementCount; r++) {
            int w = m->requirements[r].target;
            if (w < 0 || s->component[w] == c) continue;
            int at = n++;
            while (at > 0 && s->rank[w] < s->rank[targets[at - 1]]) {
                targets[at] = targets[at - 1];
                at--;
            }
            targets[at] = w;
        }
    }
    int ok = 1;
    for (int k = 0; k < n && ok; k++) ok = placeModGroup(s, targets[k]);
    free(targets);
    for (int k = s->groupStart[c]; k < s->groupStart[c + 1]; k++) s->order[s->placedCount++] = s->members[k];
    return ok;
}

// Sorts `order` (mod indices) by rules 1 and 2.
static void sortModsByPrioAndName(const ModInfo *mods, int count, int *order) {
    for (int i = 0; i < count; i++) order[i] = i;
    for (int i = 1; i < count; i++)
        for (int k = i; k > 0 && modBefore(&mods[order[k]], &mods[order[k - 1]]); k--) {
            int t = order[k]; order[k] = order[k - 1]; order[k - 1] = t;
        }
}

// Points each requirement at the mod it names (its index), or -1 when that mod isn't among them.
static void linkModRequirements(ModInfo *mods, int count) {
    for (int i = 0; i < count; i++)
        for (int r = 0; r < mods[i].requirementCount; r++) {
            ModRequirement *q = &mods[i].requirements[r];
            q->target = -1;
            for (int j = 0; j < count && q->target < 0; j++)
                if (!strcmp(mods[j].name, q->mod)) q->target = j;
        }
}

// Allocates orderMods' state, with `order` (in rule 1 and 2 order) as its base.
static int startModOrderState(ModOrderState *s, ModInfo *mods, int count, int *order) {
    memset(s, 0, sizeof *s);
    s->mods = mods;
    s->count = count;
    s->order = order;
    s->base = malloc((size_t)count * sizeof(int));
    if (s->base) memcpy(s->base, order, (size_t)count * sizeof(int));
    s->rank = malloc((size_t)count * sizeof(int));
    s->component = calloc((size_t)count, sizeof(int));
    s->index = malloc((size_t)count * sizeof(int));
    s->low = malloc((size_t)count * sizeof(int));
    s->stack = malloc((size_t)count * sizeof(int));
    s->members = malloc((size_t)count * sizeof(int));
    s->onStack = calloc((size_t)count, 1);
    s->placed = calloc((size_t)count, 1);
    if (!(s->base && s->rank && s->component && s->index && s->low && s->stack && s->members && s->onStack && s->placed)) return 0;
    for (int k = 0; k < count; k++) {
        s->rank[s->base[k]] = k;
        s->index[k] = -1;
    }
    return 1;
}

static void freeModOrderState(ModOrderState *s) {
    free(s->base); free(s->rank); free(s->component); free(s->index); free(s->low); free(s->stack);
    free(s->members); free(s->groupStart); free(s->onStack); free(s->placed);
}

// Lists each group's members, in name order, once findModGroups has found the groups.
static int listGroupMembers(ModOrderState *s) {
    s->groupStart = calloc((size_t)s->componentCount + 1, sizeof(int));
    if (!s->groupStart) return 0;
    for (int i = 0; i < s->count; i++) s->groupStart[s->component[i] + 1]++;
    for (int c = 0; c < s->componentCount; c++) s->groupStart[c + 1] += s->groupStart[c];
    int *fill = calloc((size_t)s->componentCount + 1, sizeof(int));
    if (!fill) return 0;
    for (int i = 0; i < s->count; i++) {
        int c = s->component[i], at = s->groupStart[c] + fill[c]++;
        while (at > s->groupStart[c] && compareModNames(s->mods[i].name, s->mods[s->members[at - 1]].name) < 0) {
            s->members[at] = s->members[at - 1];
            at--;
        }
        s->members[at] = i;
    }
    free(fill);
    return 1;
}

// Sets the cycle of each mod in a group of more than one mod, or that requires itself.
static void markModCycles(ModOrderState *s) {
    for (int i = 0; i < s->count; i++) {
        const ModInfo *m = &s->mods[i];
        int selfRequired = 0, c = s->component[i];
        for (int r = 0; r < m->requirementCount; r++) selfRequired |= m->requirements[r].target == i;
        if (s->groupStart[c + 1] - s->groupStart[c] > 1 || selfRequired) s->mods[i].cycle = c;
    }
}

// The warnings, in load order.
static int collectModIssues(const ModInfo *mods, int count, const int *order, ModIssue **issues, int *issueCount) {
    int issueCapacity = 0;
    for (int k = 0; k < count; k++) {
        const ModInfo *m = &mods[order[k]];
        for (int r = 0; r < m->requirementCount; r++) {
            const ModRequirement *q = &m->requirements[r];
            int kind = -1;
            if (q->target < 0) kind = MOD_REQUIREMENT_MISSING;
            else if (m->cycle >= 0 && mods[q->target].cycle == m->cycle) kind = MOD_REQUIREMENT_CYCLE;
            else if (q->want[0] && mods[q->target].version[0] && compareModVersions(mods[q->target].version, q->want) < 0)
                kind = MOD_REQUIREMENT_OLD;
            if (kind < 0) continue;
            if (!growList((void **)issues, &issueCapacity, *issueCount + 1, sizeof **issues)) return 0;
            (*issues)[(*issueCount)++] = (ModIssue){kind, order[k], r};
        }
    }
    return 1;
}

// Orders `count` mods into `order` and returns the warnings.
static int orderMods(ModInfo *mods, int count, int *order, ModIssue **issues, int *issueCount) {
    *issues = NULL;
    *issueCount = 0;
    for (int i = 0; i < count; i++) mods[i].cycle = -1;
    sortModsByPrioAndName(mods, count, order);
    if (count == 0) return 1;
    linkModRequirements(mods, count);
    ModOrderState s;
    int ok = startModOrderState(&s, mods, count, order);
    for (int k = 0; ok && k < count; k++)
        if (s.index[s.base[k]] < 0) findModGroups(&s, s.base[k]);
    ok = ok && listGroupMembers(&s);
    if (ok) markModCycles(&s);
    for (int k = 0; ok && k < count; k++) ok = placeModGroup(&s, s.base[k]);
    ok = ok && collectModIssues(mods, count, order, issues, issueCount);
    if (!ok) {
        if (s.base) memcpy(order, s.base, (size_t)count * sizeof(int));
        free(*issues);
        *issues = NULL;
        *issueCount = 0;
    }
    freeModOrderState(&s);
    return ok;
}
