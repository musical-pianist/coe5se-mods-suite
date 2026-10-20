// Key names: how a key value is written in a settings file and shown on a settings page.
#pragma once
#include <ctype.h>

// The punctuation keys that have a character of their own on a US keyboard.
#define KEY_OWN_PUNCTUATION "`-=[]\\;',./"

typedef struct { const char *name; int key; } KeyName;
static const KeyName KEY_NAMES[] = {
    {"Space", KEY_SPACE}, {"Tab", KEY_TAB}, {"Enter", KEY_ENTER}, {"Esc", KEY_ESC}, {"Backspace", KEY_BACKSPACE},
    {"Delete", KEY_DELETE}, {"Home", KEY_HOME}, {"End", KEY_END}, {"PgUp", KEY_PAGE_UP}, {"PgDn", KEY_PAGE_DOWN},
    {"Up", KEY_UP}, {"Down", KEY_DOWN}, {"Left", KEY_LEFT}, {"Right", KEY_RIGHT},
    {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4}, {"F5", KEY_F5}, {"F6", KEY_F6},
    {"F7", KEY_F7}, {"F8", KEY_F8}, {"F9", KEY_F9}, {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
};
#define KEY_NAME_COUNT (int)(sizeof KEY_NAMES / sizeof KEY_NAMES[0])

// The name of a named key, or NULL.
static const char *keyNamedName(int base) {
    for (int i = 0; i < KEY_NAME_COUNT; i++)
        if (KEY_NAMES[i].key == base) return KEY_NAMES[i].name;
    return NULL;
}

// The key without its modifier, and the modifier alone (0, KEY_CTRL, KEY_ALT, or KEY_CTRL_SHIFT).
static int keyBase(int value) { return value % KEY_CTRL; }
static int keyModifier(int value) { return value - keyBase(value); }

// 1 if `value` is a key value (KEY_NONE included).
static int keyValueIsValid(int value) {
    if (value == KEY_NONE) return 1;
    if (value < 0 || value >= KEY_CTRL_SHIFT + KEY_CTRL) return 0;
    int base = keyBase(value), modifier = keyModifier(value);
    if (base <= 0) return 0;
    if (!modifier) return (base > ' ' && base < 127) || keyNamedName(base);
    if (modifier == KEY_CTRL_SHIFT) return base >= 'a' && base <= 'z';
    if ((base >= 'a' && base <= 'z') || (base >= '0' && base <= '9') || (base < 128 && strchr(KEY_OWN_PUNCTUATION, base)))
        return 1;
    if (!keyNamedName(base) || base == KEY_ESC) return 0;
    return !(modifier == KEY_ALT && (base == KEY_TAB || base == KEY_ENTER));
}

// Writes the name of key value `value` into `out`: "ctrl+i", "Space", "B", "none".
static void keyValueName(int value, char *out, size_t size) {
    if (!size) return;
    if (value == KEY_NONE) {
        snprintf(out, size, "none");
        return;
    }
    if (!keyValueIsValid(value)) {
        snprintf(out, size, "?");
        return;
    }
    int base = keyBase(value), modifier = keyModifier(value);
    const char *prefix = modifier == KEY_CTRL ? "ctrl+" : modifier == KEY_ALT ? "alt+" : modifier == KEY_CTRL_SHIFT ? "ctrl+shift+" : "";
    const char *named = keyNamedName(base);
    if (named) snprintf(out, size, "%s%s", prefix, named);
    else snprintf(out, size, "%s%c", prefix, base);
}

// Takes a modifier word (ctrl, alt, shift) and its +, with the spaces around it, from *p.
static int takeKeyModifier(const char **p) {
    static const struct { const char *word; int modifier; } WORDS[] = {{"ctrl", KEY_CTRL}, {"alt", KEY_ALT}, {"shift", 1}};
    for (int i = 0; i < 3; i++) {
        size_t n = strlen(WORDS[i].word);
        if (_strnicmp(*p, WORDS[i].word, n)) continue;
        const char *q = *p + n;
        while (*q == ' ') q++;
        if (*q != '+') continue;
        q++;
        while (*q == ' ') q++;
        if (!*q) continue;
        *p = q;
        return WORDS[i].modifier;
    }
    return 0;
}

// Reads a key name (above) into *out.
static int keyValueFromName(const char *text, int *out) {
    if (!text) return 0;
    while (*text == ' ') text++;
    char name[40];
    if (strlen(text) >= sizeof name) return 0;
    snprintf(name, sizeof name, "%.39s", text);
    size_t len = strlen(name);
    while (len && name[len - 1] == ' ') name[--len] = '\0';
    if (!len) return 0;
    if (!_stricmp(name, "none")) {
        *out = KEY_NONE;
        return 1;
    }
    const char *p = name;
    int ctrl = 0, alt = 0, shift = 0, m;
    while (strlen(p) > 1 && (m = takeKeyModifier(&p)) != 0) {
        if (m == KEY_CTRL) ctrl++;
        else if (m == KEY_ALT) alt++;
        else shift++;
    }
    int base = 0;
    if (strlen(p) == 1) base = (unsigned char)p[0];
    else
        for (int i = 0; i < KEY_NAME_COUNT && !base; i++)
            if (!_stricmp(p, KEY_NAMES[i].name)) base = KEY_NAMES[i].key;
    if (!base || ctrl > 1 || alt > 1 || shift > 1 || (alt && (ctrl || shift))) return 0;
    int value;
    if (ctrl && shift) {
        if (!isalpha(base)) return 0;
        value = KEY_CTRL_SHIFT + tolower(base);
    } else if (ctrl || alt) {
        value = (ctrl ? KEY_CTRL : KEY_ALT) + (isalpha(base) ? tolower(base) : base);
    } else if (shift) {
        if (!isalpha(base)) return 0;
        value = toupper(base);
    } else {
        value = base;
    }
    if (!keyValueIsValid(value)) return 0;
    *out = value;
    return 1;
}
