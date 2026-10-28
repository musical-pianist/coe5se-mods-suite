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
    T_FRAME_STR,
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
    T_SLOT_AI_INCOME,
    T_SLOT_AI_TRADE,
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
    T_RECORD_WORD,
    T_AI_ATTACK_MARGIN,
    T_SLOT_ABILITY,
    T_UNIT_STATE_FLAGS,
    T_UNIT_RETURNS,
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
enum { LIST_NONE, LIST_TO_EMPTY_NAME, LIST_TO_BELOW_ONE, LIST_TO_BELOW_ONE_I32, LIST_TO_BELOW_ZERO_I32, LIST_TO_BELOW_ZERO };
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
// Values only the Windows build has an address for so far.
#ifdef __APPLE__
#define WINDOWS_ONLY_RVA(rva) 0
#define WINDOWS_ONLY_F32 T_WINDOWS_ONLY
#define WINDOWS_ONLY_FRAME_STR T_WINDOWS_ONLY
#define BUG_CHECKS_OFF_TYPE T_FLAG_U8
#else
#define WINDOWS_ONLY_RVA(rva) (rva)
#define WINDOWS_ONLY_F32 T_F32
#define WINDOWS_ONLY_FRAME_STR T_FRAME_STR
#define BUG_CHECKS_OFF_TYPE T_FLAG_I32
#endif
#define CEREMONY_INDEXES (CEREMONY_LAST + 1)
#define FORT_PART_BITS 32
#define RESOURCE_COUNT 14
#define NAME_SIZE 100
#define MESSAGE_BOX_TEXT_SIZE 10000
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
    {"slotIncome",     0,                                 T_SLOT_INCOME, PLAYER_SLOTS, 0, 0, 0, 0, RESOURCE_COUNT, 0},
    {"slotAiIncome",   0,                                 T_SLOT_AI_INCOME, PLAYER_SLOTS, 0, 0, 0, 0, RESOURCE_COUNT, 0},
    {"slotAiTrade",    0,                                 T_SLOT_AI_TRADE, PLAYER_SLOTS, 0, 0, 0, 0, RESOURCE_COUNT, 0},
    {"slotTradeType",  RVA_PLAYER_TRADE_TYPE, T_I16, 32, PLAYER_BLOCK_SIZE, 0, 1, 0, 0, 0},
    {"slotTradeMode",  RVA_PLAYER_TRADE_MODE, T_I16, 32, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
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
    {"messageBoxText", WINDOWS_ONLY_RVA(FRAME_MESSAGE_BOX_TEXT), WINDOWS_ONLY_FRAME_STR, 1, 0, MESSAGE_BOX_TEXT_SIZE, 0, SITE_MESSAGE_BOX_WAIT, 0, 0},
    {"mapPointerY",    RVA_SQUARE_INFO_Y,                 T_SITE_I32, 1, 0, 0, 0, SITE_MAP_SCREEN_WAIT, 0, 0},
    {"mouseButtons",   RVA_MOUSE_BUTTONS,                 T_I32, 1, 0, 0, 1, 0, 0, 0},
    {"uiPointerX",     WINDOWS_ONLY_RVA(RVA_UI_MOUSE_X),  WINDOWS_ONLY_F32, 1, 0, 0, 0, 0, 0, 0},
    {"uiPointerY",     WINDOWS_ONLY_RVA(RVA_UI_MOUSE_Y),  WINDOWS_ONLY_F32, 1, 0, 0, 0, 0, 0, 0},
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
    {"selectedCommander", RVA_SELECTED_COMMANDER,         T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"commanderUnit",  RVA_COMMANDERS,                    T_I32, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitual", RVA_COMMANDER_KNOWN_RITUALS,      T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     COMMANDER_KNOWN_RITUALS_MAX, 2},
    {"commanderCostAvailable", RVA_COMMANDERS,            T_COMMANDER_COST, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0,
     INCOME_NAME_COUNT, 0},
    {"commanderAiTargetX", RVA_COMMANDER_AI_TARGET_X,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiTargetY", RVA_COMMANDER_AI_TARGET_Y,     T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderAiMoveKind", RVA_COMMANDER_AI_MOVE_KIND,   T_I16, COMMANDER_TABLE_MAX, 2, 0, 0, 0, 0, 0},
    {"commanderRitualPlan", RVA_COMMANDER_AI_PLAN,        T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderDone",  RVA_COMMANDER_DONE,                T_FLAG_U8, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
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
    {"bugChecksOff",   RVA_NO_CHECKS,                     BUG_CHECKS_OFF_TYPE, 1, 0, 0, 1, 0, 0, 0},
    {"commanderName",  RVA_COMMANDER_NAMES,               T_STR, COMMANDER_TABLE_MAX, COMMANDER_SIZE, COMMANDER_NAME_SIZE, 0, 0, 0, 0},
    {"commanderStrength", RVA_COMMANDERS, T_COMMANDER_STRENGTH, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderGarrisonLeft", RVA_COMMANDERS, T_GARRISON_LEFT, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotHomeX",      RVA_PLAYER_HOME_X,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"slotHomeY",      RVA_PLAYER_HOME_Y,                 T_I16, 32, 2, 0, 0, 0, 0, 0},
    {"squareGems",     RVA_SQUARE_RECORDS + SQUARE_GEMS,  T_U8, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 0, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareCorpses",  RVA_SQUARE_RECORDS + SQUARE_CORPSES, T_I16, MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE, 0, 1, 0, MAP_COLUMN_SQUARES, SQUARE_SIZE},
    {"squareStrength", 0,                                 T_SQUARE_STRENGTH, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"squareRememberedStrength", RVA_SQUARE_RECORDS + SQUARE_AI_STRENGTH, T_REMEMBERED_STRENGTH, PLAYER_SLOTS, 4, 0, 0, 0,
     MAP_COLUMNS_MAX, SQUARE_COLUMN_SIZE},
    {"squareArmyStationary", 0,                           T_ARMY_STATIONARY, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"squareArmyHolds", 0,                                T_ARMY_HOLDS, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"unitStrength",   0,                                 T_UNIT_STRENGTH, PLAYER_SLOTS, 0, 0, 0, 0, UNIT_TABLE_MAX, 0},
    {"unitArmor",      RVA_UNITS,                         T_UNIT_ARMOR, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitVisibleTo",  0,                                 T_UNIT_VISIBLE, UNIT_TABLE_MAX, 0, 0, 0, 0, PLAYER_SLOTS, 0},
    {"monsterAttackType", RVA_MONSTER_TABLE + MONSTER_ATTACKS, T_MONSTER_ATTACK, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackWeapon", RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_WEAPON, T_MONSTER_ATTACK,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"monsterAttackValue", RVA_MONSTER_TABLE + MONSTER_ATTACKS + MONSTER_ATTACK_VALUE, T_MONSTER_ATTACK,
     MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, MONSTER_ATTACK_MAX, MONSTER_ATTACK_SIZE},
    {"weaponDmgType",  RVA_WEAPON_TABLE + WEAPON_DMGTYPE, T_I32, WEAPON_RECORDS_GAME, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"weaponAoe",      RVA_WEAPON_TABLE + WEAPON_AOE, T_I32, WEAPON_RECORDS_GAME, WEAPON_SIZE, 0, 0, 0, 0, 0},
    {"squareCitadel",  0,                                 T_SQUARE_CITADEL, PLAYER_SLOTS, 0, 0, 0, 0, MAP_COLUMNS_MAX, 0},
    {"unitMoveFlags",  RVA_UNITS,                         T_UNIT_MOVE_FLAGS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitFlags",      RVA_UNITS,                         T_UNIT_FLAG_WORDS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"monsterFlags",   RVA_MONSTER_FLAGS,                 T_I64, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"monsterAbility", RVA_MONSTER_TABLE,                 T_MONSTER_ABILITY, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0,
     RITUAL_EFFECT_ID_COUNT, 0},
    {"monsterMoveFlags", RVA_MONSTER_TABLE + MONSTER_MOVE_FLAGS, T_I64, MONSTER_TABLE_MAX, MONSTER_SIZE,
     0, 0, 0, 0, 0},
    {"monsterHp",      RVA_MONSTER_TABLE + MONSTER_HP, T_I16, MONSTER_TABLE_MAX, MONSTER_SIZE, 0, 0, 0, 0, 0},
    {"commanderMoveFlags", RVA_COMMANDERS,                T_COMMANDER_MOVE_FLAGS, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitpow", RVA_COMMANDERS,                   T_COMMANDER_RITPOW, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderRitualLevel", RVA_COMMANDERS,              T_COMMANDER_RITUAL_LEVEL, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"commanderSpellLevel", RVA_COMMANDERS,               T_COMMANDER_SPELL_LEVEL, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"terrainFlags",   RVA_TERRAIN_TABLE + TERRAIN_FLAGS, T_I64, TERRAIN_TABLE_MAX, TERRAIN_SIZE, 0, 0, 0, 0, 0},
    {"squareLibraryLevel", 0,                             T_SQUARE_LIBRARY_LEVEL, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"squareHasFort",  0,                                 T_SQUARE_HAS_FORT, MAP_COLUMNS_MAX, 0, 0, 0, 0, MAP_COLUMN_SQUARES, 0},
    {"seasonTurnsLeft", RVA_SEASON_TURNS_LEFT, T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivOther", RVA_AI_ATTACK_DIV_OTHER, T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivAi",  RVA_AI_ATTACK_DIV_AI, T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackDivHuman", RVA_AI_ATTACK_DIV_HUMAN, T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"aiAttackMargin", 0,                                 T_AI_ATTACK_MARGIN, 1, 0, 0, 0, 0, 0, 0},
    {"ceremonyName",   0,                                 T_CEREMONY_NAME,    CEREMONY_INDEXES, 0, 0, 0, 0, 0, 0},
    {"ceremonyCost",   0,                                 T_CEREMONY_COST,    CEREMONY_INDEXES, 0, 0, 1, SITE_CEREMONY_COSTS, 0, 0},
    {"ceremonyLastIndex", 0,                              T_CEREMONY_LAST,    1, 0, 0, 0, 0, 0, 0},
    {"unitXp",         RVA_UNIT_XP,                       T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitKills",      RVA_UNIT_KILLS,                    T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitItem",       RVA_UNIT_ITEMS,                    T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, UNIT_ITEM_SLOTS, 2},
    {"unitKnownSpell", RVA_UNIT_KNOWN_SPELLS,             T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, UNIT_KNOWN_SPELLS_MAX, 2},
    {"unitMemorizedSpell", RVA_UNIT_MEMORIZED_SPELLS,     T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, UNIT_MEMORIZED_SPELLS_MAX, 2},
    {"squareItem",     RVA_GROUND_ITEMS,                  T_I16, MAP_COLUMNS_MAX, MAP_COLUMN_SQUARES * GROUND_ITEMS_PER_SQUARE * 2, 0, 0, 0,
     MAP_COLUMN_SQUARES, GROUND_ITEMS_PER_SQUARE * 2},
    {"commanderRepeatRitual", RVA_COMMANDER_REPEAT_RITUAL, T_I16, COMMANDER_TABLE_MAX, COMMANDER_SIZE, 0, 0, 0, 0, 0},
    {"slotBolsterDevotions", RVA_PLAYER_BOLSTERS,         T_I32, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"slotDivineFavor", RVA_PLAYER_DIVINE_FAVOR,          T_I32, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"sealsBroken",    RVA_SEALS_BROKEN,                  T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"sealEventsDone", RVA_SEAL_EVENTS_DONE,              T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"starsReliability", RVA_STARS_RELIABILITY,           T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"starsPower",     RVA_STARS_POWER,                   T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"starsMonths",    RVA_STARS_MONTHS,                  T_I16, 1, 0, 0, 0, 0, 0, 0},
    {"starsFlags",     RVA_STARS_FLAGS,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"apocalypseMonths", RVA_APOCALYPSE_MONTHS,           T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"magicTaintedMonths", RVA_MAGIC_TAINTED_MONTHS,      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"mapZoom",        RVA_MAP_ZOOM,                      T_F32, 1, 0, 0, 0, 0, 0, 0},
    {"mapCameraX",     RVA_MAP_CAMERA_X,                  T_F32, 1, 0, 0, 0, 0, 0, 0},
    {"mapCameraY",     RVA_MAP_CAMERA_Y,                  T_F32, 1, 0, 0, 0, 0, 0, 0},
    {"mapViewPlane",   RVA_MAP_VIEW_PLANE,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"enchantLastIndex", RVA_LAST_ENCHANT,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"enchantKind",    RVA_ENCHANTS + ENCHANT_KIND,       T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"enchantCaster",  RVA_ENCHANTS + ENCHANT_CASTER,     T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"enchantOwner",   RVA_ENCHANTS + ENCHANT_OWNER,      T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"enchantRitual",  RVA_ENCHANTS + ENCHANT_RITUAL,     T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"enchantX",       RVA_ENCHANTS + ENCHANT_X,          T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"enchantY",       RVA_ENCHANTS + ENCHANT_Y,          T_I32, ENCHANTS_MAX, ENCHANT_SIZE, 0, 0, 0, 0, 0},
    {"battleSeed",     RVA_BATTLE_SEED,                   T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleX",        RVA_BATTLE_X,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleY",        RVA_BATTLE_Y,                      T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleWidth",    RVA_BATTLE_WIDTH,                  T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleRound",    RVA_BATTLE_ROUND,                  T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleWinner",   RVA_BATTLE_WINNER_PLAYER,          T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleStarveRound", RVA_BATTLE_STARVE_ROUND,        T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleChargeRound", RVA_BATTLE_CHARGE_ROUND,        T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"battleAmbush",   RVA_BATTLE_IS_AMBUSH,              INT_FLAG_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"battleAtSea",    RVA_BATTLE_AT_SEA,                 INT_FLAG_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"battleSiege",    RVA_BATTLE_IS_SIEGE,               INT_FLAG_TYPE, 1, 0, 0, 0, 0, 0, 0},
    {"eventPlayer",    RVA_EVENT_PLAYER,                  T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"eventTargetX",   RVA_EVENT_TARGET_X,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"eventTargetY",   RVA_EVENT_TARGET_Y,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"eventSquareX",   RVA_EVENT_SQUARE_X,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"eventSquareY",   RVA_EVENT_SQUARE_Y,                T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"unitLastSpawned", RVA_LAST_SPAWNED_UNIT,            T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"commanderLastSpawned", RVA_LAST_SPAWNED_COMMANDER,  T_I32, 1, 0, 0, 0, 0, 0, 0},
    {"unitTempAbilityId", RVA_UNIT_TEMP_ABILITY_IDS,      T_I16, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, UNIT_TEMP_ABILITIES, 2},
    {"unitTempAbilityValue", RVA_UNIT_TEMP_ABILITY_VALUES, T_I32, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, UNIT_TEMP_ABILITIES, 4},
    {"slotBlessingId", RVA_PLAYER_BLESSINGS,              T_I32, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, PLAYER_BLESSINGS_MAX, 8},
    {"slotBlessingValue", RVA_PLAYER_BLESSINGS + 4,       T_I32, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, PLAYER_BLESSINGS_MAX, 8},
    {"slotMostCitadels", RVA_PLAYER_SHORT,                T_I16, PLAYER_SLOTS, PLAYER_BLOCK_SIZE, 0, 0, 0, 0, 0},
    {"slotAbility",    0,                                 T_SLOT_ABILITY, PLAYER_SLOTS, 0, 0, 0, 0, RITUAL_EFFECT_ID_COUNT, 0},
    {"slotAbilitySum", 0,                                 T_SLOT_ABILITY, PLAYER_SLOTS, 0, 1, 0, 0, RITUAL_EFFECT_ID_COUNT, 0},
    {"unitStateFlags", RVA_UNITS,                         T_UNIT_STATE_FLAGS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    {"unitReturns",    RVA_UNITS,                         T_UNIT_RETURNS, UNIT_TABLE_MAX, UNIT_SIZE, 0, 0, 0, 0, 0},
    // The terrain word for fort part bit n (walls, irongate, ...), as the game's terrain reader has it.
    {"fortPartWord",   0,                                 T_FORT_PART_WORD, FORT_PART_BITS, 0, 0, 0, 0, 0, 0},
    // A content record by the .c5m words of its block: coe5.get("monster", m, "hp"), read-only.
    {"monster",        RVA_MONSTER_TABLE,                 T_RECORD_WORD, MONSTER_TABLE_MAX, MONSTER_SIZE, RECORD_MONSTER, 0, 0, 0, 0},
    {"weapon",         RVA_WEAPON_TABLE,                  T_RECORD_WORD, WEAPON_RECORDS_GAME, WEAPON_SIZE, RECORD_WEAPON, 0, 0, 0, 0},
    {"item",           RVA_ITEM_TABLE,                    T_RECORD_WORD, ITEM_TABLE_MAX, ITEM_SIZE, RECORD_ITEM, 0, 0, 0, 0},
    {"class",          RVA_CLASS_TABLE,                   T_RECORD_WORD, CLASS_TABLE_MAX, CLASS_SIZE, RECORD_CLASS, 0, 0, 0, 0},
    {"terrain",        RVA_TERRAIN_TABLE,                 T_RECORD_WORD, TERRAIN_TABLE_MAX, TERRAIN_SIZE, RECORD_TERRAIN, 0, 0, 0, 0},
    {"ritual",         RVA_RITUAL_TABLE,                  T_RECORD_WORD, COUNT_AT_CALL, RITUAL_SIZE, RECORD_RITUAL, 0, 0, 0, 0},
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
    {"squareCorpses", SQUARE_CORPSE_KINDS, 2, LIST_NONE, 0, 0, 0},
    {"squareStrength", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
    {"squareRememberedStrength", MAP_COLUMN_SQUARES, SQUARE_SIZE, LIST_NONE, 0, 0, 0},
    {"unitStrength", 0, 0, LIST_NONE, 0, 0, 1},
    {"squareCitadel", MAP_COLUMN_SQUARES, 0, LIST_NONE, 0, 0, 1},
    {"unitKnownSpell", 0, 0, LIST_TO_BELOW_ZERO, 0, 0, 0},
    {"unitMemorizedSpell", 0, 0, LIST_TO_BELOW_ZERO, 0, 0, 0},
    {"squareItem", GROUND_ITEMS_PER_SQUARE, 2, LIST_NONE, 0, 0, 0},
    {"slotBlessingId", 0, 0, LIST_TO_BELOW_ONE_I32, 0, 0, 0},
    {"slotBlessingValue", 0, 0, LIST_TO_BELOW_ONE_I32, 4, 0, 0},
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

// The ability id the .c5m word `word` sets, or -1 for a word that sets none.
static int abilityIdOfWord(const char *word) {
    size_t len = strlen(word);
    for (size_t i = 0; i < sizeof RITUAL_EFFECT_NAMES / sizeof RITUAL_EFFECT_NAMES[0]; i++)
        for (const char *at = RITUAL_EFFECT_NAMES[i].name; *at;) {
            const char *end = strchr(at, '/');
            size_t n = end ? (size_t)(end - at) : strlen(at);
            if (n == len && !memcmp(at, word, len)) return RITUAL_EFFECT_NAMES[i].id;
            at += n + (end != NULL);
        }
    return -1;
}

// Whether a variable's second index is an ability id, which a script may also give as its .c5m word.
static int takesAbilityWord(int type) { return type == T_UNIT_ABILITY || type == T_MONSTER_ABILITY || type == T_SLOT_ABILITY; }

// Whether a variable's index `which` (1 or 2) is a player, which a script may also give as a faction's name.
static int indexIsPlayer(const GameVar *v, int which) {
    if (which == 2) return v->type == T_UNIT_VISIBLE;
    return !strncmp(v->name, "slot", 4) || v->addr == RVA_SQUARE_RECORDS + SQUARE_SEEN;
}

// Finds the variable named by argument 1 of coe5.get or coe5.set, and returns its address.
static unsigned char *gameVarAddress(lua_State *L, const GameVar **out, int *arg) {
    const char *name = luaL_checkstring(L, 1);
    const GameVar *v = findGameVar(name);
    if (!v) { luaL_error(L, "unknown game variable '%s'", name); return NULL; }
    int isRitualTable = v->count == COUNT_AT_CALL && v->addr >= RVA_RITUAL_TABLE && v->addr < RVA_RITUAL_TABLE + RITUAL_SIZE;
    int movedTable = isRitualTable || !v->addr ? -1 : tableHolding(v->addr, v->stride);
    int playerTable = !isRitualTable && movedTable < 0 && v->addr && v->count == PLAYER_SLOTS && playerTableHolding(v->addr) >= 0;
    lua_Integer index = 0;
    int squareSight = v->addr == RVA_SQUARE_RECORDS + SQUARE_SEEN;
    int count = isRitualTable ? ritualSlotCount() : movedTable >= 0 ? tableRecords(movedTable) : playerTable || squareSight ? playerSlotCount() : v->count;
    GameVarShape shape = gameVarShape(v);
    if (count > 1) {
        int first = shape.everyPlayer ? -1 : 0;
        index = indexIsPlayer(v, 1) ? luaCheckPlayer(L, (*arg)++) : luaL_checkinteger(L, (*arg)++);
        if (index < first || index >= count) luaL_error(L, "%s: index %I out of range %d-%d", name, (LUAI_UACINT)index, first, count - 1);
    }
    lua_Integer index2 = 0, index3 = 0;
    int count2 = v->type == T_CAST_COUNT ? ritualSlotCount() : v->count2;
    if (count2 > 1) {
        if (takesAbilityWord(v->type) && lua_type(L, *arg) == LUA_TSTRING && !lua_isnumber(L, *arg)) {
            const char *word = lua_tostring(L, *arg);
            int id = abilityIdOfWord(word);
            if (id < 0) luaL_error(L, "%s: unknown ability word '%s'", name, word);
            lua_pushinteger(L, id);
            lua_replace(L, *arg);
        }
        index2 = indexIsPlayer(v, 2) ? luaCheckPlayer(L, (*arg)++) : luaL_checkinteger(L, (*arg)++);
        if (index2 < 0 || index2 >= count2) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index2, count2 - 1);
    }
    if (shape.count3 > 1) {
        index3 = luaL_checkinteger(L, (*arg)++);
        if (index3 < 0 || index3 >= shape.count3) luaL_error(L, "%s: index %I out of range 0-%d", name, (LUAI_UACINT)index3, shape.count3 - 1);
    }
    *out = v;
    if (v->type == T_FRAME_I32 || v->type == T_FRAME_STR) {
        CallSite running = runningFrameSite(v->site);
        uintptr_t sp = running != SITE_COUNT ? luaApi->callerRsp(running) : 0;
        return sp ? (unsigned char *)(sp + v->addr) : NULL;
    }
    if ((v->type == T_SITE_I32 || v->type == T_CEREMONY_COST) && !luaSiteDepth[v->site]) return NULL;
    if (squareSight && factionSlot((int)index)) return factionSquareSeen ? factionSquareSeen + factionSquareIndex((int)index2, (int)index3, (int)index) : NULL;
    unsigned char *table = v->type == T_CAST_COUNT ? castCountsBase()
                         : v->addr == RVA_PLANES ? planeTableAddress()
                         : movedTable >= 0 ? tableAddress(movedTable) + (v->addr - tableMoves[movedTable].rva)
                         : playerTable ? playerTableAddress(v->addr)
                         : gameBase + (isRitualTable ? ritualTableRva() + (v->addr - RVA_RITUAL_TABLE) : v->addr);
    if (shape.lastAt && index > G32(shape.lastAt)) return NULL;
    unsigned char *entries = table + (size_t)index * v->stride;
    for (lua_Integer k = 0; shape.list != LIST_NONE && k <= index2; k++) {
        const unsigned char *entry = entries - shape.nameBack + (size_t)k * v->stride2;
        int ended = shape.list == LIST_TO_BELOW_ONE       ? *(const short *)entry < 1
                    : shape.list == LIST_TO_BELOW_ONE_I32  ? *(const int *)entry < 1
                    : shape.list == LIST_TO_BELOW_ZERO_I32 ? *(const int *)entry < 0
                    : shape.list == LIST_TO_BELOW_ZERO     ? *(const short *)entry < 0
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
    if (owner < 0 || owner >= playerSlotCount()) return -1;
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
    return GAME_FN(int (*)(int), RVA_COMMANDER_STRENGTH)(commander);
#else
    return GAME_FN(int (*)(int, int), RVA_COMMANDER_FLAGGED_STRENGTH)(commander, 0);
#endif
}

// The strength of the units on square (x, y) of player `team` and its teammates.
static int strengthQuery(int team, int x, int y);
static int squareStrength(int team, int x, int y) { return strengthQuery(team, x, y); }

static int unitAliveAt(int u, int x, int y);

// 1 when the game never moves the army on square (x, y).
static int armyStationaryAt(int x, int y) {
    int last = G32(RVA_UNIT_COUNT);
    for (int u = 0; u <= last && u < UNIT_TABLE_MAX; u++)
        if (unitAliveAt(u, x, y) && G16(RVA_UNIT_MONSTER + (size_t)u * UNIT_SIZE) >= 0 && !unitStationary(u)) return 0;
    return 1;
}

// 1 when the independents' move never moves the army on square (x, y).
static int holdFlagsHold(int squareFlags, int top, unsigned long long topFlags) {
    if (squareFlags & SQUARE_FLAG_HOLDIT) return 1;
    return top >= 0 && ((topFlags >> UNIT_FLAG2_HOLDIT) & 1);
}

static int armyHoldsAt(int x, int y) {
    if (x < 0 || y < 0 || x >= G32(RVA_MAP_DEPTH) || y >= G32(RVA_MAP_H)) return 0;
    size_t square = ((size_t)x * MAP_COLUMN_SQUARES + (size_t)y) * SQUARE_SIZE;
    int squareFlags = G32(RVA_SQUARE_FLAGS + square);
    if (holdFlagsHold(squareFlags, -1, 0)) return 1;
    int owner = armyOwnerAt(x, y);
    if (owner < 0) return 0;
#ifdef __APPLE__
    (void)owner;
    int top = GAME_FN(int (*)(int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(-1, x, y, -1, -1, NULL);
#else
    int top = GAME_FN(int (*)(int, int, int, int, int, int, int *), RVA_SQUARE_TOP_UNIT)(owner, -1, x, y, -1, -1, NULL);
#endif
    if (top < 0 || top >= UNIT_TABLE_MAX) return 0;
    return holdFlagsHold(squareFlags, top, *(const unsigned long long *)GAME_PTR(RVA_UNITS + (size_t)top * UNIT_SIZE + 8));
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
    size_t at = (size_t)unit * UNIT_SIZE;
    int (*core)(int, int, int) = GAME_FN(int (*)(int, int, int), RVA_UNIT_STRENGTH_CORE);
    short hp = (short)G16(RVA_UNIT_HP + at);
    if ((short)G16(RVA_UNIT_PLAYER + at) == player) return core(unit, -1, hp);
    int monster = (short)G16(RVA_UNIT_MONSTER + at);
    if (monster < 0) return 0;
    int as = GAME_FN(int (*)(int, int), RVA_UNIT_COUNTS_AS_MONSTER)(unit, player);
    if (as >= 0) monster = as;
    const unsigned char *record = monsterRecord(monster);
    if (!record) return 0;
    short most = *(const short *)(record + MONSTER_HP);
    return core(-1, monster, hp >= most ? most : hp);
}

// A living unit's armor as the unit screen shows it, or -1.
static int unitArmorNow(int unit) {
    if (!unitAliveInTable(unit)) return -1;
    return GAME_FN(int (*)(int), RVA_UNIT_ARMOR)(unit);
}

// A living unit's state bits, its items' with them, into *flags; 0 for a unit that isn't alive.
static int unitStateFlagsNow(int unit, unsigned long long *flags) {
    if (!unitAliveInTable(unit)) return 0;
    *flags = GAME_FN(unsigned long long (*)(int), RVA_UNIT_STATE_FLAGS)(unit);
    return 1;
}

// Whether unit `unit` comes back from death, as the game decides it; -1 for an empty record.
static int unitReturnsNow(int unit) {
    if (unit < 0 || unit >= UNIT_TABLE_MAX || unit > G32(RVA_UNIT_COUNT) || (short)G16(RVA_UNIT_MONSTER + (size_t)unit * UNIT_SIZE) < 0) return -1;
    return GAME_FN(int (*)(int), RVA_UNIT_RETURNS)(unit) != 0;
}

// Whether player `player` sees unit `unit`.
static int unitVisibleToNow(int unit, int player) {
    if (!unitAliveInTable(unit)) return 0;
    size_t at = (size_t)unit * UNIT_SIZE;
    return GAME_FN(int (*)(int, int, int, int), RVA_UNIT_VISIBLE_TO)(player, unit, G32(RVA_UNIT_X + at), G32(RVA_UNIT_X + 4 + at)) != 0;
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
    return GAME_FN(int (*)(int, int, int), RVA_SQUARE_IS_CITADEL_FOR)(player, x, y);
}
