// The plugin API that CoE5SE gives to plugin DLLs.

// A wrapper has the same signature as the call it wraps, and calls `next` to go on.
#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_mac.h"
#endif
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

// Raise it with any change a built plugin sees.
#define PLUGIN_API_VERSION 9

// wrap's priority, for wrapWithPriority (API version 6).
#define WRAP_PRIORITY_DEFAULT 0

// What restartPhase returns (API version 6).
#define RESTART_NONE     0
#define RESTART_ENDING   1
#define RESTART_STARTING 2

// ---- Game numbers ------------------------------------------------------------------------------
// The game's own numbers that a plugin passes to the API or gets back from it.

// The main menu's buttons.
#define MAIN_MENU_START_NEW_GAME   1
#define MAIN_MENU_LOAD_GAME        2
#define MAIN_MENU_PREFERENCES      3
#define MAIN_MENU_NETWORK          4
#define MAIN_MENU_CREDITS          6
#define MAIN_MENU_QUIT             9
#define MAIN_MENU_MODS             10
#define MAIN_MENU_READ_TIP         11
// What a Game Menu line's onClick returns.
#define GAME_MENU_STAY             0
#define GAME_MENU_CLOSE            1
#define GAME_MENU_RESIGN           6
// Which section of the Game Menu a Lua line goes in.
#define GAME_MENU_SECTION_SAVING   0
#define GAME_MENU_SECTION_SETTINGS 1
// Map screen actions, for the game's bind_hotkey(action, key).
#define MAP_ACTION_SPECIAL_POWER   5
#define MAP_ACTION_GAME_MENU       6
#define MAP_ACTION_SQUARE_INFO     96
// The map editor's ids, as SITE_MAP_EDITOR_WAIT returns them.
#define MAP_EDITOR_HIDE_TOOLBOX    2
// Key values: the game's key codes.
#define KEY_NONE                   0
#define KEY_BACKSPACE              8
#define KEY_TAB                    9
#define KEY_ENTER                  13
#define KEY_ESC                    27
#define KEY_SPACE                  32
#define KEY_DELETE                 127
#define KEY_F1                     328
#define KEY_F2                     329
#define KEY_F3                     330
#define KEY_F4                     331
#define KEY_F5                     332
#define KEY_F6                     333
#define KEY_F7                     334
#define KEY_F8                     335
#define KEY_F9                     336
#define KEY_F10                    337
#define KEY_F11                    338
#define KEY_F12                    339
#define KEY_PAGE_DOWN              340
#define KEY_PAGE_UP                341
#define KEY_END                    342
#define KEY_HOME                   343
#define KEY_UP                     344
#define KEY_DOWN                   345
#define KEY_LEFT                   346
#define KEY_RIGHT                  347
#define KEY_CTRL                   100000
#define KEY_ALT                    200000
#define KEY_CTRL_SHIFT             300000
// sendMessage's player and result.
#define MESSAGE_EVERY_HUMAN        (-2)
#define MESSAGE_NOT_ADDED          (-1)
// A player slot's controller.
#define CONTROLLER_REMOVED         (-1)
#define CONTROLLER_HUMAN           0
#define CONTROLLER_AI              1
#define CONTROLLER_INDEPENDENT     2
// A class ability.
#define CLASS_ABILITY_ID_AI_CLASS      203
#define CLASS_ABILITY_ID_RANDOM_SKIP   204
#define CLASS_ABILITY_ID_AI_LEVEL_MIN  210
#define CLASS_ABILITY_ID_AI_LEVEL_MAX  211
#define CLASS_ABILITY_ID_STDTROOPS     222
// An aiclass value.
#define AI_CLASS_HUMAN_ONLY        (-1)
#define AI_CLASS_ANY               0
#define AI_CLASS_AI_ONLY           1
#define AI_CLASS_NEITHER           2
// A corpse kind.
#define CORPSE_KIND_SMALL          0
#define CORPSE_KIND_HUMAN          1
#define CORPSE_KIND_GIANT          2
#define CORPSE_KIND_SMALL_ANIMAL   3
#define CORPSE_KIND_LARGE_ANIMAL   4
#define CORPSE_KIND_MARKATA        5
#define CORPSE_KIND_VANARA         6
#define CORPSE_KIND_BANDAR         7
// A screen wait's mode.
#define GAME_OVER_MODE_PLAIN       4
#define GAME_OVER_MODE_FOG         9
// A new game's setup: new_world's mode, and the setup screens' ids.
#define NEW_WORLD_NORMAL           0
#define NEW_WORLD_NETWORK          1
#define NEW_WORLD_SKIP_PARTICIPANTS 2
#define MAP_SCREEN_OK              1
#define MAP_SETTINGS_OK            9
#define MAP_SETTINGS_SIZE_FIRST    100
#define PARTICIPANTS_OK            1
// The ritual screen: why a ritual can't be cast, and the ids of its rows.
#define RITUAL_REASON_MIRROR       (-2)
#define RITUAL_REASON_MAXCAST      (-3)
#define RITUAL_REASON_IMMORTAL     (-4)
#define RITUAL_REASON_MASTERY      (-6)
#define RITUAL_REASON_PLANE        (-7)
#define RITUAL_ROW_CASTABLE        1000
#define RITUAL_ROW_GREYED          12288
// The game's file browser (Load Game's list) and the square info window.
#define FILE_BROWSER_ROW_FIRST     10000
#define FILE_BROWSER_DELETE        7
#define SQUARE_INFO_OK             1
// The AI's move for one commander.
#define AI_MOVE_NONE               0
#define AI_MOVE_TARGET             1
#define AI_MOVE_EXPLORE            2
#define AI_NO_SQUARE               (-1)
#define AI_EXPLORE_BELOW           50
#define AI_FAR_EXPLORE_BELOW       10

#include "key_actions.h"

// The sites, each with the signature of the game call it is.
typedef enum {
    SITE_MAIN_MENU_WAIT,
    SITE_MAIN_MENU_PLAY_GAME,
    SITE_PLAY_GAME_NEW_WORLD,
    SITE_NEW_WORLD_MAP_SCREEN,
    SITE_NEW_WORLD_INIT_PARTICIPANTS,
    SITE_NEW_WORLD_CREATING_WORLD,
    SITE_MAP_SETTINGS_WAIT,
    SITE_PARTICIPANTS_WAIT,
    SITE_PARTICIPANTS_DIFFICULTY_WAIT,
    SITE_PARTICIPANTS_BUTTON_ROW,
    SITE_GAME_MENU_SAVING_SECTION_END,
    SITE_GAME_MENU_WAIT,
    SITE_RITUAL_SCREEN_ROW,
    SITE_RITUAL_SCREEN_LOCATION_CHECK,
    SITE_RITUAL_SCREEN_CAN_CAST,
    SITE_MAP_SCREEN_WAIT,
    SITE_MAP_MENU_BUTTON_TEXT,

    SITE_GAME_START,
    SITE_MONTH_START,
    SITE_PLAYER_TURN_START,
    SITE_HUMAN_TURN,
    SITE_AI_TURN,
    SITE_PLAYER_TURN_END,
    SITE_GAME_OVER,
    SITE_GAME_SAVE,
    SITE_GAME_LOAD,

    SITE_MODS_RUN,
    SITE_MOD_COMMAND,

    SITE_SQUARE_INFO_SCREEN,
    SITE_SQUARE_INFO_BUTTON_ROW,
    SITE_SQUARE_INFO_WAIT,

    SITE_TEXT_FORMAT,
    SITE_TEXT_BUILD,

    SITE_LOAD_GAME_BROWSER,
    SITE_FILE_BROWSER_ROW_DATE,
    SITE_FILE_BROWSER_WAIT,
    SITE_FILE_BROWSER_HOVERED,
    SITE_MAIN_MENU_MODS_SCREEN,
    SITE_MOD_BANNER_DRAW,
    SITE_MOD_ERROR_DIALOG,
    SITE_GAME_OVER_WAIT,
    SITE_EVENT_LINE,

    SITE_ARMY_WAIT,
    SITE_BATTLE_WAIT,
    SITE_MAP_EDITOR_WAIT,
    SITE_BATTLE_EDITOR_WAIT,
    SITE_RECRUIT_WAIT,
    SITE_VIEW_ARMY_WAIT,
    SITE_UNIT_LIST_WAIT,
    SITE_KEYBOARD_SHORTCUTS_WAIT,
    SITE_UNIT_LIST_SHEET,
    SITE_POLL_EVENT,
    SITE_KEY_TEXT_BAR,
    SITE_KEY_HELP_TEXT,
    SITE_KEY_TEXT_BUTTON,
    SITE_RUN_ONE_MOD,
    SITE_RUN_ONE_MOD_PATH,
    SITE_GAME_MENU_SETTINGS_SECTION_END,
    SITE_ELIMINATED_WARN,
    SITE_WARN_WAIT,
    SITE_TOOLTIP_LINE_DRAW,
    SITE_TURN_MESSAGES,
    SITE_MESSAGES_WAIT,
    SITE_AI_EXPLORE,
    SITE_AI_RALLY,
    SITE_AI_FRIENDLY_MOVE,
    SITE_AI_CONQUER,
    SITE_AI_FAR_EXPLORE,
    SITE_AI_RITUAL_CANDIDATE,
    SITE_AI_RECRUIT,
    SITE_AI_GATHER_TROOPS,
    SITE_AI_PICK_UP_LONER,
    SITE_AI_SPLIT_ARMY,
    SITE_CEREMONY_PERFORM,
    SITE_CEREMONY_COSTS,
    SITE_SEAL_BREAK,
    SITE_SEAL_EVENT,
    SITE_AI_MOVE_COMSTR,
    SITE_MAP_EDITOR,
    SITE_MAP_EDITOR_TOOLBOX_LINE,
    SITE_MAP_EDITOR_DRAW,
    SITE_RUN_ONE_MOD_LINE_READ,
    SITE_COUNT
} CallSite;

// The game's own screen widgets, for a plugin's screens.
#define UI_KEY_ENTER  0x0d
#define UI_KEY_ESCAPE (-1)
// Lua's coe5.ui has these calls too, and more.

typedef struct UiApi {
    void (*screenBegin)(const char *title);
    void (*sectionBegin)(const char *header);
    void (*sectionEnd)(void);
    void (*text)(int id, const char *text);
    void (*checkbox)(int id, int *value, const char *label);
    void (*choice)(int id, int selected, const char *label);
    void (*tooltip)(const char *text);
    void (*buttonRow)(void);
    void (*bindKey)(int id, int key);
    int (*wait)(void);
    void (*listBegin)(void);
    void (*listRow)(int id, const char *text);
    void (*listEnd)(void);
} UiApi;

// One call at an observe-only site, as the functions given to observe see it.
typedef struct CallInfo {
    unsigned long long args[6];
    unsigned long long result;
} CallInfo;

typedef struct PluginApi {
    int version;
    int (*wrap)(CallSite site, void *hook, void **next);
    uintptr_t (*callerRsp)(CallSite site);
    int (*setupAutomated)(void);
    void (*setSetupAutomated)(int on);
    int (*addModCommand)(const char *name, void (*handler)(const char *args, int mod, int level));
    int (*addGameMenuLine)(const char *label, const char *tooltip, int (*visible)(void), int (*onClick)(void));
    int (*sendMessage)(int player, const char *title, const char *text);
    const UiApi *ui;
    int (*addScreen)(const char *label, const char *tooltip, void (*open)(void));
    int (*addMainMenuButton)(const char *label, void (*onClick)(void));
    int (*observe)(CallSite site, void (*before)(const CallInfo *call), void (*after)(const CallInfo *call));
    int (*settingsPluginCount)(void);
    const char *(*settingsPluginName)(int plugin);
    void (*settingsPluginDraw)(int plugin);
    int (*settingsPluginClick)(int id);
    int (*hideMainMenuButton)(int id, const int *hidden);
    int (*addMainMenuButtonAfter)(const char *label, void (*onClick)(void), int vanillaId);
    void (*logLine)(const void *self, const char *text);
    int (*wrapWithPriority)(CallSite site, void *hook, void **next, int priority);
    int (*restartGame)(void);
    int (*restartPhase)(void);
    void *(*frameVariable)(CallSite site, const char *name);
    const int *(*setting)(const char *name);
    int (*rebindKey)(int action, int key);
    int (*addActionKey)(int action, int key);
    int (*actionKey)(int action);
} PluginApi;

#ifdef __APPLE__
#define COE5SE_DLL "coe5se.c5plugin.dylib"
#else
#define COE5SE_DLL "coe5se.c5plugin.dll"
#endif

// Defines your plugin's entry and exports the API version it was built against.
#define COE5SE_PLUGIN_LOAD(apiName) \
    __declspec(dllexport) const int coe5sePluginApiVersion = PLUGIN_API_VERSION; \
    __declspec(dllexport) int coe5sePluginLoad(const PluginApi *apiName)

// Returns the plugin API, or NULL if CoE5SE isn't loaded or hasn't started this plugin.
static inline const PluginApi *jazzyConnect(void) {
    HMODULE coe5se = GetModuleHandleA(COE5SE_DLL);
    typedef const PluginApi *(*GetApiFn)(int);
    GetApiFn getApi = coe5se ? (GetApiFn)(void *)GetProcAddress(coe5se, "jazzyGetApi") : NULL;
    return getApi ? getApi(PLUGIN_API_VERSION) : NULL;
}

// Marks a printf-style function so the compiler checks each call's arguments.
#ifndef PRINTF_FORMAT
#if defined(__MINGW32__) && !defined(__clang__)
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(gnu_printf, fmtIndex, firstArg)))
#elif defined(__GNUC__)
#define PRINTF_FORMAT(fmtIndex, firstArg) __attribute__((format(printf, fmtIndex, firstArg)))
#else
#define PRINTF_FORMAT(fmtIndex, firstArg)
#endif
#endif

// Writes a printf-style line to coe5mod.log, tagged with this DLL's file name.
static inline PRINTF_FORMAT(1, 2) void jazzyLog(const char *fmt, ...) {
    static const PluginApi *connected;
    if (!connected) connected = jazzyConnect();
    if (!connected) return;
    char text[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    connected->logLine((const void *)jazzyLog, text);
}
