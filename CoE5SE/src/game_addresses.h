#ifndef GAME_ADDRESSES_H
#define GAME_ADDRESSES_H

#ifdef __APPLE__
#define MAC(va) ((unsigned)((va) - 0x100000000ULL))
#define ADDR(win, mac) MAC(mac)
#else
#define ADDR(win, mac) (win)
#endif
#define ADDR_DIFFERS(win, mac) ADDR(win, mac)

#ifdef __APPLE__
#define FRAME_OFFSET(win, mac) (mac)
#else
#define FRAME_OFFSET(win, mac) (win)
#endif

#define MAC_INLINED            no_mac_address__the_function_is_copied_into_its_callers
#define MAC_NO_CALL            no_mac_address__this_call_is_not_made_on_mac
#define MAC_IN_CODE            no_mac_address__the_value_is_built_into_the_instructions
#define MAC_STORED_DIFFERENTLY no_mac_address__mac_keeps_this_data_another_way
#define MAC_FILE_FORMAT        no_mac_address__part_of_the_windows_file_format
#define MAC_NOT_FOUND          no_mac_address__searched_for_and_not_found
#define MAC_NOT_LOOKED_UP      no_mac_address__not_searched_for_yet
#define WIN_NO_EQUIVALENT      no_windows_address__only_the_mac_build_has_this
#define WIN_FILE_FORMAT        no_windows_address__part_of_the_mac_file_format
#define WIN_NOT_FOUND          no_windows_address__searched_for_and_not_found
#define WIN_INLINED            no_windows_address__the_function_is_copied_into_its_callers
#define WIN_NO_CALL            no_windows_address__this_call_is_not_made_on_windows
#define WIN_IN_CODE            no_windows_address__the_value_is_built_into_the_instructions
#define WIN_STORED_DIFFERENTLY no_windows_address__windows_keeps_this_data_another_way

// ---- Image layout -----------------------------------------------------------------------------------
#define RVA_TEXT_SECTION                       ADDR(        0x001000,          MAC_FILE_FORMAT)
#define RVA_RDATA_SECTION                      ADDR(        0x33a000,          MAC_FILE_FORMAT)
#define RVA_DATA_SECTION                       ADDR(        0x42c000,          MAC_FILE_FORMAT)
#define RVA_PDATA_SECTION                      ADDR(        0x136f2000,        MAC_FILE_FORMAT)
#define RVA_DATA_CONST_SEGMENT                 ADDR(        WIN_FILE_FORMAT,   0x100774000)
#define RVA_DATA_SEGMENT                       ADDR(        WIN_FILE_FORMAT,   0x10084c000)
#define RVA_LINKEDIT_SEGMENT                   ADDR(        WIN_FILE_FORMAT,   0x11399c000)
#define RVA_TEXT_CODE                          ADDR(        WIN_FILE_FORMAT,   0x100003298)
#define RVA_STUBS                              ADDR(        WIN_FILE_FORMAT,   0x1005adab0)
#define RVA_HEADER_PADDING                     ADDR(        WIN_FILE_FORMAT,   0x1000013f0)

// ---- Main menu --------------------------------------------------------------------------------------
#define RVA_MAIN_MENU                          ADDR(        0x191870,          0x10032deac)
#define RVA_MENU_PREFERENCES                   ADDR(        0x197b70,          0x10032bc80)
#define RVA_NETWORK_SCREEN                     ADDR(        0x1f17a0,          MAC_INLINED)
#define RVA_MENU_ID5                           ADDR(        0x1b72c0,          0x100514730)
#define RVA_CREDITS                            ADDR(        0x0b6890,          MAC_INLINED)
#define RVA_TEST_GAME                          ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_READ_TIP                           ADDR(        0x23ef30,          MAC_INLINED)
#define RVA_READ_TIP_2                         ADDR(        0x23e7b0,          MAC_INLINED)
#define RVA_COUNT_SAVES                        ADDR(        0x18b080,          0x1004eb1c4)
#define RVA_MENU_LOOP                          ADDR_DIFFERS(0x191982,          0x10032dfc0)
#define RVA_MODS_SCREEN_WAIT                   ADDR(        0x1ca57f,          0x10032f2b0)
#define RVA_NETWORK_SCREEN_WAIT                ADDR(        0x1f1a69,          0x10032e974)
#define RVA_MENU_FADE_ON                       ADDR(        0x1a4269c,         0x100872bec)
#define RVA_FRAME_DELTA                        ADDR(        0xd63ebdc,         0x10c922c38)
#define RVA_MENU_LABEL_SIZE                    ADDR(        0x406bc4,          MAC_IN_CODE)
#define RVA_MENU_X_OFFSET                      ADDR(        0x406c14,          MAC_IN_CODE)
#define RVA_MENU_Y_STEP                        ADDR(        0x406d8c,          MAC_IN_CODE)
#define RVA_MENU_BUTTON_W                      ADDR(        0x406c44,          MAC_IN_CODE)
#define RVA_MENU_BG_INSET                      ADDR(        0x406b58,          MAC_IN_CODE)
#define RVA_FLOAT_ONE                          ADDR(        0x406844,          MAC_IN_CODE)
#define RVA_DEFAULT_COLOUR                     ADDR(        0x406d0c,          MAC_IN_CODE)
#define RVA_FADE_START                         ADDR(        0x406cf8,          MAC_IN_CODE)
#define RVA_FADE_RATE                          ADDR(        0x406808,          MAC_IN_CODE)
#define RVA_FADE_CAP                           ADDR(        0x406b28,          MAC_IN_CODE)
#define RVA_QUIT_FADE_DELAY                    ADDR(        0x406b30,          MAC_IN_CODE)
#define RVA_BG_FADE_START                      ADDR(        0x406cd8,          MAC_IN_CODE)
#define RVA_BG_FADE_RATE                       ADDR(        0x406798,          MAC_IN_CODE)
#define RVA_BG_FADE_CAP                        ADDR(        0x406930,          MAC_IN_CODE)
#define RVA_MENU_FLOAT4                        ADDR(        WIN_NO_EQUIVALENT, 0x100757a80)
#define RVA_MAIN_MENU_WAIT_PATTERN             ADDR(        WIN_NOT_FOUND,     MAC_FILE_FORMAT)
#define RVA_NETWORK_WAIT_PATTERN               ADDR(        WIN_NOT_FOUND,     MAC_FILE_FORMAT)
#define RVA_MENU_BG_FADE_STEP                  ADDR(        0x191aa3,          0x10032e128)
#define RVA_MENU_FADE_TIMER_STEP               ADDR(        0x191ada,          0x10032e150)
#define RVA_MENU_FADE_RELOAD                   ADDR(        WIN_NO_EQUIVALENT, 0x10032e28c)
#define RVA_MENU_BANNER_FADE_SET               ADDR(        0x192600,          0x10032e60c)
#define RVA_MENU_LOOP_JUMP                     ADDR(        0x19260f,          0x10032e610)
#define RVA_MENU_PREFS_CALL                    ADDR(        0x19265e,          0x10032e6e8)
#define RVA_MENU_TEST_GAME                     ADDR_DIFFERS(0x1926b3,          0x10032ecac)
#define RVA_MENU_READ_TIP                      ADDR_DIFFERS(0x1926ff,          0x10032f43c)
#define RVA_CREDITS_LOGO_CALL                  ADDR(        0x0b69f5,          0x10032eb50)
#define RVA_QUIT_MESSAGE                       ADDR(        0x5822f72,         MAC_NOT_FOUND)

// ---- Screens and widgets ------------------------------------------------------------------------
#define RVA_WAIT_FOR_CLICK                     ADDR(        0x23ef50,          0x100485230)
#define RVA_DRAW_BUTTON                        ADDR(        0x0363a0,          0x10033fb84)
#define RVA_DRAW_IMAGE                         ADDR(        0x0370c0,          0x10033f1fc)
#define RVA_SCREEN_W                           ADDR(        0x435d1c,          0x104904c78)
#define RVA_SCREEN_H                           ADDR(        0x431824,          0x104904c7c)
#define RVA_DATA_DIR                           ADDR(        0x5828af0,         0x10c922028)

#define RVA_SCREEN_BEGIN                       ADDR(        0x03ae60,          0x10033c330)
#define RVA_SCREEN_TITLE                       ADDR(        0x033610,          0x100341ebc)
#define RVA_SECTION_BEGIN                      ADDR(        0x035560,          0x100340b2c)
#define RVA_SECTION_END                        ADDR(        0x035400,          0x100340e70)
#define RVA_SECTION_HEADER                     ADDR(        0x089690,          0x10049007c)
#define RVA_RADIO_OPTION                       ADDR(        0x0389a0,          0x10033e4bc)
#define RVA_TEXT_BUTTON                        ADDR(        0x039ad0,          0x10033d690)
#define RVA_BIND_HOTKEY                        ADDR(        0x03aae0,          0x10033cd34)
#define RVA_BIND_HOTKEY_CTRL                   ADDR(        0x03a940,          0x10033ce58)
#define RVA_BIND_HOTKEY_ALT                    ADDR(        0x03a8b0,          MAC_INLINED)
#define RVA_CHECKBOX                           ADDR(        0x038d50,          0x10033de68)
#define RVA_TOOLTIP                            ADDR(        0x039180,          0x10033dc34)
#define RVA_OPTION_TOOLTIP                     ADDR(        0x039350,          0x10033db18)
#define RVA_TEXT                               ADDR(        0x038f60,          0x10033ddcc)
#define SCREEN_TITLE_TEXT_SIZE                 0x64
#define TEXT_BUTTON_TEXT_SIZE                  0x1388
#define TEXT_LINE_TEXT_SIZE                    0x1388
#define TOOLTIP_TEXT_SIZE                      0x1388
#define CHECKBOX_TEXT_SIZE                     0x3e8
#define RADIO_OPTION_TEXT_SIZE                 0x3e7
#define PROGRESS_TEXT_TEXT_SIZE                0x1388
#define RVA_LAYOUT_NEW                         ADDR(        0x03a3f0,          0x10033cf60)
#define RVA_LAYOUT_PASS                        ADDR(        0x0304e0,          0x100341f54)
#define RVA_GUI_ALLOC                          ADDR(        0x03ac50,          0x10033cbbc)
#define RVA_TRANSLATE_FORMAT                   ADDR(        0x03d200,          0x10033ba70)
#define RVA_SCREEN_WIDTH                       ADDR(        0x03a6d0,          MAC_INLINED)
#define RVA_BUTTON_ROW                         ADDR(        0x038490,          MAC_INLINED)
#define RVA_SCREEN_LAYOUT                      ADDR(        0x0304c0,          MAC_INLINED)
#define RVA_CLICK_SOUND                        ADDR(        0x0b0e80,          MAC_INLINED)
#define RVA_SCREEN_MUSIC                       ADDR(        0x034c40,          MAC_INLINED)
#define RVA_ESCAPE_KEY                         ADDR_DIFFERS(0x12ab01c2,        0x113997cb6)
#define RVA_DISABLED_BUTTON                    ADDR(        0x0396b0,          0x10033d9a4)
#define RVA_LINE_ADVANCE                       ADDR(        0x03a500,          MAC_INLINED)
#define RVA_DRAW_SPRITE                        ADDR_DIFFERS(0x035da0,          0x1003402dc)
#define RVA_LOAD_SPRITE                        ADDR_DIFFERS(0x064200,          0x10034eda8)
#define RVA_SPRITE_BUTTON                      ADDR(        0x038380,          0x10033e558)
#define RVA_SPRITE_BUTTON_TINTED               ADDR(        0x0382e0,          MAC_INLINED)
#define RVA_DRAW_SPRITE_40                     ADDR(        0x035b70,          MAC_INLINED)
#define RVA_DRAW_SPRITE_30                     ADDR(        0x035b90,          MAC_INLINED)
#define SPRITE_KIND_MONSTER                    0x21
#define SPRITE_KIND_TERRAIN                    0x20
#define SPRITE_KIND_ITEM                       2
#define SPRITE_KIND_COUNT                      0x26
#define SPRITES_PER_KIND                       0x7fbc
#define RVA_SPRITE_DRAW_SCALED                 ADDR(        0x064f90,          0x10034f97c)
#define RVA_LOAD_SPRITE_FILE                   ADDR(        0x0646a0,          0x10034e724)
#define RVA_SPRITE_FILES                       ADDR(        0x5ea83d0,         0x1062a5800)
#define RVA_SPRITE_REPLACED                    ADDR(        0x57f2f40,         0x1062a5930)
#define RVA_REPLACED_SPRITE                    ADDR(        0x064a10,          MAC_INLINED)
#define RVA_SPRITE_WIDTHS                      ADDR(        0x5c0ee80,         0x1062a6424)
#define RVA_SPRITE_HEIGHTS                     ADDR(        0x5ea8500,         0x106504ff4)
#define RVA_TOKEN_ITEM_ICON_CALL               ADDR(        0x0fef37,          0x10057ec0c)
#define SPRITE_KIND_ICON                       0x1b
#define SPRITE_KIND_ITEM                       2
#define LAYOUT_TYPE_SPRITE_BUTTON              7
#define LAYOUT_TYPE_SPRITE                     0x14
#define RVA_MONSTER_PICKER                     ADDR(        0x087fa0,          MAC_INLINED)
#define RVA_RECRUIT_ROW                        ADDR(        0x108eb0,          MAC_INLINED)
#define RVA_SETUP_WIDTH_SCALE                  ADDR(        0x1a5bb54,         0x104904ca4)
#define RVA_SETUP_WIDTH_UNIT                   ADDR(        0x406c78,          MAC_IN_CODE)
#define RVA_ESCAPE_ON                          RVA_ESCAPE_KEY
#define RVA_TEXT_ROW                           ADDR_DIFFERS(0x03a100,          0x10033d07c)
#define RVA_GUI_LAST_SLOT                      ADDR(        0x42dc24,          0x100872bf0)
#define RVA_LAYOUT_LAST                        ADDR(        0x436724,          0x100857618)
#define RVA_COLUMN_BASE                        ADDR(        0x42dcb4,          0x100858038)
#define RVA_TEXT_HEIGHT_BASE                   ADDR(        0x432218,          0x100862640)
#define RVA_SET_TEXT_SCALE                     ADDR(        0x03a660,          MAC_INLINED)
#define RVA_LINE_WIDTH                         ADDR(        0x57c62cc,         0x10611b920)
#define RVA_WIDEST_LINE                        ADDR(        0x57ee788,         0x10611b918)
#define RVA_ROW_OPEN                           ADDR(        0x42dc8c,          0x10611b91c)
#define RVA_SCREEN_WIDTH_A                     ADDR(        0x436728,          0x100862684)
#define RVA_SCREEN_WIDTH_B                     ADDR(        0x437b18,          0x100862688)
#define RVA_GUI_NEXT_SLOT                      ADDR(        0x57ee790,         0x105b68178)
#define RVA_HOVERED_ID                         ADDR(        0x432204,          0x1008626ec)
#define RVA_TEXT_FIELD_FOCUS                   ADDR(        0x42e704,          0x1008626a0)
#define RVA_TEXT_FIELD_CURSOR                  ADDR(        0x57c58dc,         0x10611b990)
#define RVA_LAYOUT_TOP_Y                       ADDR(        0x434974,          0x10085761c)
#define RVA_SCREEN_RESET_A                     ADDR(        0x435d20,          0x100872b64)
#define RVA_SCREEN_RESET_B                     ADDR(        0x430e54,          0x10086269c)
#define RVA_SCREEN_RESET_C                     ADDR(        0x42dcfc,          0x1008626a4)
#define RVA_SCREEN_RESET_BYTE                  ADDR(        WIN_NOT_FOUND,     0x106cee568)
#define RVA_LAYOUT_MARK_OPEN                   ADDR(        0x0297e0,          MAC_INLINED)
#define RVA_LAYOUT_MARK_CLOSE                  ADDR(        0x0297a0,          MAC_INLINED)
#define LAYOUT_TYPE_MARK                       0x67

#define RVA_ROW_STRIPE                         ADDR(        0x038590,          0x10033eaf8)
#define RVA_ROW_BEGIN                          ADDR(        0x03a5e0,          MAC_INLINED)
#define RVA_ROW_AREA                           ADDR(        0x035830,          MAC_INLINED)
#define RVA_COLUMN_X                           ADDR(        0x03a380,          MAC_INLINED)
#define RVA_LIST_GAP                           ADDR(        0x038530,          MAC_INLINED)
#define RVA_LIST_STYLE                         ADDR(        0x038490,          MAC_INLINED)

#define RVA_GUI_TABLE                          ADDR(        0x12ab2ac0,        0x10b979b10)
#define GUI_STRIDE                             0x78
#define GUI_ELEMENT_Y                          0x08
#define GUI_ELEMENT_WIDTH                      0x0c
#define GUI_ELEMENT_TOOLTIP                    0x4c
#define GUI_ELEMENT_LABEL                      0x60
#define RVA_LAYOUT_ELEMENTS                    ADDR(        0x1319ab60,        0x105ba4288)
#define LAYOUT_STRIDE                          0x70
#define LAYOUT_ELEMENT_COLUMN_X                0x0c
#define LAYOUT_ELEMENT_WIDTH                   0x10
#define LAYOUT_ELEMENT_TYPE                    0x60
#define LAYOUT_ELEMENT_LINE_END                0x6c
#define LAYOUT_TYPE_LIST_STYLE                 8
#define LAYOUT_TYPE_BUTTON_ROW                 9
#define LAYOUT_TYPE_LIST_GAP                   10
#define LAYOUT_TYPE_COLUMN_X                   12
#define LAYOUT_TYPE_ROW_AREA                   15
#define LAYOUT_TYPE_ROW_BEGIN_LIMIT            100
#define RVA_LAYOUT_TABLE                       RVA_LAYOUT_ELEMENTS
#define RVA_TOOLTIP_TABLE                      ADDR(        0x13190a60,        0x10c057810)
#define RVA_TOOLTIP_COUNT                      ADDR(        0x433fa8,          0x100857620)
#define TOOLTIP_SLOTS                          5000
#define RVA_TOOLTIP_SHOWN                      ADDR(        0x42dd1c,          0x1008626f0)
#define RVA_TOOLTIP_DRAWN                      ADDR(        0x42dc54,          0x1008626f8)
#define RVA_TOOLTIP_SHOWN_AT                   ADDR(        0x5836014,         0x10611be6c)
#define RVA_TOOLTIP_FADE_ON                    ADDR(        0x42f0d8,          0x10085762c)
#define RVA_TOOLTIP_LINE_HEIGHT                ADDR(        0x42dbfc,          0x100862644)
#define TOOLTIP_BOX_BOTTOM                     16
#define TOOLTIP_BOX_DROP                       2
#define RVA_DRAW_TOOLTIP                       ADDR(        0x029ce0,          MAC_INLINED)
#define RVA_TOOLTIP_FADE_START                 ADDR(        0x406928,          MAC_IN_CODE)
#define RVA_TOOLTIP_FADE_RATE                  ADDR(        0x4067c8,          MAC_IN_CODE)
#define RVA_TOOLTIP_LINES                      ADDR(        0x123e9b40,        0x1061a69f8)
#define TOOLTIP_LINE_SIZE                      0x180
#define RVA_TOOLTIP_LINE_WIDTHS                ADDR(        0x123e8ba0,        0x1062045f8)
#define RVA_TEXT_WRAP                          ADDR(        0x053c50,          0x10034dd80)
#define RVA_BUTTON_CORE                        ADDR(        0x039790,          0x10033d808)
#define RVA_TOOLTIP_STORE                      ADDR(        0x0393d2,          0x10033db64)
#define RVA_DRAW_SPRITE_LOAD_CALL              ADDR(        0x035dd1,          0x10034031c)
#define RVA_NAME_BOX_SPRITE_CALL               ADDR_DIFFERS(0x041d4e,          0x10034a564)
#define RVA_FILE_BROWSER_FOLDER_ROW            ADDR(        0x040c54,          0x10034b554)
#define RVA_FILE_BROWSER_HOVERED_READ          ADDR_DIFFERS(0x0411fa,          0x10034bb30)

#define RVA_OPEN_GUI_FONT                      ADDR(        0x0535e0,          0x10034c764)
#define RVA_FONT_HANDLES                       ADDR(        0x59daa30,         0x1061a67e8)
#define RVA_PUT_TEXT_IN_CACHE                  ADDR(        0x052680,          0x10034cf00)
#define RVA_RENDER_TEXT_PIXELS                 ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_TTF_RENDER_UTF8_BLENDED            ADDR_DIFFERS(0x33a690,          0x1000cf878)
#define RVA_DRAW_TEXT                          ADDR(        0x02f380,          MAC_INLINED)
#define RVA_RENDER_TEXT                        ADDR(        0x0544e0,          0x10034dc70)
#define RVA_TEXT_COLOUR                        ADDR(        0x430480,          0x100857660)
#define RVA_TEXT_CELL_COLOUR                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RITUAL_GREYED_ALPHA                    0.5f

// ---- Dialogs and messages -----------------------------------------------------------------------
#define RVA_CONFIRM                            ADDR(        0x042b30,          0x1003489b4)
#define RVA_NAME_BOX                           ADDR(        0x041bb0,          0x10034a280)
#define RVA_NAME_BOX_END                       ADDR(        0x041f42,          0x10034a780)
#define NAME_BOX_LENGTH_MAX                    42
#define NAME_BOX_PROMPT_SIZE                   0x1f4
#define RVA_MESSAGE_BOX                        ADDR(        0x041f50,          0x1003499d4)
#define RVA_FILE_BROWSER                       ADDR(        0x03f430,          0x10034a780)
#define RVA_FILE_BROWSER_LAST                  ADDR(        0x43922c,          0x100862700)
#define RVA_FILE_BROWSER_NAMES                 ADDR(        0x593af80,         0x106124539)
#define RVA_FILE_BROWSER_FOLDERS               ADDR(        0x5992f80,         0x106164539)
#define FILE_BROWSER_NAME_SIZE                 0x80
#define FILE_BROWSER_ROWS_MAX                  0x800
#define RVA_HOVERED_ELEMENT                    ADDR(        0x029cd0,          MAC_INLINED)
#define RVA_FIND_HOVERED                       ADDR(        0x02cac0,          0x100347ed4)
#define RVA_ADD_MESSAGE                        ADDR(        0x1e1770,          0x1004c2254)
#define RVA_MESSAGE_TABLE                      ADDR(        0xe4a5460,         0x1085ba830)
#define MESSAGE_SIZE                           0x8a0
#define ADD_MESSAGE_TEXT_SIZE                  0x800
#define MESSAGE_TITLE                          0x6
#define MESSAGE_TEXT                           0x6a
#define RVA_EVENT_MESSAGE_CALL                 ADDR(        0x1c7067,          0x10052c104)

// ---- Game flow ----------------------------------------------------------------------------------
#define RVA_PLAY_GAME                          ADDR(        0x091ad0,          0x100389bfc)
#define RVA_NEW_WORLD                          ADDR(        0x303070,          0x100587298)
#define RVA_MAP_SETTINGS_SCREEN                ADDR(        0x2aa2a0,          0x1004b2790)
#define RVA_PROGRESS_TEXT                      ADDR(        0x0897d0,          0x1004903f8)
#define RVA_PROGRESS_LAST_DRAW                 ADDR(        0x5803378,         0x106cf0180)
#define RVA_FRAME_DELAY_MS                     ADDR(        0x1a53614,         0x104904cac)
#define RVA_RUN_ONE_MOD_PROGRESS_CALL          ADDR_DIFFERS(0x1bb677,          0x10052a614)
#define RVA_STR_CUSTOM_EVENTS                  ADDR(        0x1c61220,         0x100706d2d)
#define RVA_MAP_SCREEN                         ADDR(        0x0947b0,          0x10037b5bc)
#define RVA_LOAD_GAME                          ADDR(        0x185c10,          0x1004eb3d4)
#define RVA_RESET_GAME_STATE                   ADDR(        0x31ca10,          0x100585c0c)
#define RVA_RESET_SQUARES                      ADDR(        0x302de0,          MAC_NOT_LOOKED_UP)
#define RVA_LOAD_MAP                           ADDR(        0x1b5890,          0x100513d90)
#define RVA_INIT_PARTICIPANTS                  ADDR(        0x2ea0b0,          MAC_INLINED)
#define RVA_NETWORK_PARTICIPANTS               ADDR(        0x1ed520,          0x100502e38)
#define RVA_COMMAND_LINE_SWITCH                ADDR(        0x1950d0,          0x100328d08)
#define RVA_SWITCH_DEBUG_LEVEL_ADD             ADDR(        0x18ff77,          0x1003297c0)
#define RVA_CREATE_LOBBY_GAME                  ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_GAME_MENU                          ADDR(        0x1242b0,          MAC_INLINED)
#define RVA_EXIT_GAME                          ADDR(        0x199230,          0x10032b7e4)
#define RVA_GAME_START_PREP                    ADDR(        0x1b0ca0,          0x100491f78)
#define RVA_PLAYER_TURN_START                  ADDR(        0x167a40,          0x100392c00)
#define RVA_AI_MOVE                            ADDR_DIFFERS(0x06f7c0,          0x1004e090c)
#define RVA_AI_COMMANDER_RITUALS               ADDR(        0x06ac10,          0x1004dd078)
#define RVA_AI_PLAN_TRACE_CALL                 ADDR(        0x06bc2f,          0x1004de548)
#define RVA_AI_RITUAL_CANDIDATE_CALL           ADDR(        0x06b927,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_ALL_COMMANDERS             ADDR(        0x0701f0,          0x1004d49ac)
#define RVA_AI_COMMANDER_ROUTINE               ADDR(        0x07a7a0,          0x1004d1b48)
#define RVA_AI_RECRUIT                         ADDR(        0x0771e0,          MAC_INLINED)
#define RVA_AI_RECRUIT_CALL                    ADDR(        0x06f91b,          MAC_INLINED)
#define RVA_AI_CONQUER_STEP                    ADDR(        0x0751f0,          MAC_INLINED)
#define RVA_AI_EXPLORE_STEP                    ADDR(        0x074140,          MAC_INLINED)
#define RVA_AI_EXPLORE_STEP_CALL               ADDR(        0x070b7a,          MAC_INLINED)
#define RVA_AI_FAR_EXPLORE_STEP                ADDR(        0x073c30,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RALLY_STEP                      ADDR(        0x076520,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RALLY_STEP_CALL                 ADDR(        0x070a9a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_MOVE_STEP              ADDR(        0x076110,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_MOVE_STEP_CALL         ADDR(        0x070ae3,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_STEP_CALL               ADDR(        0x070b2c,          MAC_INLINED)
#define RVA_AI_FAR_EXPLORE_STEP_CALL           ADDR(        0x070bc8,          MAC_NOT_LOOKED_UP)
#define RVA_AI_THREAT_NEAR                     ADDR(        0x076e40,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_STRENGTH                 ADDR(        0x129080,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COMMANDER_CAN_MOVE              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_MOVE_ORDER_ROUTE                   ADDR(        0x09e3a0,          MAC_NOT_LOOKED_UP)
#define RVA_PATH_FOG                           ADDR(        0x58355b0,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_NO_TIE_RANDOM                 ADDR(        0x58355b4,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SPREAD                        ADDR(        0xe6c0560,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_X                             ADDR(        0xe75c960,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_Y                             ADDR(        0xe75cd60,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_LENGTH                        ADDR(        0x58355c4,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SQUARE_CACHE_RESET            ADDR(        0x1dc3f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_ALLY_TARGETS_NEAR         ADDR(        0x079810,          MAC_INLINED)
#define RVA_COMMANDER_AI_TARGET_X              ADDR(        0x11c0dce0,        0x10c6368bc)
#define RVA_COMMANDER_AI_TARGET_Y              ADDR(        0x11d379e0,        0x10c63b6c8)
#define RVA_COMMANDER_AI_MOVE_KIND             ADDR(        0x11d32bc0,        0x10859d368)
#define RVA_WAYTOGO                            ADDR_DIFFERS(0x1dafe0,          0x10059bf88)
#define RVA_WAYTOGO2                           ADDR_DIFFERS(0x1df5d0,          0x10059d974)
#define RVA_WAYTOGO2_EXTRA_ARG                 ADDR_DIFFERS(0x1df5d0,          0x10059d974)
#define RVA_WAYTOGO_FAR_SQUARE_CHECK           ADDR(        0x1dc160,          0x10059bbb4)
#define RVA_MAY_ENTER_SQUARE                   ADDR(        0x1df2b0,          0x10059a3ac)
#define RVA_MAY_ENTER_SQUARE_CORE              ADDR(        0x1dd050,          0x100599f4c)
#define RVA_RANDOM_PORTAL_EXIT                 ADDR(        0x2552f0,          0x10057169c)
#define RVA_ARMY_MOVE_FLAGS                    ADDR(        0x329320,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_UNIT_FLAG_42_CHECK            ADDR(        0x325930,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_IS_SEA                      ADDR(        0x2ade40,          MAC_NOT_LOOKED_UP)
#define RVA_AI_STATIONARY_CALL                 ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_AI_EXPLORE_GATE_CMP                ADDR(        0x070b5f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FAR_EXPLORE_GATE_CMP            ADDR(        0x070bad,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_ROUTE_CALL                 ADDR(        0x070d64,          MAC_NOT_LOOKED_UP)
#define RVA_AI_DANGEROUS_STEP_CALL             ADDR(        0x070e81,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_STEP_CALL                  ADDR(        0x070f4f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GOLDRALLY_STOP_CALL             ADDR(        0x071417,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RALLY_ALLY_COUNT_CALL           ADDR(        0x076d0e,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_PLAN_BONUS_CALL        ADDR(        0x076354,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_THREAT_CALL            ADDR(        0x076386,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_STRENGTH_CALL           ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_PLAN_BONUS_CALL         ADDR(        0x0758b6,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_KILLER_ROUTE_CALL       ADDR(        0x075d2f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_ALLY_COUNT_CALL         ADDR(        0x075e08,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_THREAT_CUTOFF           ADDR(        0x075e2b,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_THREAT_CALL             ADDR(        0x075e40,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_ROUTE_CALL              ADDR(        0x075fb2,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_OTHER                  ADDR(        0x5be3dbc,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_AI                     ADDR(        0x6109018,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_HUMAN                  ADDR(        0x610901c,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_STRENGTH_CALL                ADDR(        0x1dc382,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_EXEMPT_X                     ADDR(        0x1aa8474,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_EXEMPT_Y                     ADDR(        0x1aa7e44,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_NO_SEA                        ADDR(        0x58355c0,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_NO_SKY                        ADDR(        0x58355c8,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SPREAD_SQUARE                 ADDR(        0x1dbc80,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_MOVE_COST                   ADDR(        0x1dc440,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_IS_SKY                      ADDR(        0x2ab8e0,          MAC_NOT_LOOKED_UP)
#define RVA_PLANE_SAME_SPOT                    ADDR(        0x254200,          MAC_NOT_LOOKED_UP)
#define RVA_MOVE_STEP_BEFORE                   ADDR(        0x1df230,          MAC_NOT_LOOKED_UP)
#define RVA_MOVE_STEP_AFTER                    ADDR(        0x1df3d0,          MAC_NOT_LOOKED_UP)
#define RVA_OVERRUN_CHECK                      ADDR(        0x1dcdb0,          MAC_NOT_LOOKED_UP)
#define RVA_SPEND_ALL_AP_AT                    ADDR(        0x105320,          MAC_NOT_LOOKED_UP)
#define RVA_SHIPMOVE_SPEND_AP                  ADDR(        0x105590,          MAC_NOT_LOOKED_UP)
#define RVA_STEP_TARGET_X                      ADDR(        0x1aa9b7c,         MAC_NOT_LOOKED_UP)
#define RVA_STEP_TARGET_Y                      ADDR(        0x1aa80ac,         MAC_NOT_LOOKED_UP)
#define RVA_STEP_TARGET_WATCHED                ADDR(        0x58355bc,         MAC_NOT_LOOKED_UP)
#define RVA_ARMY_ABILITY_TOTAL                 ADDR(        0x12be80,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PROGRESS_DRAW                   ADDR(        0x07dd80,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_CITADELS                  ADDR(        0x077120,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CITADEL_COUNT                   ADDR(        0x5a5c0bc,         MAC_NOT_LOOKED_UP)
#define RVA_AI_ATTACK_DIV_OTHER                ADDR(        0x1aa7cf4,         MAC_NOT_LOOKED_UP)
#define RVA_AI_ATTACK_DIV_AI                   ADDR(        0x1aa75a4,         MAC_NOT_LOOKED_UP)
#define RVA_AI_ATTACK_DIV_HUMAN                ADDR(        0x1aa6674,         MAC_NOT_LOOKED_UP)
#define RVA_AI_ATTACK_MARGIN                   ADDR(        0x1aa7c74,         MAC_NOT_LOOKED_UP)
#define RVA_AI_SET_ATTACK_DIVS_COPY            ADDR(        0x06fa10,          MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY                 ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY_SCAN            ADDR(        0x07c3d0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_HANDS                     ADDR(        0x5802f18,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_HERBS                     ADDR(        0x5802f24,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_FUNGUS                    ADDR(        0x580331c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_SACRIFICES                ADDR(        0x5802f1c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_WEED                      ADDR(        0x5803318,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_GEMS                      ADDR(        0x5802f14,         MAC_NOT_LOOKED_UP)
#define RVA_AI_IRON_FLAG                       ADDR(        0x5802f28,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_LIFEFORCE                 ADDR(        0x5802f2c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_SAVE_RESOURCE                   ADDR(        0x61285d0,         MAC_NOT_LOOKED_UP)
#define RVA_AI_UPDATE_STRENGTH_MEMORY          ADDR(        0x06d660,          MAC_NOT_LOOKED_UP)
#define RVA_AI_REMEMBERED_STRENGTH             ADDR(        0x06d5f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_DANGEROUS_STEP                  ADDR(        0x079590,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_TROOPS                   ADDR(        0x06ebc0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_TROOPS_CALL              ADDR(        0x070cf3,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_LEAVE_UNLINK             ADDR(        0x06f16a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_DROP_UNLINK              ADDR(        0x06f512,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_COMSTR_CALL                ADDR(        0x070cfd,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_NO_GATHER               ADDR(        0x0b1070,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_FLAGGED_STRENGTH         ADDR(        0x129080,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_LEADER_OF                     ADDR(        0x32a580,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SQUARE                        ADDR(        0x329af0,          MAC_NOT_LOOKED_UP)
#define UNIT_AI_SAILOR                         0x200000000000ULL
#define UNIT_AI_GUARD                          0x8000000000ULL
#define RVA_AI_SPLIT_ARMY                      ADDR(        0x07b4f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_ARMY_CALL                 ADDR(        0x07b34f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_MIN_UNITS                 ADDR(        WIN_IN_CODE,       MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_MIN_UNITS_TRACE           ADDR(        0x07b61a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_MIN_UNITS_TEST            ADDR(        0x07b62a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_GUARDS_AT                  ADDR(        0x06e9c0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_DEFENDERS                  ADDR(        0x06fc80,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RANDOM_ENEMY                    ADDR(        0x07b3d0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_MOVE_START                 ADDR(        0x06dc00,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_MOVE_END                   ADDR(        0x06d840,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_TEXT                       ADDR(        0x06ab60,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_TROOPS                    ADDR(        0x079410,          MAC_NOT_LOOKED_UP)
#define RVA_AI_UNIT_STAYS                      ADDR(        0x07da10,          MAC_NOT_LOOKED_UP)
#define RVA_AI_REFORMS_HERE                    ADDR(        0x07cf40,          MAC_NOT_LOOKED_UP)
#define RVA_AI_UNIT_JOIN_KIND                  ADDR(        0x07c810,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PICK_UP_LONERS                  ADDR(        0x07bc50,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PICK_UP_LONER_CALL              ADDR(        0x07c05d,          MAC_NOT_LOOKED_UP)
#define RVA_AI_UNIT_MAY_GUARD                  ADDR(        0x07a240,          MAC_NOT_LOOKED_UP)
#define RVA_AI_BUY_COMMANDER                   ADDR(        0x079f00,          MAC_NOT_LOOKED_UP)
#define RVA_AI_BUY_TROOPS                      ADDR(        0x079a20,          MAC_NOT_LOOKED_UP)
#define RVA_AI_BUY_UPGRADE_UNLINK              ADDR(        0x079c75,          MAC_NOT_LOOKED_UP)
#define RVA_RESET_INCOME_CACHES                ADDR(        0x3292c0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_ALL_CALL                   ADDR(        0x06f929,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_DEFENDERS_CALL             ADDR(        0x06f930,          MAC_NOT_LOOKED_UP)
#define RVA_AI_TRADE_CHOICE_CALL               ADDR(        0x06f93e,          MAC_NOT_LOOKED_UP)
#define RVA_INT16_FILL                         ADDR(        0x058e20,          MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY_CALL            ADDR(        0x0703f3,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_PICK_UNIT                  ADDR(        0x1b30b0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUALS_TAINT_TEST              ADDR(        0x06ad12,          MAC_NOT_LOOKED_UP)
#define RVA_AI_APPRSPAM_EXECUTE_CALL           ADDR(        0x06b046,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_GATE_ROLL                  ADDR(        0x06b127,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_CANNOT_CAST          ADDR(        0x06bc5a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_ACCEPT_STORE               ADDR(        0x06bc80,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_POINTLESS            ADDR(        0x06bc5a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_AI_BAD               ADDR(        0x06bc5a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_ANYWHERE_CALL              ADDR(        0x06bcd0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_NOWHERE              ADDR(        0x06bc5a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_TARGET_CALL                ADDR(        0x06be3e,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_PAY_CALL                   ADDR(        0x06bfd4,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SAVE_RESOURCE_STORE             ADDR(        0x06c07c,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_EXECUTE_CALL               ADDR(        0x06c0cf,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_DONE_STORE                 ADDR(        0x06c0f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ALWAYS_EXECUTE_CALL             ADDR(        0x06c241,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUALS_PICK_UP_CALL            ADDR(        0x06c275,          MAC_NOT_LOOKED_UP)
#define RVA_AI_NEAREST_TREQ_ON_PLANE           ADDR(        0x06cc90,          MAC_NOT_LOOKED_UP)
#define RVA_AI_NEAREST_TREQ                    ADDR(        0x06cae0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_BEST_SPAWNBOOSTER_AT            ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TURN_GATE_LATE          ADDR(        0x077268,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_SAVE_CAP                ADDR(        0x0772b1,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TURN_GATE_SHIPS         ADDR(        0x077311,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_FIRST_OFFER_TEST        ADDR(        0x0774d7,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_IMPORTANT_TRY_GATE      ADDR(        0x07789d,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_MELEE_TRY_GATE          ADDR(        0x077a1d,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TROOP_GOLD_GATE         ADDR(        0x077a9f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_COM_GOLD_GATE           ADDR(        0x077b04,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_COM_ESCORT_GATE         ADDR(        0x077b36,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_SITE_DRAWS              ADDR(        0x077d4c,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_ESCORT_STRENGTH         ADDR(        0x077ebb,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TRIES                   ADDR(        0x0780f1,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PORT_FREE_BOAT_PLACES           ADDR(        0x07d5c0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COMMANDER_BOAT_PLACES           ADDR(        0x072ab0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPEC_MESSAGES_OFF               ADDR(        0x07a932,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATEWAY_CALL                    ADDR(        0x07aace,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ITEM_USE_RATING                 ADDR(        0x183b80,          MAC_NOT_LOOKED_UP)
#define MONSTER_ABILITY_AIHOLD                 0x34
#define MONSTER_ABILITY_AIPOWCOM1              0x66
#define MONSTER_ABILITY_AIMAXSHOP              0x90
#define MONSTER_ABILITY_SHIP                   0x1cf
#define RVA_AI_COUNT_PLANNERS                  ADDR(        0x079940,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FREE_SPAWNBOOSTERS              ADDR(        0x078da0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_SPAWNBOOSTER               ADDR(        0x078180,          MAC_NOT_LOOKED_UP)
#define RVA_AI_TRADE_CHOICE                    ADDR(        0x06fee0,          MAC_NOT_LOOKED_UP)
#define RVA_TRADE_INCOME_APPLY                 ADDR(        0x1494f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SQUARE_BONUS               ADDR(        0x076fb0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ITEM_VALUE                      ADDR(        0x0736f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_REDISTRIBUTE_ITEMS              ADDR(        0x073390,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SHIPS                           ADDR(        0x071540,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_AI_BAD                   ADDR(        0x06ce60,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_TARGET                   ADDR(        0x06c2e0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_BAD_TIME                 ADDR(        0x06d080,          MAC_NOT_LOOKED_UP)
#define RVA_AI_EXPLORE_TERRAIN_SKIP            ADDR(        0x075080,          MAC_NOT_LOOKED_UP)
#define RVA_AI_TARGET_IMPOSSIBLE               ADDR(        0x07cd30,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_ARMY_STRENGTH               ADDR(        0x0f90d0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STRENGTH                      ADDR(        0x0f9220,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STRENGTH_CORE                 ADDR(        0x0bc160,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_ARMOR                         ADDR(        0x3214d0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_ICEPROT_TEST                ADDR(        0x1b1a70,          MAC_NOT_LOOKED_UP)
#define RVA_PATH_STRENGTH_WEIGHT               ADDR(        0x0bd120,          MAC_NOT_LOOKED_UP)
#define RVA_INT_MIN                            ADDR(        0x057500,          MAC_NOT_LOOKED_UP)
#define RVA_INT_MAX                            ADDR(        0x05c6e0,          MAC_NOT_LOOKED_UP)
#define RVA_STRONGEST_PLAYER                   ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_STRONGEST_PLAYERS                  ADDR(        0x0f49d0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TOP_UNIT                    ADDR(        0x1b1e20,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_STRENGTH                    ADDR(        0x0f9580,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STRENGTH_FOR                  ADDR(        0x0bc070,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_COUNTS_AS_MONSTER             ADDR(        0x1d93d0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SIDE_STRENGTH               ADDR(        0x0f9250,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_MAY_ACT                  ADDR(        0x1df770,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_RITPOW                   ADDR(        0x2c40e0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_LED_COUNT                ADDR(        0x128ab0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_RANGED                    ADDR(        0x3226c0,          MAC_NOT_LOOKED_UP)
#define RVA_MONSTER_HAS_RANGED                 ADDR(        0x327270,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_RETURNS                       ADDR(        0x326940,          MAC_NOT_LOOKED_UP)
#define RVA_SHOP_OFFER_AVAILABLE               ADDR(        0x128890,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_UPGRADE_UNIT                  ADDR(        0x12bc80,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GOLD_INCOME                 ADDR(        0x334f90,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_IRON_INCOME                 ADDR(        0x32d770,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HERBS_INCOME                ADDR(        0x32dbc0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_FUNGUS_INCOME               ADDR(        0x32d2a0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_SACRIFICES_INCOME           ADDR(        0x10a950,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HANDS_INCOME                ADDR(        0x32e190,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_WEED_INCOME                 ADDR(        0x32da40,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GEM_INCOME                  ADDR(        0x32d890,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TRADE_INCOME                ADDR(        0x32e7c0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_RELICS_INCOME               ADDR(        0x32d650,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_LIFEFORCE_INCOME            ADDR(        0x32e8e0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_RELICS              ADDR(        0x0b0820,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_INCOME                      ADDR(        0x32f640,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_GOLD_INCOME                 ADDR(        0x3358b0,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_BASE_INCOME                ADDR(        0x330fc0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_LIFEFORCE_AT                ADDR(        0x334d90,          MAC_NOT_LOOKED_UP)
#define RVA_LIFEFORCE_DRAIN_SQUARE             ADDR(        0x32e2b0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_INCOME_CACHE                ADDR(        0x11c1bacc,        MAC_NOT_LOOKED_UP)
#define RVA_FREE_TROOPS_STRENGTH               ADDR(        0x128f10,          MAC_NOT_LOOKED_UP)
#define RVA_FREE_TROOPS_COUNT                  ADDR(        0x5822f14,         MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_FORT_LEVEL                  ADDR(        0x129320,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_HAS_FORT                    ADDR(        0x129930,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_ABILITY                     ADDR(        0x0b0a40,          MAC_NOT_LOOKED_UP)
#define RVA_SHOP_PROCESS_OFFER                 ADDR(        0x0aebc0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_LEVEL_SHIFT_ALL                 ADDR(        0x2e9e80,          MAC_NOT_LOOKED_UP)
#define RVA_SHARE_TEAM_SIGHT                   ADDR(        0x1af840,          MAC_NOT_LOOKED_UP)
#define RVA_ILLUSION_TERRAIN                   ADDR(        0x1b3270,          MAC_NOT_LOOKED_UP)
#define RVA_JOIN_COMMANDER                     ADDR(        0x129830,          MAC_NOT_LOOKED_UP)
#define RVA_RESOURCE_STOCK                     ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_ORDER_BURN_FOREST                  ADDR(        0x2e1780,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_CHOP_FOREST                  ADDR(        0x2e1900,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_SLAVE_HUNT                   ADDR(        0x2e3480,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_EAT_VILLAGE                  ADDR(        0x2e1af0,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST                       ADDR(        0x2e1fa0,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS                       ADDR(        0x2e3020,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS_CALL_AI               ADDR(        0x07b1bb,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS_CALL_MAP              ADDR(        0x09e8a6,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS_CALL_NO_CALLER        ADDR(        0x09e8a6,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_ORDER_NO_CALLER             ADDR(        0x09e560,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_SCREEN           ADDR(        0x2d53d9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_CRUSADE          ADDR(        0x2d53c9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_MAP              ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_NO_CALLER        ADDR(        0x09e8b9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALLS_AI              ADDR(        0x07b1dd,          MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SWITCH_BOUND              ADDR(        0x2e236e,          MAC_NOT_LOOKED_UP)
#define CEREMONY_LAST                          10
#define CEREMONY_CONVERT                       1
#define CEREMONY_BREAK_SEAL                    2
#define CEREMONY_BOLSTER_DEVOTION              3
#define CEREMONY_START_CRUSADE                 4
#define CEREMONY_CALL_INQUISITION              5
#define CEREMONY_MASTERY_ARCH_BISHOP           7
#define CEREMONY_MASTERY_CARDINAL              8
#define CEREMONY_MASTERY_PONTIFF               9
#define CEREMONY_INCREASE_FAVOR                10
#define RVA_CEREMONY_COSTS                     ADDR(        WIN_STORED_DIFFERENTLY,MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_BOLSTER_STEP         ADDR(        0x11f979ec,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_SEAL_STEP            ADDR(        0x11f48f18,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_FAVOR_STEP           ADDR(        0x11f49360,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_PONTIFF              ADDR(        0x11f979e8,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_CARDINAL             ADDR(        0x11f48e80,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_ARCH_BISHOP          ADDR(        0x11f48e44,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_INQUISITION          ADDR(        0x11f4935c,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_CRUSADE              ADDR(        0x11f497bc,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_BOLSTER_BASE         ADDR(        0x11d379cc,        MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_SEAL_BASE            ADDR(        0xfb31aa0,         MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_COST_CONVERT              ADDR(        0x11f49358,        MAC_NOT_LOOKED_UP)
#define CEREMONY_COSTS_COUNT                   11
#define CEREMONY_COST_BOLSTER_STEP_AT          0x00
#define CEREMONY_COST_SEAL_STEP_AT             0x04
#define CEREMONY_COST_FAVOR_STEP_AT            0x08
#define CEREMONY_COST_PONTIFF_AT               0x0c
#define CEREMONY_COST_CARDINAL_AT              0x10
#define CEREMONY_COST_ARCH_BISHOP_AT           0x14
#define CEREMONY_COST_INQUISITION_AT           0x18
#define CEREMONY_COST_CRUSADE_AT               0x1c
#define CEREMONY_COST_BOLSTER_BASE_AT          0x20
#define CEREMONY_COST_SEAL_BASE_AT             0x24
#define CEREMONY_COST_CONVERT_AT               0x28
#define RVA_STR_CEREMONY_CONVERT               ADDR(        0x5698540,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_BREAK_SEAL            ADDR(        0x56985e0,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_BOLSTER               ADDR(        0x5698650,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_CRUSADE               ADDR(        0x56986b8,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_INQUISITION           ADDR(        0x5698728,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_MASTERY               ADDR(        0x5698748,         MAC_NOT_LOOKED_UP)
#define RVA_STR_CEREMONY_FAVOR                 ADDR(        0x56985f0,         MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SCREEN                    ADDR(        0x2d3740,          MAC_NOT_LOOKED_UP)
#define RVA_RITUALS_OR_CEREMONIES              ADDR(        0x2e0df0,          MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SCREEN_COSTS              ADDR(        0x2d3bb1,          MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SCREEN_COSTS_END          ADDR(        0x2d3bb6,          MAC_NOT_LOOKED_UP)
#define CEREMONY_SCREEN_COSTS_SIZE             0x5
#define RVA_FAIL_IF_FLAG                       ADDR(        0x1993b0,          MAC_NOT_LOOKED_UP)
#define RVA_SEALS_BROKEN                       ADDR(        0xf07afc8,         MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENTS_DONE                   ADDR(        0xf07afcc,         MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_STEP                    ADDR(        0x164ee4,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_SWITCH                  ADDR(        0x164f03,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_THIRD                   ADDR(        0x164f0c,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_NOT_THIRD               ADDR(        0x164fbd,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_NONE                    ADDR(        0x1656a8,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_EVENT_END                     ADDR(        0x1656af,          MAC_NOT_LOOKED_UP)
#define RVA_APOCALYPSE_MONTHS                  ADDR(        0xf07afd4,         MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_FREESPAWN                   ADDR(        0x2db890,          MAC_NOT_LOOKED_UP)
#define RVA_BOLSTER_BLESSING                   ADDR(        0x2d5820,          MAC_NOT_LOOKED_UP)
#define RVA_NET_SEND_ORDER                     ADDR(        0x1f2100,          MAC_NOT_LOOKED_UP)
#define RVA_NET_SEND_BLESSINGS                 ADDR(        0x1e7150,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_BREAK_CEREMONY_CALL           ADDR(        0x2e2502,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_BREAK_ITEM_CALL               ADDR(        0x184ea2,          MAC_NOT_LOOKED_UP)
#define RVA_USE_SPECIAL_SITE                   ADDR(        0x0b18c0,          MAC_NOT_LOOKED_UP)
#define RVA_DESTROY_GATEWAY                    ADDR(        0x255130,          MAC_NOT_LOOKED_UP)
#define RVA_ALTER_TERRAIN                      ADDR(        0x1f7720,          MAC_NOT_LOOKED_UP)
#define RVA_SITE_VOID_INSANITY                 ADDR(        0x2df100,          MAC_NOT_LOOKED_UP)
#define RVA_REVEAL_DISC                        ADDR(        0x2dd630,          MAC_NOT_LOOKED_UP)
#define RVA_RUIN_SQUARE                        ADDR(        0x2e0900,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_BURN_VALUE                    ADDR(        0x2e04f0,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_CHOP_SELECTED_CALL           ADDR(        0x2e1947,          MAC_NOT_LOOKED_UP)
#define RVA_MAP_INSPECTED_X                    ADDR(        0xfb31aa4,         MAC_NOT_LOOKED_UP)
#define RVA_MAP_INSPECTED_Y                    ADDR(        0x11d379d4,        MAC_NOT_LOOKED_UP)
#define RVA_UNIT_REPICK_MEMORIZED              ADDR(        0x2e72b0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_MOVES                 ADDR(        0x11c08ec0,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_FROM_X                ADDR(        0xfb2cc40,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_FROM_Y                ADDR(        0xfb368e0,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_LAST_TARGET_X         ADDR(        0xfb89900,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_LAST_TARGET_Y         ADDR(        0xfb31ac0,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TRADE_TYPE                  ADDR(        0x11c12b34,        MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TRADE_MODE                  ADDR(        0x11c12b36,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AT                       ADDR(        0x1291a0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_HAS_SEEN_UNIT               ADDR(        0x128e40,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_USE_ITEMS                ADDR(        0x185450,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_AI_SKIP                       ADDR(        0x1d9b70,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_POINTLESS                   ADDR(        0x2cbb10,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_CASTABLE_ANYWHERE           ADDR(        0x2cb8f0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_CASTER_BONUS             ADDR(        0x128860,          MAC_NOT_LOOKED_UP)
#define RVA_SEA_AREA_SIZE                      ADDR(        0x2aded0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PROGRESS_MODE                   ADDR(        0x43c8c4,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_REFUSE_PLAYER                ADDR(        0x43c394,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_REFUSE_CLASS                 ADDR(        0x43c2dc,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_MAX_SEAS                     ADDR(        0x582f7ec,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_MAX_SKIES                    ADDR(        0x582f7e8,         MAC_NOT_LOOKED_UP)
#define RVA_MOVE_COMMANDER_STEP                ADDR(        0x1df8c0,          MAC_NOT_LOOKED_UP)
#define RVA_CLEAR_SQUARE_MARKS                 ADDR(        0x1b3870,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_RESISTANCE_1D1280             ADDR(        0x329ef0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_FIRE_RESISTANCE               RVA_UNIT_RESISTANCE_1D1280
#define RVA_UNIT_ACID_RESISTANCE               ADDR(        0x3209c0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_COLD_RESISTANCE               ADDR(        0x32a210,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SHOCK_RESISTANCE              ADDR(        0x329b20,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_POISON_RESISTANCE             ADDR(        0x326620,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STATE_FLAGS                   ADDR(        0x32a5e0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_AFFLICTION_LIST               ADDR(        0x1d3ea0,          MAC_NOT_LOOKED_UP)
#define UNIT_FEEBLEMINDED                      (1ULL << 25)
#define RVA_UNIT_AFFLICTION_LINE               ADDR(        0x1d1200,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AP_LEFT                  ADDR(        0x328eb0,          MAC_NOT_LOOKED_UP)
#define RVA_RESOURCE_INCOME                    ADDR(        0x3298e0,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FX_FIRST                    ADDR(        0x2c5990,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FX_FIRST_OR                 ADDR(        0x2cbb00,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_UNITS_NAMED                  ADDR(        0x1299f0,          0x10047d458)
#define RVA_COUNT_MONSTER_AT                   ADDR(        0x12c780,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_ABILITY_TOTAL               ADDR(        0x1b38d0,          MAC_NOT_LOOKED_UP)
#define RVA_RELEASE_FOLLOWERS                  ADDR(        0x129030,          MAC_NOT_LOOKED_UP)
#define RVA_CHECK_FOLLOWER_LINKS               ADDR(        0x111190,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_LINK_CHECK_CALL             ADDR(        0x2c9bad,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_UNFOLLOW_TARGET             ADDR(        0x2c8d36,          MAC_NOT_LOOKED_UP)
#define RVA_TELEPORT_ARMY                      ADDR(        0x2dc360,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_LEADER_UNLINK             ADDR(        0x07bb2d,          MAC_NOT_LOOKED_UP)
#define RVA_TRANSFER_UNIT_TO_COMMANDER         ADDR(        0x111670,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MAY_JOIN_COMMANDER            ADDR(        0x111750,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MAY_LEAVE_LEADER              ADDR(        0x111820,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SPEED_CLASS                   ADDR(        0x325af0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_COMMANDS_MENU                 ADDR(        0x111ba0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_COMMANDS_MENU_CALL            ADDR(        0x1219ee,          MAC_NOT_LOOKED_UP)
#define RVA_TRANSFER_UNITS_IN_RECT             ADDR(        0x111330,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_GRID_UNIT_AT                  ADDR(        0x0e6290,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_UNIT_GRID_CELL                ADDR(        0x0e5ff0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_TRANSFER_CALL           ADDR(        0x121f91,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_FOLLOWERS_CALL          ADDR(        0x121fc1,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_NEW_ID                  ADDR(        0x121ff0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_RELEASE                 ADDR(        0x12201e,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_RECT_RELEASE                  ADDR(        0x1115c9,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_TAKE_ALL_CALL                 ADDR(        0x120bc3,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_DROP_STATIONARY               ADDR(        0x122191,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_SELF_LINK_CLEAR               ADDR(        0x1221c5,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_EXIT_FLUSH_ALL                ADDR(        0x1221d6,          MAC_NOT_LOOKED_UP)
#define RVA_LED_COUNT_CACHE_READY              ADDR(        0x5822e9c,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_PRICE                    ADDR(        0x0b10c0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_FREE_COMMANDERS              ADDR(        0x129790,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_SPELL_ATTACK              ADDR(        0x1d9af0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_SPELL_WEAPON              ADDR(        0x329a00,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_WATER_MOVE                ADDR(        0x329e70,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_FLIGHT_KIND                   ADDR(        0x32a520,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_PLANE                         ADDR(        0x329ec0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_SIZE                   ADDR(        0x321e50,          0x10056bd8c)
#define RVA_UNIT_BATTLE_SIZE_2                 ADDR(        0x325750,          0x10056bcd8)
#define RVA_MONSTER_BATTLE_SIZE                ADDR(        0x31ee00,          MAC_INLINED)
#define RVA_MONSTER_SPRITE_SIZE                ADDR(        0x0f9b70,          0x10057d4f4)
#define RVA_UNIT_BOAT_SIZE                     ADDR(        0x3272d0,          0x10056a480)
#define RVA_UNIT_AFFLICTION_COUNT              ADDR(        0x0f6f50,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_REMOVE_ITEM                   ADDR(        0x1853b0,          MAC_NOT_LOOKED_UP)
#define RVA_ITEM_ABILITY_NTH                   ADDR(        0x17e6b0,          MAC_NOT_LOOKED_UP)
#define RVA_ITEM_ABILITY                       ADDR(        0x185440,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_FLAG_21_TEST               ADDR(        0x31cf90,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEXT                        ADDR(        0x1b3770,          MAC_NOT_LOOKED_UP)
#define RVA_GAME_GLOBALS_INIT                  ADDR(        0x1168b0,          MAC_NOT_LOOKED_UP)
#define RVA_GATEWAY_CHECK                      ADDR(        0x255550,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TURN_END                    ADDR(        0x1e1b40,          0x1004c5704)
#define RVA_MOVE_WANDERING_MONSTERS            ADDR(        0x17b030,          MAC_INLINED)
#define RVA_MOVE_PLAYER_LONERS                 ADDR(        0x167580,          MAC_INLINED)
#define RVA_MOVE_PLAYER_STUPIDS                ADDR(        0x1676a0,          MAC_INLINED)
#define RVA_DO_ALL_FIGHTS                      ADDR(        0x0f4fc0,          0x1003dc52c)
#define RVA_BATTLE_SAVE_OTHERS                 ADDR(        0x18e590,          MAC_INLINED)
#define RVA_RETURN_IMMORTALS                   ADDR(        0x123b50,          0x10046a2ac)
#define RVA_MOVE_INFERNO_PLAYER                ADDR(        0x179e10,          MAC_INLINED)
#define RVA_MOVE_HADES_PLAYER                  ADDR(        0x1788c0,          MAC_INLINED)
#define RVA_MOVE_CELESTIAL_PLAYER              ADDR(        0x17d6d0,          MAC_INLINED)
#define RVA_MOVE_ALL_INDEPENDENTS              ADDR(        0x17acf0,          0x100547e68)
#define RVA_MOVE_INDEPENDENT_ARMY              ADDR(        0x172b30,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_MOVED                        ADDR(        0x11f497e0,        MAC_NOT_LOOKED_UP)
#define RVA_INDEP_LOVED_TERRAIN                ADDR(        0x16e550,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_LIKED_TERRAIN                ADDR(        0x16e110,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_NEAR_PREFERRED               ADDR(        0x16e070,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_HATED_TERRAIN                ADDR(        0x16dee0,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_MOVE_TRY                     ADDR(        0x16d210,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_MOVE_STEP                    ADDR(        0x16d8f0,          MAC_NOT_LOOKED_UP)
#define RVA_SET_HOLDIT                         ADDR(        0x173360,          MAC_NOT_LOOKED_UP)
#define RVA_GOLD_MINE_ATTACK                   ADDR(        0x26cc60,          MAC_NOT_LOOKED_UP)
#define RVA_NEAREST_UNIT_ABILITY_IS            ADDR(        0x1ae7b0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SWIMS                         ADDR(        0x104e10,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_NOLAND                        ADDR(        0x104df0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MINDLESS                      ADDR(        0x1d9c30,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SOUTH                       ADDR(        0x2ad720,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_NORTH                       ADDR(        0x2ad6d0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_WATER_NEAR                   ADDR(        0x32d180,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_LAND_NEAR                    ADDR(        0x32d060,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_RUIN_FORM                  ADDR(        0x32e6e0,          MAC_NOT_LOOKED_UP)
#define SQUARE_FLAG_HOLDIT                     0x10
#define UNIT_FLAG2_HOLDIT                      41
#define RVA_SQUARE_ARMY_STATIONARY             ADDR(        0x1126b0,          MAC_NOT_LOOKED_UP)
#define RVA_WANDERER_PICK_TARGET               ADDR(        0x169fd0,          MAC_NOT_LOOKED_UP)
#define RVA_WANDERER_AT                        ADDR(        0x177a80,          MAC_NOT_LOOKED_UP)
#define RVA_WANDERER_PICK_UP                   ADDR(        0x147a50,          MAC_NOT_LOOKED_UP)
#define RVA_LONER_STEP                         ADDR(        0x1733e0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_UNITS_FLAGGED_AT             ADDR(        0x118050,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_ALL_STRENGTH                ADDR(        0x0bc050,          MAC_NOT_LOOKED_UP)
#define RVA_REMOVE_COMMANDER_ARMY              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_IMMORTAL_REFORM_PLACE              ADDR(        0x327300,          MAC_NOT_LOOKED_UP)
#define RVA_NEW_COMMANDER_ID                   ADDR(        0x119530,          MAC_NOT_LOOKED_UP)
#define RVA_INFERNO_ACTIVE                     ADDR(        0x5822f54,         MAC_NOT_LOOKED_UP)
#define RVA_CELESTIAL_ACTIVE                   ADDR(        0x5822f58,         MAC_NOT_LOOKED_UP)
#define RVA_PRIMAL_PORTALS_OPEN                ADDR(        0x5822f50,         MAC_NOT_LOOKED_UP)
#define RVA_GAME_STATISTICS                    ADDR(        0x31d590,          0x10053bd50)
#define RVA_AFTER_GAME_TURN                    ADDR(        0x161bc0,          0x1003a0c44)
#define RVA_MONTH_SQUARE_RESET                 ADDR(        0x1409c0,          MAC_NOT_LOOKED_UP)
#define RVA_MONTH_SQUARE_RESET_CALL            ADDR(        0x166abd,          MAC_NOT_LOOKED_UP)
#define RVA_MAGIC_TAINTED_MONTHS               ADDR(        0x5822f40,         MAC_NOT_LOOKED_UP)
#define RVA_DROWN_CHECK                        ADDR(        0x1146b0,          MAC_INLINED)
#define RVA_CRASH_LANDING_CHECK                ADDR(        0x114070,          MAC_INLINED)
#define RVA_TURN_UNIT_UPKEEP                   ADDR(        0x143eb0,          0x100399fbc)
#define RVA_SQUARE_UPKEEP                      ADDR(        0x1dd860,          0x100599200)
#define RVA_BATTLE_SETUP                       ADDR(        0x0f7110,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SETUP_CALL                  ADDR(        0x0f5c25,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DEPLOY                      ADDR(        0x0f88a0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DEPLOY_CALL                 ADDR(        0x0f5c31,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SETUP_SIDE                  ADDR(        0x0e62d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SETUP_AMBUSH                ADDR(        0x0e92d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_BEST_DEPLOY_SQUARE          ADDR(        0x0eabd0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_COLUMN_FREE_ROW             ADDR(        0x0ea4d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_INTRO_SCREEN                ADDR(        0x0e1ce0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SHOWN_TO_SLOT               ADDR(        0x32b1a0,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_ADVANCE_ATT_CALL             ADDR(        0x0f31b1,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_PLAN_CALL                    ADDR(        0x0f335f,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_DOATTACK_CALL                ADDR(        0x0f3877,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_WINNER_CALL                  ADDR(        0x0f3a81,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ADVANCE_UNIT                ADDR(        0x0c7030,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FIGHT_MOVE                  ADDR(        0x0cb8d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_KILLER_MOVE                 ADDR(        0x0cb510,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ADVANCE_RANK                ADDR(        0x0cb360,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FLY_MOVE                    ADDR(        0x0cab20,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PICK_FIXED_TARGET           ADDR(        0x0db3e0,          MAC_NOT_LOOKED_UP)
#define RVA_ADVANCE_FIXED_TARGET_CALL          ADDR(        0x0c86dd,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PATHFIND                    ADDR(        0x0b7f40,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_NEAREST_ENEMY               ADDR(        0x0ce0d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_NEAREST_ENEMY_RING          ADDR(        0x0ce2d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_DISTANCE               ADDR(        0x0dbb60,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_CAN_STAND                   ADDR(        0x0ec4f0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_IN_CONTACT                    ADDR(        0x0c9660,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_LINE_OF_SIGHT               ADDR(        0x0db900,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ENEMY_EXTENT                ADDR(        0x0dbff0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_COUNT_RANK                  ADDR(        0x0dc1c0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_WINNER                      ADDR(        0x0cc760,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_RANDOM_TARGET               ADDR(        0x0b9bc0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_AOE_SCORE                   ADDR(        0x0ba2d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_TARGET_USEFUL               ADDR(        0x0b8bb0,          MAC_NOT_LOOKED_UP)
#define RVA_WEAPON_RANGE_FOR                   ADDR(        0x31eed0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_ATTACK_ROW                    ADDR(        0x0d79f0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_RANK                   ADDR(        0x323a90,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_SLOWNESS               ADDR(        0x3256a0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_FAST                   ADDR(        0x322640,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_INCAPACITATED                 ADDR(        0x0f1300,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MORALE                        ADDR(        0x322830,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_LOCAL_LEADERSHIP            ADDR(        0x0e4f60,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FEAR_AT                     ADDR(        0x0e49b0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FLEE                        ADDR(        0x0e4960,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_REMOVE_FROM_FIELD           ADDR(        0x0e5e10,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_AT                     ADDR(        0x0ee1d0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_AT_2                   ADDR(        0x0ee020,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_POSITION               ADDR(        0x0e60f0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PLACE_UNIT                  ADDR(        0x0e5c10,          0x1003cdc78)
#define RVA_BATTLE_AFTERMATH_RETURN            ADDR(        0x0e2500,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_AFTER_FIGHT_UPKEEP          ADDR(        0x0cf470,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DELAY_FOR_SPEED             ADDR(        0x0ffc50,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_SIDE_UNITS                   ADDR(        0x1172e0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ROUND                       ADDR(        0x11f418f8,        MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DELAY                       ADDR(        0xfb2cc24,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_WIDTH                       ADDR(        0x43e0cc,          0x1048937f0)
#define RVA_BATTLE_HEIGHT                      ADDR(        WIN_IN_CODE,       MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_WINNER_PLAYER               ADDR(        0x43e254,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_STARVE_ROUND                ADDR(        0x43ddd4,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_CHARGE_ROUND                ADDR(        0x43dda4,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_IS_AMBUSH                   ADDR(        0x5804bc8,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_AT_SEA                      ADDR(        0x5804bd4,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PLANE_3_OR_4                ADDR(        0x5804be4,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_IS_SIEGE                    ADDR(        0x5804c14,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_OVER                        ADDR(        0x5804bdc,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_NOT_A_SIEGE                 ADDR(        0x5804c0c,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ATT_RANK_COUNTS             ADDR(        0x64ab870,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DEF_RANK_COUNTS             ADDR(        0x64ab880,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ENEMY_EXTENTS               ADDR(        0x12335880,        MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_GRID                        ADDR(        0x11fb1860,        0x10ca5a178)
#define RVA_BATTLE_SPEED_SETTING               ADDR(        0x1a3b3fc,         MAC_NOT_LOOKED_UP)
#define UNIT_BATTLE_SIDE                       0x110
#define UNIT_ATTACKS_MADE                      0x111
#define UNIT_ATTACK_PLAN                       0x112
#define UNIT_ZAP_ITEM                          0x122
#define UNIT_FIXED_TARGET                      0x150
#define RVA_GAME_OVER                          ADDR(        0x11fc50,          MAC_INLINED)
#define RVA_GAME_OVER_SCREEN                   ADDR(        0x10d110,          0x100473ea0)
#define RVA_GAME_OVER_EFFECT                   ADDR_DIFFERS(0x2d00b0,          0x10051c778)
#define RVA_START_TUNE                         ADDR(        0x1229e0,          0x100474fe8)
#define RVA_SAVE_GAME                          ADDR_DIFFERS(0x18e850,          0x1004f20fc)
#define RVA_AUTOSAVE                           ADDR(        0x0941a0,          MAC_INLINED)
#define RVA_SAVE_DIALOG                        ADDR(        0x18b3a0,          0x1004f4bf0)
#define RVA_LOAD_GAME_SCREEN                   ADDR(        0x18af50,          MAC_INLINED)
#define RVA_STR_LOAD_GAME_TITLE                ADDR(        0x1c58b60,         0x1006fe7b8)
#define RVA_LOAD_GAME_BROWSER_SKIPS            ADDR_DIFFERS(0x1a3b13c,         0x100837fc0)
#define RVA_LOAD_GAME_BROWSER_NAME             ADDR_DIFFERS(0x5822f6c,         0x1005cc753)
#define RVA_STR_LOADED_START_MM                ADDR(        0x1c5c058,         0x10060609b)
#define RVA_LOAD_START_TEST                    ADDR(        0x5835680,         0x11307d7d4)
#define RVA_LOAD_START_MARK                    ADDR_DIFFERS(0x58356b4,         0x106cee4f0)
#define RVA_PLAYER_ELIMINATED                  ADDR(        0x110ee0,          0x100472058)
#define RVA_STR_ELIMINATED                     ADDR(        0x1b297a0,         0x100631a4e)
#define RVA_ELIMINATED_WARN_CALL               ADDR(        0x111062,          0x100472ea0)
#define RVA_STR_PERCENT_S_ELIMINATED           ADDR(        0x440784,          0x1005b6ffd)
#define RVA_WARN_WAIT_CALL                     ADDR(        0x043166,          0x10034896c)
#define RVA_WARN_TIMED_WAIT_CALL               ADDR(        0x043186,          0x100348910)
#define RVA_WARN_CLOSE_MS                      ADDR(        0x57ef20c,         MAC_NOT_FOUND)
#define RVA_MODS_SCREEN                        ADDR(        0x1c9fb0,          MAC_INLINED)
#define RVA_KEYBOARD_SHORTCUTS                 ADDR(        0x12ca30,          0x1005124b8)
#define RVA_ROLL_CLASS                         ADDR(        0x0afa80,          MAC_INLINED)
#define RVA_AUTO_NAME                          ADDR(        0x124ed0,          0x100471854)
#define RVA_AUTO_NAME_CALL_COSTLIST            ADDR(        0x0af986,          0x1003628cc)
#define RVA_AUTO_NAME_CALL_PLAYER              ADDR(        0x0b0565,          0x1003617c0)
#define RVA_AUTO_NAME_CALL_HELPER              ADDR(        0x10ee75,          0x100471f34)
#define RVA_AUTO_NAME_CALL_NEW_COMMANDER       ADDR(        0x1287c9,          0x10046e344)
#define RVA_AUTO_NAME_CALL_NET_SETUP           ADDR(        0x1ee402,          0x1005044b4)
#define RVA_AUTO_NAME_CALL_SETUP               ADDR(        0x304f44,          0x100589b3c)
#define RVA_AUTO_NAME_CALL_SERVER              ADDR(        WIN_NO_CALL,       0x1004fa578)
#define RVA_MONSTER_NAME_TYPE                  ADDR(        0x11f560,          0x1004711f0)
#define RVA_AUTO_NAME_LAST                     ADDR(        0x660ef80,         0x106cd54dc)
#define RVA_AUTO_NAME_COUNTED                  ADDR(        0x5822e94,         0x106cd54d8)
#define AUTO_NAME_TYPES                        62
#define AUTO_NAME_LIST_SIZE                    0x5dc
#define RVA_PLACE_PLAYER                       ADDR(        0x0a8710,          0x10036e384)
#define RVA_ROLL_SUB_SOCIETY                   ADDR(        0x2d0420,          0x100512894)
#define RVA_MONTHLY_UPDATE                     ADDR(        0x198d80,          0x100333ed8)
#define RVA_SQUARE_INFO                        ADDR(        0x331950,          0x1004c9e64)
#define RVA_SQUARE_INFO_ROW_START              ADDR_DIFFERS(0x33478c,          0x1004cd560)
#define RVA_SQUARE_PANEL                       ADDR(        0x0a3740,          MAC_NOT_FOUND)
#define RVA_LOBBY                              ADDR(        0x1e3320,          0x10059f068)
#define RVA_DISCONNECT                         ADDR(        0x1f28e0,          0x1005072ac)
#define RVA_SDL_MAIN                           ADDR(        0x18fca0,          0x10032f518)
#define RVA_WIN_MAIN                           ADDR(        0x001180,          MAC_NOT_FOUND)
#define RVA_WIN_MAIN_SET_MAIN_READY_CALL       ADDR(        0x0012b5,          MAC_NO_CALL)
#define RVA_WIN_MAIN_SDL_MAIN_CALL             ADDR(        0x0012bf,          MAC_NO_CALL)
#define RVA_SET_MAIN_READY_THUNK               ADDR(        0x02434d,          MAC_NO_CALL)
#define RVA_SET_MAIN_READY_IMPORT              ADDR(        0x33a4e0,          MAC_NO_CALL)
#define RVA_STEAM_DLOPEN                       ADDR(        WIN_INLINED,       0x1000101ec)
#define RVA_STEAM_DLSYM                        ADDR(        WIN_INLINED,       0x1000101f8)
#define RVA_GAME_MENU_JUMPS                    ADDR(        WIN_NO_EQUIVALENT, 0x1007588f8)
#define RVA_RESET_MODS                         ADDR(        0x1caa20,          0x100528e48)
#define RVA_BEFORE_GAME_START                  ADDR(        0x0942c0,          0x10038992c)
#define RVA_TOP_BAR                            ADDR(        0x09c280,          0x10037791c)
#define RVA_DATE                               ADDR(        0x118dc0,          MAC_INLINED)
#define RVA_UNIT_SHOWN_NAME                    ADDR(        0x0f8ad0,          0x1003bbd1c)
#define RVA_RECRUIT_SCREEN                     ADDR(        0x125290,          0x100479f3c)
#define RVA_RECRUIT_LOCATION_LAST              ADDR(        0x44193c,          MAC_NOT_LOOKED_UP)
#define RVA_RECRUIT_LOCATION_X                 ADDR(        0x11f48f20,        MAC_NOT_LOOKED_UP)
#define RVA_RECRUIT_LOCATION_Y                 ADDR(        0x11f493a0,        MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_PLAYER            FRAME_OFFSET(0x000098,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_X                 FRAME_OFFSET(0x000094,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_Y                 FRAME_OFFSET(0x000090,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_CLICK_LATCH       FRAME_OFFSET(0x0000d0,          MAC_NOT_LOOKED_UP)
#define RVA_PLANE_SCREEN                       ADDR(        0x1b0010,          0x1004a6ab4)
#define RVA_UNIT_STATS_SCREEN                  ADDR(        0x1d46e0,          0x100521268)
#define RVA_ABILITY_LIST                       ADDR(        0x1d1200,          0x10051c98c)
#define RVA_ABILITY_LOOKUP                     ADDR(        0x0b6fb0,          0x1004ea748)
#define RVA_ADDPLANE                           ADDR(        0x2508d0,          0x10056e0c4)
#define RVA_PLAYER_COLOUR_RPC                  ADDR(        0x3020d0,          0x100586eac)
#define RVA_DIFFICULTY_POPUP                   ADDR(        0x302c40,          0x100586738)
#define RVA_SAVE_LOBBY_GAME                    ADDR(        0x12e820,          0x1004f8458)
#define RVA_SERVER_SCREEN                      ADDR(        0x1f3540,          0x1004f8f44)
#define RVA_SETUP_SERVER                       ADDR(        0x1f1230,          0x1005024d4)
#define RVA_NET_CLIENT_GAME                    ADDR(        0x1e8ee0,          0x1005073e8)
#define RVA_NET_MODVAR                         ADDR(        0x1eac10,          MAC_INLINED)
#define RVA_NEW_WORLD_NET_AFTER                ADDR(        0x1ad6b0,          MAC_INLINED)
#define RVA_OPENDIR                            ADDR(        0x05c080,          0x1005aec2c)

#define RVA_NEW_WORLD_RESET_CALL               ADDR(        0x303137,          0x100587328)
#define RVA_NEW_WORLD_UNIQUE_INIT              ADDR(        0x30313c,          0x100587334)
#define RVA_PARTICIPANTS_RANDOM_COLOURS        ADDR(        0x304a5e,          0x100587be0)
#define RVA_PARTICIPANTS_COLOUR_BUTTON         ADDR(        0x304d34,          0x10058931c)
#define RVA_NEW_WORLD_NET_INIT_CALL            ADDR(        0x303242,          MAC_NO_CALL)
#define RVA_NEW_WORLD_INIT_CALL                ADDR(        0x30354d,          MAC_NO_CALL)
#define RVA_PARTICIPANTS_ROW_START             ADDR_DIFFERS(0x303fb4,          0x100588a48)
#define RVA_PARTICIPANTS_ROW_START_NET         ADDR_DIFFERS(0x303fb4,          0x1005884ec)
#define RVA_PARTICIPANTS_DIFFICULTY_CALL       ADDR(        0x304b70,          0x10058921c)
#define RVA_NEW_WORLD_UNIQUE_CHECKBOX          ADDR_DIFFERS(0x303f9e,          0x100588a28)
#define RVA_NEW_WORLD_CREATING_WORLD_JOIN      ADDR(        0x304d68,          0x100589450)
#define RVA_NEW_WORLD_CLASS_CHECK              ADDR(        0x304d9c,          0x1005895b0)
#define RVA_NEW_WORLD_UNIQUE_READ              ADDR(        0x304d80,          0x100589470)
#define RVA_NEW_WORLD_NAME_CHECK               ADDR(        0x304e38,          0x100589a68)
#define RVA_NEW_WORLD_NAME_BUFFER              ADDR(        0x304e62,          0x100589a64)
#define RVA_NEW_WORLD_NAME_BOX_CALL            ADDR(        0x304e92,          0x100589a8c)
#define RVA_NEW_WORLD_PLACE_PLAYER_CALL        ADDR(        0x305126,          0x10058a0e0)
#define RVA_NEW_WORLD_CREATING_WORLD_2         ADDR(        WIN_NO_EQUIVALENT, 0x10058a7b8)
#define FRAME_NEW_WORLD_UNIQUE_RANDOM          FRAME_OFFSET(0x000064,          0x228)

#define RVA_GAME_START_CALL                    ADDR(        0x091e8f,          0x10038a648)
#define RVA_MONTH_START_CALL                   ADDR(        0x09206c,          0x10038a978)
#define RVA_TURN_START_AI_CALL                 ADDR(        0x09221f,          0x10038af74)
#define RVA_TURN_START_AI_SINGLE_CALL          ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_TURN_START_INDEPENDENT_CALL        ADDR(        0x0924b6,          0x10038ad48)
#define RVA_TURN_START_HUMAN_CALL              ADDR(        0x0925ce,          0x10038abc0)
#define RVA_TURN_START_HUMAN_SINGLE_CALL       ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_AI_TURN_CALL                       ADDR(        0x092251,          0x10038afb0)
#define RVA_HUMAN_TURN_CALL                    ADDR(        0x0927b6,          0x10038b3fc)
#define RVA_TURN_HANDOFF_BOX                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_TURN_HANDOFF_BOX_CALL              ADDR_DIFFERS(0x0926ea,          0x10038b348)
#define RVA_STR_TURN_HANDOFF                   ADDR(        0x1b29bb0,         0x100631da3)
#define RVA_LONE_HUMAN                         ADDR(        0xf07afc0,         0x11307d7b4)
#define RVA_PLAYER_TURN_END_CALL               ADDR(        0x0922d2,          0x10038c358)
#define RVA_GAME_OVER_CALL                     ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_WORLD_SIZE_LOG                     ADDR(        0x092d4a,          0x10039067c)
#define RVA_GAME_OVER_SCREEN_CALL              ADDR(        0x093e1c,          0x100390454)
#define RVA_GAME_OVER_WAIT_CALL                ADDR(        0x10d573,          0x100474650)
#define RVA_LOAD_TURN_END_CALL                 ADDR(        0x186b7e,          0x1004ed628)
#define RVA_EXIT_DISCONNECT_CALL               ADDR(        0x1992ee,          0x10032b8b4)
#define RVA_RESET_EVENT_VARS                   ADDR(        0x31ce90,          0x100586240)
#define RVA_MAP_GAME_MENU_CALL                 ADDR(        0x09753b,          MAC_NO_CALL)
#define RVA_MAP_SQUARE_INFO_CALL               ADDR(        0x097a03,          0x1003846c4)
#define RVA_MAP_SQUARE_PANEL_CALL              ADDR(        0x0a0470,          MAC_NOT_FOUND)
#define RVA_GAME_MENU_REDRAW                   ADDR(        0x124352,          0x100380810)
#define RVA_GAME_MENU_MAP_DESCRIPTION          ADDR(        0x1244c6,          0x100380914)
#define RVA_GAME_MENU_SETTINGS_SECTION_END     ADDR(        0x1245b8,          0x1003809a0)
#define RVA_GAME_MENU_Q_KEY                    ADDR(        0x124848,          0x100380b0c)
#define RVA_GAME_MENU_SAVE_CALL                ADDR(        0x124ce4,          0x100380df0)
#define RVA_GAME_MENU_PREFS_CALL               ADDR(        0x124d04,          0x100380e08)
#define RVA_GAME_MENU_SAVE_QUIT_CALL           ADDR(        0x124d2e,          0x100380e00)
#define RVA_GAME_MENU_RESIGN_SET               ADDR(        0x124e24,          0x1003812c8)
#define RVA_SAVE_DIALOG_SAVE_CALL              ADDR(        0x18b5fc,          0x1004f4d1c)
#define RVA_SAVE_DIALOG_SAVE_FOPEN_CALL        ADDR(        0x18b5ca,          MAC_NO_CALL)
#define RVA_LOBBY_SAVE_CALL                    ADDR(        WIN_NO_CALL,       0x1003b8324)
#define RVA_SAVE_LOBBY_GAME_CALL               ADDR(        0x12e8c6,          0x1004f84e8)
#define RVA_SERVER_SAVE_CALL                   ADDR(        WIN_NO_CALL,       0x1004fa844)
#define RVA_SETUP_SERVER_LOAD_CALL             ADDR(        0x1f167f,          0x100502b48)
#define RVA_NET_CLIENT_LOAD_CALL               ADDR(        0x1ea8a9,          0x10050b86c)
#define RVA_SETUP_SERVER_LOAD_SCREEN_CALL      ADDR(        0x1f16a0,          0x100502b0c)
#define RVA_SETUP_SERVER_PLAY_GAME_CALL        ADDR(        0x1f16de,          0x10050288c)
#define RVA_MAIN_MENU_LOAD_CALL                ADDR(        0x18b066,          0x10032e6a4)
#define RVA_MAIN_MENU_LOADED_SET               ADDR(        0x192630,          0x10032e6b8)
#define RVA_CMDLINE_LOAD_CALL                  ADDR(        0x191371,          0x100333dc4)
#define RVA_CMDLINE_PLAY_GAME_CALL             ADDR(        0x1913b4,          0x100333e54)
#define RVA_BENCH_LOAD_CALL                    ADDR(        0x191165,          0x1003338f4)
#define RVA_BENCH_PLAY_GAME_CALL               ADDR(        0x1911b2,          0x100333a20)
#define RVA_AUTORUN_SAVE_CALL                  ADDR(        0x091f2e,          0x10038a6f8)
#define RVA_AUTOSAVE_SAVE_CALL                 ADDR(        0x09278c,          0x10038b3c4)
#define RVA_AUTOSAVE_FUNCTION_SAVE_CALL        ADDR(        0x09425b,          MAC_INLINED)
#define RVA_AUTORUN_SAVE_FOPEN_CALL            ADDR(        0x091f05,          MAC_NO_CALL)
#define RVA_AUTOSAVE_SAVE_FOPEN_CALL           ADDR(        0x092764,          MAC_NO_CALL)
#define RVA_AUTOSAVE_FUNCTION_SAVE_FOPEN_CALL  ADDR(        0x094231,          MAC_NO_CALL)
#define RVA_NET_CLIENT_MAP_SCREEN_CALL         ADDR(        0x1ea916,          0x10050b8dc)
#define RVA_NET_CLIENT_SQUARE_INFO_CALL        ADDR(        0x1e9480,          MAC_NOT_FOUND)
#define RVA_NET_CLIENT_DATE_CALL               ADDR(        0x1e966e,          MAC_NO_CALL)
#define RVA_TOP_BAR_DATE_CALL                  ADDR(        0x09c57d,          MAC_NO_CALL)

// ---- User folder and startup folders ------------------------------------------------------------
#define RVA_SETUP_DIRS                         ADDR(        0x194520,          MAC_INLINED)
#define RVA_EXPAND_PATH                        ADDR(        0x05b9a0,          0x1003367ac)
#define RVA_MAKE_DIR                           ADDR(        0x059250,          0x100336c14)
#define RVA_GETENV                             ADDR_DIFFERS(0x00ede8,          0x1005ae6bc)
#define RVA_USER_DIR                           ADDR(        0x58261b0,         0x113827830)
#define RVA_USER_DIR_EXPAND_CALL               ADDR_DIFFERS(0x1948ff,          0x10032f900)
#define RVA_EXPAND_PATH_VAR_GETENV_CALL        ADDR(        0x05ba87,          0x10033686c)
#define RVA_EXPAND_PATH_HOME_GETENV_CALL       ADDR(        0x05baca,          0x1003368b4)
#define RVA_COE5_SAVE_GETENV_CALL              ADDR(        0x19490b,          0x10032f90c)
#define RVA_COE5_DATA_GETENV_CALL              ADDR(        0x19461f,          0x10032f79c)
#define RVA_MAPS_DIR                           ADDR(        0x5829e90,         0x10b848b00)
#define RVA_BATMAPS_DIR                        ADDR(        0x58294c0,         0x10b903289)
#define RVA_SAVES_DIR                          ADDR(        0x582a860,         0x11220798d)
#define RVA_GAMELOGS_DIR                       ADDR(        0x5825710,         0x112206fc8)
#define RVA_OLDIPADR_FILE                      ADDR(        0x5827750,         0x1088104d1)
#define RVA_TEMP_DIR                           ADDR(        0x582b230,         0x10880f8d0)
#define RVA_STR_USER_DIR_DEFAULT               ADDR(        0x1c5ade8,         MAC_IN_CODE)
#define RVA_STR_COE5_SAVE                      ADDR(        0x1c5afe8,         0x1006053e7)
#define RVA_STR_COE5_DATA                      ADDR(        0x1c5afb8,         0x100605394)

// ---- Game state ---------------------------------------------------------------------------------
#define RVA_TURN                               ADDR(        0x4438bc,          0x1049c81c0)
#define RVA_ACTIVE_PLAYER                      ADDR(        0x443ad4,          0x105a0ae90)
#define RVA_LOADED_GAME                        ADDR_DIFFERS(0x5822f18,         0x10ce02ee4)
#define RVA_LOADED_MAP_NAME                    ADDR(        0x582bc00,         0x1136f570d)
#define RVA_RESIGN                             ADDR_DIFFERS(0x58294b4,         0x10ce02ef0)
#define RVA_AUTORUN                            ADDR_DIFFERS(0x582811c,         0x10ce0cb58)
#define RVA_SKIP_SETUP_A                       ADDR_DIFFERS(0x5822f78,         0x1136f6108)
#define RVA_SKIP_SETUP_B                       ADDR_DIFFERS(0x582395c,         0x10c399464)
#define RVA_MAP_DESCRIPTION                    ADDR_DIFFERS(0xf06c3c0,         0x11306ebb4)
#define RVA_SIMULTANEOUS                       ADDR(        WIN_IN_CODE,       MAC_IN_CODE)
#define RVA_AUTOSAVE_ON                        ADDR_DIFFERS(0x5828ae8,         0x1069b16a4)
#define RVA_AI_MOVE_DELAY                      ADDR(        0x1a531ac,         MAC_NOT_LOOKED_UP)
#define RVA_WORLD_W                            ADDR(        0xd63e8e8,         0x1121121dc)
#define RVA_WORLD_H                            ADDR(        0xd63e8ec,         0x1121121e0)
#define RVA_SQUARE_INFO_X                      ADDR(        0x1abb0a4,         0x100873504)
#define RVA_SQUARE_INFO_Y                      ADDR(        0x1aba44c,         0x100873508)
#define RVA_POINTER_SQUARE                     ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_PLANES                             ADDR(        0xd63e8c0,         0x1121121b4)
#define PLANE_SIZE                             0x48
#define PLANE_NAME_SIZE                        0x20
#define PLANE_TABLE_MAX                        12
#define PLANE_X                                0x20
#define PLANE_Y                                0x24
#define PLANE_W                                0x28
#define PLANE_H                                0x2c
#define RVA_LAST_PLANE                         ADDR(        0x1adb2e4,         0x1049c81b8)
#define RVA_PLANE_AT                           ADDR(        0x2554e0,          0x10056e04c)
#define PLANE_ELYSIUM2                         10
#define RVA_CREATE_EXTRA_PLANES                ADDR(        0x2a3680,          0x1004b4924)
#define RVA_LOAD_MAP_PLANE                     ADDR(        0x1b5d40,          0x100513c6c)
#define RVA_LOAD_AZTLAN_PLANE                  ADDR(        0x247190,          MAC_INLINED)
#define RVA_LOAD_PRIMAL_PLANE                  ADDR(        0x24b690,          0x100578718)
#define RVA_CREATE_CELESTIAL_PLANE             ADDR(        0x24b0c0,          0x100578ab8)
#define RVA_CREATE_SKY_PLANE                   ADDR(        0x24f1b0,          0x100579c9c)
#define RVA_CREATE_AGARTHA_PLANE               ADDR(        0x24cf60,          0x1005726ec)
#define RVA_CREATE_INFERNO_PLANE               ADDR(        0x24fc70,          0x10056e21c)
#define RVA_CREATE_HADES_PLANE                 ADDR(        0x244a50,          MAC_INLINED)
#define RVA_CREATE_ELEMENTAL_PLANE             ADDR(        0x248930,          0x1005754d8)
#define RVA_MAP_PLANE_INDEX                    ADDR(        0x1a6b99c,         0x100873500)
#define RVA_MAP_FILE_PLANE                     ADDR(        0x582c67c,         0x108810eec)
#define RVA_MAP_FILE_HAS_PLANE                 ADDR(        0x582c640,         0x10ce0cb48)
#define RVA_MAP_PLANE_WORD_CHECK               ADDR(        0x1b6063,          0x1005132fc)
#define RVA_ADDPLANE_CAP_CHECK                 ADDR(        WIN_INLINED,       0x10056e110)
#define RVA_ELYSIUM2_INDEX_CHECK               ADDR(        0x2a3aea,          0x1004b4f3c)
#define RVA_VOID_INDEX_CHECK                   ADDR(        0x2a39d0,          0x1004b4ea8)
#define RVA_VIEW_PLANE                         ADDR(        0x23e2e0,          0x10048f948)
#define RVA_PLANE_CAMERA_X                     ADDR(        0x9fab1d0,         0x106cee504)
#define RVA_PLANE_CAMERA_Y                     ADDR(        0x9fab200,         0x106cee530)
#define RVA_TOP_BAR_PLANE_SLOT_CHECK           ADDR(        0x09d496,          0x100379c28)
#define RVA_PICK_PLANE_SLOT_CHECK              ADDR(        0x1b0182,          MAC_NOT_FOUND)
#define RVA_LONG_PRESS                         ADDR(        0x23e9f0,          MAC_INLINED)
#define SQUARE_AI_STRENGTH                     0xac
#define SQUARE_TRUE_TERRAIN_SEEN               0x83
#define SQUARE_SEEN                            0x43
#define RVA_DATE_TEXT                          ADDR(        0x5822eb0,         0x106cee464)
#define RVA_SEASON_NAMES                       ADDR(        0x344400,          0x100834710)
#define RVA_UNIT_NAME_RING                     ADDR(        0x64a7090,         0x1069b16dc)
#define RVA_STACK_COOKIE                       ADDR_DIFFERS(0x42c000,          0x100774220)
#define RVA_SEASON                             ADDR(        0x44390c,          0x1049c81c4)
#define RVA_SEASON_TURNS_LEFT                  ADDR(        0x44388c,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_NAME_RING_INDEX               ADDR(        0x5804bb4,         0x1069b16d8)
#define RVA_MESSAGE_LAST                       ADDR(        0x1aaa76c,         0x1049c81bc)
#define RVA_SQUARES                            ADDR(        0xa20ac60,         0x10d011c88)
#define RVA_TEST_GAME_FLAG                     ADDR_DIFFERS(0x4438dc,          0x1059bcc88)
#define RVA_TIP_SHOWN                          ADDR_DIFFERS(0x5835718,         0x106cee5ac)
#define RVA_TIP_TEXT                           ADDR(        0x5835758,         0x106cee5ad)

#define RVA_NET_CLIENT                         ADDR_DIFFERS(0x5828ae4,         0x1136f56ec)
#define RVA_NET_SERVER                         ADDR_DIFFERS(0x58294b8,         0x10880c9ec)
#define RVA_NET_LOBBY                          ADDR_DIFFERS(0x5823948,         0x1136f56e8)

// ---- New game settings --------------------------------------------------------------------------
#define RVA_SOCIETY                            ADDR(        0xf07afaa,         0x11307d79e)
#define RVA_COMMON_CAUSE                       ADDR(        0xf07afb8,         0x11307d7ac)
#define RVA_SCORE_GRAPHS                       ADDR(        0xf07afbc,         0x11307d7b0)
#define RVA_BATTLE_REPORTS                     ADDR(        0xfb2cbcc,         0x10ce0cb46)
#define RVA_START_FLAGS                        ADDR(        0xf07afa8,         0x11307d79c)
#define RVA_WILDER                             ADDR(        0xf07afd0,         0x11307d7c4)
#define RVA_MAP_W                              ADDR(        0x582c5cc,         0x11307d7cc)
#define RVA_MAP_H                              ADDR(        0x582c5c8,         0x11307d7d0)
#define RVA_MAP_DEPTH                          ADDR(        0x582c5c4,         0x1121121b0)
#define RVA_NORTH_PART                         ADDR(        0x1a5222c,         0x105a0aecc)
#define RVA_SOUTH_PART                         ADDR(        0x1a46374,         0x105a0aed0)
#define RVA_UNIQUE_RANDOM_DEFAULT              ADDR_DIFFERS(0x5824d3c,         0x10ce0cb40)
#define RVA_TEST_GAME_A                        RVA_MAP_W
#define RVA_TEST_GAME_B                        RVA_MAP_H
#define RVA_CLI_MAP_W                          ADDR(        0x1a407a4,         0x100872c14)
#define RVA_CLI_MAP_H                          ADDR(        0x1a50edc,         0x100872c18)
#define RVA_CLI_SOCIETY                        ADDR(        0x1a3b3bc,         0x100872c20)
#define RVA_START_DEFAULT                      ADDR(        0x1a3b334,         0x100872c1c)
#define RVA_WILDER_DEFAULT                     ADDR_DIFFERS(0x1a4ce7c,         0x107e9827c)
#define RVA_SUBSOC                             ADDR_DIFFERS(0x58261ac,         0x107e98280)
#define RVA_SETUP_NETWORK                      ADDR(        0xf07ae20,         0x11307d614)
#define RVA_CLI_GRAPHS                         ADDR_DIFFERS(0x5822f7c,         0x107e98278)
#define RVA_CLI_COMMON_CAUSE                   ADDR_DIFFERS(0x5823950,         0x106cd54d4)
#define RVA_CLI_NEW_GAME                       ADDR_DIFFERS(0x582433c,         0x108810494)
#define RVA_CLI_MAP_EDITOR                     ADDR_DIFFERS(0x58260d8,         0x105aa6804)
#define RVA_SDL_MAIN_STARTUP_MODS_RUN_CALL     ADDR(        0x1910a9,          0x100332fbc)
#define RVA_CLI_BATTLE_REPORTS                 ADDR_DIFFERS(0x5823958,         0x10ce0cb44)
#define RVA_MAP_SIZE_WIDTHS                    ADDR_DIFFERS(0x406e20,          0x10075c660)
#define RVA_MAP_SIZE_HEIGHTS                   ADDR_DIFFERS(0x406e10,          0x10075c674)
#define RVA_SOCIETY_TABLE                      ADDR(        0x5697b10,         0x100872c90)
#define SOCIETY_TABLE_MAX                      12
#define RVA_MAP_SETTINGS_SUB_SOCIETY_CALL      ADDR(        WIN_NO_CALL,       0x1004b2f24)
#define RVA_MAP_SETTINGS_COMMON_CAUSE_CHECKBOX ADDR(        0x2aac09,          0x1004b2afc)
#define RVA_MAP_SETTINGS_START_FLAGS_CLEAR     ADDR(        0x2ab1aa,          0x1004b3880)
#define RVA_MAP_SETTINGS_WIDTHS_REF            ADDR(        0x2aa330,          0x1004b3ca4)
#define RVA_MAP_SETTINGS_HEIGHTS_REF           ADDR(        0x2aa33d,          0x1004b3d50)
#define RVA_AUTO_NAME_TABLE                    ADDR(        0x344420,          0x10077ecd0)
#define RVA_CLI_CLUSTER_START_STORE            ADDR_DIFFERS(0x19587c,          0x100329994)
#define RVA_CLI_NO_CLUSTER_START_STORE         ADDR(        0x1958ad,          0x1003299a0)
#define RVA_CLI_WEST_EAST_STORE                ADDR_DIFFERS(0x19590b,          0x100329994)
#define RVA_CLI_NORTH_PART_STORE               ADDR(        0x196bcb,          0x10032a084)
#define RVA_CLI_SOUTH_PART_STORE               ADDR(        0x196c0d,          0x10032a0ac)
#define RVA_MAP_SETTINGS_START_READ            ADDR(        0x2aa3b4,          0x1004b283c)
#define RVA_MAP_SETTINGS_NORTH_DEFAULT         ADDR(        0x2aa629,          0x1004b2a74)
#define RVA_MAP_SETTINGS_SOUTH_DEFAULT         ADDR(        0x2aa63f,          0x1004b2a8c)
#define RVA_MAP_SETTINGS_NORTH_STORE           ADDR(        0x2ab033,          0x1004b3270)
#define RVA_MAP_SETTINGS_SOUTH_STORE           ADDR(        0x2ab0b6,          0x1004b330c)
#define RVA_MAP_SETTINGS_WILDER_STORE          ADDR(        0x2ab1e4,          0x1004b38f0)
#define RVA_NUMBER_BOX                         ADDR(        0x042770,          0x100348f74)
#define RVA_SETTINGS_FROM_CLI                  ADDR(        0x1112c0,          MAC_INLINED)
#define RVA_LOAD_GAME_WILDER                   ADDR(        0x18aa5c,          0x1004ebe44)
#define RVA_SAVE_GAME_WILDER                   ADDR(        0x18adc0,          0x1004f26e0)
#define RVA_SETTINGS_BLOCK_LOAD                ADDR(        0x18a7d0,          MAC_NOT_FOUND)
#define RVA_SETTINGS_BLOCK_SAVE                ADDR(        0x18ac10,          MAC_NOT_FOUND)
#define RVA_WORLD_NORTH_PART                   ADDR(        0x5835f54,         0x108810ea4)
#define RVA_WORLD_SOUTH_PART                   ADDR(        0x5835f50,         0x108810ea8)
#define RVA_SOCIETY_PRESETS                    ADDR(        0x2d04d0,          MAC_INLINED)
#define RVA_WORLD_PART_BANDS                   ADDR(        0x2a99c0,          0x1004aba3c)
#define RVA_WORLD_PART_CHECK                   ADDR(        0x2a6644,          0x1004b877c)
#define RVA_WORLD_PART_TERRAIN                 ADDR(        0x2a6640,          MAC_NOT_FOUND)

#define RVA_NEW_WORLD_BATTLE_REPORTS_INIT      ADDR(        0x30314d,          0x100587358)
#define RVA_SAVE_GAME_BATTLE_REPORTS           ADDR(        0x18ec54,          0x1004f251c)
#define RVA_SAVE_GAME_COMMON_CAUSE             ADDR(        0x18ac74,          0x1004f25f0)
#define RVA_SAVE_GAME_SCORE_GRAPHS             ADDR(        0x18acb0,          0x1004f2618)
#define RVA_LOAD_GAME_BATTLE_REPORTS           ADDR(        0x18620a,          0x1004ebc08)
#define RVA_LOAD_GAME_COMMON_CAUSE             ADDR(        0x18a8b4,          0x1004ebd04)
#define RVA_LOAD_GAME_SCORE_GRAPHS             ADDR(        0x18a8f8,          0x1004ebd40)
#define RVA_SAVE_PLAYERS                       ADDR(        0x187d10,          MAC_INLINED)
#define RVA_SAVE_PLAYERS_AI_LEVEL              ADDR(        0x187f17,          0x1004f2e84)
#define RVA_LOAD_PLAYERS_AI_LEVEL              ADDR(        0x1879d7,          0x1004ecc44)
#define RVA_TURN_START_AI_LEVEL                ADDR(        0x167b83,          0x100392ee8)
#define RVA_AI_LEVEL_BONUS                     ADDR(        0x344290,          0x10075c6a8)
#define RVA_ADD_RESOURCE                       ADDR(        0x327250,          MAC_NOT_LOOKED_UP)
#define AI_LEVEL_TABLE_MAX                     11
#define RVA_COMMON_CAUSE_CHECK                 ADDR(        0x11f060,          0x100472058)
#define RVA_WINNER_CHECK                       ADDR(        0x11cbd0,          MAC_NOT_LOOKED_UP)
#define RVA_GAME_WINNER                        ADDR(        0xfb2cbc0,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_COMMANDER               ADDR(        0x10d000,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_CITADEL                 ADDR(        0x10cd10,          MAC_NOT_LOOKED_UP)
#define RVA_PLANES_ALIKE                       ADDR(        0x253c70,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_CLASS_ABILITY               ADDR(        0x0b0ee0,          MAC_NOT_LOOKED_UP)
#define RVA_CLASS_ABILITY_NTH                  ADDR(        0x0b01c0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_OWNER                    ADDR(        0x128df0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_REPORTS_BATTLE_READ         ADDR(        0x0f5b75,          0x1003dd708)
#define RVA_TOP_BAR_SCORE_GRAPHS               ADDR(        0x09d965,          0x10037b048)
#define RVA_MISC_MENU_SCORE_GRAPHS             ADDR(        0x0a133c,          0x1003736a4)
#define RVA_PLAYER_OVERVIEW_SCORE_GRAPHS       ADDR(        0x1231d1,          0x100482670)
#define RVA_NET_SCORE_GRAPHS_READ              ADDR(        0x1ea131,          0x10050a6f8)
#define RVA_MAP_KEY_SCORE_GRAPHS               ADDR(        0x098359,          MAC_NOT_FOUND)
#define RVA_TYPED_WORD_SCORE_GRAPHS            ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)

// ---- Players ------------------------------------------------------------------------------------
#define PLAYER_SLOTS                           32
#define RVA_SLOT_CONTROLLER                    ADDR(        0xfb31a60,         0x1121af100)
#define RVA_SLOT_CLASS                         ADDR(        0xfb2cbe0,         0x112206f88)
#define RVA_PLAYER_HOME_X                      ADDR(        0x11d32b80,        0x10cce2dc4)
#define RVA_PLAYER_HOME_Y                      ADDR(        0xfb8e720,         0x10cce2e04)
#define RVA_SLOT_AI                            ADDR(        0x11c12b32,        0x10cce2e76)
#define RVA_SLOT_TEAM                          ADDR(        0x11c12b30,        0x10cce2e74)
#define RVA_SLOT_NAME                          ADDR(        0x11c12b00,        0x10cce2e44)
#define PLAYER_BLOCK_SIZE                      0x9004
#define RVA_CAST_COUNTS                        ADDR(        0x11c12b38,        0x10cce2e7c)
#define RVA_AI_LEVEL_NAMES                     ADDR(        0x3442c0,          0x100848390)
#define RVA_SLOT_COLOUR                        ADDR(        0x11c1bab8,        0x10ccebdfc)
#define RVA_SLOT_LOCK_CLASS                    ADDR(        0xf07ae26,         0x11307d61a)
#define RVA_SLOT_LOCK_CONTROLLER               ADDR(        0xf07aea6,         0x11307d69a)
#define RVA_SLOT_LOCK_AI                       ADDR(        0xf07aee6,         0x11307d6da)
#define RVA_SLOT_LOCK_TEAM                     ADDR(        0xf07ae66,         0x11307d65a)
#define RVA_PLAYER_INTS                        ADDR(        0x11c14430,        0x10cce4774)
#define RVA_PLAYER_SHORT                       ADDR(        0x11c14428,        0x10cce476c)
#define RVA_PLAYER_LEADER                      ADDR(        0x11c1442c,        0x10cce4770)
#define RVA_PLAYER_BLESSINGS                   ADDR(        0x11c14468,        0x10cce47ac)
#define RVA_PLAYER_BOLSTERS                    ADDR(        0x11c1baac,        0x10ccebdf0)
#define RVA_PLAYER_DIVINE_FAVOR                ADDR(        0x11c1bab0,        0x10ccebdf4)
#define RVA_PLAYER_INT_A                       RVA_PLAYER_BOLSTERS
#define RVA_PLAYER_INT_B                       RVA_PLAYER_DIVINE_FAVOR
#define RVA_PLAYER_INT_C                       ADDR(        0x11c1bab4,        0x10ccebdf8)
#define RVA_PLAYER_COLOURS                     ADDR(        0x1a299c0,         0x1049c81c8)
#define RVA_RESET_SLOT_COLOURS                 ADDR(        0x31cc38,          0x100586090)
#define RVA_CHOOSE_COLOUR_SCREEN               ADDR(        0x088c20,          0x100490844)
#define RVA_LOAD_PLAYER_2H                     ADDR(        0x185850,          0x1004f4f40)
#define RVA_SAVE_PLAYER_2H                     ADDR(        0x185510,          MAC_INLINED)
#define RVA_MAP_LINE                           ADDR(        0x1b5e50,          0x100512bc0)
#define RVA_MAP_FIXEDPLAYER_LOCKS              ADDR(        0x1b6931,          0x100513890)
#define RVA_RESET_MAP_LOCKS                    ADDR(        0x1ad9f0,          MAC_INLINED)
#define RVA_PLAY_GAME_CONTROLLER_DONE          ADDR(        0x09228d,          0x10038c2e0)
#define RVA_ELIMINATE_PLAYER                   ADDR(        0x107140,          MAC_INLINED)
#define RVA_ELIMINATE_CONTROLLER               ADDR(        0x10717d,          0x100472eec)
#define RVA_ELIMINATE_CONTROLLER_HUMAN         ADDR(        0x1071a7,          0x100472f30)
#define RVA_GAME_OVER_WINNER_HUMAN             ADDR(        0x10d1e6,          0x100474220)
#define RVA_CONQUER_SQUARE                     ADDR(        0x1aeb20,          0x1004aa984)
#define RVA_CONQUER_CONTROLLER_STORE           ADDR(        0x1aee20,          0x1004aab6c)
#define RVA_SQUARE_IS_CITADEL_FOR              ADDR(        0x0b14d0,          MAC_NOT_LOOKED_UP)
#define RVA_CAN_OWN_SQUARE                     ADDR(        0x331190,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SET_OWNER                   ADDR(        0x334eb0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_FUNGUS              ADDR(        0x0b0fc0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_HERBS               ADDR(        0x0b2620,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_WEED                ADDR(        0x0b1420,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_SACRIFICES          ADDR(        0x0b06c0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_HANDS               ADDR(        0x0b0dd0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_GEMS                ADDR(        0x0b1820,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_ANY_GEMS            ADDR(        0x0b0770,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_LIFEFORCE           ADDR(        0x0b0610,          MAC_NOT_LOOKED_UP)
#define RVA_SERVER_SET_CONTROLLER              ADDR(        0x1f3dfa,          0x1004fa948)
#define RVA_SERVER_SET_CONTROLLER_HUMAN        ADDR(        0x1f3e16,          0x1004fa97c)
#define RVA_REPLACE_CONTROLLER                 ADDR(        0x1f05f0,          0x1005021d0)
#define RVA_LOAD_PLAYERS_TURN_CONTROLLER       ADDR_DIFFERS(0x187605,          0x1004ec574)
#define RVA_SAVE_GAME_CONTROLLERS              ADDR_DIFFERS(0x187dc1,          0x1004f2cfc)
#define RVA_LOAD_PLAYERS_TEAM                  ADDR(        0x18799d,          0x1004ecc1c)
#define RVA_SAVE_PLAYERS_TEAM                  ADDR(        0x187ef2,          0x1004f2e5c)
#define RVA_PARTICIPANTS_TEAM_WRAP             ADDR(        0x304c18,          0x1005892d4)
#define RVA_TEAMLESS_OWN_TEAM                  ADDR(        0x32aca0,          0x100511550)
#define RVA_PLAYERCOLOR_STORE                  ADDR(        0x1bbe88,          0x10052f704)
#define RVA_FILE_READ_FIXED                    ADDR(        0x0583d0,          0x100337550)
#define RVA_FILE_WRITE_FIXED                   ADDR_DIFFERS(0x0584a0,          0x1004f3264)
#define RVA_NET_READ_FIXED8                    ADDR(        0x068a80,          MAC_INLINED)
#define RVA_PLAYER_COLOUR_RPC_TEAMMATE         ADDR(        0x30246e,          0x1005871e8)

// ---- Mods ---------------------------------------------------------------------------------------
#define RVA_MOD_NAMES                          ADDR(        0xeed0200,         0x11307d7d8)
#define RVA_MOD_FOLDERS                        ADDR(        0xeed0300,         0x11307d8d8)
#define MOD_RECORD_SIZE                        0xce0
#define MOD_TABLE_MAX                          500
#define MOD_RECORD_ENABLED                     0xcd0
#define RVA_MOD_LAST_INDEX                     ADDR(        0x1a6bb4c,         0x105a0aed4)
#define RVA_EVENT_VARS                         ADDR(        0xf062780,         0x10ce02efc)
#define EVENT_VAR_COUNT                        10000
#define RVA_EVENT_STATE                        ADDR(        0x582edb0,         0x10c127a10)
#define RVA_EVENT_FILE                         ADDR(        0x582edd0,         0x10c398aa0)
#define RVA_RUN_ENABLED_MODS                   ADDR(        0x1cb6f0,          0x10053a2c0)
#define RVA_RUN_ONE_MOD                        ADDR(        0x1bb260,          0x100529e28)
#define RVA_MOD_NAME_CHECK                     ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_BAD_MOD_NAME_ERROR                 ADDR(        0x1bb44c,          0x10052a024)
#define RVA_FIND_ALL_MODS_RUN_CALL             ADDR(        0x1c9f47,          0x100333934)
#define RVA_EVENT_RUNNER                       ADDR(        0x1c5650,          0x10052a730)
#define RVA_RUN_ONE_MOD_EVENT_CALL             ADDR_DIFFERS(0x1bb61c,          0x10052a284)
#define RVA_EVENT_FILE_EVENT_CALL              ADDR(        0x1cb5e6,          0x10053a078)
#define RVA_RANDOM_SQUARE_OF_TERRAIN           ADDR(        0x1b1c30,          0x1004a8838)
#define RVA_RANDOM_RITUAL_SQUARE               ADDR(        0x2ad240,          MAC_NOT_LOOKED_UP)
#define RVA_EVENT_RANDLOC_CALL                 ADDR(        0x1c7185,          0x10052c4dc)
#define RVA_MOD_COMMAND                        ADDR(        0x1bb930,          0x10052ef18)
#define RVA_MOD_ERROR                          ADDR(        0x1c9870,          0x100529c48)
#define RVA_FIND_MOD                           ADDR(        0x1baf10,          0x10053a0fc)
#define RVA_WRITE_CONFIG                       ADDR(        0x193d00,          0x10032a848)
#define RVA_PREFS_WRITE_CONFIG                 ADDR(        0x198cc9,          0x10032dea4)
#define RVA_MODS_SCREEN_WRITE_CONFIG           ADDR(        0x1ca774,          0x10032f468)
#define RVA_MOD_ERROR_WRITE_CONFIG             ADDR(        0x1c99c0,          0x100529dd8)
#define RVA_READ_CONFIG                        ADDR(        0x1930c0,          0x10032ae60)
#define RVA_READ_CONFIG_ENABLE_MOD             ADDR(        0x19369e,          0x10032b338)
#define RVA_FIND_ALL_MODS                      ADDR(        0x1c9ab0,          MAC_INLINED)
#define RVA_ENABLED_MODS_SCREEN                ADDR(        0x1cb8b0,          0x10053b7f4)
#define RVA_ENABLE_SAVE_MODS                   ADDR(        0x1887b0,          MAC_NOT_FOUND)
#define RVA_OLD_WORKSHOP                       ADDR(        0x336840,          MAC_INLINED)
#define RVA_OLD_WORKSHOP_READ                  ADDR(        0x336bae,          0x1003326d8)
#define RVA_WRITE_STRING                       ADDR(        0x057ae0,          0x10033771c)
#define RVA_READ_STRING                        ADDR(        0x057960,          MAC_INLINED)
#define RVA_FONT_FILE                          ADDR(        0x03bef0,          MAC_INLINED)
#define RVA_FONT_FILE_CHECK                    ADDR_DIFFERS(0x1bc118,          0x100538cc0)
#define RVA_FONT_FILE_TTF_CHECK                ADDR(        0x1bc154,          0x100538d48)
#define RVA_HAS_EXTENSION                      ADDR(        0x059b40,          MAC_INLINED)
#define RVA_ADD_TRANSLATION                    ADDR(        0x03d000,          0x10033bf18)
#define RVA_ADD_FX_NAME                        ADDR(        0x1c8510,          0x100538f9c)
#define RVA_PARSER_ABILITY_WORD                RVA_ADD_FX_NAME
#define RVA_MOD_IMAGE_CHECK                    ADDR(        0x1c9160,          0x1005389c4)
#define RVA_IMAGE_SIZE_READ                    ADDR(        0x027320,          0x10033a254)
#define RVA_FILE_CASE_CHECK                    ADDR(        0x05a5a0,          0x10033606c)
#define RVA_DIR_CASE_CHECK                     ADDR(        0x1c9240,          0x100529a48)
#define RVA_DIR_NAME_CASE_CHECK                ADDR(        0x05a7d0,          0x100335e28)
#define RVA_DIR_EXISTS                         ADDR(        0x05bf60,          MAC_INLINED)
#define RVA_FILE_EXISTS                        ADDR(        0x05e2c0,          MAC_INLINED)
#define RVA_DIR_NEXT_ENTRY                     ADDR_DIFFERS(0x00f4b8,          0x1005aee18)
#define RVA_MODS_DIR                           ADDR(        0x5828120,         0x10c399465)
#define RVA_WORKSHOP_ITEMS                     ADDR(        0xa0d9580,         0x1136f6170)
#define WORKSHOP_ITEM_SIZE                     0x9c6
#define RVA_MODS_CLOSEDIR_CALL                 ADDR(        WIN_NO_CALL,       0x100332d50)
#define RVA_MODS_WORKSHOP_FIND_MOD_CALL        ADDR(        0x1c9c93,          0x100332da4)
#define RVA_LOBBY_MODS_OPENDIR_CALL            ADDR(        0x1c9d13,          0x100332e18)
#define RVA_LOBBY_MODS_CLOSEDIR_CALL           ADDR(        WIN_NO_CALL,       0x100332ea0)
#define RVA_CLOSEDIR                           ADDR(        0x05bfc0,          0x1005ae488)
#define RVA_PARSER_WORD                        ADDR(        0x9e9df10,         0x10c127a14)
#define RVA_SQUARE_EVENT_A                     ADDR(        0x9e76e10,         0x10c063d40)
#define SQUARE_EVENT_A_SIZE                    160000
#define RVA_SQUARE_EVENT_B                     ADDR(        0x9e9e750,         0x10c08ae40)
#define RVA_EVENT_PLAYER                       ADDR(        0x1a93de4,         0x101cb6e14)
#define RVA_EVENT_TARGET_X                     ADDR(        0x1a955e4,         0x101cb6e20)
#define RVA_EVENT_TARGET_Y                     ADDR(        0x1a924a4,         0x101cb6e24)
#define RVA_EVENT_SQUARE_X                     ADDR(        0x1a929ac,         0x101cb6e18)
#define RVA_EVENT_SQUARE_Y                     ADDR(        0x1a93c3c,         0x101cb6e1c)
#define RVA_EVENT_PLAYER_ARG                   ADDR(        0x1c8460,          0x100539b30)
#define RVA_EVENT_INT_ARG                      ADDR(        0x1b7130,          0x100512a80)
#define RVA_EVENT_TEXT_ARG                     ADDR(        0x1b7000,          0x100512b04)
#define RVA_FIND_UNIT_AT                       ADDR(        0x105900,          0x100480bd8)
#define RVA_FIND_ITEM_HOLDER_AT                ADDR(        0x17e2c0,          0x1004e9b64)
#define RVA_FIND_UNIT_AT_END                   ADDR(        0x105a9b,          0x1004810cc)
#define RVA_FIND_ITEM_HOLDER_AT_END            ADDR(        0x17e483,          0x1004ea370)
#define RVA_ARMY_AT_FOR_PLAYER                 ADDR(        0x104410,          MAC_INLINED)
#define RVA_SAME_TEAM                          ADDR(        0x32b680,          MAC_INLINED)
#define RVA_EVENT_HASUNIT_CALL                 ADDR(        0x1c634a,          0x10052dd84)
#define RVA_EVENT_HASUNITHERE_CALL             ADDR(        0x1c6440,          0x10052df48)
#define RVA_EVENT_HASITEM_CALL                 ADDR(        0x1c681e,          0x10052e548)
#define RVA_EVENT_HASITEMHERE_CALL             ADDR(        0x1c68e0,          0x10052e658)
#define RVA_COUNT_UNITS                        ADDR(        0x104c10,          0x100481e84)
#define RVA_SQUARE_OWNER_AT                    ADDR(        0x335550,          MAC_INLINED)
#define RVA_ARMY_OWNER_AT                      ADDR(        0x129950,          MAC_INLINED)
#define RVA_SQUARE_ARMY_AT                     ADDR(        0x129480,          MAC_INLINED)
#define RVA_SQUARE_ARMY                        ADDR(        0xfb3b700,         0x1120c3fb0)
#define RVA_MOD_LINE                           ADDR(        0x1a959b4,         0x101cce558)
#define RVA_EVENT_FILE_RUNNER                  ADDR(        0x1cb3e0,          0x100539d94)
#define RVA_WORKSHOP_LAST                      ADDR(        0x1b18714,         0x105a0aee0)
#define RVA_EVENT_KILL_SQUARE                  ADDR(        0x118d10,          0x100481de0)
#define RVA_PROMOTE_UNITS_AT                   ADDR(        0x10f310,          0x100471a94)
#define RVA_PROMOTE_UNIT                       ADDR(        0x10f500,          0x10046da54)
#define RVA_DAMAGE_UNIT                        ADDR(        0x0bd640,          0x1003b9354)
#define RVA_REMOVE_UNIT                        ADDR(        0x0f89b0,          0x1003b9014)
#define RVA_GIVE_ITEM_BY_NAME                  ADDR(        WIN_INLINED,       0x1004e58f8)
#define RVA_TAKE_ITEM_BY_NAME                  ADDR(        0x1803c0,          0x1004e5a14)
#define RVA_GIVE_RANDOM_ITEM                   ADDR(        0x183a10,          0x1004e5fb8)
#define RVA_EVENT_FILE_COPY                    ADDR(        WIN_INLINED,       0x100529f80)
#define RVA_EVENT_FILE_CLEAR                   ADDR(        0x1cb6a4,          0x10053a0a4)
#define RVA_EVENT_STATE_START                  ADDR(        0x1c56ef,          0x10052a8d0)
#define RVA_EVENT_STATE_SQUARE                 ADDR(        0x1c57df,          0x10052a94c)
#define RVA_EVENT_STATE_END                    ADDR(        0x1c5818,          0x10052a980)
#define RVA_EVENT_STATE_FAIL                   ADDR(        0x1c828e,          0x10052c004)
#define RVA_EVENT_STATE_CLEAR                  ADDR(        0x1bb654,          0x10052a2b8)
#define RVA_RUN_MODS_SELF_CALL                 ADDR(        0x1cb82b,          0x10053a5e8)
#define RVA_PLAY_GAME_START_RUN_MODS_CALL      ADDR(        0x091d8c,          0x10038a140)
#define RVA_PLAY_GAME_END_RUN_MODS_CALL        ADDR(        0x093e50,          0x1003919f0)
#define RVA_GAME_END_FN_RUN_MODS_CALL          ADDR(        0x094171,          MAC_NOT_FOUND)
#define RVA_TURN_START_RUN_MODS_CALL           ADDR(        0x167ad2,          0x100392cc4)
#define RVA_LOAD_GAME_RUN_MODS_CALL            ADDR(        0x185e40,          0x1004eb98c)
#define RVA_STARTUP_RUN_MODS_CALL              ADDR(        0x1910a9,          0x100332fbc)
#define RVA_MODS_SCREEN_RESET_CALL             ADDR(        0x1ca767,          0x10032f45c)
#define RVA_MODS_SCREEN_RUN_MODS_CALL          ADDR(        0x1ca76f,          0x10032f464)
#define RVA_MODS_OPENDIR_CALL                  ADDR(        0x1c9b32,          0x100332cc0)
#define RVA_MOD_NAME_COMPARE                   ADDR(        0x05c010,          MAC_INLINED)
#define RVA_MOD_TABLE_SORT                     ADDR(        0x1c9e40,          0x100332ea4)
#define RVA_LINE_NUMBER_ARG                    ADDR(        0x1b7130,          0x100512a80)
#define RVA_PARSER_MODVERSION                  ADDR(        0x1bbbcb,          0x10052f0a8)
#define RVA_PARSER_MODPRIO                     ADDR(        0x1bbc27,          0x10052f47c)
#define RVA_PARSER_CLEAR_CURRENT               ADDR(        0x1c5600,          MAC_INLINED)
#define RVA_ADD_MOD_FILE                       ADDR_DIFFERS(0x1c8be0,          0x100538aa4)
#define RVA_MOD_FILE_LIST                      ADDR(        0xe9ee200,         0x11320fd60)
#define MOD_FILE_ENTRY_SIZE                    0x200
#define RVA_MOD_FILE_LAST_INDEX                ADDR(        0x1a95334,         0x105a0aed8)
#define RVA_MOD_FILE_BYTES                     ADDR(        0x582edb4,         0x11320fd58)
#define RVA_MOD_CHECKSUMS                      ADDR(        0xeed0ed4,         0x11307e4ac)
#define RVA_MOD_CRC                            ADDR(        0x1ca940,          MAC_INLINED)
#define RVA_LOAD_MOD_LIST                      ADDR(        0x1887b0,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST                      ADDR(        0x188ac0,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST_NAME_WRITE           ADDR(        0x188b36,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST_CRC_WRITE            ADDR(        0x188b78,          MAC_NOT_FOUND)
#define RVA_MOD_CRC_FIRST_TURN                 ADDR(        0x1a3b2cc,         0x100872c68)
#define RVA_MOD_CRC_WARNED                     ADDR_DIFFERS(0x5822f74,         0x1085ba7b0)
#define RVA_LOAD_GAME_MOD_CRC_CALL             ADDR(        0x188a1e,          MAC_NO_CALL)
#define RVA_SAVE_GAME_MOD_CRC_CALL             ADDR(        0x18e95c,          MAC_NO_CALL)
#define RVA_SAVE_MOD_LIST_MOD_CRC_CALL         ADDR(        0x188b43,          MAC_NOT_FOUND)
#define RVA_LOAD_MOD_LIST_MOD_CRC_CALL         ADDR(        0x188a1e,          MAC_NOT_FOUND)
#define RVA_LOAD_GAME_FOPEN_CALL               ADDR(        0x185c7f,          0x1004eb5a4)
#define RVA_LOAD_GAME_MOD_NAME_READ            ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_LOAD_GAME_MOD_NAME_READ_NEXT       ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_LOAD_GAME_MOD_CRC_READ             ADDR_DIFFERS(0x188946,          0x1004eb814)
#define LOAD_GAME_MOD_NAME_AFTER_CRC           4
#define LOAD_MOD_LIST_NAME_AFTER_CRC           0x10
#define RVA_WARN_FORMAT                        ADDR(        0x042f40,          0x100348658)
#define RVA_LOAD_GAME_MOD_DIFFERS_CALL         ADDR(        0x188a35,          0x1004eb940)
#define RVA_LOAD_GAME_MOD_WARNED_SET           ADDR_DIFFERS(0x188a3a,          0x1004eb948)
#define RVA_LOAD_GAME_RESET_MODS_CALL          ADDR(        0x185e0d,          0x1004eb954)
#define SAVE_VERSION_MOD_CHECKSUMS             0xc3
#define RVA_LOAD_GAME_MOD_NOT_FOUND            ADDR(        0x188a8c,          0x1004ee6ec)
#define RVA_LOAD_GAME_MOD_FLAGS_CLEAR          ADDR(        0x188800,          0x1004eb704)
#define RVA_LOAD_GAME_MOD_ENABLE               ADDR(        0x1889df,          0x1004eb870)
#define RVA_SAVE_GAME_MOD_NAME_WRITE           ADDR(        0x18e94e,          0x1004f22b8)
#define RVA_SAVE_GAME_MOD_CRC_WRITE            ADDR(        0x18e98c,          0x1004f2280)
#define RVA_SAVE_GAME_MOD_LIST_END             ADDR(        WIN_INLINED,       0x1004f2348)

// ---- Game data tables ---------------------------------------------------------------------------
#define RVA_MONSTER_TABLE                      ADDR(        0x1c65270,         0x104b515f8)
#define RVA_MONSTER_COUNT                      ADDR(        0x1aa635c,         0x10487ff28)
#define RVA_MONSTER_CURRENT                    ADDR(        0x1a929d4,         0x101cb6e30)
#define RVA_MONSTER_SPRITES                    ADDR(        0x1c652a0,         0x104b51628)
#define MONSTER_SIZE                           0x9d8
#define MONSTER_NAME_SIZE                      0x30
#define MONSTER_TABLE_MAX                      6000
#define MONSTER_NAME_SIZE                      0x30
#define RVA_MONSTER_FLAGS                      ADDR(        0x1c65440,         0x104b517c8)
#define MONSTER_MOVE_FLAGS                     0x1c8
#define ITEM_MOVE_FLAGS                        0x58
#define ITEM_FLAG_CURSED                       0x400000000000ULL
#define RVA_PARSER_MOVE_WORD                   ADDR(        0x1c8750,          MAC_NOT_LOOKED_UP)
#define RVA_PARSER_FLAG_WORD                   ADDR(        0x1c8ae0,          0x100538eb0)
#define RVA_SOULLESS_FORM                      ADDR(        0x1d9fe0,          0x1004912c4)
#define RVA_UNIT_TRAIT_LIST                    ADDR(        0x1d1740,          0x10051cd7c)
#define MONSTER_ABILITY_LIMITED_GEM_BONUS      0xbf
#define MONSTER_ABILITY_RANGE_BONUS            0xc7
#define MONSTER_ABILITY_WALL_COVER             0x198
#define MONSTER_ABILITY_WALL_RANGE_BONUS       0x1a6
#define MONSTER_ABILITY_TRIPLE_ENTITY          0x1c6
#define MONSTER_ABILITY_MIRROR_CLOAKING        0x43b
#define MONSTER_ABILITY_MAGIC_TATTOO           0x4cb
#define MONSTER_ABILITY_REINCARNATION          0x4f4
#define MONSTER_ABILITY_BRINGER_OF_PLENTY      0x4fa
#define MONSTER_FLAG_FIRE_100                  0
#define MONSTER_FLAG_COLD_100                  1
#define MONSTER_FLAG_POISON_100                2
#define MONSTER_FLAG_SHOCK_100                 15
#define MONSTER_FLAG_CAUSES_FEAR               29
#define RVA_HATED_KIND_NAMES                   ADDR(        0x3faad0,          0x100838cd0)
#define RVA_FIND_MONSTER                       ADDR(        0x1da1f0,          0x1004914f4)
#define MONSTER_ABILITIES                      0x1d8
#define MONSTER_ABILITY_PAIRS                  256
#define MONSTER_ABILITY_POWER_BASE             0x1f4
#define MONSTER_ABILITY_STARTITEM              0x4a
#define PARSER_STARTITEM_MAX                   4
#define RVA_PARSER_STARTITEM_COUNT             ADDR(        0x582edcc,         MAC_NOT_LOOKED_UP)
#define MONSTER_ARMOR                          0x34
#define MONSTER_HP                             0x36
#define MONSTER_STR                            0x38
#define MONSTER_MORALE                         0x3a
#define MONSTER_RANK                           0x3c
#define MONSTER_MR                             0x1c0
#define MONSTER_ATTACKS                        0x40
#define MONSTER_ATTACK_SIZE                    0x18
#define MONSTER_ATTACK_MAX                     16
#define MONSTER_ATTACK_WEAPON                  0x8
#define MONSTER_ATTACK_VALUE                   0x10
#define RVA_MONSTER_ABILITY                    ADDR(        0x129730,          MAC_INLINED)
#define RVA_NAME_COMPARE                       ADDR_DIFFERS(0x057830,          0x1005aef50)
#define RVA_UNIT_SPAWN_OFFSET_READ             ADDR(        0x145328,          0x10039cc28)
#define RVA_UNIT_GROW_OFFSET_READ              ADDR(        0x1456f1,          0x10039d668)
#define RVA_WEAPON_TABLE                       ADDR(        0x5700780,         0x104904cb0)
#define WEAPON_SIZE                            0xa0
#define WEAPON_TRGRANK                         0x40
#define WEAPON_RANGE                           0x44
#define WEAPON_INIT                            0x48
#define WEAPON_DMGTYPE                         0x4c
#define WEAPON_DMG                             0x50
#define WEAPON_AOE                             0x58
#define WEAPON_FLAGS                           0x60
#define WEAPON_NEXT                            0x90
#define WEAPON_NEXT_DMG                        0x98
#define RVA_WEAPON_FREE_SLOT                   ADDR(        0x0b8860,          0x1003f8634)
#define RVA_WEAPON_CURRENT                     ADDR(        0x1a94ff4,         0x101cb6e34)
#define RVA_ITEM_TABLE                         ADDR(        0x1be6730,         0x1048937f8)
#define ITEM_SIZE                              0x1d0
#define ITEM_NAME_SIZE                         0x30
#define ITEM_TABLE_MAX                         1000
#define RVA_ITEM_COUNT                         ADDR(        0x1a3b0b4,         0x101cce550)
#define RVA_ITEM_CURRENT                       ADDR(        0x1a92484,         0x101cb6e2c)
#define RVA_ITEM_FLAGS                         ADDR(        0x1be6770,         0x104893838)
#define RVA_ITEM_STATE_FLAGS                   ADDR(        0x1be6778,         0x104893840)
#define RVA_CLASS_TABLE                        ADDR(        0x18a0720,         0x1049c8358)
#define CLASS_SIZE                             0x1f74
#define CLASS_NAME_SIZE                        0x40
#define CLASS_TABLE_MAX                        200
#define RVA_CLASS_COUNT                        ADDR(        0x43d0d4,          0x1049c8354)
#define RVA_CLASS_LEADERS                      ADDR(        0x18a0760,         0x1049c8398)
#define RVA_CLASS_SIDE_A                       ADDR(        0x45cf20,          0x1008735d8)
#define RVA_CLASS_SIDE_B                       ADDR(        0x445820,          0x101cb6e50)
#define RVA_CLASS_CURRENT                      ADDR(        0x1a95d34,         0x101cb6e44)
#define CLASS_HOME_TERRAIN                     0x70
#define CLASS_START_UNITS                      0x84
#define CLASS_START_UNITS_MAX                  16
#define CLASS_START_UNIT_SIZE                  0x40
#define CLASS_ABILITIES                        0x484
#define CLASS_ABILITY_PAIRS                    16
#define CLASS_ABILITY_STDTROOPS                0xde
#define CLASS_ABILITY_AI_CLASS                 0xcb
#define CLASS_ABILITY_RANDOM_SKIP              0xcc
#define CLASS_ABILITY_AI_LEVEL_MIN             0xd2
#define CLASS_ABILITY_AI_LEVEL_MAX             0xd3
#define CLASS_RECRUITS_SIZE                    0x19f00
#define CLASS_RECRUITS_MAX                     160
#define CLASS_RECRUIT_SIZE                     0x298
#define RECRUIT_MONSTER_NAME                   0x0
#define RECRUIT_CHANCE                         0x30
#define RECRUIT_COUNT                          0x34
#define RECRUIT_GOLD                           0x38
#define RECRUIT_GOLD_DIE                       0x3c
#define RECRUIT_IRON                           0x40
#define RECRUIT_FLAGS                          0x44
#define RECRUIT_EXTRA_COSTS                    0x48
#define RECRUIT_LIMITERS                       0x52
#define RECRUIT_LIMITERS_MAX                   12
#define RECRUIT_TERRAIN                        0x294
#define CLASS_HERO_SIZE                        0x78
#define CLASS_HERO_NAME                        0x30
#define RVA_CLASS_ADD_RECRUIT                  ADDR(        0x0b0340,          0x1003617c4)
#define RVA_CLASS_ADD_START_UNIT               ADDR(        0x0b0230,          0x1003618c0)
#define RVA_CLASS_ABILITY                      ADDR(        0x0b01c0,          0x100361b30)
#define RVA_CLASS_SET_ABILITY                  ADDR(        0x0b0010,          0x1003620a8)
#define RVA_CLASS_CLEAR_ABILITY                ADDR_DIFFERS(0x0b00d0,          0x100361e38)
#define RVA_CLASS_PRINT_ALL_RECRUITS           ADDR(        0x0b58e0,          0x1004c1f6c)
#define RVA_CLASS_PRINT_RECRUITS               ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_HERO_ARRIVAL                       ADDR(        0x089df0,          MAC_INLINED)
#define RVA_TERRAIN_TABLE                      ADDR(        0x56aeaa0,         0x1059bcc90)
#define TERRAIN_SIZE                           0xa0
#define TERRAIN_NAME_SIZE                      0x50
#define TERRAIN_TABLE_MAX                      2000
#define TERRAIN_MATCH_CODE_MIN                 -1500
#define TERRAIN_GOLD                           0x30
#define TERRAIN_IRON                           0x32
#define TERRAIN_TRADE                          0x34
#define TERRAIN_HERBS                          0x36
#define TERRAIN_FUNGUS                         0x38
#define TERRAIN_WEED                           0x3a
#define TERRAIN_SACRIFICES                     0x3c
#define TERRAIN_HANDS                          0x40
#define TERRAIN_LIFEFORCE                      0x44
#define TERRAIN_FLAGS                          0x50
#define TERRAIN_FLAG_PORT                      0x400000000000000ULL
#define TERRAIN_APCOST                         0x46
#define RVA_TERRAIN_CURRENT                  ADDR(        0x1a95e74,         0x101cb6e3c)
#define RVA_TERRAIN_GROUP_NAMES                ADDR(        0xd4a6580,         0x10c81c17c)
#define RVA_TERRAIN_GROUP_MEMBERS              ADDR(        0xd4ae280,         0x10c823e7c)
#define RVA_TERRAIN_GROUP_CURRENT              ADDR(        0x1a92464,         0x101cb6e40)
#define RVA_BUILTIN_GROUP_NAMES                ADDR(        0x405b00,          0x100847ed8)
#define RVA_RITUAL_TABLE                       ADDR(        0x2ae2fb0,         0x101cce668)
#define RVA_RITUAL_CODE_FIRST                  ADDR(        0x2ae000,          MAC_FILE_FORMAT)
#define RVA_RITUAL_CODE_LAST                   ADDR(        0x2ce000,          MAC_FILE_FORMAT)
#define RITUAL_SIZE                            0x3ba8
#define RITUAL_NAME_SIZE                       0x30
#define RITUAL_RITPOW                          0x30
#define RITUAL_LEVEL                           0x34
#define RITUAL_COSTS                           0x38
#define RITUAL_COSTS_MAX                       4
#define RITUAL_COST_SIZE                       8
#define RITUAL_TERRAIN                         0x5c
#define RITUAL_FX_VALUE                        8
#define RITUAL_FX                              0x60
#define RITUAL_FX_MAX                          20
#define RITUAL_FX_SIZE                         0x10
#define RITUAL_FX_SUMMONING                    900
#define RITUAL_FLAGS                           0x1a0
#define RITUAL_STRINGS                         0x1a8
#define RITUAL_STRINGS_MAX                     58
#define RITUAL_STRING_SIZE                     0x100
#define RVA_RITUAL_COUNT                       ADDR(        0x1afed24,         0x101cce58c)
#define RVA_RITPOW_COUNT                       ADDR(        0x1afe4ac,         0x101cce594)
#define RVA_RITUAL_CURRENT                     ADDR(        0x1a92984,         0x101cb6e38)
#define RVA_FIND_RITUAL                        ADDR(        0x2be3a0,          MAC_NOT_FOUND)
#define RVA_ALLOC_RITUAL                       ADDR(        0x2be470,          0x1005672c8)
#define RVA_ALLOC_RITUAL_TABLE_REF             ADDR_DIFFERS(0x2be498,          0x100567304)

#define RVA_INIT_RITUALS                       ADDR(        0x2bea70,          MAC_INLINED)
#define RVA_INIT_RITUALS_BOUND                 ADDR_DIFFERS(0x190dab,          0x100331704)
#define RVA_INIT_RITUALS_BOUND_RESET_MODS      ADDR_DIFFERS(0x2bea8f,          0x1005293b4)
#define RVA_INIT_RITUALS_BOUND_3               ADDR_DIFFERS(0x2bea8f,          0x100553114)
#define RVA_INIT_RITUALS_BOUND_RESET_STATE     ADDR_DIFFERS(0x2bea8f,          0x100585cec)
#define RVA_INIT_RITUALS_NO_END_CALL           ADDR(        0x2beace,          0x100331714)
#define RVA_RITUAL_ADD_FX                      ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_RITUAL_ADD_FX_BOUND                ADDR(        0x1c85f1,          0x1005391ac)
#define RVA_RITUAL_ADD_STRING                  ADDR(        0x2be5f0,          0x100566b70)
#define RVA_RITUAL_ADD_STRING_BOUND            ADDR(        0x2be5fe,          0x100566b80)
#define RVA_ALLOC_RITUAL_BOUND                 ADDR_DIFFERS(0x2be486,          0x1005672ec)
#define RVA_ALLOC_RITUAL_FAIL_BRANCH           ADDR_DIFFERS(0x2be48b,          0x1005672f0)
#define RVA_OUT_OF_RITUALS_CALL                ADDR(        0x1bf6eb,          0x100532f9c)
#define RVA_MOD_COMMAND_EXIT                   ADDR(        0x1c55c3,          0x10052ef64)
#define RVA_RANDOM_RITUAL                      ADDR(        0x2bde50,          MAC_INLINED)
#define RVA_RANDOM_RITUAL_BOUND                ADDR(        0x2bdeaa,          MAC_INLINED)
#define RVA_RANDOM_RITUAL_1A_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x10055342c)
#define RVA_RANDOM_RITUAL_1A_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x10055350c)
#define RVA_RANDOM_RITUAL_1B_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x1005535a4)
#define RVA_RANDOM_RITUAL_1B_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x10055366c)
#define RVA_RANDOM_RITUAL_2A_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x100553ee0)
#define RVA_RANDOM_RITUAL_2A_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x100553fb0)
#define RVA_RANDOM_RITUAL_2B_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x100554068)
#define RVA_RANDOM_RITUAL_2B_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x100554130)
#define RVA_RANDOM_RITUAL_3_ENTRY              ADDR(        WIN_NO_EQUIVALENT, 0x100554d4c)
#define RVA_RANDOM_RITUAL_3_RESUME             ADDR(        WIN_NO_EQUIVALENT, 0x100554e4c)
#define RVA_EXECUTE_RITUAL                     ADDR(        0x2c59f0,          0x10055d920)
#define RVA_EXECUTE_RITUAL_BOUND               ADDR(        0x2c5ac6,          0x10055d98c)
#define RVA_RITUAL_PR_BOUND                    ADDR(        0x2c2813,          0x100558b88)
#define RVA_RITUAL_CPR_BOUND                   ADDR(        0x2c3634,          0x10055876c)
#define RVA_UNIT_RITUAL_LIST_BOUND             ADDR(        0x1d79ab,          MAC_NOT_FOUND)
#define RVA_RITUAL_SCREEN_LOOP_1               ADDR(        0x2ae97a,          0x10035fca4)
#define RVA_RITUAL_SCREEN_LOOP_2               ADDR(        0x2afe82,          0x10035ffb0)
#define RVA_RITUAL_ROW_ID_GREYED               ADDR_DIFFERS(0x2afe05,          0x10036078c)
#define RVA_RITUAL_ROW_ID_GREYED_2             ADDR_DIFFERS(0x2afe05,          0x1003607f0)
#define RVA_RITUAL_ROW_ID_GREYED_3             ADDR_DIFFERS(0x2afe05,          0x100360848)
#define RVA_RITUAL_ROW_ID_GREYED_4             ADDR_DIFFERS(0x2afe05,          0x1003609ec)
#define RVA_RITUAL_ROW_ID_GREYED_5             ADDR_DIFFERS(0x2afe05,          0x1003608f4)
#define RVA_RITUAL_ROW_ID_GREYED_6             ADDR_DIFFERS(0x2afe05,          0x100360928)
#define RVA_RITUAL_ROW_ID_GREYED_7             ADDR_DIFFERS(0x2afe05,          0x10036097c)
#define RVA_RITUAL_ROW_ID_CASTABLE             ADDR_DIFFERS(0x2b0077,          0x100360a48)
#define RVA_RITUAL_ROW_ID_JOIN                 ADDR(        WIN_NO_EQUIVALENT, 0x10036085c)
#define RVA_RITUAL_CLICK_DECODE                ADDR(        0x2b034d,          0x100361440)
#define RVA_RITUAL_RIGHT_CLICK_DECODE          ADDR(        0x2b059d,          0x100360d30)
#define RVA_RITUAL_CLICK_CASTABLE_BOUND        ADDR(        0x2b035a,          0x100361444)
#define RVA_RITUAL_RIGHT_CLICK_BAND            ADDR_DIFFERS(0x2b05a3,          0x100360d38)
#define RVA_RITUAL_RIGHT_CLICK_GREYED          ADDR_DIFFERS(0x2b05ab,          0x100360d3c)
#define RVA_BACKUP_TABLES                      ADDR(        0x1ca7c0,          MAC_INLINED)
#define RVA_RITUAL_BACKUP                      ADDR(        0x582f7a8,         0x10c063d20)
#define RVA_RITUAL_BACKUP_SIZE                 ADDR(        0x190fb0,          0x100332c24)
#define RVA_RITUAL_BACKUP_COPY_SIZE            ADDR(        0x190fba,          0x100332c40)
#define RVA_RITUAL_RESTORE_SIZE                ADDR(        0x1cab3a,          0x100528f34)
#define RVA_CHECK_VANILLA_NAMES                ADDR(        0x116360,          MAC_INLINED)
#define RVA_TURN_CAST_COUNTS                   ADDR(        0x2bfe60,          MAC_INLINED)
#define RVA_TURN_CAST_COUNTS_REF               ADDR_DIFFERS(0x169dd4,          0x100399d58)
#define RVA_LOAD_2H_CAST_COUNTS                ADDR_DIFFERS(0x188429,          0x1004f525c)
#define RVA_LOAD_GAME_CAST_COUNTS              ADDR_DIFFERS(0x187a5f,          0x1004ecca4)
#define RVA_LOAD_PLAYERS                       ADDR(        0x187420,          MAC_INLINED)
#define RVA_EXECUTE_RITUAL_CAST_COUNTS         ADDR_DIFFERS(0x2c6fef,          0x10055f058)
#define RVA_SAVE_GAME_CAST_COUNT_READ          ADDR_DIFFERS(0x187fa6,          0x1004f2f00)
#define RVA_SAVE_2H_CAST_COUNT_READ            ADDR_DIFFERS(0x188690,          0x10050af28)
#define RVA_CAST_STRIDE_AI_MAXCAST             ADDR(        0x06cfe7,          0x1004dfc78)
#define RVA_CAST_STRIDE_EXECUTE_ADD            ADDR_DIFFERS(0x2c6fe5,          0x10055f040)
#define RVA_CAST_STRIDE_EXECUTE_INC            ADDR(        0x2c9b7c,          MAC_STORED_DIFFERENTLY)
#define RVA_CAST_STRIDE_CAN_CAST               ADDR(        0x2ca3d6,          0x10055c2a0)
#define RVA_CAST_STRIDE_FX_40E                 ADDR(        0x2b65a1,          0x10055784c)
#define RVA_CAST_STRIDE_TURN                   ADDR(        0x169dcd,          0x100399d54)
#define RVA_CAST_STRIDE_TURN_PLAYER_29         ADDR(        WIN_NO_EQUIVALENT, 0x100392fd8)
#define RVA_SAVE_GAME_END_MARKER               ADDR(        0x18f7e0,          0x1004f4bb4)
#define RVA_SAVE_GAME_FCLOSE                   ADDR(        0x18f7f0,          0x1004f4bc8)
#define RVA_SERVER_SAVE_END_MARKER             ADDR(        0x1f4675,          MAC_NO_CALL)
#define RVA_SAVE_2H_FCLOSE                     ADDR_DIFFERS(0x185776,          0x10050af90)
#define RVA_LOAD_GAME_END_MARKER               ADDR(        0x186f98,          0x1004ee30c)
#define RVA_SAVE_2H_END_MARKER                 ADDR_DIFFERS(0x185754,          0x10050af7c)
#define RVA_LOAD_2H_END_MARKER                 ADDR(        0x185b16,          0x1004f52b8)
#define RVA_LOAD_2H_CAST_COUNTS_CALL           ADDR(        0x188432,          0x1004f5264)
#define RVA_SAVE_2H_CAST_COUNTS_CALL           ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_SAVE_GAME_CAST_COUNTS_CALL         ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_LOAD_GAME_CAST_COUNTS_CALL         ADDR(        0x187a65,          0x1004eccac)
#define RVA_SAVE_PLAYER_BLOCK                  ADDR(        0x188480,          MAC_INLINED)
#define RVA_SAVE_PLAYER_BLOCK_CALL             ADDR(        0x185738,          MAC_INLINED)
#define RVA_LOAD_PLAYER_BLOCK                  ADDR(        0x1881f0,          MAC_INLINED)
#define RVA_RESET_PLAYERS_CLEAR                ADDR_DIFFERS(0x31cb37,          0x100585e7c)
#define RVA_KEY_CHECK_PLAYERS_CLEAR            ADDR_DIFFERS(0x14378c,          0x1003b5a64)
#define RVA_COMPACT_PLAYERS_MOVE               ADDR_DIFFERS(0x1f0903,          0x1004f8ed4)
#define RVA_COMPACT_PLAYERS_MOVE_2             ADDR_DIFFERS(0x1f0903,          0x1004f8ed4)
#define RVA_ENCHANTS                           ADDR(        0xd49c940,         0x10c9180bc)
#define ENCHANT_SIZE                           0x28
#define RVA_LAST_ENCHANT                       ADDR(        0x1afe784,         0x101cce590)
#define ENCHANTS_MAX                           1000
#define RVA_LOAD_ENCHANTS                      ADDR(        0x18d260,          0x1004efd48)
#define RVA_NEW_ENCHANT                        ADDR(        0x2c1400,          0x100563b54)
#define RVA_NET_SEND_ENCHANT                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_NET_ENCHANT_UPDATE                 ADDR(        0x1eaf00,          0x100506d54)
#define RVA_COMMANDER_REPEAT_RITUAL            ADDR(        0x11d3c844,        0x10ce0cbb4)
#define RVA_COMMANDER_REPEAT_PERCENT           ADDR(        0x11d3c848,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_FIELD_10                 ADDR(        0x11d3c840,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_PLAN                  ADDR(        0x11d3c8cc,        0x10ce0cc3c)
#define RVA_PERFORM_REPEAT_RITUALS             ADDR(        0x2c1c60,          0x100566510)
#define RVA_SAVE_COMMANDER                     ADDR(        0x18c760,          0x1004f0fc0)
#define RVA_LOAD_COMMANDER                     ADDR(        0x18b650,          0x1004ef86c)
#define RVA_NET_PUT_FULL_UNIT                  ADDR(        0x1e7550,          0x100500674)
#define RVA_SEND_ALL_ENABLED_MODS              ADDR(        WIN_INLINED,       MAC_INLINED)

#define RVA_SPELL_TABLE                        ADDR(        0x569f570,         0x100839030)
#define SPELL_SIZE                             0x28
#define SPELL_PATH                             0x8
#define SPELL_LEVEL                            0xc
#define SPELL_WEAPON                           0x10
#define SPELL_FLAGS                            0x20
#define RVA_SPELL_TABLE_END                    ADDR(        0x56adfd0,         0x100847a90)
#define RVA_SPELL_PATH_NAMES                   ADDR(        0x405ee0,          0x100838d10)
#define RVA_UNIT_PATH_LEVEL                    ADDR(        0x2e72e0,          0x1004bb6a0)
#define RVA_UNIT_PATH_BONUS                    ADDR(        0x2e4ba0,          0x1004bb238)
#define RVA_UNIT_PATH_BOOSTS                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_UNIT_SPELL_WEAPON_LEVEL            ADDR(        0x2e73f0,          0x1004bbc94)
#define RVA_UNIT_ABILITY_TOTAL                 ADDR(        0x117a00,          0x1004668a4)
#define RVA_ITEM_ABILITY_HAS_VALUE             ADDR(        WIN_INLINED,       0x1004e5ed0)
#define RVA_IS_SPELL_ATTACK                    ADDR(        0x0e0940,          MAC_INLINED)
#define RVA_ADD_MONSTER_ATTACK                 ADDR(        0x0b8a00,          0x1003f83fc)
#define RVA_MONSTER_SET_ABILITY                ADDR(        0x1d9c90,          0x100491198)
#define RVA_ITEM_SET_ABILITY                   ADDR(        0x17e530,          MAC_INLINED)
#define RVA_ITEM_ADD_ABILITY                   ADDR(        0x17e600,          MAC_INLINED)
#define RVA_PARSER_SPELLWEAPON50S_PATH         ADDR(        0x1bd34b,          0x100530804)
#define RVA_PARSER_SPELLWEAPON_PATH            ADDR(        0x1bd3cb,          0x10053089c)
#define RVA_PARSER_SPELLWEAPONBONUS_PATH       ADDR(        0x1bd44b,          0x100530938)
#define RVA_PARSER_SPELLWEAPON50_PATH          ADDR(        0x1bd4cb,          0x1005309b4)
#define RVA_PARSER_SPELLWEAPONSINGLE_PATH      ADDR(        0x1bd54b,          0x100530a4c)
#define RVA_ITEM_ABILITY_LINES                 ADDR(        0x17f2a0,          MAC_INLINED)
#define RVA_ITEM_PATH_BOOST_BOUND              ADDR_DIFFERS(0x17f4cb,          0x100525b38)
#define RVA_ITEM_PATH_BOOST_BOUND_2            ADDR_DIFFERS(0x17f4cb,          0x100525b84)
#define RVA_ITEM_PATH_BOOST_BOUND_3            ADDR_DIFFERS(0x17f4cb,          0x100525bd0)
#define RVA_ITEM_PATH_BOOST_BOUND_4            ADDR_DIFFERS(0x17f4cb,          0x100525c18)
#define RVA_UNIT_DEBUG_SCREEN                  ADDR(        0x1cfa80,          MAC_INLINED)
#define RVA_UNIT_DEBUG_PATH_BOUND              ADDR(        0x1cfd9d,          0x100526f08)
#define RVA_SPELL_BROWSER_PATH_BOUND           ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_STATS_SPELL_ATTACK_PATH_BOUND      ADDR(        0x1d6436,          0x1005238e4)
#define RVA_STATS_KNOWN_SPELLS_PATH_LOOP       ADDR(        0x1d77a8,          0x10052413c)
#define RVA_LIBRARY_PATH_LOOP                  ADDR(        0x2de448,          0x10035d6d0)
#define RVA_LEARN_START_SPELLS_PATH_LOOP       ADDR(        0x2e64ec,          0x1004bcba8)
#define RVA_CHOOSE_MEMORIZED_PATH_LOOP         ADDR(        0x2e6dc0,          0x1004bdc34)
#define RVA_SPELL_BUG_CHECK_PATH_BOUND         ADDR(        0x12acb9,          0x1004648d4)
#define RVA_MASTERY_TRANSFORM                  ADDR(        0x2b2e20,          0x10055d5a0)
#define RVA_RITUAL_MASTERY_PATH_LOOP           ADDR(        0x2c698d,          0x10055fb20)
#define RVA_RITUAL_TRANSFORM_PATH_LOOP         ADDR(        0x2c6a32,          0x10055fd00)
#define RVA_PATH_NAMES_READ_ITEM_BOOST         ADDR_DIFFERS(0x17f4d0,          0x100525b40)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_2       ADDR_DIFFERS(0x17f4d0,          0x100525b8c)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_3       ADDR_DIFFERS(0x17f4d0,          0x100525bd8)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_4       ADDR_DIFFERS(0x17f4d0,          0x100525c20)
#define RVA_PATH_NAMES_READ_STATS_LEVEL        ADDR_DIFFERS(0x1d6459,          0x1005238f4)
#define RVA_PATH_NAMES_READ_STATS_KNOWN        ADDR_DIFFERS(0x1d6d80,          0x100524284)
#define RVA_PATH_NAMES_READ_LIBRARY            ADDR_DIFFERS(0x2de171,          0x10035d7d4)
#define RVA_PATH_NAMES_READ_LIBRARY_2          ADDR(        0x2de1a4,          MAC_NOT_FOUND)
#define RVA_PATH_NAMES_READ_DEBUG_UNIT         ADDR_DIFFERS(0x1cfda3,          0x100526ea8)
#define RVA_PATH_NAMES_READ_SPELL_BROWSER      ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_SPELL_BUG_CHECK_SPELL_LOOP_SKIP    ADDR_DIFFERS(0x12ac90,          0x1004648d4)
#define RVA_SPELL_BUG_CHECK_SPELL_LOOP_END     ADDR(        0x12ad73,          0x100464950)
#define RVA_UNIT_COMMANDER_INDEX               ADDR(        0x1299b0,          MAC_INLINED)
#define RVA_MASTERY_COMMANDER_CALL             ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_MASTERY_SNAPSHOT_CALL              ADDR(        0x2b2eb6,          0x10055d620)
#define RVA_MASTERY_CHOOSE_MEMORIZED_CALL      ADDR(        0x2b339d,          0x10055d78c)
#define RVA_LIBRARY_MASTERY_CALL               ADDR(        0x2de666,          0x10035dcb8)
#define RVA_RITUAL_MASTERY_COMMANDER_CALL      ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_RITUAL_MASTERY_SNAPSHOT_CALL       ADDR(        0x2c698d,          0x10055fb20)
#define RVA_RITUAL_MASTERY_MEMORIZE_CALL       ADDR(        0x2c698d,          0x10055fb20)
#define RVA_RITUAL_TRANSFORM_SNAPSHOT_CALL     ADDR(        0x2c6a25,          0x10055fcec)
#define RVA_RITUAL_TRANSFORM_FIND_MONSTER_CALL ADDR(        0x2c6a43,          0x10055fd10)
#define RVA_RITUAL_TRANSFORM_MEMORIZE_CALL     ADDR(        0x2c6ac4,          0x10056073c)
#define RVA_LEARN_START_SPELLS_CALL            ADDR(        0x0beddd,          0x1003ba3d0)
#define RVA_LEARN_START_SPELLS_CALL_2          ADDR(        0x10f803,          0x10046db60)
#define RVA_LEARN_START_SPELLS_CALL_3          ADDR(        0x129f99,          0x10046f1d4)
#define RVA_LEARN_START_SPELLS_CALL_4          ADDR(        0x1240ae,          0x10046a670)
#define RVA_LEARN_START_SPELLS_CALL_5          ADDR(        0x1870a5,          0x1004ee3b8)
#define RVA_LEARN_START_SPELLS_CALL_6          ADDR(        0x1873cc,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL              ADDR(        0x0bede5,          0x1003ba3d8)
#define RVA_CHOOSE_MEMORIZED_CALL_2            ADDR(        0x110218,          0x10046dec4)
#define RVA_CHOOSE_MEMORIZED_CALL_3            ADDR(        0x11032e,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_4            ADDR(        0x12a323,          0x10046f1dc)
#define RVA_CHOOSE_MEMORIZED_CALL_5            ADDR(        0x1240b6,          0x10046a678)
#define RVA_CHOOSE_MEMORIZED_CALL_6            ADDR(        0x1d8605,          0x100527258)
#define RVA_CHOOSE_MEMORIZED_CALL_7            ADDR(        0x1f6fef,          0x1004fb088)
#define RVA_CHOOSE_MEMORIZED_CALL_8            ADDR(        0x2e291e,          0x100352e18)
#define RVA_CHOOSE_MEMORIZED_CALL_9            ADDR(        0x2e4aa9,          0x1004befa0)
#define RVA_CHOOSE_MEMORIZED_JUMP              ADDR_DIFFERS(0x2e439b,          0x1004bf5a8)
#define RVA_CHOOSE_MEMORIZED_JUMP_2            ADDR(        0x2e72c9,          MAC_INLINED)
#define RVA_CHOOSE_MEMORIZED_CALL_10           ADDR(        0x070606,          0x1004d51d4)
#define RVA_CHOOSE_MEMORIZED_CALL_11           ADDR(        0x1870b1,          0x1004ee3d0)
#define RVA_CHOOSE_MEMORIZED_CALL_12           ADDR(        0x1873d7,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_13           ADDR(        0x1843f0,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_14           ADDR(        0x1844b4,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_15           ADDR(        0x184541,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_16           ADDR(        0x2b3367,          MAC_NOT_FOUND)
#define RVA_LIBRARY_KNOWN_COUNT_CALL           ADDR(        0x2de08e,          MAC_INLINED)
#define RVA_LIBRARY_ROWS_END_CALL              ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_LIBRARY_ROWS_END_LAYOUT_CALL       ADDR(        0x2de451,          MAC_INLINED)
#define RVA_STATS_KNOWN_COUNT_CALL             ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_STATS_KNOWN_END_CALL               ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_STATS_KNOWN_END_LAYOUT_CALL        ADDR(        0x1d79f7,          MAC_INLINED)
#define RVA_STATS_KNOWN_BOXES_SKIP             ADDR(        0x1d6cb4,          MAC_INLINED)
#define FRAME_STATS_SCREEN_UNIT                FRAME_OFFSET(0x000044,          MAC_INLINED)
#define FRAME_STATS_SCREEN_STRIPE              FRAME_OFFSET(0x000048,          MAC_INLINED)
#define RVA_TITLE_CELL                         ADDR(        0x03a000,          MAC_INLINED)
#define RVA_HEADER_TEXT                        ADDR(        0x039f40,          0x10033d3c4)
#define RVA_TEXT_CELL                          ADDR(        0x039be0,          MAC_INLINED)
#define RVA_SPELL_NAME_BUTTON                  ADDR(        0x039620,          MAC_INLINED)
#define RVA_MEMORIZE_ALLOWANCE                 ADDR(        0x2e4750,          MAC_INLINED)
#define RVA_MEMORIZED_SLOT                     ADDR(        0x2e4700,          0x1004be9a0)
#define RVA_SPELL_RANGE_BONUS                  ADDR(        0x31eed0,          0x10056c338)
#define RVA_DAMAGE_TEXT                        ADDR(        0x1cec90,          0x100527328)
#define RVA_UNIT_OWNER                         ADDR(        0x1292f0,          MAC_INLINED)
#define RVA_GET_UNIT_X                         ADDR(        0x329e40,          MAC_INLINED)
#define RVA_GET_UNIT_Y                         ADDR(        0x32a770,          MAC_INLINED)
#define RVA_LIBRARY_LEVEL                      ADDR(        0x1b1990,          MAC_INLINED)
#define RVA_SQUARE_LIBRARY_LEVEL               ADDR(        0x1b1bd0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_ENCHANT_VALUE               ADDR(        0x2be7b0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MAX_HP                        ADDR(        0x325eb0,          0x10056aaf8)
#define RVA_UNIT_FLAGS                         ADDR(        0x328f10,          0x100569f38)
#define RVA_UNIT_AP_LEFT                       ADDR(        0x3206b0,          MAC_INLINED)
#define UNIT_AP_USED                           0x1c
#define UNIT_FLAG_SLOW                         0x8
#define UNIT_FLAG_FAST                         0x10
#define UNIT_FLAG_FLY_OR_FLOAT                 0x81
#define UNIT_FLAG_WATER_MOVE                   0x40
#define UNIT_FLAG_MT_NOLAND                    0x200
#define UNIT_FLAG_PASSWALL                     0x100000000ULL
#define UNIT_FLAG_SHIPMOVE                     0x10000000000ULL
#define UNIT_FLAG_TUNNEL                       0x40000000000ULL
#define RVA_UNIT_HAS_FLAG_4                    ADDR(        0x1d9be0,          MAC_INLINED)
#define RVA_COUNT_MEMORIZED_OF_TYPE            ADDR(        0x2e3f50,          MAC_INLINED)
#define RVA_PARSER_LEVELUP_EFFECT              ADDR(        0x1c4377,          0x10053776c)
#define RVA_PARSER_LEVELUPMON_EFFECT           ADDR(        0x1c4390,          0x100537788)
#define RVA_ADD_KNOWN_SPELL                    ADDR(        0x2e6f10,          0x1004bc5c0)
#define RVA_RANDOM_SPELL                       ADDR(        0x2e4030,          0x1004bafb8)
#define RVA_RANDOM_SPELL_NOT_KNOWN             ADDR(        0x2e4280,          0x1004bed00)
#define RVA_LEARN_START_SPELLS                 ADDR(        0x2e5910,          0x1004bcb40)
#define RVA_LEARN_RANDOM_SPELL                 ADDR(        0x2e4a40,          0x1004bed9c)
#define RVA_CHOOSE_MEMORIZED                   ADDR(        0x2e6560,          0x1004bdbbc)
#define RVA_RANDOM_MEMORIZED_SPELL             ADDR(        0x2e56e0,          0x1004bd43c)
#define RVA_MEMORIZE_SPELL                     ADDR(        0x2e7110,          0x1004bc680)
#define RVA_UNMEMORIZE_SPELL                   ADDR(        0x2e45f0,          MAC_INLINED)
#define RVA_UNMEMORIZE_SPELL_BOUND             ADDR_DIFFERS(0x2e4610,          0x1004bdcc8)
#define RVA_UNMEMORIZE_SPELL_BOUND_2           ADDR_DIFFERS(0x2e4610,          0x1004be7b8)
#define RVA_UNMEMORIZE_SPELL_BOUND_3           ADDR_DIFFERS(0x2e4610,          0x100526b54)
#define RVA_COUNT_KNOWN_SPELLS                 ADDR(        0x2e4780,          0x1004bcf98)
#define RVA_COUNT_SPELLS                       ADDR(        0x2e4130,          MAC_INLINED)
#define RVA_SPELL_INFO_BOX                     ADDR(        0x2e4410,          MAC_INLINED)
#define RVA_COUNT_MEMORIZED_SPELLS             ADDR(        0x2e43b0,          0x1004bd100)
#define RVA_LEARN_PROMOTION_SPELLS             ADDR(        0x2e4330,          0x1004bf3dc)
#define RVA_UNIT_FIRST_SPELL_PATH              ADDR(        0x2e7210,          0x1004befc4)
#define RVA_RANDOM_1_TO_N                      ADDR(        0x0589c0,          0x1003373e0)
#define RVA_COMMANDER_ADD_AP_USED              ADDR(        0x12c650,          0x1004817a8)
#define RVA_UNIT_ADD_AP_USED                   ADDR(        0x118990,          MAC_INLINED)
#define RVA_FLUSH_COMMANDER_CACHES             ADDR(        0x129af0,          MAC_INLINED)
#define RVA_COMMANDER_ARMY_AP_LEFT             ADDR(        0x328d90,          0x10056a940)
#define COMMANDER_AP_CACHE                     0xa3
#define RVA_COMMANDER_CACHE_INTS               ADDR(        0x6605360,         0x106cd5de8)
#define RVA_START_SPELLS_MORE_SPELLS_CALL      ADDR(        WIN_NO_CALL,       0x1004bcc98)
#define RVA_START_SPELLS_FORTUNE_CHECK         ADDR_DIFFERS(0x2e62d2,          0x1004bcd88)
#define RVA_START_SPELLS_FORTUNE_CHECK_2       ADDR(        WIN_NO_EQUIVALENT, 0x1004bcf60)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL         ADDR_DIFFERS(0x2c69ac,          0x10055fb38)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL_2       ADDR_DIFFERS(0x2c69cb,          0x10055fb38)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL_3       ADDR_DIFFERS(0x2c69ea,          0x10055fb38)
#define RVA_RITUAL_SUMMON_LEARN_CALL           ADDR(        0x2c64f7,          0x100560d48)
#define RVA_RITUAL_CAN_CAST_NEWSPELL           ADDR(        0x2ca7c7,          0x10055c44c)
#define RVA_RITUAL_CAN_CAST_NEWSPELL_2         ADDR(        0x2ca82e,          0x10055c47c)
#define RVA_RITUAL_CAN_CAST_NEWSPELL_3         ADDR(        0x2ca88e,          0x10055c4ac)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL         ADDR(        WIN_NO_CALL,       0x1004e6af0)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL_2       ADDR(        WIN_NO_CALL,       0x1004e6b58)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL_3       ADDR(        WIN_NO_CALL,       0x1004e6bc0)
#define RVA_LIBRARY_ROW_PATH_LEVEL_CALL        ADDR(        0x2de0a9,          0x10035d6e0)
#define RVA_LIBRARY_LEARN_CALL                 ADDR_DIFFERS(0x2de856,          0x10035e138)
#define RVA_USE_LIBRARY_LEARN_CALL             ADDR(        0x2e3a6e,          0x10035d02c)
#define RVA_AI_USE_LIBRARY_CALL                ADDR(        0x07ab45,          0x1004d219c)
#define RVA_PRIEST_PROMOTION_LEARN_CALL        ADDR(        0x2e2915,          0x100352d00)
#define RVA_DOATTACK_PATH_BONUS_CALL           ADDR(        0x0d47eb,          0x1003ebf38)
#define RVA_DOATTACK_SPELL_PICK_CALL           ADDR(        0x0d48c4,          0x1003ec004)
#define RVA_DOATTACK_CAST_COUNT                ADDR_DIFFERS(0x0d4937,          0x1003ec06c)
#define RVA_STATS_KNOWN_PATH_LEVEL_CALL        ADDR(        0x1d6d2f,          0x1005241b0)
#define RVA_FIND_SPELL                         ADDR(        0x2e7020,          MAC_INLINED)
#define RVA_SPELL_USEFUL                       ADDR(        0x2e57f0,          0x1004bfbe0)
#define RVA_SPELL_FLAG_LINES                   ADDR(        0x2e4810,          0x1004bf5ac)
#define RVA_ARMY_HAS_SPELL_FLAG                ADDR(        0x2e4ae0,          0x1004bfce0)
#define RVA_SPELL_BROWSER                      ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_SPELL_BROWSER_BOUND                ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_LISTSPELLS_BOUND                   ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_NET_RECV_MEMORIZED_BOUND           ADDR_DIFFERS(0x1f6fbb,          0x1004fadd0)
#define RVA_STATS_SPELL_ROW_ID                 ADDR_DIFFERS(0x1d714f,          0x100524590)
#define RVA_STATS_SPELL_ROW_DECODE             ADDR(        0x1d8466,          0x100524ef0)
#define RVA_STATS_SPELL_ROW_RIGHT_BOUND        ADDR_DIFFERS(0x1d846c,          0x100524efc)
#define RVA_STATS_SPELL_ROW_RIGHT_SHIFT        ADDR(        WIN_NO_EQUIVALENT, 0x100524ef8)
#define RVA_STATS_SPELL_ROW_LEFT_OFFSET        ADDR_DIFFERS(0x1d8477,          0x100524f04)
#define RVA_STATS_SPELL_ROW_LEFT_SHIFT         ADDR(        WIN_NO_EQUIVALENT, 0x100524f0c)
#define RVA_STATS_SPELL_ROW_LEFT_BOUND         ADDR_DIFFERS(0x1d847d,          0x100524f10)
#define RVA_STATS_SPELL_ROW_LEFT_BOUND_2       ADDR(        WIN_NO_EQUIVALENT, 0x100525688)
#define RVA_STATS_SPELL_ROW_LEFT_SPELL         ADDR_DIFFERS(0x1d84c3,          0x100524f04)
#define RVA_STATS_SPELL_ROW_INFO_SPELL         ADDR_DIFFERS(0x1d8580,          0x100524f24)
#define RVA_STATS_SPELL_ROW_INFO_SHIFT         ADDR(        WIN_NO_EQUIVALENT, 0x100524f2c)
#define RVA_STATS_SPELL_ROW_INFO_BOUND         ADDR(        WIN_NO_EQUIVALENT, 0x100524f30)
#define RVA_FIND_SPELL_TABLE_REF               ADDR_DIFFERS(0x2e704b,          0x1004bc5ec)
#define RVA_NET_RECV_MEMORIZED_BRANCH          ADDR_DIFFERS(0x1f6fc1,          0x1004fadd4)
#define RVA_SLOW_SPELL_ENCODE                  ADDR_DIFFERS(0x0d4a6a,          0x1003ece30)
#define RVA_SLOW_SPELL_DECODE                  ADDR(        0x0d4853,          0x1003ebfa0)
#define RVA_AI_ZAP_SLOW_SPELL_CHECK            ADDR_DIFFERS(0x0cd49d,          0x1003e528c)
#define RVA_SPELL_BUG_CHECK                    ADDR(        0x12a400,          0x100464248)
#define RVA_NO_CHECKS                          ADDR(        0x582394c,         MAC_NOT_LOOKED_UP)
#define RVA_PREF_END_TURN_WARNING              ADDR(        0x5828118,         MAC_NOT_LOOKED_UP)
#define RVA_UNIT_ALIVE_AT                      ADDR(        0x3265e0,          MAC_NOT_FOUND)
#define RVA_DOATTACK                           ADDR(        0x0d4520,          0x1003eb9c4)
#define RVA_GET_TARGET_SQUARE                  ADDR(        0x0d6160,          0x1003bf68c)
#define RVA_LIBRARY_SCREEN                     ADDR(        0x2ddb80,          0x10035d110)
#define RVA_USE_LIBRARY                        ADDR(        0x2e38e0,          0x10035cd88)
#define RVA_ITEM_USE                           ADDR(        0x184060,          0x1004e6828)
#define RVA_FIGHT_THE_FIGHT                    ADDR(        0x0f2270,          0x1003e0078)
#define RVA_FIGHT_ZAP_CASTABLE_CALL            ADDR(        0x0f359c,          0x1003e417c)
#define RVA_AI_ZAP_CHOICE                      ADDR(        0x0cca00,          MAC_INLINED)
#define RVA_AI_ZAP_CASTABLE_CALL               ADDR(        0x0ccb6c,          0x1003e4308)
#define RVA_BATTLE_ITEM_EFFECTS                ADDR(        0x0d6e20,          MAC_INLINED)
#define RVA_ITEM_CASTABLE_SPELL                ADDR(        0x183610,          0x1004e8fc8)
#define RVA_ITEM_INFO_SCREEN                   ADDR(        0x17eca0,          MAC_INLINED)
#define RVA_ITEM_SPELL_LINE                    ADDR(        0x17fe00,          MAC_NOT_FOUND)
#define RVA_ITEM_SPELL_STATS                   ADDR(        0x17fd80,          MAC_NOT_FOUND)
#define RVA_KNOWN_SPELL_SLOT_BY_NAME           ADDR(        0x2e3d10,          MAC_NOT_FOUND)
#define RVA_MEMORIZE_SPELL_BY_NAME             ADDR(        0x2e4000,          MAC_NOT_FOUND)
#define RVA_UNIT_KNOWN_SPELLS                  ADDR(        0xfb8e7a0,         0x110049890)
#define RVA_UNIT_MEMORIZED_SPELLS              ADDR(        0xfb8e818,         0x110049908)
#define RVA_UNIT_SLOW_SPELL                    ADDR(        0xfb8e8a4,         0x110049994)
#define RVA_SAVE_UNIT                          ADDR(        0x18c2d0,          0x1004f09b0)
#define RVA_LOAD_UNIT                          ADDR(        0x18d5f0,          0x1004ef27c)
#define RVA_NET_SEND_UNITS                     ADDR(        0x1f29d0,          0x10050e188)
#define RVA_FIND_WEAPON                        ADDR(        0x0b8ad0,          0x1003f8388)

#define RVA_UNIT_ITEMS                         ADDR(        0xfb8e854,         0x110049944)
#define UNIT_ITEM_SLOTS                        8
#define RVA_UNIT_TEMP_ABILITY_IDS              ADDR(        0xfb8e86e,         0x11004995e)
#define RVA_UNIT_TEMP_ABILITY_VALUES           ADDR(        0xfb8e880,         0x110049970)
#define RVA_SET_TEMP_ABILITY                   ADDR_DIFFERS(0x1189d0,          0x10046875c)
#define RVA_GET_TEMP_ABILITY                   ADDR(        0x117280,          0x100468858)
#define RVA_COMMANDER_KNOWN_RITUALS            ADDR(        0x11d3c84c,        0x10ce0cbbc)
#define RVA_GROUND_ITEMS                       ADDR(        0xf957fc0,         0x10cb0e1c4)
#define GROUND_ITEMS_PER_SQUARE                6
#define MAP_COLUMN_SQUARES                     160
#define MAP_COLUMNS_MAX                        1000
#define RVA_ADD_GROUND_ITEMS                   ADDR(        0x26c5b0,          0x1003f88b4)
#define RVA_SQUARE_RECORDS                     ADDR(        0xa20ac40,         0x10d011c68)
#define SQUARE_SIZE                            0x13c
#define SQUARE_ARMY_OWNER                      0x3a
#define SQUARE_OWNER                           0x42
#define SQUARE_GEMS                            0xa3
#define SQUARE_GEM_KINDS                       4
#define SQUARE_TERRAIN_TYPE                    0x20
#define RVA_SAVE_SQUARE                        ADDR(        0x18bf00,          0x1004f15a8)
#define RVA_LOAD_SQUARE                        ADDR(        0x18cbc0,          0x1004ee864)
#define RVA_SAVE_ENCHANTS                      ADDR(        0x18bc40,          0x1004f138c)
#define RVA_SHOP_LAST                          ADDR(        0x11c14528,        0x10cce486c)
#define RVA_SHOP_OFFERS                        ADDR(        0x11c1452c,        0x10cce4870)
#define SHOP_OFFERS_MAX                        160
#define RVA_SHOP_OFFER_COUNTS                  ADDR(        0x11c147ac,        0x10cce4af0)
#define RVA_SHOP_ADD                           ADDR(        0x0af6e0,          0x100362748)
#define RVA_SHOP_BUILD                         ADDR(        0x0a9210,          MAC_NOT_LOOKED_UP)
#define RVA_SHOP_ADD_ITEM_OFFER_CALL           ADDR(        0x0aeb54,          MAC_NOT_LOOKED_UP)
#define RVA_SPECIAL_LOCATION_RECRUITS          ADDR(        0x1a29b40,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_TERRAIN_CITADEL         ADDR(        0x1b0ba0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_SQUARES_WITH_FLAGS           ADDR(        0x331420,          MAC_NOT_LOOKED_UP)
#define RVA_LOAD_SHOP                          ADDR(        0x189c60,          MAC_INLINED)
#define RVA_FIND_ITEM                          ADDR(        0x1850f0,          0x1004e5994)
#define RVA_AUTO_RECRUIT_SQUARES               ADDR(        0x11c142a8,        0x10cce45ec)
#define RVA_AUTO_RECRUIT_MONSTERS              ADDR(        0x11c143a8,        0x10cce46ec)
#define AUTO_RECRUIT_ORDERS_MAX                64
#define RVA_AUTO_RECRUIT                       ADDR(        0x166b60,          MAC_INLINED)
#define RVA_MESSAGE_ATTACH_SIZES               ADDR(        0xe4a5cd8,         0x1085bb0a8)
#define RVA_MESSAGE_ATTACHMENTS                ADDR(        0xe4a5ce8,         0x1085bb0b8)
#define RVA_MESSAGE_SCREEN                     ADDR(        0x1e1f20,          0x1004c26cc)
#define RVA_TURN_MESSAGES_CALL                 ADDR(        0x094ab7,          0x10037bc24)
#define RVA_VIEW_MESSAGES_CALL                 ADDR(        0x0978b8,          0x100381204)
#define RVA_MESSAGES_WAIT_CALL                 ADDR(        0x1e26f9,          0x1004c2f24)
#define RVA_LOAD_BATTLE_REPLAY                ADDR(        0x0f4260,          MAC_INLINED)
#define RVA_LOAD_PART_STATS                    ADDR(        0x0ba740,          0x1003f7d64)
#define RVA_SAVE_PART_STATS                    ADDR(        0x0baac0,          0x1003df968)
#define RVA_SAVE_BATTLE_REPLAY                 ADDR(        0x0f1d40,          MAC_NOT_FOUND)
#define RVA_NET_SEND_SQUARE                    ADDR(        0x1f31c0,          0x10050ca30)
#define RVA_NET_SEND_UNIT                      ADDR(        0x1f25c0,          0x10050e5b4)
#define RVA_NET_SEND_MEMORIZED                 ADDR(        0x1f0f40,          0x100510e08)
#define RVA_NET_SEND_AUTO_RECRUIT              ADDR(        0x1f2b90,          0x100510af4)
#define RVA_NET_GIVE_ITEM                      ADDR(        0x1f1030,          MAC_INLINED)
#define RVA_FIND_SHOP_OFFER                    ADDR(        0x11ed60,          MAC_INLINED)
#define RVA_SHOP_OFFER_MESSAGE                 ADDR(        0x0af540,          0x1003629e0)
#define RVA_SHOP_COSTS                         ADDR(        0x11c14a2c,        0x10cce4d70)
#define SHOP_COST_COUNT                        19
#define RVA_SHOP_OFFER_FLAGS                   ADDR(        0x11c179ac,        0x10cce7cf0)
#define SHOP_FLAG_ONCE                         0x1
#define SHOP_FLAG_AI_IMPORTANT                 0x2
#define SHOP_FLAG_EXTRA                        0x4
#define SHOP_FLAG_AI_NEVER                     0x10
#define SHOP_FLAG_AI_FIRST_TURNS               0x200
#define SHOP_OFFER_TEXT_SIZE                   0x30
#define SQUARE_FLAG_RECRUITED                  0x1
#define RVA_SHOP_OFFER_TERRAIN                 ADDR(        0x11c17c2c,        0x10cce7f70)
#define RVA_SHOP_OFFER_NAMES                   ADDR(        0x11c17eac,        0x10cce81f0)
#define RVA_SHOP_OFFER_LIMITS                  ADDR(        0x11c19cac,        0x10cce9ff0)
#define RVA_ITEM_SLOT_TYPE                     ADDR(        0x1be6764,         0x10489382c)
#define RVA_GIVE_ITEM                          ADDR(        0x180480,          0x1004e5058)
#define RVA_GIVE_ITEM_AND_ENFORCE              ADDR(        0x185370,          0x1004e5754)
#define RVA_SHOP_ITEM_CAN_RECEIVE              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_SHOP_ITEM_RECEIVER                 ADDR(        0x183140,          MAC_NOT_LOOKED_UP)
#define RVA_ENFORCE_ITEMS                      ADDR(        0x1837c0,          0x1004e5794)
#define RVA_CAN_EQUIP_ITEM                     ADDR(        0x1851c0,          0x1004e4730)
#define RVA_REMOVE_ENCHANT                     ADDR(        0x2be1b0,          0x1005641e4)
#define RVA_REBUILD_ENCHANT_FLAGS              ADDR(        0x2bfdc0,          0x1005659fc)
#define RVA_LOAD_GAME_ENCHANT_FLAGS            ADDR(        0x186fa2,          0x1004ee314)
#define RVA_SQUARE_FLAGS                       ADDR(        0xa20ace8,         0x10d011d10)
#define RVA_FREE                               ADDR(        0x05e710,          0x1003347a4)
#define RVA_MESSAGE_ATTACH                     ADDR(        WIN_INLINED,       0x1004c598c)
#define RVA_MESSAGE_DETACH                     ADDR(        0x1e0f90,          MAC_INLINED)
#define RVA_BATTLE_ATTACH_CALL                 ADDR(        WIN_NO_CALL,       0x1003deb2c)
#define RVA_BATTLE_REPLAY_WRITE                ADDR(        0x0f5b90,          0x1003dd734)
#define RVA_SAVE_FILE_VERSION                  ADDR(        0x5822f68,         0x10857c9a0)
#define RVA_BATTLE_SEED                        ADDR(        0x11fb1840,        0x108564670)
#define RVA_BATTLE_PLAYER_A                    ADDR(        0x43e28c,          0x101cce59c)
#define RVA_BATTLE_PLAYER_B                    ADDR(        0x43e2a4,          0x101cce598)
#define RVA_BATTLE_X                           ADDR(        0x43e0a4,          0x104904c84)
#define RVA_BATTLE_Y                           ADDR(        0x43dffc,          0x104904c88)
#define RVA_FILE_WRITE_INT64                   ADDR(        0x0582e0,          MAC_INLINED)
#define RVA_FILE_READ_INT64                    ADDR(        0x0587f0,          MAC_INLINED)
#define RVA_FILE_WRITE_ENDED_SHORTS            ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_FILE_READ_ENDED_SHORTS             ADDR(        WIN_INLINED,       MAC_INLINED)
#define REPLAY_MAGIC                           0x775615
#define REPLAY_VERSION                         0xcb
#define PART_STATS_MAGIC                       0x77491
#define PART_STATS_VERSION_MAX                 1
#define PART_STATS_RECORD_SIZE                 16
#define SAVED_UNIT_SIZE                        270
#define SAVED_UNIT_KNOWN_SPELLS                0x20
#define SAVED_UNIT_MEMORIZED_SPELLS            0x98
#define SAVED_UNIT_ITEMS                       0xd4
#define SAVED_UNIT_MONSTER                     0xe8
#define SAVED_UNIT_TEMP_ABILITY_IDS            0xee
#define SAVED_UNIT_TEMP_ABILITY_VALUES         0xf6
#define SAVED_COMMANDER_AFTER_NAME             0x20
#define SAVED_SQUARE_BEFORE_CORPSES            0x2e
#define SAVED_SQUARE_AFTER_CORPSES             0xcd

#define RVA_MAP_FILES                          ADDR(        0xe77d200,         0x10c127aa0)
#define RVA_MAP_FILE_COUNT                     ADDR(        0x1a96054,         0x1008735d4)
#define RVA_TRANSLATION_HASHES                 ADDR(        0x58ffe80,         0x105b8fc50)
#define RVA_TRANSLATION_STRINGS                ADDR(        0x57c62d0,         0x105b68b50)
#define RVA_TRANSLATION_COUNT                  ADDR(        0x435d58,          0x100857670)
#define RVA_TRANSLATION_HASH                   ADDR(        0x05c2d0,          MAC_INLINED)
#define RVA_TRANSLATION_NORMALIZE              ADDR(        0x05c4b0,          MAC_INLINED)
#define RVA_DUMPSTRINGS_FILE                   ADDR(        0x57eddb0,         0x105b68188)

// ---- Rituals and the map ------------------------------------------------------------------------
#define RVA_RITUAL_SCREEN                      ADDR(        0x2ae490,          0x10035e7a4)
#define RVA_RITUAL_CAN_CAST                    ADDR(        0x2c9c40,          0x10055bf14)
#define RVA_RITUAL_LOCATION_CHECK              ADDR(        0x2cd170,          0x1005598d4)
#define RVA_RITUAL_ROW                         ADDR_DIFFERS(0x2d6920,          0x100352118)
#define RITUAL_COST_TYPE_MAX                   50
#define RVA_RESOURCE_NAMES                     ADDR(        0x3f9e90,          0x1008482b8)
#define RVA_INCOME_NAMES                       ADDR(        0x344360,          0x100847b68)
#define INCOME_NAME_COUNT                      19
#define RVA_RITUAL_CHECK_COSTS                 ADDR(        0x2c3600,          0x100558740)
#define RVA_RITUAL_PAY_COSTS                   ADDR(        0x2c27f0,          0x100558b68)
#define RVA_RESOURCE_AVAILABLE                 ADDR(        0x3201f0,          0x1005695bc)
#define RVA_CAN_AFFORD_AT                      ADDR(        0x320300,          0x100569720)
#define RVA_PAY_RESOURCE_AT                    ADDR(        0x320430,          0x10056982c)
#define RVA_COUNT_EXPENDABLE_AT                ADDR(        0x2b5ca0,          0x100557d94)
#define RVA_COUNT_ABILITY_AT                   ADDR(        0x2c0d60,          0x10055824c)
#define RVA_REMOVE_ABILITY_UNITS_AT            ADDR(        0x2b5920,          0x100558418)
#define RVA_CORPSES_AT                         ADDR(        0x3355f0,          MAC_INLINED)
#define SQUARE_CORPSES                         0x2a
#define SQUARE_CORPSE_KINDS                    8
#define MONSTER_ABILITY_EXPENDABLE             0x1ae
#define MONSTER_ABILITY_EXPENDABLE_2           0x4ca
#define MONSTER_ABILITY_GOBLIN                 0x4b3
#define RVA_COMMANDERS                         ADDR(        0x11d3c830,        0x10ce0cba0)
#define COMMANDER_SIZE                         0xd4
#define COMMANDER_TABLE_MAX                    9990
#define COMMANDER_KNOWN_RITUALS_MAX            64
#define RVA_COMMANDER_NAMES                    ADDR(        0x11d3c800,        0x10ce0cb70)
#define COMMANDER_NAME_SIZE                    0x30
#define RVA_LAST_COMMANDER                    ADDR(        0x11d379d0,        0x10ce0cb6c)
#define RVA_COMMANDER_DONE                     ADDR(        0x11d3c8d2,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_SET_DONE                 ADDR(        0x0a0f10,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_NEXT_COMMANDER                ADDR(        0x0a0c10,          MAC_NOT_LOOKED_UP)
#define RVA_SELECT_NEXT_COMMANDER              ADDR(        0x0a0b70,          MAC_NOT_LOOKED_UP)
#define RVA_DROP_DEAD_FROM_GROUP               ADDR(        0x1df040,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_COMMANDERS_WITH_AP           ADDR(        0x09dba0,          MAC_NOT_LOOKED_UP)
#define RVA_SELECTED_COMMANDER                 ADDR(        0xfb2cbc8,         MAC_NOT_LOOKED_UP)
#define RVA_SELECTED_COMMANDER_GROUP           ADDR(        0xfb2cbc4,         MAC_NOT_LOOKED_UP)
#define COMMANDER_ORDER_GROUP                  0x4
#define RVA_COMMANDER_UNIT                     ADDR(        0x12c620,          MAC_INLINED)
#define RVA_UNIT_X                             ADDR(        0xfb8e794,         0x110049884)
#define UNIT_SIZE                              0x158
#define RVA_UNIT_LEADER                        ADDR(        0xfb8e790,         MAC_NOT_LOOKED_UP)
#define RVA_UNITS                              ADDR(        0xfb8e780,         0x110049870)
#define RVA_UNIT_MONSTER                       ADDR(        0xfb8e868,         0x110049958)
#define RVA_UNIT_COUNT                         ADDR(        0xfb8e760,         0x110049868)
#define RVA_UNIT_HP                            ADDR(        0xfb8e866,         0x110049956)
#define RVA_UNIT_PLAYER                        ADDR(        0xfb8e86a,         0x11004995a)
#define RVA_UNIT_XP                            ADDR(        0xfb8e864,         0x110049954)
#define RVA_UNIT_KILLS                         ADDR(        0xfb8e86c,         0x11004995c)
#define UNIT_COMMANDER                         0x10
#define UNIT_TABLE_MAX                         99000
#define MONSTER_FLAG_NO_COMMANDER              35
#define MONSTER_ABILITY_UNIQUE                 0x39
#define RVA_NEW_UNIT                           ADDR(        0x129be0,          0x10046ed48)
#define RVA_NEW_COMMANDER                      ADDR(        0x128590,          0x10046df50)
#define RVA_ALLOC_UNIT                         ADDR(        0x110530,          0x10046d6a4)
#define RVA_ALLOC_TEMP_UNIT                    ADDR(        0x110950,          MAC_INLINED)
#define RVA_RESERVED_UNIT                      ADDR(        0x1f2210,          MAC_INLINED)
#define RVA_RESERVED_COMMANDER                 ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_REBUILD_SQUARE_ARMY                ADDR(        0x18f830,          0x1004f01a0)
#define RVA_UNIT_FIT_TERRAIN                   ADDR(        0x115390,          0x100467970)
#define RVA_CONQUER_CHECK                      ADDR(        0x1afe50,          0x1004ab3e4)
#define RVA_UPDATE_SIGHT                       ADDR(        0x19aa90,          0x10049fa54)
#define RVA_UPDATE_SIGHT_AROUND                ADDR(        0x1af0a0,          MAC_INLINED)
#define RVA_RESET_SQUARE_CACHE                 ADDR(        0x1b3710,          0x100499080)
#define RVA_SIGHT_RAYS                         ADDR(        0x19c140,          MAC_NOT_LOOKED_UP)
#define RVA_SIGHT_BOX                          ADDR(        0x1adfa0,          MAC_NOT_LOOKED_UP)
#define RVA_MARK_SQUARE_IN_VIEW                ADDR(        0x1b0fd0,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_TRUE_TERRAIN_SEEN           ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_COPY_TEAM_SIGHT                    ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_REVEAL_MAP_FOR                     ADDR(        0x1b0a50,          MAC_NOT_LOOKED_UP)
#define RVA_SIGHT_CACHE_RESET                  ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SEEN_AT                     ADDR(        0x3358c0,          MAC_NOT_LOOKED_UP)
#define RVA_SEEN_NEIGHBOURS_COUNT              ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SIGHT_LEVEL                 ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_UNIT_VISIBLE_TO                    ADDR(        0x118140,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STEALTH_LEVEL                 ADDR(        0x31f920,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BLINDNESS                     ADDR(        0x325090,          MAC_NOT_LOOKED_UP)
#define RVA_FIRST_TEAM_UNIT_AT                 ADDR(        0x105100,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_UNIT_AT                     ADDR(        0x1051f0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_UNIT_AT                 ADDR(        0x31f860,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEAM_ABILITY                ADDR(        0x112770,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEAM_ABILITY_B              ADDR(        0x112c90,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TERRAIN_FLAGS               ADDR(        0x3354e0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_MARK_BOX                    ADDR(        0x19c060,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_MARK_DRAW                   ADDR(        0x19c730,          MAC_INLINED)
#define RVA_MONSTER_VARIANT                    ADDR(        0x108670,          0x100479870)
#define RVA_RECRUIT_BUY                        ADDR(        0x1080c0,          0x100479a5c)
#define RVA_POPULATE                           ADDR(        0x26c120,          0x1003f8b04)
#define RVA_POPULATE_NO_COMMANDER              ADDR(        0x272e10,          MAC_INLINED)
#define RVA_POPULATE_COUNT_FIRST               ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_POPULATE_COMMANDER                 ADDR(        0x272b60,          0x1003f8fac)
#define RVA_SUMMON_STRING                      ADDR(        0x171ea0,          0x10053e7b0)
#define RVA_SUMMON_STRING_FULL                 ADDR(        0x1786c0,          MAC_INLINED)
#define RVA_LAST_SPAWNED_UNIT                  ADDR(        0x1aeba34,         0x101cb6e28)
#define RVA_LAST_SPAWNED_COMMANDER             ADDR(        0x1ae8fbc,         0x100872b50)
#define RVA_EXECUTE_RITUAL_STRING_PICK         ADDR(        0x2c5ed2,          MAC_NOT_LOOKED_UP)
#define RVA_EXECUTE_RITUAL_SUMMON_CALL         ADDR(        0x2c6651,          MAC_NOT_LOOKED_UP)
#define RVA_EXECUTE_RITUAL_FARSUMMON_CALL      ADDR(        0x2c6702,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_STRING               ADDR(        0x2b3410,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_PERCENT_CURVE        ADDR(        0x2b3567,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_CONTROL_ROLL         ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_UNITS                ADDR(        0x2d6040,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_CONTROL_CHECK        ADDR(        0x2d6655,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FAR_SUMMON_UNITS            ADDR(        0x2d74a0,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_OPEN_DICE                   ADDR(        0x0589e0,          MAC_NOT_LOOKED_UP)
#define RVA_GET_START_RITUALS                  ADDR(        0x2c0130,          MAC_NOT_LOOKED_UP)
#define RVA_GET_START_RITUALS_CONTROLLER       ADDR(        0x2c0210,          MAC_NOT_LOOKED_UP)
#define RVA_START_RITUAL_SLOTS                 ADDR(        0x406dc0,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_MAY_LEARN                   ADDR(        0x2bc850,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_LEARN_RITUAL             ADDR(        0x2be0b0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_KNOWS_RITUAL             ADDR(        0x2be340,          MAC_NOT_LOOKED_UP)
#define RVA_LEARN_LEVEL_RITUALS                ADDR(        0x2bff60,          MAC_NOT_LOOKED_UP)
#define RVA_LEARN_NEW_SCHOOL_RITUALS           ADDR(        0x2bc530,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_MONSTER_WITH_ABILITY        ADDR(        0x1d9170,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_NEWRIT_CALL                 ADDR(        0x2c696a,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_GAINRIT_CALL                ADDR(        0x2c878f,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_TRANSFORMTARG               ADDR(        0x2c8272,          MAC_NOT_LOOKED_UP)
#define RVA_LIBRARY_MASTERY_LEARN_CALL         ADDR(        0x2de688,          MAC_NOT_LOOKED_UP)
#define RVA_LIBRARY_MASTERY_COST               ADDR(        0x2d09b0,          MAC_NOT_LOOKED_UP)
#define RVA_PROMOTE_GET_START_RITUALS_CALL     ADDR(        0x11015c,          MAC_NOT_LOOKED_UP)
#define RVA_NEW_COMMANDER_START_RITUALS_CALL   ADDR(        0x128805,          MAC_NOT_LOOKED_UP)
#define RITUAL_FLAG_FREE                       0x1
#define RITUAL_FLAG_NOSTART                    0x2
#define RITUAL_FLAG_NODEMON                    0x4
#define RITUAL_FLAG_RARESTART                  0x8
#define RITUAL_FLAG_NOTFORPOOR                 0x10
#define RITUAL_FLAG_START                      0x20
#define RITUAL_FLAG_NEXTTOO                    0x40
#define RITUAL_FLAG_NOUNDEAD                   0x80
#define RITUAL_FLAG_NOFEMALE                   0x100
#define RITUAL_FLAG_AFTERPREV                  0x200
#define RITUAL_FX_NEWRIT                       904
#define RITUAL_FX_TRANSFORMTARG                914
#define RITUAL_FX_GAINRIT                      916
#define RITUAL_FX_LEVELUP                      944
#define RITUAL_FX_LEVELUPMON                   945
#define RITUAL_FX_NOLEARNFX                    963
#define RITUAL_FX_PROMOTION                    974
#define RITUAL_FX_AIRARE                       1102
#define MONSTER_ABILITY_ALLRIT                 0x62
#define MONSTER_ABILITY_MASTERY                0x32b
#define MONSTER_ABILITY_LEVELUPMON_OK          0x32c
#define MONSTER_ABILITY_LIBMASTERY             0x32d
#define MONSTER_ABILITY_MONTAG_MASTERY         0x32e
#define MONSTER_ABILITY_MONTAG                 0x320
#define RVA_EXECUTE_RITUAL_SIGHT_CALL          ADDR(        0x2c9bc0,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_SIGHT_WHOLE_MAP             ADDR(        0x1b1960,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_SIGHT_HUMAN_CONTROLLER      ADDR(        0x19b6b0,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_DRAW_SQUARES                ADDR(        0x1b26a0,          MAC_NOT_LOOKED_UP)
#define RVA_CAST_RITUAL_BY_NAME                ADDR(        0x2be130,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SCREEN_EXECUTE_CALL         ADDR(        0x2b0511,          MAC_NOT_LOOKED_UP)
#define RVA_REPEAT_RITUAL_EXECUTE_CALL         ADDR(        0x2c2216,          MAC_NOT_LOOKED_UP)
#define RVA_STARS_RELIABILITY                  ADDR(        0xf07afac,         MAC_NOT_LOOKED_UP)
#define RVA_STARS_POWER                        ADDR(        0xf07afae,         MAC_NOT_LOOKED_UP)
#define RVA_STARS_MONTHS                       ADDR(        0xf07afb0,         MAC_NOT_LOOKED_UP)
#define RVA_STARS_FLAGS                        ADDR(        0xf07afb4,         MAC_NOT_LOOKED_UP)
#define RVA_STARS_READING_TEXT                 ADDR(        0x2dcac0,          MAC_NOT_LOOKED_UP)
#define RITUAL_FX_DEFCTRL                      901
#define RITUAL_FX_VARCOST                      902
#define RITUAL_FX_ASTROSUM                     908
#define RITUAL_FX_TERRBOOST                    948
#define RITUAL_FX_TERRSCALE50                  949
#define RITUAL_FX_SACRSCALE                    950
#define RITUAL_FX_SUM0CHANCE                   951
#define RITUAL_FX_SUM0SNOW                     956
#define RITUAL_FX_SETPLAYER                    961
#define RITUAL_FX_FAILPLAYER                   966
#define RITUAL_FX_FARSUMMON                    968
#define MONSTER_ABILITY_CTRLCHANCE             0x321
#define RVA_UNIT_NO_COMMANDER_FLAG             ADDR(        0x1d9aa0,          MAC_INLINED)
#define RVA_RITUAL_SUMMON_STRING               ADDR(        0x2b3410,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_CALL                 ADDR(        0x2c6651,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FARSUMMON_CALL              ADDR(        0x2c6702,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_TO_UNIT                     ADDR(        0x2d6040,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_TO_UNIT_LINK                ADDR(        0x2d66c2,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_TO_UNIT_FAIL_24             ADDR(        0x2dca10,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_AT_SQUARE                   ADDR(        0x2d74a0,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMONED_UNIT_SETUP                ADDR(        0x2d3430,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_MONSTER_RANDOM                ADDR(        0x1da1b0,          MAC_NOT_LOOKED_UP)
#define RVA_MONSTER_UNIT_FIRST                 ADDR(        0x106b70,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_PROMOTE_EXISTING            ADDR(        0x2b26e0,          MAC_NOT_LOOKED_UP)
#define RVA_MAP_X_BOUND                        ADDR(        0x582c5c4,         0x1121121b0)
#define RVA_LAST_TERRAIN                       ADDR(        0x1b185c4,         0x1059bcc7c)
#define RVA_TERRAIN_NAME                       ADDR(        0x2beae0,          0x10055507c)
#define RVA_TERRAIN_NAME_TEXT                  ADDR(        0x5835f60,         0x10c81c118)
#define TERRAIN_NAME_TEXT_SIZE                 0x64
#define RVA_SQUARE_TERRAIN                     ADDR(        0x3355a0,          MAC_INLINED)
#define RVA_TERRAIN_MATCH                      ADDR(        0x2c4390,          0x100555130)
#define RVA_TERRAIN_NEAR_COUNT                 ADDR(        0x199ee0,          0x1004a9354)
#define RITUAL_FX_NONEARBY1REQ                 1003
#define RVA_RITUAL_STRING_DECODE               ADDR(        0x2b3f90,          0x10055b1cc)
#define RVA_COUNT_MONSTERS_AT                  ADDR(        0x11f430,          0x10046d8b4)
#define RVA_COUNT_MONSTERS_HOME_AT             ADDR(        0x106900,          MAC_INLINED)
#define RVA_COUNT_MONSTERS_ON_PLANE            ADDR(        0x106a00,          0x10047efdc)
#define RVA_COUNT_PLAYER_MONSTERS              ADDR(        0x129360,          0x10047f15c)
#define RVA_ENCHANT_COUNT                      ADDR(        0x2cf7c0,          0x1005564c4)
#define RVA_ENCHANT_COUNT_KINDS                ADDR(        0x2cd080,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_COUNT_IN_BOX               ADDR(        0x335440,          MAC_NOT_LOOKED_UP)
#define RVA_ENCHANT_FIND                       ADDR(        0x2c1a80,          0x10055b470)
#define RVA_SQUARE_FORT_PARTS                  ADDR(        0x12b380,          0x1004753f0)
#define RVA_FIND_UNIT_WITH_ABILITY_AT          ADDR(        0x116ed0,          0x1004810cc)
#define RVA_UNIT_RESISTANCES                   ADDR(        0x32a7a0,          0x10056768c)
#define RVA_UNIT_BENEFITS                      ADDR(        0x320f60,          0x1005679f8)
#define RVA_COMMANDER_ABILITY_TOTAL            ADDR(        0x1294c0,          0x100468900)
#define RVA_COUNT_PLAYER_PORTALS               ADDR(        0x2516c0,          0x100571304)
#define RVA_SQUARE_PORTAL_AT                   ADDR(        0x241350,          MAC_INLINED)
#define SQUARE_PORTAL                          0x3e
#define RVA_AI_REVEAL_PORTAL_EXITS             ADDR(        0x251810,          MAC_NOT_LOOKED_UP)
#define RVA_REVEAL_PORTAL_EXITS                ADDR(        0x242330,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_LISTED                      ADDR(        0x2b4640,          MAC_INLINED)
#define RVA_GET_UNIT_HP                        ADDR(        0x329290,          MAC_INLINED)
#define RVA_UNIT_HOME_X                        ADDR(        0xfb8e87a,         0x11004996a)
#define RITUAL_FX_LEVELREQ                     905
#define RITUAL_FX_RESRESTRICT                  912
#define RITUAL_FX_RESREQ                       913
#define RITUAL_FX_BENRESTRICT                  924
#define RITUAL_FX_NOMONREQ                     946
#define RITUAL_FX_NOMONHOMEREQ                 947
#define RITUAL_FX_FXREQ                        962
#define RITUAL_FX_NOFXHEREREQ                  973
#define RITUAL_FX_ENCHHEREREQ                  980
#define RITUAL_FX_NOFORTREQ                    986
#define RITUAL_FX_FORTREQ                      987
#define RITUAL_FX_HASPORTALREQ                 993
#define RITUAL_FX_NOPORTALREQ                  998
#define RITUAL_FX_NOMONWORLDREQ                1015
#define RITUAL_FX_NOMONPLAYERREQ               1016
#define RITUAL_FX_NOMONMASTERYREQ              1017
#define RITUAL_FX_NOMONELYSIUMREQ              1018
#define RITUAL_FX_NOPLANEREQ                   1034
#define RITUAL_FX_HOMEREQ                      1060
#define RITUAL_FX_FEWMONREQ                    1061
#define RITUAL_FX_MONWORLDREQ                  1062
#define RITUAL_FX_MONPLAYERREQ                 1063
#define RITUAL_FX_MINMONREQ                    1071
#define RITUAL_FX_EVENTVARREQ                  1072
#define RITUAL_FX_NOEVENTVARREQ                1073
#define RITUAL_FX_HOMECITREQ                   1074
#define RITUAL_FX_SEASONREQ                    1085
#define RVA_FORMAT_AREA_SHAPE                  ADDR(        0x1cf810,          0x100520e90)

// ---- Input --------------------------------------------------------------------------------------
#define RVA_POLL_SDL_EVENTS                    ADDR(        0x04eaa0,          MAC_INLINED)
#define RVA_GET_MOUSE_X                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_GET_MOUSE_Y                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_MOUSE_Y_UNFLIPPED                  ADDR(        0x57efc14,         MAC_IN_CODE)
#define RVA_WINDOW_FULLSCREEN_MODE             ADDR(        0x593af78,         0x113997000)
#define RVA_MOUSE_Y_UNFLIPPED_OFF              RVA_WINDOW_FULLSCREEN_MODE
#define RVA_GET_FULLSCREEN_MODE                ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_FULLSCREEN_SETTING                 ADDR(        0x1a47cb4,         0x1008575e0)
#define RVA_GET_MOUSE_WHEEL                    ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_GET_MOUSE_BUTTONS                  ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_WINDOW_W                           ADDR(        0x12ab037c,        0x113997120)
#define RVA_WINDOW_H                           ADDR(        0x12ab0384,        0x113997020)
#define RVA_SDL_WINDOW_W                       ADDR(        0x5993798,         0x11399711c)
#define RVA_SDL_WINDOW_H                       ADDR(        0x5993784,         0x113997124)
#define RVA_WHEEL                              ADDR(        0x57f0614,         0x113997028)
#define RVA_KEY_STATE                          ADDR(        0x12ab01e0,        0x113997128)
#define RVA_KEY_CODES                          ADDR(        0x12ab0144,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_F1                        ADDR_DIFFERS(0x12ab014c,        0x1139970c0)
#define RVA_MOTION_X                           ADDR(        0x57efbdc,         0x11399701c)
#define RVA_MOTION_Y                           ADDR(        0x57f0670,         0x113997024)
#define RVA_PRESS_X                            ADDR(        0x439e7c,          0x105aa402c)
#define RVA_PRESS_Y                            ADDR(        0x43a2d4,          0x105aa4030)
#define RVA_BUTTONS_HELD                       ADDR(        0x57f064c,         0x113997030)
#define RVA_PRESSED                            ADDR(        0x57f0630,         0x11399702c)
#define RVA_WIN_W                              RVA_WINDOW_W
#define RVA_WIN_H                              RVA_WINDOW_H
#define RVA_SDL_SCALE_W                        RVA_SDL_WINDOW_W
#define RVA_SDL_SCALE_H                        RVA_SDL_WINDOW_H
#define RVA_PLAYER1_NAME                       RVA_SLOT_NAME
#define RVA_HOVER_SLOT                         ADDR(        0x42dd34,          0x100862694)
#define RVA_MOUSE_BUTTONS                      ADDR(        0x58356e8,         0x10c96df3c)
#define RVA_CLICK_LATCH                        ADDR_DIFFERS(0x57ef164,         0x10611be5c)
#define RVA_UI_MOUSE_X                         ADDR(        0x58356ec,         0x10c96df40)
#define RVA_UI_MOUSE_Y                         ADDR(        0x58356e4,         0x10c96df44)
#define RVA_ELEMENT_AT                         RVA_FIND_HOVERED
#define RVA_DRAW_CALL                          ADDR(        0x24055f,          0x10048aee0)
#define RVA_DRAW_PASS                          ADDR(        0x02cfc0,          0x100345ecc)
#define RVA_HIT_TEST_CALL                      ADDR(        0x23f520,          MAC_NO_CALL)
#define RVA_HIT_TEST                           ADDR(        0x02a890,          MAC_INLINED)
#define RVA_SET_SDL_WINDOW_SIZE                ADDR(        0x04fd10,          0x1005acbec)
#define RVA_INPUT_INIT                         ADDR(        0x04df90,          0x1005ad08c)
#define RVA_DRAG_OFFSET                        ADDR(        0x12ab2aa0,        0x10611be58)
#define RVA_RESIZE_WINDOW_SIZE                 ADDR(        0x050086,          0x1005acee0)
#define RVA_INPUT_INIT_KEY_CLEAR               ADDR(        0x04dfd3,          0x1005ad0d0)
#define RVA_ESCAPE_STORE                       ADDR_DIFFERS(0x04e5b2,          0x1005ad600)
#define RVA_BIND_HOTKEY_ESCAPE_READ            ADDR(        0x03ab46,          0x10033cdf4)
#define RVA_PRESS_SCALE                        ADDR(        0x04f249,          0x100485a9c)
#define RVA_PRESS_DIVISOR                      ADDR(        0x04f254,          0x100485ab0)
#define RVA_WHEEL_ADD                          ADDR_DIFFERS(0x04f1e9,          0x100485770)
#define RVA_WAIT_WHEEL_READ                    ADDR_DIFFERS(0x23f39c,          0x1004864e0)
#define RVA_WAIT_UI_MOUSE                      ADDR(        0x23f348,          0x100486498)
#define RVA_WAIT_FRAME_BUTTONS                 ADDR(        0x23f3e7,          0x10048650c)
#define RVA_WAIT_SQUARE_RESET                  ADDR(        0x23f3ed,          0x100486518)
#define RVA_ELEMENT_AT_CALL_1                  ADDR(        0x02a960,          0x1004866e0)
#define RVA_HOVER_STORE                        ADDR(        0x02a968,          0x1004866e8)
#define RVA_ELEMENT_AT_CALL_0                  ADDR(        0x02a992,          0x1004866fc)
#define RVA_WAIT_GAME_OVER_FX_1                ADDR(        0x23f75e,          0x100487e24)
#define RVA_WAIT_GAME_OVER_FX_2                ADDR(        0x23f781,          0x100487e48)
#define RVA_WAIT_BANNER_FADE_STEP              ADDR(        0x23fb60,          0x100488d64)
#define RVA_WAIT_LOGO_CALL                     ADDR(        0x23fbf1,          0x100488dcc)

#define RVA_TEXT_EVENT_FIRST_BYTE              ADDR(        0x04eb87,          0x100485980)
#define RVA_TEXT_ACCENT_STRINGS                ADDR(        0x439704,          MAC_IN_CODE)
#define RVA_TEXT_ACCENT_CODES                  ADDR(        0x12ab01a8,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_DOWN                           ADDR(        0x04f460,          MAC_INLINED)
#define RVA_KEY_SYM_TO_CODE                    ADDR(        0x04f8a0,          0x1005ac948)
#define RVA_KEY_BUFFER                         ADDR(        0x59937c0,         0x113997034)
#define RVA_KEY_BUFFER_LAST                    ADDR(        0x439dc4,          0x105aa4034)
#define RVA_KEY_SUPPRESS_CODE                  ADDR(        0x57f0648,         0x113997118)
#define RVA_POP_KEY                            ADDR(        0x0503b0,          MAC_INLINED)
#define RVA_HIT_TEST_POP_KEY_1                 ADDR_DIFFERS(0x02ac99,          0x1004869e8)
#define RVA_HIT_TEST_POP_KEY_2                 ADDR_DIFFERS(0x02b753,          0x100487600)
#define RVA_GUI_KEYPRESS                       ADDR(        0x431828,          0x100857630)
#define RVA_GUI_KEYPRESS_MORE                  ADDR(        0x432208,          0x100857634)
#define RVA_TEXT_INPUT_START                   ADDR(        0x050280,          MAC_INLINED)
#define RVA_TEXT_INPUT_STOP                    ADDR(        0x050210,          MAC_INLINED)
#define RVA_TEXT_FIELD                         ADDR(        0x038750,          0x10033e6d8)
#define RVA_NAME_BOX_TEXT_FIELD_CALL           ADDR(        0x041dda,          0x10034a5d4)
#define RVA_PLAYER_NAME_MAX_LEN                ADDR_DIFFERS(0x304e78,          0x10034a5c4)

// ---- Menus and keys -------------------------------------------------------------------------------
#define RVA_BANNER_FADE                        ADDR(        0x1ab8d9c,         0x101cce55c)
#define RVA_SHORTCUTS_TABLE                    ADDR(        0x3f9f10,          0x100837fd0)
#define RVA_HOTKEY_TABLE                       ADDR(        0x1319a6a0,        0x10611b994)
#define HOTKEY_SIZE                            8
#define RVA_HOTKEY_COUNT                       ADDR(        0x42dca4,          0x100857624)
#define RVA_HOTKEY_COUNT_RESET                 ADDR(        0x03af91,          0x10033c464)
#define RVA_HOTKEY_LOOKUP                      ADDR(        0x02b581,          0x100486b9c)
#define RVA_MAP_BIND_SHORTCUTS_LIST            ADDR(        0x096c65,          0x100380050)
#define RVA_MAP_BIND_RECRUIT                   ADDR(        0x09507f,          0x10037f7e0)
#define RVA_MAP_BIND_END_TURN                  ADDR(        0x0953f5,          0x10037f858)
#define RVA_MAP_BIND_MESSAGES                  ADDR(        0x095581,          0x10037f87c)
#define RVA_MAP_BIND_NEXT_COMMANDER            ADDR(        0x094ff0,          0x10037f7d4)
#define RVA_MAP_BIND_PREVIOUS_COMMANDER        ADDR(        0x09725f,          0x100380148)
#define RVA_MAP_BIND_COMMANDER_DONE            ADDR(        0x09578f,          0x10037f8ac)
#define RVA_MAP_BIND_TRANSFER                  ADDR(        0x095103,          0x10037f7ec)
#define RVA_MAP_BIND_SPECIAL_POWER             ADDR(        0x09518f,          0x10037f7f8)
#define RVA_MAP_BIND_UNIT_OVERVIEW             ADDR(        0x096d8b,          0x100380088)
#define RVA_MAP_BIND_PLAYER_OVERVIEW           ADDR(        0x096cef,          0x10038006c)
#define RVA_MAP_BIND_CLASS_DESCRIPTION         ADDR(        0x096f6a,          0x1003800dc)
#define RVA_MAP_BIND_MAP_DESCRIPTION           ADDR(        0x09700a,          0x1003800f8)
#define RVA_MAP_BIND_SCORE_GRAPHS              ADDR(        0x0970aa,          0x100380114)
#define RVA_MAP_BIND_ACTIVE_MODS               ADDR(        0x09714a,          0x100380130)
#define RVA_MAP_BIND_GAME_MENU_F10             ADDR(        0x09521a,          0x10037f814)
#define RVA_MAP_BIND_GAME_MENU_F11             ADDR(        0x0952ba,          0x10037f830)
#define RVA_MAP_BIND_GAME_MENU_F12             ADDR(        0x09535a,          0x10037f84c)
#define RVA_MAP_BIND_F9                        ADDR(        0x096e30,          0x1003800a4)
#define RVA_MAP_BIND_MENU_KEY                  ADDR(        0x096ecd,          0x1003800c0)
#define RVA_MAP_BIND_TERRAIN_INFO              ADDR(        0x0972df,          0x100380154)
#define RVA_MAP_BIND_COMMANDER_STATS           ADDR(        0x09547a,          0x10037f864)
#define RVA_MAP_BIND_TRADE                     ADDR(        0x095601,          0x10037f888)
#define RVA_MAP_BIND_SENTRY                    ADDR(        0x095686,          0x10037f894)
#define RVA_MAP_BIND_LOCATION_POWER            ADDR(        0x09570a,          0x10037f8a0)
#define RVA_MAP_BIND_BURN_FOREST_SCREEN        ADDR(        0x096ab1,          0x10037ff38)
#define RVA_MAP_BIND_BURN_FOREST_NOW           ADDR(        0x0971df,          0x10038013c)
#define RVA_MAP_BIND_HUNT_SLAVES               ADDR(        0x096b2f,          0x10037ff44)
#define RVA_MAP_BIND_PLANE_SHIFT               ADDR(        0x099e22,          0x10037e3c8)
#define RVA_MAP_BIND_CONTINUE                  ADDR(        0x0973ef,          0x10038016c)
#define RVA_MAP_BIND_CONTINUE_ALL              ADDR(        0x09736a,          0x100380160)
#define RVA_MAP_BIND_TEST_GAME                 ADDR(        0x095500,          0x10037f870)
#define RVA_MAP_BIND_HOME                      ADDR(        0x096bca,          0x100380044)
#define RVA_MAP_BIND_SQUARE_DETAILS            ADDR_DIFFERS(0x096b85,          0x100380028)
#define RVA_MAP_BIND_MAP_ICONS                 ADDR_DIFFERS(0x096b78,          0x10037ffa8)
#define RVA_MAP_BIND_ASTROLOGY                 ADDR_DIFFERS(0x09d87b,          0x10037aa9c)
#define RVA_MAP_BIND_REPEAT_ORDERS             ADDR_DIFFERS(0x097438,          0x1003801d0)
#define RVA_MAP_BIND_SCRY                      ADDR_DIFFERS(0x09580d,          0x10037f920)
#define RVA_MAP_BIND_MOVE_FIRST                ADDR(        0x09587f,          0x10037f948)
#define RVA_MAP_BIND_LOOK_FIRST                ADDR(        0x0965a0,          0x10037fae8)
#define RVA_ARMY_SCREEN                        ADDR(        0x11fdb0,          MAC_INLINED)
#define RVA_BATTLE_SCREEN                      ADDR(        0x0ffcb0,          0x1005826a4)
#define RVA_BATTLE_EDITOR                      ADDR(        0x084c60,          0x100518d14)
#define RVA_VIEW_ARMY_SCREEN                   ADDR(        0x122330,          0x10047dd70)
#define RVA_UNIT_LIST                          ADDR(        0x0f9f40,          MAC_INLINED)
#define RVA_SHEET_SCREEN                       ADDR(        0x03e480,          MAC_INLINED)
#define RVA_SHEET_ADD_SHORTCUT                 ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_SHEET_SHORTCUT_KEYS                ADDR(        0x12ab2a10,        0x10617f3b0)
#define RVA_SHEET_SHORTCUT_IDS                 ADDR(        0x12ab2a40,        0x10617f3e0)
#define RVA_MISC_MENU                          ADDR(        0x0a0ff0,          0x10037349c)
#define RVA_BIND_HOTKEY_NEGATIVE               ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_MAP_VIEW_UPDATE                    ADDR(        0x23cd60,          MAC_INLINED)
#define RVA_BATTLE_VIEW_UPDATE                 ADDR(        0x23c680,          MAC_INLINED)
#define RVA_WAIT_TAB_READ                      ADDR(        0x24025d,          MAC_NOT_FOUND)
#define RVA_SDL_POLL_EVENT                     ADDR_DIFFERS(WIN_INLINED,       0x10000fc9c)
#define RVA_SDL_POLL_EVENT_SLOT                ADDR_DIFFERS(0x33a580,          0x100852020)
#define RVA_SDL_PUSH_EVENT_SLOT                ADDR(        WIN_NO_EQUIVALENT, 0x100852038)
#define RVA_SWAP_WINDOW                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_WAIT_SWAP_CALL                     ADDR_DIFFERS(0x240757,          0x10048c518)
#define RVA_SDL_GL_SWAP_WINDOW                 ADDR_DIFFERS(WIN_INLINED,       0x100011178)
#define RVA_SDL_GL_SWAP_WINDOW_SLOT            ADDR_DIFFERS(0x33a658,          0x100852de8)
#define RVA_SDL_WINDOW                         ADDR(        0x57f0618,         0x113996ff8)
#define RVA_FRAME_COUNT                        ADDR(        0x58356b8,         0x106cee4f8)
#define RVA_KEY_CODE_BACKSPACE                 ADDR(        0x12ab01a0,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_TAB                       ADDR(        0x12ab01a4,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_ENTER                     ADDR(        0x12ab014e,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_DELETE                    ADDR(        0x12ab0376,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_PAGE_DOWN                 ADDR(        0x12ab018e,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_PAGE_UP                   ADDR(        0x12ab0388,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_END                       ADDR(        0x12ab01aa,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_HOME                      ADDR(        0x12ab0378,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_UP                        ADDR(        0x12ab0374,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_DOWN                      ADDR(        0x12ab0380,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT                      ADDR(        0x12ab0370,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_RIGHT                     ADDR(        0x12ab0382,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_SHIFT                ADDR(        0x12ab01b8,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_CTRL                 ADDR(        0x12ab01b6,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_RIGHT_CTRL                ADDR(        0x12ab017c,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_ALT                  ADDR(        0x12ab0198,        MAC_STORED_DIFFERENTLY)

// ---- Map editor and the map view ----------------------------------------------------------------
#define RVA_MAP_EDITOR_ACTIVE                  ADDR_DIFFERS(0x582c680,         0x108810eb0)
#define RVA_MAP_EDITOR_MENU_SKIP               ADDR(        0x1b75d5,          0x100514abc)
#define RVA_MAP_EDITOR_TITLE_CALL              ADDR(        0x1b7689,          0x100514b28)
#define RVA_MAP_EDITOR_REMOVE_FANCY_CALL       ADDR(        0x1b76fc,          0x100514ba0)
#define RVA_MAP_EDITOR_CREATE_FANCY_CALL       ADDR(        0x1b7719,          0x100514bbc)
#define RVA_MAP_EDITOR_SETTINGS_CALL           ADDR(        0x1b772a,          0x100514bcc)
#define RVA_MAP_EDITOR_COORDS_CALL             ADDR(        0x1b7865,          0x100514dac)
#define RVA_MAP_EDITOR_OVERLAY_DRAW_CALL       ADDR(        0x1b8c83,          0x10051576c)
#define RVA_CREATE_FANCY_TERRAIN               ADDR(        0x1b4ab0,          MAC_INLINED)
#define RVA_SAVE_MAP                           ADDR(        0x1b5210,          MAC_INLINED)
#define MAP_EDITOR_FREE_ID_FIRST               0x2c
#define RVA_MAP_ZOOM                           ADDR(        0x1ab9e1c,         0x10087350c)
#define RVA_MAP_CAMERA_X                       ADDR(        0x58356e0,         0x10b848af8)
#define RVA_MAP_CAMERA_Y                       ADDR(        0x58356bc,         0x10b848afc)
#define RVA_MAP_ZOOM_SPEED                     ADDR(        0x583572c,         0x106cee55c)
#define RVA_MAP_VIEW_PLANE                     ADDR(        0x58356f0,         0x10c921cfc)
#define RVA_MAP_VIEW_DRAW_CALL                 ADDR(        0x23ff5e,          MAC_NO_CALL)
#define RVA_DRAW_LINE_UI                       ADDR(        0x030380,          MAC_INLINED)
#define RVA_FILL_RECT_UI                       ADDR(        0x030280,          MAC_NOT_LOOKED_UP)
#define GUI_TYPE_LINE                          0x1d
#define RVA_BATTLE_GRID_ON                     ADDR(        0x43e3dc,          0x1048937d8)
#define RVA_BATTLE_FIELD_DRAW                  ADDR(        0x0fc4d0,          0x10057f7e0)
#define RVA_BATTLE_GRID_SPRITE_CALL            ADDR(        0x0fc79d,          0x10057fa1c)
#define BATTLE_GRID_SPRITE                     0x17
#define MAP_EDITOR_FREE_ID_LAST                999
#define RVA_GL_STATE                           ADDR(        0x029020,          0x100350e94)
#define GL_STATE_LINES                         0x16
#define RVA_TEXT_RENDER_FLAGS                  ADDR(        0x57c58d4,         0x105b68184)
#define DRAW_TEXT_SIZE_SCALE                   0.75f
#define RVA_GL_COLOR4F                         ADDR_DIFFERS(0x33a460,          0x1005ae740)
#define RVA_GL_LINE_WIDTH                      ADDR_DIFFERS(0x33a3f0,          0x1005ae848)
#define RVA_GL_ENABLE_CLIENT_STATE             ADDR_DIFFERS(0x33a4b8,          0x1005ae7a0)
#define RVA_GL_DISABLE_CLIENT_STATE            ADDR_DIFFERS(0x33a3f8,          0x1005ae77c)
#define RVA_GL_VERTEX_POINTER                  ADDR_DIFFERS(0x33a428,          0x1005ae92c)
#define RVA_GL_DRAW_ARRAYS                     ADDR_DIFFERS(0x33a3d0,          0x1005ae788)

// ---- Utilities ----------------------------------------------------------------------------------
#define RVA_TRACE                              ADDR(        0x028e80,          0x10033ad60)
#define RVA_SNPRINTF                           ADDR_DIFFERS(0x001430,          0x1005aef14)
#define RVA_VSPRINTF                           ADDR_DIFFERS(0x005724,          0x1005af0e8)
#define RVA_PRINTF_OPTIONS                     ADDR(        0x001300,          MAC_NOT_FOUND)
#define RVA_SYSTEM                             ADDR(        0x0062b0,          0x1005af094)
#define RVA_GETTIMEOFDAY                       ADDR(        WIN_NO_EQUIVALENT, 0x1005ae6e0)
#define RVA_TRACE_B                            ADDR(        0x0288f0,          0x10033af88)
#define RVA_TRACE_C                            ADDR(        0x028ce0,          0x10033ae74)
#define RVA_DEBUG_LEVEL                        ADDR(        0x42d01c,          0x1059bcc78)
#define RVA_TRACE_B_CHECK                      ADDR(        0x028924,          0x10033afc8)
#define RVA_TRACE_C_CHECK                      ADDR(        0x028d14,          0x10033aeb4)
#define RVA_TRACE_ERROR                        ADDR(        0x028a90,          0x10033abf4)
#define RVA_TRACE_ERROR_WRITE                  ADDR(        0x028b50,          MAC_NOT_LOOKED_UP)
#define TRACE_PROLOGUE_SIZE                    0x44
#define TRACE_ERROR_PROLOGUE_SIZE              0x40
#define RVA_TRACE_TIMER                        ADDR(        0x028700,          MAC_NOT_LOOKED_UP)
#define RVA_TRACE_TIMER_MARK                   ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_TRACE_TO_LOG_FILE                  ADDR(        0x57c589c,         MAC_NOT_FOUND)
#define RVA_TRACE_LOG_START                    ADDR(        0x028cb0,          MAC_NOT_FOUND)
#define RVA_TYPED_WORD_DEBUG_LEVEL             ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_TRACE_TIMESTAMPS                   ADDR_DIFFERS(0x57c58a4,         0x105b68154)
#define RVA_SDL_LOADOBJECT_SLOT                ADDR(        WIN_NO_EQUIVALENT, 0x100852390)
#define RVA_SDL_LOADFUNCTION_SLOT              ADDR(        WIN_NO_EQUIVALENT, 0x100852398)
#define RVA_SDL_LOADOBJECT_INIT                ADDR(        WIN_NO_EQUIVALENT, 0x100016ed4)
#define RVA_SDL_LOADFUNCTION_INIT              ADDR(        WIN_NO_EQUIVALENT, 0x100016f00)
#define RVA_FAIL_IF                            ADDR(        0x05b450,          0x100334808)
#define RVA_ALLOC                              ADDR(        0x05e300,          0x100334488)
#define RVA_TICKS_MS                           ADDR_DIFFERS(0x05e210,          0x1003348d4)
#define RVA_FILE_READ_INT                      ADDR(        0x0588b0,          0x100336afc)
#define RVA_FILE_READ_SHORT                    ADDR(        0x0586c0,          MAC_INLINED)
#define RVA_FILE_READ_ARRAY                    ADDR(        0x057e00,          MAC_INLINED)
#define RVA_FILE_WRITE_INT                     ADDR(        0x058380,          MAC_INLINED)
#define RVA_FILE_WRITE_ARRAY                   ADDR(        0x057ea0,          MAC_INLINED)
#define RVA_FATAL_FORMAT                       ADDR(        0x05e670,          0x1003346d0)
#define RVA_RANDOM                             ADDR(        0x05b600,          0x100335cf0)
#define RVA_RANDOM_OPEN_DIE                    ADDR(        0x058b10,          MAC_NOT_LOOKED_UP)
#define RVA_INT_SQRT                           ADDR(        0x056d90,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_INDEX                       ADDR(        0x57f1a34,         0x105aa91e0)
#define RVA_RANDOM_SHIFT                       ADDR(        0x57f1a58,         0x105aa9248)
#define RVA_RANDOM_XOR                         ADDR(        0x57f1a30,         0x105aa924c)
#define RVA_RANDOM_SEED_FROM_CLOCK             ADDR(        0x05ac00,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_SET_SEED                    ADDR(        0x05abb0,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_RAND_BELOW                     ADDR(        0x05e5f0,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_SEED_FROM_CLOCK                ADDR(        0x05aa60,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_RAND                           ADDR(        0x00eff0,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_SRAND                          ADDR(        0x00f01c,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SEED_DRAW_CALL              ADDR(        0x0f56ce,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_TABLE                       ADDR(        0x343790,          0x10075bc34)
#define RVA_MEMSET                             ADDR(        0x002ef0,          0x1005aea88)
#define RVA_MEMMOVE                            ADDR(        0x002630,          0x1005aea7c)
#define RVA_MEMCPY                             ADDR_DIFFERS(0x002630,          0x1005aea70)
#define RVA_BZERO                              ADDR(        WIN_NO_EQUIVALENT, 0x1005ae464)
#define RVA_FREAD                              ADDR(        0x00e6a0,          0x1005ae638)
#define RVA_FWRITE                             ADDR(        0x005fe0,          0x1005ae6b0)
#define RVA_FCLOSE                             ADDR(        0x005ab8,          0x1005ae59c)
#define RVA_FOPEN                              ADDR(        0x005a28,          0x1005ae608)
#define RVA_STRCMP_STUB                        ADDR(        WIN_NO_EQUIVALENT, 0x1005aef80)
#define RVA_CHKSTK                             ADDR(        0x023ef0,          MAC_STORED_DIFFERENTLY)
#define RVA_CHKSTK_DARWIN_GOT                  ADDR(        WIN_NO_EQUIVALENT, 0x100774210)
#define RVA_FILE_BYTES_WRITTEN                 ADDR(        0x57f1a40,         0x105aa9290)

// ---- The CRT's stack-cookie check and exception handlers ----------------------------------------
#define RVA_SECURITY_COOKIE                    ADDR(        0x42c000,          MAC_NOT_FOUND)
#define RVA_SECURITY_CHECK_COOKIE              ADDR(        0x001880,          MAC_NOT_FOUND)
#define RVA_CHECK_COOKIE_FAIL_JMP              ADDR(        0x00189c,          MAC_NOT_FOUND)
#define RVA_REPORT_GSFAILURE                   ADDR(        0x0018d8,          MAC_NOT_FOUND)
#define RVA_GS_HANDLER_CHECK                   ADDR(        0x023e60,          MAC_NOT_FOUND)
#define RVA_C_SPECIFIC_HANDLER                 ADDR(        0x002c74,          MAC_NOT_FOUND)
#define RVA_SCRT_SET_UNHANDLED_FILTER          ADDR(        0x002360,          MAC_NOT_FOUND)
#define RVA_STACK_CHK_FAIL_STUB                ADDR(        WIN_NO_EQUIVALENT, 0x1005ae35c)
#define RVA_SDL_QUIT_SIGNAL_HANDLER            ADDR(        WIN_NO_EQUIVALENT, 0x100025c78)
#define RVA_SDL_INSTALL_QUIT_SIGNAL            ADDR(        WIN_NO_EQUIVALENT, 0x100025c24)
#define RVA_SDL_REMOVE_QUIT_SIGNAL             ADDR(        WIN_NO_EQUIVALENT, 0x100025cbc)
#define RVA_SDL_QUIT_SIGNALLED                 ADDR(        WIN_NO_EQUIVALENT, 0x105aa586c)

// ---- Strings ------------------------------------------------------------------------------------
#define RVA_STR_MAINMENU                       ADDR(        0x1c5bfa0,         0x100606044)
#define RVA_STR_START_NEW_GAME                 ADDR(        0x1c5bfe8,         0x10060607b)
#define RVA_STR_LOAD_GAME                      ADDR(        0x1c5bff8,         0x1006fe7b8)
#define RVA_STR_NETWORK                        ADDR(        0x1c5c018,         0x1006ff562)
#define RVA_STR_PREFERENCES                    ADDR(        0x1c5c020,         0x100631623)
#define RVA_STR_MODS                           ADDR(        0x1c5c02c,         0x10070700c)
#define RVA_STR_CREDITS                        ADDR(        0x1c5c038,         0x10060608a)
#define RVA_STR_READ_TIP                       ADDR(        0x1c5c040,         0x100606092)
#define RVA_STR_QUIT                           ADDR(        0x1c5c04c,         0x1007102b2)
#define RVA_STR_PERCENT_S                      ADDR(        0x1a3ba1c,         0x1005b6ffd)
#define RVA_STR_OPEN_MODS_DIR                  ADDR(        0x1c616a8,         0x10070708a)
#define RVA_STR_GUIFONT                        ADDR(        0x432230,          0x100857674)
#define RVA_STR_GUIFONT_FANCY                  ADDR(        0x435350,          0x100861c7c)
#define RVA_STR_GUIFONT_TEXTY                  ADDR(        0x431840,          0x10085803c)
#define RVA_FONT_FILE_TABLE                    ADDR(        WIN_NOT_FOUND,     0x10077e6b0)
#define RVA_STR_LOAD_GAME_DISABLED             ADDR(        0x1c5c008,         0x1006fe7b8)
#define RVA_STR_GAME_MENU                      ADDR(        0x1b292b0,         0x100631610)
#define RVA_STR_TIEVT                          ADDR(        0x43b368,          0x10071242a)
#define RVA_STR_TTF_EXTENSION                  ADDR(        0x1c5db78,         0x100707274)
#define RVA_STR_NO_TTF_FILE                    ADDR(        0x1c5db80,         0x100707279)
#define RVA_STR_FONT_FILE_MISSING              ADDR(        0x1c5db58,         0x100707256)
#define RVA_STR_MOD_DIFFERS                    ADDR(        0x1c58cd0,         0x1006fd284)

#endif   // GAME_ADDRESSES_H

// ---- Call sites -----------------------------------------------------------------------------------
#if defined(GAME_ADDRESSES_SITE_TABLE) && !defined(GAME_ADDRESSES_SITE_TABLE_DONE)
#define GAME_ADDRESSES_SITE_TABLE_DONE

#define MAX_SITE_CALLS                         24

typedef struct {
    unsigned int rvas[MAX_SITE_CALLS], targetRva;
    const char *name;
    int captureMenuFade;
} SiteInfo;

#define SITE_CALLS(...) __VA_ARGS__
#ifdef __APPLE__
#define M(va) MAC(va)
#define SITE(name, fade, winCalls, winTarget, macCalls, macTarget) {{SITE_CALLS macCalls}, macTarget, name, fade}
#else
#define SITE(name, fade, winCalls, winTarget, macCalls, macTarget) {{SITE_CALLS winCalls}, winTarget, name, fade}
#endif
static const SiteInfo SITES[SITE_COUNT] = {
    [SITE_MAIN_MENU_WAIT] = SITE("main_menu_wait", 1,
        /* windows */ (0x192592),       0x23ef50,
        /* mac     */ (M(0x10032e5a0)), M(0x100485230)),
    [SITE_MAIN_MENU_PLAY_GAME] = SITE("main_menu_play_game", 0,
        /* windows */ (0x1925fb),       0x091ad0,
        /* mac     */ (M(0x10032e600)), M(0x100389bfc)),
    [SITE_PLAY_GAME_NEW_WORLD] = SITE("play_game_new_world", 0,
        /* windows */ (0x091bf0),       0x303070,
        /* mac     */ (M(0x10038a020)), M(0x100587298)),
    [SITE_NEW_WORLD_MAP_SCREEN] = SITE("new_world_map_screen", 0,
        /* windows */ (0x30320a),       0x2aa2a0,
        /* mac     */ (M(0x100587488)), M(0x1004b2790)),
    // @platform-split new_world_init_participants: no Mac call
    [SITE_NEW_WORLD_INIT_PARTICIPANTS] = SITE("new_world_init_participants", 0,
        /* windows */ (0x30354d), 0x2ea0b0,
        /* mac     */ (0),        0),
    [SITE_NEW_WORLD_CREATING_WORLD] = SITE("new_world_creating_world", 0,
        /* windows */ (0x304d6f),       0x0897d0,
        /* mac     */ (M(0x10058945c)), M(0x1004903f8)),
    [SITE_MAP_SETTINGS_WAIT] = SITE("map_settings_wait", 0,
        /* windows */ (0x2aaee7),       0x23ef50,
        /* mac     */ (M(0x1004b2cc0)), M(0x100485230)),
    [SITE_PARTICIPANTS_WAIT] = SITE("participants_wait", 0,
        /* windows */ (0x3045f8),       0x23ef50,
        /* mac     */ (M(0x1005887c0)), M(0x100485230)),
    [SITE_PARTICIPANTS_DIFFICULTY_WAIT] = SITE("participants_difficulty_wait", 0,
        /* windows */ (0x302d55),       0x23ef50,
        /* mac     */ (M(0x100586aa8)), M(0x100485230)),
    // @platform-split participants_button_row: Mac's chain ends in CoE5SE's copy of button_row
    [SITE_PARTICIPANTS_BUTTON_ROW] = SITE("participants_button_row", 0,
        /* windows */ (0x303fb4),                       0x03a3f0,
        /* mac     */ (M(0x100588a48), M(0x1005884ec)), M(0x10033cf60)),
    [SITE_GAME_MENU_SAVING_SECTION_END] = SITE("game_menu_saving_section_end", 0,
        /* windows */ (0x12484d),       0x035400,
        /* mac     */ (M(0x100380b10)), M(0x100340e70)),
    [SITE_GAME_MENU_WAIT] = SITE("game_menu_wait", 0,
        /* windows */ (0x124c9c),       0x23ef50,
        /* mac     */ (M(0x100380d80)), M(0x100485230)),
    [SITE_RITUAL_SCREEN_ROW] = SITE("ritual_screen_row", 0,
        /* windows */ (0x2afe66),       0x2d6920,
        /* mac     */ (M(0x10036089c)), M(0x100352118)),
    [SITE_RITUAL_SCREEN_LOCATION_CHECK] = SITE("ritual_screen_location_check", 0,
        /* windows */ (0x2afdc4),       0x2cd170,
        /* mac     */ (M(0x1003606c4)), M(0x1005598d4)),
    [SITE_RITUAL_SCREEN_CAN_CAST] = SITE("ritual_screen_can_cast", 0,
        /* windows */ (0x2afdd1),       0x2c9c40,
        /* mac     */ (M(0x1003606d4)), M(0x10055bf14)),
    [SITE_MAP_SCREEN_WAIT] = SITE("map_screen_wait", 0,
        /* windows */ (0x097447),       0x23ef50,
        /* mac     */ (M(0x100380490)), M(0x100485230)),
    [SITE_MAP_MENU_BUTTON_TEXT] = SITE("map_menu_button_text", 0,
        /* windows */ (0x09da11),       0x038f60,
        /* mac     */ (M(0x10037b3bc)), M(0x10033ddcc)),
    [SITE_GAME_START] = SITE("game_start", 0,
        /* windows */ (0x091e8f),       0x1b0ca0,
        /* mac     */ (M(0x10038a648)), M(0x100491f78)),
    [SITE_MONTH_START] = SITE("month_start", 0,
        /* windows */ (0x09206c),       0x0897d0,
        /* mac     */ (M(0x10038a978)), M(0x1004903f8)),
    [SITE_PLAYER_TURN_START] = SITE("player_turn_start", 0,
        /* windows */ (0x09221f, 0x0924b6, 0x0925ce),                   0x167a40,
        /* mac     */ (M(0x10038abc0), M(0x10038ad48), M(0x10038af74)), M(0x100392c00)),
    [SITE_HUMAN_TURN] = SITE("human_turn", 0,
        /* windows */ (0x0927b6),       0x0947b0,
        /* mac     */ (M(0x10038b3fc)), M(0x10037b5bc)),
    [SITE_AI_TURN] = SITE("ai_turn", 0,
        /* windows */ (0x092251),       0x06f7c0,
        /* mac     */ (M(0x10038afb0)), M(0x1004e090c)),
    [SITE_PLAYER_TURN_END] = SITE("player_turn_end", 0,
        /* windows */ (0x0922d2),       0x1e1b40,
        /* mac     */ (M(0x10038c358)), M(0x1004c5704)),
    // @platform-split game_over: Mac's call is the Game Over screen's, not game_over's
    [SITE_GAME_OVER] = SITE("game_over", 0,
        /* windows */ (0x093e1c),       0x10d110,
        /* mac     */ (M(0x100390454)), M(0x100473ea0)),
    // @platform-split game_save: Windows' calls pass (name, FILE *); CoE5SE's outermost wrapper passes (path, name) on
    [SITE_GAME_SAVE] = SITE("game_save", 0,
        /* windows */ (0x091f2e, 0x09278c, 0x09425b, 0x18b5fc),       0x18e850,
        /* mac     */ (M(0x10038a6f8), M(0x10038b3c4), M(0x1004f4d1c)), M(0x1004f20fc)),
    [SITE_GAME_LOAD] = SITE("game_load", 0,
        /* windows */ (0x18b066, 0x191165, 0x191371),                   0x185c10,
        /* mac     */ (M(0x10032e6a4), M(0x1003338f4), M(0x100333dc4)), M(0x1004eb3d4)),
    [SITE_MODS_RUN] = SITE("mods_run", 0,
        /* windows */ (0x091d8c, 0x093e50, 0x094171, 0x167ad2, 0x185e40, 0x1910a9, 0x1ca76f),                           0x1cb6f0,
        /* mac     */ (M(0x10038a140), M(0x1003919f0), M(0x100392cc4), M(0x1004eb98c), M(0x100332fbc), M(0x10032f464)), M(0x10053a2c0)),
    [SITE_MOD_COMMAND] = SITE("mod_command", 0,
        /* windows */ (0x1bb640),                       0x1bb930,
        /* mac     */ (M(0x10052a2a8), M(0x10052a5f4)), M(0x10052ef18)),
    [SITE_SQUARE_INFO_SCREEN] = SITE("square_info_screen", 0,
        /* windows */ (0x097a03),       0x331950,
        /* mac     */ (M(0x1003846c4)), M(0x1004c9e64)),
    // @platform-split square_info_button_row: Mac's chain ends in CoE5SE's copy of button_row
    [SITE_SQUARE_INFO_BUTTON_ROW] = SITE("square_info_button_row", 0,
        /* windows */ (0x33478c),       0x03a3f0,
        /* mac     */ (M(0x1004cd560)), M(0x10033cf60)),
    [SITE_SQUARE_INFO_WAIT] = SITE("square_info_wait", 0,
        /* windows */ (0x3349f5),       0x23ef50,
        /* mac     */ (M(0x1004cd644)), M(0x100485230)),
    [SITE_TEXT_FORMAT] = SITE("text_format", 0,
        /* windows */ (0x0336a9, 0x039b4d, 0x039fbd, 0x039dfe, 0x039d1d, 0x03982d, 0x039a80, 0x038dc6, 0x038a15,
                       0x038bd1, 0x039216, 0x0361b4, 0x036473, 0x038ff6, 0x042fbe, 0x042bc0), 0x005724,
        /* mac     */ (0),                                                                    0),
    [SITE_TEXT_BUILD] = SITE("text_build", 0,
        /* windows */ (0x0f8b83, 0x0f8bfb, 0x0f8f51, 0x0f907c, 0x0f909b, 0x118e9c, 0x2afbd2, 0x2afc36, 0x2afcc4,
                       0x2afd19, 0x2afda3, 0x2afdef, 0x2affe7, 0x2b0055, 0x2b0063, 0x2c2487, 0x2c24d6, 0x2c2548,
                       0x2c25b8, 0x2c2628, 0x2c269a, 0x2c270c), 0x001430,
        /* mac     */ (0),                                      0),
    [SITE_LOAD_GAME_BROWSER] = SITE("load_game_browser", 0,
        /* windows */ (0x18afd4, 0x18b032), 0x03f430,
        /* mac     */ (M(0x10032e680)),     M(0x10034a780)),
    [SITE_FILE_BROWSER_ROW_DATE] = SITE("file_browser_row_date", 0,
        /* windows */ (0x040cde),       0x039ad0,
        /* mac     */ (M(0x10034b5c4)), M(0x10033d690)),
    [SITE_FILE_BROWSER_WAIT] = SITE("file_browser_wait", 0,
        /* windows */ (0x04118a),       0x23ef50,
        /* mac     */ (M(0x10034baec)), M(0x100485230)),
    [SITE_FILE_BROWSER_HOVERED] = SITE("file_browser_hovered", 0,
        /* windows */ (0), 0,
        /* mac     */ (0), 0),
    [SITE_MAIN_MENU_MODS_SCREEN] = SITE("main_menu_mods_screen", 0,
        /* windows */ (0x1926eb),       0x1c9fb0,
        /* mac     */ (M(0x10032f468)), M(0x10032a848)),
    // @platform-split mod_banner_draw: Mac's call goes to a copy with another signature
    [SITE_MOD_BANNER_DRAW] = SITE("mod_banner_draw", 0,
        /* windows */ (0x23fb07),       0x02f9c0,
        /* mac     */ (M(0x100488d0c)), M(0x100345874)),
    [SITE_MOD_ERROR_DIALOG] = SITE("mod_error_dialog", 0,
        /* windows */ (0x1c9984),       0x041f50,
        /* mac     */ (M(0x100529d44)), M(0x1003499d4)),
    [SITE_GAME_OVER_WAIT] = SITE("game_over_wait", 0,
        /* windows */ (0x10d573),       0x23ef50,
        /* mac     */ (M(0x100474650)), M(0x100485230)),
    [SITE_EVENT_LINE] = SITE("event_line", 0,
        /* windows */ (0x1bb61c, 0x1cb5e6),                                             0x1c5650,
        /* mac     */ (M(0x10052a284), M(0x10052a44c), M(0x10052a5dc), M(0x10053a078)), M(0x10052a730)),
    [SITE_ARMY_WAIT] = SITE("army_wait", 0,
        /* windows */ (0x120a80),       0x23ef50,
        /* mac     */ (M(0x1003850f8)), M(0x100485230)),
    [SITE_BATTLE_WAIT] = SITE("battle_wait", 0,
        /* windows */ (0x100480),       0x23ef50,
        /* mac     */ (M(0x100582a48)), M(0x100485230)),
    [SITE_MAP_EDITOR_WAIT] = SITE("map_editor_wait", 0,
        /* windows */ (0x1b8c8f),       0x23ef50,
        /* mac     */ (M(0x100515778)), M(0x100485230)),
    [SITE_BATTLE_EDITOR_WAIT] = SITE("battle_editor_wait", 0,
        /* windows */ (0x085a88),       0x23ef50,
        /* mac     */ (M(0x1005198fc)), M(0x100485230)),
    [SITE_RECRUIT_WAIT] = SITE("recruit_wait", 0,
        /* windows */ (0x126e36),       0x23ef50,
        /* mac     */ (M(0x10047c7ec)), M(0x100485230)),
    [SITE_VIEW_ARMY_WAIT] = SITE("view_army_wait", 0,
        /* windows */ (0x122782),       0x23ef50,
        /* mac     */ (M(0x10047e188)), M(0x100485230)),
    [SITE_UNIT_LIST_WAIT] = SITE("unit_list_wait", 0,
        /* windows */ (0x03f0d6),       0x23ef50,
        /* mac     */ (M(0x100584530)), M(0x100485230)),
    [SITE_KEYBOARD_SHORTCUTS_WAIT] = SITE("keyboard_shortcuts_wait", 0,
        /* windows */ (0x12cef0),       0x23ef50,
        /* mac     */ (M(0x1005125ac)), M(0x100485230)),
    // @platform-split unit_list_sheet: no Mac call
    [SITE_UNIT_LIST_SHEET] = SITE("unit_list_sheet", 0,
        /* windows */ (0x0faabd), 0x03e480,
        /* mac     */ (0),        0),
    // @platform-split poll_event: Windows' calls go through the import slot
    [SITE_POLL_EVENT] = SITE("poll_event", 0,
        /* windows */ (0x04eafa, 0x04f295), 0x33a580,
        /* mac     */ (M(0x1004855e0)),     M(0x10000fc9c)),
    [SITE_KEY_TEXT_BAR] = SITE("key_text_bar", 0,
        /* windows */ (0x09c58c, 0x09d6a4, 0x09d708, 0x09d75c, 0x09d7b0, 0x09d820, 0x09d8c4, 0x09d918, 0x09d97d,
                       0x09d9ac, 0x099613, 0x0996c7, 0x099750, 0x0997da, 0x099e95, 0x09a495, 0x09a8f0, 0x09ab01),       0x038f60,
        /* mac     */ (M(0x100377be8), M(0x100379ef8), M(0x10037a240), M(0x10037a46c), M(0x10037a69c), M(0x10037a8e4),
                       M(0x10037abf4), M(0x10037ae20), M(0x10037b058), M(0x10037b170), M(0x10037da70), M(0x10037dba0),
                       M(0x10037dd1c), M(0x10037de18), M(0x10037e3d4), M(0x10037ec94), M(0x10037ef9c), M(0x10037f2a4)), M(0x10033ddcc)),
    [SITE_KEY_HELP_TEXT] = SITE("key_help_text", 0,
        /* windows */ (0x1b76ce, 0x1b76eb, 0x1b7708, 0x1b7747, 0x1b7797),                               0x039180,
        /* mac     */ (M(0x100514b74), M(0x100514b90), M(0x100514bac), M(0x100514be8), M(0x100514c34)), M(0x10033dc34)),
    [SITE_KEY_TEXT_BUTTON] = SITE("key_text_button", 0,
        /* windows */ (0x0a06fc),       0x039ad0,
        /* mac     */ (M(0x100376df0)), M(0x10033d690)),
    [SITE_RUN_ONE_MOD] = SITE("run_one_mod", 0,
        /* windows */ (0x1cb862, 0x1ca95a),                                                             0x1bb260,
        /* mac     */ (M(0x10053a630), M(0x10053a67c), M(0x10053a6c8), M(0x10053a714), M(0x10053a760), M(0x10053a7ac),
                       M(0x10053a7f8), M(0x10053a844), M(0x10053a890), M(0x1004eb8b0), M(0x1004f22d4)), M(0x100529e28)),
    [SITE_RUN_ONE_MOD_PATH] = SITE("run_one_mod_path", 0,
        /* windows */ (0x1bb372),       0x001430,
        /* mac     */ (M(0x100529f5c)), M(0x1005aef14)),
    [SITE_ELIMINATED_WARN] = SITE("eliminated_warn", 0,
        /* windows */ (0x111062),       0x042f40,
        /* mac     */ (M(0x100472ea0)), M(0x100348658)),
    [SITE_WARN_WAIT] = SITE("warn_wait", 0,
        /* windows */ (0x043166, 0x043186),             0x23ef50,
        /* mac     */ (M(0x10034896c), M(0x100348910)), M(0x100485230)),
    [SITE_GAME_MENU_SETTINGS_SECTION_END] = SITE("game_menu_settings_section_end", 0,
        /* windows */ (0x1245b8),       0x035400,
        /* mac     */ (M(0x1003809a0)), M(0x100340e70)),
    // @platform-split tooltip_line_draw: Mac's call is RVA_RENDER_TEXT, with another signature
    [SITE_TOOLTIP_LINE_DRAW] = SITE("tooltip_line_draw", 0,
        /* windows */ (0x02a24b),       0x02f380,
        /* mac     */ (M(0x10048be1c)), M(0x10034dc70)),
    [SITE_TURN_MESSAGES] = SITE("turn_messages", 0,
        /* windows */ (0x094ab7),       0x1e1f20,
        /* mac     */ (M(0x10037bc24)), M(0x1004c26cc)),
    [SITE_MESSAGES_WAIT] = SITE("messages_wait", 0,
        /* windows */ (0x1e26f9),       0x23ef50,
        /* mac     */ (M(0x1004c2f24)), M(0x100485230)),
    // @platform-split ai_explore: no Mac call
    [SITE_AI_EXPLORE] = SITE("ai_explore", 0,
        /* windows */ (0x070b7a), 0x074140,
        /* mac     */ (0),        0),
    // @platform-split ai_steps: no Mac call
    [SITE_AI_RALLY] = SITE("ai_rally", 0,
        /* windows */ (0x070a9a), 0x076520,
        /* mac     */ (0),        0),
    [SITE_AI_FRIENDLY_MOVE] = SITE("ai_friendly_move", 0,
        /* windows */ (0x070ae3), 0x076110,
        /* mac     */ (0),        0),
    [SITE_AI_CONQUER] = SITE("ai_conquer", 0,
        /* windows */ (0x070b2c), 0x0751f0,
        /* mac     */ (0),        0),
    [SITE_AI_FAR_EXPLORE] = SITE("ai_far_explore", 0,
        /* windows */ (0x070bc8), 0x073c30,
        /* mac     */ (0),        0),
    // @platform-split ai_ritual_candidate: no Mac call
    [SITE_AI_RITUAL_CANDIDATE] = SITE("ai_ritual_candidate", 0,
        /* windows */ (0x06b927), 0x2cbb10,
        /* mac     */ (0),        0),
    // @platform-split ai_recruit: no Mac call
    [SITE_AI_RECRUIT] = SITE("ai_recruit", 0,
        /* windows */ (0x06f91b), 0x0771e0,
        /* mac     */ (0),        0),
    // @platform-split ai_army: no Mac call
    [SITE_AI_GATHER_TROOPS] = SITE("ai_gather_troops", 0,
        /* windows */ (0x070cf3), 0x06ebc0,
        /* mac     */ (0),        0),
    [SITE_AI_PICK_UP_LONER] = SITE("ai_pick_up_loner", 0,
        /* windows */ (0x07c05d), 0x129830,
        /* mac     */ (0),        0),
    [SITE_AI_SPLIT_ARMY] = SITE("ai_split_army", 0,
        /* windows */ (0x07b34f), 0x07b4f0,
        /* mac     */ (0),        0),
    // @platform-split ai_army: no Mac call
    [SITE_AI_MOVE_COMSTR] = SITE("ai_move_comstr", 0,
        /* windows */ (0x070cfd), 0x129080,
        /* mac     */ (0),        0),
    // @platform-split ceremony_sites: no Mac call
    [SITE_CEREMONY_PERFORM] = SITE("ceremony_perform", 0,
        /* windows */ (0x2d53d9, 0x2d53c9, 0x09e8b9, 0x07b1dd, 0x07b1ff, 0x07b221, 0x07b240, 0x07b26b, 0x07b296,
                       0x07b2b2, 0x07b32f, 0x07b33f), 0x2e1fa0,
        /* mac     */ (0),                            0),
    [SITE_CEREMONY_COSTS] = SITE("ceremony_costs", 0,
        /* windows */ (0x2d3bb1, 0x07b1bb, 0x09e8a6), 0x2e3020,
        /* mac     */ (0),                            0),
    [SITE_SEAL_BREAK] = SITE("seal_break", 0,
        /* windows */ (0x2e2502, 0x184ea2), 0x1f2100,
        /* mac     */ (0),                  0),
    [SITE_SEAL_EVENT] = SITE("seal_event", 0,
        /* windows */ (0), 0,
        /* mac     */ (0), 0),
    [SITE_MAP_EDITOR] = SITE("map_editor", 0,
        /* windows */ (0x192686, 0x1ca704, 0x191336),                   0x1b72c0,
        /* mac     */ (M(0x10032e9d4), M(0x10032ed40), M(0x100333d58)), M(0x100514730)),
    [SITE_MAP_EDITOR_TOOLBOX_LINE] = SITE("map_editor_toolbox_line", 0,
        /* windows */ (0x1b7719),       0x039ad0,
        /* mac     */ (M(0x100514bbc)), M(0x10033d690)),
    [SITE_MAP_EDITOR_DRAW] = SITE("map_editor_draw", 0,
        /* windows */ (0x24055f),       0x02cfc0,
        /* mac     */ (M(0x10048aee0)), M(0x100345ecc)),
};
#undef M
#undef SITE
#endif   // GAME_ADDRESSES_SITE_TABLE

// ---- Code CoE5SE copied ---------------------------------------------------------------------------
#if defined(GAME_ADDRESSES_CODE_TABLE) && !defined(GAME_ADDRESSES_CODE_TABLE_DONE)
#define GAME_ADDRESSES_CODE_TABLE_DONE

typedef struct { unsigned int offset, targetRva; } CodeCall;

#define CODE_LIST(...) __VA_ARGS__
#ifdef __APPLE__
#define CODE_BYTES(win, mac) CODE_LIST mac
#else
#define CODE_BYTES(win, mac) CODE_LIST win
#endif
#define CODE_CALLS(win, mac) {CODE_BYTES(win, mac)}

static const char NAME_BOX_CODE[] = CODE_BYTES(
    /* windows */ ("4c 8b dc 53 55 56 57 41 54 41 55 41 56 41 57 48 81 ec 68 02 00 00 41 0f 29 73 a8 41 0f 29 7b 98"
                   "48 8b 05 ?? ?? ?? ?? 48 33 c4 48 89 84 24 30 02 00 00 48 8b b4 24 d8 02 00 00 4d 8d 7b 40 41 8b"
                   "d9 49 8b f8 44 8b f2 8b e9 e8 ?? ?? ?? ?? 4c 8b 8c 24 e0 02 00 00 48 8d 54 24 30 45 33 ed 4c 89"
                   "7c 24 28 41 b8 f4 01 00 00 4c 89 6c 24 20 48 8b 08 48 83 c9 01 e8 ?? ?? ?? ?? 41 bf 2a 00 00 00"
                   "c7 05 ?? ?? ?? ?? 01 00 00 00 41 3b df 44 0f 4c fb ff 15 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 2b c1 89"
                   "05 ?? ?? ?? ?? e8 ?? ?? ?? ?? 8b 9c 24 d0 02 00 00 4c 8d 25 ?? ?? ?? ?? f3 0f 10 3d ?? ?? ?? ??"
                   "f3 0f 10 35 ?? ?? ?? ?? e8 ?? ?? ?? ?? 66 0f 6e 0d ?? ?? ?? ?? f3 0f 10 05 ?? ?? ?? ?? 0f 5b c9"
                   "f3 0f 5d cf 0f 2f c1 f3 0f 11 0d ?? ?? ?? ?? f3 0f 11 0d ?? ?? ?? ?? 77 08 f3 0f 11 0d ?? ?? ??"
                   "?? 48 8b d6 48 8d 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 78 28 48 98 48 6b c8 70 b8"
                   "0a 00 00 00 42 c7 44 21 0c 00 00 a0 40 42 c7 44 21 10 00 00 80 3f 66 42 89 44 21 60 e8 ?? ?? ??"
                   "?? 44 89 2d ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 78 1b 48 98 48 6b c8 70 b8 08 00 00 00 66 42 89 44"
                   "21 60 46 89 6c 21 10 46 88 6c 21 6c e8 ?? ?? ?? ?? 85 c0 78 1a 48 98 48 6b c8 70 b8 67 00 00 00"
                   "42 c7 44 21 10 00 80 79 c4 66 42 89 44 21 60 45 8b c6 c7 44 24 20 1e 00 00 00 8b d5 33 c9 e8 ??"
                   "?? ?? ?? e8 ?? ?? ?? ?? 85 c0 78 1a 48 98 48 6b c8 70 b8 67 00 00 00 42 c7 44 21 10 00 c0 79 c4"
                   "66 42 89 44 21 60 e8 ?? ?? ?? ?? 85 c0 78 35 f3 0f 10 05 ?? ?? ?? ?? 48 98 48 6b c8 70 b8 0c 00"
                   "00 00 44 89 2d ?? ?? ?? ?? f3 42 0f 11 44 21 0c 46 89 6c 21 10 46 89 2c 21 66 42 89 44 21 60 46"
                   "88 6c 21 6c 41 b9 64 00 00 00 48 8d 44 24 30 48 89 44 24 28 41 8b d7 48 8d 05 ?? ?? ?? ?? 48 8b"
                   "cf 48 89 44 24 20 45 8d 41 9d e8 ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 78 1b 48 98 48 6b c8 70 b8 09"
                   "00 00 00 66 42 89 44 21 60 46 89 6c 21 10 46 88 6c 21 6c 48 63 15 ?? ?? ?? ?? c7 05 ?? ?? ?? ??"
                   "01 00 00 00 85 d2 78 5d 48 8b ca 7e 27 48 6b c1 70 4c 8d 05 ?? ?? ?? ?? 49 03 c0 0f 1f 44 00 00"
                   "66 83 38 64 7c 0e ff ca 48 ff c9 48 83 e8 70 48 85 c9 7f ec f3 0f 10 0d ?? ?? ?? ?? f3 0f 10 05"
                   "?? ?? ?? ?? 48 63 c2 48 6b c8 70 0f 2f c1 42 c6 44 21 6c 01 77 08 f3 0f 11 0d ?? ?? ?? ?? 44 89"
                   "2d ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 78 28 48 98 48 6b c8 70 b8 0a 00 00 00 42 c7 44 21 0c 00 00"
                   "40 40 42 c7 44 21 10 00 00 80 3f 66 42 89 44 21 60 e8 ?? ?? ?? ?? 48 8d 15 ?? ?? ?? ?? b9 01 00"
                   "00 00 e8 ?? ?? ?? ?? 85 db 74 11 48 8d 15 ?? ?? ?? ?? b9 02 00 00 00 e8 ?? ?? ?? ?? 0f 28 d6 0f"
                   "28 ce 83 c9 ff e8 ?? ?? ?? ?? 33 d2 8d 4a 01 e8 ?? ?? ?? ?? 83 f8 02 74 15 83 f8 01 0f 85 86 fd"
                   "ff ff e8 ?? ?? ?? ?? b8 01 00 00 00 eb 10 0f b6 05 ?? ?? ?? ?? 88 07 e8 ?? ?? ?? ?? 33 c0 48 8b"
                   "8c 24 30 02 00 00 48 33 cc e8 ?? ?? ?? ?? 0f 28 b4 24 50 02 00 00 0f 28 bc 24 40 02 00 00 48 81"
                   "c4 68 02 00 00 41 5f 41 5e 41 5d 41 5c 5f 5e 5d 5b c3"),
    /* mac     */ ("e9 23 b9 6d fc 6f 01 a9 fa 67 02 a9 f8 5f 03 a9 f6 57 04 a9 f4 4f 05 a9 fd 7b 06 a9 fd 83 01 91"
                   "ff c3 0c d1 e5 0f 00 f9 f3 03 02 aa e1 17 00 b9 ?? ?? ?? ?? 08 ?? ?? f9 08 01 40 f9 a8 03 19 f8"
                   "a8 43 00 91 e8 13 00 f9 ?? ?? ?? ?? 42 ?? ?? 91 e0 b3 00 91 a3 43 00 91 81 3e 80 52 ?? ?? ?? ??"
                   "e0 83 08 91 01 00 80 d2 ?? ?? ?? ?? 08 7d 80 52 e9 23 42 b9 ea 2b 82 b9 6b ba 89 52 4b 0c a2 72"
                   "4a 7d 0b 9b 4b fd 7f d3 4a fd 66 93 4a 01 0b 0b 28 29 08 1b ?? ?? ?? ?? 28 ?? ?? b9 ?? ?? ?? ??"
                   "88 ?? ?? 39 c8 03 00 37 ?? ?? ?? ?? 08 ?? ?? 39 1f 05 00 71 41 03 00 54 ?? ?? ?? ?? 00 ?? ?? 91"
                   "?? ?? ?? ?? c0 02 00 b4 ?? ?? ?? ?? 08 ?? ?? b9 1f 01 14 71 41 02 00 54 ?? ?? ?? ?? 08 ?? ?? b9"
                   "1f 81 0c 71 c1 01 00 54 ?? ?? ?? ?? 08 ?? ?? 91 00 05 41 ad e0 07 12 ad 00 f1 c3 3c e9 83 05 91"
                   "20 f1 8f 3c 01 01 40 ad e1 03 11 ad e0 83 08 91 ?? ?? ?? ?? 28 00 80 52 88 ?? ?? 39 1c 00 80 12"
                   "?? ?? ?? ?? ?? ?? ?? ?? d6 ?? ?? 91 ?? ?? ?? ?? 5a ?? ?? 91 1b 0e 80 52 ?? ?? ?? ?? 08 ?? ?? fd"
                   "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 35 00 80 52 1f 20 03 d5 09 ?? ?? fd ?? ?? ?? ?? ?? ?? ?? ??"
                   "1f ?? ?? 39 ?? ?? ?? ?? 1c ?? ?? b9 ?? ?? ?? ?? 1c ?? ?? b9 1f 20 03 d5 1c ?? ?? b9 ?? ?? ?? ??"
                   "1f ?? ?? b9 ?? ?? ?? ?? 1f ?? ?? b9 ?? ?? ?? ?? 00 ?? ?? bd 00 a4 20 0f 00 d8 61 5e 08 00 d8 d2"
                   "48 0a f8 f2 01 01 67 9e 00 28 61 1e 00 40 62 1e ?? ?? ?? ?? 00 ?? ?? bd ?? ?? ?? ?? 00 ?? ?? bd"
                   "00 d8 21 5e 08 79 a8 52 01 01 27 1e 00 58 21 1e ?? ?? ?? ?? 00 ?? ?? bd 1f 20 03 d5 00 ?? ?? bd"
                   "01 ?? ?? bd 20 20 20 1e 20 cc 20 1e 00 ?? ?? bd e8 0f 40 f9 e8 03 00 f9 e0 03 16 aa ?? ?? ?? ??"
                   "?? ?? ?? ?? 80 04 f8 37 08 68 bb 9b 08 c1 00 fc 49 01 80 52 09 c1 00 79 e9 ?? ?? b9 c9 03 f8 37"
                   "28 69 1b 9b 08 b1 41 39 08 01 00 34 00 ?? ?? bd 21 ?? ?? bd 00 20 21 1e 00 cc 21 1e 00 ?? ?? bd"
                   "00 e4 00 2f 02 00 00 14 20 ?? ?? bd 29 69 1b 9b 22 85 41 2d 21 c0 22 1e 42 c0 22 1e e9 e7 01 b2"
                   "49 33 93 f2 29 f7 e7 f2 23 01 67 9e 41 04 43 1f 21 40 62 1e 00 28 21 1e 20 ?? ?? bd c8 00 00 34"
                   "01 ?? ?? bd 20 20 20 1e 20 cc 20 1e 00 ?? ?? bd 3f ?? ?? b9 95 ?? ?? 39 ?? ?? ?? ?? c0 00 f8 37"
                   "08 68 bb 9b 09 01 80 52 09 c1 00 79 1f 11 00 b9 1f b1 01 39 ?? ?? ?? ?? e0 00 f8 37 08 68 bb 9b"
                   "09 00 90 52 29 8f b8 72 09 11 00 b9 e9 0c 80 52 09 c1 00 79 00 00 80 52 21 04 80 52 e2 17 40 b9"
                   "c3 03 80 52 ?? ?? ?? ?? ?? ?? ?? ?? e0 00 f8 37 08 68 bb 9b 09 00 98 52 29 8f b8 72 09 11 00 b9"
                   "e9 0c 80 52 09 c1 00 79 ?? ?? ?? ?? 60 01 f8 37 ?? ?? ?? ?? 00 ?? ?? bd 08 68 bb 9b 00 0d 00 bd"
                   "1f 11 00 b9 1f 01 00 b9 89 01 80 52 09 c1 00 79 1f b1 01 39 3f ?? ?? b9 e8 b3 00 91 e8 03 00 f9"
                   "e0 03 13 aa 21 03 80 52 22 00 80 52 83 0c 80 52 e4 03 16 aa ?? ?? ?? ?? ?? ?? ?? ?? c0 00 f8 37"
                   "08 68 bb 9b 29 01 80 52 09 c1 00 79 1f 11 00 b9 1f b1 01 39 9f ?? ?? 39 e8 ?? ?? b9 28 02 f8 37"
                   "08 01 00 34 09 69 bb 9b 29 c1 c0 79 3f 91 01 71 8b 00 00 54 08 05 00 71 6c ff ff 54 08 00 80 52"
                   "08 69 bb 9b 15 b1 01 39 00 ?? ?? bd 21 ?? ?? bd 00 20 21 1e 00 cc 21 1e 00 ?? ?? bd 3f ?? ?? b9"
                   "?? ?? ?? ?? 80 04 f8 37 08 68 bb 9b 09 c1 00 fc 49 01 80 52 09 c1 00 79 e9 ?? ?? b9 c9 03 f8 37"
                   "28 69 1b 9b 08 b1 41 39 08 01 00 34 00 ?? ?? bd 21 ?? ?? bd 00 20 21 1e 00 cc 21 1e 00 ?? ?? bd"
                   "00 e4 00 2f 02 00 00 14 20 ?? ?? bd 29 69 1b 9b 22 85 41 2d 21 c0 22 1e 42 c0 22 1e e9 e7 01 b2"
                   "49 33 93 f2 29 f7 e7 f2 23 01 67 9e 41 04 43 1f 21 40 62 1e 00 28 21 1e 20 ?? ?? bd c8 00 00 34"
                   "01 ?? ?? bd 20 20 20 1e 20 cc 20 1e 00 ?? ?? bd 3f ?? ?? b9 20 00 80 52 ?? ?? ?? ?? 21 ?? ?? 91"
                   "?? ?? ?? ?? 00 10 3e 1e 01 10 3e 1e 00 00 80 12 ?? ?? ?? ?? 20 00 80 52 01 00 80 52 ?? ?? ?? ??"
                   "1f 04 00 71 80 00 00 54 1f 08 00 71 61 e6 ff 54 7f 02 00 39 ?? ?? ?? ?? 68 ?? ?? 39 1f 05 00 71"
                   "01 01 00 54 ?? ?? ?? ?? 08 ?? ?? 91 00 05 40 ad e0 07 11 ad e0 83 08 91 ?? ?? ?? ?? 7f ?? ?? 39"
                   "a8 03 59 f8 ?? ?? ?? ?? 29 ?? ?? f9 29 01 40 f9 3f 01 08 eb 41 01 00 54 ff c3 0c 91 fd 7b 46 a9"
                   "f4 4f 45 a9 f6 57 44 a9 f8 5f 43 a9 fa 67 42 a9 fc 6f 41 a9 e9 23 c7 6c c0 03 5f d6 ?? ?? ?? ??"));
static const CodeCall NAME_BOX_CALLS[] = CODE_CALLS(
    /* windows */ ({0x049, RVA_PRINTF_OPTIONS}, {0x075, RVA_VSPRINTF}, {0x0a5, RVA_TEXT_INPUT_START}, {0x0c8, RVA_SCREEN_BEGIN},
                   {0x10b, RVA_SCREEN_TITLE}, {0x110, RVA_LAYOUT_NEW}, {0x13c, RVA_LINE_ADVANCE}, {0x148, RVA_LAYOUT_NEW},
                   {0x16c, RVA_LAYOUT_NEW}, {0x19e, RVA_DRAW_SPRITE}, {0x1a3, RVA_LAYOUT_NEW}, {0x1c6, RVA_LAYOUT_NEW},
                   {0x22a, RVA_TEXT_FIELD}, {0x22f, RVA_LAYOUT_NEW}, {0x2c5, RVA_LAYOUT_NEW}, {0x2f1, RVA_LINE_ADVANCE},
                   {0x302, RVA_TEXT_BUTTON}, {0x317, RVA_TEXT_BUTTON}, {0x325, RVA_LAYOUT_PASS}, {0x32f, RVA_WAIT_FOR_CLICK},
                   {0x342, RVA_TEXT_INPUT_STOP}, {0x357, RVA_TEXT_INPUT_STOP}, {0x369, RVA_SECURITY_CHECK_COOKIE}),
    /* mac     */ ({0x05c, RVA_VSPRINTF}, {0x068, RVA_GETTIMEOFDAY}, {0x0c0, RVA_GETENV}, {0x110, RVA_SYSTEM},
                   {0x158, RVA_SCREEN_BEGIN}, {0x1fc, RVA_SCREEN_TITLE}, {0x200, RVA_LAYOUT_NEW}, {0x298, RVA_LAYOUT_NEW},
                   {0x2b4, RVA_LAYOUT_NEW}, {0x2e4, RVA_DRAW_SPRITE}, {0x2e8, RVA_LAYOUT_NEW}, {0x308, RVA_LAYOUT_NEW},
                   {0x354, RVA_TEXT_FIELD}, {0x358, RVA_LAYOUT_NEW}, {0x3c0, RVA_LAYOUT_NEW}, {0x460, RVA_TEXT_BUTTON},
                   {0x470, RVA_LAYOUT_PASS}, {0x47c, RVA_WAIT_FOR_CLICK}, {0x4b8, RVA_SYSTEM}, {0x4fc, RVA_STACK_CHK_FAIL_STUB}));

static const char FIND_UNIT_AT_CODE[] = CODE_BYTES(
    /* windows */ ("40 53 55 56 41 56 41 57 b8 80 9c 00 00 e8 ?? ?? ?? ?? 48 2b e0 48 8b 05 ?? ?? ?? ?? 48 33 c4 48"
                   "89 84 24 60 9c 00 00 33 db 48 63 ea 4d 8b f1 45 8b f8 8b f1 85 d2 78 13 8b d5 e8 ?? ?? ?? ?? 85"
                   "c0 75 08 83 c8 ff e9 31 01 00 00 48 89 bc 24 b0 9c 00 00 44 8b c3 8b 3d ?? ?? ?? ?? 85 ff 0f 88"
                   "0d 01 00 00 4c 89 a4 24 78 9c 00 00 4c 8d 15 ?? ?? ?? ?? 4c 89 ac 24 70 9c 00 00 4c 8b db 4c 8d"
                   "2d ?? ?? ?? ?? 66 66 66 0f 1f 84 00 00 00 00 00 66 41 83 ba d2 00 00 00 00 0f 8e 9b 00 00 00 48"
                   "83 fd ff 74 28 45 85 c0 79 05 83 c8 ff eb 03 41 8b 02 3b c5 0f 85 80 00 00 00 45 85 c0 79 05 83"
                   "c8 ff eb 04 41 8b 42 04 41 3b c7 75 6d 83 fe ff 74 1b 45 85 c0 78 0f 44 3b c7 7f 0a 41 0f bf 82"
                   "d6 00 00 00 eb 03 83 c8 ff 3b c6 75 4d 41 80 3e 00 74 34 49 0f bf 82 d4 00 00 00 4d 8b ce 48 69"
                   "c0 d8 09 00 00 49 03 c5 4c 2b c8 0f 1f 44 00 00 0f b6 10 42 0f b6 0c 08 2b d1 75 07 48 ff c0 85"
                   "c9 75 ed 85 d2 75 13 49 81 fb 10 27 00 00 7d 0a ff c3 46 89 44 9c 20 49 ff c3 41 ff c0 49 81 c2"
                   "58 01 00 00 44 3b c7 0f 8e 43 ff ff ff 4c 8b ac 24 70 9c 00 00 4c 8b a4 24 78 9c 00 00 85 db 7e"
                   "10 8b cb e8 ?? ?? ?? ?? 48 63 c8 8b 44 8c 20 eb 03 83 c8 ff 48 8b bc 24 b0 9c 00 00 48 8b 8c 24"
                   "60 9c 00 00 48 33 cc e8 ?? ?? ?? ?? 48 81 c4 80 9c 00 00 41 5f 41 5e 5e 5d 5b c3"),
    /* mac     */ ("fc 6f ba a9 fa 67 01 a9 f8 5f 02 a9 f6 57 03 a9 f4 4f 04 a9 fd 7b 05 a9 fd 43 01 91 09 8a 93 52"
                   "?? ?? ?? ?? 10 ?? ?? f9 00 02 3f d6 ff 27 40 d1 ff 43 31 d1 f3 03 03 aa f4 03 02 aa f5 03 01 aa"
                   "f6 03 00 aa ?? ?? ?? ?? 08 ?? ?? f9 08 01 40 f9 a8 03 1a f8 61 06 f8 37 88 02 15 2a 00 00 80 12"
                   "a8 22 f8 37 ?? ?? ?? ?? 08 ?? ?? b9 ?? ?? ?? ?? 29 ?? ?? b9 1f 01 15 6b 24 c1 54 7a cd 21 00 54"
                   "?? ?? ?? ?? 08 ?? ?? 91 09 28 80 52 a8 22 a9 9b 08 d9 f4 78 c8 1c 00 34 56 04 f8 37 28 04 f8 37"
                   "00 00 80 12 df 7e 00 71 6c 20 00 54 e8 03 15 2a 89 7e 40 93 8a 27 80 52 0b b0 98 52 ?? ?? ?? ??"
                   "8c ?? ?? 91 08 31 0b 9b 28 21 0a 9b 08 75 c0 79 1f 7d 00 71 0c 1f 00 54 09 01 16 2a c9 1e f8 37"
                   "1f 01 16 6b e0 01 00 54 ?? ?? ?? ?? 29 ?? ?? 91 8a 00 92 52 08 25 2a 9b 08 61 c0 79 1f 05 00 71"
                   "6b 19 00 54 c9 26 2a 9b 29 61 c0 79 00 00 80 12 3f 05 00 71 0b 1d 00 54 1f 01 09 6b c1 1c 00 54"
                   "?? ?? ?? ?? 18 ?? ?? b9 38 18 f8 37 bf 06 00 31 80 07 00 54 18 7f 40 93 df 06 00 31 c0 0c 00 54"
                   "19 00 80 d2 17 00 80 52 1a 07 00 91 ?? ?? ?? ?? 7b ?? ?? 91 ?? ?? ?? ?? 9c ?? ?? 91 08 00 00 14"
                   "e8 03 00 91 19 d9 37 b8 f7 06 00 11 39 07 00 91 7b 63 05 91 5f 03 19 eb 00 15 00 54 68 c3 df 78"
                   "1f 05 00 71 4b ff ff 54 19 01 f8 37 68 a3 52 b8 1f 01 15 6b c1 fe ff 54 68 e3 52 b8 1f 01 14 6b"
                   "61 fe ff 54 06 00 00 14 bf 06 00 31 01 fe ff 54 08 00 80 12 1f 01 14 6b a1 fd ff 54 3f 03 18 eb"
                   "ad 00 00 54 08 00 80 12 1f 01 16 6b 01 fd ff 54 04 00 00 14 68 03 c0 79 1f 01 16 6b 81 fc ff 54"
                   "68 02 40 39 68 01 00 34 68 e3 9f 78 09 3b 81 52 00 71 09 9b e1 03 13 aa ?? ?? ?? ?? 1f 00 00 71"
                   "08 e2 84 52 e0 02 48 7a cb fa ff 54 d8 ff ff 17 e8 e1 84 52 ff 02 08 6b ac fa ff 54 d1 ff ff 17"
                   "df 06 00 31 60 0b 00 54 14 00 80 d2 17 00 80 52 15 07 00 91 ?? ?? ?? ?? 39 ?? ?? 91 1c 3b 81 52"
                   "1a e2 84 52 ?? ?? ?? ?? 7b ?? ?? 91 08 00 00 14 e8 03 00 91 14 d9 37 b8 f7 06 00 11 94 06 00 91"
                   "39 63 05 91 bf 02 14 eb 80 0d 00 54 28 c3 df 78 1f 05 00 71 4b ff ff 54 9f 02 18 eb a9 00 00 54"
                   "08 00 80 12 1f 01 16 6b a1 fe ff 54 04 00 00 14 28 03 c0 79 1f 01 16 6b 21 fe ff 54 68 02 40 39"
                   "28 01 00 34 28 e3 9f 78 00 6d 1c 9b e1 03 13 aa ?? ?? ?? ?? 1f 00 00 71 e0 02 5a 7a ab fc ff 54"
                   "e7 ff ff 17 e8 e1 84 52 ff 02 08 6b 8c fc ff 54 e0 ff ff 17 16 00 80 d2 17 00 80 52 18 07 00 91"
                   "?? ?? ?? ?? 39 ?? ?? 91 1c 3b 81 52 1a e2 84 52 ?? ?? ?? ?? 7b ?? ?? 91 08 00 00 14 e8 03 00 91"
                   "16 d9 37 b8 f7 06 00 11 d6 06 00 91 39 63 05 91 1f 03 16 eb 20 08 00 54 28 e3 df 78 1f 05 00 71"
                   "4b ff ff 54 16 01 f8 37 28 c3 52 b8 1f 01 15 6b c1 fe ff 54 28 03 53 b8 1f 01 14 6b 61 fe ff 54"
                   "06 00 00 14 bf 06 00 31 01 fe ff 54 08 00 80 12 1f 01 14 6b a1 fd ff 54 68 02 40 39 28 01 00 34"
                   "28 03 80 79 00 6d 1c 9b e1 03 13 aa ?? ?? ?? ?? 1f 00 00 71 e0 02 5a 7a 2b fc ff 54 e3 ff ff 17"
                   "e8 e1 84 52 ff 02 08 6b 0c fc ff 54 dc ff ff 17 14 00 80 d2 17 00 80 52 15 07 00 11 ?? ?? ?? ??"
                   "d6 ?? ?? 91 f8 e1 84 52 f9 03 00 91 1a 3b 81 52 1b e2 84 52 ?? ?? ?? ?? 9c ?? ?? 91 07 00 00 14"
                   "34 db 37 b8 f7 06 00 11 94 06 00 91 d6 62 05 91 bf 02 14 eb 20 02 00 54 c8 e2 df 78 1f 05 00 71"
                   "4b ff ff 54 68 02 40 39 28 01 00 34 c8 02 80 79 00 71 1a 9b e1 03 13 aa ?? ?? ?? ?? 1f 00 00 71"
                   "e0 02 5b 7a eb fd ff 54 f0 ff ff 17 ff 02 18 6b cc fd ff 54 eb ff ff 17 ff 06 00 71 8b 00 00 54"
                   "a1 00 00 54 08 00 80 d2 21 00 00 14 00 00 80 12 21 00 00 14 ?? ?? ?? ?? 2e ?? ?? b9 ?? ?? ?? ??"
                   "08 ?? ?? 91 0a 79 6e b8 ?? ?? ?? ?? 0b ?? ?? b9 4c 25 cb 1a cd 03 80 52 ad 01 0b 4b 4a 21 cd 1a"
                   "4a 01 0c 2a ?? ?? ?? ?? 8d ?? ?? b9 4a 01 0d 4a cf 05 00 11 2f ?? ?? b9 df cd 07 71 4b 01 00 54"
                   "3f ?? ?? b9 69 05 00 11 09 ?? ?? b9 7f 75 00 71 ab 00 00 54 1f ?? ?? b9 28 04 82 52 a8 01 08 0b"
                   "88 ?? ?? b9 48 09 d7 1a 08 a9 17 1b e9 03 00 91 20 79 68 b8 a8 03 5a f8 ?? ?? ?? ?? 29 ?? ?? f9"
                   "29 01 40 f9 3f 01 08 eb 41 01 00 54 ff 27 40 91 ff 43 31 91 fd 7b 45 a9 f4 4f 44 a9 f6 57 43 a9"
                   "f8 5f 42 a9 fa 67 41 a9 fc 6f c6 a8 c0 03 5f d6 ?? ?? ?? ??"));
static const CodeCall FIND_UNIT_AT_CALLS[] = CODE_CALLS(
    /* windows */ ({0x00d, RVA_CHKSTK}, {0x03a, RVA_ARMY_AT_FOR_PLAYER}, {0x163, RVA_RANDOM}, {0x187, RVA_SECURITY_CHECK_COOKIE}),
    /* mac     */ ({0x1f8, RVA_STRCMP_STUB}, {0x2b0, RVA_STRCMP_STUB}, {0x36c, RVA_STRCMP_STUB}, {0x3f8, RVA_STRCMP_STUB},
                   {0x4f0, RVA_STACK_CHK_FAIL_STUB}));

static const char FIND_ITEM_HOLDER_AT_CODE[] = CODE_BYTES(
    /* windows */ ("40 56 41 54 41 55 41 56 41 57 b8 80 9c 00 00 e8 ?? ?? ?? ?? 48 2b e0 48 8b 05 ?? ?? ?? ?? 48 33"
                   "c4 48 89 84 24 60 9c 00 00 33 f6 4c 63 e2 4d 8b f9 45 8b e8 44 8b f1 85 d2 78 14 41 8b d4 e8 ??"
                   "?? ?? ?? 85 c0 75 08 83 c8 ff e9 53 01 00 00 48 89 ac 24 78 9c 00 00 45 33 db 8b 2d ?? ?? ?? ??"
                   "85 ed 0f 88 2f 01 00 00 48 89 9c 24 b0 9c 00 00 48 8d 0d ?? ?? ?? ?? 48 89 bc 24 70 9c 00 00 48"
                   "8d 1d ?? ?? ?? ?? 33 ff 49 8b c4 0f 1f 44 00 00 66 83 bb d2 00 00 00 00 0f 8e a3 00 00 00 48 83"
                   "f8 ff 74 27 45 85 db 79 05 83 c8 ff eb 02 8b 03 41 3b c4 0f 85 85 00 00 00 45 85 db 79 05 83 c8"
                   "ff eb 03 8b 43 04 41 3b c5 75 73 41 83 fe ff 74 1b 45 85 db 78 0e 44 3b dd 7f 09 0f bf 83 d6 00"
                   "00 00 eb 03 83 c8 ff 41 3b c6 75 52 45 33 d2 4c 8d 8b c0 00 00 00 41 0f b7 01 66 85 c0 78 32 48"
                   "0f bf c0 4c 69 c0 d0 01 00 00 49 8b c7 4c 03 c1 4d 2b c7 0f b6 10 42 0f b6 0c 00 2b d1 75 07 48"
                   "ff c0 85 c9 75 ed 85 d2 74 55 48 8d 0d ?? ?? ?? ?? 49 ff c2 49 83 c1 02 49 83 fa 08 7c b8 49 8b"
                   "c4 41 ff c3 48 8d 0d ?? ?? ?? ?? 48 81 c3 58 01 00 00 44 3b dd 0f 8e 35 ff ff ff 48 8b bc 24 70"
                   "9c 00 00 48 8b 9c 24 b0 9c 00 00 85 f6 7e 28 8b ce e8 ?? ?? ?? ?? 48 63 c8 8b 44 8c 20 eb 1b 49"
                   "8b c4 48 81 ff 10 27 00 00 7d b6 ff c6 44 89 5c bc 20 48 ff c7 eb aa 83 c8 ff 48 8b ac 24 78 9c"
                   "00 00 48 8b 8c 24 60 9c 00 00 48 33 cc e8 ?? ?? ?? ?? 48 81 c4 80 9c 00 00 41 5f 41 5e 41 5d 41"
                   "5c 5e c3"),
    /* mac     */ ("fc 6f ba a9 fa 67 01 a9 f8 5f 02 a9 f6 57 03 a9 f4 4f 04 a9 fd 7b 05 a9 fd 43 01 91 09 8a 93 52"
                   "?? ?? ?? ?? 10 ?? ?? f9 00 02 3f d6 ff 27 40 d1 ff 43 31 d1 f3 03 03 aa f4 03 02 aa f5 03 01 aa"
                   "f6 03 00 aa ?? ?? ?? ?? 08 ?? ?? f9 08 01 40 f9 a8 03 1a f8 61 06 f8 37 88 02 15 2a 00 00 80 12"
                   "68 3b f8 37 ?? ?? ?? ?? 08 ?? ?? b9 ?? ?? ?? ?? 29 ?? ?? b9 1f 01 15 6b 24 c1 54 7a 8d 3a 00 54"
                   "?? ?? ?? ?? 08 ?? ?? 91 09 28 80 52 a8 22 29 9b 08 d9 f4 78 88 35 00 34 56 04 f8 37 28 04 f8 37"
                   "00 00 80 12 df 7e 00 71 2c 39 00 54 a8 7e 40 93 89 7e 40 93 8a 27 80 52 0b b0 98 52 ?? ?? ?? ??"
                   "8c ?? ?? 91 08 31 0b 9b 28 21 0a 9b 08 75 c0 79 1f 7d 00 71 cc 37 00 54 09 01 16 2a 89 37 f8 37"
                   "1f 01 16 6b e0 01 00 54 ?? ?? ?? ?? 29 ?? ?? 91 8a 00 92 52 08 25 2a 9b 08 61 c0 79 1f 05 00 71"
                   "2b 32 00 54 c9 26 2a 9b 29 61 c0 79 00 00 80 12 3f 05 00 71 cb 35 00 54 1f 01 09 6b 81 35 00 54"
                   "?? ?? ?? ?? 18 ?? ?? b9 f8 30 f8 37 19 7f 40 92 bf 06 00 31 40 0e 00 54 28 7f 40 93 ?? ?? ?? ??"
                   "18 ?? ?? 91 df 06 00 31 20 19 00 54 1a 00 80 d2 17 00 80 52 1b 05 00 91 ?? ?? ?? ?? 9c ?? ?? 91"
                   "08 00 00 14 e8 03 00 91 1a d9 37 b8 f7 06 00 11 5a 07 00 91 9c 63 05 91 7f 03 1a eb a0 2d 00 54"
                   "88 27 c0 79 1f 05 00 71 4b ff ff 54 1a 01 f8 37 88 03 54 b8 1f 01 15 6b c1 fe ff 54 88 43 54 b8"
                   "1f 01 14 6b 61 fe ff 54 06 00 00 14 bf 06 00 31 01 fe ff 54 08 00 80 12 1f 01 14 6b a1 fd ff 54"
                   "08 00 80 12 9a 00 f8 37 3f 03 1a 6b 4b 00 00 54 88 2f c0 79 1f 01 16 6b c1 fc ff 54 88 03 c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 07 00 34 88 07 c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 06 00 34 88 0b c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 05 00 34 88 0f c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 04 00 34 88 13 c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 03 00 34 88 17 c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 02 00 34 88 1b c0 79"
                   "e8 00 f8 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? a0 01 00 34 88 1f c0 79"
                   "88 f5 ff 37 08 3d 40 92 09 3a 80 52 01 61 09 9b e0 03 13 aa ?? ?? ?? ?? c0 f4 ff 35 08 e2 84 52"
                   "ff 02 08 6b 0b f4 ff 54 a2 ff ff 17 e8 e1 84 52 ff 02 08 6b ec f3 ff 54 9b ff ff 17 df 06 00 31"
                   "20 17 00 54 14 00 80 d2 17 00 80 52 15 07 00 91 ?? ?? ?? ?? 39 ?? ?? 91 1a 3a 80 52 fb e1 84 52"
                   "?? ?? ?? ?? 9c ?? ?? 91 08 00 00 14 e8 03 00 91 14 d9 37 b8 f7 06 00 11 94 06 00 91 39 63 05 91"
                   "bf 02 14 eb 60 1f 00 54 28 27 c0 79 1f 05 00 71 4b ff ff 54 08 00 80 12 94 00 f8 37 1f 03 14 6b"
                   "4b 00 00 54 28 2f c0 79 1f 01 16 6b 61 fe ff 54 28 03 c0 79 c8 00 f8 37 08 3d 40 92 01 71 1a 9b"
                   "e0 03 13 aa ?? ?? ?? ?? c0 06 00 34 28 07 c0 79 c8 00 f8 37 08 3d 40 92 01 71 1a 9b e0 03 13 aa"
                   "?? ?? ?? ?? e0 05 00 34 28 0b c0 79 c8 00 f8 37 08 3d 40 92 01 71 1a 9b e0 03 13 aa ?? ?? ?? ??"
                   "00 05 00 34 28 0f c0 79 c8 00 f8 37 08 3d 40 92 01 71 1a 9b e0 03 13 aa ?? ?? ?? ?? 20 04 00 34"
                   "28 13 c0 79 c8 00 f8 37 08 3d 40 92 01 71 1a 9b e0 03 13 aa ?? ?? ?? ?? 40 03 00 34 28 17 c0 79"
                   "c8 00 f8 37 08 3d 40 92 01 71 1a 9b e0 03 13 aa ?? ?? ?? ?? 60 02 00 34 28 1b c0 79 c8 00 f8 37"
                   "08 3d 40 92 01 71 1a 9b e0 03 13 aa ?? ?? ?? ?? 80 01 00 34 28 1f c0 79 08 f8 ff 37 08 3d 40 92"
                   "01 71 1a 9b e0 03 13 aa ?? ?? ?? ?? 60 f7 ff 35 08 e2 84 52 ff 02 08 6b ab f6 ff 54 b7 ff ff 17"
                   "ff 02 1b 6b ac f6 ff 54 b1 ff ff 17 16 00 80 d2 17 00 80 52 19 05 00 91 1a 3a 80 52 fb e1 84 52"
                   "?? ?? ?? ?? 9c ?? ?? 91 08 00 00 14 e8 03 00 91 16 d9 37 b8 f7 06 00 11 d6 06 00 91 9c 63 05 91"
                   "3f 03 16 eb 60 14 00 54 88 03 c0 79 1f 05 00 71 4b ff ff 54 16 01 f8 37 88 e3 52 b8 1f 01 15 6b"
                   "c1 fe ff 54 88 23 53 b8 1f 01 14 6b 61 fe ff 54 06 00 00 14 bf 06 00 31 01 fe ff 54 08 00 80 12"
                   "1f 01 14 6b a1 fd ff 54 88 e3 de 78 c8 00 f8 37 08 3d 40 92 01 61 1a 9b e0 03 13 aa ?? ?? ?? ??"
                   "c0 06 00 34 88 03 df 78 c8 00 f8 37 08 3d 40 92 01 61 1a 9b e0 03 13 aa ?? ?? ?? ?? e0 05 00 34"
                   "88 23 df 78 c8 00 f8 37 08 3d 40 92 01 61 1a 9b e0 03 13 aa ?? ?? ?? ?? 00 05 00 34 88 43 df 78"
                   "c8 00 f8 37 08 3d 40 92 01 61 1a 9b e0 03 13 aa ?? ?? ?? ?? 20 04 00 34 88 63 df 78 c8 00 f8 37"
                   "08 3d 40 92 01 61 1a 9b e0 03 13 aa ?? ?? ?? ?? 40 03 00 34 88 83 df 78 c8 00 f8 37 08 3d 40 92"
                   "01 61 1a 9b e0 03 13 aa ?? ?? ?? ?? 60 02 00 34 88 a3 df 78 c8 00 f8 37 08 3d 40 92 01 61 1a 9b"
                   "e0 03 13 aa ?? ?? ?? ?? 80 01 00 34 88 c3 df 78 48 f7 ff 37 08 3d 40 92 01 61 1a 9b e0 03 13 aa"
                   "?? ?? ?? ?? a0 f6 ff 35 08 e2 84 52 ff 02 08 6b eb f5 ff 54 b1 ff ff 17 ff 02 1b 6b ec f5 ff 54"
                   "ab ff ff 17 14 00 80 d2 17 00 80 52 35 07 00 91 ?? ?? ?? ?? d6 ?? ?? 91 18 3a 80 52 19 e2 84 52"
                   "fa 03 00 91 fb e1 84 52 ?? ?? ?? ?? 9c ?? ?? 91 07 00 00 14 54 db 37 b8 f7 06 00 11 94 06 00 91"
                   "d6 62 05 91 bf 02 14 eb 40 08 00 54 c8 26 c0 79 1f 05 00 71 4b ff ff 54 c8 02 c0 79 c8 00 f8 37"
                   "08 3d 40 92 01 71 18 9b e0 03 13 aa ?? ?? ?? ?? a0 06 00 34 c8 06 c0 79 c8 00 f8 37 08 3d 40 92"
                   "01 71 18 9b e0 03 13 aa ?? ?? ?? ?? c0 05 00 34 c8 0a c0 79 c8 00 f8 37 08 3d 40 92 01 71 18 9b"
                   "e0 03 13 aa ?? ?? ?? ?? e0 04 00 34 c8 0e c0 79 c8 00 f8 37 08 3d 40 92 01 71 18 9b e0 03 13 aa"
                   "?? ?? ?? ?? 00 04 00 34 c8 12 c0 79 c8 00 f8 37 08 3d 40 92 01 71 18 9b e0 03 13 aa ?? ?? ?? ??"
                   "20 03 00 34 c8 16 c0 79 c8 00 f8 37 08 3d 40 92 01 71 18 9b e0 03 13 aa ?? ?? ?? ?? 40 02 00 34"
                   "c8 1a c0 79 c8 00 f8 37 08 3d 40 92 01 71 18 9b e0 03 13 aa ?? ?? ?? ?? 60 01 00 34 c8 1e c0 79"
                   "e8 f8 ff 37 08 3d 40 92 01 71 18 9b e0 03 13 aa ?? ?? ?? ?? 1f 00 00 71 e0 02 59 7a cb f7 ff 54"
                   "bf ff ff 17 ff 02 1b 6b ac f7 ff 54 ba ff ff 17 ff 06 00 71 8b 00 00 54 a1 00 00 54 08 00 80 d2"
                   "21 00 00 14 00 00 80 12 21 00 00 14 ?? ?? ?? ?? 2e ?? ?? b9 ?? ?? ?? ?? 08 ?? ?? 91 0a 79 6e b8"
                   "?? ?? ?? ?? 0b ?? ?? b9 4c 25 cb 1a cd 03 80 52 ad 01 0b 4b 4a 21 cd 1a 4a 01 0c 2a ?? ?? ?? ??"
                   "8d ?? ?? b9 4a 01 0d 4a cf 05 00 11 2f ?? ?? b9 df cd 07 71 4b 01 00 54 3f ?? ?? b9 69 05 00 11"
                   "09 ?? ?? b9 7f 75 00 71 ab 00 00 54 1f ?? ?? b9 28 04 82 52 a8 01 08 0b 88 ?? ?? b9 48 09 d7 1a"
                   "08 a9 17 1b e9 03 00 91 20 79 68 b8 a8 03 5a f8 ?? ?? ?? ?? 29 ?? ?? f9 29 01 40 f9 3f 01 08 eb"
                   "41 01 00 54 ff 27 40 91 ff 43 31 91 fd 7b 45 a9 f4 4f 44 a9 f6 57 43 a9 f8 5f 42 a9 fa 67 41 a9"
                   "fc 6f c6 a8 c0 03 5f d6 ?? ?? ?? ??"));
static const CodeCall FIND_ITEM_HOLDER_AT_CALLS[] = CODE_CALLS(
    /* windows */ ({0x00f, RVA_CHKSTK}, {0x03e, RVA_ARMY_AT_FOR_PLAYER}, {0x171, RVA_RANDOM}, {0x1ad, RVA_SECURITY_CHECK_COOKIE}),
    /* mac     */ ({0x1f4, RVA_STRCMP_STUB}, {0x214, RVA_STRCMP_STUB}, {0x234, RVA_STRCMP_STUB}, {0x254, RVA_STRCMP_STUB},
                   {0x274, RVA_STRCMP_STUB}, {0x294, RVA_STRCMP_STUB}, {0x2b4, RVA_STRCMP_STUB}, {0x2d4, RVA_STRCMP_STUB},
                   {0x384, RVA_STRCMP_STUB}, {0x3a0, RVA_STRCMP_STUB}, {0x3bc, RVA_STRCMP_STUB}, {0x3d8, RVA_STRCMP_STUB},
                   {0x3f4, RVA_STRCMP_STUB}, {0x410, RVA_STRCMP_STUB}, {0x42c, RVA_STRCMP_STUB}, {0x448, RVA_STRCMP_STUB},
                   {0x4fc, RVA_STRCMP_STUB}, {0x518, RVA_STRCMP_STUB}, {0x534, RVA_STRCMP_STUB}, {0x550, RVA_STRCMP_STUB},
                   {0x56c, RVA_STRCMP_STUB}, {0x588, RVA_STRCMP_STUB}, {0x5a4, RVA_STRCMP_STUB}, {0x5c0, RVA_STRCMP_STUB},
                   {0x64c, RVA_STRCMP_STUB}, {0x668, RVA_STRCMP_STUB}, {0x684, RVA_STRCMP_STUB}, {0x6a0, RVA_STRCMP_STUB},
                   {0x6bc, RVA_STRCMP_STUB}, {0x6d8, RVA_STRCMP_STUB}, {0x6f4, RVA_STRCMP_STUB}, {0x710, RVA_STRCMP_STUB},
                   {0x808, RVA_STACK_CHK_FAIL_STUB}));

#ifndef __APPLE__
// @platform-split trace_filter: Windows only; the Mac trace functions weren't read for it
static const char TRACE_PROLOGUE_CODE[] =
    "48 89 4c 24 08 48 89 54 24 10 4c 89 44 24 18 4c 89 4c 24 20 53 b8 80 23 00 00 e8 ?? ?? ?? ?? 48"
    "2b e0 48 8b 05 ?? ?? ?? ?? 48 33 c4 48 89 84 24 60 23 00 00 83 3d ?? ?? ?? ?? ?? 48 8b d9 0f 8c"
    "?? ?? ?? ??";
static const char TRACE_ERROR_PROLOGUE_CODE[] =
    "48 89 4c 24 08 48 89 54 24 10 4c 89 44 24 18 4c 89 4c 24 20 53 b8 80 23 00 00 e8 ?? ?? ?? ?? 48"
    "2b e0 48 8b 05 ?? ?? ?? ?? 48 33 c4 48 89 84 24 60 23 00 00 83 3d ?? ?? ?? ?? 01 48 8b d9 7c ??";
static const CodeCall TRACE_PROLOGUE_CALLS[] = {{0x01a, RVA_CHKSTK}};

// @platform-split ceremony_sites: no Mac call
static const char SEAL_EVENT_STEP_CODE[] =
    "8b 05 ?? ?? ?? ?? be 0c 00 00 00 3b 05 ?? ?? ?? ?? 0f 8d ad 07 00 00 ff c0 89 05 ?? ?? ?? ?? 83"
    "f8 03 0f 85 b1 00 00 00";
#endif

#undef CODE_BYTES
#undef CODE_CALLS
#undef CODE_LIST
#endif   // GAME_ADDRESSES_CODE_TABLE
