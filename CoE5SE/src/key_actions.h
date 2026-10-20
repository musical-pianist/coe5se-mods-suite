// The key actions: every line of the game's Keyboard Shortcuts list that a key runs.
#pragma once

// The screens whose keys the list covers.
enum {
    KEY_SCREEN_MAP = 1,
    KEY_SCREEN_ARMY,
    KEY_SCREEN_BATTLE,
    KEY_SCREEN_MAP_EDITOR,
    KEY_SCREEN_BATTLE_EDITOR,
    KEY_SCREEN_RECRUIT,
    KEY_SCREEN_PARTICIPANTS,
    KEY_SCREEN_VIEW_ARMY,
    KEY_SCREEN_UNIT_LIST,
    KEY_SCREEN_FILE_BROWSER,
    KEY_SCREEN_COUNT
};
#define KEY_BOUND 1
#define KEY_HELD  2

// clang-format off
#define KEY_ACTION_LIST(KEY_ACTION) \
    KEY_ACTION(MAP_SHORTCUTS_LIST,       mapShortcutsList,      MAP, BOUND, 0x2e,  '?',                  1, "List keyboard shortcuts") \
    KEY_ACTION(MAP_RECRUIT,              mapRecruit,            MAP, BOUND, 2,     'r',                  2, "Recruit") \
    KEY_ACTION(MAP_END_TURN,             mapEndTurn,            MAP, BOUND, 7,     'y',                  4, "End turn") \
    KEY_ACTION(MAP_MESSAGES,             mapMessages,           MAP, BOUND, 0xd,   'm',                  5, "View messages") \
    KEY_ACTION(MAP_NEXT_COMMANDER,       mapNextCommander,      MAP, BOUND, 1,     'n',                  6, "Next commander") \
    KEY_ACTION(MAP_PREVIOUS_COMMANDER,   mapPreviousCommander,  MAP, BOUND, 0x4c,  'j',                  7, "Previous commander") \
    KEY_ACTION(MAP_COMMANDER_DONE,       mapCommanderDone,      MAP, BOUND, 0x11,  KEY_SPACE,            8, "Commander is done for this turn") \
    KEY_ACTION(MAP_TRANSFER,             mapTransfer,           MAP, BOUND, 4,     't',                  9, "Transfer troops") \
    KEY_ACTION(MAP_SPECIAL_POWER,        mapSpecialPower,       MAP, BOUND, 5,     'p',                 10, "Use commander's special power") \
    KEY_ACTION(MAP_UNIT_OVERVIEW,        mapUnitOverview,       MAP, BOUND, 0x30,  KEY_F1,              11, "Unit overview") \
    KEY_ACTION(MAP_PLAYER_OVERVIEW,      mapPlayerOverview,     MAP, BOUND, 0x2f,  KEY_F2,              12, "Player overview") \
    KEY_ACTION(MAP_CLASS_DESCRIPTION,    mapClassDescription,   MAP, BOUND, 0x47,  KEY_F3,              13, "Class description") \
    KEY_ACTION(MAP_SCORE_GRAPHS,         mapScoreGraphs,        MAP, BOUND, 0x49,  KEY_F5,              14, "View score graphs") \
    KEY_ACTION(MAP_ACTIVE_MODS,          mapActiveMods,         MAP, BOUND, 0x4a,  KEY_F6,              15, "View active mods") \
    KEY_ACTION(MAP_GAME_MENU,            mapGameMenu,           MAP, BOUND, 6,     KEY_F10,             16, "Settings, save, quit (the Game Menu)") \
    KEY_ACTION(MAP_SQUARE_INFO,          mapSquareInfo,         MAP, BOUND, 0x60,  'i',                 17, "Square terrain info") \
    KEY_ACTION(MAP_COMMANDER_STATS,      mapCommanderStats,     MAP, BOUND, 9,     'v',                 18, "View commander stats") \
    KEY_ACTION(MAP_TRADE,                mapTrade,              MAP, BOUND, 0xe,   'l',                 19, "Trade administration") \
    KEY_ACTION(MAP_SENTRY,               mapSentry,             MAP, BOUND, 0xf,   's',                 20, "Sentry commander") \
    KEY_ACTION(MAP_LOCATION_POWER,       mapLocationPower,      MAP, BOUND, 0x10,  'o',                 21, "Use location's special power") \
    KEY_ACTION(MAP_BURN_FOREST_SCREEN,   mapBurnForestScreen,   MAP, BOUND, 0x26,  'b',                 22, "Burn forest screen") \
    KEY_ACTION(MAP_BURN_FOREST_NOW,      mapBurnForestNow,      MAP, BOUND, 0x4b,  'B',                 23, "Burn forest immediately") \
    KEY_ACTION(MAP_HUNT_SLAVES,          mapHuntSlaves,         MAP, BOUND, 0x28,  'h',                 24, "Hunt for slaves") \
    KEY_ACTION(MAP_PLANE_SHIFT,          mapPlaneShift,         MAP, BOUND, 0x27,  'u',                 25, "Plane shift") \
    KEY_ACTION(MAP_CONTINUE,             mapContinue,           MAP, BOUND, 0x62,  ',',                 26, "Continue moving") \
    KEY_ACTION(MAP_CONTINUE_ALL,         mapContinueAll,        MAP, BOUND, 0x61,  '.',                 27, "Continue moving for all commanders") \
    KEY_ACTION(MAP_SQUARE_DETAILS,       mapSquareDetails,      MAP, BOUND, 0x2a,  KEY_CTRL + 'i',      28, "Detailed square info") \
    KEY_ACTION(MAP_ASTROLOGY,            mapAstrology,          MAP, BOUND, 0x4f,  KEY_CTRL + 'a',      29, "Astrology reading") \
    KEY_ACTION(MAP_REPEAT_ORDERS,        mapRepeatOrders,       MAP, BOUND, 0x44d, KEY_CTRL + 'r',      30, "Execute all repeat orders now") \
    KEY_ACTION(MAP_SCRY,                 mapScry,               MAP, BOUND, 0x12,  KEY_ALT + 's',       31, "Scry") \
    KEY_ACTION(MAP_CENTER,               mapCenter,             MAP, BOUND, 0x2d,  KEY_HOME,            32, "Center map on starting citadel") \
    KEY_ACTION(MAP_HIDE_ICONS,           mapHideIcons,          MAP, HELD,  0,     KEY_TAB,             33, "Hide map icons (hold)") \
    KEY_ACTION(MAP_ZOOM_IN,              mapZoomIn,             MAP, HELD,  0,     KEY_PAGE_UP,         34, "Zoom in") \
    KEY_ACTION(MAP_ZOOM_OUT,             mapZoomOut,            MAP, HELD,  0,     KEY_PAGE_DOWN,       34, "Zoom out") \
    KEY_ACTION(MAP_SCROLL_UP,            mapScrollUp,           MAP, HELD,  0,     KEY_UP,              35, "Scroll map up") \
    KEY_ACTION(MAP_SCROLL_DOWN,          mapScrollDown,         MAP, HELD,  0,     KEY_DOWN,            35, "Scroll map down") \
    KEY_ACTION(MAP_SCROLL_LEFT,          mapScrollLeft,         MAP, HELD,  0,     KEY_LEFT,            35, "Scroll map left") \
    KEY_ACTION(MAP_SCROLL_RIGHT,         mapScrollRight,        MAP, HELD,  0,     KEY_RIGHT,           35, "Scroll map right") \
    KEY_ACTION(MAP_MOVE_UP_LEFT,         mapMoveUpLeft,         MAP, BOUND, 0x1e,  'q',                 36, "Move commander up-left") \
    KEY_ACTION(MAP_MOVE_UP,              mapMoveUp,             MAP, BOUND, 0x1f,  'w',                 36, "Move commander up") \
    KEY_ACTION(MAP_MOVE_UP_RIGHT,        mapMoveUpRight,        MAP, BOUND, 0x20,  'e',                 36, "Move commander up-right") \
    KEY_ACTION(MAP_MOVE_LEFT,            mapMoveLeft,           MAP, BOUND, 0x21,  'a',                 36, "Move commander left") \
    KEY_ACTION(MAP_MOVE_RIGHT,           mapMoveRight,          MAP, BOUND, 0x22,  'd',                 36, "Move commander right") \
    KEY_ACTION(MAP_MOVE_DOWN_LEFT,       mapMoveDownLeft,       MAP, BOUND, 0x23,  'z',                 36, "Move commander down-left") \
    KEY_ACTION(MAP_MOVE_DOWN,            mapMoveDown,           MAP, BOUND, 0x24,  'x',                 36, "Move commander down") \
    KEY_ACTION(MAP_MOVE_DOWN_RIGHT,      mapMoveDownRight,      MAP, BOUND, 0x25,  'c',                 36, "Move commander down-right") \
    KEY_ACTION(MAP_LOOK_UP_LEFT,         mapLookUpLeft,         MAP, BOUND, 0x50,  'Q',                 37, "Info on the square up-left") \
    KEY_ACTION(MAP_LOOK_UP,              mapLookUp,             MAP, BOUND, 0x51,  'W',                 37, "Info on the square up") \
    KEY_ACTION(MAP_LOOK_UP_RIGHT,        mapLookUpRight,        MAP, BOUND, 0x52,  'E',                 37, "Info on the square up-right") \
    KEY_ACTION(MAP_LOOK_LEFT,            mapLookLeft,           MAP, BOUND, 0x53,  'A',                 37, "Info on the square to the left") \
    KEY_ACTION(MAP_LOOK_RIGHT,           mapLookRight,          MAP, BOUND, 0x54,  'D',                 37, "Info on the square to the right") \
    KEY_ACTION(MAP_LOOK_DOWN_LEFT,       mapLookDownLeft,       MAP, BOUND, 0x55,  'Z',                 37, "Info on the square down-left") \
    KEY_ACTION(MAP_LOOK_DOWN,            mapLookDown,           MAP, BOUND, 0x56,  'X',                 37, "Info on the square down") \
    KEY_ACTION(MAP_LOOK_DOWN_RIGHT,      mapLookDownRight,      MAP, BOUND, 0x57,  'C',                 37, "Info on the square down-right") \
    KEY_ACTION(MAP_LOOK_HERE,            mapLookHere,           MAP, BOUND, 0x58,  'S',                 37, "Info on the commander's square") \
    KEY_ACTION(ARMY_SHORTCUTS_LIST,      armyShortcutsList,     ARMY, BOUND, 8,    '?',                 39, "List keyboard shortcuts") \
    KEY_ACTION(ARMY_SELECT_ALL,          armySelectAll,         ARMY, BOUND, 2,    'a',                 40, "Select/deselect all units") \
    KEY_ACTION(ARMY_SELECT_AFFLICTED,    armySelectAfflicted,   ARMY, BOUND, 6,    'b',                 41, "Select units with battle afflictions") \
    KEY_ACTION(ARMY_DESELECT_SLOW_AP,    armyDeselectSlowAp,    ARMY, BOUND, 0xf,  'c',                 42, "Deselect units with fewer AP left") \
    KEY_ACTION(ARMY_SELECT_FAST,         armySelectFast,        ARMY, BOUND, 5,    'f',                 43, "Select all fast units") \
    KEY_ACTION(ARMY_SELECT_HORROR,       armySelectHorror,      ARMY, BOUND, 0x11, 'h',                 44, "Select horror marked units") \
    KEY_ACTION(ARMY_SHOW_ITEMS,          armyShowItems,         ARMY, BOUND, 0x10, 'i',                 45, "Highlight units with magic items") \
    KEY_ACTION(ARMY_DESELECT_LANDLUBBERS, armyDeselectLandlubbers, ARMY, BOUND, 9, 'l',                 46, "Deselect all landlubbers") \
    KEY_ACTION(ARMY_SELECT_TYPE,         armySelectType,        ARMY, BOUND, 3,    'm',                 47, "Select the type under the pointer") \
    KEY_ACTION(ARMY_SELECT_SLOW,         armySelectSlow,        ARMY, BOUND, 4,    's',                 48, "Select all slow units") \
    KEY_ACTION(ARMY_SELECT_WOUNDED,      armySelectWounded,     ARMY, BOUND, 7,    'w',                 49, "Select all wounded units") \
    KEY_ACTION(ARMY_SELECT_TEN,          armySelectTen,         ARMY, BOUND, 0xc,  '+',                 50, "Select 10 of the type under the pointer") \
    KEY_ACTION(ARMY_DESELECT_TEN,        armyDeselectTen,       ARMY, BOUND, 0xd,  '-',                 50, "Deselect 10 of the type under the pointer") \
    KEY_ACTION(ARMY_SCROLL_UP,           armyScrollUp,          ARMY, HELD,  0,    KEY_UP,              51, "Scroll battlefield up") \
    KEY_ACTION(ARMY_SCROLL_DOWN,         armyScrollDown,        ARMY, HELD,  0,    KEY_DOWN,            51, "Scroll battlefield down") \
    KEY_ACTION(ARMY_SCROLL_LEFT,         armyScrollLeft,        ARMY, HELD,  0,    KEY_LEFT,            51, "Scroll battlefield left") \
    KEY_ACTION(ARMY_SCROLL_RIGHT,        armyScrollRight,       ARMY, HELD,  0,    KEY_RIGHT,           51, "Scroll battlefield right") \
    KEY_ACTION(ARMY_SELECT_MENU,         armySelectMenu,        ARMY, BOUND, 0xe,  KEY_F2,              55, "Select units menu") \
    KEY_ACTION(ARMY_EXIT,                armyExit,              ARMY, BOUND, 1,    KEY_ESC,             56, "Exit screen") \
    KEY_ACTION(BATTLE_SHORTCUTS_LIST,    battleShortcutsList,   BATTLE, BOUND, 7,  '?',                 58, "List keyboard shortcuts") \
    KEY_ACTION(BATTLE_SCROLL_LEFT,       battleScrollLeft,      BATTLE, HELD,  0,  KEY_LEFT,            59, "Scroll battlefield left") \
    KEY_ACTION(BATTLE_SCROLL_RIGHT,      battleScrollRight,     BATTLE, HELD,  0,  KEY_RIGHT,           59, "Scroll battlefield right") \
    KEY_ACTION(BATTLE_MESSAGES_UP,       battleMessagesUp,      BATTLE, BOUND, 5,  '-',                 61, "Scroll battle messages back") \
    KEY_ACTION(BATTLE_MESSAGES_DOWN,     battleMessagesDown,    BATTLE, BOUND, 6,  '+',                 61, "Scroll battle messages forward") \
    KEY_ACTION(BATTLE_MESSAGES_PAGE_UP,  battleMessagesPageUp,  BATTLE, BOUND, 0xd, KEY_PAGE_UP,        62, "Scroll battle messages a page back") \
    KEY_ACTION(BATTLE_MESSAGES_PAGE_DOWN, battleMessagesPageDown, BATTLE, BOUND, 0xe, KEY_PAGE_DOWN,    62, "Scroll battle messages a page forward") \
    KEY_ACTION(BATTLE_PAUSE,             battlePause,           BATTLE, BOUND, 3,  KEY_SPACE,           63, "Pause battle") \
    KEY_ACTION(BATTLE_TERRAIN_ONLY,      battleTerrainOnly,     BATTLE, HELD,  0,  KEY_TAB,             64, "View terrain only (hold)") \
    KEY_ACTION(BATTLE_SLOW_MOTION,       battleSlowMotion,      BATTLE, BOUND, 8,  'z',                 65, "Slow motion") \
    KEY_ACTION(BATTLE_NORMAL_SPEED,      battleNormalSpeed,     BATTLE, BOUND, 2,  'n',                 66, "Normal speed") \
    KEY_ACTION(BATTLE_FAST_FORWARD,      battleFastForward,     BATTLE, BOUND, 4,  'f',                 67, "Fast forward") \
    KEY_ACTION(BATTLE_SPEED_UP,          battleSpeedUp,         BATTLE, BOUND, 1,  's',                 68, "Speed up one notch") \
    KEY_ACTION(BATTLE_SKIP,              battleSkip,            BATTLE, BOUND, 9,  'q',                 69, "Skip entire battle") \
    KEY_ACTION(BATTLE_LOG,               battleLog,             BATTLE, BOUND, 0xc, 'l',                70, "Hide/show battle log") \
    KEY_ACTION(BATTLE_GRID,              battleGrid,            BATTLE, BOUND, 0xa, 'g',                71, "Hide/show grid") \
    KEY_ACTION(BATTLE_TERRAIN,           battleTerrain,         BATTLE, BOUND, 0xb, 't',                72, "Hide/show terrain") \
    KEY_ACTION(BATTLE_UNIT_LIST,         battleUnitList,        BATTLE, BOUND, 0xf, KEY_F1,             73, "View unit list") \
    KEY_ACTION(MAP_EDITOR_SHORTCUTS_LIST, mapEditorShortcutsList, MAP_EDITOR, BOUND, 0x13, '?',          75, "List keyboard shortcuts") \
    KEY_ACTION(MAP_EDITOR_MENU,          mapEditorMenu,         MAP_EDITOR, BOUND, 2,    KEY_TAB,       76, "Hide/show menu") \
    KEY_ACTION(MAP_EDITOR_PENCIL_1,      mapEditorPencil1,      MAP_EDITOR, BOUND, 0xb,  '1',           77, "Pencil size 1") \
    KEY_ACTION(MAP_EDITOR_PENCIL_2,      mapEditorPencil2,      MAP_EDITOR, BOUND, 0xc,  '2',           77, "Pencil size 2") \
    KEY_ACTION(MAP_EDITOR_PENCIL_3,      mapEditorPencil3,      MAP_EDITOR, BOUND, 0xd,  '3',           77, "Pencil size 3") \
    KEY_ACTION(MAP_EDITOR_CLEAR_SQUARE,  mapEditorClearSquare,  MAP_EDITOR, BOUND, 6,    'c',           78, "Clear special attributes for square") \
    KEY_ACTION(MAP_EDITOR_NAME_SQUARE,   mapEditorNameSquare,   MAP_EDITOR, BOUND, 0x14, 'n',           79, "Name square") \
    KEY_ACTION(MAP_EDITOR_GATEWAY,       mapEditorGateway,      MAP_EDITOR, BOUND, 0x22, 'g',           83, "Create gateway") \
    KEY_ACTION(MAP_EDITOR_OVERLAY,       mapEditorOverlay,      MAP_EDITOR, BOUND, 0x1f, 'o',           84, "Toggle overlay on/off") \
    KEY_ACTION(MAP_EDITOR_QUIT,          mapEditorQuit,         MAP_EDITOR, BOUND, 1,    'q',           85, "Quit map editor") \
    KEY_ACTION(MAP_EDITOR_RANDOM_WORLD,  mapEditorRandomWorld,  MAP_EDITOR, BOUND, 5,    'r',           86, "Recreate random world") \
    KEY_ACTION(MAP_EDITOR_TERRAIN_LIST,  mapEditorTerrainList,  MAP_EDITOR, BOUND, 7,    't',           87, "Select terrain from a list") \
    KEY_ACTION(MAP_EDITOR_UNFLOOD,       mapEditorUnflood,      MAP_EDITOR, BOUND, 0x29, 'u',           88, "Unflood all squares") \
    KEY_ACTION(MAP_EDITOR_FIXED_TEMP,    mapEditorFixedTemp,    MAP_EDITOR, BOUND, 0x21, 'E',           89, "Set fixed temp on entire map") \
    KEY_ACTION(MAP_EDITOR_REMOVE_FANCY,  mapEditorRemoveFancy,  MAP_EDITOR, BOUND, 0x1b, 'F',           90, "Remove all fancy terrains") \
    KEY_ACTION(MAP_EDITOR_LOAD_OVERLAY,  mapEditorLoadOverlay,  MAP_EDITOR, BOUND, 0x1e, 'O',           91, "Load overlay image") \
    KEY_ACTION(MAP_EDITOR_FIX_TEMPS,     mapEditorFixTemps,     MAP_EDITOR, BOUND, 0x1d, 'T',           92, "Fix temperatures") \
    KEY_ACTION(MAP_EDITOR_CIRCULAR_TEMP, mapEditorCircularTemp, MAP_EDITOR, BOUND, 0x27, KEY_CTRL + 'u', 93, "Circular temperature on map") \
    KEY_ACTION(MAP_EDITOR_LOAD,          mapEditorLoad,         MAP_EDITOR, BOUND, 8,    KEY_CTRL + 'l', 94, "Load map") \
    KEY_ACTION(MAP_EDITOR_DRAW_TEMP,     mapEditorDrawTemp,     MAP_EDITOR, BOUND, 0x25, KEY_CTRL + 'e', 95, "Enter temperature to draw with") \
    KEY_ACTION(MAP_EDITOR_RESIZE,        mapEditorResize,       MAP_EDITOR, BOUND, 0x26, KEY_CTRL + 'r', 96, "Resize map (scale)") \
    KEY_ACTION(MAP_EDITOR_SAVE,          mapEditorSave,         MAP_EDITOR, BOUND, 9,    KEY_CTRL + 's', 97, "Save map") \
    KEY_ACTION(MAP_EDITOR_REMOVE_COLD,   mapEditorRemoveCold,   MAP_EDITOR, BOUND, 0x20, KEY_CTRL + 't', 98, "Remove all cold temperatures") \
    KEY_ACTION(MAP_EDITOR_REDUCE_SQUARE, mapEditorReduceSquare, MAP_EDITOR, BOUND, 0x2a, KEY_ALT + 'r',  99, "Reduce square") \
    KEY_ACTION(MAP_EDITOR_DESTROY_SQUARE, mapEditorDestroySquare, MAP_EDITOR, BOUND, 0x2b, KEY_ALT + 'd', 100, "Destroy square") \
    KEY_ACTION(MAP_EDITOR_MOVE_UP,       mapEditorMoveUp,       MAP_EDITOR, BOUND, 0xf,  'U',          101, "Move entire map up") \
    KEY_ACTION(MAP_EDITOR_MOVE_LEFT,     mapEditorMoveLeft,     MAP_EDITOR, BOUND, 0x10, 'H',          101, "Move entire map left") \
    KEY_ACTION(MAP_EDITOR_MOVE_DOWN,     mapEditorMoveDown,     MAP_EDITOR, BOUND, 0x11, 'J',          101, "Move entire map down") \
    KEY_ACTION(MAP_EDITOR_MOVE_RIGHT,    mapEditorMoveRight,    MAP_EDITOR, BOUND, 0x12, 'K',          101, "Move entire map right") \
    KEY_ACTION(MAP_EDITOR_ZOOM_IN,       mapEditorZoomIn,       MAP_EDITOR, HELD,  0,    KEY_PAGE_UP,  106, "Zoom in") \
    KEY_ACTION(MAP_EDITOR_ZOOM_OUT,      mapEditorZoomOut,      MAP_EDITOR, HELD,  0,    KEY_PAGE_DOWN, 107, "Zoom out") \
    KEY_ACTION(MAP_EDITOR_SCROLL_UP,     mapEditorScrollUp,     MAP_EDITOR, HELD,  0,    KEY_UP,       108, "Scroll map up") \
    KEY_ACTION(MAP_EDITOR_SCROLL_DOWN,   mapEditorScrollDown,   MAP_EDITOR, HELD,  0,    KEY_DOWN,     108, "Scroll map down") \
    KEY_ACTION(MAP_EDITOR_SCROLL_LEFT,   mapEditorScrollLeft,   MAP_EDITOR, HELD,  0,    KEY_LEFT,     108, "Scroll map left") \
    KEY_ACTION(MAP_EDITOR_SCROLL_RIGHT,  mapEditorScrollRight,  MAP_EDITOR, HELD,  0,    KEY_RIGHT,    108, "Scroll map right") \
    KEY_ACTION(MAP_EDITOR_PLANE_ABOVE,   mapEditorPlaneAbove,   MAP_EDITOR, HELD,  0,    '<',          109, "Plane above") \
    KEY_ACTION(MAP_EDITOR_PLANE_BELOW,   mapEditorPlaneBelow,   MAP_EDITOR, HELD,  0,    '>',          110, "Plane below") \
    KEY_ACTION(MAP_EDITOR_REFRESH,       mapEditorRefresh,      MAP_EDITOR, BOUND, 0x1c, KEY_F5,       111, "Refresh map") \
    KEY_ACTION(BATTLE_EDITOR_SHORTCUTS_LIST, battleEditorShortcutsList, BATTLE_EDITOR, BOUND, 0x18, '?', 113, "List keyboard shortcuts") \
    KEY_ACTION(BATTLE_EDITOR_GROUND_MODE, battleEditorGroundMode, BATTLE_EDITOR, BOUND, 0x19, KEY_TAB, 114, "Switch to/from ground edit mode") \
    KEY_ACTION(BATTLE_EDITOR_CENTER_LINE, battleEditorCenterLine, BATTLE_EDITOR, BOUND, 6, KEY_CTRL + 'w', 115, "Show center line") \
    KEY_ACTION(BATTLE_EDITOR_REPLACER,   battleEditorReplacer,  BATTLE_EDITOR, BOUND, 0x1c, 'r',       116, "Replacer") \
    KEY_ACTION(BATTLE_EDITOR_DRAW,       battleEditorDraw,      BATTLE_EDITOR, BOUND, 0x1d, 'q',       117, "Draw normally") \
    KEY_ACTION(BATTLE_EDITOR_LOAD,       battleEditorLoad,      BATTLE_EDITOR, BOUND, 8,    KEY_CTRL + 'l', 118, "Load") \
    KEY_ACTION(BATTLE_EDITOR_SAVE,       battleEditorSave,      BATTLE_EDITOR, BOUND, 9,    KEY_CTRL + 's', 119, "Save") \
    KEY_ACTION(BATTLE_EDITOR_MONSTER,    battleEditorMonster,   BATTLE_EDITOR, BOUND, 4,    'm',       120, "Select monster") \
    KEY_ACTION(BATTLE_EDITOR_CLEAR,      battleEditorClear,     BATTLE_EDITOR, BOUND, 5,    'C',       121, "Clear all") \
    KEY_ACTION(BATTLE_EDITOR_SCROLL_UP,  battleEditorScrollUp,  BATTLE_EDITOR, BOUND, 0x14, 'u',       122, "Scroll terrain up") \
    KEY_ACTION(BATTLE_EDITOR_SCROLL_LEFT, battleEditorScrollLeft, BATTLE_EDITOR, BOUND, 0x15, 'h',     122, "Scroll terrain left") \
    KEY_ACTION(BATTLE_EDITOR_SCROLL_DOWN, battleEditorScrollDown, BATTLE_EDITOR, BOUND, 0x16, 'j',     122, "Scroll terrain down") \
    KEY_ACTION(BATTLE_EDITOR_SCROLL_RIGHT, battleEditorScrollRight, BATTLE_EDITOR, BOUND, 0x17, 'k',   122, "Scroll terrain right") \
    KEY_ACTION(BATTLE_EDITOR_NO_DEPLOY,  battleEditorNoDeploy,  BATTLE_EDITOR, BOUND, 0x1b, KEY_ALT + 'b', 123, "No deploy mark") \
    KEY_ACTION(BATTLE_EDITOR_CATAPULT,   battleEditorCatapult,  BATTLE_EDITOR, BOUND, 0x22, KEY_ALT + 'c', 124, "Catapult mark") \
    KEY_ACTION(BATTLE_EDITOR_EFFIGY,     battleEditorEffigy,    BATTLE_EDITOR, BOUND, 0x26, KEY_ALT + 'e', 125, "Effigy mark") \
    KEY_ACTION(BATTLE_EDITOR_MAGE,       battleEditorMage,      BATTLE_EDITOR, BOUND, 0x21, KEY_ALT + 'm', 126, "Mage mark") \
    KEY_ACTION(BATTLE_EDITOR_MELEE,      battleEditorMelee,     BATTLE_EDITOR, BOUND, 0x24, KEY_ALT + 's', 127, "Melee soldier mark") \
    KEY_ACTION(BATTLE_EDITOR_DEMON_HAND, battleEditorDemonHand, BATTLE_EDITOR, BOUND, 0x1e, KEY_ALT + 'd', 128, "Demon hand mark") \
    KEY_ACTION(BATTLE_EDITOR_DEMON_HEAD, battleEditorDemonHead, BATTLE_EDITOR, BOUND, 0x1f, KEY_ALT + 'h', 129, "Demon head mark") \
    KEY_ACTION(BATTLE_EDITOR_FIREWALL,   battleEditorFirewall,  BATTLE_EDITOR, BOUND, 0x20, KEY_ALT + 'v', 130, "Firewall mark") \
    KEY_ACTION(BATTLE_EDITOR_NO_TREES,   battleEditorNoTrees,   BATTLE_EDITOR, BOUND, 0x23, KEY_ALT + 't', 131, "No random trees/bushes here") \
    KEY_ACTION(BATTLE_EDITOR_CLEAR_NO_TREES, battleEditorClearNoTrees, BATTLE_EDITOR, BOUND, 0x25, KEY_ALT + 'y', 132, "Clear no random trees/bushes mark") \
    KEY_ACTION(RECRUIT_CLEAR_HERE,       recruitClearHere,      RECRUIT, BOUND, 3,    'c',              136, "Clear auto-recruitment at this location") \
    KEY_ACTION(RECRUIT_CLEAR_ALL,        recruitClearAll,       RECRUIT, BOUND, 4,    'v',              137, "Clear auto-recruitment everywhere") \
    KEY_ACTION(RECRUIT_LOCATION_1,       recruitLocation1,      RECRUIT, BOUND, 1000, '1',              138, "Recruitment location 1") \
    KEY_ACTION(RECRUIT_LOCATION_2,       recruitLocation2,      RECRUIT, BOUND, 1001, '2',              138, "Recruitment location 2") \
    KEY_ACTION(RECRUIT_LOCATION_3,       recruitLocation3,      RECRUIT, BOUND, 1002, '3',              138, "Recruitment location 3") \
    KEY_ACTION(RECRUIT_LOCATION_4,       recruitLocation4,      RECRUIT, BOUND, 1003, '4',              138, "Recruitment location 4") \
    KEY_ACTION(RECRUIT_LOCATION_5,       recruitLocation5,      RECRUIT, BOUND, 1004, '5',              138, "Recruitment location 5") \
    KEY_ACTION(RECRUIT_LOCATION_6,       recruitLocation6,      RECRUIT, BOUND, 1005, '6',              138, "Recruitment location 6") \
    KEY_ACTION(RECRUIT_LOCATION_7,       recruitLocation7,      RECRUIT, BOUND, 1006, '7',              138, "Recruitment location 7") \
    KEY_ACTION(RECRUIT_LOCATION_8,       recruitLocation8,      RECRUIT, BOUND, 1007, '8',              138, "Recruitment location 8") \
    KEY_ACTION(RECRUIT_LOCATION_9,       recruitLocation9,      RECRUIT, BOUND, 1008, '9',              138, "Recruitment location 9") \
    KEY_ACTION(RECRUIT_PREVIOUS,         recruitPrevious,       RECRUIT, BOUND, 5,    KEY_LEFT,         139, "Previous recruitment location") \
    KEY_ACTION(RECRUIT_NEXT,             recruitNext,           RECRUIT, BOUND, 6,    KEY_RIGHT,        140, "Next recruitment location") \
    KEY_ACTION(RECRUIT_SHORTCUTS_LIST,   recruitShortcutsList,  RECRUIT, BOUND, 2,    '?',              141, "List keyboard shortcuts") \
    KEY_ACTION(RECRUIT_EXIT,             recruitExit,           RECRUIT, BOUND, 1,    KEY_ESC,          142, "Exit recruitment") \
    KEY_ACTION(RECRUIT_EXIT_ENTER,       recruitExitEnter,      RECRUIT, BOUND, 1,    KEY_ENTER,        142, "Exit recruitment, second key") \
    KEY_ACTION(PARTICIPANTS_ADD,         participantsAdd,       PARTICIPANTS, BOUND, 2,   'n',          144, "Add new player") \
    KEY_ACTION(PARTICIPANTS_COLORS,      participantsColors,    PARTICIPANTS, BOUND, 0xb, 'r',          145, "Randomize player colors by team") \
    KEY_ACTION(PARTICIPANTS_ALL_HUMAN,   participantsAllHuman,  PARTICIPANTS, BOUND, 6,   'H',          146, "Set all players to human controlled") \
    KEY_ACTION(PARTICIPANTS_ALL_AI,      participantsAllAi,     PARTICIPANTS, BOUND, 7,   'A',          147, "Set all players to AI controlled") \
    KEY_ACTION(PARTICIPANTS_AI_UP,       participantsAiUp,      PARTICIPANTS, BOUND, 9,   '+',          148, "Increase all AI levels by 1") \
    KEY_ACTION(PARTICIPANTS_AI_DOWN,     participantsAiDown,    PARTICIPANTS, BOUND, 0xa, '-',          149, "Decrease all AI levels by 1") \
    KEY_ACTION(PARTICIPANTS_REMOVE,      participantsRemove,    PARTICIPANTS, BOUND, 5,   KEY_BACKSPACE, 150, "Remove last player") \
    KEY_ACTION(PARTICIPANTS_OK,          participantsOk,        PARTICIPANTS, BOUND, 1,   KEY_ENTER,    151, "Ok") \
    KEY_ACTION(PARTICIPANTS_CANCEL,      participantsCancel,    PARTICIPANTS, BOUND, 3,   KEY_ESC,      152, "Cancel") \
    KEY_ACTION(VIEW_ARMY_SHOW_ITEMS,     viewArmyShowItems,     VIEW_ARMY, BOUND, 4, 'i',               154, "Highlight units with magic items") \
    KEY_ACTION(VIEW_ARMY_SHORTCUTS_LIST, viewArmyShortcutsList, VIEW_ARMY, BOUND, 3, '?',               155, "List keyboard shortcuts") \
    KEY_ACTION(VIEW_ARMY_SCROLL_LEFT,    viewArmyScrollLeft,    VIEW_ARMY, HELD,  0, KEY_LEFT,          156, "Scroll battlefield left") \
    KEY_ACTION(VIEW_ARMY_SCROLL_RIGHT,   viewArmyScrollRight,   VIEW_ARMY, HELD,  0, KEY_RIGHT,         156, "Scroll battlefield right") \
    KEY_ACTION(VIEW_ARMY_TERRAIN_ONLY,   viewArmyTerrainOnly,   VIEW_ARMY, HELD,  0, KEY_TAB,           158, "View terrain only (hold)") \
    KEY_ACTION(VIEW_ARMY_EXIT,           viewArmyExit,          VIEW_ARMY, BOUND, 1, KEY_ENTER,         159, "Exit screen") \
    KEY_ACTION(VIEW_ARMY_EXIT_ESC,       viewArmyExitEsc,       VIEW_ARMY, BOUND, 1, KEY_ESC,           159, "Exit screen, second key") \
    KEY_ACTION(UNIT_LIST_COMMANDERS,     unitListCommanders,    UNIT_LIST, BOUND, 3, 'c',               161, "Only show commanders") \
    KEY_ACTION(UNIT_LIST_SHOW_ALL,       unitListShowAll,       UNIT_LIST, BOUND, 2, 'A',               162, "Show all (terrain units too)") \
    KEY_ACTION(UNIT_LIST_SHORTCUTS_LIST, unitListShortcutsList, UNIT_LIST, BOUND, 4, '?',               163, "List keyboard shortcuts") \
    KEY_ACTION(UNIT_LIST_EXIT,           unitListExit,          UNIT_LIST, BOUND, 1, KEY_ENTER,         164, "Exit screen") \
    KEY_ACTION(UNIT_LIST_EXIT_ESC,       unitListExitEsc,       UNIT_LIST, BOUND, 1, KEY_ESC,           164, "Exit screen, second key") \
    KEY_ACTION(FILE_BROWSER_DELETE,      fileBrowserDelete,     FILE_BROWSER, BOUND, 7, KEY_DELETE,     166, "Delete save file under pointer") \
    KEY_ACTION(FILE_BROWSER_DELETE_CTRL, fileBrowserDeleteCtrl, FILE_BROWSER, BOUND, 7, KEY_CTRL + 'd', 166, "Delete save file, second key") \
    KEY_ACTION(FILE_BROWSER_SORT_TIME,   fileBrowserSortTime,   FILE_BROWSER, BOUND, 6, KEY_CTRL + 't', 167, "Sort by time") \
    KEY_ACTION(FILE_BROWSER_SORT_NAME,   fileBrowserSortName,   FILE_BROWSER, BOUND, 5, KEY_CTRL + 'n', 168, "Sort by name") \
    KEY_ACTION(FILE_BROWSER_SHORTCUTS_LIST, fileBrowserShortcutsList, FILE_BROWSER, BOUND, 8, '?',     169, "List keyboard shortcuts") \
    KEY_ACTION(FILE_BROWSER_CANCEL,      fileBrowserCancel,     FILE_BROWSER, BOUND, 2, KEY_ESC,        170, "Cancel")
// clang-format on

// The action numbers: KEY_ACTION_NONE is no action, and KEY_ACTION_COUNT is one past the last.
#define KEY_ACTION_ENUM(name, lua, screen, how, id, key, row, label) KEY_ACTION_##name,
enum { KEY_ACTION_NONE, KEY_ACTION_LIST(KEY_ACTION_ENUM) KEY_ACTION_COUNT };
#undef KEY_ACTION_ENUM
