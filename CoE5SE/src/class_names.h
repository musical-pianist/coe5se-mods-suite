// selectclass "<name>": a class picked by its name, the game's or a mod's.
#pragma once

#define SELECTCLASS_WORD "selectclass"

static char classNameLine[sizeof SELECTCLASS_WORD + 16];

// The class named `name`, by the game's rule; -1 for none.
static int classByName(const char *name) {
    int last = G32(RVA_CLASS_COUNT), cap = tableRecords(TABLE_CLASS) - 1;
    if (last > cap) last = cap;
    for (int c = 0; c <= last; c++)
        if (!strncmp((const char *)(tableAddress(TABLE_CLASS) + (size_t)c * CLASS_SIZE), name, CLASS_NAME_SIZE)) return c;
    return -1;
}

// Turns `selectclass "<name>"` into `selectclass <number>`; returns the line to go on with.
static const char *classNameLineHook(const char *line, size_t len, int mod, int level) {
    if (level != 2 || !spellWordIs(line, len, SELECTCLASS_WORD)) return line;
    const char *args = line + len;
    skipSpaces(&args);
    if (args[0] != '"') return line;
    char name[CLASS_NAME_SIZE + 1];
    const char *p = args;
    int nameLength = readSpellQuoted(&p, name, sizeof name);
    int c = -1;
    if (nameLength <= 0) {
        refuseC5mLine(SELECTCLASS_WORD, spellModName(mod), "no_name", "line=%d text=%.80s", G32(RVA_MOD_LINE), args);
        if (modErrorBox) modErrorBox("CoE5SE: No class name in double quotes");
    } else if (nameLength >= CLASS_NAME_SIZE) {
        refuseC5mLine(SELECTCLASS_WORD, spellModName(mod), "name_too_long", "line=%d text=%.80s max=%d", G32(RVA_MOD_LINE), args, CLASS_NAME_SIZE - 1);
        if (modErrorBox) modErrorBox("CoE5SE: Class name longer than %d characters", CLASS_NAME_SIZE - 1);
    } else if ((c = classByName(name)) < 0) {
        refuseC5mLine(SELECTCLASS_WORD, spellModName(mod), "unknown_class", "line=%d name=%s", G32(RVA_MOD_LINE), name);
        if (modErrorBox) modErrorBox("CoE5SE: No class called '%s'", name);
    }
    snprintf(classNameLine, sizeof classNameLine, SELECTCLASS_WORD " %d", c);
    return classNameLine;
}
