// Translating what goes into the game's text: a GUI call's %s arguments, which the game leaves as they are.

// ---- Translating what goes into the game's text --------------------------------------------
// The game translates only a GUI call's format string, not its %s arguments.
typedef int (*TextFormatFn)(u64 options, char *buf, size_t size, const char *fmt, void *locale, va_list args);
static TextFormatFn nextTextFormat;

#define MAX_FORMAT_ARGS    32
#define MAX_TRANSLATED_ARGS 8
#define TRANSLATED_ARG_SIZE 2048

// Marks each argument slot of a printf format that is a narrow %s.
static int scanFormatArgs(const char *fmt, int stringArg[MAX_FORMAT_ARGS]) {
    int slots = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') continue;
        if (*++p == '%') continue;
        while (*p && strchr("-+ #0", *p)) p++;
        if (*p == '*') { if (slots >= MAX_FORMAT_ARGS) return -1; stringArg[slots++] = 0; p++; }
        else while (*p >= '0' && *p <= '9') p++;
        if (*p == '.') {
            p++;
            if (*p == '*') { if (slots >= MAX_FORMAT_ARGS) return -1; stringArg[slots++] = 0; p++; }
            else while (*p >= '0' && *p <= '9') p++;
        }
        int wide = 0;
        if (!strncmp(p, "I64", 3) || !strncmp(p, "I32", 3)) p += 3;
        else if (*p == 'l' || *p == 'w') { wide = 1; p++; if (*p == 'l') p++; }
        else if (*p == 'h') { p++; if (*p == 'h') p++; }
        else if (*p && strchr("LIjzt", *p)) p++;
        if (!*p || !strchr("cCdiouxXeEfFgGaApsSZ", *p) || slots >= MAX_FORMAT_ARGS) return -1;
        stringArg[slots++] = *p == 's' && !wide;
    }
    return slots;
}

// Replaces each %s argument that has a translation with the translated string.
static void translateStringArgs(const int *stringArg, int slots, u64 *args,
                                char translated[MAX_TRANSLATED_ARGS][TRANSLATED_ARG_SIZE]) {
    for (int k = 0, used = 0; k < slots && used < MAX_TRANSLATED_ARGS; k++) {
        const char *s = (const char *)args[k];
        if (!stringArg[k] || !s || strlen(s) >= TRANSLATED_ARG_SIZE - 1) continue;
        translateText(s, translated[used], TRANSLATED_ARG_SIZE);
        if (strcmp(translated[used], s)) args[k] = (u64)translated[used++];
    }
}

static char translatedArgs[MAX_TRANSLATED_ARGS][TRANSLATED_ARG_SIZE];
static int busy;

// Wraps the GUI functions' formatting call, so their %s arguments translate.
static int textFormatHook(u64 options, char *buf, size_t size, const char *fmt, void *locale, va_list args) {
    int stringArg[MAX_FORMAT_ARGS];
    int slots = fmt && !busy ? scanFormatArgs(fmt, stringArg) : -1;
    if (slots <= 0) return nextTextFormat(options, buf, size, fmt, locale, args);
    u64 copy[MAX_FORMAT_ARGS];
    memcpy(copy, args, slots * sizeof copy[0]);
    busy = 1;
    translateStringArgs(stringArg, slots, copy, translatedArgs);
    busy = 0;
    return nextTextFormat(options, buf, size, fmt, locale, (va_list)copy);
}

// The display-name builders format their text before drawing it.
#define MAX_BUILD_ARGS 5
typedef int (*TextBuildFn)(char *buf, u64 size, const char *fmt, u64, u64, u64, u64, u64);
static TextBuildFn nextTextBuild;

static int textBuildHook(char *buf, u64 size, const char *fmt, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5) {
    static char translatedFormat[TRANSLATED_ARG_SIZE];
    int stringArg[MAX_FORMAT_ARGS];
    int slots = fmt && !busy ? scanFormatArgs(fmt, stringArg) : -1;
    if (slots < 0 || slots > MAX_BUILD_ARGS || strlen(fmt) >= TRANSLATED_ARG_SIZE - 1)
        return nextTextBuild(buf, size, fmt, a1, a2, a3, a4, a5);
    u64 args[MAX_BUILD_ARGS] = {a1, a2, a3, a4, a5};
    busy = 1;
    translateText(fmt, translatedFormat, sizeof translatedFormat);
    translateStringArgs(stringArg, slots, args, translatedArgs);
    busy = 0;
    return nextTextBuild(buf, size, translatedFormat, args[0], args[1], args[2], args[3], args[4]);
}
