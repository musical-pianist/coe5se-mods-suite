// The rules every Mod Settings declaration follows.
#pragma once

// The limits the rules above name.
#define SETTING_NAME_MAX     32
#define SETTING_TEXT_MAX     56
#define SETTING_TIP_MAX      255
#define SETTING_OPTIONS_MIN  2
#define SETTING_OPTIONS_MAX  8
#define SETTINGS_PAGE_LINES  30
#define SETTINGS_BOX_LINES   2
#define SETTINGS_PAGE_BUTTON_LINES 2
#define SETTING_PAGES_MAX    100

// Counts changes to the left-out settings.
static unsigned int leftOutVersion;

// Why a declaration was left out: a reason word and a short phrase.
typedef struct {
    const char *reason;
    char text[80];
} SettingProblem;

// Fills *p with `reason` and the printf-style text.
static int settingProblemV(SettingProblem *p, const char *reason, const char *fmt, va_list ap) {
    p->reason = reason;
    vsnprintf(p->text, sizeof p->text, fmt, ap);
    return 1;
}
static PRINTF_FORMAT(3, 4) int settingProblem(SettingProblem *p, const char *reason, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    settingProblemV(p, reason, fmt, ap);
    va_end(ap);
    return 1;
}

// Rule 1.
static int checkSettingName(const char *what, const char *name, SettingProblem *p) {
    if (!name || !name[0]) return settingProblem(p, "no_name", "no %s", what);
    size_t len = strlen(name);
    if (len > SETTING_NAME_MAX)
        return settingProblem(p, "name_too_long", "%s over %d characters", what, SETTING_NAME_MAX);
    for (const char *c = name; *c; c++)
        if (!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_'))
            return settingProblem(p, "bad_name", "%s has a character other than letters, digits, _", what);
    if (!((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z')))
        return settingProblem(p, "name_not_letter_first", "%s must start with a letter", what);
    return 0;
}

// Rule 3.
static int checkSettingText(const char *what, const char *text, int max, SettingProblem *p) {
    if ((!text || !text[0]) && strcmp(what, "tip"))
        return settingProblem(p, "empty_text", "empty %s", what);
    if (text && (int)strlen(text) > max)
        return settingProblem(p, "text_too_long", "%s over %d characters", what, max);
    return 0;
}

// Rule 5 and rule 4 for a choice.
static int checkSettingChoice(int optionCount, int defaultIndex, SettingProblem *p) {
    if (optionCount < SETTING_OPTIONS_MIN)
        return settingProblem(p, "too_few_options", "needs %d to %d options, has %d", SETTING_OPTIONS_MIN, SETTING_OPTIONS_MAX,
                              optionCount);
    if (optionCount > SETTING_OPTIONS_MAX)
        return settingProblem(p, "too_many_options", "needs %d to %d options, has %d", SETTING_OPTIONS_MIN, SETTING_OPTIONS_MAX,
                              optionCount);
    if (defaultIndex < 0 || defaultIndex >= optionCount)
        return settingProblem(p, "default_out_of_range", "default %d is not an option (0 to %d)", defaultIndex,
                              optionCount - 1);
    return 0;
}

// Rule 4 for an on/off setting.
static int checkSettingOnOffDefault(int defaultValue, SettingProblem *p) {
    if (defaultValue != 0 && defaultValue != 1)
        return settingProblem(p, "default_out_of_range", "default %d is not 0 or 1", defaultValue);
    return 0;
}

// Rule 9 at a [page] line, where `pagesBefore` of the mod's pages were kept before it.
static int checkSettingPageCount(int pagesBefore, SettingProblem *p) {
    if (pagesBefore >= SETTING_PAGES_MAX) return settingProblem(p, "too_many_pages", "a mod has at most %d pages", SETTING_PAGES_MAX);
    return 0;
}

// Rule 9 once a page's settings are read, where `keptSettings` of them were kept.
static int checkSettingPageSettings(int keptSettings, SettingProblem *p) {
    if (keptSettings < 1) return settingProblem(p, "empty_page", "page has no settings");
    return 0;
}

// Rule 7.
static int settingPageLines(int isChoice, int optionCount, int startsBox, int *boxOpen) {
    if (isChoice) {
        *boxOpen = 0;
        return SETTINGS_BOX_LINES + optionCount;
    }
    int lines = 1 + (startsBox || !*boxOpen ? SETTINGS_BOX_LINES : 0);
    *boxOpen = 1;
    return lines;
}
