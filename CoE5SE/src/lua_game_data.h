// The Lua host's named game data: the names coe5.get and coe5.set take, and where each value lives.

// ---- Named game data: coe5.get(name[, index]), coe5.set(name, [index,] value) ----------------
// Scripts name the values they read and write, and only this table holds the addresses.

// The slot* names take a player slot, 0-31, the game's own slot numbers.
enum {
    T_U8, T_I16, T_I32,
    T_STR,
    T_FLAG_U8,
    T_FLAG_POSITIVE,
    T_FRAME_I32,
    T_FLAG_I32,
    T_PTR_STR,
    T_F32,
    T_FLAG_I16,
    T_FLAG_TWO,
    T_SITE_I32,
    T_ARMY_OWNER,
    T_CLASS_STDTROOPS,
    T_CLASS_ABILITY,
    T_RITUAL_SUMMONS,
    T_I64,
    T_TICKS_MS,
    T_EFFECT_NAME,
    T_RITUAL_TERRAIN_NAME,
    T_RITUAL_TERRAIN_AT,
    T_RITUAL_EFFECT_MET,
    T_RITUAL_EFFECT_TERRAIN_NAME,
    T_CAST_COUNT,
    T_UNIT_AP_LEFT,
    T_UNIT_ABILITY,
    T_COMMANDER_COST,
    T_SLOT_INCOME,
    T_TERRAIN_INCOME,
    T_I8,
    T_PLANE_AT,
    T_COMMANDER_STRENGTH,
    T_WINDOWS_ONLY,
    T_GAME_CHANGED,
    T_SQUARE_STRENGTH,
    T_REMEMBERED_STRENGTH,
    T_ARMY_STATIONARY,
    T_ARMY_HOLDS,
    T_GARRISON_LEFT,
    T_UNIT_STRENGTH,
    T_UNIT_ARMOR,
    T_UNIT_VISIBLE,
    T_MONSTER_ATTACK,
    T_SQUARE_CITADEL,
    T_UNIT_STATIONARY,
    T_CEREMONY_NAME,
    T_CEREMONY_COST,
    T_CEREMONY_LAST,
    T_UNIT_MOVE_FLAGS,
    T_UNIT_FLAG_WORDS,
    T_COMMANDER_MOVE_FLAGS,
    T_COMMANDER_RITPOW,
    T_COMMANDER_RITUAL_LEVEL,
    T_COMMANDER_SPELL_LEVEL,
    T_SQUARE_LIBRARY_LEVEL,
    T_MONSTER_ABILITY,
    T_FORT_PART_WORD,
    T_PLANE_TINT,
    T_SQUARE_HAS_FORT,
};
typedef struct {
    const char *name;
    uintptr_t addr;
    int type, count, stride, size, writable;
    CallSite site;
    int count2, stride2;
} GameVar;
// A square value coe5.set writes only while the world is built, inside the area being built.
#define WRITABLE_WHILE_BUILDING 2
// The variables that need more than two indexes, by name.
enum { LIST_NONE, LIST_TO_EMPTY_NAME, LIST_TO_BELOW_ONE, LIST_TO_BELOW_ONE_I32, LIST_TO_BELOW_ZERO_I32 };
typedef struct { const char *name; int count3, stride3, list, nameBack; uintptr_t lastAt; int everyPlayer; } GameVarShape;
// Types that differ between the two builds.
#ifdef __APPLE__
#define LOADED_GAME_TYPE T_U8
#define NETWORK_GAME_TYPE T_FLAG_U8
#define INT_FLAG_TYPE T_FLAG_U8
#define WILDER_DEFAULT_TYPE T_FLAG_U8
#else
#define LOADED_GAME_TYPE T_I32
#define NETWORK_GAME_TYPE T_FLAG_POSITIVE
#define INT_FLAG_TYPE T_FLAG_I32
#define WILDER_DEFAULT_TYPE T_FLAG_TWO
#endif
// @platform-split ai_test_values: bugChecksOff and commanderStrength read nil on Mac (RVA_NO_CHECKS and RVA_COMMANDER_STRENGTH not looked up)
// Values only the Windows build has an address for so far.
#ifdef __APPLE__
#define WINDOWS_ONLY_RVA(rva) 0
#define BUG_CHECKS_OFF_TYPE T_WINDOWS_ONLY
#define COMMANDER_STRENGTH_TYPE T_WINDOWS_ONLY
#else
#define WINDOWS_ONLY_RVA(rva) (rva)
#define BUG_CHECKS_OFF_TYPE T_FLAG_I32
#define COMMANDER_STRENGTH_TYPE T_COMMANDER_STRENGTH
#endif
// @platform-split ai_square_strength: squareStrength and squareRememberedStrength read nil on Mac (RVA_SQUARE_STRENGTH not looked up, SQUARE_AI_STRENGTH not checked there)
// A square's strength, now and as an AI player remembers it: Windows only.
#ifdef __APPLE__
#define SQUARE_STRENGTH_TYPE T_WINDOWS_ONLY
#define REMEMBERED_STRENGTH_TYPE T_WINDOWS_ONLY
#else
#define SQUARE_STRENGTH_TYPE T_SQUARE_STRENGTH
#define REMEMBERED_STRENGTH_TYPE T_REMEMBERED_STRENGTH
#endif
// @platform-split army_stationary: squareArmyStationary reads nil on Mac (RVA_SQUARE_ARMY_STATIONARY not looked up)
// Whether the game ever moves the army on a square: Windows only.
#ifdef __APPLE__
#define ARMY_STATIONARY_TYPE T_WINDOWS_ONLY
#else
#define ARMY_STATIONARY_TYPE T_ARMY_STATIONARY
#endif
// @platform-split army_holds: squareArmyHolds reads nil on Mac (RVA_SQUARE_TOP_UNIT not looked up)
// Whether the independents' move holds the army on a square: Windows only.
#ifdef __APPLE__
#define ARMY_HOLDS_TYPE T_WINDOWS_ONLY
#else
#define ARMY_HOLDS_TYPE T_ARMY_HOLDS
#endif
// @platform-split unit_visible: unitVisibleTo reads nil on Mac (RVA_UNIT_VISIBLE_TO not looked up)
// Whether a player sees a unit: Windows only.
#ifdef __APPLE__
#define UNIT_VISIBLE_TYPE T_WINDOWS_ONLY
#else
#define UNIT_VISIBLE_TYPE T_UNIT_VISIBLE
#endif
// @platform-split garrison_left: commanderGarrisonLeft reads nil on Mac (RVA_AI_GATHER_TROOPS and the functions it calls not looked up)
// What stays on a commander's square when it leaves: Windows only.
#ifdef __APPLE__
#define GARRISON_LEFT_TYPE T_WINDOWS_ONLY
#else
#define GARRISON_LEFT_TYPE T_GARRISON_LEFT
#endif
// @platform-split ai_unit_fight: unitStrength, unitArmor, monsterAttackType, monsterAttackWeapon, monsterAttackValue, weaponDmgType and weaponAoe read nil on Mac (RVA_UNIT_STRENGTH_FOR and RVA_UNIT_ARMOR not looked up, MONSTER_ATTACKS and the WEAPON_ offsets not checked there)
// What a fight estimate reads of units, monsters and weapons: Windows only.
#ifdef __APPLE__
#define UNIT_STRENGTH_TYPE T_WINDOWS_ONLY
#define UNIT_ARMOR_TYPE T_WINDOWS_ONLY
#define MONSTER_ATTACK_TYPE T_WINDOWS_ONLY
#define WEAPON_NUMBER_TYPE T_WINDOWS_ONLY
#else
#define UNIT_STRENGTH_TYPE T_UNIT_STRENGTH
#define UNIT_ARMOR_TYPE T_UNIT_ARMOR
#define MONSTER_ATTACK_TYPE T_MONSTER_ATTACK
#define WEAPON_NUMBER_TYPE T_I32
#endif
// @platform-split square_citadel: squareCitadel reads nil on Mac (RVA_SQUARE_IS_CITADEL_FOR not looked up)
// Whether a square is a citadel for a player: Windows only.
#ifdef __APPLE__
#define SQUARE_CITADEL_TYPE T_WINDOWS_ONLY
#else
#define SQUARE_CITADEL_TYPE T_SQUARE_CITADEL
#endif
// @platform-split ceremony_values: ceremonyName, ceremonyCost and ceremonyLastIndex read nil on Mac (the ceremony code and table not looked up)
// The Voice of El's ceremonies: their names, the cost table, and the last number.
#ifdef __APPLE__
#define CEREMONY_NAME_TYPE T_WINDOWS_ONLY
#define CEREMONY_COST_TYPE T_WINDOWS_ONLY
#define CEREMONY_LAST_TYPE T_WINDOWS_ONLY
#else
#define CEREMONY_NAME_TYPE T_CEREMONY_NAME
#define CEREMONY_COST_TYPE T_CEREMONY_COST
#define CEREMONY_LAST_TYPE T_CEREMONY_LAST
#endif
#define CEREMONY_INDEXES (CEREMONY_LAST + 1)
#define FORT_PART_BITS 32
// @platform-split slot_income: slotIncome reads nil on Mac (the income functions player_turn_start calls not looked up)
// A player's income at its next turn start: Windows only.
#ifdef __APPLE__
#define SLOT_INCOME_TYPE T_WINDOWS_ONLY
#else
#define SLOT_INCOME_TYPE T_SLOT_INCOME
#endif
// @platform-split commander_done: commanderDone reads nil on Mac (RVA_COMMANDER_DONE not looked up)
// Whether the player gave a commander "Commander is done for this turn": Windows only.
#ifdef __APPLE__
#define COMMANDER_DONE_TYPE T_WINDOWS_ONLY
#else
#define COMMANDER_DONE_TYPE T_FLAG_U8
#endif
// @platform-split ai_move_reads: monsterMoveFlags, monsterHp, slotTradeType, slotTradeMode, commanderMoveFlags, commanderRitpow, commanderRitualLevel, terrainFlags, squareLibraryLevel, squareHasFort, seasonTurnsLeft, aiAttackDivOther, aiAttackDivAi, aiAttackDivHuman and aiAttackMargin read nil on Mac (RVA_ARMY_MOVE_FLAGS, RVA_COMMANDER_RITPOW, RVA_SQUARE_LIBRARY_LEVEL, RVA_SEASON_TURNS_LEFT and the four RVA_AI_ATTACK_ globals not looked up, MONSTER_MOVE_FLAGS and TERRAIN_FLAGS not checked there)
// What the AI's move and target choices read: Windows only.
#ifdef __APPLE__
#define AI_MOVE_I64_TYPE T_WINDOWS_ONLY
#define AI_MOVE_I32_TYPE T_WINDOWS_ONLY
#define AI_MOVE_I16_TYPE T_WINDOWS_ONLY
#define COMMANDER_MOVE_FLAGS_TYPE T_WINDOWS_ONLY
#define COMMANDER_RITPOW_TYPE T_WINDOWS_ONLY
#define COMMANDER_RITUAL_LEVEL_TYPE T_WINDOWS_ONLY
#define SQUARE_LIBRARY_LEVEL_TYPE T_WINDOWS_ONLY
#define SQUARE_HAS_FORT_TYPE T_WINDOWS_ONLY
#else
#define AI_MOVE_I64_TYPE T_I64
#define AI_MOVE_I32_TYPE T_I32
#define AI_MOVE_I16_TYPE T_I16
#define COMMANDER_MOVE_FLAGS_TYPE T_COMMANDER_MOVE_FLAGS
#define COMMANDER_RITPOW_TYPE T_COMMANDER_RITPOW
#define COMMANDER_RITUAL_LEVEL_TYPE T_COMMANDER_RITUAL_LEVEL
#define SQUARE_LIBRARY_LEVEL_TYPE T_SQUARE_LIBRARY_LEVEL
#define SQUARE_HAS_FORT_TYPE T_SQUARE_HAS_FORT
#endif
#define RESOURCE_COUNT 14
#define NAME_SIZE 100
#define COUNT_AT_CALL 0
#define SQUARE_COLUMN_SIZE (MAP_COLUMN_SQUARES * SQUARE_SIZE)
// Each row: name, address, type, count, stride, size, writable, site, count2, stride2.
static const GameVar GAME_VARS[] = {
    {"turn",           RVA_TURN,                          T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"networkGame",    RVA_NET_CLIENT,                    NETWORK_GAME_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"loadedGame",     RVA_LOADED_GAME,                   LOADED_GAME_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"loadedMapName",  RVA_LOADED_MAP_NAME,               T_STR, 1, 0, 256, 0, 0, 0, 0},
    {"society",        RVA_SOCIETY,                       T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"commonCause",    RVA_COMMON_CAUSE,                  T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"scoreGraphs",    RVA_SCORE_GRAPHS,                  T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"battleReports",  RVA_BATTLE_REPORTS,                T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"startFlags",     RVA_START_FLAGS,                   T_I16, 1, 0, 0, 1, 0, 0, 0},
    {"wilder",         RVA_WILDER,                        T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapW",           RVA_MAP_W,                         T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapH",           RVA_MAP_H,                         T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"mapDepth",       RVA_MAP_DEPTH,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"northPart",      RVA_NORTH_PART,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"southPart",      RVA_SOUTH_PART,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"slotController", RVA_SLOT_CONTROLLER,               T_I16, 32, 2, 0, 1, 0, 0, 0},
    {"slotClass",      RVA_SLOT_CLASS,                    T_I16, 32, 2, 0, 1, 0, 0, 0},
    {"slotAi",         RVA_SLOT_AI,                       T_I16, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotTeam",       RVA_SLOT_TEAM,                     T_I16, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotName",       RVA_SLOT_NAME,                     T_STR, 32, PLAYER_BLOCK_SIZE, 32, 1, 0, 0, 0},
    {"slotColorRed",   RVA_SLOT_COLOUR,                   T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotColorGreen", RVA_SLOT_COLOUR + 4,               T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotColorBlue",  RVA_SLOT_COLOUR + 8,               T_F32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotResource",   RVA_PLAYER_INTS,                   T_I32, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 14, 4},
    {"slotIncome",     0,                                 SLOT_INCOME_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, RESOURCE_COUNT, 0},
    {"slotTradeType",  WINDOWS_ONLY_RVA(RVA_PLAYER_TRADE_TYPE), AI_MOVE_I16_TYPE, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotTradeMode",  WINDOWS_ONLY_RVA(RVA_PLAYER_TRADE_MODE), AI_MOVE_I16_TYPE, 32, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"newWorldUniqueRandom", FRAME_NEW_WORLD_UNIQUE_RANDOM, T_FRAME_I32, 1, 0, 0, 1, SITE_NEW_WORLD_CREATING_WORLD, 0, 0},
    {"cliMapW",        RVA_CLI_MAP_W,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"cliMapH",        RVA_CLI_MAP_H,                     T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"cliSociety",     RVA_CLI_SOCIETY,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"subSociety",     RVA_SUBSOC,                        INT_FLAG_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"startDefault",   RVA_START_DEFAULT,                 T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"wilderDefault",  RVA_WILDER_DEFAULT,                WILDER_DEFAULT_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"setupNetwork",   RVA_SETUP_NETWORK,                 T_FLAG_I16, 1, 0, 0, 0, 0, 0, 0},
    {"aiLevelName",    RVA_AI_LEVEL_NAMES,                T_PTR_STR, AI_LEVEL_TABLE_MAX, 8, NAME_SIZE, 0, 0, 0, 0},
    {"screenW",        RVA_SCREEN_W,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"screenH",        RVA_SCREEN_H,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"hoveredId",      RVA_HOVERED_ID,                    T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"bannerFade",     RVA_BANNER_FADE,                   T_F32, 1, 0, 0, 1, 0, 0, 0},
    {"activePlayer",   RVA_ACTIVE_PLAYER,                 T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"season",         RVA_SEASON,                        T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"seasonName",     RVA_SEASON_NAMES,                  T_PTR_STR, 4, 8, NAME_SIZE, 0, 0, 0, 0},
    {"date",           RVA_DATE_TEXT,                     T_STR, 1, 0, 100, 0, 0, 0, 0},
    {"worldW",         RVA_WORLD_W,                       T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"worldH",         RVA_WORLD_H,                       T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"uniqueRandomDefault", RVA_UNIQUE_RANDOM_DEFAULT,    INT_FLAG_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"slotLockClass",      RVA_SLOT_LOCK_CLASS,           T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockController", RVA_SLOT_LOCK_CONTROLLER,      T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockAi",         RVA_SLOT_LOCK_AI,              T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotLockTeam",       RVA_SLOT_LOCK_TEAM,            T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"eventVar",       RVA_EVENT_VARS,                    T_I32, EVENT_VAR_COUNT, 4, 0, 1, 0, 0, 0},
    {"modLastIndex",   RVA_MOD_LAST_INDEX,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"modName",        RVA_MOD_NAMES,                     T_STR, MOD_TABLE_MAX, MOD_RECORD_SIZE, 256, 0, 0, 0, 0},
    {"modFolder",      RVA_MOD_FOLDERS,                   T_STR, MOD_TABLE_MAX, MOD_RECORD_SIZE, 256, 0, 0, 0, 0},
    {"modEnabled",     RVA_MOD_NAMES + MOD_RECORD_ENABLED, T_I16, MOD_TABLE_MAX, MOD_RECORD_SIZE, 0, 0, 0, 0, 0},
    {"monsterName",    RVA_MONSTER_TABLE,                 T_STR, MONSTER_TABLE_MAX, MONSTER_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"monsterLastIndex", RVA_MONSTER_COUNT,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"monsterSprite",  RVA_MONSTER_SPRITES,               T_I32, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"itemName",       RVA_ITEM_TABLE,                    T_STR, ITEM_TABLE_MAX, ITEM_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"itemLastIndex",  RVA_ITEM_COUNT,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"className",      RVA_CLASS_TABLE,                   T_STR, CLASS_TABLE_MAX, CLASS_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"classLeader",    RVA_CLASS_LEADERS,                 T_STR, CLASS_TABLE_MAX, CLASS_SIZE, NAME_SIZE, 0, 0, 0, 0},
    {"classLastIndex", RVA_CLASS_COUNT,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"terrainName",    RVA_TERRAIN_TABLE,                 T_STR, TERRAIN_TABLE_MAX, TERRAIN_SIZE, TERRAIN_NAME_SIZE, 0, 0, 0, 0},
    {"terrainLastIndex", RVA_LAST_TERRAIN,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"terrainIncome",  RVA_TERRAIN_TABLE,                 T_TERRAIN_INCOME, TERRAIN_TABLE_MAX, TERRAIN_SIZE, 0, 0, 0, RESOURCE_COUNT, 0},
    {"ritualName",     RVA_RITUAL_TABLE,                  T_STR, COUNT_AT_CALL, RITUAL_SIZE, 48, 0, 0, 0, 0},
    {"ritualLastIndex", RVA_RITUAL_COUNT,                 T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"resourceName",   RVA_RESOURCE_NAMES,                T_PTR_STR, 14, 8, NAME_SIZE, 0, 0, 0, 0},
    {"incomeName",     RVA_INCOME_NAMES,                  T_PTR_STR, INCOME_NAME_COUNT, 8, NAME_SIZE, 0, 0, 0, 0},
    {"classRecruitName", RVA_CLASS_SIDE_A + RECRUIT_MONSTER_NAME, T_STR, CLASS_TABLE_MAX, CLASS_RECRUITS_SIZE, MONSTER_NAME_SIZE, 0, 0,
     CLASS_RECRUITS_MAX, CLASS_RECRUIT_SIZE},
    {"classRecruitCount", RVA_CLASS_SIDE_A + RECRUIT_COUNT, T_I32, CLASS_TABLE_MAX, CLASS_RECRUITS_SIZE, 0, 0, 0,
     CLASS_RECRUITS_MAX, CLASS_RECRUIT_SIZE},
    {"classStartArmy", RVA_CLASS_TABLE + CLASS_START_UNITS, T_STR, CLASS_TABLE_MAX, CLASS_SIZE, CLASS_START_UNIT_SIZE, 0, 0,
     CLASS_START_UNITS_MAX, CLASS_START_UNIT_SIZE},
    {"classStandardTroops", RVA_CLASS_TABLE, T_CLASS_STDTROOPS, CLASS_TABLE_MAX, CLASS_SIZE, 0, 0, 0, 0, 0},
    {"classAbility",   RVA_CLASS_TABLE,                   T_CLASS_ABILITY, CLASS_TABLE_MAX, CLASS_SIZE, 0, 0, 0, INT_MAX, 0},
    {"mapPointerX",    RVA_SQUARE_INFO_X,                 T_SITE_I32, 1, 0, 0, 0, SITE_MAP_SCREEN_WAIT, 0, 0},
    {"mapPointerY",    RVA_SQUARE_INFO_Y,                 T_SITE_I32, 1, 0, 0, 0, SITE_MAP_SCREEN_WAIT, 0, 0},
    {"mouseButtons",   RVA_MOUSE_BUTTONS,                 T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"squareSeen",     RVA_SQUARE_RECORDS + SQUARE_SEEN,  T_U8, PLAYER_SLOTS, 1, 0, 0, 0, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE},
    {"squareArmy",     RVA_SQUARE_ARMY,                   T_I16, MAP_COLUMNS_MAX, MAP_COLUMN_SQUARES * 2, 0, 0, 0, MAP_COLUMN_SQUARES, 2},
    {"squareArmyOwner", RVA_SQUARE_RECORDS + SQUARE_ARMY_OWNER, T_ARMY_OWNER, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0,
     MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareOwner",    RVA_SQUARE_RECORDS + SQUARE_OWNER, T_I8, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareTerrain",  RVA_SQUARE_RECORDS + SQUARE_TERRAIN_TYPE, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0,
     WRITABLE_WHILE_BUILDING, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squarePortal",   RVA_SQUARE_RECORDS + SQUARE_PORTAL, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, WRITABLE_WHILE_BUILDING, 0,
     MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareGround",   RVA_SQUARE_RECORDS + SQUARE_GROUND, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, WRITABLE_WHILE_BUILDING, 0,
     MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareTemperature", RVA_SQUARE_RECORDS + SQUARE_TEMPERATURE, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0,
     WRITABLE_WHILE_BUILDING, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareName",     RVA_SQUARE_RECORDS + SQUARE_NAME,  T_STR, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, SQUARE_NAME_SIZE,
     WRITABLE_WHILE_BUILDING, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squarePlane",    0,                                 T_PLANE_AT, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"commanderLastIndex", RVA_LAST_COMMANDER,            T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"commanderUnit",  RVA_COMMANDERS,                    T_I32, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitual", RVA_COMMANDER_KNOWN_RITUALS,      T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     COMMANDER_KNOWN_RITUALS_MAX, 2},
    {"commanderCostAvailable", RVA_COMMANDERS,            T_COMMANDER_COST, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     INCOME_NAME_COUNT, 0},
    {"commanderAiTargetX", RVA_COMMANDER_AI_TARGET_X,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiTargetY", RVA_COMMANDER_AI_TARGET_Y,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiMoveKind", RVA_COMMANDER_AI_MOVE_KIND,   T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderRitualPlan", RVA_COMMANDER_AI_PLAN,        T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderDone",  WINDOWS_ONLY_RVA(RVA_COMMANDER_DONE), COMMANDER_DONE_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotLeader",     RVA_PLAYER_LEADER,                 T_I32, 32, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"unitLastIndex",  RVA_UNIT_COUNT,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"unitOwner",      RVA_UNIT_PLAYER,                   T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitHp",         RVA_UNIT_HP,                       T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitMonster",    RVA_UNIT_MONSTER,                  T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitCommander",  RVA_UNITS + UNIT_COMMANDER,        T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitStationary", RVA_UNITS,                         T_UNIT_STATIONARY, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"ritualSummons",  RVA_RITUAL_TABLE + RITUAL_FX,      T_RITUAL_SUMMONS, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualString",   RVA_RITUAL_TABLE + RITUAL_STRINGS, T_STR, COUNT_AT_CALL, RITUAL_SIZE, RITUAL_STRING_SIZE, 0, 0,
     RITUAL_STRINGS_MAX, RITUAL_STRING_SIZE},
    {"ritualRitpow",   RVA_RITUAL_TABLE + RITUAL_RITPOW,  T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualLevel",    RVA_RITUAL_TABLE + RITUAL_LEVEL,   T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualCostType", RVA_RITUAL_TABLE + RITUAL_COSTS,   T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_COSTS_MAX, RITUAL_COST_SIZE},
    {"ritualCost",     RVA_RITUAL_TABLE + RITUAL_COSTS + 4, T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_COSTS_MAX,
     RITUAL_COST_SIZE},
    {"ritualTerrain",  RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualEffect",   RVA_RITUAL_TABLE + RITUAL_FX,      T_I32, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"ritualEffectValue", RVA_RITUAL_TABLE + RITUAL_FX + RITUAL_FX_VALUE, T_I64, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"ritualFlags",    RVA_RITUAL_TABLE + RITUAL_FLAGS,   T_I64, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualTerrainName", RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_RITUAL_TERRAIN_NAME, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, 0, 0},
    {"ritualTerrainAt", RVA_RITUAL_TABLE + RITUAL_TERRAIN, T_RITUAL_TERRAIN_AT, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     MAP_COLUMNS_MAX, 0},
    {"ritualEffectMet", RVA_RITUAL_TABLE + RITUAL_FX,     T_RITUAL_EFFECT_MET, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0, RITUAL_FX_MAX,
     RITUAL_FX_SIZE},
    {"ritualEffectTerrainName", RVA_RITUAL_TABLE + RITUAL_FX, T_RITUAL_EFFECT_TERRAIN_NAME, COUNT_AT_CALL, RITUAL_SIZE, 0, 0, 0,
     RITUAL_FX_MAX, RITUAL_FX_SIZE},
    {"effectName",     0,                                 T_EFFECT_NAME, RITUAL_EFFECT_ID_COUNT, 0, 0, 0, 0, 0, 0},
    {"slotRitualCasts", RVA_CAST_COUNTS,                  T_CAST_COUNT, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, COUNT_AT_CALL, 2},
    {"unitX",          RVA_UNIT_X,                        T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitY",          RVA_UNIT_X + 4,                    T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitApLeft",     RVA_UNITS,                         T_UNIT_AP_LEFT, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitApUsed",     RVA_UNITS + UNIT_AP_USED,          T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 1, 0, 0, 0},
    {"unitAbility",    RVA_UNITS,                         T_UNIT_ABILITY, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, RITUAL_EFFECT_ID_COUNT, 0},
    {"planeName",      RVA_PLANES,                        T_STR, PLANE_RECORDS_MOVED, PLANE_SIZE, PLANE_NAME_SIZE, 0, 0, 0, 0},
    {"planeLastIndex", RVA_LAST_PLANE,                    T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"planeTint",      RVA_PLANES,                        T_PLANE_TINT, PLANE_RECORDS_MOVED, PLANE_SIZE, 0, 0, 0, 0, 0},
    {"ticksMs",        RVA_TICKS_MS,                      T_TICKS_MS, 1, 0, 0, 0, 0, 0, 0},
    {"tooltipLineHeight", RVA_TOOLTIP_LINE_HEIGHT,        T_F32, 1, 0, 0, 0, 0, 0, 0},
    {"traceLevel",     RVA_DEBUG_LEVEL,                   T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"bugChecksOff",   WINDOWS_ONLY_RVA(RVA_NO_CHECKS),   BUG_CHECKS_OFF_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"commanderName",  RVA_COMMANDER_NAMES,               T_STR, COMMANDER_TABLE_MAX, COMMANDER_SIZE, COMMANDER_NAME_SIZE, 0, 0, 0, 0},
    {"commanderStrength", WINDOWS_ONLY_RVA(RVA_COMMANDERS), COMMANDER_STRENGTH_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderGarrisonLeft", WINDOWS_ONLY_RVA(RVA_COMMANDERS), GARRISON_LEFT_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotHomeX",      RVA_PLAYER_HOME_X,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotHomeY",      RVA_PLAYER_HOME_Y,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"squareGems",     RVA_SQUARE_RECORDS + SQUARE_GEMS,  T_U8, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareStrength", 0,                                 SQUARE_STRENGTH_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"squareRememberedStrength", WINDOWS_ONLY_RVA(RVA_SQUARE_RECORDS + SQUARE_AI_STRENGTH), REMEMBERED_STRENGTH_TYPE, PLAYER_SLOTS, 4, 0, 0, 0,
     MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE},
    {"squareArmyStationary", 0,                           ARMY_STATIONARY_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"squareArmyHolds", 0,                                ARMY_HOLDS_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"unitStrength",   0,                                 UNIT_STRENGTH_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, UNIT_TABLE_MAX, 0},
    {"unitArmor",      WINDOWS_ONLY_RVA(RVA_UNITS),       UNIT_ARMOR_TYPE, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitVisibleTo",  0,                                 UNIT_VISIBLE_TYPE, UNIT_TABLE_MAX, 0, 0, 0, 0, PLAYER_SLOTS, 0},
    {"monsterAttackType", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS), MONSTER_ATTACK_TYPE, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackWeapon", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_WEAPON), MONSTER_ATTACK_TYPE,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackValue", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_VALUE), MONSTER_ATTACK_TYPE,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"weaponDmgType",  WINDOWS_ONLY_RVA(RVA_WEAPON_TABLE + WEAPON_DMGTYPE), WEAPON_NUMBER_TYPE, WEAPON_RECORDS_GAME, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"weaponAoe",      WINDOWS_ONLY_RVA(RVA_WEAPON_TABLE + WEAPON_AOE), WEAPON_NUMBER_TYPE, WEAPON_RECORDS_GAME, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"squareCitadel",  0,                                 SQUARE_CITADEL_TYPE, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"unitMoveFlags",  RVA_UNITS,                         T_UNIT_MOVE_FLAGS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitFlags",      RVA_UNITS,                         T_UNIT_FLAG_WORDS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"monsterFlags",   RVA_MONSTER_FLAGS,                 T_I64, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"monsterAbility", RVA_MONSTER_TABLE,                 T_MONSTER_ABILITY, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0,
     RITUAL_EFFECT_ID_COUNT, 0},
    {"monsterMoveFlags", WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_MOVE_FLAGS), AI_MOVE_I64_TYPE, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, 0, 0},
    {"monsterHp",      WINDOWS_ONLY_RVA(RVA_MONSTER_TABLE + MONSTER_HP), AI_MOVE_I16_TYPE, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"commanderMoveFlags", RVA_COMMANDERS,                COMMANDER_MOVE_FLAGS_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitpow", RVA_COMMANDERS,                   COMMANDER_RITPOW_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitualLevel", RVA_COMMANDERS,              COMMANDER_RITUAL_LEVEL_TYPE, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderSpellLevel", RVA_COMMANDERS,               T_COMMANDER_SPELL_LEVEL, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"terrainFlags",   WINDOWS_ONLY_RVA(RVA_TERRAIN_TABLE + TERRAIN_FLAGS), AI_MOVE_I64_TYPE, TERRAIN_TABLE_MAX, TERRAIN_SIZE, 0, 0, 0, 0, 0},
    {"squareLibraryLevel", 0,                             SQUARE_LIBRARY_LEVEL_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"squareHasFort",  0,                                 SQUARE_HAS_FORT_TYPE, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"seasonTurnsLeft", WINDOWS_ONLY_RVA(RVA_SEASON_TURNS_LEFT), AI_MOVE_I16_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivOther", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_OTHER), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivAi",  WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_AI), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivHuman", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_DIV_HUMAN), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackMargin", WINDOWS_ONLY_RVA(RVA_AI_ATTACK_MARGIN), AI_MOVE_I32_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"ceremonyName",   0,                                 CEREMONY_NAME_TYPE, CEREMONY_INDEXES, 0, 0, 0, 0, 0, 0},
    {"ceremonyCost",   0,                                 CEREMONY_COST_TYPE, CEREMONY_INDEXES, 0, 0, 1, SITE_CEREMONY_COSTS, 0, 0},
    {"ceremonyLastIndex", 0,                              CEREMONY_LAST_TYPE, 1, 0, 0, 0, 0, 0, 0},
    // The terrain word for fort part bit n (walls, irongate, ...), as the game's terrain reader has it.
    {"fortPartWord",   0,                                 T_FORT_PART_WORD, FORT_PART_BITS, 0, 0, 0, 0, 0, 0},
};
static const GameVarShape GAME_VAR_SHAPES[] = {
    {"classRecruitName", 0, 0, LIST_TO_EMPTY_NAME, RECRUIT_MONSTER_NAME, 0, 0},
    {"classRecruitCount", 0, 0, LIST_TO_EMPTY_NAME, RECRUIT_COUNT, 0, 0},
    {"classStartArmy", 0, 0, LIST_TO_EMPTY_NAME, 0, 0, 0},
    {"squareSeen", MAP_COLUMN_SQUARES, SQUARE_SIZE, LIST_NONE, 0, 0, 0},
    {"commanderRitual", 0, 0, LIST_TO_BELOW_ONE, 0, 0, 0},
    {"ritualString", 0, 0, LIST_TO_EMPTY_NAME, 0, 0, 0},
    {"ritualCostType", 0, 0, LIST_TO_BELOW_ZERO_I32, 0, 0, 0},
    {"ritualCost", 0, 0, LIST_TO_BELOW_ZERO_I32, 4, 0, 0},
    {"ritualEffect", 0, 0, LIST_TO_BELOW_ONE_I32, 0, 0, 0},
    {"ritualEffectValue", 0, 0, LIST_TO_BELOW_ONE_I32, RITUAL_FX_VALUE, 0, 0},
    {"ritualTerrainAt", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 0},
    {"ritualEffectMet", COMMANDER_TABLE_MAX, 0, LIST_NONE, 0, 0, 0},
    {"planeName", 0, 0, LIST_NONE, 0, RVA_LAST_PLANE, 0},
    {"planeTint", 0, 0, LIST_NONE, 0, RVA_LAST_PLANE, 0},
    {"squareGems", SQUARE_GEM_KINDS, 1, LIST_NONE, 0, 0, 0},
    {"squareStrength", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
    {"squareRememberedStrength", MAP_COLUMN_SQUARES, SQUARE_SIZE, LIST_NONE, 0, 0, 0},
    {"unitStrength", 0, 0, LIST_NONE, 0, 0, 1},
    {"squareCitadel", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
};
// Variable v's GameVarShape, or one with no third index and no list.
static GameVarShape gameVarShape(const GameVar *v) {
    for (size_t i = 0; i < sizeof GAME_VAR_SHAPES / sizeof GAME_VAR_SHAPES[0]; i++)
        if (!strcmp(GAME_VAR_SHAPES[i].name, v->name)) return GAME_VAR_SHAPES[i];
    return (GameVarShape){v->name, 0, 0, LIST_NONE, 0, 0, 0};
}
static int luaSiteDepth[SITE_COUNT];

// Returns a site of the same game function as `site` whose Lua wrap runs now, or SITE_COUNT.
static CallSite runningFrameSite(CallSite site) {
    for (size_t i = 0; i < sizeof FRAME_SITES / sizeof FRAME_SITES[0]; i++) {
        int inGroup = 0;
        for (size_t k = 0; k < FRAME_SITES[i].count; k++) inGroup |= FRAME_SITES[i].sites[k] == site;
        if (!inGroup) continue;
        for (size_t k = 0; k < FRAME_SITES[i].count; k++)
            if (luaSiteDepth[FRAME_SITES[i].sites[k]]) return FRAME_SITES[i].sites[k];
    }
    return SITE_COUNT;
}

// The GAME_VARS row named `name` (case-sensitive), or NULL.
#define GAME_VAR_SLOTS 512
_Static_assert(sizeof GAME_VARS / sizeof GAME_VARS[0] * 2 <= GAME_VAR_SLOTS, "GAME_VAR_SLOTS is too small");
static short gameVarSlots[GAME_VAR_SLOTS];

static const GameVar *findGameVar(const char *name) {
    static int built;
    if (!built) {
        for (size_t i = 0; i < sizeof GAME_VARS / sizeof GAME_VARS[0]; i++) {
            unsigned int h = gameWordHash(GAME_VARS[i].name) & (GAME_VAR_SLOTS - 1);
            while (gameVarSlots[h] && strcmp(GAME_VARS[gameVarSlots[h] - 1].name, GAME_VARS[i].name)) h = (h + 1) & (GAME_VAR_SLOTS - 1);
            gameVarSlots[h] = (short)(i + 1);
        }
        built = 1;
    }
    for (unsigned int h = gameWordHash(name) & (GAME_VAR_SLOTS - 1); gameVarSlots[h]; h = (h + 1) & (GAME_VAR_SLOTS - 1))
        if (!strcmp(GAME_VARS[gameVarSlots[h] - 1].name, name)) return &GAME_VARS[gameVarSlots[h] - 1];
    return NULL;
}

// Finds the variable named by argument 1 of coe5.get or coe5.set, and returns its address.
static unsigned char *gameVarAddress(lua_State *L, const GameVar **out, int *arg) {
    const char *name = luaL_checkstring(L, 1);
    const GameVar *v = findGameVar(name);
    if (!v) { luaL_error(L, "unknown game variable '%s'", name); return NULL; }
    int isRitualTable = v->count == COUNT_AT_CALL && v->addr >= RVA_RITUAL_TABLE && v->addr < RVA_RITUAL_TABLE + RITUAL_SIZE;
    int movedTable = isRitualTable || !v->addr ? -1 : tableHolding(v->addr, v->stride);
    lua_Integer index = 0;
    int count = isRitualTable ? ritualSlotCount() : movedTable >= 0 ? tableRecords(movedTable) : v->count;
    GameVarShape shape = gameVarShape(v);
    if (count > 1) {
        int first = shape.everyPlayer ? -1 : 0;
        index = luaL_checkinteger(L, (*arg)++);
        if (index < first || index >= count) luaL_error(L, "%s: index %I out of range %d-%d", name, (LUAI_UACINT)index, first, count - 1);
    }
    lua_Integer index2 = 0, index3 = 0;
    int count2 = v->type == T_CAST_COUNT ? ritualSlotCount() : v->count2;
    if (count2 > 1) {
        index2 = luaL_checkinteger(L, (*arg)++);
        if (index2 < 0 || index2 >= count2) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index2, count2 - 1);
    }
    if (shape.count3 > 1) {
        index3 = luaL_checkinteger(L, (*arg)++);
        if (index3 < 0 || index3 >= shape.count3) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index3, shape.count3 - 1);
    }
    *out = v;
    if (v->type == T_FRAME_I32) {
        CallSite running = runningFrameSite(v->site);
        uintptr_t sp = running != SITE_COUNT ? luaApi->callerRsp(running) : 0;
        return sp ? (unsigned char *)(sp + v->addr) : NULL;
    }
    if ((v->type == T_SITE_I32 || v->type == T_CEREMONY_COST) && !luaSiteDepth[v->site]) return NULL;
    unsigned char *table = v->type == T_CAST_COUNT ? castCountsBase()
                         : v->addr == RVA_PLANES ? planeTableAddress()
                         : movedTable >= 0 ? tableAddress(movedTable) + (v->addr - tableMoves[movedTable].rva)
                         : gameBase + (isRitualTable ? ritualTableRva() + (v->addr - RVA_RITUAL_TABLE) : v->addr);
    if (shape.lastAt && index > G32(shape.lastAt)) return NULL;
    unsigned char *entries = table + (size_t)index * v->stride;
    for (lua_Integer k = 0; shape.list != LIST_NONE && k <= index2; k++) {
        const unsigned char *entry = entries - shape.nameBack + (size_t)k * v->stride2;
        int ended = shape.list == LIST_TO_BELOW_ONE       ? *(const short *)entry < 1
                    : shape.list == LIST_TO_BELOW_ONE_I32  ? *(const int *)entry < 1
                    : shape.list == LIST_TO_BELOW_ZERO_I32 ? *(const int *)entry < 0
                                                           : !*entry;
        if (ended) return NULL;
    }
    return entries + (size_t)index2 * v->stride2 + (size_t)index3 * shape.stride3;
}

// The player whose army stands on square (x, y), or -1.
static int armyOwnerAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return -1;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    if (!G16(RVA_SQUARE_ARMY + square * 2)) return -1;
    return G16(RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_ARMY_OWNER);
}

// A unit's action points left this turn.
static int unitApLeft(int unit) {
    int max = 0;
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) >= 0) {
        unsigned long long flags = GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(unit);
        max = flags & UNIT_FLAG_FAST ? 4 : flags & UNIT_FLAG_SLOW ? 2 : 3;
    }
    return max - G16(RVA_UNITS + (size_t)unit * UNIT_SIZE + UNIT_AP_USED);
}

// 1 when the unit never moves on the world map.
static int unitStationary(int unit) {
    if (G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) < 0) return 0;
    return (GAME_FN(unsigned long long (*)(int), RVA_UNIT_FLAGS)(unit) >> MONSTER_FLAG_NO_COMMANDER) & 1;
}

// The corpses on square (x, y).
static int corpsesAt(int x, int y, int humanoidOnly) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H) || x >= MAP_COLUMNS_MAX || y >= MAP_COLUMN_SQUARES) return 0;
    size_t square = (size_t)x * MAP_COLUMN_SQUARES + (size_t)y;
    int count = 0;
    for (int kind = 0; kind < SQUARE_CORPSE_KINDS; kind++)
        if (!humanoidOnly || kind < 3 || kind > 5) count += G16(RVA_SQUARE_RECORDS + square * SQUARE_SIZE + SQUARE_CORPSES + (size_t)kind * 2);
    return count;
}

// What the owner of commander `commander`'s unit has of cost type `type` at the unit's square.
static int commanderCostAvailable(int commander, int type, int *have) {
    int unit = G32(RVA_COMMANDERS + (size_t)commander * COMMANDER_SIZE);
    if (unit < 0 || unit >= UNIT_TABLE_MAX) return -1;
    int owner = G16(RVA_UNIT_PLAYER + (size_t)unit * UNIT_SIZE);
    if (owner < 0 || owner >= PLAYER_SLOTS) return -1;
    int x = G32(RVA_UNIT_X + (size_t)unit * UNIT_SIZE), y = G32(RVA_UNIT_X + 4 + (size_t)unit * UNIT_SIZE);
    if (type == 14 || type == 17) *have = corpsesAt(x, y, type == 14);
    else *have = GAME_FN(int (*)(int, int, int, int), RVA_RESOURCE_AVAILABLE)(owner, type, x, y);
    return 0;
}

// The .c5m word of ritual effect `id`, or NULL for an id the list lacks.
static const char *ritualEffectWord(int id) {
    for (size_t i = 0; i < sizeof RITUAL_EFFECT_NAMES / sizeof RITUAL_EFFECT_NAMES[0]; i++)
        if (RITUAL_EFFECT_NAMES[i].id == id) return RITUAL_EFFECT_NAMES[i].name;
    return NULL;
}

// The game's name for a terrain code.
static void pushGameTerrainName(lua_State *L, int code) {
    GAME_FN(void (*)(int), RVA_TERRAIN_NAME)(code);
    const char *text = (const char *)(gameBase + RVA_TERRAIN_NAME_TEXT);
    lua_pushlstring(L, text, strnlen(text, TERRAIN_NAME_TEXT_SIZE));
}

// Whether square (x, y) counts for a ritual's terrain `code`.
static int ritualTerrainAt(int code, int x, int y) {
    if (code == -1) return 1;
    if (code >= 0 && code < TERRAIN_TABLE_MAX) {
        int here = G16(RVA_SQUARE_RECORDS + ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE + SQUARE_TERRAIN_TYPE);
        if (here < 0 || here >= TERRAIN_TABLE_MAX) return 0;
        const char *need = (const char *)(gameBase + RVA_TERRAIN_TABLE + (size_t)code * TERRAIN_SIZE);
        const char *have = (const char *)(gameBase + RVA_TERRAIN_TABLE + (size_t)here * TERRAIN_SIZE);
        return strncmp(need, have, TERRAIN_NAME_SIZE) == 0;
    }
    if (code < -1 && code >= TERRAIN_MATCH_CODE_MIN) return GAME_FN(int (*)(int, int, int), RVA_TERRAIN_MATCH)(code, x, y) != 0;
    return -1;
}

// The terrain code of ritual effect `fx` when it is a nearby or nonearby rule.
static int ritualNearbyRule(const unsigned char *fx, int *code) {
    int rule = *(const int *)fx - RITUAL_FX_NONEARBY1REQ;
    if (rule < 0 || rule >= 10) return -1;
    long long value = *(const long long *)(fx + RITUAL_FX_VALUE);
    if (!((value >= TERRAIN_MATCH_CODE_MIN && value < TERRAIN_TABLE_MAX) || (value >= 8001 && value <= 11999))) return -1;
    *code = (int)value;
    return rule;
}

// The game's ability total of a unit.
static int unitAbilityTotal(int unit, int ability, int add) {
    return GAME_FN(int (*)(int, int, int, char *, int, int *), RVA_UNIT_ABILITY_TOTAL)(unit, ability, add, NULL, 0, NULL);
}

// The game's strength of a commander's army.
static int commanderStrength(int commander) {
#ifdef __APPLE__
    (void)commander;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_COMMANDER_FLAGGED_STRENGTH)(commander, 0);
#endif
}

// The strength of the units on square (x, y) of player `team` and its teammates.
static int squareStrength(int team, int x, int y) {
#ifdef __APPLE__
    (void)team, (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int, int), RVA_SQUARE_STRENGTH)(team, x, y);
#endif
}

// 1 when the game never moves the army on square (x, y).
static int armyStationaryAt(int x, int y) {
#ifdef __APPLE__
    (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_SQUARE_ARMY_STATIONARY)(x, y);
#endif
}

// 1 when the independents' move never moves the army on square (x, y).
static int holdFlagsHold(int squareFlags, int top, unsigned long long topFlags) {
    if (squareFlags & SQUARE_FLAG_HOLDIT) return 1;
    return top >= 0 && ((topFlags >> UNIT_FLAG2_HOLDIT) & 1);
}

static int armyHoldsAt(int x, int y) {
#ifdef __APPLE__
    (void)x, (void)y;
    return holdFlagsHold(0, -1, 0);
#else
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    int squareFlags = G32(RVA_SQUARE_FLAGS + square);
    if (holdFlagsHold(squareFlags, -1, 0)) return 1;
    int owner = armyOwnerAt(x, y);
    if (owner < 0) return 0;
    int top = GAME_FN(int (*)(int, int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(owner, -1, x, y, -1, -1, NULL);
    if (top < 0 || top >= UNIT_TABLE_MAX) return 0;
    return holdFlagsHold(squareFlags, top, *(const unsigned long long *)GAME_PTR(RVA_UNITS + (size_t)top * UNIT_SIZE + 8));
#endif
}

// 1 when unit u stands on square (x, y) alive.
static int unitAliveAt(int u, int x, int y) {
    if (u < 0 || u >= UNIT_TABLE_MAX || x < 0) return 0;
    size_t at = (size_t)u * UNIT_SIZE;
    return (short)G16(RVA_UNIT_HP + at) > 0 && G32(RVA_UNIT_X + at) == x && G32(RVA_UNIT_X + 4 + at) == y;
}

// 1 when unit u is in the game's unit table with hp above 0.
static int unitAliveInTable(int u) {
    return u >= 0 && u < UNIT_TABLE_MAX && u <= G32(RVA_UNIT_COUNT) && (short)G16(RVA_UNIT_HP + (size_t)u * UNIT_SIZE) > 0;
}

// A unit's strength as player `player` counts it.
static int unitStrengthFor(int player, int unit) {
    if (!unitAliveInTable(unit)) return 0;
#ifdef __APPLE__
    (void)player;
    return 0;
#else
    return GAME_FN(int (*)(int, int), RVA_UNIT_STRENGTH_FOR)(player, unit);
#endif
}

// A living unit's armor as the unit screen shows it, or -1.
static int unitArmorNow(int unit) {
    if (!unitAliveInTable(unit)) return -1;
#ifdef __APPLE__
    return -1;
#else
    return GAME_FN(int (*)(int), RVA_UNIT_ARMOR)(unit);
#endif
}

// Whether player `player` sees unit `unit`.
static int unitVisibleToNow(int unit, int player) {
    if (!unitAliveInTable(unit)) return 0;
#ifdef __APPLE__
    (void)player;
    return 0;
#else
    size_t at = (size_t)unit * UNIT_SIZE;
    return GAME_FN(int (*)(int, int, int, int), RVA_UNIT_VISIBLE_TO)(player, unit, G32(RVA_UNIT_X + at), G32(RVA_UNIT_X + 4 + at)) != 0;
#endif
}

// The int at byte `field` of row `row` of a monster's attack list.
static int monsterAttackField(const unsigned char *rows, int row, int field, int *value) {
    for (int k = 0; k <= row && k < MONSTER_ATTACK_MAX; k++) {
        const unsigned char *r = rows + (size_t)k * MONSTER_ATTACK_SIZE;
        if (*(const int *)r < 1 || *(const int *)(r + MONSTER_ATTACK_WEAPON) < 0) return -1;
    }
    if (row < 0 || row >= MONSTER_ATTACK_MAX) return -1;
    *value = *(const int *)(rows + (size_t)row * MONSTER_ATTACK_SIZE + field);
    return 0;
}

// 1 when square (x, y) is a citadel for `player`.
static int squareCitadelFor(int player, int x, int y) {
#ifdef __APPLE__
    (void)player, (void)x, (void)y;
    return 0;
#else
    return GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y);
#endif
}
