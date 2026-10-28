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
#define WIN_NOT_LOOKED_UP      no_windows_address__not_searched_for_yet
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
#define RVA_TEXT_CODE                          ADDR(        WIN_FILE_FORMAT,   0x100002f7c)
#define RVA_STUBS                              ADDR(        WIN_FILE_FORMAT,   0x1005ada50)
#define RVA_HEADER_PADDING                     ADDR(        WIN_FILE_FORMAT,   0x1000013f0)

// ---- Main menu --------------------------------------------------------------------------------------
#define RVA_MAIN_MENU                          ADDR(        0x191a60,          0x10032db90)
#define RVA_MENU_PREFERENCES                   ADDR(        0x197d60,          0x10032b964)
#define RVA_NETWORK_SCREEN                     ADDR(        0x1f1a90,          MAC_INLINED)
#define RVA_MENU_ID5                           ADDR(        0x1b74b0,          0x100514604)
#define RVA_CREDITS                            ADDR(        0x0b6ad0,          MAC_INLINED)
#define RVA_TEST_GAME                          ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_READ_TIP                           ADDR(        0x23f220,          MAC_INLINED)
#define RVA_READ_TIP_2                         ADDR(        0x23eaa0,          MAC_INLINED)
#define RVA_COUNT_SAVES                        ADDR(        0x18b380,          0x1004eb098)
#define RVA_MENU_LOOP                          ADDR_DIFFERS(0x191b72,          0x10032dca4)
#define RVA_MODS_SCREEN_WAIT                   ADDR(        0x1ca7ef,          0x10032ef94)
#define RVA_NETWORK_SCREEN_WAIT                ADDR(        0x1f1d59,          0x10032e658)
#define RVA_MENU_FADE_ON                       ADDR(        0x1a40794,         0x100872bec)
#define RVA_FRAME_DELTA                        ADDR(        0xd63ec3c,         0x10c922c48)
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
#define RVA_MENU_BG_FADE_STEP                  ADDR(        0x191c93,          0x10032de0c)
#define RVA_MENU_FADE_TIMER_STEP               ADDR(        0x191cca,          0x10032de34)
#define RVA_MENU_FADE_RELOAD                   ADDR(        WIN_NO_EQUIVALENT, 0x10032df70)
#define RVA_MENU_BANNER_FADE_SET               ADDR(        0x1927f0,          0x10032e2f0)
#define RVA_MENU_LOOP_JUMP                     ADDR(        0x1927ff,          0x10032e2f4)
#define RVA_MENU_PREFS_CALL                    ADDR(        0x19284e,          0x10032e3cc)
#define RVA_MENU_TEST_GAME                     ADDR_DIFFERS(0x1928a3,          0x10032e990)
#define RVA_MENU_READ_TIP                      ADDR_DIFFERS(0x1928ef,          0x10032f120)
#define RVA_CREDITS_LOGO_CALL                  ADDR(        0x0b6c35,          0x10032e834)
#define RVA_QUIT_MESSAGE                       ADDR(        0x5822fb2,         MAC_NOT_FOUND)

// ---- Screens and widgets ------------------------------------------------------------------------
#define RVA_WAIT_FOR_CLICK                     ADDR(        0x23f240,          0x100484ef8)
#define RVA_DRAW_BUTTON                        ADDR(        0x0363a0,          0x10033f7d8)
#define RVA_DRAW_BUTTON_PLAIN                  ADDR(        0x036120,          0x10033fc38)
#define RVA_DRAW_FRAME_IMAGE                   ADDR_DIFFERS(0x036cc0,          0x10033f1a8)
#define RVA_DRAW_IMAGE                         ADDR(        0x0370c0,          0x10033ee50)
#define RVA_SCREEN_W                           ADDR(        0x435d1c,          0x104904c78)
#define RVA_SCREEN_H                           ADDR(        0x431824,          0x104904c7c)
#define RVA_DATA_DIR                           ADDR(        0x5828b30,         0x10c922038)

#define RVA_SCREEN_BEGIN                       ADDR(        0x03ae60,          0x10033bf84)
#define RVA_SCREEN_TITLE                       ADDR(        0x033610,          0x100341b10)
#define RVA_SECTION_BEGIN                      ADDR(        0x035560,          0x100340780)
#define RVA_SECTION_END                        ADDR(        0x035400,          0x100340ac4)
#define RVA_SECTION_HEADER                     ADDR(        0x0898a0,          0x10048fd44)
#define RVA_RADIO_OPTION                       ADDR(        0x0389a0,          0x10033e110)
#define RVA_TEXT_BUTTON                        ADDR(        0x039ad0,          0x10033d2e4)
#define RVA_BIND_HOTKEY                        ADDR(        0x03aae0,          0x10033c988)
#define RVA_BIND_HOTKEY_CTRL                   ADDR(        0x03a940,          0x10033caac)
#define RVA_BIND_HOTKEY_ALT                    ADDR(        0x03a8b0,          MAC_INLINED)
#define RVA_CHECKBOX                           ADDR(        0x038d50,          0x10033dabc)
#define RVA_TOOLTIP                            ADDR(        0x039180,          0x10033d888)
#define RVA_OPTION_TOOLTIP                     ADDR(        0x039350,          0x10033d76c)
#define RVA_TEXT                               ADDR(        0x038f60,          0x10033da20)
#define SCREEN_TITLE_TEXT_SIZE                 0x64
#define TEXT_BUTTON_TEXT_SIZE                  0x1388
#define TEXT_LINE_TEXT_SIZE                    0x1388
#define TOOLTIP_TEXT_SIZE                      0x1388
#define CHECKBOX_TEXT_SIZE                     0x3e8
#define RADIO_OPTION_TEXT_SIZE                 0x3e7
#define PROGRESS_TEXT_TEXT_SIZE                0x1388
#define RVA_LAYOUT_NEW                         ADDR(        0x03a3f0,          0x10033cbb4)
#define RVA_LAYOUT_PASS                        ADDR(        0x0304e0,          0x100341ba8)
#define RVA_GUI_ALLOC                          ADDR(        0x03ac50,          0x10033c810)
#define RVA_TRANSLATE_FORMAT                   ADDR(        0x03d200,          0x10033b6c4)
#define RVA_SCREEN_WIDTH                       ADDR(        0x03a6d0,          MAC_INLINED)
#define RVA_BUTTON_ROW                         ADDR(        0x038490,          MAC_INLINED)
#define RVA_SCREEN_LAYOUT                      ADDR(        0x0304c0,          MAC_INLINED)
#define RVA_CLICK_SOUND                        ADDR(        0x0b10c0,          MAC_INLINED)
#define RVA_SCREEN_MUSIC                       ADDR(        0x034c40,          MAC_INLINED)
#define RVA_ESCAPE_KEY                         ADDR_DIFFERS(0x12ab0222,        0x113997cf6)
#define RVA_DISABLED_BUTTON                    ADDR(        0x0396b0,          0x10033d5f8)
#define RVA_LINE_ADVANCE                       ADDR(        0x03a500,          MAC_INLINED)
#define RVA_DRAW_SPRITE                        ADDR_DIFFERS(0x035da0,          0x10033ff30)
#define RVA_LOAD_SPRITE                        ADDR_DIFFERS(0x0643c0,          0x10034e9fc)
#define RVA_SPRITE_BUTTON                      ADDR(        0x038380,          0x10033e1ac)
#define RVA_SPRITE_BUTTON_TINTED               ADDR(        0x0382e0,          MAC_INLINED)
#define RVA_DRAW_SPRITE_40                     ADDR(        0x035b70,          MAC_INLINED)
#define RVA_DRAW_SPRITE_30                     ADDR(        0x035b90,          MAC_INLINED)
#define SPRITE_KIND_MONSTER                    0x21
#define SPRITE_KIND_TERRAIN                    0x20
#define SPRITE_KIND_ITEM                       2
#define SPRITE_KIND_COUNT                      0x26
#define SPRITES_PER_KIND                       0x7fbc
#define RVA_SPRITE_DRAW_SCALED                 ADDR(        0x0651a0,          0x10034f5c4)
#define RVA_LOAD_SPRITE_FILE                   ADDR(        0x0648b0,          0x10034e378)
#define RVA_SPRITE_FILES                       ADDR(        0x5ea8420,         0x1062a5800)
#define RVA_SPRITE_REPLACED                    ADDR(        0x57f2f80,         0x1062a5930)
#define RVA_REPLACED_SPRITE                    ADDR(        0x064c20,          MAC_INLINED)
#define RVA_SPRITE_WIDTHS                      ADDR(        0x5c0eed0,         0x1062a6424)
#define RVA_SPRITE_HEIGHTS                     ADDR(        0x5ea8550,         0x106504ff4)
#define RVA_TOKEN_ITEM_ICON_CALL               ADDR(        0x0ff177,          0x10057ebac)
#define SPRITE_KIND_ICON                       0x1b
#define SPRITE_KIND_ITEM                       2
#define LAYOUT_TYPE_SPRITE_BUTTON              7
#define LAYOUT_TYPE_SPRITE                     0x14
#define RVA_MONSTER_PICKER                     ADDR(        0x0881b0,          MAC_INLINED)
#define RVA_RECRUIT_ROW                        ADDR(        0x109140,          MAC_INLINED)
#define RVA_SETUP_WIDTH_SCALE                  ADDR(        0x1a5abcc,         0x104904ca4)
#define RVA_SETUP_WIDTH_UNIT                   ADDR(        0x406c78,          MAC_IN_CODE)
#define RVA_ESCAPE_ON                          RVA_ESCAPE_KEY
#define RVA_TEXT_ROW                           ADDR_DIFFERS(0x03a100,          0x10033ccd0)
#define RVA_GUI_LAST_SLOT                      ADDR(        0x42dc24,          0x100872bf0)
#define RVA_LAYOUT_LAST                        ADDR(        0x436724,          0x100857618)
#define RVA_COLUMN_BASE                        ADDR(        0x42dcb4,          0x100858038)
#define RVA_TEXT_HEIGHT_BASE                   ADDR(        0x432218,          0x100862640)
#define RVA_SET_TEXT_SCALE                     ADDR(        0x03a660,          MAC_INLINED)
#define RVA_LINE_WIDTH                         ADDR(        0x57c630c,         0x10611b920)
#define RVA_WIDEST_LINE                        ADDR(        0x57ee7c8,         0x10611b918)
#define RVA_ROW_OPEN                           ADDR(        0x42dc8c,          0x10611b91c)
#define RVA_SCREEN_WIDTH_A                     ADDR(        0x436728,          0x100862684)
#define RVA_SCREEN_WIDTH_B                     ADDR(        0x437b18,          0x100862688)
#define RVA_GUI_NEXT_SLOT                      ADDR(        0x57ee7d0,         0x105b68178)
#define RVA_HOVERED_ID                         ADDR(        0x432204,          0x1008626ec)
#define RVA_TEXT_FIELD_FOCUS                   ADDR(        0x42e704,          0x1008626a0)
#define RVA_TEXT_FIELD_CURSOR                  ADDR(        0x57c591c,         0x10611b990)
#define RVA_LAYOUT_TOP_Y                       ADDR(        0x434974,          0x10085761c)
#define RVA_SCREEN_RESET_A                     ADDR(        0x435d20,          0x100872b64)
#define RVA_SCREEN_RESET_B                     ADDR(        0x430e54,          0x10086269c)
#define RVA_SCREEN_RESET_C                     ADDR(        0x42dcfc,          0x1008626a4)
#define RVA_SCREEN_RESET_BYTE                  ADDR(        WIN_NOT_FOUND,     0x106cee568)
#define RVA_LAYOUT_MARK_OPEN                   ADDR(        0x0297e0,          MAC_INLINED)
#define RVA_LAYOUT_MARK_CLOSE                  ADDR(        0x0297a0,          MAC_INLINED)
#define LAYOUT_TYPE_MARK                       0x67

#define RVA_ROW_STRIPE                         ADDR(        0x038590,          0x10033e74c)
#define RVA_ROW_BEGIN                          ADDR(        0x03a5e0,          MAC_INLINED)
#define RVA_ROW_AREA                           ADDR(        0x035830,          MAC_INLINED)
#define RVA_COLUMN_X                           ADDR(        0x03a380,          MAC_INLINED)
#define RVA_LIST_GAP                           ADDR(        0x038530,          MAC_INLINED)
#define RVA_LIST_STYLE                         ADDR(        0x038490,          MAC_INLINED)

#define RVA_GUI_TABLE                          ADDR(        0x12ab2b20,        0x10b979b10)
#define GUI_STRIDE                             0x78
#define GUI_ELEMENT_Y                          0x08
#define GUI_ELEMENT_WIDTH                      0x0c
#define GUI_ELEMENT_TOOLTIP                    0x4c
#define GUI_ELEMENT_LABEL                      0x60
#define RVA_LAYOUT_ELEMENTS                    ADDR(        0x1319abc0,        0x105ba4288)
#define LAYOUT_STRIDE                          0x70
#define LAYOUT_ELEMENT_COLUMN_X                0x0c
#define LAYOUT_ELEMENT_WIDTH                   0x10
#define LAYOUT_ELEMENT_TYPE                    0x60
#define LAYOUT_ELEMENT_LINE_END                0x6c
#define LAYOUT_ELEMENT_COLOUR                  0x14
#define LAYOUT_TYPE_LIST_STYLE                 8
#define LAYOUT_TYPE_BUTTON_ROW                 9
#define LAYOUT_TYPE_LIST_GAP                   10
#define LAYOUT_TYPE_COLUMN_X                   12
#define LAYOUT_TYPE_ROW_AREA                   15
#define LAYOUT_TYPE_ROW_BEGIN_LIMIT            100
#define RVA_LAYOUT_TABLE                       RVA_LAYOUT_ELEMENTS
#define RVA_TOOLTIP_TABLE                      ADDR(        0x13190ac0,        0x10c057810)
#define RVA_TOOLTIP_COUNT                      ADDR(        0x433fa8,          0x100857620)
#define TOOLTIP_SLOTS                          5000
#define RVA_TOOLTIP_SHOWN                      ADDR(        0x42dd1c,          0x1008626f0)
#define RVA_TOOLTIP_DRAWN                      ADDR(        0x42dc54,          0x1008626f8)
#define RVA_TOOLTIP_SHOWN_AT                   ADDR(        0x583605c,         0x10611be6c)
#define RVA_TOOLTIP_FADE_ON                    ADDR(        0x42f0d8,          0x10085762c)
#define RVA_TOOLTIP_LINE_HEIGHT                ADDR(        0x42dbfc,          0x100862644)
#define TOOLTIP_BOX_BOTTOM                     16
#define TOOLTIP_BOX_DROP                       2
#define RVA_DRAW_TOOLTIP                       ADDR(        0x029ce0,          MAC_INLINED)
#define RVA_TOOLTIP_FADE_START                 ADDR(        0x406928,          MAC_IN_CODE)
#define RVA_TOOLTIP_FADE_RATE                  ADDR(        0x4067c8,          MAC_IN_CODE)
#define RVA_TOOLTIP_LINES                      ADDR(        0x123e9ba0,        0x1061a69f8)
#define TOOLTIP_LINE_SIZE                      0x180
#define RVA_TOOLTIP_LINE_WIDTHS                ADDR(        0x123e8c00,        0x1062045f8)
#define RVA_TEXT_WRAP                          ADDR(        0x053c50,          0x10034d9d4)
#define RVA_BUTTON_CORE                        ADDR(        0x039790,          0x10033d45c)
#define RVA_TOOLTIP_STORE                      ADDR(        0x0393d2,          0x10033d7b8)
#define RVA_DRAW_SPRITE_LOAD_CALL              ADDR(        0x035dd1,          0x10033ff70)
#define RVA_NAME_BOX_SPRITE_CALL               ADDR_DIFFERS(0x041d4e,          0x10034a1b8)
#define RVA_FILE_BROWSER_FOLDER_ROW            ADDR(        0x040c54,          0x10034b1a8)
#define RVA_FILE_BROWSER_HOVERED_READ          ADDR_DIFFERS(0x0411fa,          0x10034b784)

#define RVA_OPEN_GUI_FONT                      ADDR(        0x0535e0,          0x10034c3b8)
#define RVA_FONT_HANDLES                       ADDR(        0x59daa70,         0x1061a67e8)
#define RVA_PUT_TEXT_IN_CACHE                  ADDR(        0x052680,          0x10034cb54)
#define RVA_RENDER_TEXT_PIXELS                 ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_TTF_RENDER_UTF8_BLENDED            ADDR_DIFFERS(0x33a690,          0x1000cf55c)
#define RVA_DRAW_TEXT                          ADDR(        0x02f380,          MAC_INLINED)
#define RVA_RENDER_TEXT                        ADDR(        0x0544e0,          0x10034d8c4)
#define RVA_TEXT_COLOUR                        ADDR(        0x430480,          0x100857660)
#define RVA_TEXT_CELL_COLOUR                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RITUAL_GREYED_ALPHA                    0.5f

// ---- Dialogs and messages -----------------------------------------------------------------------
#define RVA_CONFIRM                            ADDR(        0x042b30,          0x100348608)
#define RVA_NAME_BOX                           ADDR(        0x041bb0,          0x100349ed4)
#define RVA_NAME_BOX_END                       ADDR(        0x041f42,          0x10034a3d4)
#define NAME_BOX_LENGTH_MAX                    42
#define NAME_BOX_PROMPT_SIZE                   0x1f4
#define RVA_MESSAGE_BOX                        ADDR(        0x041f50,          0x100349628)
#define RVA_ENGLISH_TEXT_PASS                  ADDR(        0x05c8a0,          0x10033490c)
#define RVA_FILE_BROWSER                       ADDR(        0x03f430,          0x10034a3d4)
#define RVA_FILE_BROWSER_LAST                  ADDR(        0x43922c,          0x100862700)
#define RVA_FILE_BROWSER_NAMES                 ADDR(        0x593afd0,         0x106124539)
#define RVA_FILE_BROWSER_FOLDERS               ADDR(        0x5992fd0,         0x106164539)
#define FILE_BROWSER_NAME_SIZE                 0x80
#define FILE_BROWSER_ROWS_MAX                  0x800
#define RVA_HOVERED_ELEMENT                    ADDR(        0x029cd0,          MAC_INLINED)
#define RVA_FIND_HOVERED                       ADDR(        0x02cac0,          0x100347b28)
#define RVA_ADD_MESSAGE                        ADDR(        0x1e1a60,          0x1004c1f18)
#define RVA_MESSAGE_TABLE                      ADDR(        0xe4a54c0,         0x1085ba830)
#define MESSAGE_SIZE                           0x8a0
#define MESSAGE_TO                             0x0
#define ADD_MESSAGE_TEXT_SIZE                  0x800
#define MESSAGE_TITLE                          0x6
#define MESSAGE_TEXT                           0x6a
#define RVA_EVENT_MESSAGE_CALL                 ADDR(        0x1c72da,          0x10052c01c)

// ---- Game flow ----------------------------------------------------------------------------------
#define RVA_PLAY_GAME                          ADDR(        0x091ce0,          0x100389844)
#define RVA_NEW_WORLD                          ADDR(        0x303360,          0x100587238)
#define RVA_MAP_SETTINGS_SCREEN                ADDR(        0x2aa590,          0x1004b2454)
#define RVA_PROGRESS_TEXT                      ADDR(        0x0899e0,          0x1004900c0)
#define RVA_PROGRESS_LAST_DRAW                 ADDR(        0x58033b8,         0x106cf0180)
#define RVA_FRAME_DELAY_MS                     ADDR(        0x1a532ec,         0x104904cac)
#define RVA_RUN_ONE_MOD_PROGRESS_CALL          ADDR_DIFFERS(0x1bb8d7,          0x10052a52c)
#define RVA_STR_CUSTOM_EVENTS                  ADDR(        0x1c61220,         0x100706cee)
#define RVA_MAP_SCREEN                         ADDR(        0x0949d0,          0x10037b204)
#define RVA_LOAD_GAME                          ADDR(        0x185f10,          0x1004eb2a8)
#define RVA_RESET_GAME_STATE                   ADDR(        0x31cd00,          0x100585bac)
#define RVA_RESET_SQUARES                      ADDR(        0x3030d0,          MAC_NOT_LOOKED_UP)
#define RVA_LOAD_MAP                           ADDR(        0x1b5a80,          0x100513c64)
#define RVA_INIT_PARTICIPANTS                  ADDR(        0x2ea3a0,          MAC_INLINED)
#define RVA_NETWORK_PARTICIPANTS               ADDR(        0x1ed810,          0x100502d0c)
#define RVA_COMMAND_LINE_SWITCH                ADDR(        0x1952c0,          0x1003289ec)
#define RVA_SWITCH_DEBUG_LEVEL_ADD             ADDR(        0x190277,          0x1003294a4)
#define RVA_CREATE_LOBBY_GAME                  ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_GAME_MENU                          ADDR(        0x124550,          MAC_INLINED)
#define RVA_EXIT_GAME                          ADDR(        0x199420,          0x10032b4c8)
#define RVA_GAME_START_PREP                    ADDR(        0x1b0e90,          0x100491c3c)
#define RVA_GAME_START_PREP_DONE               ADDR_DIFFERS(0x582b264,         0x107dcb5cc)
#define RVA_GAME_START_ONCE_TEST               ADDR_DIFFERS(0x09208b,          0x100491c6c)
#define RVA_PLAYER_TURN_START                  ADDR(        0x167ce0,          0x100392874)
#define RVA_AI_MOVE                            ADDR_DIFFERS(0x06f9d0,          0x1004e05a8)
#define RVA_AI_COMMANDER_RITUALS               ADDR(        0x06ae20,          0x1004dcd14)
#define RVA_AI_PLAN_TRACE_CALL                 ADDR(        0x06be3f,          0x1004de1e4)
#define RVA_AI_RITUAL_CANDIDATE_CALL           ADDR(        0x06bb37,          0x1004de000)
#define RVA_AI_MOVE_ALL_COMMANDERS             ADDR(        0x070400,          0x1004d4648)
#define RVA_AI_COMMANDER_ROUTINE               ADDR(        0x07a9b0,          0x1004d17e4)
#define RVA_AI_RECRUIT                         ADDR(        0x0773f0,          MAC_INLINED)
#define RVA_AI_RECRUIT_CALL                    ADDR(        0x06fb2b,          MAC_INLINED)
#define RVA_AI_RECRUIT_COPY_START              ADDR(        WIN_NO_EQUIVALENT,0x1004e0cd8)
#define RVA_AI_RECRUIT_COPY_END                ADDR(        WIN_NO_EQUIVALENT,0x1004e38bc)
#define RVA_AI_CONQUER_STEP                    ADDR(        0x075400,          MAC_INLINED)
#define RVA_AI_EXPLORE_STEP                    ADDR(        0x074350,          MAC_INLINED)
#define RVA_AI_RALLY_COPY_START                ADDR(        WIN_NO_EQUIVALENT,0x1004d62cc)
#define RVA_AI_RALLY_COPY_END                  ADDR(        WIN_NO_EQUIVALENT,0x1004dbb38)
#define RVA_AI_RALLY_COPY_NONE                 ADDR(        WIN_NO_EQUIVALENT,0x1004d64b0)
#define RVA_AI_FRIENDLY_COPY_START             ADDR(        WIN_NO_EQUIVALENT,0x1004d64b8)
#define RVA_AI_FRIENDLY_COPY_END               ADDR(        WIN_NO_EQUIVALENT,0x1004d86e8)
#define RVA_AI_CONQUER_COPY_START              ADDR(        WIN_NO_EQUIVALENT,0x1004d8730)
#define RVA_AI_CONQUER_COPY_END                ADDR(        WIN_NO_EQUIVALENT,0x1004d9b4c)
#define RVA_AI_EXPLORE_COPY_START              ADDR(        WIN_NO_EQUIVALENT,0x1004d9b90)
#define RVA_AI_EXPLORE_COPY_END                ADDR(        WIN_NO_EQUIVALENT,0x1004da510)
#define AI_MOVE_MAC_COMMANDER_AT               0xe8
#define AI_MOVE_MAC_COMSTR_AT                  0x78
#define RVA_AI_EXPLORE_STEP_CALL               ADDR(        0x070d8a,          MAC_INLINED)
#define RVA_AI_FAR_EXPLORE_STEP                ADDR(        0x073e40,          0x1004d3888)
#define RVA_AI_RALLY_STEP                      ADDR(        0x076730,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RALLY_STEP_CALL                 ADDR(        0x070caa,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_MOVE_STEP              ADDR(        0x076320,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_MOVE_STEP_CALL         ADDR(        0x070cf3,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_STEP_CALL               ADDR(        0x070d3c,          MAC_INLINED)
#define RVA_AI_FAR_EXPLORE_STEP_CALL           ADDR(        0x070dd8,          MAC_NOT_LOOKED_UP)
#define RVA_AI_THREAT_NEAR                     ADDR(        0x077050,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_STRENGTH                 ADDR_DIFFERS(0x129320,          0x10047f724)
#define RVA_AI_COMMANDER_CAN_MOVE              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_MOVE_ORDER_ROUTE                   ADDR(        0x09e5c0,          0x100373d38)
#define RVA_PATH_FOG                           ADDR_DIFFERS(0x58355f8,         0x1121124dc)
#define RVA_PATH_NO_TIE_RANDOM                 ADDR(        0x58355fc,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SPREAD                        ADDR(        0xe6c05c0,         0x1121124f0)
#define RVA_PATH_X                             ADDR(        0xe75c9c0,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_Y                             ADDR(        0xe75cdc0,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_LENGTH                        ADDR(        0x5835608,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SQUARE_CACHE_RESET            ADDR(        0x1dc6e0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_ALLY_TARGETS_NEAR         ADDR(        0x079a20,          MAC_INLINED)
#define RVA_COMMANDER_AI_TARGET_X              ADDR(        0x11c0dd40,        0x10c6368c4)
#define RVA_COMMANDER_AI_TARGET_Y              ADDR(        0x11d37a40,        0x10c63b6d0)
#define RVA_COMMANDER_AI_MOVE_KIND             ADDR(        0x11d32c20,        0x10859d368)
#define RVA_WAYTOGO                            ADDR_DIFFERS(0x1db2d0,          0x10059bf28)
#define RVA_WAYTOGO2                           ADDR_DIFFERS(0x1df8c0,          0x10059d914)
#define RVA_WAYTOGO2_EXTRA_ARG                 ADDR_DIFFERS(0x1df8c0,          0x10059d914)
#define RVA_WAYTOGO_FAR_SQUARE_CHECK           ADDR(        0x1dc450,          0x10059bb54)
#define RVA_FAR_CHECK_SQUARE_CALL              ADDR(        0x1dc672,          0x10059bee0)
#define RVA_FAR_CHECK_SQUARE_CALL_2            ADDR(        0x1dc68d,          MAC_NO_CALL)
#define RVA_FAR_CHECK_SQUARE_CALL_3            ADDR(        0x1dc6a1,          MAC_NO_CALL)
#define RVA_MAY_ENTER_SQUARE                   ADDR(        0x1df5a0,          0x10059a34c)
#define RVA_MAY_ENTER_SQUARE_CORE              ADDR(        0x1dd340,          0x100599eec)
#define RVA_RANDOM_PORTAL_EXIT                 ADDR(        0x2555e0,          0x100571630)
#define RVA_ARMY_MOVE_FLAGS                    ADDR(        0x329610,          0x10056a184)
#define RVA_ARMY_UNIT_FLAG_42_CHECK            ADDR(        0x325c20,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_IS_SEA                       ADDR(        0x2ae130,           MAC_INLINED)
#define RVA_AI_STATIONARY_CALL                 ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_AI_EXPLORE_GATE_CMP                ADDR(        0x070d6f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FAR_EXPLORE_GATE_CMP            ADDR(        0x070dbd,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_ROUTE_CALL                 ADDR(        0x070f74,          MAC_NOT_LOOKED_UP)
#define RVA_AI_DANGEROUS_STEP_CALL             ADDR(        0x071091,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_STEP_CALL                  ADDR(        0x07115f,          0x1004db724)
#define RVA_AI_GOLDRALLY_STOP_CALL             ADDR(        0x071627,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RALLY_ALLY_COUNT_CALL           ADDR(        0x076f1e,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_PLAN_BONUS_CALL        ADDR(        0x076564,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FRIENDLY_THREAT_CALL            ADDR(        0x076596,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_STRENGTH_CALL           ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_PLAN_BONUS_CALL         ADDR(        0x075ac6,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_KILLER_ROUTE_CALL       ADDR(        0x075f3f,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_ALLY_COUNT_CALL         ADDR(        0x076018,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_THREAT_CUTOFF           ADDR(        0x07603b,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_THREAT_CALL             ADDR(        0x076050,          MAC_NOT_LOOKED_UP)
#define RVA_AI_CONQUER_ROUTE_CALL              ADDR(        0x0761c2,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_OTHER                  ADDR(        0x5be324c,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_AI                     ADDR(        0x5be3e08,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_BOUND_HUMAN                  ADDR(        0x5be3e0c,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_STRENGTH_CALL                ADDR(        0x1dc672,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_EXEMPT_X                     ADDR(        0x1aa809c,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_EXEMPT_Y                     ADDR(        0x1aa7ce4,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_NO_SEA                        ADDR(        0x583560c,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_NO_SKY                        ADDR(        0x5835610,         MAC_NOT_LOOKED_UP)
#define RVA_PATH_SPREAD_SQUARE                 ADDR(        0x1dbf70,          MAC_INLINED)
#define RVA_SQUARE_MOVE_COST                   ADDR(        0x1dc730,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_IS_SKY                      ADDR(        0x2abbd0,          MAC_NOT_LOOKED_UP)
#define RVA_PLANE_SAME_SPOT                    ADDR(        0x2544f0,          0x1005707f0)
#define RVA_MOVE_STEP_BEFORE                   ADDR_DIFFERS(0x1df520,          0x10059da7c)
#define RVA_MOVE_STEP_AFTER                    ADDR(        0x1df6c0,          0x10059db74)
#define RVA_OVERRUN_CHECK                      ADDR(        0x1dd0a0,          MAC_NOT_LOOKED_UP)
#define RVA_SPEND_ALL_AP_AT                    ADDR(        0x105560,          0x1004815b8)
#define RVA_SHIPMOVE_SPEND_AP                  ADDR(        0x1057d0,          MAC_NOT_LOOKED_UP)
#define RVA_STEP_TARGET_X                      ADDR(        0x1aa910c,         0x105a0aeb8)
#define RVA_STEP_TARGET_Y                      ADDR(        0x1aa7e34,         0x105a0aebc)
#define RVA_STEP_TARGET_WATCHED                ADDR_DIFFERS(0x5835604,         0x1121af108)
#define RVA_GROUP_MOVE                         ADDR(        0x1deb20,          0x10059b730)
#define RVA_HUMAN_MULTI_MOVE                   ADDR(        0x09e000,          0x100376df4)
#define RVA_CONTINUE_MOVE_ORDERS               ADDR(        0x09df00,          0x10037731c)
#define RVA_MAP_SCREEN_MULTI_MOVE_CALL         ADDR(        0x098c96,          0x10038804c)
#define RVA_GATEWAY_BUTTON_CALL                ADDR(        0x099c8d,          0x10037de80)
#define RVA_COMMANDER_PANEL                    ADDR(        0x099450,          MAC_INLINED)
#define RVA_COMMANDER_PANEL_CALL               ADDR(        0x0951c3,          MAC_INLINED)
#define RVA_COMMANDER_PANEL_FRAME_CALL         ADDR_DIFFERS(0x09b122,          0x10037f3b0)
#define RVA_CMD_PANEL_CACHED_FOR               ADDR(        0x43d0ac,          MAC_NOT_LOOKED_UP)
#define RVA_CMD_PANEL_CHOP                     ADDR(        0x5804ba0,         0x1069b1680)
#define RVA_CMD_PANEL_BURN                     ADDR(        0x5804b94,         0x1069b1684)
#define RVA_UI_TEXT_SCALE                      ADDR(        0x1a593e4,         0x1049c8350)
#define RVA_GATEWAY_PORTAL_READ                ADDR(        0x098fb1,          0x100388330)
#define RVA_GATEWAY_EXIT_CALL                  ADDR(        0x098fd0,          0x100388350)
#define RVA_GATEWAY_REVEAL_CALL                ADDR_DIFFERS(0x099005,          0x1003883b0)
#define RVA_GATEWAY_GROUP_MOVE_CALL            ADDR(        0x099077,          0x1003884fc)
#define RVA_MOVE_REFUSED_TEXT_CALL             ADDR_DIFFERS(0x099157,          0x1003885e4)
#define RVA_STEP_PORTAL_TO_READ                ADDR(        0x1dfd2b,          0x10059a700)
#define RVA_STEP_PORTAL_FROM_READ              ADDR(        0x1dfd69,          0x10059a6e4)
#define RVA_STEP_REFUSED_RETURN                ADDR_DIFFERS(0x1e0302,          0x10059a80c)
#define RVA_SPREAD_PORTAL_READ                 ADDR(        0x1dc25f,          0x10059cc8c)
#define RVA_WAYTOGO_BACK_PORTAL_READ           ADDR(        0x1dba80,          0x10059d3f8)
#define RVA_MONSTER_PATH                       ADDR(        0x1dedf0,          0x10059de44)
#define RVA_MONSTER_PATH_SPREAD_SQUARE         ADDR(        0x1daeb0,          MAC_INLINED)
#define RVA_MONSTER_PATH_PORTAL_READ           ADDR(        0x1db167,          0x10059e780)
#define RVA_INDEP_PORTAL_READ                  ADDR(        0x16d8c2,          0x1005471e8)
#define RVA_AI_EXPLORE_PORTAL_READ             ADDR(        0x074a9a,          0x1004dbcb4)
#define RVA_WANDER_MOVE_STEP_CALL              ADDR(        0x17d571,          0x10038db00)
#define RVA_WANDER_MOVE_STEP_CALL_2            ADDR(        0x17d7a4,          0x10038debc)
#define RVA_TUNNEL_DIG                         ADDR(        0x128160,          0x1004848e8)
#define RVA_METEOR_STRIKE                      ADDR(        0x12de10,          MAC_INLINED)
#define RVA_NET_RECEIVE_SQUARE                 ADDR(        0x1eb9e0,          0x100505fec)
#define RVA_GROUP_MOVE_STEP_CALL               ADDR(        0x1decfc,          0x10059ba24)
#define RVA_INDEP_EXIT_CALL                    ADDR(        0x16d8f4,          0x100547218)
#define RVA_SPREAD_EXIT_CHECK_CALL             ADDR(        0x1dc366,          0x10059ce94)
#define RVA_SPREAD_EXIT_CHECK_CALL_2           ADDR(        WIN_NO_EQUIVALENT, 0x10059cf78)
#define RVA_WAYTOGO_BACK_TRACE_CALL            ADDR(        0x1dbb3d,          0x10059d5a0)
#define RVA_WAYTOGO_BACK_TRACE_CALL_2          ADDR(        WIN_NO_EQUIVALENT, 0x10059d6b0)
#define RVA_WAYTOGO_CALL                       ADDR(        0x1df93c,          0x10059d970)
#define RVA_WAYTOGO_CALL_2                     ADDR(        0x1dfa04,          0x10059da24)
#define MOVE_STEP_REFUSED                      -9
#define RVA_ARMY_ABILITY_TOTAL                 ADDR(        0x12c120,          0x10046915c)
#define RVA_AI_PROGRESS_DRAW                   ADDR(        0x07df90,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_CITADELS                  ADDR(        0x077330,          0x1004d30a4)
#define RVA_AI_CITADEL_COUNT                   ADDR(        0x59eac7c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_ATTACK_DIV_OTHER                ADDR(        0x1aa7c64,         0x105a0aeac)
#define RVA_AI_ATTACK_DIV_AI                   ADDR(        0x1aa6664,         0x105a0aeb0)
#define RVA_AI_ATTACK_DIV_HUMAN                ADDR(        0x1aa634c,         0x105a0aeb4)
#define AI_ATTACK_MARGIN                       15
#define RVA_AI_ATTACK_MARGIN                   ADDR(        0x1aa7594,         MAC_IN_CODE)
#define RVA_AI_SET_ATTACK_DIVS_COPY            ADDR(        0x06fc20,          MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY                 ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY_SCAN            ADDR(        0x07c5e0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_HANDS                     ADDR(        0x5802f58,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_HERBS                     ADDR(        0x5802f64,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_FUNGUS                    ADDR(        0x580335c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_SACRIFICES                ADDR(        0x5802f5c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_WEED                      ADDR(        0x5803358,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_GEMS                      ADDR(        0x5802f54,         MAC_NOT_LOOKED_UP)
#define RVA_AI_IRON_FLAG                       ADDR(        0x5802f68,         MAC_NOT_LOOKED_UP)
#define RVA_AI_WANTS_LIFEFORCE                 ADDR(        0x5802f6c,         MAC_NOT_LOOKED_UP)
#define RVA_AI_SAVE_RESOURCE                   ADDR(        0x6128620,         MAC_NOT_LOOKED_UP)
#define RVA_AI_UPDATE_STRENGTH_MEMORY          ADDR(        0x06d870,          0x1004e4204)
#define RVA_AI_STRENGTH_MEMORY_CALL            ADDR(        0x06fb32,          0x1004e38c0)
#define RVA_AI_STRENGTH_MEMORY_CALL_2          ADDR(        0x06fb47,          0x1004e3ba0)
#define RVA_AI_STRENGTH_MEMORY_CALL_3          ADDR(        WIN_NO_EQUIVALENT, 0x1004e3bb8)
#define RVA_AI_STRENGTH_MEMORY_CALL_4          ADDR(        WIN_NO_EQUIVALENT, 0x1004e3c8c)
#define RVA_AI_STRENGTH_MEMORY_SQUARE_CALL     ADDR(        0x06d917,          0x1004e42cc)
#define RVA_AI_STRENGTH_MEMORY_PROGRESS_CALL   ADDR(        0x06d9d2,          0x1004e42e4)
#define RVA_AI_REMEMBERED_STRENGTH             ADDR(        0x06d800,          MAC_NOT_LOOKED_UP)
#define RVA_AI_DANGEROUS_STEP                  ADDR(        0x0797a0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_TROOPS                   ADDR(        0x06edd0,          MAC_INLINED)
#define RVA_AI_GATHER_TROOPS_CALL              ADDR(        0x070f03,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_COPY_START               ADDR(        WIN_NO_EQUIVALENT,0x1004da6f8)
#define RVA_AI_GATHER_COPY_END                 ADDR(        WIN_NO_EQUIVALENT,0x1004db4e4)
#define RVA_AI_GATHER_LEAVE_UNLINK             ADDR(        0x06f37a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATHER_DROP_UNLINK              ADDR(        0x06f722,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MOVE_COMSTR_CALL                ADDR(        0x070f0d,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_NO_GATHER                ADDR(        0x0b12b0,          MAC_INLINED)
#define RVA_COMMANDER_FLAGGED_STRENGTH         ADDR(        0x129320,          0x10047f604)
#define RVA_UNIT_LEADER_OF                     ADDR(        0x32a870,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SQUARE                        ADDR(        0x329de0,          MAC_NOT_LOOKED_UP)
#define UNIT_AI_SAILOR                         0x200000000000
#define UNIT_FLAGS_AI_SKIP                     0x4000000022000000
#define UNIT_INSANE                            0x2000
#define ABILITY_WANDER                         0x4b9
#define ABILITY_FARSIGHT                       0x92
#define ABILITY_FARSIGHTSEA                    0x50b
#define ABILITY_WANDERATTACK                   0x4c9
#define ABILITY_REVERTOWNER                    0x1ef
#define RVA_REVERTOWNER_BOUND                  ADDR(        0x107da7,          0x1004730a4)
#define RVA_PLAYER_TINT_CLAMP                  ADDR(        0x2e8989,          0x10056d6b4)
#define UNIT_RESISTANCE_NOLEADER               39
#define UNIT_AI_GUARD                          0x8000000000
#define RVA_AI_SPLIT_ARMY                      ADDR(        0x07b700,          MAC_INLINED)
#define RVA_AI_SPLIT_ARMY_CALL                 ADDR(        0x07b55f,          MAC_INLINED)
#define RVA_AI_SPLIT_MIN_UNITS                 ADDR(        WIN_IN_CODE,       MAC_NOT_LOOKED_UP)
#define RVA_AI_SPLIT_MIN_UNITS_TRACE           ADDR(        0x07b82a,          0x1004d2680)
#define RVA_AI_SPLIT_MIN_UNITS_TEST            ADDR(        0x07b83a,          0x1004d2698)
#define RVA_AI_SPLIT_COPY_START                ADDR(        WIN_NO_EQUIVALENT,0x1004d2500)
#define RVA_AI_SPLIT_COPY_END                  ADDR(        WIN_NO_EQUIVALENT,0x1004d2570)
#define AI_SPLIT_MIN_UNITS_MAC                 240
#define RVA_AI_MAKE_GUARDS_AT                  ADDR(        0x06ebd0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_DEFENDERS                  ADDR(        0x06fe90,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RANDOM_ENEMY                    ADDR(        0x07b5e0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_MOVE_START                 ADDR(        0x06de10,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_MOVE_END                   ADDR(        0x06da50,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_TEXT                       ADDR(        0x06ad70,          MAC_NOT_LOOKED_UP)
#define RVA_AI_COUNT_TROOPS                    ADDR(        0x079620,          MAC_NOT_LOOKED_UP)
#define RVA_AI_UNIT_STAYS                      ADDR(        0x07dc20,          MAC_NOT_LOOKED_UP)
#define RVA_AI_REFORMS_HERE                    ADDR(        0x07d150,          0x1004d0fec)
#define RVA_AI_UNIT_JOIN_KIND                  ADDR(        0x07ca20,          0x1004d1158)
#define RVA_AI_PICK_UP_LONERS                  ADDR(        0x07be60,          0x1004d12fc)
#define RVA_AI_PICK_UP_LONER_CALL              ADDR(        0x07c26d,          0x1004d16a8)
#define RVA_AI_UNIT_MAY_GUARD                  ADDR(        0x07a450,          0x1004d2aec)
#define RVA_AI_BUY_COMMANDER                   ADDR(        0x07a110,          MAC_INLINED)
#define RVA_AI_BUY_TROOPS                      ADDR(        0x079c30,          MAC_INLINED)
#define RVA_AI_BUY_UPGRADE_UNLINK              ADDR(        0x079e85,          MAC_NOT_LOOKED_UP)
#define RVA_RESET_INCOME_CACHES                 ADDR(        0x3295b0,           0x100567fa8)
#define RVA_AI_MOVE_ALL_CALL                   ADDR(        0x06fb39,          MAC_NOT_LOOKED_UP)
#define RVA_AI_MAKE_DEFENDERS_CALL             ADDR(        0x06fb40,          MAC_NOT_LOOKED_UP)
#define RVA_AI_TRADE_CHOICE_CALL               ADDR(        0x06fb4e,          MAC_NOT_LOOKED_UP)
#define RVA_INT16_FILL                         ADDR(        0x058fd0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_STRONGEST_ENEMY_CALL            ADDR(        0x070603,          MAC_NOT_LOOKED_UP)
#define RVA_AI_VIEW_PICK_UNIT                  ADDR(        0x1b32a0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUALS_TAINT_TEST              ADDR(        0x06af22,          MAC_NOT_LOOKED_UP)
#define RVA_AI_APPRSPAM_EXECUTE_CALL           ADDR(        0x06b256,          0x1004dd2e8)
#define RVA_AI_PLAN_GATE_ROLL                  ADDR(        0x06b337,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_CANNOT_CAST          ADDR(        0x06be6a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_ACCEPT_STORE               ADDR(        0x06be90,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_POINTLESS            ADDR(        0x06be6a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_AI_BAD               ADDR(        0x06be6a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_ANYWHERE_CALL              ADDR(        0x06bee0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SCRAP_NOWHERE              ADDR(        0x06be6a,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_TARGET_CALL                ADDR(        0x06c04e,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_PAY_CALL                   ADDR(        0x06c1e4,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SAVE_RESOURCE_STORE             ADDR(        0x06c28c,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_EXECUTE_CALL               ADDR(        0x06c2df,          0x1004de784)
#define RVA_AI_PLAN_DONE_STORE                 ADDR(        0x06c300,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ALWAYS_EXECUTE_CALL             ADDR(        0x06c451,          0x1004de5b0)
#define RVA_AI_RITUALS_PICK_UP_CALL            ADDR(        0x06c485,          MAC_NOT_LOOKED_UP)
#define RVA_AI_NEAREST_TREQ_ON_PLANE           ADDR(        0x06cea0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_NEAREST_TREQ                    ADDR(        0x06ccf0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_BEST_SPAWNBOOSTER_AT            ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TURN_GATE_LATE          ADDR(        0x077478,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_SAVE_CAP                ADDR(        0x0774c1,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TURN_GATE_SHIPS         ADDR(        0x077521,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_FIRST_OFFER_TEST        ADDR(        0x0776e7,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_IMPORTANT_TRY_GATE      ADDR(        0x077aad,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_MELEE_TRY_GATE          ADDR(        0x077c2d,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TROOP_GOLD_GATE         ADDR(        0x077caf,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_COM_GOLD_GATE           ADDR(        0x077d14,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_COM_ESCORT_GATE         ADDR(        0x077d46,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_SITE_DRAWS              ADDR(        0x077f5c,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_ESCORT_STRENGTH         ADDR(        0x0780cb,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RECRUIT_TRIES                   ADDR(        0x078301,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PORT_FREE_BOAT_PLACES           ADDR(        0x07d7d0,          MAC_INLINED)
#define RVA_AI_COMMANDER_BOAT_PLACES           ADDR(        0x072cc0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SPEC_MESSAGES_OFF               ADDR(        0x07ab42,          MAC_NOT_LOOKED_UP)
#define RVA_AI_GATEWAY_CALL                    ADDR(        0x07acde,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ITEM_USE_RATING                 ADDR(        0x183e80,          MAC_NOT_LOOKED_UP)
#define MONSTER_ABILITY_AIHOLD                 0x34
#define MONSTER_ABILITY_AIPOWCOM1              0x66
#define MONSTER_ABILITY_AIMAXSHOP              0x90
#define MONSTER_ABILITY_SHIP                   0x1cf
#define RVA_AI_COUNT_PLANNERS                  ADDR(        0x079b50,          MAC_NOT_LOOKED_UP)
#define RVA_AI_FREE_SPAWNBOOSTERS              ADDR(        0x078fb0,          MAC_INLINED)
#define RVA_AI_BEST_BOOSTER_AT                 ADDR(        0x0792c0,          MAC_INLINED)
#define RVA_AI_MAKE_SPAWNBOOSTER               ADDR(        0x078390,          MAC_INLINED)
#define RVA_AI_TRADE_CHOICE                    ADDR(        0x0700f0,          MAC_NOT_LOOKED_UP)
#define RVA_TRADE_INCOME_APPLY                 ADDR(        0x149790,          MAC_NOT_LOOKED_UP)
#define RVA_AI_PLAN_SQUARE_BONUS               ADDR(        0x0771c0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_ITEM_VALUE                      ADDR(        0x073900,          MAC_NOT_LOOKED_UP)
#define RVA_AI_REDISTRIBUTE_ITEMS              ADDR(        0x0735a0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_SHIPS                           ADDR(        0x071750,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_AI_BAD                   ADDR(        0x06d070,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_TARGET                   ADDR(        0x06c4f0,          MAC_NOT_LOOKED_UP)
#define RVA_AI_RITUAL_BAD_TIME                 ADDR(        0x06d290,          MAC_NOT_LOOKED_UP)
#define RVA_AI_EXPLORE_TERRAIN_SKIP            ADDR(        0x075290,          MAC_NOT_LOOKED_UP)
#define RVA_AI_TARGET_IMPOSSIBLE               ADDR(        0x07cf40,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_ARMY_STRENGTH               ADDR(        0x0f9310,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STRENGTH                      ADDR(        0x0f9460,          MAC_INLINED)
#define RVA_UNIT_STRENGTH_CORE                 ADDR(        0x0bc3a0,          0x1003f6384)
#define RVA_UNIT_ARMOR                         ADDR(        0x3217c0,          0x10056ac1c)
#define RVA_SQUARE_ICEPROT_TEST                ADDR(        0x1b1c60,          0x1004a8d28)
#define RVA_PATH_STRENGTH_WEIGHT               ADDR(        0x0bd360,          MAC_NOT_LOOKED_UP)
#define RVA_INT_MIN                            ADDR(        0x0576b0,          MAC_NOT_LOOKED_UP)
#define RVA_INT_MAX                            ADDR(        0x05c890,          MAC_NOT_LOOKED_UP)
#define RVA_STRONGEST_PLAYER                   ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_STRONGEST_PLAYERS                  ADDR(        0x0f4c10,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TOP_UNIT                    ADDR_DIFFERS(0x1b2010,          0x100498ef0)
#define RVA_SQUARE_STRENGTH                    ADDR(        0x0f97c0,          0x1003f7130)
#define RVA_UNIT_STRENGTH_FOR                  ADDR(        0x0bc2b0,          MAC_INLINED)
#define RVA_UNIT_COUNTS_AS_MONSTER             ADDR(        0x1d9680,          0x100520f70)
#define RVA_SQUARE_SIDE_STRENGTH               ADDR(        0x0f9490,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_MAY_ACT                  ADDR(        0x1dfa60,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_RITPOW                   ADDR(        0x2c43d0,          MAC_NOT_FOUND)
#define RVA_COMMANDER_LED_COUNT                ADDR(        0x128d50,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_RANGED                    ADDR(        0x3229b0,          0x10056cb44)
#define RVA_MONSTER_HAS_RANGED                 ADDR(        0x327560,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_RETURNS                       ADDR(        0x326c30,          0x100569408)
#define RVA_SHOP_OFFER_AVAILABLE               ADDR(        0x128b30,          0x10047d3d8)
#define RVA_FIND_UPGRADE_UNIT                  ADDR(        0x12bf20,          0x100479288)
#define RVA_PLAYER_GOLD_INCOME                 ADDR(        0x335290,          0x1004cee58)
#define RVA_PLAYER_IRON_INCOME                 ADDR(        0x32da70,          0x1004cf274)
#define RVA_PLAYER_HERBS_INCOME                ADDR(        0x32dec0,          0x1004cfe3c)
#define RVA_PLAYER_FUNGUS_INCOME               ADDR(        0x32d5a0,          0x1004d0120)
#define RVA_PLAYER_SACRIFICES_INCOME           ADDR(        0x10abe0,          0x100476c74)
#define RVA_PLAYER_HANDS_INCOME                ADDR(        0x32e490,          0x1004cf97c)
#define RVA_PLAYER_WEED_INCOME                 ADDR(        0x32dd40,          0x1004d0418)
#define RVA_PLAYER_GEM_INCOME                  ADDR(        0x32db90,          0x1004cf6bc)
#define RVA_PLAYER_TRADE_INCOME                ADDR(        0x32eac0,          0x1004cf498)
#define RVA_PLAYER_RELICS_INCOME               ADDR(        0x32d950,          0x1004cfba4)
#define RVA_PLAYER_LIFEFORCE_INCOME            ADDR(        0x32ebe0,          0x1004d0bec)
#define RVA_PLAYER_GATHERS_RELICS              ADDR(        0x0b0a60,          0x1003723e4)
#define RVA_SQUARE_INCOME                      ADDR(        0x32f940,          0x1004cd400)
#define RVA_SQUARE_GOLD_INCOME                 ADDR(        0x335bb0,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_BASE_INCOME                ADDR(        0x3312c0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_LIFEFORCE_AT                ADDR(        0x335090,          MAC_NOT_LOOKED_UP)
#define RVA_LIFEFORCE_DRAIN_SQUARE             ADDR(        0x32e5b0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_INCOME_CACHE                 ADDR(        0x11c1bb2c,         0x10ccebe20)
#define RVA_FREE_TROOPS_STRENGTH               ADDR(        0x1291b0,          0x100474f08)
#define RVA_FREE_TROOPS_COUNT                  ADDR(        0x5822f54,         0x10857c554)
#define RVA_SQUARE_FORT_LEVEL                  ADDR(        0x1295c0,          MAC_INLINED)
#define RVA_SQUARE_HAS_FORT                    ADDR(        0x129bd0,          MAC_INLINED)
#define RVA_PLAYER_ABILITY                     ADDR(        0x0b0c80,          0x100362218)
#define RVA_SHOP_PROCESS_OFFER                 ADDR(        0x0aede0,          0x1003629f0)
#define RVA_AI_LEVEL_SHIFT_ALL                 ADDR(        0x2ea170,          MAC_NOT_LOOKED_UP)
#define RVA_SHARE_TEAM_SIGHT                   ADDR(        0x1afa30,          0x10049f4a4)
#define RVA_ILLUSION_TERRAIN                   ADDR(        0x1b3460,          0x100491634)
#define RVA_JOIN_COMMANDER                     ADDR(        0x129ad0,          0x1004840fc)
#define RVA_RESOURCE_STOCK                     ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_ORDER_BURN_FOREST                  ADDR(        0x2e1a70,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_CHOP_FOREST                  ADDR(        0x2e1bf0,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_SLAVE_HUNT                   ADDR(        0x2e3770,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_EAT_VILLAGE                  ADDR(        0x2e1de0,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST                       ADDR(        0x2e2290,          0x1003522f0)
#define RVA_PRIEST_COSTS                       ADDR(        0x2e3310,          0x100351bcc)
#define RVA_PRIEST_COSTS_CALL_AI               ADDR(        0x07b3cb,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS_CALL_MAP              ADDR(        0x09eac6,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_COSTS_CALL_NO_CALLER        ADDR(        0x09eac6,          MAC_NOT_LOOKED_UP)
#define RVA_PRIEST_ORDER_NO_CALLER             ADDR(        0x09e780,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_SCREEN           ADDR(        0x2d56c9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_CRUSADE          ADDR(        0x2d56b9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_MAP              ADDR(        WIN_NO_CALL,       MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALL_NO_CALLER        ADDR(        0x09ead9,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_PRIEST_CALLS_AI              ADDR(        0x07b3ed,          MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SWITCH_BOUND              ADDR_DIFFERS(0x2e265e,          0x100352424)
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
#define RVA_CEREMONY_COST_BOLSTER_STEP         ADDR(        0x11f97a4c,        0x106cee4ec)
#define RVA_CEREMONY_COST_SEAL_STEP            ADDR(        0x11f48f78,        0x106cee4e8)
#define RVA_CEREMONY_COST_FAVOR_STEP           ADDR(        0x11f493c0,        0x106cee4e4)
#define RVA_CEREMONY_COST_PONTIFF              ADDR(        0x11f97a48,        MAC_STORED_DIFFERENTLY)
#define RVA_CEREMONY_COST_CARDINAL             ADDR(        0x11f48ee0,        MAC_STORED_DIFFERENTLY)
#define RVA_CEREMONY_COST_ARCH_BISHOP          ADDR(        0x11f48ea4,        MAC_STORED_DIFFERENTLY)
#define RVA_CEREMONY_COST_ARCH_BISHOP_SET      ADDR(        WIN_STORED_DIFFERENTLY,0x106cee4d8)
#define RVA_CEREMONY_COST_CARDINAL_SET         ADDR(        WIN_STORED_DIFFERENTLY,0x106cee4dc)
#define RVA_CEREMONY_COST_PONTIFF_SET          ADDR(        WIN_STORED_DIFFERENTLY,0x106cee4e0)
#define RVA_MASTERY_COST_FILL_1                ADDR(        WIN_NO_EQUIVALENT,0x100351d10)
#define RVA_MASTERY_COST_FILL_2                ADDR(        WIN_NO_EQUIVALENT,0x100351d18)
#define RVA_MASTERY_COST_FILL_3                ADDR(        WIN_NO_EQUIVALENT,0x100351d20)
#define RVA_MASTERY_COST_FILL_4                ADDR(        WIN_NO_EQUIVALENT,0x10035e718)
#define RVA_MASTERY_COST_FILL_5                ADDR(        WIN_NO_EQUIVALENT,0x10035e720)
#define RVA_MASTERY_COST_FILL_6                ADDR(        WIN_NO_EQUIVALENT,0x10035e728)
#define RVA_MASTERY_COST_READ_1                ADDR(        WIN_NO_EQUIVALENT,0x1003527f0)
#define RVA_MASTERY_COST_READ_2                ADDR(        WIN_NO_EQUIVALENT,0x10035289c)
#define RVA_MASTERY_COST_READ_3                ADDR(        WIN_NO_EQUIVALENT,0x100352968)
#define RVA_MASTERY_COST_READ_4                ADDR(        WIN_NO_EQUIVALENT,0x10035ef58)
#define RVA_MASTERY_COST_READ_5                ADDR(        WIN_NO_EQUIVALENT,0x10035f024)
#define RVA_MASTERY_COST_READ_6                ADDR(        WIN_NO_EQUIVALENT,0x10035f058)
#define RVA_MASTERY_COST_READ_7                ADDR(        WIN_NO_EQUIVALENT,0x10035f0f4)
#define RVA_MASTERY_COST_READ_8                ADDR(        WIN_NO_EQUIVALENT,0x10035f19c)
#define RVA_MASTERY_COST_READ_9                ADDR(        WIN_NO_EQUIVALENT,0x10035283c)
#define RVA_MASTERY_COST_READ_10               ADDR(        WIN_NO_EQUIVALENT,0x100352904)
#define RVA_MASTERY_COST_READ_11               ADDR(        WIN_NO_EQUIVALENT,0x1003529d0)
#define MASTERY_COST_FILLS                     6
#define MASTERY_COST_READS                     11
#define CEREMONY_COST_ARCH_BISHOP_GOLD         100
#define CEREMONY_COST_CARDINAL_GOLD            250
#define CEREMONY_COST_PONTIFF_GOLD             600
#define RVA_CEREMONY_COST_INQUISITION          ADDR(        0x11f493bc,        0x106cee4d4)
#define RVA_CEREMONY_COST_CRUSADE              ADDR(        0x11f4981c,        0x106cee4d0)
#define RVA_CEREMONY_COST_BOLSTER_BASE         ADDR(        0x11d37a2c,        0x10857c550)
#define RVA_CEREMONY_COST_SEAL_BASE            ADDR(        0xfb31b00,         0x10857c54c)
#define RVA_CEREMONY_COST_CONVERT              ADDR(        0x11f493b8,        0x106cee4cc)
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
#define RVA_STR_CEREMONY_CONVERT               ADDR(        0x5698590,         0x10060949c)
#define RVA_STR_CEREMONY_BREAK_SEAL            ADDR(        0x5698630,         0x10060955b)
#define RVA_STR_CEREMONY_BOLSTER               ADDR(        0x56986a0,         0x1006095c2)
#define RVA_STR_CEREMONY_CRUSADE               ADDR(        0x5698708,         0x1006095e0)
#define RVA_STR_CEREMONY_INQUISITION           ADDR(        0x5698778,         0x10060962d)
#define RVA_STR_CEREMONY_MASTERY               ADDR(        0x5698798,         0x100609674)
#define RVA_STR_CEREMONY_FAVOR                 ADDR(        0x5698640,         0x100609583)
#define RVA_CEREMONY_SCREEN                    ADDR(        0x2d3a30,          MAC_INLINED)
#define RVA_RITUALS_OR_CEREMONIES              ADDR(        0x2e10e0,          MAC_NOT_LOOKED_UP)
#define RVA_CEREMONY_SCREEN_COSTS              ADDR_DIFFERS(0x2d3ea1,          0x10035e74c)
#define RVA_CEREMONY_SCREEN_COSTS_END          ADDR(        0x2d3ea6,          MAC_NOT_LOOKED_UP)
#define CEREMONY_SCREEN_COSTS_SIZE             0x5
#define RVA_FAIL_IF_FLAG                       ADDR(        0x1995a0,          MAC_NOT_LOOKED_UP)
#define RVA_SEALS_BROKEN                       ADDR(        0xf07b028,         0x11307d7fc)
#define RVA_SEAL_EVENTS_DONE                   ADDR(        0xf07b02c,         0x11307d800)
#define RVA_SEAL_EVENT_STEP                    ADDR(        0x165184,          0x1003aae30)
#define RVA_SEAL_EVENT_SWITCH                  ADDR_DIFFERS(0x1651a3,          0x1003aae48)
#define RVA_SEAL_EVENT_THIRD                   ADDR(        0x1651ac,          0x1003aae6c)
#define RVA_SEAL_EVENT_NOT_THIRD               ADDR(        0x16525d,          MAC_NOT_FOUND)
#define RVA_SEAL_EVENT_NONE                    ADDR_DIFFERS(0x165948,          0x1003ac520)
#define RVA_SEAL_EVENT_END                     ADDR(        0x16594f,          0x1003ac520)
#define RVA_APOCALYPSE_MONTHS                  ADDR(        0xf07b034,         0x11307d808)
#define RVA_PRIEST_FREESPAWN                   ADDR(        0x2dbb80,          MAC_NOT_LOOKED_UP)
#define RVA_BOLSTER_BLESSING                   ADDR(        0x2d5b10,          MAC_NOT_LOOKED_UP)
#define RVA_NET_SEND_ORDER                     ADDR(        0x1f23f0,          0x10050ed2c)
#define RVA_NET_SEND_BLESSINGS                 ADDR(        0x1e7440,          MAC_NOT_LOOKED_UP)
#define RVA_SEAL_BREAK_CEREMONY_CALL           ADDR(        0x2e27f2,          0x1003525c0)
#define RVA_SEAL_BREAK_ITEM_CALL               ADDR(        0x1851a2,          0x1004e829c)
#define RVA_USE_SPECIAL_SITE                   ADDR(        0x0b1b00,          MAC_NOT_LOOKED_UP)
#define RVA_DESTROY_GATEWAY                    ADDR(        0x255420,          MAC_NOT_LOOKED_UP)
#define RVA_ALTER_TERRAIN                      ADDR(        0x1f7a10,          MAC_NOT_LOOKED_UP)
#define RVA_SITE_VOID_INSANITY                 ADDR(        0x2df3f0,          MAC_NOT_LOOKED_UP)
#define RVA_REVEAL_DISC                        ADDR(        0x2dd920,          0x100354fb0)
#define RVA_RUIN_SQUARE                        ADDR(        0x2e0bf0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_BURN_VALUE                    ADDR(        0x2e07e0,          MAC_NOT_LOOKED_UP)
#define RVA_ORDER_CHOP_SELECTED_CALL           ADDR(        0x2e1c37,          MAC_NOT_LOOKED_UP)
#define RVA_MAP_INSPECTED_X                    ADDR(        0xfb31b04,         MAC_NOT_LOOKED_UP)
#define RVA_MAP_INSPECTED_Y                    ADDR(        0x11d37a34,        MAC_NOT_LOOKED_UP)
#define RVA_UNIT_REPICK_MEMORIZED              ADDR(        0x2e75a0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_MOVES                 ADDR(        0x11c08f20,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_FROM_X                ADDR(        0xfb2cca0,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_FROM_Y                ADDR(        0xfb36940,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_LAST_TARGET_X         ADDR(        0xfb89960,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AI_LAST_TARGET_Y         ADDR(        0xfb31b20,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TRADE_TYPE                  ADDR(        0x11c12b94,        0x10cce2e88)
#define RVA_PLAYER_TRADE_MODE                  ADDR(        0x11c12b96,        0x10cce2e8a)
#define RVA_COMMANDER_AT                       ADDR(        0x129440,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_HAS_SEEN_UNIT               ADDR(        0x1290e0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_USE_ITEMS                ADDR(        0x185750,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_AI_SKIP                       ADDR(        0x1d9e20,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_POINTLESS                   ADDR(        0x2cbe00,          0x10055c75c)
#define RVA_RITUAL_CASTABLE_ANYWHERE           ADDR(        0x2cbbe0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_CASTER_BONUS             ADDR(        0x128b00,          0x10047e954)
#define RVA_SEA_AREA_SIZE                      ADDR(        0x2ae1c0,          0x1004acd3c)
#define RVA_AI_PROGRESS_MODE                   ADDR(        0x43c8c4,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_REFUSE_PLAYER                ADDR(        0x43c394,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_REFUSE_CLASS                 ADDR(        0x43c2dc,          MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_MAX_SEAS                     ADDR(        0x58355f4,         MAC_NOT_LOOKED_UP)
#define RVA_ROUTE_MAX_SKIES                    ADDR(        0x58355f0,         MAC_NOT_LOOKED_UP)
#define RVA_MOVE_COMMANDER_STEP                ADDR(        0x1dfbb0,          0x10059a4bc)
#define RVA_CLEAR_SQUARE_MARKS                 ADDR(        0x1b3a60,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_RESISTANCE_1D1280             ADDR(        0x32a1e0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_FIRE_RESISTANCE               RVA_UNIT_RESISTANCE_1D1280
#define RVA_UNIT_ACID_RESISTANCE               ADDR(        0x320cb0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_COLD_RESISTANCE               ADDR(        0x32a500,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SHOCK_RESISTANCE              ADDR(        0x329e10,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_POISON_RESISTANCE             ADDR(        0x326910,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_STATE_FLAGS                   ADDR(        0x32a8d0,          0x100567408)
#define RVA_UNIT_AFFLICTION_LIST               ADDR(        0x1d4150,          MAC_NOT_LOOKED_UP)
#define UNIT_FEEBLEMINDED                      (1ULL << 25)
#define RVA_UNIT_AFFLICTION_LINE               ADDR(        0x1d14b0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_AP_LEFT                  ADDR(        0x3291a0,          MAC_NOT_LOOKED_UP)
#define RVA_RESOURCE_INCOME                    ADDR(        0x329bd0,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FX_FIRST                    ADDR(        0x2c5c80,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FX_FIRST_OR                 ADDR(        0x2cbdf0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_UNITS_NAMED                  ADDR(        0x129c90,          0x10047d120)
#define RVA_COUNT_MONSTER_AT                   ADDR(        0x12ca20,          0x10047ea18)
#define RVA_SQUARE_ABILITY_TOTAL               ADDR(        0x1b3ac0,          0x1004a94d0)
#define RVA_RELEASE_FOLLOWERS                  ADDR(        0x1292d0,          MAC_NOT_LOOKED_UP)
#define RVA_CHECK_FOLLOWER_LINKS               ADDR(        0x111420,          0x1004679dc)
#define RVA_RITUAL_LINK_CHECK_CALL             ADDR(        0x2c9e9d,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_UNFOLLOW_TARGET             ADDR(        0x2c9026,          MAC_NOT_LOOKED_UP)
#define RVA_TELEPORT_ARMY                      ADDR(        0x2dc650,          0x100358c50)
#define RVA_TELEPORT_SQUARE                    ADDR(        0x2dcb30,          MAC_INLINED)
#define RVA_TELEPORT_UNIT                      ADDR(        0x2dcd20,          MAC_INLINED)
#define RVA_TELEPORT_COMMANDER                 ADDR(        0x2dcd50,          0x10035910c)
#define RVA_NET_SEND_MOVED_UNITS               ADDR(        0x1f1130,          0x10050c598)
#define RVA_STEP_BURN_FOREST                   ADDR(        0x1072f0,          MAC_INLINED)
#define RVA_AI_SPLIT_LEADER_UNLINK             ADDR(        0x07bd3d,          MAC_NOT_LOOKED_UP)
#define RVA_TRANSFER_UNIT_TO_COMMANDER         ADDR(        0x111900,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MAY_JOIN_COMMANDER            ADDR(        0x1119e0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MAY_LEAVE_LEADER              ADDR(        0x111ab0,          MAC_INLINED)
#define RVA_UNIT_SPEED_CLASS                   ADDR(        0x325de0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_COMMANDS_MENU                 ADDR(        0x111e30,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_COMMANDS_MENU_CALL            ADDR(        0x121c7e,          MAC_NOT_LOOKED_UP)
#define RVA_TRANSFER_UNITS_IN_RECT             ADDR(        0x1115c0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_GRID_UNIT_AT                  ADDR(        0x0e64d0,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_UNIT_GRID_CELL                ADDR(        0x0e6230,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_TRANSFER_CALL           ADDR(        0x122221,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_FOLLOWERS_CALL          ADDR(        0x122251,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_NEW_ID                  ADDR(        0x122280,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_CLICK_RELEASE                 ADDR(        0x1222ae,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_RECT_RELEASE                  ADDR(        0x111859,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_TAKE_ALL_CALL                 ADDR(        0x120e53,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_DROP_STATIONARY               ADDR(        0x122421,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_SELF_LINK_CLEAR               ADDR(        0x122455,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_EXIT_FLUSH_ALL                ADDR(        0x122466,          MAC_NOT_LOOKED_UP)
#define RVA_LED_COUNT_CACHE_READY              ADDR(        0x5822edc,         MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_PRICE                    ADDR(        0x0b1300,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_FREE_COMMANDERS              ADDR(        0x129a30,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_SPELL_ATTACK              ADDR(        0x1d9da0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_HAS_SPELL_WEAPON              ADDR(        0x329cf0,          0x10056c5e8)
#define RVA_UNIT_HAS_WATER_MOVE                ADDR(        0x32a160,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_FLIGHT_KIND                   ADDR(        0x32a810,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_PLANE                         ADDR(        0x32a1b0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_SIZE                   ADDR(        0x322140,          0x10056bd20)
#define RVA_UNIT_BATTLE_SIZE_2                 ADDR(        0x325a40,          0x10056bc6c)
#define RVA_MONSTER_BATTLE_SIZE                ADDR(        0x31f0f0,          MAC_INLINED)
#define RVA_MONSTER_SPRITE_SIZE                ADDR(        0x0f9db0,          0x10057d488)
#define RVA_UNIT_BOAT_SIZE                     ADDR(        0x3275c0,          0x10056a414)
#define RVA_UNIT_AFFLICTION_COUNT              ADDR(        0x0f7190,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_REMOVE_ITEM                   ADDR(        0x1856b0,          MAC_NOT_LOOKED_UP)
#define RVA_ITEM_ABILITY_NTH                   ADDR(        0x17e950,          0x1004e4aac)
#define RVA_ITEM_ABILITY                       ADDR(        0x185740,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_FLAG_21_TEST               ADDR(        0x31d280,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEXT                        ADDR(        0x1b3960,          0x1004a9160)
#define RVA_GAME_GLOBALS_INIT                  ADDR(        0x116b40,          MAC_NOT_LOOKED_UP)
#define RVA_GATEWAY_CHECK                      ADDR(        0x255840,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_TURN_END                    ADDR(        0x1e1e30,          0x1004c53c8)
#define RVA_MOVE_WANDERING_MONSTERS            ADDR(        0x17b2d0,          MAC_INLINED)
#define RVA_MOVE_PLAYER_LONERS                 ADDR(        0x167820,          MAC_INLINED)
#define RVA_MOVE_PLAYER_STUPIDS                ADDR(        0x167940,          MAC_INLINED)
#define RVA_DO_ALL_FIGHTS                      ADDR(        0x0f5200,          0x1003dc1a0)
#define RVA_BATTLE_SAVE_OTHERS                 ADDR(        0x18e890,          MAC_INLINED)
#define RVA_BATTLE_LOAD_OTHERS                 ADDR(        0x18e650,          0x1004f4c7c)
#define RVA_BATTLE_TEMP_FOLDER                 ADDR(        0x198ed0,          MAC_INLINED)
#define RVA_RETURN_IMMORTALS                   ADDR(        0x123df0,          0x100469f20)
#define RVA_MOVE_INFERNO_PLAYER                ADDR(        0x17a0b0,          MAC_INLINED)
#define RVA_MOVE_HADES_PLAYER                  ADDR(        0x178b60,          MAC_INLINED)
#define RVA_MOVE_CELESTIAL_PLAYER              ADDR(        0x17d970,          MAC_INLINED)
#define RVA_MOVE_ALL_INDEPENDENTS              ADDR(        0x17af90,          0x100547dfc)
#define RVA_MOVE_INDEPENDENT_ARMY              ADDR(        0x172dd0,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_MOVED                        ADDR(        0x11f49840,        0x10c5e86c4)
#define RVA_INDEP_LOVED_TERRAIN                ADDR(        0x16e7f0,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_LIKED_TERRAIN                ADDR(        0x16e3b0,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_NEAR_PREFERRED               ADDR(        0x16e310,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_HATED_TERRAIN                ADDR(        0x16e180,          MAC_NOT_LOOKED_UP)
#define RVA_INDEP_MOVE_TRY                     ADDR(        0x16d4b0,          0x100546c24)
#define RVA_INDEP_MOVE_STEP                    ADDR(        0x16db90,          0x100546458)
#define RVA_SET_HOLDIT                         ADDR(        0x173600,          MAC_NOT_LOOKED_UP)
#define RVA_GOLD_MINE_ATTACK                   ADDR(        0x26cf50,          MAC_NOT_LOOKED_UP)
#define RVA_NEAREST_UNIT_ABILITY_IS            ADDR(        0x1ae9a0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_SWIMS                         ADDR(        0x105050,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_NOLAND                        ADDR(        0x105030,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MINDLESS                      ADDR(        0x1d9ee0,          0x10051fc24)
#define RVA_SQUARE_SOUTH                       ADDR(        0x2ada10,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_NORTH                       ADDR(        0x2ad9c0,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_WATER_NEAR                   ADDR(        0x32d480,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_LAND_NEAR                    ADDR(        0x32d360,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_RUIN_FORM                  ADDR(        0x32e9e0,          MAC_NOT_LOOKED_UP)
#define SQUARE_FLAG_HOLDIT                     0x10
#define SQUARE_FLAG_ILLUSION                   0x4
#define SQUARE_FLAG_NO_SIGHT                   0x20000
#define UNIT_FLAG2_HOLDIT                      41
#define RVA_SQUARE_ARMY_STATIONARY             ADDR(        0x112940,          MAC_NOT_FOUND)
#define RVA_WANDERER_PICK_TARGET               ADDR(        0x16a270,          0x1005483b0)
#define RVA_WANDERER_AT                         ADDR(        0x177d20,           0x100547c84)
#define RVA_WANDERER_PICK_UP                   ADDR(        0x147cf0,          MAC_NOT_LOOKED_UP)
#define RVA_LONER_STEP                         ADDR(        0x173680,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_UNITS_FLAGGED_AT             ADDR(        0x1182e0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_ALL_STRENGTH                ADDR(        0x0bc290,          MAC_NOT_LOOKED_UP)
#define RVA_REMOVE_COMMANDER_ARMY              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_IMMORTAL_REFORM_PLACE              ADDR(        0x3275f0,          0x10056845c)
#define RVA_NEW_COMMANDER_ID                   ADDR(        0x1197c0,          MAC_NOT_LOOKED_UP)
#define RVA_INFERNO_ACTIVE                      ADDR(        0x5822f94,          0x10c5e86b0)
#define RVA_CELESTIAL_ACTIVE                    ADDR(        0x5822f98,          0x10c5e86b4)
#define RVA_PRIMAL_PORTALS_OPEN                 ADDR(        0x5822f90,          0x10c5e86b8)
#define RVA_GAME_STATISTICS                    ADDR(        0x31d880,          0x10053bce4)
#define RVA_AFTER_GAME_TURN                    ADDR(        0x161e60,          0x1003a08b8)
#define RVA_MONTH_SQUARE_RESET                 ADDR(        0x140c60,          MAC_NOT_LOOKED_UP)
#define RVA_MONTH_SQUARE_RESET_CALL            ADDR(        0x166d5d,          MAC_NOT_LOOKED_UP)
#define RVA_MAGIC_TAINTED_MONTHS               ADDR(        0x5822f80,         0x10ce02edc)
#define RVA_DROWN_CHECK                        ADDR(        0x114940,          MAC_INLINED)
#define RVA_CRASH_LANDING_CHECK                ADDR(        0x114300,          MAC_INLINED)
#define RVA_TURN_UNIT_UPKEEP                   ADDR(        0x144150,          0x100399c30)
#define RVA_SQUARE_UPKEEP                      ADDR(        0x1ddb50,          0x1005991a0)
#define RVA_BATTLE_SETUP                       ADDR(        0x0f7350,          0x1003cf664)
#define RVA_BATTLE_SETUP_CALL                  ADDR(        0x0f5e65,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_BG_IMAGE_CALL               ADDR(        0x0fc0ab,          0x100581450)
#define RVA_BATTLE_SEED_TRACE_CALL             ADDR(        0x0f7535,          0x1003cf854)
#define RVA_BATTLE_SIZE_TRACE_CALL             ADDR(        0x0f8033,          0x1003cfff0)
#define RVA_BATTLE_TREES_TRACE_CALL            ADDR(        0x0eed28,          0x1003d1424)
#define RVA_BATTLE_FORT_TRACE_CALL             ADDR(        0x0f8a4f,          0x1003d1be4)
#define RVA_BATTLE_EDITOR_SAVE_OPEN_CALL       ADDR(        0x0861c8,          0x10051a7ec)
#define RVA_BATTLE_EDITOR_SAVE_CLOSE_CALL      ADDR(        0x088177,          0x10051b220)
#define RVA_BATTLE_EDITOR_BROWSE_CALL          ADDR(        0x085ece,          0x100519e94)
#define RVA_BATTLE_EDITOR_LOAD_CALL            ADDR_DIFFERS(0x085f35,          0x10051acec)
#define RVA_BATTLE_EDITOR_ACTIVE               ADDR_DIFFERS(0x58033b4,         0x10c922c60)
#define RVA_LOAD_BATMAP                        ADDR_DIFFERS(0x0873b0,          0x100518024)
#define RVA_SAVE_BATMAP                        ADDR(        0x087e50,          MAC_INLINED)
#define RVA_BATTLE_CREATE_TREES                ADDR(        0x0eec30,          MAC_INLINED)
#define RVA_BATTLE_PLACE_TERRAIN_UNIT          ADDR_DIFFERS(0x0f0de0,          0x1003c7e44)
#define RVA_BATTLE_BACKGROUND                  ADDR(        0x0fbe90,          0x100580be8)
#define RVA_SQUARE_DEFAULT_GROUND              ADDR(        0x0b5370,          0x100550bfc)
#define RVA_BATTLE_DEPLOY                      ADDR(        0x0f8ae0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DEPLOY_CALL                 ADDR(        0x0f5e71,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SETUP_SIDE                  ADDR(        0x0e6510,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SETUP_AMBUSH                ADDR(        0x0e9510,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_BEST_DEPLOY_SQUARE          ADDR(        0x0eae10,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_COLUMN_FREE_ROW             ADDR(        0x0ea710,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_INTRO_SCREEN                ADDR(        0x0e1f20,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SHOWN_TO_SLOT               ADDR(        0x32b490,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_SHOWN_STORE                  ADDR(        0x0f5d43,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_SHOWN_STORE_NEXT             ADDR(        0x0f5d48,          MAC_NOT_LOOKED_UP)
#define DO_ALL_FIGHTS_ROLES                    0x40
#define DO_ALL_FIGHTS_SHOWN                    0x58
#define RVA_FIGHT_ADVANCE_ATT_CALL             ADDR(        0x0f33f1,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_PLAN_CALL                    ADDR(        0x0f359f,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_DOATTACK_CALL                ADDR(        0x0f3ab7,          MAC_NOT_LOOKED_UP)
#define RVA_FIGHT_WINNER_CALL                  ADDR(        0x0f3cc1,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ADVANCE_UNIT                ADDR(        0x0c7270,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FIGHT_MOVE                  ADDR(        0x0cbb10,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_KILLER_MOVE                 ADDR(        0x0cb750,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ADVANCE_RANK                ADDR(        0x0cb5a0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FLY_MOVE                    ADDR(        0x0cad60,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PICK_FIXED_TARGET           ADDR(        0x0db620,          MAC_NOT_LOOKED_UP)
#define RVA_ADVANCE_FIXED_TARGET_CALL          ADDR(        0x0c891d,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PATHFIND                    ADDR(        0x0b8180,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_NEAREST_ENEMY               ADDR(        0x0ce310,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_NEAREST_ENEMY_RING          ADDR(        0x0ce510,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_DISTANCE               ADDR(        0x0dbda0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_CAN_STAND                   ADDR(        0x0ec730,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_IN_CONTACT                    ADDR(        0x0c98a0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_LINE_OF_SIGHT               ADDR(        0x0dbb40,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ENEMY_EXTENT                ADDR(        0x0dc230,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_COUNT_RANK                  ADDR(        0x0dc400,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_WINNER                      ADDR(        0x0cc9a0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_RANDOM_TARGET               ADDR(        0x0b9e00,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_AOE_SCORE                   ADDR(        0x0ba510,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_TARGET_USEFUL               ADDR(        0x0b8df0,          0x1003cc62c)
#define BATTLE_TARGET_NOT_AFFECTED             -3
#define RVA_WEAPON_RANGE_FOR                   ADDR(        0x31f1c0,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_ATTACK_ROW                    ADDR(        0x0d7c30,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_RANK                   ADDR(        0x323d80,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_SLOWNESS               ADDR(        0x325990,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_BATTLE_FAST                   ADDR(        0x322930,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_INCAPACITATED                 ADDR(        0x0f1540,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_MORALE                        ADDR(        0x322b20,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_LOCAL_LEADERSHIP            ADDR(        0x0e51a0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FEAR_AT                     ADDR(        0x0e4bf0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_FLEE                        ADDR(        0x0e4ba0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_REMOVE_FROM_FIELD           ADDR(        0x0e6050,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_AT                     ADDR(        0x0ee410,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_AT_2                   ADDR(        0x0ee260,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_UNIT_POSITION               ADDR(        0x0e6330,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_PLACE_UNIT                  ADDR(        0x0e5e50,          0x1003cd8ec)
#define RVA_BATTLE_AFTERMATH_RETURN            ADDR(        0x0e2740,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_AFTER_FIGHT_UPKEEP          ADDR(        0x0cf6b0,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DELAY_FOR_SPEED             ADDR(        0x0ffe90,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_SIDE_UNITS                   ADDR(        0x117570,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ROUND                       ADDR(        0x11f41958,        0x106cee4c8)
#define RVA_BATTLE_DELAY                       ADDR(        0xfb2cc84,         0x10cb0e188)
#define RVA_BATTLE_WIDTH                       ADDR(        0x43e084,          0x1048937f0)
#define RVA_BATTLE_HEIGHT                      ADDR(        WIN_IN_CODE,       MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_WINNER_PLAYER               ADDR(        0x43e1f4,          0x100872b5c)
#define RVA_BATTLE_STARVE_ROUND                ADDR(        0x43dd84,          0x100872b08)
#define RVA_BATTLE_CHARGE_ROUND                ADDR(        0x43dd6c,          0x100872b0c)
#define RVA_BATTLE_IS_AMBUSH                   ADDR_DIFFERS(0x5804c08,         0x106cd5de4)
#define RVA_BATTLE_AT_SEA                      ADDR_DIFFERS(0x5804c14,         0x107ee6490)
#define RVA_BATTLE_PLANE_3_OR_4                ADDR(        0x5804c24,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_IS_SIEGE                    ADDR_DIFFERS(0x5804c54,         0x10c921d10)
#define RVA_BATTLE_OVER                        ADDR_DIFFERS(0x5804c1c,         0x10c96df3c)
#define RVA_BATTLE_NOT_A_SIEGE                 ADDR_DIFFERS(0x5804c4c,         0x1069b16b4)
#define RVA_BATTLE_ATT_RANK_COUNTS             ADDR(        0x64ab8c0,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_DEF_RANK_COUNTS             ADDR(        0x64ab8d0,         MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_ENEMY_EXTENTS               ADDR(        0x123358e0,        MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_GRID                        ADDR(        0x11fb18c0,        0x10ca5a188)
#define RVA_BATTLE_SPEED_SETTING               ADDR(        0x1a3b3cc,         MAC_NOT_LOOKED_UP)
#define UNIT_BATTLE_SIDE                       0x110
#define UNIT_ATTACKS_MADE                      0x111
#define UNIT_ATTACK_PLAN                       0x112
#define UNIT_ZAP_ITEM                          0x122
#define UNIT_FIXED_TARGET                      0x150
#define RVA_GAME_OVER                          ADDR(        0x11fee0,          MAC_INLINED)
#define RVA_GAME_OVER_SCREEN                   ADDR(        0x10d3a0,          0x100473b14)
#define RVA_GAME_OVER_EFFECT                   ADDR_DIFFERS(0x2d03a0,          0x10051c64c)
#define RVA_START_TUNE                         ADDR(        0x122c70,          0x100474c5c)
#define RVA_SAVE_GAME                          ADDR_DIFFERS(0x18eb50,          0x1004f1fd0)
#define RVA_AUTOSAVE                           ADDR(        0x0943c0,          MAC_INLINED)
#define RVA_SAVE_DIALOG                        ADDR(        0x18b6a0,          0x1004f4ac4)
#define RVA_LOAD_GAME_SCREEN                   ADDR(        0x18b250,          MAC_INLINED)
#define RVA_STR_LOAD_GAME_TITLE                ADDR(        0x1c58b60,         0x1006fe779)
#define RVA_LOAD_GAME_BROWSER_SKIPS            ADDR_DIFFERS(0x1a3b0f4,         0x100837fc0)
#define RVA_LOAD_GAME_BROWSER_NAME             ADDR_DIFFERS(0x5822fac,         0x1005cc6f3)
#define RVA_STR_LOADED_START_MM                ADDR(        0x1c5c058,         0x10060603b)
#define RVA_LOAD_START_TEST                    ADDR(        0x58356c8,         0x11307d814)
#define RVA_LOAD_START_MARK                    ADDR_DIFFERS(0x58356fc,         0x106cee4f0)
#define RVA_PLAYER_ELIMINATED                  ADDR(        0x111170,          0x100471ccc)
#define RVA_STR_ELIMINATED                     ADDR(        0x1b297a0,         0x1006319ec)
#define RVA_ELIMINATED_WARN_CALL               ADDR(        0x1112f2,          0x100472b14)
#define RVA_STR_PERCENT_S_ELIMINATED           ADDR(        0x440734,          0x1005b6f9d)
#define RVA_WARN_WAIT_CALL                     ADDR(        0x043166,          0x1003485c0)
#define RVA_WARN_TIMED_WAIT_CALL               ADDR(        0x043186,          0x100348564)
#define RVA_WARN_CLOSE_MS                      ADDR(        0x57ef24c,         MAC_NOT_FOUND)
#define RVA_MODS_SCREEN                        ADDR(        0x1ca220,          MAC_INLINED)
#define RVA_KEYBOARD_SHORTCUTS                 ADDR(        0x12ccd0,          0x10051238c)
#define RVA_ROLL_CLASS                         ADDR(        0x0afca0,          MAC_INLINED)
#define RVA_AUTO_NAME                          ADDR(        0x125170,          0x1004714c8)
#define RVA_AUTO_NAME_CALL_COSTLIST            ADDR(        0x0afba6,          0x100362514)
#define RVA_AUTO_NAME_CALL_PLAYER              ADDR(        0x0b0785,          0x100361408)
#define RVA_AUTO_NAME_CALL_HELPER              ADDR(        0x10f105,          0x100471ba8)
#define RVA_AUTO_NAME_CALL_NEW_COMMANDER       ADDR(        0x128a69,          0x10046dfb8)
#define RVA_AUTO_NAME_CALL_NET_SETUP           ADDR(        0x1ee6f2,          0x100504388)
#define RVA_AUTO_NAME_CALL_SETUP               ADDR(        0x305234,          0x100589adc)
#define RVA_AUTO_NAME_CALL_SERVER              ADDR(        WIN_NO_CALL,       0x1004fa44c)
#define RVA_MONSTER_NAME_TYPE                  ADDR(        0x11f7f0,          0x100470e64)
#define RVA_AUTO_NAME_LAST                     ADDR(        0x660efd0,         0x106cd54dc)
#define RVA_AUTO_NAME_COUNTED                  ADDR(        0x5822ed4,         0x106cd54d8)
#define AUTO_NAME_TYPES                        62
#define AUTO_NAME_LIST_SIZE                    0x5dc
#define RVA_PLACE_PLAYER                       ADDR(        0x0a8930,          0x10036dfcc)
#define RVA_ROLL_SUB_SOCIETY                   ADDR(        0x2d0710,          0x100512768)
#define RVA_MONTHLY_UPDATE                     ADDR(        0x198f70,          0x100333a1c)
#define RVA_SQUARE_INFO                        ADDR(        0x331c50,          0x1004c9b00)
#define RVA_SQUARE_INFO_ROW_START              ADDR_DIFFERS(0x334a8c,          0x1004cd1fc)
#define RVA_SQUARE_PANEL                       ADDR(        0x0a3960,          MAC_NOT_FOUND)
#define RVA_LOBBY                              ADDR(        0x1e3610,          0x10059f008)
#define RVA_DISCONNECT                         ADDR(        0x1f2bd0,          0x100507180)
#define RVA_SDL_MAIN                           ADDR(        0x18ffa0,          0x10032f1fc)
#define RVA_WIN_MAIN                           ADDR(        0x001180,          MAC_NOT_FOUND)
#define RVA_WIN_MAIN_SET_MAIN_READY_CALL       ADDR(        0x0012b5,          MAC_NO_CALL)
#define RVA_WIN_MAIN_SDL_MAIN_CALL             ADDR(        0x0012bf,          MAC_NO_CALL)
#define RVA_SET_MAIN_READY_THUNK               ADDR(        0x02434d,          MAC_NO_CALL)
#define RVA_SET_MAIN_READY_IMPORT              ADDR(        0x33a4e0,          MAC_NO_CALL)
#define RVA_STEAM_DLOPEN                       ADDR(        WIN_INLINED,       0x10000fed0)
#define RVA_STEAM_DLSYM                        ADDR(        WIN_INLINED,       0x10000fedc)
#define RVA_GAME_MENU_JUMPS                    ADDR(        WIN_NO_EQUIVALENT, 0x1007588f8)
#define RVA_RESET_MODS                         ADDR(        0x1cacb0,          0x100528d1c)
#define RVA_BEFORE_GAME_START                  ADDR(        0x0944e0,          0x100389574)
#define RVA_TOP_BAR                            ADDR(        0x09c4a0,          0x100377564)
#define RVA_DATE                               ADDR(        0x119050,          MAC_INLINED)
#define RVA_UNIT_SHOWN_NAME                    ADDR(        0x0f8d10,          0x1003bb990)
#define RVA_COMMANDER_THE_NAME                 ADDR(        0x0f93b0,          MAC_INLINED)
#define RVA_COMMANDER_THE_NAME_INDEX           ADDR(       0x5804c40,          0x1069b1eb0)
#define RVA_COMMANDER_THE_NAME_RING            ADDR(       0x64ebae0,          0x1069b1eb4)
#define RVA_RECRUIT_SCREEN                     ADDR(        0x125530,          0x100479c04)
#define RVA_RECRUIT_LOCATION_LAST              ADDR(        0x441914,          MAC_NOT_LOOKED_UP)
#define RVA_RECRUIT_LOCATION_X                 ADDR(        0x11f48f80,        MAC_NOT_LOOKED_UP)
#define RVA_RECRUIT_LOCATION_Y                 ADDR(        0x11f49400,        MAC_NOT_LOOKED_UP)
#define FRAME_MESSAGE_BOX_TEXT                 FRAME_OFFSET(0x000020,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_PLAYER            FRAME_OFFSET(0x000098,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_X                 FRAME_OFFSET(0x000094,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_Y                 FRAME_OFFSET(0x000090,          MAC_NOT_LOOKED_UP)
#define FRAME_RECRUIT_SCREEN_CLICK_LATCH       FRAME_OFFSET(0x0000d0,          MAC_NOT_LOOKED_UP)
#define RVA_PLANE_SCREEN                       ADDR(        0x1b0200,          0x1004a6778)
#define RVA_UNIT_STATS_SCREEN                  ADDR(        0x1d4990,          0x10052113c)
#define RVA_ABILITY_LIST                       ADDR(        0x1d14b0,          0x10051c860)
#define RVA_ABILITY_LOOKUP                     ADDR(        0x0b71f0,          0x1004ea61c)
#define RVA_ADDPLANE                           ADDR(        0x250bc0,          0x10056e058)
#define RVA_PLAYER_COLOUR_RPC                  ADDR(        0x3023c0,          0x100586e4c)
#define RVA_DIFFICULTY_POPUP                   ADDR(        0x302f30,          0x1005866d8)
#define RVA_SAVE_LOBBY_GAME                    ADDR(        0x12eac0,          0x1004f832c)
#define RVA_SERVER_SCREEN                      ADDR(        0x1f3830,          0x1004f8e18)
#define RVA_SETUP_SERVER                       ADDR(        0x1f1520,          0x1005023a8)
#define RVA_NET_CLIENT_GAME                    ADDR(        0x1e91d0,          0x1005072bc)
#define RVA_NET_MODVAR                         ADDR(        0x1eaf00,          MAC_INLINED)
#define RVA_NEW_WORLD_NET_AFTER                ADDR(        0x1ad8a0,          MAC_INLINED)
#define RVA_OPENDIR                            ADDR(        0x05c230,          0x1005aebcc)

#define RVA_NEW_WORLD_RESET_CALL               ADDR(        0x303427,          0x1005872c8)
#define RVA_NEW_WORLD_UNIQUE_INIT              ADDR(        0x30342c,          0x1005872d4)
#define RVA_PARTICIPANTS_RANDOM_COLOURS        ADDR(        0x304d4e,          0x100587b80)
#define RVA_PARTICIPANTS_COLOUR_BUTTON         ADDR(        0x305024,          0x1005892bc)
#define RVA_NEW_WORLD_NET_INIT_CALL            ADDR(        0x303532,          MAC_NO_CALL)
#define RVA_NEW_WORLD_INIT_CALL                ADDR(        0x30383d,          MAC_NO_CALL)
#define RVA_TWO_PLAYER_TEAMS                   ADDR_DIFFERS(0x5826bb4,       0x10ce0cb4c)
#define RVA_INIT_PARTICIPANTS_COPY             ADDR(        WIN_NO_EQUIVALENT,0x100587684)
#define RVA_INIT_PARTICIPANTS_JOIN             ADDR(        WIN_NO_EQUIVALENT,0x100587a44)
#define RVA_INIT_PARTICIPANTS_CONTROLLERS      ADDR(        WIN_IN_CODE,0x100758250)
#define RVA_PARTICIPANTS_ROW_START             ADDR_DIFFERS(0x3042a4,          0x1005889e8)
#define RVA_PARTICIPANTS_ROW_START_NET         ADDR_DIFFERS(0x3042a4,          0x10058848c)
#define RVA_PARTICIPANTS_DIFFICULTY_CALL       ADDR(        0x304e60,          0x1005891bc)
#define RVA_NEW_WORLD_UNIQUE_CHECKBOX          ADDR_DIFFERS(0x30428e,          0x1005889c8)
#define RVA_NEW_WORLD_CREATING_WORLD_JOIN      ADDR(        0x305058,          0x1005893f0)
#define RVA_NEW_WORLD_CLASS_CHECK              ADDR(        0x30508c,          0x100589550)
#define RVA_NEW_WORLD_UNIQUE_READ              ADDR(        0x305070,          0x100589410)
#define RVA_NEW_WORLD_NAME_CHECK               ADDR(        0x305128,          0x100589a08)
#define RVA_NEW_WORLD_NAME_BUFFER              ADDR(        0x305152,          0x100589a04)
#define RVA_NEW_WORLD_NAME_BOX_CALL            ADDR(        0x305182,          0x100589a2c)
#define RVA_NEW_WORLD_PLACE_PLAYER_CALL        ADDR(        0x305416,          0x10058a080)
#define RVA_NEW_WORLD_CREATING_WORLD_2         ADDR(        WIN_NO_EQUIVALENT, 0x10058a758)
#define FRAME_NEW_WORLD_UNIQUE_RANDOM          FRAME_OFFSET(0x000064,          0x228)

#define RVA_GAME_START_CALL                    ADDR(        0x09209f,          0x10038a290)
#define RVA_MONTH_START_CALL                   ADDR(        0x09227c,          0x10038a5c0)
#define RVA_TURN_START_AI_CALL                 ADDR(        0x09242f,          0x10038abbc)
#define RVA_TURN_START_AI_SINGLE_CALL          ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_TURN_START_INDEPENDENT_CALL        ADDR(        0x0926c6,          0x10038a990)
#define RVA_TURN_START_HUMAN_CALL              ADDR(        0x0927de,          0x10038a808)
#define RVA_TURN_START_HUMAN_SINGLE_CALL       ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_AI_TURN_CALL                       ADDR(        0x092461,          0x10038abf8)
#define RVA_HUMAN_TURN_CALL                    ADDR(        0x0929cf,          0x10038b050)
#define RVA_TURN_HANDOFF_BOX                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_TURN_HANDOFF_BOX_CALL              ADDR_DIFFERS(0x092903,          0x10038af9c)
#define RVA_STR_TURN_HANDOFF                   ADDR(        0x1b29bb0,         0x100631d41)
#define RVA_LONE_HUMAN                         ADDR(        0xf07b020,         0x11307d7f4)
#define RVA_PLAYER_TURN_END_CALL               ADDR(        0x0924e2,          0x10038bfac)
#define RVA_GAME_OVER_CALL                     ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_WORLD_SIZE_LOG                     ADDR(        0x092f69,          0x1003902d0)
#define RVA_GAME_OVER_SCREEN_CALL              ADDR(        0x09403c,          0x1003900a8)
#define RVA_GAME_OVER_WAIT_CALL                ADDR(        0x10d803,          0x1004742c4)
#define RVA_LOAD_TURN_END_CALL                 ADDR(        0x186e7e,          0x1004ed4fc)
#define RVA_EXIT_DISCONNECT_CALL               ADDR(        0x1994de,          0x10032b598)
#define RVA_RESET_EVENT_VARS                   ADDR(        0x31d180,          0x1005861e0)
#define RVA_MAP_GAME_MENU_CALL                 ADDR(        0x09775b,          MAC_NO_CALL)
#define RVA_MAP_SQUARE_INFO_CALL               ADDR(        0x097c23,          0x10038430c)
#define RVA_MAP_SQUARE_PANEL_CALL              ADDR(        0x0a0690,          MAC_NOT_FOUND)
#define RVA_GAME_MENU_REDRAW                   ADDR(        0x1245f2,          0x100380458)
#define RVA_GAME_MENU_MAP_DESCRIPTION          ADDR(        0x124766,          0x10038055c)
#define RVA_GAME_MENU_SETTINGS_SECTION_END     ADDR(        0x124858,          0x1003805e8)
#define RVA_GAME_MENU_Q_KEY                    ADDR(        0x124ae8,          0x100380754)
#define RVA_GAME_MENU_SAVE_CALL                ADDR(        0x124f84,          0x100380a38)
#define RVA_GAME_MENU_PREFS_CALL               ADDR(        0x124fa4,          0x100380a50)
#define RVA_GAME_MENU_SAVE_QUIT_CALL           ADDR(        0x124fce,          0x100380a48)
#define RVA_GAME_MENU_RESIGN_SET               ADDR(        0x1250c4,          0x100380f10)
#define RVA_SAVE_DIALOG_SAVE_CALL              ADDR(        0x18b8fc,          0x1004f4bf0)
#define RVA_SAVE_DIALOG_SAVE_FOPEN_CALL        ADDR(        0x18b8ca,          MAC_NO_CALL)
#define RVA_LOBBY_SAVE_CALL                    ADDR(        WIN_NO_CALL,       0x1003b7f98)
#define RVA_SAVE_LOBBY_GAME_CALL               ADDR(        0x12eb66,          0x1004f83bc)
#define RVA_SERVER_SAVE_CALL                   ADDR(        WIN_NO_CALL,       0x1004fa718)
#define RVA_SETUP_SERVER_LOAD_CALL             ADDR(        0x1f196f,          0x100502a1c)
#define RVA_NET_CLIENT_LOAD_CALL               ADDR(        0x1eab99,          0x10050b740)
#define RVA_SETUP_SERVER_LOAD_SCREEN_CALL      ADDR(        0x1f1990,          0x1005029e0)
#define RVA_SETUP_SERVER_PLAY_GAME_CALL        ADDR(        0x1f19ce,          0x100502760)
#define RVA_MAIN_MENU_LOAD_CALL                ADDR(        0x18b366,          0x10032e388)
#define RVA_MAIN_MENU_LOADED_SET               ADDR(        0x192820,          0x10032e39c)
#define RVA_CMDLINE_LOAD_CALL                  ADDR(        0x191561,          0x100333908)
#define RVA_CMDLINE_PLAY_GAME_CALL             ADDR(        0x1915a4,          0x100333998)
#define RVA_BENCH_LOAD_CALL                    ADDR(        0x191355,          0x100333438)
#define RVA_BENCH_PLAY_GAME_CALL               ADDR(        0x1913a2,          0x100333564)
#define RVA_AUTORUN_SAVE_CALL                  ADDR(        0x09213e,          0x10038a340)
#define RVA_AUTOSAVE_SAVE_CALL                 ADDR(        0x0929a5,          0x10038b018)
#define RVA_AUTOSAVE_FUNCTION_SAVE_CALL        ADDR(        0x09447b,          MAC_INLINED)
#define RVA_AUTORUN_SAVE_FOPEN_CALL            ADDR(        0x092115,          MAC_NO_CALL)
#define RVA_AUTOSAVE_SAVE_FOPEN_CALL           ADDR(        0x09297d,          MAC_NO_CALL)
#define RVA_AUTOSAVE_FUNCTION_SAVE_FOPEN_CALL  ADDR(        0x094451,          MAC_NO_CALL)
#define RVA_NET_CLIENT_MAP_SCREEN_CALL         ADDR(        0x1eac06,          0x10050b7b0)
#define RVA_NET_CLIENT_SQUARE_INFO_CALL        ADDR(        0x1e9770,          MAC_NOT_FOUND)
#define RVA_NET_CLIENT_DATE_CALL               ADDR(        0x1e995e,          MAC_NO_CALL)
#define RVA_TOP_BAR_DATE_CALL                  ADDR(        0x09c79d,          MAC_NO_CALL)

// ---- User folder and startup folders ------------------------------------------------------------
#define RVA_SETUP_DIRS                         ADDR(        0x194710,          MAC_INLINED)
#define RVA_EXPAND_PATH                        ADDR(        0x05bb50,          0x1003362f0)
#define RVA_MAKE_DIR                           ADDR(        0x059400,          0x100336758)
#define RVA_GETENV                             ADDR_DIFFERS(0x00ede8,          0x1005ae65c)
#define RVA_USER_DIR                           ADDR(        0x58261f0,         0x113827870)
#define RVA_USER_DIR_EXPAND_CALL               ADDR_DIFFERS(0x194aef,          0x10032f5e4)
#define RVA_EXPAND_PATH_VAR_GETENV_CALL        ADDR(        0x05bc37,          0x1003363b0)
#define RVA_EXPAND_PATH_HOME_GETENV_CALL       ADDR(        0x05bc7a,          0x1003363f8)
#define RVA_COE5_SAVE_GETENV_CALL              ADDR(        0x194afb,          0x10032f5f0)
#define RVA_COE5_DATA_GETENV_CALL              ADDR(        0x19480f,          0x10032f480)
#define RVA_MAPS_DIR                           ADDR(        0x5829ed0,         0x10b848b00)
#define RVA_BATMAPS_DIR                        ADDR(        0x5829500,         0x10b903289)
#define RVA_SAVES_DIR                          ADDR(        0x582a8a0,         0x1122079cd)
#define RVA_GAMELOGS_DIR                       ADDR(        0x5825750,         0x112207008)
#define RVA_OLDIPADR_FILE                      ADDR(        0x5827790,         0x1088104d1)
#define RVA_TEMP_DIR                           ADDR(        0x582b270,         0x10880f8d0)
#define RVA_STR_USER_DIR_DEFAULT               ADDR(        0x1c5ade8,         MAC_IN_CODE)
#define RVA_STR_COE5_SAVE                      ADDR(        0x1c5afe8,         0x100605387)
#define RVA_STR_COE5_DATA                      ADDR(        0x1c5afb8,         0x100605334)

// ---- Game state ---------------------------------------------------------------------------------
#define RVA_TURN                               ADDR(        0x443894,          0x1049c81c0)
#define RVA_ACTIVE_PLAYER                      ADDR(        0x443a9c,          0x105a0ae90)
#define RVA_LOADED_GAME                        ADDR_DIFFERS(0x5822f58,         0x10ce02ef4)
#define RVA_LOADED_MAP_NAME                    ADDR(        0x582bc40,         0x1136f574d)
#define RVA_RESIGN                             ADDR_DIFFERS(0x58294f4,         0x10ce02f00)
#define RVA_AUTORUN                            ADDR_DIFFERS(0x582815c,         0x10ce0cb68)
#define RVA_SKIP_SETUP_A                       ADDR_DIFFERS(0x5822fb8,         0x1136f6148)
#define RVA_SKIP_SETUP_B                       ADDR_DIFFERS(0x582399c,         0x10c39946c)
#define RVA_MAP_DESCRIPTION                    ADDR_DIFFERS(0xf06c420,         0x11306ebf4)
#define RVA_SIMULTANEOUS                       ADDR(        WIN_IN_CODE,       MAC_IN_CODE)
#define RVA_AUTOSAVE_ON                        ADDR_DIFFERS(0x5828b28,         0x1069b16a4)
#define RVA_AI_MOVE_DELAY                      ADDR(        0x1a52c7c,         MAC_NOT_LOOKED_UP)
#define RVA_WORLD_W                            ADDR(        0xd63e948,         0x1121121ec)
#define RVA_WORLD_H                            ADDR(        0xd63e94c,         0x1121121f0)
#define RVA_SQUARE_INFO_X                      ADDR(        0x1aba43c,         0x100873504)
#define RVA_SQUARE_INFO_Y                      ADDR(        0x1ab9e0c,         0x100873508)
#define RVA_POINTER_SQUARE                     ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_PLANES                             ADDR(        0xd63e920,         0x1121121c4)
#define PLANE_SIZE                             0x48
#define PLANE_NAME_SIZE                        0x20
#define PLANE_TABLE_MAX                        12
#define PLANE_X                                0x20
#define PLANE_Y                                0x24
#define PLANE_W                                0x28
#define PLANE_H                                0x2c
#define PLANE_TINT_ON                          0x30
#define PLANE_TINT                             0x34
#define RVA_MAP_DRAW_PLANE_TINT                ADDR(        0x19e164,          0x10049eb20)
#define RVA_BLOOD_SUN_UPDATE                   ADDR(        0x1b1710,          MAC_INLINED)
#define RVA_MAP_SCREEN_BLOOD_SUN_CALL          ADDR(        0x094bc0,          MAC_NO_CALL)
#define RVA_RITUAL_BLOOD_SUN_CALL              ADDR(        0x2c9eb5,          MAC_NO_CALL)
#define RVA_MAP_SCREEN_BLOOD_SUN_DONE          ADDR(        0x094bc5,          0x10037b718)
#define RVA_RITUAL_BLOOD_SUN_DONE              ADDR(        0x2c9eba,          0x100563000)
#define RVA_BLOOD_SUN_COLOUR                   ADDR(        0x406e50,          0x100757e80)
#define RVA_SKY_UNDERLAY_CHECK                 ADDR(        0x24023b,          MAC_NOT_LOOKED_UP)
#define PLANE_FLAGS                            0x44
#define PLANE_FLAG_DISCOVERED                  1
#define RVA_LAST_PLANE                         ADDR(        0x1adb26c,         0x1049c81b8)
#define RVA_PLANE_AT                           ADDR(        0x2557d0,          0x10056dfe0)
#define PLANE_ELYSIUM2                         10
#define RVA_CREATE_EXTRA_PLANES                ADDR(        0x2a3970,          0x1004b45e8)
#define RVA_LOAD_MAP_PLANE                     ADDR(        0x1b5f30,          0x100513b40)
#define RVA_LOAD_AZTLAN_PLANE                  ADDR(        0x247480,          MAC_INLINED)
#define RVA_LOAD_PRIMAL_PLANE                  ADDR(        0x24b980,          0x1005786ac)
#define RVA_CREATE_CELESTIAL_PLANE             ADDR(        0x24b3b0,          0x100578a4c)
#define RVA_CREATE_SKY_PLANE                   ADDR(        0x24f4a0,          0x100579c30)
#define RVA_CREATE_AGARTHA_PLANE               ADDR(        0x24d250,          0x100572680)
#define RVA_CREATE_INFERNO_PLANE               ADDR(        0x24ff60,          0x10056e1b0)
#define RVA_CREATE_HADES_PLANE                 ADDR(        0x244d40,          MAC_INLINED)
#define RVA_CREATE_ELEMENTAL_PLANE             ADDR(        0x248c20,          0x10057546c)
#define RVA_MAP_PLANE_INDEX                    ADDR(        0x1a6b7b4,         0x100873500)
#define RVA_MAP_FILE_PLANE                     ADDR(        0x582c6bc,         0x108810eec)
#define RVA_MAP_FILE_HAS_PLANE                 ADDR(        0x582c680,         0x10ce0cb58)
#define RVA_MAP_PLANE_WORD_CHECK               ADDR(        0x1b6253,          0x1005131d0)
#define RVA_LOAD_MAP_PLANE_LINE_CALL           ADDR(        0x1b5ff4,          0x100513bfc)
#define RVA_MAP_LINE_MAPSIZE_ADDPLANE_CALL     ADDR(        0x1b63dc,          0x1005132ac)
#define RVA_MAP_VERSION                        ADDR(        0x582c68c,         0x108810ee8)
#define RVA_MAP_LINE_ROW_BOUNDS                ADDR(        0x1b6454,          0x1005132f4)
#define RVA_MAP_LINE_SQUARENAME_BOUNDS         ADDR(        0x1b6c69,          0x100513858)
#define RVA_MAP_LINE_SQUARESPEC_OR             ADDR(        0x1b6db3,          0x100513954)
#define RVA_MAP_LINE_PORTAL_BOUNDS             ADDR(        0x1b6e32,          0x1005139a0)
#define RVA_ADDPLANE_CAP_CHECK                 ADDR(        WIN_INLINED,       0x10056e0a4)
#define RVA_ELYSIUM2_INDEX_CHECK               ADDR(        0x2a3dda,          0x1004b4c00)
#define RVA_VOID_INDEX_CHECK                   ADDR(        0x2a3cc0,          0x1004b4b6c)
#define RVA_CENTER_MAP_ON                      ADDR(        0x0b0b00,          MAC_NOT_LOOKED_UP)
#define RVA_VIEW_PLANE                         ADDR(        0x23e5d0,          0x10048f610)
#define RVA_PLANE_CAMERA_X                     ADDR(        0x9fab220,         0x106cee504)
#define RVA_PLANE_CAMERA_Y                     ADDR(        0x9fab250,         0x106cee530)
#define RVA_TOP_BAR_PLANE_SLOT_CHECK           ADDR(        0x09d6b6,          0x100379870)
#define RVA_LOAD_PLANE_RECORD                  ADDR(        0x189ba0,          MAC_INLINED)
#define RVA_LOAD_GAME_PLANES_CALL              ADDR(        0x187215,          MAC_INLINED)
#define RVA_LOAD_GAME_DEPTH_FROM_ELYSIUM2      RVA_LOAD_GAME_MAP_DEPTH
#define RVA_SAVE_PLANE_RECORD                  ADDR(        0x189dc0,          MAC_INLINED)
#define RVA_SAVE_GAME_PLANES_CALL              ADDR(        0x18fa4a,          MAC_INLINED)
#define RVA_MAP_DRAW_PLANE                     ADDR_DIFFERS(0x19c920,          0x10049ac6c)
#define RVA_PLANE_SPAWN_CHANCE                 ADDR_DIFFERS(0x253440,          0x100571d78)
#define RVA_TAINTED_MAGIC_PASS                 ADDR(        0x142e90,          MAC_INLINED)
#define RVA_WORLD_STEP_MMR                     RVA_MARK_RIVER
#define RVA_WORLD_STEP_RL2                     ADDR(        0x242a60,          0x1004b3b40)
#define RVA_PICK_PLANE_SLOT_CHECK              ADDR(        0x1b0372,          MAC_NOT_FOUND)
#define RVA_PLANE_SAME_SPOT_CAP_CHECK          ADDR(        0x254576,          0x100570860)
#define RVA_PLANESHIFT_UNIT                     ADDR(        0x2540b0,           MAC_INLINED)
#define RVA_NEW_PORTAL_PAIR                    ADDR(        0x2542c0,          0x1005782e8)
#define RVA_PLANE_ORDER_STEP                   ADDR(        0x252af0,          MAC_INLINED)
#define RVA_MAP_VIEW_PLANE_LAST_STOP           ADDR(        0x23d90a,          0x10048dc24)
#define RVA_COUNT_TERRAIN_ON_PLANE             ADDR_DIFFERS(0x32c610,          0x1004cea8c)
#define RVA_SET_GROUND_PASS                    RVA_SET_GROUND_RECT
#define RVA_UNIT_SHAPE_UPDATE                  RVA_UNIT_FIT_TERRAIN
#define RVA_UNIT_PLANE_SHAPE_ID                ADDR_DIFFERS(0x115f9b,          0x100467938)
#define RVA_UNIT_PLANE_SHAPE_CALL              ADDR_DIFFERS(0x115fd7,          0x100467950)
#define MONSTER_ABILITY_LANDSHAPE             0x9e
#define MONSTER_ABILITY_ELYSIUMSHAPE           0x4d8
#define RITUAL_FX_SETPLANE                     938
#define RITUAL_FX_PLANEREQ                     939
#define RITUAL_FX_PLANELOC                     960
#define RITUAL_FX_RANDOMLOC                    969
#define RITUAL_FX_RANDTERRLOC                  971
#define RITUAL_FX_RANDNEWLOC                   972
#define RITUAL_FX_PLANESWAP                    984
#define RITUAL_FX_MULTIPLANE                   1048
#define RITUAL_FX_AIONLYPLANE                  1104
#define RVA_RITUAL_SETPLANE_RANDOM             ADDR_DIFFERS(0x2c650f,          0x10056055c)
#define RVA_RITUAL_PLANESWAP_DST_CHECK         ADDR_DIFFERS(0x2c8cf7,          0x1005622c8)
#define RVA_PLACE_HOME_PORTAL                  ADDR(        0x0a43e0,          MAC_INLINED)
#define RVA_START_CHECK                        ADDR(        0x2ea4c0,          MAC_INLINED)
#define RVA_AI_ONLY_PLANE_CMP                  ADDR(        0x06b8a7,          MAC_NOT_LOOKED_UP)
#define RVA_CREATE_RANDOM_MAP_ADDPLANE         ADDR_DIFFERS(0x2a1407,          0x1004b4ce4)
#define RVA_ADDPLANE_MAP_DEPTH                 ADDR(        0x250cf1,          0x10056e164)
#define RVA_LOAD_GAME_MAP_DEPTH                ADDR(        0x18722a,          0x1004ee070)
#define RVA_BUG_CHECK_ON_ELYSIUM2              ADDR(        0x12ae0d,          0x100464360)
#define RVA_VIEW_PLANE_CLAMP                   ADDR(        0x23e5f6,          0x10048f63c)
#define RVA_VIEW_PLANE_CAMERA_RESET            ADDR_DIFFERS(0x1ad87dc,         0x106cee500)
#define RVA_TOP_BAR_PLANE_LOOP_END             ADDR(        0x09d80d,          0x100379868)
#define RVA_TOP_BAR_PLANE_SLOT_WIDTH           ADDR_DIFFERS(0x09d69f,          0x1003797b0)
#define RVA_PLANE_SEEN                         ADDR(        0x2543f0,          MAC_INLINED)
#define RVA_PLAYER_PLANE_SEEN                  ADDR(        0x9fab280,         0x10c921ed8)
#define RVA_MAP_BG_BUILT                       ADDR(        0x63fbbc0,         0x10c6919f0)
#define RVA_MAP_BG_PIXELS                      ADDR(        0x63fbbf0,         0x10c691a78)
#define RVA_MAP_BG_DIRTY                       ADDR(        0x6498050,         0x10c691ad0)
#define RVA_MAP_BG_SIZE                        ADDR(        0x6498080,         0x10c691a1c)
#define RVA_MAP_BG_BUILDER                     ADDR(        0x0b3130,          0x1005510fc)
#define RVA_MAP_BG_BUILDER_IMAGE_CALL          ADDR(        0x0b331f,          0x1005514e4)
#define RVA_MAP_DRAW_PLANE_CLAMP               ADDR(        0x19c981,          MAC_NOT_FOUND)
#define RVA_MAP_BG_DIRTY_CHECK                 ADDR(        0x17ed63,          0x1004e6d44)
#define RVA_MAP_BG_DIRTY_CHECK_EDITOR          ADDR_DIFFERS(0x1b4855,          0x1005160d0)
#define RVA_MAP_BG_MARK                        ADDR(        0x0b5650,          0x100551684)
#define RVA_MAP_BG_MARK_CHECK                  ADDR_DIFFERS(0x0b5666,          0x10055169c)
#define RVA_PLANE_TABLE_REF                    ADDR_DIFFERS(0x1b0cc4,          0x10056dff8)
#define RVA_MAP_BG_BUILT_REF                   ADDR_DIFFERS(0x091d57,          0x100389918)
#define RVA_MAP_BG_DIRTY_REF                   ADDR_DIFFERS(0x0b56aa,          0x1005516c4)
#define RVA_ICON_BUTTON_AT                     ADDR_DIFFERS(0x037c90,          0x10033ed44)
#define RVA_ICON_BUTTON_TINTED_AT              ADDR(        0x0372b0,          0x10033ef9c)
#define PLANE_SPRITE_SHOWN                     0x59
#define PLANE_SPRITE_VIEWED                    0x63
#define PLANE_BUTTON_ID_FIRST                  1000
#define MISC_TRS_SPRITES                       181
#define RVA_TOP_BAR_PLANE_SHOWN_TEXT_CALL      ADDR(        0x09d7af,          0x10037985c)
#define RVA_TOP_BAR_PLANE_VIEWED_TEXT_CALL     ADDR(        0x09d742,          MAC_NO_CALL)
#define RVA_TOP_BAR_PLANE_UNKNOWN_TEXT_CALL    ADDR(        0x09d801,          MAC_NO_CALL)
#define RVA_EDITOR_PLANE_SHOWN_TEXT_CALL       ADDR(        0x1b3ff2,          0x100514f28)
#define RVA_EDITOR_PLANE_VIEWED_TEXT_CALL      ADDR(        0x1b3f9c,          MAC_NO_CALL)
#define RVA_EDITOR_PLANE_UNKNOWN_TEXT_CALL     ADDR(        0x1b4042,          MAC_NO_CALL)
#define RVA_PLANE_SCREEN_CALL_RITUAL_A         ADDR(        0x2c7694,          0x100560a5c)
#define RVA_PLANE_SCREEN_CALL_RITUAL_B         ADDR(        0x2c77c6,          0x100560af8)
#define RVA_PLANE_SCREEN_CALL_OTHER            ADDR(        0x2df32e,          0x100354d2c)
#define RVA_PLANE_SCREEN_ROW_END_CALL          ADDR(        0x1b079a,          0x1004a7020)
#define RVA_GAME_OVER_PLANE_ROW_END_CALL       ADDR(        0x1af76c,          0x1004747f0)
#define RVA_PLANE_INFO_SCREEN                  ADDR(        0x254a80,          0x10057c974)
#define RVA_PLANE_INFO_ICON_CALL               ADDR(        0x254c06,          0x10057cbcc)
#define RVA_SAVE_GAME_MAGIC_WRITE              ADDR(        0x18eb81,          0x1004f2028)
#define RVA_SERVER_SAVE_MAGIC_WRITE            ADDR(        0x1f4449,          MAC_NO_CALL)
#define RVA_LOAD_GAME_MAGIC_READ               ADDR(        0x185faf,          0x1004eb498)
#define RVA_COUNT_SAVES_MAGIC_READ             ADDR(        0x18b541,          0x1004eb1ac)
#define RVA_RITUAL_SETPLANE_RANDOM_CALL        ADDR(        0x2c6513,          0x100560560)
#define RVA_REPLACE_SPRITE                     ADDR(        0x063770,          0x10034fe40)
#define RVA_LONG_PRESS                         ADDR(        0x23ece0,          MAC_INLINED)
#define SQUARE_AI_STRENGTH                     0xac
#define SQUARE_TRUE_TERRAIN_SEEN               0x83
#define SQUARE_SEEN                            0x43
#define SQUARE_SEEN_MARK                       0x63
#define RVA_DATE_TEXT                          ADDR(        0x5822ef0,         0x106cee464)
#define RVA_SEASON_NAMES                       ADDR(        0x344400,          0x100834710)
#define RVA_UNIT_NAME_RING                     ADDR(        0x64a70e0,         0x1069b16dc)
#define RVA_STACK_COOKIE                       ADDR_DIFFERS(0x42c000,          0x100774220)
#define RVA_SEASON                             ADDR(        0x4438e4,          0x1049c81c4)
#define RVA_SEASON_TURNS_LEFT                  ADDR(        0x443864,          0x1049c81c6)
#define RVA_UNIT_NAME_RING_INDEX               ADDR(        0x5804bf4,         0x1069b16d8)
#define RVA_MESSAGE_LAST                       ADDR(        0x1aaa1fc,         0x1049c81bc)
#define RVA_SQUARES                            ADDR(        0xa20acc0,         0x10d011c98)
#define RVA_TEST_GAME_FLAG                     ADDR_DIFFERS(0x4438bc,          0x1059bcc88)
#define RVA_TIP_SHOWN                          ADDR_DIFFERS(0x5835760,         0x106cee5ac)
#define RVA_TIP_TEXT                           ADDR(        0x58357a0,         0x106cee5ad)

#define RVA_NET_CLIENT                         ADDR_DIFFERS(0x5828b24,         0x1136f572c)
#define RVA_NET_SERVER                         ADDR_DIFFERS(0x58294f8,         0x10880c9ec)
#define RVA_NET_LOBBY                          ADDR_DIFFERS(0x5823988,         0x1136f5728)

// ---- Plane generation: Celestial, Sky, Agartha, Void -------------------------------------------
#define RVA_GEN_SCRATCH_MARKS                  ADDR(        0x9fab3e0,         0x107e98282)
#define RVA_GEN_MARK_WALK                      ADDR(        0x2a8ac0,          0x1004ba7e8)
#define RVA_GEN_MARKS_TO_TERRAIN               ADDR(        0x2a9650,          0x1004b9c6c)
#define RVA_GEN_MARKS_TO_TERRAIN_CHANCE        ADDR_DIFFERS(0x2aa380,          0x1004ae3ac)
#define RVA_GEN_MARK_NEAR_TERRAIN              ADDR(        0x2a8880,          0x1004b9f30)
#define RVA_GEN_MARK_VOID_NEAR                 ADDR_DIFFERS(0x2aa240,          0x1004b9dc4)
#define RVA_GEN_MARK_RECT                      ADDR(        0x2a9bd0,          0x1004b9a00)
#define RVA_GEN_SET_FIXED_TEMPERATURE          ADDR(        0x2a8630,          0x1004ab500)
#define RVA_GEN_FIND_CAVE_SPOT                 ADDR_DIFFERS(0x2a9960,          0x1004ba308)
#define RVA_GEN_DIAGONAL_FIX                   ADDR_DIFFERS(0x254650,          0x100572224)
#define RVA_GEN_FLOOD_PLANE                    ADDR(        0x199930,          0x1004aa05c)
#define RVA_GEN_SQUARE_IS_WET                  ADDR(        0x2ad320,          MAC_INLINED)
#define RVA_GEN_WET_NEIGHBOURS                 ADDR(        0x2adac0,          0x1004acb84)
#define RVA_GEN_UPDATE_MOIST_WALLS             ADDR(        0x199730,          0x1004aa488)
#define RVA_GEN_SET_GROUND                     RVA_SET_GROUND_RECT
#define RVA_GEN_SPREAD_TERRAIN                 ADDR(        0x2418c0,          0x100573c7c)
#define RVA_CHANGE_SQUARE_TERRAIN              ADDR(        0x334e50,          0x1004c6114)
#define RVA_GEN_KATABATIC_FIRST                ADDR(        0x1b1590c,         0x100872c38)
#define RVA_CREATE_VOID_PLANE_CODE             ADDR(        0x2a3c5b,          0x1004b4b54)
#define RVA_GEN_SET_TEMPERATURES               RVA_WORLD_PART_BANDS

// ---- Plane generation: Inferno, Hades, Elemental, Aztlan, Primal, specials ----------------------------
#define RVA_PLANE_SPECIAL_TERRAIN              ADDR(        0x2e8d60,          0x100596f44)
#define RVA_INFERNO_SPECIAL_TERRAIN            ADDR(        0x247530,          0x10056f250)
#define RVA_HADES_SPECIAL_TERRAIN              ADDR(        0x243b90,          MAC_INLINED)
#define RVA_ELEMENTAL_SPECIAL_TERRAIN          ADDR(        0x245430,          0x100576828)
#define RVA_GROUND_FROM_NEIGHBOURS             ADDR(        0x0b3a00,          MAC_NOT_LOOKED_UP)
#define RVA_INIT_SQUARE_CORPSES                ADDR(        0x32c850,          0x1004ce0e8)
#define RVA_INIT_SQUARE_GEMS                   ADDR(        0x32d6c0,          0x1004c65ec)
#define RVA_NAME_ALL_SQUARES                   ADDR(        0x2e9290,          MAC_INLINED)
#define RVA_SQUARE_NAME_TEXT                   ADDR(        0x2e9500,          0x100598548)
#define RVA_MAP_MARKS                          RVA_GEN_SCRATCH_MARKS
#define RVA_MAP_MARKS_CLEAR_CALL               ADDR(        0x2a2803,          MAC_NOT_LOOKED_UP)
#define RVA_MARKS_SAVE                         ADDR(        0x2a1290,          MAC_NOT_LOOKED_UP)
#define RVA_MARKS_RESTORE                      ADDR(        0x2a1220,          MAC_NOT_LOOKED_UP)
#define RVA_MARK_RECT                          RVA_GEN_MARK_RECT
#define RVA_MARK_TO_TERRAIN                    RVA_GEN_MARKS_TO_TERRAIN_CHANCE
#define RVA_MARK_TERRAIN_CHANGE                RVA_GEN_MARKS_TO_TERRAIN
#define RVA_MARK_BLOB                          ADDR(        0x2a8380,          0x1004b9798)
#define RVA_MARK_CAVE                          RVA_GEN_MARK_WALK
#define RVA_MARK_RIVER                         ADDR(        0x2a8de0,          0x1004ad7bc)
#define RVA_RIVER_DISTANCE                     ADDR(        0xd5a2520,         0x107e4a078)
#define RVA_RIVER_COST                         ADDR(        0xd5f0720,         0x107dfbe78)
#define RVA_MARK_RANDOM_LOC                    ADDR(        0x2a8750,          0x1004ba0a0)
#define RVA_MARK_NEIGHBOURS                    RVA_GEN_MARK_NEAR_TERRAIN
#define RVA_CAVE_WALL_SPOT                     RVA_GEN_FIND_CAVE_SPOT
#define RVA_SET_COLD_FLAGS                     ADDR(        0x2a89d0,          MAC_INLINED)
#define RVA_HEAT_BLOB                          ADDR(        0x2a6c30,          MAC_NOT_LOOKED_UP)
#define RVA_SET_PORTAL_PAIR                    ADDR(        0x241690,          0x1005705a8)
#define RVA_LINK_SQUARES_NEW_PORTAL            RVA_NEW_PORTAL_PAIR
#define RVA_RANDOM_LAND_SQUARE                 RVA_RANDOM_RITUAL_SQUARE
#define RVA_SQUARE_IS_OPEN_LAND                ADDR(        0x2a3f70,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_WATERLIKE                   ADDR(        0x2adf10,          MAC_NOT_LOOKED_UP)
#define RVA_SPREAD_TERRAIN                     RVA_GEN_SPREAD_TERRAIN
#define RVA_INFERNO_CENTRE_X                   ADDR(        0x1ae02d4,         0x10487ff30)
#define RVA_INFERNO_CENTRE_Y                   ADDR(        0x1ae0664,         0x10487ff34)
#define RVA_TERRAIN_MATCH_ELEMENTAL_QUARTER    ADDR(        0x2c4fa6,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_MATCH_INFERNO_REGION       ADDR(        0x2c5513,          MAC_NOT_LOOKED_UP)
#define RVA_SET_TEMPERATURE_RECT               RVA_WORLD_PART_BANDS
#define SQUARE_GROUND                          0x22
#define SQUARE_TEMPERATURE                     0x26
#define SQUARE_FLAG_COLD                       8
#define SQUARE_COLD_TEMPERATURE_MAX            25
#define SQUARE_GROUND_LAST                     28
#define SQUARE_NAME                            0x0
#define SQUARE_NAME_SIZE                       0x20

// ---- New game settings --------------------------------------------------------------------------
#define RVA_SOCIETY                            ADDR(        0xf07b00a,         0x11307d7de)
#define RVA_COMMON_CAUSE                       ADDR(        0xf07b018,         0x11307d7ec)
#define RVA_SCORE_GRAPHS                       ADDR(        0xf07b01c,         0x11307d7f0)
#define RVA_BATTLE_REPORTS                     ADDR(        0xfb2cc2c,         0x10ce0cb56)
#define RVA_START_FLAGS                        ADDR(        0xf07b008,         0x11307d7dc)
#define RVA_WILDER                             ADDR(        0xf07b030,         0x11307d804)
#define RVA_MAP_W                              ADDR(        0x582c60c,         0x11307d80c)
#define RVA_MAP_H                              ADDR(        0x582c608,         0x11307d810)
#define RVA_MAP_DEPTH                          ADDR(        0x582c604,         0x1121121c0)
#define RVA_MAP_EDITOR_DEPTH_NOP               ADDR(        WIN_NO_EQUIVALENT, 0x100516134)
#define RVA_NORTH_PART                       ADDR(        0x1a51f34,         0x105a0aecc)
#define RVA_SOUTH_PART                         ADDR(        0x1a4449c,         0x105a0aed0)
#define RVA_UNIQUE_RANDOM_DEFAULT              ADDR_DIFFERS(0x5824d7c,         0x10ce0cb50)
#define RVA_TEST_GAME_A                        RVA_MAP_W
#define RVA_TEST_GAME_B                        RVA_MAP_H
#define RVA_CLI_MAP_W                          ADDR(        0x1a3f08c,         0x100872c14)
#define RVA_CLI_MAP_H                          ADDR(        0x1a50e9c,         0x100872c18)
#define RVA_CLI_SOCIETY                        ADDR(        0x1a3b38c,         0x100872c20)
#define RVA_START_DEFAULT                      ADDR(        0x1a3b2fc,         0x100872c1c)
#define RVA_WILDER_DEFAULT                     ADDR_DIFFERS(0x1a4c9fc,         0x107e9827c)
#define RVA_SUBSOC                             ADDR_DIFFERS(0x58261ec,         0x107e98280)
#define RVA_SETUP_NETWORK                      ADDR(        0xf07ae80,         0x11307d654)
#define RVA_CLI_GRAPHS                         ADDR_DIFFERS(0x5822fbc,         0x107e98278)
#define RVA_CLI_COMMON_CAUSE                   ADDR_DIFFERS(0x5823990,         0x106cd54d4)
#define RVA_CLI_NEW_GAME                       ADDR_DIFFERS(0x582437c,         0x108810494)
#define RVA_CLI_MAP_EDITOR                     ADDR_DIFFERS(0x5826118,         0x105aa6804)
#define RVA_SDL_MAIN_STARTUP_MODS_RUN_CALL     ADDR(        0x191299,          0x100332b00)
#define RVA_CLI_BATTLE_REPORTS                 ADDR_DIFFERS(0x5823998,         0x10ce0cb54)
#define RVA_MAP_SIZE_WIDTHS                    ADDR_DIFFERS(0x406e20,          0x10075c660)
#define RVA_MAP_SIZE_HEIGHTS                   ADDR_DIFFERS(0x406e10,          0x10075c674)
#define RVA_SOCIETY_TABLE                      ADDR(        0x5697b60,         0x100872c90)
#define SOCIETY_TABLE_MAX                      12
#define RVA_MAP_SETTINGS_SUB_SOCIETY_CALL      ADDR(        WIN_NO_CALL,       0x1004b2be8)
#define RVA_MAP_SETTINGS_COMMON_CAUSE_CHECKBOX ADDR(        0x2aaef9,          0x1004b27c0)
#define RVA_MAP_SETTINGS_START_FLAGS_CLEAR     ADDR(        0x2ab49a,          0x1004b3544)
#define RVA_MAP_SETTINGS_WIDTHS_REF            ADDR(        0x2aa620,          0x1004b3968)
#define RVA_MAP_SETTINGS_HEIGHTS_REF           ADDR(        0x2aa62d,          0x1004b3a14)
#define RVA_AUTO_NAME_TABLE                    ADDR(        0x344420,          0x10077ecd0)
#define RVA_CLI_CLUSTER_START_STORE            ADDR_DIFFERS(0x195a6c,          0x100329678)
#define RVA_CLI_NO_CLUSTER_START_STORE         ADDR(        0x195a9d,          0x100329684)
#define RVA_CLI_WEST_EAST_STORE                ADDR_DIFFERS(0x195afb,          0x100329678)
#define RVA_CLI_NORTH_PART_STORE               ADDR(        0x196dbb,          0x100329d68)
#define RVA_CLI_SOUTH_PART_STORE               ADDR(        0x196dfd,          0x100329d90)
#define RVA_MAP_SETTINGS_START_READ            ADDR(        0x2aa6a4,          0x1004b2500)
#define RVA_MAP_SETTINGS_NORTH_DEFAULT         ADDR(        0x2aa919,          0x1004b2738)
#define RVA_MAP_SETTINGS_SOUTH_DEFAULT         ADDR(        0x2aa92f,          0x1004b2750)
#define RVA_MAP_SETTINGS_NORTH_STORE           ADDR(        0x2ab323,          0x1004b2f34)
#define RVA_MAP_SETTINGS_SOUTH_STORE           ADDR(        0x2ab3a6,          0x1004b2fd0)
#define RVA_MAP_SETTINGS_WILDER_STORE          ADDR(        0x2ab4d4,          0x1004b35b4)
#define RVA_NUMBER_BOX                         ADDR(        0x042770,          0x100348bc8)
#define RVA_SETTINGS_FROM_CLI                  ADDR(        0x111550,          MAC_INLINED)
#define RVA_LOAD_GAME_WILDER                   ADDR(        0x18ad5c,          0x1004ebd18)
#define RVA_SAVE_GAME_WILDER                   ADDR(        0x18b0c0,          0x1004f25b4)
#define RVA_SETTINGS_BLOCK_LOAD                ADDR(        0x18aad0,          MAC_NOT_FOUND)
#define RVA_SETTINGS_BLOCK_SAVE                ADDR(        0x18af10,          MAC_NOT_FOUND)
#define RVA_WORLD_NORTH_PART                   ADDR(        0x5835f9c,         0x108810ea4)
#define RVA_WORLD_SOUTH_PART                   ADDR(        0x5835f98,         0x108810ea8)
#define RVA_SOCIETY_PRESETS                    ADDR(        0x2d07c0,          MAC_INLINED)
#define RVA_WORLD_PART_BANDS                   ADDR(        0x2a9cb0,          0x1004ab700)
#define RVA_WORLD_PART_CHECK                   ADDR(        0x2a6934,          0x1004b8440)
#define RVA_WORLD_PART_TERRAIN                 ADDR(        0x2a6930,          MAC_NOT_FOUND)

#define RVA_NEW_GAME_WORLD                     ADDR(        0x3056c0,          0x100591684)
#define RVA_NEW_WORLD_NEW_GAME_CALL            ADDR(        0x30533a,          0x100589fd8)
#define RVA_NEW_WORLD_EXTRA_PLANES_CALL        ADDR(        0x305348,          0x100589fbc)
#define RVA_NEW_WORLD_SPECIAL_TERRAIN_CALL     ADDR(        0x30534f,          0x100589fc4)
#define RVA_NEW_GAME_RESET_CACHE_CALL          ADDR(        0x3057bf,          0x1005917c8)
#define RVA_NEW_GAME_PRESETS_CALL              ADDR(        0x3057cb,          MAC_NO_CALL)
#define RVA_NEW_GAME_CREATE_MAP_CALL           ADDR(        0x3057eb,          0x100591b4c)
#define RVA_NEW_GAME_EXTRA_PLANES_CALL         ADDR(        0x3057f9,          0x100591b5c)
#define RVA_NEW_GAME_CROSSINGS_CALL            ADDR(        0x30580e,          MAC_NO_CALL)
#define RVA_NEW_GAME_DECORATE_CALL             ADDR(        0x31b3e5,          0x10059266c)
#define RVA_NEW_GAME_SET_GROUND_CALL           ADDR_DIFFERS(0x31cbab,          0x100596a00)
#define RVA_NEW_GAME_SPECIAL_TERRAIN_CALL      ADDR(        0x31ccd0,          0x100596ee0)
#define RVA_CREATE_RANDOM_MAP                  ADDR(        0x2a1350,          0x1004b4c5c)
#define RVA_CREATE_MAP_SETTEMP_CALL            ADDR(        0x2a2d62,          0x1004b7240)
#define RVA_CREATE_MAP_ENLARGE_CALL_1          ADDR_DIFFERS(0x2a38b7,          0x1004b969c)
#define RVA_CREATE_MAP_ENLARGE_CALL_2          ADDR_DIFFERS(0x2a38da,          0x1004b96ac)
#define RVA_MAPGEN_HEIGHT_MAP                  ADDR(        0x054670,          MAC_INLINED)
#define RVA_MAPGEN_REMOVE_SMALL_ISLANDS        ADDR(        0x2a5bc0,          MAC_INLINED)
#define RVA_MAPGEN_SEA_PART                    ADDR(        0x2a4010,          MAC_INLINED)
#define RVA_MAPGEN_RANDOM_SQUARE               ADDR(        0x2a3e10,          0x1004b4344)
#define RVA_MAPGEN_SPEC_BLOB                   ADDR(        0x2a6240,          0x1004ac2f8)
#define RVA_MAPGEN_ADJUST_TEMPERATURE          ADDR(        0x2a6ac0,          MAC_INLINED)
#define RVA_MAPGEN_SIDE_WATER                  ADDR(        0x2ada60,          MAC_INLINED)
#define RVA_MAPGEN_NEAREST_TERRAIN             ADDR(        0x2a4fb0,          0x1004ae5fc)
#define RVA_MAPGEN_DRAW_RIVER                  ADDR(        0x2a5920,          0x1004ae0bc)
#define RVA_MAPGEN_SQUARE_SCRATCH              RVA_GEN_SCRATCH_MARKS
#define RVA_RANDOM_OPEN_DIE_SUM                RVA_DIE_OPEN_ENDED
#define RVA_SET_GROUND_RECT                    ADDR(        0x0b4b40,          0x100550464)
#define RVA_SET_GROUND_PLANE_FOUND             ADDR_DIFFERS(0x0b4c98,          0x1005505f4)
#define RVA_SET_GROUND_TERRAIN_READ            ADDR_DIFFERS(0x0b4c17,          0x100550584)
#define RVA_SET_GROUND_PLANE                   ADDR(        0x0b39c0,          MAC_INLINED)
#define RVA_ENLARGE_PLANE                      ADDR_DIFFERS(0x2420d0,          0x10056ed10)
#define RVA_MAPGEN_RIVER_CROSSINGS             ADDR(        0x2a5190,          MAC_INLINED)
#define RVA_DECORATE_SQUARE                    ADDR(        0x2abc10,          0x1004af774)
#define RVA_DECORATE_PLANE0_TEST               ADDR_DIFFERS(0x2ad08c,          0x1004b2300)
#define RVA_DECORATE_WORLD_W_READ              ADDR_DIFFERS(0x2acc47,          0x1004b1a84)
#define RVA_PUT_RARE_SQUARE                    ADDR(        0x3021f0,          MAC_INLINED)
#define RVA_MAPGEN_PORTAL_PAIR                 RVA_NEW_PORTAL_PAIR
#define RVA_MAPGEN_ANCIENT_FOREST              ADDR(        0x2e9f80,          MAC_INLINED)
#define RVA_MAPGEN_CLEAR_AREA                  ADDR(        0x2a5dc0,          MAC_INLINED)
#define RVA_MAPGEN_SLOT_IN_PLAY                ADDR(        0x31ee10,          MAC_INLINED)
#define RVA_MAPGEN_COUNT_TERRAIN               ADDR(        0x2a50d0,          MAC_INLINED)
#define RVA_GEN_RUIN_TEMPLE_SHARE              ADDR(        0x11f48ed8,        0x108810e9e)
#define RVA_GEN_DEAD_FOREST_CHANCE             ADDR(        0x11f49380,        0x108810e9a)
#define RVA_GEN_ANCIENT_FOREST_RATE            ADDR(        0x11f9efa0,        0x10ce0cb72)
#define RVA_GEN_SWAMP_CHANCE                   ADDR(        0x11f49818,        0x108810ef2)
#define RVA_GEN_BATTLEFIELD_SHARE              ADDR(        0x11f49816,        0x108810e98)
#define RVA_GEN_BATTLEFIELD_CHANCE             ADDR(        0x11f97a44,        0x10c922a00)
#define RVA_GEN_CASTLE_CHANCE                  ADDR(        0x11f48edc,        0x108810e96)
#define RVA_GEN_TOWER_CHANCE                   ADDR(        0x11f48f16,        0x10c922a02)
#define RVA_GEN_FORD_CHANCE                    ADDR(        0x11f49814,        0x108810ea0)
#define RVA_GEN_UNUSED                         ADDR(        0x11f49820,        MAC_NOT_FOUND)
#define RVA_GEN_SKY_FEATURE_CHANCE             ADDR(        0x11f48f34,        0x10ce0cb76)
#define RVA_GEN_LIBRARY_CHANCE                 ADDR(        0x11f48f54,        0x10ce0cb74)
#define RVA_GEN_RARE_CHANCE                    ADDR(        0x11f48eda,        0x10ce0cb70)
#define RVA_GEN_HILLS_CHANCE                   ADDR_DIFFERS(0x11f991d0,        0x108810ef4)
#define RVA_GEN_TOWN_SHARE                     ADDR(        0x11f48f14,        0x10c9229fc)
#define RVA_GEN_ANCIENT_RUIN_CHANCE            ADDR(        0x11f48ea8,        0x10c9229fe)
#define RVA_GEN_VILLAGE_CHANCE                 ADDR(        0x11f493c4,        0x10ce0cb6e)
#define RVA_GEN_MINE_CHANCE                    ADDR(        0x11f493b4,        0x108810e9c)
#define RVA_GEN_REALM_WORD                     ADDR(        0x11d20bd8,        0x10cdf0ecc)

#define RVA_NEW_WORLD_BATTLE_REPORTS_INIT      ADDR(        0x30343d,          0x1005872f8)
#define RVA_SAVE_GAME_BATTLE_REPORTS           ADDR(        0x18ef54,          0x1004f23f0)
#define RVA_SAVE_GAME_COMMON_CAUSE             ADDR(        0x18af74,          0x1004f24c4)
#define RVA_SAVE_GAME_SCORE_GRAPHS             ADDR(        0x18afb0,          0x1004f24ec)
#define RVA_LOAD_GAME_BATTLE_REPORTS           ADDR(        0x18650a,          0x1004ebadc)
#define RVA_LOAD_GAME_COMMON_CAUSE             ADDR(        0x18abb4,          0x1004ebbd8)
#define RVA_LOAD_GAME_SCORE_GRAPHS             ADDR(        0x18abf8,          0x1004ebc14)
#define RVA_SAVE_PLAYERS                       ADDR(        0x188010,          MAC_INLINED)
#define RVA_SAVE_PLAYERS_AI_LEVEL              ADDR(        0x188217,          0x1004f2d58)
#define RVA_LOAD_PLAYERS_AI_LEVEL              ADDR(        0x187cd7,          0x1004ecb18)
#define RVA_TURN_START_AI_LEVEL                ADDR(        0x167e23,          0x100392b5c)
#define RVA_AI_LEVEL_BONUS                     ADDR(        0x344290,          0x10075c6a8)
#define RVA_ADD_RESOURCE                       ADDR(        0x327540,          MAC_NOT_LOOKED_UP)
#define AI_LEVEL_TABLE_MAX                     11
#define RVA_COMMON_CAUSE_CHECK                 ADDR(        0x11f2f0,          0x100471ccc)
#define RVA_WINNER_CHECK                       ADDR(        0x11ce60,          MAC_NOT_LOOKED_UP)
#define RVA_GAME_WINNER                        ADDR(        0xfb2cc20,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_COMMANDER               ADDR(        0x10d290,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_CITADEL                 ADDR(        0x10cfa0,          MAC_NOT_LOOKED_UP)
#define RVA_PLANES_ALIKE                       ADDR(        0x253f60,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_CLASS_ABILITY               ADDR(        0x0b1120,          MAC_INLINED)
#define RVA_CLASS_ABILITY_NTH                  ADDR(        0x0b03e0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_OWNER                    ADDR(        0x129090,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_REPORTS_BATTLE_READ         ADDR(        0x0f5db5,          0x1003dd37c)
#define RVA_TOP_BAR_SCORE_GRAPHS               ADDR(        0x09db85,          0x10037ac90)
#define RVA_MISC_MENU_SCORE_GRAPHS             ADDR(        0x0a155c,          0x1003732ec)
#define RVA_PLAYER_OVERVIEW_SCORE_GRAPHS       ADDR(        0x123461,          0x100482338)
#define RVA_NET_SCORE_GRAPHS_READ              ADDR(        0x1ea421,          0x10050a5cc)
#define RVA_MAP_KEY_SCORE_GRAPHS               ADDR(        0x098579,          MAC_NOT_FOUND)
#define RVA_TYPED_WORD_SCORE_GRAPHS            ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)

// ---- Players ------------------------------------------------------------------------------------
#define PLAYER_SLOTS                           32
#define RVA_SLOT_CONTROLLER                    ADDR(        0xfb31ac0,         0x1121af140)
#define RVA_SLOT_CLASS                         ADDR(        0xfb2cc40,         0x112206fc8)
#define RVA_PLAYER_HOME_X                      ADDR(        0x11d32be0,        0x10cce2dd4)
#define RVA_PLAYER_HOME_Y                      ADDR(        0xfb8e780,         0x10cce2e14)
#define RVA_SLOT_AI                            ADDR(        0x11c12b92,        0x10cce2e86)
#define RVA_SLOT_TEAM                          ADDR(        0x11c12b90,        0x10cce2e84)
#define RVA_SLOT_NAME                          ADDR(        0x11c12b60,        0x10cce2e54)
#define PLAYER_BLOCK_SIZE                      0x9004
#define RVA_CAST_COUNTS                        ADDR(        0x11c12b98,        0x10cce2e8c)
#define RVA_AI_LEVEL_NAMES                     ADDR(        0x3442c0,          0x100848390)
#define RVA_SLOT_COLOUR                        ADDR(        0x11c1bb18,        0x10ccebe0c)
#define RVA_SLOT_LOCK_CLASS                    ADDR(        0xf07ae86,         0x11307d65a)
#define RVA_SLOT_LOCK_CONTROLLER               ADDR(        0xf07af06,         0x11307d6da)
#define RVA_SLOT_LOCK_AI                       ADDR(        0xf07af46,         0x11307d71a)
#define RVA_SLOT_LOCK_TEAM                     ADDR(        0xf07aec6,         0x11307d69a)
#define RVA_PLAYER_INTS                        ADDR(        0x11c14490,        0x10cce4784)
#define RVA_PLAYER_SHORT                       ADDR(        0x11c14488,        0x10cce477c)
#define RVA_PLAYER_LEADER                      ADDR(        0x11c1448c,        0x10cce4780)
#define RVA_PLAYER_BLESSINGS                   ADDR(        0x11c144c8,        0x10cce47bc)
#define PLAYER_BLESSINGS_MAX                   24
#define RVA_PLAYER_BOLSTERS                    ADDR(        0x11c1bb0c,        0x10ccebe00)
#define RVA_PLAYER_DIVINE_FAVOR                ADDR(        0x11c1bb10,        0x10ccebe04)
#define RVA_PLAYER_INT_A                       RVA_PLAYER_BOLSTERS
#define RVA_PLAYER_INT_B                       RVA_PLAYER_DIVINE_FAVOR
#define RVA_PLAYER_INT_C                       ADDR(        0x11c1bb14,        0x10ccebe08)
#define RVA_PLAYER_COLOURS                     ADDR(        0x1a299b0,         0x1049c81c8)
#define RVA_RESET_SLOT_COLOURS                 ADDR(        0x31cf28,          0x100586030)
#define RVA_CHOOSE_COLOUR_SCREEN               ADDR(        0x088e30,          0x10049050c)
#define RVA_LOAD_PLAYER_2H                     ADDR(        0x185b50,          0x1004f4e14)
#define RVA_SAVE_PLAYER_2H                     ADDR(        0x185810,          MAC_INLINED)
#define RVA_MAP_LINE                           ADDR(        0x1b6040,          0x100512a94)
#define RVA_MAP_FIXEDPLAYER_LOCKS              ADDR(        0x1b6b21,          0x100513764)
#define RVA_RESET_MAP_LOCKS                    ADDR(        0x1adbe0,          MAC_INLINED)
#define RVA_PLAY_GAME_CONTROLLER_DONE          ADDR(        0x09249d,          0x10038bf34)
#define RVA_ELIMINATE_PLAYER                   ADDR(        0x1073d0,          MAC_INLINED)
#define RVA_ELIMINATE_CONTROLLER               ADDR(        0x10740d,          0x100472b60)
#define RVA_ELIMINATE_CONTROLLER_HUMAN         ADDR(        0x107437,          0x100472ba4)
#define RVA_GAME_OVER_WINNER_HUMAN             ADDR(        0x10d476,          0x100473e94)
#define RVA_CONQUER_SQUARE                     ADDR(        0x1aed10,          0x1004aa648)
#define RVA_CONQUER_CONTROLLER_STORE           ADDR(        0x1af010,          0x1004aa830)
#define RVA_SQUARE_IS_CITADEL_FOR              ADDR(        0x0b1710,          0x100362778)
#define RVA_CAN_OWN_SQUARE                     ADDR(        0x331490,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SET_OWNER                   ADDR(        0x3351b0,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_GATHERS_FUNGUS              ADDR(        0x0b1200,          0x100371ce0)
#define RVA_PLAYER_GATHERS_HERBS               ADDR(        0x0b2860,          0x100371e54)
#define RVA_PLAYER_GATHERS_WEED                ADDR(        0x0b1660,          0x100371fc8)
#define RVA_PLAYER_GATHERS_SACRIFICES          ADDR(        0x0b0900,          MAC_INLINED)
#define RVA_PLAYER_GATHERS_HANDS               ADDR(        0x0b1010,          0x1003721bc)
#define RVA_PLAYER_GATHERS_GEMS                ADDR(        0x0b1a60,          MAC_INLINED)
#define RVA_PLAYER_GATHERS_ANY_GEMS            ADDR(        0x0b09b0,          0x100371ac8)
#define RVA_COMMANDER_GATHERS_SACRIFICES       ADDR(        0x08d130,          0x10037155c)
#define RVA_COMMANDER_GATHERS_GEMS             ADDR(        0x08f470,          0x1003717cc)
#define RVA_PLAYER_GATHERS_LIFEFORCE           ADDR(        0x0b0850,          0x100372514)
#define RVA_SERVER_SET_CONTROLLER              ADDR(        0x1f40ea,          0x1004fa81c)
#define RVA_SERVER_SET_CONTROLLER_HUMAN        ADDR(        0x1f4106,          0x1004fa850)
#define RVA_REPLACE_CONTROLLER                 ADDR(        0x1f08e0,          0x1005020a4)
#define RVA_LOAD_PLAYERS_TURN_CONTROLLER       ADDR_DIFFERS(0x187905,          0x1004ec448)
#define RVA_SAVE_GAME_CONTROLLERS              ADDR_DIFFERS(0x1880c1,          0x1004f2bd0)
#define RVA_LOAD_PLAYERS_TEAM                  ADDR(        0x187c9d,          0x1004ecaf0)
#define RVA_SAVE_PLAYERS_TEAM                  ADDR(        0x1881f2,          0x1004f2d30)
#define RVA_PARTICIPANTS_TEAM_WRAP             ADDR(        0x304f08,          0x100589274)
#define RVA_COUNT_SLOTS_WITH_CONTROLLER        ADDR(        0x3032a0,          MAC_NOT_LOOKED_UP)
#define RVA_PARTICIPANTS_ADD_SLOT_BOUND        ADDR(        0x304951,          MAC_NOT_LOOKED_UP)
#define RVA_TEAMLESS_OWN_TEAM                  ADDR(        0x32af90,          0x100511424)
#define RVA_PLAYERCOLOR_STORE                  ADDR(        0x1bc0e8,          0x10052f620)
#define RVA_FILE_READ_FIXED                    ADDR(        0x058580,          0x100337094)
#define RVA_FILE_WRITE_FIXED                   ADDR_DIFFERS(0x058650,          0x1004f3138)
#define RVA_NET_READ_FIXED8                    ADDR(        0x068c90,          MAC_INLINED)
#define RVA_PLAYER_COLOUR_RPC_TEAMMATE         ADDR(        0x30275e,          0x100587188)

// ---- Mods ---------------------------------------------------------------------------------------
#define RVA_MOD_NAMES                          ADDR(        0xeed0260,         0x11307d818)
#define RVA_MOD_FOLDERS                        ADDR(        0xeed0360,         0x11307d918)
#define MOD_RECORD_SIZE                        0xce0
#define MOD_TABLE_MAX                          500
#define MOD_RECORD_ENABLED                     0xcd0
#define RVA_MOD_LAST_INDEX                     ADDR(        0x1a6b9a4,         0x105a0aed4)
#define RVA_EVENT_VARS                         ADDR(        0xf0627e0,         0x10ce02f0c)
#define EVENT_VAR_COUNT                        10000
#define RVA_EVENT_STATE                        ADDR(        0x582edf0,         0x10c127a18)
#define RVA_EVENT_FILE                         ADDR(        0x582ee20,         0x10c398aa8)
#define RVA_RUN_ENABLED_MODS                   ADDR(        0x1cb9a0,          0x10053a1dc)
#define RVA_RUN_ONE_MOD                        ADDR(        0x1bb4c0,          0x100529d40)
#define RVA_MOD_NAME_CHECK                     ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_BAD_MOD_NAME_ERROR                 ADDR(        0x1bb6ac,          0x100529f3c)
#define RVA_FIND_ALL_MODS_RUN_CALL             ADDR(        0x1ca1b7,          0x100333478)
#define RVA_EVENT_RUNNER                       ADDR(        0x1c58b0,          0x10052a648)
#define RVA_RUN_ONE_MOD_EVENT_CALL             ADDR_DIFFERS(0x1bb87c,          0x10052a19c)
#define RVA_EVENT_FILE_EVENT_CALL              ADDR(        0x1cb896,          0x100539f94)
#define RVA_RANDOM_SQUARE_OF_TERRAIN           ADDR(        0x1b1e20,          0x1004a84fc)
#define RVA_RANDOM_RITUAL_SQUARE               ADDR(        0x2ad530,          0x1004b3b40)
#define RVA_EVENT_RANDLOC_CALL                 ADDR(        0x1c73f5,          0x10052c3f4)
#define RVA_MOD_COMMAND                        ADDR(        0x1bbb90,          0x10052ee34)
#define RVA_MOD_ERROR                          ADDR(        0x1c9ae0,          0x100529b60)
#define RVA_MOD_ERROR_MODE                     ADDR_DIFFERS(0x1a9240c,         0x10c063cf0)
#define RVA_FIND_MOD                           ADDR(        0x1bb170,          0x10053a018)
#define RVA_WRITE_CONFIG                       ADDR(        0x193ef0,          0x10032a52c)
#define RVA_PREFS_WRITE_CONFIG                 ADDR(        0x198eb9,          0x10032db88)
#define RVA_MODS_SCREEN_WRITE_CONFIG           ADDR(        0x1ca9e4,          0x10032f14c)
#define RVA_MOD_ERROR_WRITE_CONFIG             ADDR(        0x1c9c30,          0x100529cf0)
#define RVA_READ_CONFIG                        ADDR(        0x1932b0,          0x10032ab44)
#define RVA_READ_CONFIG_ENABLE_MOD             ADDR(        0x19388e,          0x10032b01c)
#define RVA_SDL_MAIN_FIND_ALL_MODS_CALL        ADDR(        0x191201,          MAC_INLINED)
#define RVA_FIND_ALL_MODS                      ADDR(        0x1c9d20,          MAC_INLINED)
#define RVA_ENABLED_MODS_SCREEN                ADDR(        0x1cbb60,          0x10053b788)
#define RVA_ENABLE_SAVE_MODS                   ADDR(        0x188ab0,          MAC_NOT_FOUND)
#define RVA_OLD_WORKSHOP                       ADDR(        0x336b40,          MAC_INLINED)
#define RVA_OLD_WORKSHOP_READ                  ADDR(        0x336eae,          0x1003321ec)
#define RVA_WRITE_STRING                       ADDR(        0x057c90,          0x100337260)
#define RVA_READ_STRING                        ADDR(        0x057b10,          MAC_INLINED)
#define RVA_FONT_FILE                          ADDR(        0x03bef0,          MAC_INLINED)
#define RVA_FONT_FILE_CHECK                    ADDR_DIFFERS(0x1bc378,          0x100538bdc)
#define RVA_FONT_FILE_TTF_CHECK                ADDR(        0x1bc3b4,          0x100538c64)
#define RVA_HAS_EXTENSION                      ADDR(        0x059cf0,          MAC_INLINED)
#define RVA_ADD_TRANSLATION                    ADDR(        0x03d000,          0x10033bb6c)
#define RVA_ADD_FX_NAME                        ADDR(        0x1c8780,          0x100538eb8)
#define RVA_PARSER_ABILITY_WORD                RVA_ADD_FX_NAME
#define RVA_MOD_IMAGE_CHECK                    ADDR(        0x1c93d0,          0x1005388e0)
#define RVA_IMAGE_SIZE_READ                    ADDR(        0x027320,          0x100339ea8)
#define RVA_FILE_CASE_CHECK                    ADDR(        0x05a750,          0x100335bb0)
#define RVA_DIR_CASE_CHECK                     ADDR(        0x1c94b0,          0x100529960)
#define RVA_DIR_NAME_CASE_CHECK                ADDR(        0x05a980,          0x10033596c)
#define RVA_DIR_EXISTS                         ADDR(        0x05c110,          MAC_INLINED)
#define RVA_FILE_EXISTS                        ADDR(        0x05e470,          MAC_INLINED)
#define RVA_DIR_NEXT_ENTRY                     ADDR_DIFFERS(0x00f4b8,          0x1005aedb8)
#define RVA_MODS_DIR                           ADDR(        0x5828160,         0x10c39946d)
#define RVA_WORKSHOP_ITEMS                     ADDR(        0xa0d95e0,         0x1136f61b0)
#define WORKSHOP_ITEM_SIZE                     0x9c6
#define RVA_MODS_CLOSEDIR_CALL                 ADDR(        WIN_NO_CALL,       0x100332894)
#define RVA_MODS_WORKSHOP_FIND_MOD_CALL        ADDR(        0x1c9f03,          0x1003328e8)
#define RVA_LOBBY_MODS_OPENDIR_CALL            ADDR(        0x1c9f83,          0x10033295c)
#define RVA_LOBBY_MODS_CLOSEDIR_CALL           ADDR(        WIN_NO_CALL,       0x1003329e4)
#define RVA_CLOSEDIR                           ADDR(        0x05c170,          0x1005ae428)
#define RVA_PARSER_WORD                        ADDR(        0x9e9df60,         0x10c127a1c)
#define RVA_SQUARE_EVENT_A                     ADDR(        0x9e76e60,         0x10c063d48)
#define SQUARE_EVENT_A_SIZE                    160000
#define RVA_SQUARE_EVENT_B                     ADDR(        0x9e9e7a0,         0x10c08ae48)
#define RVA_EVENT_PLAYER                       ADDR(        0x1a93c2c,         0x101cb6e14)
#define RVA_EVENT_TARGET_X                     ADDR(        0x1a95324,         0x101cb6e20)
#define RVA_EVENT_TARGET_Y                     ADDR(        0x1a92474,         0x101cb6e24)
#define RVA_EVENT_SQUARE_X                     ADDR(        0x1a92974,         0x101cb6e18)
#define RVA_EVENT_SQUARE_Y                     ADDR(        0x1a929c4,         0x101cb6e1c)
#define RVA_EVENT_PLAYER_ARG                   ADDR(        0x1c86d0,          0x100539a4c)
#define RVA_EVENT_INT_ARG                      ADDR(        0x1b7320,          0x100512954)
#define RVA_EVENT_TEXT_ARG                     ADDR(        0x1b71f0,          0x1005129d8)
#define RVA_FIND_UNIT_AT                       ADDR(        0x105b40,          0x1004808a0)
#define RVA_FIND_ITEM_HOLDER_AT                ADDR(        0x17e560,          0x1004e9a38)
#define RVA_FIND_UNIT_AT_END                   ADDR(        0x105cdb,          0x100480d94)
#define RVA_FIND_ITEM_HOLDER_AT_END            ADDR(        0x17e723,          0x1004ea244)
#define RVA_ARMY_AT_FOR_PLAYER                 ADDR(        0x104650,          MAC_INLINED)
#define RVA_SAME_TEAM                          ADDR(        0x32b970,          MAC_INLINED)
#define RVA_EVENT_HASUNIT_CALL                 ADDR(        0x1c65aa,          0x10052dc9c)
#define RVA_EVENT_HASUNITHERE_CALL             ADDR(        0x1c66a0,          0x10052de60)
#define RVA_EVENT_HASITEM_CALL                 ADDR(        0x1c6a7e,          0x10052e464)
#define RVA_EVENT_HASITEMHERE_CALL             ADDR(        0x1c6b40,          0x10052e574)
#define RVA_COUNT_UNITS                        ADDR(        0x104e50,          0x100481b4c)
#define RVA_SQUARE_OWNER_AT                    ADDR(        0x335850,          MAC_INLINED)
#define RVA_ARMY_OWNER_AT                      ADDR(        0x129bf0,          MAC_INLINED)
#define RVA_SQUARE_ARMY_AT                     ADDR(        0x129720,          MAC_INLINED)
#define RVA_SQUARE_ARMY                        ADDR(        0xfb3b760,         0x1120c3fc0)
#define RVA_MOD_LINE                           ADDR(        0x1a957c4,         0x101cce558)
#define RVA_EVENT_FILE_RUNNER                  ADDR(        0x1cb690,          0x100539cb0)
#define RVA_WORKSHOP_LAST                      ADDR(        0x1b186ec,         0x105a0aee0)
#define RVA_EVENT_KILL_SQUARE                  ADDR(        0x118fa0,          0x100481aa8)
#define RVA_PROMOTE_UNITS_AT                   ADDR(        0x10f5a0,          0x100471708)
#define RVA_PROMOTE_UNIT                       ADDR(        0x10f790,          0x10046d6c8)
#define RVA_DAMAGE_UNIT                        ADDR(        0x0bd880,          0x1003b8fc8)
#define RVA_REMOVE_UNIT                        ADDR(        0x0f8bf0,          0x1003b8c88)
#define RVA_GIVE_ITEM_BY_NAME                  ADDR(        0x183cd0,          0x1004e57cc)
#define RVA_TAKE_ITEM_BY_NAME                  ADDR(        0x180660,          0x1004e58e8)
#define RVA_GIVE_RANDOM_ITEM                   ADDR(        0x183d10,          0x1004e5e8c)
#define RVA_RANDOM_ITEM_PICK                   ADDR(        0x180180,          0x1004e5a94)
#define RVA_EVENT_FILE_COPY                    ADDR(        WIN_INLINED,       0x100529e98)
#define RVA_EVENT_FILE_CLEAR                   ADDR(        0x1cb954,          0x100539fc0)
#define RVA_EVENT_STATE_START                  ADDR(        0x1c594f,          0x10052a7e8)
#define RVA_EVENT_STATE_SQUARE                 ADDR(        0x1c5a3f,          0x10052a864)
#define RVA_EVENT_STATE_END                    ADDR(        0x1c5a78,          0x10052a898)
#define RVA_EVENT_STATE_FAIL                   ADDR(        0x1c84fe,          0x10052bf1c)
#define RVA_EVENT_STATE_CLEAR                  ADDR(        0x1bb8b4,          0x10052a1d0)
#define RVA_RUN_MODS_SELF_CALL                 ADDR(        0x1cbadb,          0x10053a504)
#define RVA_PLAY_GAME_START_RUN_MODS_CALL      ADDR(        0x091f9c,          0x100389d88)
#define RVA_PLAY_GAME_END_RUN_MODS_CALL        ADDR(        0x094070,          0x100391658)
#define RVA_GAME_END_FN_RUN_MODS_CALL          ADDR(        0x094391,          MAC_NOT_FOUND)
#define RVA_TURN_START_RUN_MODS_CALL           ADDR(        0x167d72,          0x100392938)
#define RVA_LOAD_GAME_RUN_MODS_CALL            ADDR(        0x186140,          0x1004eb860)
#define RVA_STARTUP_RUN_MODS_CALL              ADDR(        0x191299,          0x100332b00)
#define RVA_MODS_SCREEN_RESET_CALL             ADDR(        0x1ca9d7,          0x10032f140)
#define RVA_MODS_SCREEN_RUN_MODS_CALL          ADDR(        0x1ca9df,          0x10032f148)
#define RVA_MODS_OPENDIR_CALL                  ADDR(        0x1c9da2,          0x100332804)
#define RVA_MOD_NAME_COMPARE                   ADDR(        0x05c1c0,          MAC_INLINED)
#define RVA_MOD_TABLE_SORT                     ADDR(        0x1ca0b0,          0x1003329e8)
#define RVA_LINE_NUMBER_ARG                    ADDR(        0x1b7320,          0x100512954)
#define RVA_PARSER_MODVERSION                  ADDR(        0x1bbe2b,          0x10052efc4)
#define RVA_PARSER_MODPRIO                     ADDR(        0x1bbe87,          0x10052f398)
#define RVA_PARSER_CLEAR_CURRENT               ADDR(        0x1c5860,          MAC_INLINED)
#define RVA_ADD_MOD_FILE                       ADDR_DIFFERS(0x1c8e50,          0x1005389c0)
#define RVA_MOD_FILE_LIST                      ADDR(        0xe9ee260,         0x11320fda0)
#define MOD_FILE_ENTRY_SIZE                    0x200
#define RVA_MOD_FILE_LAST_INDEX                ADDR(        0x1a9507c,         0x105a0aed8)
#define RVA_MOD_FILE_BYTES                     ADDR(        0x582edf4,         0x11320fd98)
#define RVA_MOD_CHECKSUMS                      ADDR(        0xeed0f34,         0x11307e4ec)
#define RVA_MOD_CRC                            ADDR(        0x1cabd0,          MAC_INLINED)
#define RVA_LOAD_MOD_LIST                      ADDR(        0x188ab0,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST                      ADDR(        0x188dc0,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST_NAME_WRITE           ADDR(        0x188e36,          MAC_NOT_FOUND)
#define RVA_SAVE_MOD_LIST_CRC_WRITE            ADDR(        0x188e78,          MAC_NOT_FOUND)
#define RVA_MOD_CRC_FIRST_TURN                 ADDR(        0x1a3b2ac,         0x100872c68)
#define RVA_MOD_CRC_WARNED                     ADDR_DIFFERS(0x5822fb4,         0x1085ba7b0)
#define RVA_LOAD_GAME_MOD_CRC_CALL             ADDR(        0x188d1e,          MAC_NO_CALL)
#define RVA_SAVE_GAME_MOD_CRC_CALL             ADDR(        0x18ec5c,          MAC_NO_CALL)
#define RVA_SAVE_MOD_LIST_MOD_CRC_CALL         ADDR(        0x188e43,          MAC_NOT_FOUND)
#define RVA_LOAD_MOD_LIST_MOD_CRC_CALL         ADDR(        0x188d1e,          MAC_NOT_FOUND)
#define RVA_LOAD_GAME_FOPEN_CALL               ADDR(        0x185f7f,          0x1004eb478)
#define RVA_LOAD_GAME_MOD_NAME_READ            ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_LOAD_GAME_MOD_NAME_READ_NEXT       ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_LOAD_GAME_MOD_CRC_READ             ADDR_DIFFERS(0x188c46,          0x1004eb6e8)
#define LOAD_GAME_MOD_NAME_AFTER_CRC           4
#define LOAD_MOD_LIST_NAME_AFTER_CRC           0x10
#define RVA_WARN_FORMAT                        ADDR(        0x042f40,          0x1003482ac)
#define RVA_LOAD_GAME_MOD_DIFFERS_CALL         ADDR(        0x188d35,          0x1004eb814)
#define RVA_LOAD_GAME_MOD_WARNED_SET           ADDR_DIFFERS(0x188d3a,          0x1004eb81c)
#define RVA_LOAD_GAME_RESET_MODS_CALL          ADDR(        0x18610d,          0x1004eb828)
#define SAVE_VERSION_MOD_CHECKSUMS             0xc3
#define RVA_LOAD_GAME_MOD_NOT_FOUND            ADDR(        0x188d8c,          0x1004ee5c0)
#define RVA_LOAD_GAME_MOD_FLAGS_CLEAR          ADDR(        0x188b00,          0x1004eb5d8)
#define RVA_LOAD_GAME_MOD_ENABLE               ADDR(        0x188cdf,          0x1004eb744)
#define RVA_SAVE_GAME_MOD_NAME_WRITE           ADDR(        0x18ec4e,          0x1004f218c)
#define RVA_SAVE_GAME_MOD_CRC_WRITE            ADDR(        0x18ec8c,          0x1004f2154)
#define RVA_SAVE_GAME_MOD_LIST_END             ADDR(        WIN_INLINED,       0x1004f221c)

// ---- Game data tables ---------------------------------------------------------------------------
#define RVA_MONSTER_TABLE                      ADDR(        0x1c652c0,         0x104b515f8)
#define RVA_MONSTER_COUNT                      ADDR(        0x1aa61bc,         0x10487ff28)
#define RVA_MONSTER_CURRENT                    ADDR(        0x1a9299c,         0x101cb6e30)
#define RVA_MONSTER_SPRITES                    ADDR(        0x1c652f0,         0x104b51628)
#define MONSTER_SIZE                           0x9d8
#define MONSTER_NAME_SIZE                      0x30
#define MONSTER_TABLE_MAX                      6000
#define MONSTER_TABLE_BYTES                    0xe6b680
#define RVA_MONSTER_BACKUP_SIZE                ADDR(        0x1910e3,          MAC_NOT_LOOKED_UP)
#define RVA_MONSTER_BACKUP_COPY_SIZE           ADDR(        0x1910ed,          MAC_NOT_LOOKED_UP)
#define RVA_MONSTER_RESTORE_SIZE               ADDR(        0x1cad52,          MAC_NOT_LOOKED_UP)
#define RVA_NEWMONSTER_CAP                     ADDR(        0x1bc567,          0x10052f8c0)
#define RVA_FIND_MONSTER_TABLE_REF             ADDR(        0x1da4bb,          0x100491270)
#define RVA_ARMY_LIST_COUNTS                   ADDR(        0x11f430e0,        0x106cdfa04)
#define RVA_ARMY_LIST_UNITS                    ADDR(        0x11f991e0,        0x106ce57c4)
#define RVA_ARMY_LIST_LEAST_A                  ADDR(        0x11f97a60,        0x106ceb584)
#define RVA_ARMY_LIST_LEAST_B                  ADDR(        0x11f41960,        0x106ceccf4)
#define RVA_MONSTER_SPRITE_LABELS              ADDR(        0x582f830,         0x10b8494c4)
#define RVA_MONSTER_SPRITE_LABELS_FILL_BOUND   ADDR(        0x1da1c3,          0x100490c90)
#define RVA_BATTLE_SUMMONS                     ADDR(        0x12335900,        0x107ee6496)
#define ARMY_LIST_COUNT_SIZE                   0x4
#define ARMY_LIST_LEAST_SIZE                   0x1
#define BATTLE_SUMMONS_SIZE                    0x2
#define MONSTER_NAME_SIZE                      0x30
#define RVA_MONSTER_FLAGS                      ADDR(        0x1c65490,         0x104b517c8)
#define MONSTER_FLAG_NOT_ARMY                  0x400000000
#define MONSTER_MOVE_FLAGS                     0x1c8
#define ITEM_GROUP_MASK                        0x188
#define ITEM_MOVE_FLAGS                        0x58
#define ITEM_ABILITIES                         0x60
#define ITEM_ABILITY_PAIRS                     20
#define ITEM_FLAG_CURSED                       0x400000000000
#define RVA_PARSER_MOVE_WORD                   ADDR(        0x1c89c0,          0x10053962c)
#define RVA_PARSER_FLAG_WORD                   ADDR(        0x1c8d50,          0x100538dcc)
#define RVA_PARSER_TERRAIN_FLAG_WORD           ADDR(        0x1c8c20,          0x100539734)
#define RVA_PARSER_WEAPON_FLAG_WORD            ADDR(        0x1c8cf0,          0x1005395cc)
#define RVA_PARSER_RITUAL_FLAG_WORD            ADDR(        0x1c8c90,          0x1005396d4)
#define RVA_PARSER_TERRAIN_ABILITY_WORD        ADDR(        0x1c8ac0,          0x1005397b0)
#define RVA_SOULLESS_FORM                      ADDR(        0x1da2d0,          0x100490f88)
#define RVA_UNIT_TRAIT_LIST                    ADDR(        0x1d19f0,          0x10051cc50)
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
#define RVA_FIND_MONSTER                       ADDR(        0x1da4e0,          0x1004911b8)
#define MONSTER_ABILITIES                      0x1d8
#define MONSTER_ABILITY_PAIRS                  256
#define MONSTER_ABILITY_POWER_BASE             0x1f4
#define MONSTER_ABILITY_STARTITEM              0x4a
#define PARSER_STARTITEM_MAX                   4
#define RVA_PARSER_STARTITEM_COUNT             ADDR(        0x582ee14,         MAC_NOT_LOOKED_UP)
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
#define RVA_MONSTER_ABILITY                    ADDR(        0x1299d0,          MAC_INLINED)
#define RVA_NAME_COMPARE                       ADDR_DIFFERS(0x0579e0,          0x1005aeef0)
#define RVA_UNIT_SPAWN_OFFSET_READ             ADDR(        0x1455c8,          0x10039c89c)
#define RVA_UNIT_GROW_OFFSET_READ              ADDR(        0x145991,          0x10039d2dc)
#define RVA_WEAPON_TABLE                       ADDR(        0x57007d0,         0x104904cb0)
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
#define RVA_WEAPON_FREE_SLOT                   ADDR(        0x0b8aa0,          0x1003f82a8)
#define RVA_WEAPON_CURRENT                     ADDR(        0x1a93fe4,         0x101cb6e34)
#define WEAPON_RECORDS_GAME                    5000
#define RVA_WEAPON_FREE_SLOT_CAP               ADDR(        0x0b8ae4,          0x1003f82ec)
#define RVA_WEAPON_OUT_OF_WEAPONS_CALL         ADDR(        0x0b8af5,          0x1003f830c)
#define RVA_FIND_WEAPON_TABLE_REF              ADDR(        0x0b8d3b,          0x1003f802c)
#define RVA_SELECTWEAPON_BOUND                 ADDR(        0x1bf0f5,          0x1005325f0)
#define RVA_NEXTWEP_BOUND                      ADDR(        0x1bf341,          0x10053273c)
#define RVA_NEXTALWAYSWEP_BOUND                ADDR(        0x1bf401,          0x100532820)
#define RVA_ITEMWEP_BOUND                      ADDR(        0x1c1301,          0x1005343a4)
#define RVA_WEAPON_NAME_LINE_BOUND             ADDR(        0x1d006b,          0x100526de8)
#define RVA_WEAPON_CLEAR_SIZE                  ADDR(        0x165167,          0x1003aae0c)
#define RVA_WEAPON_BACKUP_SIZE                 ADDR(        0x191155,          0x100332718)
#define RVA_WEAPON_BACKUP_COPY_SIZE            ADDR(        0x19115f,          0x100332734)
#define RVA_ITEM_BACKUP_SIZE                   ADDR(        0x1911db,          0x1003327a0)
#define RVA_ITEM_BACKUP_COPY_SIZE              ADDR(        0x1911e5,          0x1003327bc)
#define RVA_ITEM_RESTORE_SIZE                  ADDR(        0x1cae06,          0x100528e48)
#define RVA_WEAPON_RESTORE_SIZE                ADDR(        0x1cadac,          0x100528de8)
#define RVA_WEAPON_HIT                         ADDR(        0x0dd1a0,          0x1003c2198)
#define RVA_WEAPON_HIT_MULTIPLY                ADDR_DIFFERS(0x0dda33,          0x1003c2eec)
#define RVA_WEAPON_HIT_DAMAGE_CHECK            ADDR_DIFFERS(0x0dda38,          0x1003c2ef0)
#define RVA_WEAPON_HIT_DAMAGE_GO_ON            ADDR(        0x0dda41,          0x1003c2ef4)
#define RVA_WEAPON_HIT_NO_DAMAGE               ADDR(        0x0ddbf9,          MAC_IN_CODE)
#define RVA_WEAPON_HIT_MULTIPLY_END            ADDR(        0x0dda79,          0x1003c2f20)
#define RVA_WEAPON_HIT_PROLOGUE_END            ADDR(        0x0dd1bc,          0x1003c220c)
#define RVA_WEAPON_HIT_DAMAGE_DIE_CALL         ADDR(        0x0dd44f,          0x1003c2790)
#define RVA_BATTLE_LOG                         ADDR(        0x100dc0,          0x1005818dc)
#define RVA_WEAPON_TEXT                        ADDR(        0x1ccff0,          0x1005275d0)
#define RVA_WEAPON_TEXT_CALL                   ADDR(        0x1d18e2,          0x100527480)
#define RVA_WEAPON_TEXT_NEXT_CALL              ADDR(        0x1d19c3,          0x1005274dc)
#define RVA_BATTLE_TARGET_USEFUL_CALL          ADDR(        0x0d6b19,          0x1003c0780)
#define RVA_SPELL_USEFUL_TARGET_CALL           ADDR_DIFFERS(0x2e5bd8,          0x1004bf988)
#define RVA_ITEM_TABLE                         ADDR(        0x1be6730,         0x1048937f8)
#define ITEM_SIZE                              0x1d0
#define ITEM_NAME_SIZE                         0x30
#define ITEM_TABLE_MAX                         1000
#define RVA_ITEM_COUNT                         ADDR(        0x1a3b04c,         0x101cce550)
#define RVA_NEWITEM_CAP                        ADDR(        0x1c0cf1,          0x100533fa8)
#define RVA_GROUND_ITEMS_LOAD_CHECK            ADDR(        0x18670f,          0x1004ebf48)
#define RVA_GROUND_ITEMS_LOAD_CHECK_2          ADDR(        0x18a8a1,          MAC_NOT_FOUND)
#define RVA_FIND_ITEM_TABLE_REF                ADDR(        0x185412,          0x1004e5894)
#define RVA_ITEM_CURRENT                       ADDR(        0x1a92454,         0x101cb6e2c)
#define RVA_ITEM_FLAGS                         ADDR(        0x1be6770,         0x104893838)
#define RVA_ITEM_STATE_FLAGS                   ADDR(        0x1be6778,         0x104893840)
#define RVA_CLASS_TABLE                        ADDR(        0x18a0710,         0x1049c8358)
#define RVA_CLASS_END_SCAN_STOP                ADDR(        0x0b07da,          0x100529230)
#define RVA_CLASS_END_SCAN_STOP_2              ADDR(        0x190ef8,          0x1003312a4)
#define RVA_CLASS_END_SCAN_STOP_3              ADDR(        0x0b07da,          0x100391614)
#define CLASS_SIZE                             0x1f74
#define CLASS_NAME_SIZE                        0x40
#define CLASS_TABLE_MAX                        200
#define RVA_CLASS_COUNT                        ADDR(        0x43d0d4,          0x1049c8354)
#define RVA_CLASS_LEADERS                      ADDR(        0x18a0750,         0x1049c8398)
#define RVA_CLASS_SIDE_A                       ADDR(        0x45cf10,          0x1008735d8)
#define RVA_CLASS_SIDE_B                       ADDR(        0x445810,          0x101cb6e50)
#define RVA_CLASS_CURRENT                      ADDR(        0x1a959a4,         0x101cb6e44)
#define RVA_CLASS_LAST_RECRUIT                 ADDR(        0x1a93dd4,         0x101cb6e48)
#define RVA_RECLIMITER_COUNT                   ADDR(        0x582f7e4,         0x10c127a8c)
#define CLASS_HEROES_SIZE                      0x1e0
#define RVA_WESTEAST_TEAM_ANGLES               ADDR(        0x62240e0,         0x1069b07a0)
#define RVA_CLASS_TABLE_COPY                   ADDR(        0x6224400,         0x1068274f4)
#define RVA_NEWCLASS_CAP                       ADDR(        0x1bda27,          0x100530c70)
#define RVA_CLASS_BACKUP_SIZE                  ADDR(        0x191109,          0x1003326c8)
#define RVA_CLASS_RECRUITS_BACKUP_SIZE         ADDR(        0x19112f,          0x1003326f0)
#define RVA_CLASS_RESTORE_SIZE                 ADDR(        0x1cad70,          0x100528da8)
#define RVA_CLASS_RECRUITS_RESTORE_SIZE        ADDR(        0x1cad8e,          0x100528dc8)
#define RVA_CLASS_COPY_SIZE_1                  ADDR(        0x094043,          MAC_NOT_LOOKED_UP)
#define RVA_CLASS_COPY_SIZE_2                  ADDR(        0x094364,          MAC_NOT_LOOKED_UP)
#define RVA_CLASS_COPY_SIZE_3                  ADDR(        0x0b0794,          MAC_NOT_LOOKED_UP)
#define RVA_CLASS_COPY_SIZE_4                  ADDR(        0x0b0807,          MAC_NOT_LOOKED_UP)
#define RVA_CLASS_COPY_SIZE_5                  ADDR(        0x190f49,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_CLASS                         ADDR(        0x089b70,          0x100391e48)
#define RVA_FIND_CLASS_TABLE_REF               ADDR(        0x089b87,          0x100391e78)
#define RVA_CLASS_PICKER                       ADDR(        0x302870,          0x100586200)
#define RVA_CLASS_PICKER_INFO_ID               ADDR(        0x302b8d,          0x100586504)
#define RVA_CLASS_PICKER_DECODE                ADDR(        0x302e8b,          0x100586678)
#define RVA_CLASS_PICKER_RETURN                ADDR(        0x302edd,          0x1005866a4)
#define RVA_CLASS_PICKER_INFO_CALL             ADDR(        0x302ed3,          0x1005862b8)
#define RVA_CLASS_PICKER_LOOP                  ADDR(        0x302910,          0x1005862c0)
#define CLASS_HOME_TERRAIN                     0x70
#define CLASS_START_UNITS                      0x84
#define CLASS_START_UNITS_MAX                  16
#define CLASS_START_UNIT_SIZE                  0x40
#define CLASS_ABILITIES                        0x484
#define CLASS_ABILITY_PAIRS                    16
#define CLASS_ABILITY_STDTROOPS                0xde
#define CLASS_ABILITY_AI_CLASS                 0xcb
#define CLASS_ABILITY_STARTPLANE               0xeb
#define CLASS_ABILITY_ASSASSIN_CHANCE          0xe1
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
#define RECRUIT_LIMITER_SIZE                   0x30
#define RECRUIT_TERRAIN                        0x294
#define CLASS_HERO_SIZE                        0x78
#define CLASS_HERO_NAME                        0x30
#define RVA_CLASS_ADD_RECRUIT                  ADDR(        0x0b0560,          0x10036140c)
#define RVA_CLASS_ADD_START_UNIT               ADDR(        0x0b0450,          0x100361508)
#define RVA_CLASS_ABILITY                      ADDR(        0x0b03e0,          0x100361778)
#define RVA_CLASS_SET_ABILITY                  ADDR(        0x0b0230,          0x100361cf0)
#define RVA_CLASS_CLEAR_ABILITY                ADDR_DIFFERS(0x0b02f0,          0x100361a80)
#define RVA_CLASS_PRINT_ALL_RECRUITS           ADDR(        0x0b5b20,          0x1004c1c30)
#define RVA_CLASS_PRINT_RECRUITS               ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_HERO_ARRIVAL                       ADDR(        0x08a000,          MAC_INLINED)
#define RVA_TERRAIN_TABLE                      ADDR(        0x56aeaf0,         0x1059bcc90)
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
#define TERRAIN_ABILITIES                      0x58
#define TERRAIN_ABILITY_PAIRS                  4
#define TERRAIN_FLAGS                          0x50
#define TERRAIN_FLAG_PORT                      0x400000000000000
#define TERRAIN_FLAG_SIGHT_SQUARE_ONLY         0x2000000
#define TERRAIN_FLAG_SIGHT_FARSIGHT            0x1000000
#define TERRAIN_FLAG_SEEN_NEAR                 0x10000000000000
#define TERRAIN_FLAG_SEEPAST                   0x4000000000000000
#define TERRAIN_APCOST                         0x46
#define RVA_TERRAIN_CURRENT                  ADDR(        0x1a95d24,         0x101cb6e3c)
#define RVA_TERRAIN_GROUP_NAMES                ADDR(        0xd4a65e0,         0x10c81c184)
#define RVA_TERRAIN_GROUP_MEMBERS              ADDR(        0xd4ae2e0,         0x10c823e84)
#define RVA_TERRAIN_GROUP_CURRENT              ADDR(        0x1a9242c,         0x101cb6e40)
#define RVA_BUILTIN_GROUP_NAMES                ADDR(        0x405b00,          0x100847ed8)
#define RVA_RITUAL_TABLE                       ADDR(        0x2ae3000,         0x101cce668)
#define RVA_RITUAL_CODE_FIRST                  ADDR(        0x2ae2f0,          MAC_FILE_FORMAT)
#define RVA_RITUAL_CODE_LAST                   ADDR(        0x2ce2f0,          MAC_FILE_FORMAT)
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
#define RVA_RITUAL_COUNT                       ADDR(        0x1afe774,         0x101cce58c)
#define RVA_RITPOW_COUNT                       ADDR(        0x1afe48c,         0x101cce594)
#define RVA_RITUAL_CURRENT                     ADDR(        0x1a92494,         0x101cb6e38)
#define RVA_FIND_RITUAL                        ADDR(        0x2be690,          MAC_INLINED)
#define RVA_ALLOC_RITUAL                       ADDR(        0x2be760,          0x10056725c)
#define RVA_ALLOC_RITUAL_TABLE_REF             ADDR_DIFFERS(0x2be788,          0x100567298)

#define RVA_INIT_RITUALS                       ADDR(        0x2bed60,          MAC_INLINED)
#define RVA_INIT_RITUALS_BOUND                 ADDR_DIFFERS(0x190f84,          0x100331330)
#define RVA_INIT_RITUALS_BOUND_RESET_MODS      ADDR_DIFFERS(0x2bed7f,          0x1005292bc)
#define RVA_INIT_RITUALS_BOUND_3               ADDR_DIFFERS(0x2bed7f,          0x1005530a8)
#define RVA_INIT_RITUALS_BOUND_RESET_STATE     ADDR_DIFFERS(0x2bed7f,          0x100585c8c)
#define RVA_INIT_RITUALS_NO_END_CALL           ADDR(        0x2bedbe,          0x100331340)
#define RVA_RITUAL_ADD_FX                      ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_RITUAL_ADD_FX_BOUND                ADDR(        0x1c8861,          0x1005390c8)
#define RVA_RITUAL_ADD_STRING                  ADDR(        0x2be8e0,          0x100566b04)
#define RVA_RITUAL_ADD_STRING_BOUND            ADDR(        0x2be8ee,          0x100566b14)
#define RVA_ALLOC_RITUAL_BOUND                 ADDR_DIFFERS(0x2be776,          0x100567280)
#define RVA_ALLOC_RITUAL_FAIL_BRANCH           ADDR_DIFFERS(0x2be77b,          0x100567284)
#define RVA_OUT_OF_RITUALS_CALL                ADDR(        0x1bf94b,          0x100532eb8)
#define RVA_MOD_COMMAND_EXIT                   ADDR(        0x1c5823,          0x10052ee80)
#define RVA_RANDOM_RITUAL                      ADDR(        0x2be140,          MAC_INLINED)
#define RVA_RANDOM_RITUAL_BOUND                ADDR(        0x2be19a,          MAC_INLINED)
#define RVA_RANDOM_RITUAL_1A_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x1005533c0)
#define RVA_RANDOM_RITUAL_1A_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x1005534a0)
#define RVA_RANDOM_RITUAL_1B_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x100553538)
#define RVA_RANDOM_RITUAL_1B_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x100553600)
#define RVA_RANDOM_RITUAL_2A_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x100553e74)
#define RVA_RANDOM_RITUAL_2A_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x100553f44)
#define RVA_RANDOM_RITUAL_2B_ENTRY             ADDR(        WIN_NO_EQUIVALENT, 0x100553ffc)
#define RVA_RANDOM_RITUAL_2B_RESUME            ADDR(        WIN_NO_EQUIVALENT, 0x1005540c4)
#define RVA_RANDOM_RITUAL_3_ENTRY              ADDR(        WIN_NO_EQUIVALENT, 0x100554ce0)
#define RVA_RANDOM_RITUAL_3_RESUME             ADDR(        WIN_NO_EQUIVALENT, 0x100554de0)
#define RVA_EXECUTE_RITUAL                     ADDR(        0x2c5ce0,          0x10055d8b4)
#define RVA_RITUAL_FX_RANDOM_EQUIP             ADDR_DIFFERS(0x2c9a76,          0x100562d40)
#define RVA_RITUAL_FX_MAKE_ITEM                ADDR_DIFFERS(0x2c97ff,          0x100562b14)
#define RVA_EXECUTE_RITUAL_BOUND               ADDR(        0x2c5db6,          0x10055d920)
#define RVA_RITUAL_PR_BOUND                    ADDR(        0x2c2b03,          0x100558b1c)
#define RVA_RITUAL_CPR_BOUND                   ADDR(        0x2c3924,          0x100558700)
#define RVA_UNIT_RITUAL_LIST_BOUND             ADDR(        0x1d7c5b,          MAC_NOT_FOUND)
#define RVA_RITUAL_SCREEN_LOOP_1               ADDR(        0x2aec6a,          0x10035f8ec)
#define RVA_RITUAL_SCREEN_LOOP_2               ADDR(        0x2b0172,          0x10035fbf8)
#define RVA_RITUAL_ROW_ID_GREYED               ADDR_DIFFERS(0x2b00f5,          0x1003603d4)
#define RVA_RITUAL_ROW_ID_GREYED_2             ADDR_DIFFERS(0x2b00f5,          0x100360438)
#define RVA_RITUAL_ROW_ID_GREYED_3             ADDR_DIFFERS(0x2b00f5,          0x100360490)
#define RVA_RITUAL_ROW_ID_GREYED_4             ADDR_DIFFERS(0x2b00f5,          0x100360634)
#define RVA_RITUAL_ROW_ID_GREYED_5             ADDR_DIFFERS(0x2b00f5,          0x10036053c)
#define RVA_RITUAL_ROW_ID_GREYED_6             ADDR_DIFFERS(0x2b00f5,          0x100360570)
#define RVA_RITUAL_ROW_ID_GREYED_7             ADDR_DIFFERS(0x2b00f5,          0x1003605c4)
#define RVA_RITUAL_ROW_ID_CASTABLE             ADDR_DIFFERS(0x2b0367,          0x100360690)
#define RVA_RITUAL_ROW_ID_JOIN                 ADDR(        WIN_NO_EQUIVALENT, 0x1003604a4)
#define RVA_RITUAL_CLICK_DECODE                ADDR(        0x2b063d,          0x100361088)
#define RVA_RITUAL_RIGHT_CLICK_DECODE          ADDR(        0x2b088d,          0x100360978)
#define RVA_RITUAL_CLICK_CASTABLE_BOUND        ADDR(        0x2b064a,          0x10036108c)
#define RVA_RITUAL_RIGHT_CLICK_BAND            ADDR_DIFFERS(0x2b0893,          0x100360980)
#define RVA_RITUAL_RIGHT_CLICK_GREYED          ADDR_DIFFERS(0x2b089b,          0x100360984)
#define RVA_BACKUP_TABLES                      ADDR(        0x1caa30,          MAC_INLINED)
#define RVA_RITUAL_BACKUP                      ADDR(        0x582f7f0,         0x10c063d20)
#define RVA_RITUAL_BACKUP_SIZE                 ADDR(        0x19117b,          0x100332740)
#define RVA_RITUAL_BACKUP_COPY_SIZE            ADDR(        0x191185,          0x10033275c)
#define RVA_RITUAL_RESTORE_SIZE                ADDR(        0x1cadca,          0x100528e08)
#define RVA_CHECK_VANILLA_NAMES                ADDR(        0x1165f0,          MAC_INLINED)
#define RVA_TURN_CAST_COUNTS                   ADDR(        0x2c0150,          MAC_INLINED)
#define RVA_TURN_CAST_COUNTS_REF               ADDR_DIFFERS(0x16a074,          0x1003999cc)
#define RVA_LOAD_2H_CAST_COUNTS                ADDR_DIFFERS(0x188729,          0x1004f5130)
#define RVA_LOAD_GAME_CAST_COUNTS              ADDR_DIFFERS(0x187d5f,          0x1004ecb78)
#define RVA_LOAD_PLAYERS                       ADDR(        0x187720,          MAC_INLINED)
#define RVA_EXECUTE_RITUAL_CAST_COUNTS         ADDR_DIFFERS(0x2c72df,          0x10055efec)
#define RVA_SAVE_GAME_CAST_COUNT_READ          ADDR_DIFFERS(0x1882a6,          0x1004f2dd4)
#define RVA_SAVE_2H_CAST_COUNT_READ            ADDR_DIFFERS(0x188990,          0x10050adfc)
#define RVA_CAST_STRIDE_AI_MAXCAST             ADDR(        0x06d1f7,          0x1004df914)
#define RVA_CAST_STRIDE_EXECUTE_ADD            ADDR_DIFFERS(0x2c72d5,          0x10055efd4)
#define RVA_CAST_STRIDE_EXECUTE_INC            ADDR(        0x2c9e6c,          MAC_STORED_DIFFERENTLY)
#define RVA_CAST_STRIDE_CAN_CAST               ADDR(        0x2ca6c6,          0x10055c234)
#define RVA_CAST_STRIDE_FX_40E                 ADDR(        0x2b6891,          0x1005577e0)
#define RVA_CAST_STRIDE_TURN                   ADDR(        0x16a06d,          0x1003999c8)
#define RVA_CAST_STRIDE_TURN_PLAYER_29         ADDR(        WIN_NO_EQUIVALENT, 0x100392c4c)
#define RVA_SAVE_GAME_END_MARKER               ADDR(        0x18fae0,          0x1004f4a88)
#define RVA_SAVE_GAME_FCLOSE                   ADDR(        0x18faf0,          0x1004f4a9c)
#define RVA_SERVER_SAVE_END_MARKER             ADDR(        0x1f4965,          MAC_NO_CALL)
#define RVA_SAVE_2H_FCLOSE                     ADDR_DIFFERS(0x185a76,          0x10050ae64)
#define RVA_LOAD_GAME_END_MARKER               ADDR(        0x187298,          0x1004ee1e0)
#define RVA_SAVE_2H_END_MARKER                 ADDR_DIFFERS(0x185a54,          0x10050ae50)
#define RVA_LOAD_2H_END_MARKER                 ADDR(        0x185e16,          0x1004f518c)
#define RVA_LOAD_2H_CAST_COUNTS_CALL           ADDR(        0x188732,          0x1004f5138)
#define RVA_SAVE_2H_CAST_COUNTS_CALL           ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_SAVE_GAME_CAST_COUNTS_CALL         ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_LOAD_GAME_CAST_COUNTS_CALL         ADDR(        0x187d65,          0x1004ecb80)
#define RVA_SAVE_PLAYER_BLOCK                  ADDR(        0x188780,          MAC_INLINED)
#define RVA_SAVE_PLAYER_BLOCK_CALL             ADDR(        0x185a38,          MAC_INLINED)
#define RVA_LOAD_PLAYER_BLOCK                  ADDR(        0x1884f0,          MAC_INLINED)
#define RVA_RESET_PLAYERS_CLEAR                ADDR_DIFFERS(0x31ce27,          0x100585e1c)
#define RVA_KEY_CHECK_PLAYERS_CLEAR            ADDR_DIFFERS(0x143a2c,          0x1003b56d8)
#define RVA_COMPACT_PLAYERS_MOVE               ADDR_DIFFERS(0x1f0bf3,          0x1004f8da8)
#define RVA_COMPACT_PLAYERS_MOVE_2             ADDR_DIFFERS(0x1f0bf3,          0x1004f8da8)
#define RVA_ENCHANTS                           ADDR(        0xd49c9a0,         0x10c9180c4)
#define ENCHANT_SIZE                           0x28
#define ENCHANT_KIND                           0x0
#define ENCHANT_CASTER                         0x4
#define ENCHANT_OWNER                          0x8
#define ENCHANT_RITUAL                         0xc
#define ENCHANT_X                              0x10
#define ENCHANT_Y                              0x14
#define ENCHANT_KIND_ILLUSION                  0x1d
#define ENCHANT_KIND_GUILD_TRADERS             31
#define ENCHANT_KIND_GUILD_MERCHANTS           32
#define ENCHANT_KIND_GUILD_THIEVES             33
#define ENCHANT_KIND_GUILD_ASSASSINS           34
#define ENCHANT_KIND_GUILD_SHIPWRIGHTS         35
#define ENCHANT_KIND_GUILD_CARTOGRAPHERS       36
#define ENCHANT_KIND_GUILD_MINERS              37
#define ENCHANT_KIND_GUILD_MASONS              38
#define ENCHANT_KIND_GUILD_SAGES               39
#define ENCHANT_KIND_GUILD_MAGES               40
#define ENCHANT_KIND_GUILD_MERCENARIES         41
#define ENCHANT_KIND_GUILD_ADVENTURERS         42
#define ENCHANT_KIND_GUILD_APOTHECARIES        43
#define ENCHANT_KIND_GUILD_ALCHEMISTS          44
#define ENCHANT_KIND_GUILD_BANKERS             45
#define ENCHANT_KIND_GUILD_SHADOW_STALKERS     46
#define ENCHANT_KIND_GUILD_VOID_PILOTS         47
#define ENCHANT_KIND_GUILD_ENGINEERS           48
#define ENCHANT_KIND_GUILD_ARCANE              49
#define ENCHANT_KIND_GUILD_BEASTMASTERS        50
#define ENCHANT_KIND_GUILD_ALCHEMIST_MAGUS     51
#define RVA_LAST_ENCHANT                       ADDR(        0x1afe49c,         0x101cce590)
#define ENCHANTS_MAX                           1000
#define RVA_LOAD_ENCHANTS                      ADDR(        0x18d560,          0x1004efc1c)
#define RVA_NEW_ENCHANT                        ADDR(        0x2c16f0,          0x100563ae8)
#define RVA_NET_SEND_ENCHANT                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_NET_ENCHANT_UPDATE                 ADDR(        0x1eb1f0,          0x100506c28)
#define RVA_COMMANDER_REPEAT_RITUAL            ADDR(        0x11d3c8a4,        0x10ce0cbc4)
#define RVA_COMMANDER_REPEAT_PERCENT           ADDR(        0x11d3c8a8,        MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_FIELD_10                 ADDR(        0x11d3c8a0,        0x10ce0cbc0)
#define RVA_COMMANDER_AI_PLAN                  ADDR(        0x11d3c92c,        0x10ce0cc4c)
#define RVA_PERFORM_REPEAT_RITUALS             ADDR(        0x2c1f50,          0x1005664a4)
#define RVA_SAVE_COMMANDER                     ADDR(        0x18ca60,          0x1004f0e94)
#define RVA_LOAD_COMMANDER                     ADDR(        0x18b950,          0x1004ef740)
#define RVA_NET_PUT_FULL_UNIT                  ADDR(        0x1e7840,          0x100500548)
#define RVA_SEND_ALL_ENABLED_MODS              ADDR(        WIN_INLINED,       MAC_INLINED)

#define RVA_SPELL_TABLE                        ADDR(        0x569f5c0,         0x100839030)
#define SPELL_SIZE                             0x28
#define SPELL_PATH                             0x8
#define SPELL_LEVEL                            0xc
#define SPELL_WEAPON                           0x10
#define SPELL_FLAGS                            0x20
#define RVA_SPELL_TABLE_END                    ADDR(        0x56ae020,         0x100847a90)
#define RVA_SPELL_PATH_NAMES                   ADDR(        0x405ee0,          0x100838d10)
#define RVA_UNIT_PATH_LEVEL                    ADDR(        0x2e75d0,          0x1004bb364)
#define RVA_UNIT_PATH_BONUS                    ADDR(        0x2e4e90,          0x1004baefc)
#define RVA_UNIT_PATH_BOOSTS                   ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_UNIT_SPELL_WEAPON_LEVEL            ADDR(        0x2e76e0,          0x1004bb958)
#define RVA_UNIT_ABILITY_TOTAL                 ADDR(        0x117c90,          0x100466518)
#define RVA_ITEM_ABILITY_HAS_VALUE             ADDR(        WIN_INLINED,       0x1004e8754)
#define RVA_IS_SPELL_ATTACK                    ADDR(        0x0e0b80,          MAC_INLINED)
#define RVA_ADD_MONSTER_ATTACK                 ADDR(        0x0b8c40,          0x1003f8070)
#define RVA_MONSTER_SET_ABILITY                ADDR(        0x1d9f40,          0x100490e5c)
#define RVA_ITEM_SET_ABILITY                   ADDR(        0x17e7d0,          MAC_INLINED)
#define RVA_ITEM_ADD_ABILITY                   ADDR(        0x17e8a0,          MAC_INLINED)
#define RVA_PARSER_SPELLWEAPON50S_PATH         ADDR(        0x1bd5ab,          0x100530720)
#define RVA_PARSER_SPELLWEAPON_PATH            ADDR(        0x1bd62b,          0x1005307b8)
#define RVA_PARSER_SPELLWEAPONBONUS_PATH       ADDR(        0x1bd6ab,          0x100530854)
#define RVA_PARSER_SPELLWEAPON50_PATH          ADDR(        0x1bd72b,          0x1005308d0)
#define RVA_PARSER_SPELLWEAPONSINGLE_PATH      ADDR(        0x1bd7ab,          0x100530968)
#define RVA_ITEM_ABILITY_LINES                 ADDR(        0x17f540,          MAC_INLINED)
#define RVA_ITEM_PATH_BOOST_BOUND              ADDR_DIFFERS(0x17f76b,          0x100525a0c)
#define RVA_ITEM_PATH_BOOST_BOUND_2            ADDR_DIFFERS(0x17f76b,          0x100525a58)
#define RVA_ITEM_PATH_BOOST_BOUND_3            ADDR_DIFFERS(0x17f76b,          0x100525aa4)
#define RVA_ITEM_PATH_BOOST_BOUND_4            ADDR_DIFFERS(0x17f76b,          0x100525aec)
#define RVA_UNIT_DEBUG_SCREEN                  ADDR(        0x1cfd30,          MAC_INLINED)
#define RVA_UNIT_DEBUG_PATH_BOUND              ADDR(        0x1d004d,          0x100526ddc)
#define RVA_SPELL_BROWSER_PATH_BOUND           ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_STATS_SPELL_ATTACK_PATH_BOUND      ADDR(        0x1d66e6,          0x1005237b8)
#define RVA_STATS_KNOWN_SPELLS_PATH_LOOP       ADDR(        0x1d7a58,          0x100524010)
#define RVA_LIBRARY_PATH_LOOP                  ADDR(        0x2de738,          0x10035d318)
#define RVA_LEARN_START_SPELLS_PATH_LOOP       ADDR(        0x2e67dc,          0x1004bc86c)
#define RVA_CHOOSE_MEMORIZED_PATH_LOOP         ADDR(        0x2e70b0,          0x1004bd8f8)
#define RVA_SPELL_BUG_CHECK_PATH_BOUND         ADDR(        0x12af59,          0x100464548)
#define RVA_MASTERY_TRANSFORM                  ADDR(        0x2b3110,          0x10055d534)
#define RVA_RITUAL_MASTERY_PATH_LOOP           ADDR(        0x2c6c7d,          0x10055fab4)
#define RVA_RITUAL_TRANSFORM_PATH_LOOP         ADDR(        0x2c6d22,          0x10055fc94)
#define RVA_PATH_NAMES_READ_ITEM_BOOST         ADDR_DIFFERS(0x17f770,          0x100525a14)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_2       ADDR_DIFFERS(0x17f770,          0x100525a60)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_3       ADDR_DIFFERS(0x17f770,          0x100525aac)
#define RVA_PATH_NAMES_READ_ITEM_BOOST_4       ADDR_DIFFERS(0x17f770,          0x100525af4)
#define RVA_PATH_NAMES_READ_STATS_LEVEL        ADDR_DIFFERS(0x1d6709,          0x1005237c8)
#define RVA_PATH_NAMES_READ_STATS_KNOWN        ADDR_DIFFERS(0x1d7030,          0x100524158)
#define RVA_PATH_NAMES_READ_LIBRARY            ADDR_DIFFERS(0x2de461,          0x10035d41c)
#define RVA_PATH_NAMES_READ_LIBRARY_2          ADDR(        0x2de494,          MAC_NOT_FOUND)
#define RVA_PATH_NAMES_READ_DEBUG_UNIT         ADDR_DIFFERS(0x1d0053,          0x100526d7c)
#define RVA_PATH_NAMES_READ_SPELL_BROWSER      ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_SPELL_BUG_CHECK_SPELL_LOOP_SKIP    ADDR_DIFFERS(0x12af30,          0x100464548)
#define RVA_SPELL_BUG_CHECK_SPELL_LOOP_END     ADDR(        0x12b013,          0x1004645c4)
#define RVA_UNIT_COMMANDER_INDEX               ADDR(        0x129c50,          MAC_INLINED)
#define RVA_MASTERY_COMMANDER_CALL             ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_MASTERY_SNAPSHOT_CALL              ADDR(        0x2b31a6,          0x10055d5b4)
#define RVA_MASTERY_CHOOSE_MEMORIZED_CALL      ADDR(        0x2b368d,          0x10055d720)
#define RVA_LIBRARY_MASTERY_CALL               ADDR(        0x2de956,          0x10035d900)
#define RVA_RITUAL_MASTERY_COMMANDER_CALL      ADDR(        WIN_NO_CALL,       MAC_NO_CALL)
#define RVA_RITUAL_MASTERY_SNAPSHOT_CALL       ADDR(        0x2c6c7d,          0x10055fab4)
#define RVA_RITUAL_MASTERY_MEMORIZE_CALL       ADDR(        0x2c6c7d,          0x10055fab4)
#define RVA_RITUAL_TRANSFORM_SNAPSHOT_CALL     ADDR(        0x2c6d15,          0x10055fc80)
#define RVA_RITUAL_TRANSFORM_FIND_MONSTER_CALL ADDR(        0x2c6d33,          0x10055fca4)
#define RVA_RITUAL_TRANSFORM_MEMORIZE_CALL     ADDR(        0x2c6db4,          0x1005606d0)
#define RVA_LEARN_START_SPELLS_CALL            ADDR(        0x0bf01d,          0x1003ba044)
#define RVA_LEARN_START_SPELLS_CALL_2          ADDR(        0x10fa93,          0x10046d7d4)
#define RVA_LEARN_START_SPELLS_CALL_3          ADDR(        0x12a239,          0x10046ee48)
#define RVA_LEARN_START_SPELLS_CALL_4          ADDR(        0x12434e,          0x10046a2e4)
#define RVA_LEARN_START_SPELLS_CALL_5          ADDR(        0x1873a5,          0x1004ee28c)
#define RVA_LEARN_START_SPELLS_CALL_6          ADDR(        0x1876cc,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL              ADDR(        0x0bf025,          0x1003ba04c)
#define RVA_CHOOSE_MEMORIZED_CALL_2            ADDR(        0x1104a8,          0x10046db38)
#define RVA_CHOOSE_MEMORIZED_CALL_3            ADDR(        0x1105be,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_4            ADDR(        0x12a5c3,          0x10046ee50)
#define RVA_CHOOSE_MEMORIZED_CALL_5            ADDR(        0x124356,          0x10046a2ec)
#define RVA_CHOOSE_MEMORIZED_CALL_6            ADDR(        0x1d88b5,          0x10052712c)
#define RVA_CHOOSE_MEMORIZED_CALL_7            ADDR(        0x1f72df,          0x1004faf5c)
#define RVA_CHOOSE_MEMORIZED_CALL_8            ADDR(        0x2e2c0e,          0x100352a60)
#define RVA_CHOOSE_MEMORIZED_CALL_9            ADDR(        0x2e4d99,          0x1004bec64)
#define RVA_CHOOSE_MEMORIZED_JUMP              ADDR_DIFFERS(0x2e468b,          0x1004bf26c)
#define RVA_CHOOSE_MEMORIZED_JUMP_2            ADDR(        0x2e75b9,          MAC_INLINED)
#define RVA_CHOOSE_MEMORIZED_CALL_10           ADDR(        0x070816,          0x1004d4e70)
#define RVA_CHOOSE_MEMORIZED_CALL_11           ADDR(        0x1873b1,          0x1004ee2a4)
#define RVA_CHOOSE_MEMORIZED_CALL_12           ADDR(        0x1876d7,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_13           ADDR(        0x1846f0,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_14           ADDR(        0x1847b4,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_15           ADDR(        0x184841,          MAC_NOT_FOUND)
#define RVA_CHOOSE_MEMORIZED_CALL_16           ADDR(        0x2b3657,          MAC_NOT_FOUND)
#define RVA_LIBRARY_KNOWN_COUNT_CALL           ADDR(        0x2de37e,          MAC_INLINED)
#define RVA_LIBRARY_ROWS_END_CALL              ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_LIBRARY_ROWS_END_LAYOUT_CALL       ADDR(        0x2de741,          MAC_INLINED)
#define RVA_STATS_KNOWN_COUNT_CALL             ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_STATS_KNOWN_END_CALL               ADDR(        WIN_NO_CALL,       MAC_INLINED)
#define RVA_STATS_KNOWN_END_LAYOUT_CALL        ADDR(        0x1d7ca7,          MAC_INLINED)
#define RVA_STATS_KNOWN_BOXES_SKIP             ADDR(        0x1d6f64,          MAC_INLINED)
#define FRAME_STATS_SCREEN_UNIT                FRAME_OFFSET(0x000044,          MAC_INLINED)
#define FRAME_STATS_SCREEN_STRIPE              FRAME_OFFSET(0x000048,          MAC_INLINED)
#define RVA_TITLE_CELL                         ADDR(        0x03a000,          MAC_INLINED)
#define RVA_HEADER_TEXT                        ADDR(        0x039f40,          0x10033d018)
#define RVA_TEXT_CELL                          ADDR(        0x039be0,          MAC_INLINED)
#define RVA_SPELL_NAME_BUTTON                  ADDR(        0x039620,          MAC_INLINED)
#define RVA_MEMORIZE_ALLOWANCE                 ADDR(        0x2e4a40,          MAC_INLINED)
#define RVA_MEMORIZED_SLOT                     ADDR(        0x2e49f0,          0x1004be664)
#define RVA_SPELL_RANGE_BONUS                  ADDR(        0x31f1c0,          0x10056c2cc)
#define RVA_DAMAGE_TEXT                        ADDR(        0x1cef40,          0x1005271fc)
#define RVA_UNIT_OWNER                         ADDR(        0x129590,          MAC_INLINED)
#define RVA_GET_UNIT_X                         ADDR(        0x32a130,          MAC_INLINED)
#define RVA_GET_UNIT_Y                         ADDR(        0x32aa60,          MAC_INLINED)
#define RVA_LIBRARY_LEVEL                      ADDR(        0x1b1b80,          MAC_INLINED)
#define RVA_PLAYER_LIBRARY_TOTAL               ADDR(        0x1b1220,          0x1004ab3c8)
#define RVA_COUNT_SQUARES_OF_TERRAIN           ADDR(        0x32e330,          0x1004cec8c)
#define RVA_COUNT_GROUP_CITADELS               ADDR(        0x0af680,          MAC_INLINED)
#define RVA_COUNT_GROUP_SQUARES                ADDR(        0x2c1cb0,          MAC_INLINED)
#define RVA_MONSTER_GATHERS_RESOURCE           ADDR(        0x091230,          0x100363c8c)
#define RVA_SQUARE_LIBRARY_LEVEL               ADDR(        0x1b1dc0,          MAC_INLINED)
#define RVA_SQUARE_ENCHANT_VALUE               ADDR(        0x2beaa0,          0x100565e68)
#define RVA_UNIT_MAX_HP                        ADDR(        0x3261a0,          0x10056aa8c)
#define RVA_UNIT_FLAGS                         ADDR(        0x329200,          0x100569ecc)
#define RVA_UNIT_AP_LEFT                       ADDR(        0x3209a0,          MAC_INLINED)
#define UNIT_AP_USED                           0x1c
#define UNIT_FLAG_SLOW                         0x8
#define UNIT_FLAG_FAST                         0x10
#define UNIT_FLAG_FLY_OR_FLOAT                 0x81
#define UNIT_FLAG_WATER_MOVE                   0x40
#define UNIT_FLAG_MT_NOLAND                    0x200
#define UNIT_FLAG_PASSWALL                     0x100000000
#define UNIT_FLAG_SHIPMOVE                     0x10000000000
#define UNIT_FLAG_TUNNEL                       0x40000000000
#define RVA_UNIT_HAS_FLAG_4                    ADDR(        0x1d9e90,          MAC_INLINED)
#define RVA_COUNT_MEMORIZED_OF_TYPE            ADDR(        0x2e4240,          MAC_INLINED)
#define RVA_PARSER_LEVELUP_EFFECT              ADDR(        0x1c45d7,          0x100537688)
#define RVA_PARSER_LEVELUPMON_EFFECT           ADDR(        0x1c45f0,          0x1005376a4)
#define RVA_ADD_KNOWN_SPELL                    ADDR(        0x2e7200,          0x1004bc284)
#define RVA_RANDOM_SPELL                       ADDR(        0x2e4320,          0x1004bac7c)
#define RVA_RANDOM_SPELL_NOT_KNOWN             ADDR(        0x2e4570,          0x1004be9c4)
#define RVA_LEARN_START_SPELLS                 ADDR(        0x2e5c00,          0x1004bc804)
#define RVA_LEARN_RANDOM_SPELL                 ADDR(        0x2e4d30,          0x1004bea60)
#define RVA_CHOOSE_MEMORIZED                   ADDR(        0x2e6850,          0x1004bd880)
#define RVA_RANDOM_MEMORIZED_SPELL             ADDR(        0x2e59d0,          0x1004bd100)
#define RVA_ADD_AFFLICTION                     ADDR(        0x0f4dc0,          0x1003ca400)
#define RVA_UNIT_CARD                          ADDR(        0x1d4990,          0x10052113c)
#define RVA_MEMORIZE_SPELL                     ADDR(        0x2e7400,          0x1004bc344)
#define RVA_UNMEMORIZE_SPELL                   ADDR(        0x2e48e0,          MAC_INLINED)
#define RVA_UNMEMORIZE_SPELL_BOUND             ADDR_DIFFERS(0x2e4900,          0x1004bd98c)
#define RVA_UNMEMORIZE_SPELL_BOUND_2           ADDR_DIFFERS(0x2e4900,          0x1004be47c)
#define RVA_UNMEMORIZE_SPELL_BOUND_3           ADDR_DIFFERS(0x2e4900,          0x100526a28)
#define RVA_COUNT_KNOWN_SPELLS                 ADDR(        0x2e4a70,          0x1004bcc5c)
#define RVA_COUNT_SPELLS                       ADDR(        0x2e4420,          MAC_INLINED)
#define RVA_SPELL_INFO_BOX                     ADDR(        0x2e4700,          MAC_INLINED)
#define RVA_COUNT_MEMORIZED_SPELLS             ADDR(        0x2e46a0,          0x1004bcdc4)
#define RVA_LEARN_PROMOTION_SPELLS             ADDR(        0x2e4620,          0x1004bf0a0)
#define RVA_UNIT_FIRST_SPELL_PATH              ADDR(        0x2e7500,          0x1004bec88)
#define RVA_RANDOM_1_TO_N                      ADDR(        0x058b70,          0x100336f24)
#define RVA_COMMANDER_ADD_AP_USED              ADDR(        0x12c8f0,          0x100481470)
#define RVA_UNIT_ADD_AP_USED                   ADDR(        0x118c20,          MAC_INLINED)
#define RVA_FLUSH_COMMANDER_CACHES             ADDR(        0x129d90,          MAC_INLINED)
#define RVA_COMMANDER_ARMY_AP_LEFT             ADDR(        0x329080,          0x10056a8d4)
#define COMMANDER_AP_CACHE                     0xa3
#define RVA_COMMANDER_CACHE_INTS               ADDR(        0x66053b0,         0x106cd5de8)
#define RVA_START_SPELLS_MORE_SPELLS_CALL      ADDR(        WIN_NO_CALL,       0x1004bc95c)
#define RVA_START_SPELLS_FORTUNE_CHECK         ADDR_DIFFERS(0x2e65c2,          0x1004bca4c)
#define RVA_START_SPELLS_FORTUNE_CHECK_2       ADDR(        WIN_NO_EQUIVALENT, 0x1004bcc24)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL         ADDR_DIFFERS(0x2c6c9c,          0x10055facc)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL_2       ADDR_DIFFERS(0x2c6cbb,          0x10055facc)
#define RVA_RITUAL_NEWSPELL_LEARN_CALL_3       ADDR_DIFFERS(0x2c6cda,          0x10055facc)
#define RVA_RITUAL_SUMMON_LEARN_CALL           ADDR(        0x2c67e7,          0x100560cdc)
#define RVA_RITUAL_CAN_CAST_NEWSPELL           ADDR(        0x2caab7,          0x10055c3e0)
#define RVA_RITUAL_CAN_CAST_NEWSPELL_2         ADDR(        0x2cab1e,          0x10055c410)
#define RVA_RITUAL_CAN_CAST_NEWSPELL_3         ADDR(        0x2cab7e,          0x10055c440)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL         ADDR(        WIN_NO_CALL,       0x1004e69c4)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL_2       ADDR(        WIN_NO_CALL,       0x1004e6a2c)
#define RVA_ITEM_USE_SCROLL_LEARN_CALL_3       ADDR(        WIN_NO_CALL,       0x1004e6a94)
#define RVA_LIBRARY_ROW_PATH_LEVEL_CALL        ADDR(        0x2de399,          0x10035d328)
#define RVA_LIBRARY_LEARN_CALL                 ADDR_DIFFERS(0x2deb46,          0x10035dd80)
#define RVA_USE_LIBRARY_LEARN_CALL             ADDR(        0x2e3d5e,          0x10035cc74)
#define RVA_AI_USE_LIBRARY_CALL                ADDR(        0x07ad55,          0x1004d1e38)
#define RVA_PRIEST_PROMOTION_LEARN_CALL        ADDR(        0x2e2c05,          0x100352948)
#define RVA_DOATTACK_PATH_BONUS_CALL           ADDR(        0x0d4a2b,          0x1003ebbac)
#define RVA_DOATTACK_SPELL_PICK_CALL           ADDR(        0x0d4b04,          0x1003ebc78)
#define RVA_DOATTACK_CAST_COUNT                ADDR_DIFFERS(0x0d4b77,          0x1003ebce0)
#define RVA_STATS_KNOWN_PATH_LEVEL_CALL        ADDR(        0x1d6fdf,          0x100524084)
#define RVA_FIND_SPELL                         ADDR(        0x2e7310,          MAC_INLINED)
#define RVA_SPELL_USEFUL                       ADDR(        0x2e5ae0,          0x1004bf8a4)
#define RVA_SPELL_FLAG_LINES                   ADDR(        0x2e4b00,          0x1004bf270)
#define RVA_ARMY_HAS_SPELL_FLAG                ADDR(        0x2e4dd0,          0x1004bf9a4)
#define RVA_UNIT_HEAL_RATE                     ADDR(        0x324cb0,          MAC_INLINED)
#define RVA_SPELL_BROWSER                      ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_SPELL_BROWSER_BOUND                ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_LISTSPELLS_BOUND                   ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_NET_RECV_MEMORIZED_BOUND           ADDR_DIFFERS(0x1f72ab,          0x1004faca4)
#define RVA_STATS_SPELL_ROW_ID                 ADDR_DIFFERS(0x1d73ff,          0x100524464)
#define RVA_STATS_SPELL_ROW_DECODE             ADDR(        0x1d8716,          0x100524dc4)
#define RVA_STATS_SPELL_ROW_RIGHT_BOUND        ADDR_DIFFERS(0x1d871c,          0x100524dd0)
#define RVA_STATS_SPELL_ROW_RIGHT_SHIFT        ADDR(        WIN_NO_EQUIVALENT, 0x100524dcc)
#define RVA_STATS_SPELL_ROW_LEFT_OFFSET        ADDR_DIFFERS(0x1d8727,          0x100524dd8)
#define RVA_STATS_SPELL_ROW_LEFT_SHIFT         ADDR(        WIN_NO_EQUIVALENT, 0x100524de0)
#define RVA_STATS_SPELL_ROW_LEFT_BOUND         ADDR_DIFFERS(0x1d872d,          0x100524de4)
#define RVA_STATS_SPELL_ROW_LEFT_BOUND_2       ADDR(        WIN_NO_EQUIVALENT, 0x10052555c)
#define RVA_STATS_SPELL_ROW_LEFT_SPELL         ADDR_DIFFERS(0x1d8773,          0x100524dd8)
#define RVA_STATS_SPELL_ROW_INFO_SPELL         ADDR_DIFFERS(0x1d8830,          0x100524df8)
#define RVA_STATS_SPELL_ROW_INFO_SHIFT         ADDR(        WIN_NO_EQUIVALENT, 0x100524e00)
#define RVA_STATS_SPELL_ROW_INFO_BOUND         ADDR(        WIN_NO_EQUIVALENT, 0x100524e04)
#define RVA_FIND_SPELL_TABLE_REF               ADDR_DIFFERS(0x2e733b,          0x1004bc2b0)
#define RVA_NET_RECV_MEMORIZED_BRANCH          ADDR_DIFFERS(0x1f72b1,          0x1004faca8)
#define RVA_SLOW_SPELL_ENCODE                  ADDR_DIFFERS(0x0d4caa,          0x1003ecaa4)
#define RVA_SLOW_SPELL_DECODE                  ADDR(        0x0d4a93,          0x1003ebc14)
#define RVA_AI_ZAP_SLOW_SPELL_CHECK            ADDR_DIFFERS(0x0cd6dd,          0x1003e4f00)
#define RVA_SPELL_BUG_CHECK                    ADDR(        0x12a6a0,          0x100463ebc)
#define RVA_NO_CHECKS                          ADDR_DIFFERS(0x582398c,         0x106cd54c8)
#define RVA_PREF_END_TURN_WARNING              ADDR(        0x5828158,         MAC_NOT_LOOKED_UP)
#define RVA_UNIT_ALIVE_AT                      ADDR(        0x3268d0,          MAC_NOT_FOUND)
#define RVA_DOATTACK                           ADDR(        0x0d4760,          0x1003eb638)
#define RVA_GET_TARGET_SQUARE                  ADDR(        0x0d63a0,          0x1003bf300)
#define RVA_LIBRARY_SCREEN                     ADDR(        0x2dde70,          0x10035cd58)
#define RVA_USE_LIBRARY                        ADDR(        0x2e3bd0,          0x10035c9d0)
#define RVA_ITEM_USE                           ADDR(        0x184360,          0x1004e66fc)
#define RVA_FIGHT_THE_FIGHT                    ADDR(        0x0f24b0,          0x1003dfcec)
#define RVA_FIGHT_ZAP_CASTABLE_CALL            ADDR(        0x0f37dc,          0x1003e3df0)
#define RVA_AI_ZAP_CHOICE                      ADDR(        0x0ccc40,          MAC_INLINED)
#define RVA_AI_ZAP_CASTABLE_CALL               ADDR(        0x0ccdac,          0x1003e3f7c)
#define RVA_BATTLE_ITEM_EFFECTS                ADDR(        0x0d7060,          MAC_INLINED)
#define RVA_ITEM_CASTABLE_SPELL                ADDR(        0x183910,          0x1004e8e9c)
#define RVA_ITEM_INFO_SCREEN                   ADDR(        0x17ef40,          MAC_INLINED)
#define RVA_ITEM_SPELL_LINE                    ADDR(        0x1800a0,          MAC_NOT_FOUND)
#define RVA_ITEM_SPELL_STATS                   ADDR(        0x180020,          MAC_NOT_FOUND)
#define RVA_KNOWN_SPELL_SLOT_BY_NAME           ADDR(        0x2e4000,          MAC_NOT_FOUND)
#define RVA_MEMORIZE_SPELL_BY_NAME             ADDR(        0x2e42f0,          MAC_NOT_FOUND)
#define RVA_UNIT_KNOWN_SPELLS                  ADDR(        0xfb8e800,         0x1100498a0)
#define RVA_UNIT_MEMORIZED_SPELLS              ADDR(        0xfb8e878,         0x110049918)
#define RVA_UNIT_SLOW_SPELL                    ADDR(        0xfb8e904,         0x1100499a4)
#define RVA_SAVE_UNIT                          ADDR(        0x18c5d0,          0x1004f0884)
#define RVA_LOAD_UNIT                          ADDR(        0x18d8f0,          0x1004ef150)
#define RVA_NET_SEND_UNITS                     ADDR(        0x1f2cc0,          0x10050e05c)
#define RVA_FIND_WEAPON                        ADDR(        0x0b8d10,          0x1003f7ffc)

#define RVA_UNIT_ITEMS                         ADDR(        0xfb8e8b4,         0x110049954)
#define UNIT_ITEM_SLOTS                        8
#define RVA_UNIT_TEMP_ABILITY_IDS              ADDR(        0xfb8e8ce,         0x11004996e)
#define RVA_UNIT_TEMP_ABILITY_VALUES           ADDR(        0xfb8e8e0,         0x110049980)
#define UNIT_TEMP_ABILITIES                    4
#define RVA_SET_TEMP_ABILITY                   ADDR_DIFFERS(0x118c60,          0x1004683d0)
#define RVA_GET_TEMP_ABILITY                   ADDR(        0x117510,          0x1004684cc)
#define RVA_COMMANDER_KNOWN_RITUALS            ADDR(        0x11d3c8ac,        0x10ce0cbcc)
#define RVA_GROUND_ITEMS                       ADDR(        0xf958020,         0x10cb0e1d4)
#define GROUND_ITEMS_PER_SQUARE                6
#define MAP_COLUMN_SQUARES                     160
#define MAP_COLUMNS_MAX                        1000
#define RVA_ADD_GROUND_ITEMS                   ADDR(        0x26c8a0,          0x1003f8528)
#define RVA_SQUARE_RECORDS                     ADDR(        0xa20aca0,         0x10d011c78)
#define SQUARE_SIZE                            0x13c
#define SQUARE_ARMY_OWNER                      0x3a
#define SQUARE_HOLDER                          0x3c
#define SQUARE_OWNER                           0x42
#define SQUARE_GEMS                            0xa3
#define SQUARE_GEM_KINDS                       4
#define SQUARE_TERRAIN_TYPE                    0x20
#define RVA_SAVE_SQUARE                        ADDR(        0x18c200,          0x1004f147c)
#define RVA_LOAD_SQUARE                        ADDR(        0x18cec0,          0x1004ee738)
#define RVA_SAVE_GAME_SQUARES_CALL             ADDR(        0x18f1c8,          0x1004f2980)
#define RVA_LOAD_GAME_SQUARES_CALL             ADDR(        0x1867b7,          0x1004ebfe0)
#define RVA_SERVER_SAVE_SQUARES_CALL           ADDR(        0x1f4597,          MAC_NO_CALL)
#define RVA_GROUND_ITEMS_CLEAR                 ADDR(        0x186608,          0x1004ebe68)
#define RVA_SQUARE_WORK_I16                    RVA_GEN_SCRATCH_MARKS
#define RVA_MAP_IMAGE_SQUARES_I32              ADDR(        0x63fbc50,         0x10c691afc)
#define RVA_MAP_IMAGE_SQUARES_I16              ADDR(        0x63ad9c0,         MAC_NOT_FOUND)
#define RVA_MAP_IMAGE_SQUARE_FLAGS             ADDR(        0x6128740,         0x10c660898)
#define RVA_BACKGROUND_SQUARES_A               RVA_MAP_BG_FLOODED
#define RVA_BACKGROUND_SQUARES_B               RVA_MAP_BG_GROUNDS
#define RVA_WORLD_GEN_SQUARES_A                RVA_RIVER_DISTANCE
#define RVA_WORLD_GEN_SQUARES_B                RVA_RIVER_COST
#define RVA_PATH_SQUARE_CACHE                  ADDR(        0x9f7f300,         0x1121dfec4)
#define RVA_SAVE_ENCHANTS                      ADDR(        0x18bf40,          0x1004f1260)
#define RVA_SHOP_LAST                          ADDR(        0x11c14588,        0x10cce487c)
#define RVA_SHOP_OFFERS                        ADDR(        0x11c1458c,        0x10cce4880)
#define SHOP_OFFERS_MAX                        160
#define RVA_SHOP_OFFER_COUNTS                  ADDR(        0x11c1480c,        0x10cce4b00)
#define RVA_SHOP_ADD                           ADDR(        0x0af900,          0x100362390)
#define RVA_SHOP_BUILD                         ADDR(        0x0a9430,          0x100364a6c)
#define RVA_SHOP_BUILD_CALL                    ADDR(        0x1681b0,          0x100395234)
#define RVA_SHOP_CLASS_OFFER_CALL              ADDR(        0x0a9ac9,          0x1003659dc)
#define RVA_SHOP_LOCATION_OFFER_CALL           ADDR(        0x0ad7e5,          0x10036a93c)
#define RVA_SHOP_UNITS_COUNT_CALL              ADDR(        0x0af2f5,          0x10036321c)
#define RVA_SHOP_UNITS_STEP_3                  ADDR(        0x0af2ff,          0x100363228)
#define RVA_SHOP_UNITS_STEP_9                  ADDR(        0x0af30a,          0x100363238)
#define SHOP_BUILD_MESSAGE_CALLS               48
#define RVA_SHOP_ADD_ITEM_OFFER_CALL           ADDR(        0x0aed74,          MAC_NOT_LOOKED_UP)
#define RVA_SPECIAL_LOCATION_RECRUITS          ADDR(        0x1a29b30,         MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_HAS_TERRAIN_CITADEL         ADDR(        0x1b0d90,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_SQUARES_WITH_FLAGS           ADDR(        0x331720,          MAC_NOT_LOOKED_UP)
#define RVA_LOAD_SHOP                          ADDR(        0x189f60,          MAC_INLINED)
#define RVA_FIND_ITEM                          ADDR(        0x1853f0,          0x1004e5868)
#define RVA_AUTO_RECRUIT_SQUARES               ADDR(        0x11c14308,        0x10cce45fc)
#define RVA_AUTO_RECRUIT_MONSTERS              ADDR(        0x11c14408,        0x10cce46fc)
#define AUTO_RECRUIT_ORDERS_MAX                64
#define RVA_AUTO_RECRUIT                       ADDR(        0x166e00,          MAC_INLINED)
#define RVA_MESSAGE_ATTACH_SIZES               ADDR(        0xe4a5d38,         0x1085bb0a8)
#define RVA_MESSAGE_ATTACHMENTS                ADDR(        0xe4a5d48,         0x1085bb0b8)
#define RVA_MESSAGE_SCREEN                     ADDR(        0x1e2210,          0x1004c2390)
#define RVA_TURN_MESSAGES_CALL                 ADDR(        0x094cd7,          0x10037b86c)
#define RVA_VIEW_MESSAGES_CALL                 ADDR(        0x097ad8,          0x100380e4c)
#define RVA_MESSAGES_WAIT_CALL                 ADDR(        0x1e29e9,          0x1004c2be8)
#define RVA_LOAD_BATTLE_REPLAY                ADDR(        0x0f44a0,          MAC_INLINED)
#define RVA_LOAD_PART_STATS                    ADDR(        0x0ba980,          0x1003f79d8)
#define RVA_SAVE_PART_STATS                    ADDR(        0x0bad00,          0x1003df5dc)
#define RVA_SAVE_BATTLE_REPLAY                 ADDR(        0x0f1f80,          MAC_NOT_FOUND)
#define RVA_NET_SEND_SQUARE                    ADDR(        0x1f34b0,          0x10050c904)
#define RVA_NET_SEND_UNIT                      ADDR(        0x1f28b0,          0x10050e488)
#define RVA_NET_SEND_MEMORIZED                 ADDR(        0x1f1230,          0x100510cdc)
#define RVA_NET_SEND_AUTO_RECRUIT              ADDR(        0x1f2e80,          0x1005109c8)
#define RVA_NET_GIVE_ITEM                      ADDR(        0x1f1320,          MAC_INLINED)
#define RVA_FIND_SHOP_OFFER                    ADDR(        0x11eff0,          MAC_INLINED)
#define RVA_SHOP_OFFER_MESSAGE                 ADDR(        0x0af760,          0x100362628)
#define RVA_SHOP_MERGE_MESSAGES                ADDR(        0x1e1fb0,          MAC_INLINED)
#define RVA_COUNT_NAME_TEXT                    ADDR(        0x119a90,          0x10046c27c)
#define RVA_SHOP_COSTS                         ADDR(        0x11c14a8c,        0x10cce4d80)
#define SHOP_COST_COUNT                        19
#define RVA_SHOP_OFFER_FLAGS                   ADDR(        0x11c17a0c,        0x10cce7d00)
#define SHOP_FLAG_ONCE                         0x1
#define SHOP_FLAG_AI_IMPORTANT                 0x2
#define SHOP_FLAG_EXTRA                        0x4
#define SHOP_FLAG_AI_NEVER                     0x10
#define SHOP_FLAG_AI_FIRST_TURNS               0x200
#define SHOP_OFFER_TEXT_SIZE                   0x30
#define SQUARE_FLAG_RECRUITED                  0x1
#define RVA_SHOP_OFFER_TERRAIN                 ADDR(        0x11c17c8c,        0x10cce7f80)
#define RVA_SHOP_OFFER_NAMES                   ADDR(        0x11c17f0c,        0x10cce8200)
#define RVA_SHOP_OFFER_LIMITS                  ADDR(        0x11c19d0c,        0x10ccea000)
#define RVA_ITEM_SLOT_TYPE                     ADDR(        0x1be6764,         0x10489382c)
#define RVA_GIVE_ITEM                          ADDR(        0x180720,          0x1004e4f2c)
#define RVA_GIVE_ITEM_AND_ENFORCE              ADDR(        0x185670,          0x1004e5628)
#define RVA_SHOP_ITEM_CAN_RECEIVE              ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_SHOP_ITEM_RECEIVER                 ADDR(        0x183440,          MAC_NOT_LOOKED_UP)
#define RVA_ENFORCE_ITEMS                      ADDR(        0x183ac0,          0x1004e5668)
#define RVA_CAN_EQUIP_ITEM                     ADDR(        0x1854c0,          0x1004e4604)
#define RVA_REMOVE_ENCHANT                     ADDR(        0x2be4a0,          0x100564178)
#define RVA_REBUILD_ENCHANT_FLAGS              ADDR(        0x2c00b0,          0x100565990)
#define RVA_LOAD_GAME_ENCHANT_FLAGS            ADDR(        0x1872a2,          0x1004ee1e8)
#define RVA_SQUARE_FLAGS                       ADDR(        0xa20ad48,         0x10d011d20)
#define RVA_FREE                               ADDR(        0x05e8c0,          0x1003342e8)
#define RVA_MESSAGE_ATTACH                     ADDR(        WIN_INLINED,       0x1004c5650)
#define RVA_MESSAGE_DETACH                     ADDR(        0x1e1280,          MAC_INLINED)
#define RVA_BATTLE_ATTACH_CALL                 ADDR(        WIN_NO_CALL,       0x1003de7a0)
#define RVA_BATTLE_REPLAY_WRITE                ADDR(        0x0f5dd0,          0x1003dd3a8)
#define RVA_SAVE_FILE_VERSION                  ADDR(        0x5822fa8,         0x10857c9a0)
#define RVA_BATTLE_SEED                        ADDR(        0x11fb18a0,        0x108564670)
#define RVA_BATTLE_PLAYER_A                    ADDR(        0x43e234,          0x101cce59c)
#define RVA_BATTLE_PLAYER_B                    ADDR(        0x43e26c,          0x101cce598)
#define RVA_BATTLE_X                           ADDR(        0x43e014,          0x104904c84)
#define RVA_BATTLE_Y                           ADDR(        0x43dfc4,          0x104904c88)
#define RVA_FILE_WRITE_INT64                   ADDR(        0x058490,          MAC_INLINED)
#define RVA_FILE_READ_INT64                    ADDR(        0x0589a0,          MAC_INLINED)
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

#define RVA_MAP_FILES                          ADDR(        0xe77d260,         0x10c127aa8)
#define RVA_MAP_FILE_COUNT                     ADDR(        0x1a95e64,         0x1008735d4)
#define RVA_TRANSLATION_HASHES                 ADDR(        0x58ffed0,         0x105b8fc50)
#define RVA_TRANSLATION_STRINGS                ADDR(        0x57c6310,         0x105b68b50)
#define RVA_TRANSLATION_COUNT                  ADDR(        0x435d58,          0x100857670)
#define RVA_TRANSLATION_HASH                   ADDR(        0x05c480,          MAC_INLINED)
#define RVA_TRANSLATION_NORMALIZE              ADDR(        0x05c660,          MAC_INLINED)
#define RVA_DUMPSTRINGS_FILE                   ADDR(        0x57eddf0,         0x105b68188)

// ---- Rituals and the map ------------------------------------------------------------------------
#define RVA_RITUAL_SCREEN                      ADDR(        0x2ae780,          0x10035e3ec)
#define RVA_RITUAL_CAN_CAST                    ADDR(        0x2c9f30,          0x10055bea8)
#define RVA_RITUAL_LOCATION_CHECK              ADDR(        0x2cd460,          0x100559868)
#define RVA_RITUAL_ROW                         ADDR_DIFFERS(0x2d6c10,          0x100351d60)
#define RITUAL_COST_TYPE_MAX                   50
#define RVA_RESOURCE_NAMES                     ADDR(        0x3f9e90,          0x1008482b8)
#define RVA_INCOME_NAMES                       ADDR(        0x344360,          0x100847b68)
#define INCOME_NAME_COUNT                      19
#define RVA_RITUAL_CHECK_COSTS                 ADDR(        0x2c38f0,          0x1005586d4)
#define RVA_RITUAL_PAY_COSTS                   ADDR(        0x2c2ae0,          0x100558afc)
#define RVA_RESOURCE_AVAILABLE                 ADDR(        0x3204e0,          0x100569550)
#define RVA_CAN_AFFORD_AT                      ADDR(        0x3205f0,          0x1005696b4)
#define RVA_PAY_RESOURCE_AT                    ADDR(        0x320720,          0x1005697c0)
#define RVA_COUNT_EXPENDABLE_AT                ADDR(        0x2b5f90,          0x100557d28)
#define RVA_COUNT_ABILITY_AT                   ADDR(        0x2c1050,          0x1005581e0)
#define RVA_REMOVE_ABILITY_UNITS_AT            ADDR(        0x2b5c10,          0x1005583ac)
#define RVA_CORPSES_AT                         ADDR(        0x3358f0,          MAC_INLINED)
#define RVA_UNIT_LEAVE_CORPSE                  ADDR(        0x0c0020,          0x1003bef90)
#define RVA_DAMAGE_LEAVE_CORPSE_CALL           ADDR(        0x0bfb9a,          0x1003bb250)
#define RVA_HP_LOSS_LEAVE_CORPSE_CALL          ADDR(        0x0bd7df,          0x1003a8278)
#define RVA_RAISE_DEAD                         ADDR(        0x10bb60,          0x1004753dc)
#define RVA_RAISE_ANIMALS                      ADDR(        0x10ace0,          0x100476324)
#define RVA_RITUAL_RAISE_DEAD_CALL             ADDR(        0x2c8a39,          0x100561e54)
#define RVA_RITUAL_RAISE_ANIMALS_CALL          ADDR(        0x2c8a4d,          0x100562288)
#define RVA_RITUAL_PUT_CORPSES_TEXT_CALL       ADDR(        0x2c99ee,          0x100562d28)
#define RVA_INSANITY_ADD                       ADDR(        0x116240,          MAC_INLINED)
#define SQUARE_CORPSES                         0x2a
#define SQUARE_CORPSE_KINDS                    8
#define MONSTER_ABILITY_EXPENDABLE             0x1ae
#define MONSTER_ABILITY_EXPENDABLE_2           0x4ca
#define MONSTER_ABILITY_GOBLIN                 0x4b3
#define RVA_COMMANDERS                         ADDR(        0x11d3c890,        0x10ce0cbb0)
#define COMMANDER_SIZE                         0xd4
#define COMMANDER_TABLE_MAX                    9990
#define COMMANDER_KNOWN_RITUALS_MAX            64
#define RVA_COMMANDER_NAMES                    ADDR(        0x11d3c860,        0x10ce0cb80)
#define COMMANDER_NAME_SIZE                    0x30
#define RVA_LAST_COMMANDER                    ADDR(        0x11d37a30,        0x10ce0cb7c)
#define RVA_COMMANDER_DONE                     ADDR(        0x11d3c932,        0x10ce0cc52)
#define RVA_COMMANDER_SET_DONE                 ADDR(        0x0a1130,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_NEXT_COMMANDER                ADDR(        0x0a0e30,          MAC_NOT_LOOKED_UP)
#define RVA_SELECT_NEXT_COMMANDER              ADDR(        0x0a0d90,          MAC_NOT_LOOKED_UP)
#define RVA_DROP_DEAD_FROM_GROUP               ADDR(        0x1df330,          MAC_NOT_LOOKED_UP)
#define RVA_COUNT_COMMANDERS_WITH_AP           ADDR(        0x09ddc0,          MAC_NOT_LOOKED_UP)
#define RVA_SELECTED_COMMANDER                 ADDR(        0xfb2cc28,         0x10cb0e18c)
#define RVA_SELECTED_COMMANDER_GROUP           ADDR(        0xfb2cc24,         0x10cb0e190)
#define COMMANDER_ORDER_GROUP                  0x4
#define COMMANDER_MOVE_ORDER_X                 0x8
#define COMMANDER_MOVE_ORDER_Y                 0xc
#define RVA_COMMANDER_UNIT                     ADDR(        0x12c8c0,          MAC_INLINED)
#define RVA_UNIT_X                             ADDR(        0xfb8e7f4,         0x110049894)
#define UNIT_SIZE                              0x158
#define RVA_UNIT_LEADER                        ADDR(        0xfb8e7f0,         0x110049890)
#define RVA_UNITS                              ADDR(        0xfb8e7e0,         0x110049880)
#define RVA_UNIT_MONSTER                       ADDR(        0xfb8e8c8,         0x110049968)
#define RVA_UNIT_COUNT                         ADDR(        0xfb8e7c0,         0x110049878)
#define RVA_UNIT_HP                            ADDR(        0xfb8e8c6,         0x110049966)
#define RVA_UNIT_PLAYER                        ADDR(        0xfb8e8ca,         0x11004996a)
#define RVA_UNIT_XP                            ADDR(        0xfb8e8c4,         0x110049964)
#define RVA_UNIT_KILLS                         ADDR(        0xfb8e8cc,         0x11004996c)
#define UNIT_COMMANDER                         0x10
#define UNIT_TABLE_MAX                         99000
#define MONSTER_FLAG_NO_COMMANDER              35
#define MONSTER_ABILITY_UNIQUE                 0x39
#define RVA_NEW_UNIT                           ADDR(        0x129e80,          0x10046e9bc)
#define RVA_NEW_COMMANDER                      ADDR(        0x128830,          0x10046dbc4)
#define RVA_ALLOC_UNIT                         ADDR(        0x1107c0,          0x10046d318)
#define RVA_ALLOC_TEMP_UNIT                    ADDR(        0x110be0,          MAC_INLINED)
#define RVA_RESERVED_UNIT                      ADDR(        0x1f2500,          MAC_INLINED)
#define RVA_RESERVED_COMMANDER                 ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_REBUILD_SQUARE_ARMY                ADDR(        0x18fb30,          0x1004f0074)
#define RVA_UNIT_HOLDS_SQUARE                  ADDR(        0x320140,          MAC_INLINED)
#define RVA_UNIT_FIT_TERRAIN                   ADDR(        0x115620,          0x1004675e4)
#define RVA_CONQUER_CHECK                      ADDR(        0x1b0040,          0x1004ab0a8)
#define RVA_SQUARE_STRONGEST_OWNER             ADDR(        0x104ab0,          0x100465d70)
#define RVA_UPDATE_SIGHT                       ADDR(        0x19ac80,          0x10049f718)
#define RVA_UPDATE_SIGHT_AROUND                ADDR(        0x1af290,          MAC_INLINED)
#define RVA_RESET_SQUARE_CACHE                 ADDR(        0x1b3900,          0x100498d44)
#define RVA_SIGHT_RAYS                         ADDR(        0x19c330,          MAC_NOT_LOOKED_UP)
#define RVA_SIGHT_BOX                          ADDR(        0x1ae190,          0x1004a5ed4)
#define RVA_SIGHT_SOURCE_NEAR                  ADDR(        0x19bf30,          MAC_NOT_LOOKED_UP)
#define RVA_MARK_SQUARE_IN_VIEW                ADDR(        0x1b11c0,          MAC_INLINED)
#define RVA_UPDATE_TRUE_TERRAIN_SEEN           ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_COPY_TEAM_SIGHT                    ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_REVEAL_MAP_CALL                    ADDR(        0x10d88d,          0x100474354)
#define RVA_PLANE_DISCOVERED_PASS              ADDR(        0x253930,          MAC_INLINED)
#define RVA_PLANE_DISCOVERED_NEXT_CALL         ADDR(        0x166d4e,          0x1003b74d8)
#define RVA_DEAD_UNITS_PASS                    ADDR_DIFFERS(0x1810b0,          0x1004ea468)
#define RVA_REVEAL_MAP_FOR                     ADDR(        0x1b0c40,          0x1004a6044)
#define RVA_SIGHT_CACHE_RESET                  ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SEEN_AT                     ADDR(        0x335bc0,          MAC_NOT_LOOKED_UP)
#define RVA_SEEN_NEIGHBOURS_COUNT              ADDR(        WIN_NOT_FOUND,     MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SIGHT_LEVEL                 ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_UNIT_VISIBLE_TO                    ADDR(        0x1183d0,          0x100467e98)
#define RVA_UNIT_STEALTH_LEVEL                 ADDR(        0x31fc10,          MAC_INLINED)
#define RVA_UNIT_BLINDNESS                     ADDR(        0x325380,          0x100567328)
#define RVA_FIRST_TEAM_UNIT_AT                 ADDR(        0x105340,          MAC_NOT_LOOKED_UP)
#define RVA_PLAYER_UNIT_AT                     ADDR(        0x105430,          0x100468204)
#define RVA_PLAYER_HAS_UNIT_AT                 ADDR(        0x31fb50,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEAM_ABILITY                ADDR(        0x112a00,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_TEAM_ABILITY_B              ADDR(        0x112f20,          0x1004693b8)
#define RVA_SQUARE_TERRAIN_FLAGS               ADDR(        0x3357e0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_MARK_BOX                    ADDR(        0x19c250,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_MARK_DRAW                   RVA_MAP_DRAW_PLANE
#define RVA_MONSTER_VARIANT                    ADDR(        0x108900,          0x100479538)
#define RVA_RECRUIT_BUY                        ADDR(        0x108350,          0x100479724)
#define RVA_POPULATE                           ADDR(        0x26c410,          0x1003f8778)
#define RVA_POPULATE_NO_COMMANDER              ADDR(        0x273100,          MAC_INLINED)
#define RVA_POPULATE_COUNT_FIRST               ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_POPULATE_COMMANDER                 ADDR(        0x272e50,          0x1003f8c20)
#define RVA_SUMMON_STRING                      ADDR(        0x172140,          0x10053e744)
#define RVA_SUMMON_STRING_FULL                 ADDR(        0x178960,          MAC_INLINED)
#define RVA_LAST_SPAWNED_UNIT                  ADDR(        0x1ae8fac,         0x101cb6e28)
#define RVA_LAST_SPAWNED_COMMANDER             ADDR(        0x1ae88e4,         0x100872b50)
#define RVA_NEW_WORLD_POPULATE_CALL            ADDR(        0x3054b9,          0x10058c7ac)
#define RVA_NEW_WORLD_POP_CELESTIAL_CALL       ADDR(        0x3054cc,          MAC_NO_CALL)
#define RVA_NEW_WORLD_POP_SKY_CALL             ADDR(        0x3054df,          0x10058e3b0)
#define RVA_NEW_WORLD_POP_AGARTHA_CALL         ADDR(        0x3054f2,          MAC_NO_CALL)
#define RVA_NEW_WORLD_POP_INFERNO_CALL         ADDR(        0x305505,          0x1005904dc)
#define RVA_NEW_WORLD_POP_HADES_CALL           ADDR(        0x305518,          0x1005904f0)
#define RVA_NEW_WORLD_POP_AZTLAN_CALL          ADDR(        0x30552b,          0x100590504)
#define RVA_NEW_WORLD_POP_PRIMAL_CALL          ADDR(        0x30553e,          0x100590518)
#define RVA_NEW_WORLD_POP_ELEMENTAL_CALL       ADDR(        0x305551,          0x10059052c)
#define RVA_NEW_WORLD_POP_VOID_CALL            ADDR(        0x305564,          MAC_NO_CALL)
#define RVA_NEW_WORLD_POP_AFTER_VOID_CALL      ADDR_DIFFERS(0x305564,          0x100590db0)
#define RVA_POPULATE_ELYSIUM                   ADDR(        0x2558a0,          0x10040c29c)
#define RVA_POPULATE_ELYSIUM_CASE_CHECK        ADDR(        0x255a68,          0x10040c4c4)
#define RVA_POPULATE_ELYSIUM_CASES             ADDR_DIFFERS(0x25ab44,          0x100758e3e)
#define RVA_POPULATE_CELESTIAL                 ADDR(        0x279ff0,          MAC_INLINED)
#define RVA_POPULATE_SKY                       ADDR(        0x299a40,          0x100451aa4)
#define RVA_POPULATE_AGARTHA                   ADDR(        0x273120,          MAC_INLINED)
#define RVA_POPULATE_INFERNO                   ADDR(        0x28bf20,          0x10043af50)
#define RVA_POPULATE_HADES                     ADDR(        0x27c220,          0x10044699c)
#define RVA_POPULATE_AZTLAN                    ADDR(        0x275230,          0x10045d49c)
#define RVA_POPULATE_PRIMAL                    ADDR(        0x293940,          0x100458958)
#define RVA_POPULATE_ELEMENTAL                 ADDR(        0x285090,          0x10042df60)
#define RVA_POPULATE_VOID                      ADDR(        0x29eb90,          MAC_INLINED)
#define RVA_POPULATE_VOID_POOL                 ADDR(        0x29fc10,          0x10045075c)
#define RVA_POPULATE_WANDERERS                 ADDR(        0x25aec0,          MAC_INLINED)
#define RVA_POPULATE_WANDERER_SQUARE           ADDR(        0x2ad460,          MAC_INLINED)
#define RVA_POPULATE_WANDERER_SQUARE_CALL      ADDR(        0x25a992,          0x10042bbe0)
#define RVA_RANDOM_RITUAL_SQUARE_EMPTY_TEST    ADDR(        0x2ad82e,          0x1004b3f40)
#define RVA_POPULATE_ELYSIUM_FILTER            ADDR(        0x255a13,          0x10040c480)
#define RVA_POPULATE_CELESTIAL_FILTER          ADDR(        0x27a0c8,          MAC_INLINED)
#define RVA_POPULATE_SKY_FILTER                ADDR(        0x299b3b,          0x100451bd8)
#define RVA_POPULATE_AGARTHA_FILTER            ADDR(        0x273206,          MAC_INLINED)
#define RVA_POPULATE_INFERNO_FILTER            ADDR(        0x28c00d,          0x10043b120)
#define RVA_POPULATE_INFERNO_CASES             ADDR_DIFFERS(0x291c88,          0x10075969c)
#define RVA_POPULATE_INFERNO_BELPHEGOR         ADDR(        0x28c97e,          0x10043b660)
#define RVA_POPULATE_INFERNO_SIN               ADDR(        0x28da34,          0x10043b6dc)
#define RVA_POPULATE_INFERNO_ABADDON           ADDR(        0x28ed53,          0x10043c428)
#define RVA_POPULATE_INFERNO_BUER              ADDR(        0x28fcc8,          0x10043b81c)
#define RVA_POPULATE_INFERNO_GERYON            ADDR(        0x290c05,          0x10043c4a8)
#define RVA_POPULATE_INFERNO_CAPITOLIUM        ADDR(        0x29104f,          0x10043c5a4)
#define RVA_POPULATE_INFERNO_MINE_POOL         ADDR(        0x291de0,          0x10043ac84)
#define RVA_POPULATE_INFERNO_DEVIL_POOL        ADDR(        0x292790,          MAC_INLINED)
#define RVA_POPULATE_INFERNO_VILLAGE_POOL      ADDR(        0x292ac0,          0x10043a930)
#define RVA_POPULATE_INFERNO_OVEN_POOL         ADDR(        0x292cc0,          MAC_INLINED)
#define RVA_POPULATE_INFERNO_SINNERS_POOL      ADDR(        0x292ff0,          MAC_INLINED)
#define RVA_POPULATE_INFERNO_CITY_POOL         ADDR(        0x293390,          0x1004399ec)
#define RVA_POPULATE_HADES_CITADEL             ADDR(        0x27e091,          0x100447418)
#define RVA_POPULATE_HADES_FILTER              ADDR(        0x27c32a,          0x100446ae4)
#define RVA_POPULATE_AZTLAN_FILTER             ADDR(        0x275312,          0x10045d5c8)
#define RVA_POPULATE_PRIMAL_FILTER             ADDR(        0x293a36,          0x100458a88)
#define RVA_POPULATE_ELEMENTAL_FILTER          ADDR(        0x285186,          0x10042e0c0)
#define RVA_POPULATE_NEXUS_FILTER              ADDR(        0x285615,          0x100435300)
#define RVA_POPULATE_VOID_FILTER               ADDR(        0x29ec82,          MAC_INLINED)
#define RVA_POPULATE_SET_HOLDIT                ADDR(        0x26ce80,          0x1003f8af8)
#define RVA_POPULATE_ITEM_CHANCE               ADDR(        0x26c7c0,          MAC_INLINED)
#define RVA_POPULATE_FOLLOWERS                 ADDR(        0x26c990,          0x1003f8e60)
#define RVA_POPULATE_FUNGUS                    ADDR(        0x26cba0,          0x1003f935c)
#define RVA_POPULATE_COMMANDERS                ADDR(        0x272b90,          MAC_INLINED)
#define RVA_POPULATE_IMPERIAL_STATUE           ADDR(        0x25ad30,          0x10040c068)
#define RVA_SQUARE_COLD_TEST                   ADDR(        0x1b1300,          MAC_INLINED)
#define RVA_DIE_OPEN_ENDED                     ADDR(        0x058ee0,          0x100336910)
#define RVA_DICE_OPEN_ENDED                    ADDR(        0x058dc0,          0x100336a64)
#define RVA_DICE                               ADDR(        0x058b30,          MAC_INLINED)
#define RVA_EXECUTE_RITUAL_STRING_PICK         ADDR(        0x2c61c2,          MAC_NOT_LOOKED_UP)
#define RVA_EXECUTE_RITUAL_SUMMON_CALL         ADDR(        0x2c6941,          0x100560d74)
#define RVA_EXECUTE_RITUAL_FARSUMMON_CALL      ADDR(        0x2c69f2,          0x10055f9b0)
#define RVA_RITUAL_SUMMON_STRING               ADDR(        0x2b3700,          0x10055cd1c)
#define RVA_RITUAL_SUMMON_PERCENT_CURVE        ADDR(        0x2b3857,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_CONTROL_ROLL         ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_SUMMON_UNITS                ADDR(        0x2d6330,          0x100357f50)
#define RVA_RITUAL_SUMMON_CONTROL_CHECK        ADDR(        0x2d6945,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_FAR_SUMMON_UNITS            ADDR(        0x2d7790,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_OPEN_DICE                   ADDR(        0x058b90,          0x100336d68)
#define RVA_RITUAL_SUMMON_PART_TRACE_CALL      ADDR(        0x2b3975,          0x10055cfc8)
#define RVA_RITUAL_SUMMON_DICE_CALL            ADDR(        0x2b3edc,          0x10055d184)
#define RVA_RITUAL_STRING_COPY_CALL            ADDR(        0x2c635a,          0x10055ed98)
#define RVA_STRING_COPY                        ADDR_DIFFERS(0x05e7d0,          0x1005ae314)
#define RVA_GET_START_RITUALS                  ADDR(        0x2c0420,          MAC_NOT_LOOKED_UP)
#define RVA_GET_START_RITUALS_CONTROLLER       ADDR(        0x2c0500,          MAC_NOT_LOOKED_UP)
#define RVA_START_RITUAL_SLOTS                 ADDR(        0x406dc0,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_MAY_LEARN                   ADDR(        0x2bcb40,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_LEARN_RITUAL             ADDR(        0x2be3a0,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_KNOWS_RITUAL             ADDR(        0x2be630,          MAC_NOT_LOOKED_UP)
#define RVA_LEARN_LEVEL_RITUALS                ADDR(        0x2c0250,          MAC_NOT_LOOKED_UP)
#define RVA_LEARN_NEW_SCHOOL_RITUALS           ADDR(        0x2bc820,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_MONSTER_WITH_ABILITY        ADDR(        0x1d9420,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_NEWRIT_CALL                 ADDR(        0x2c6c5a,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_GAINRIT_CALL                ADDR(        0x2c8a7f,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_TRANSFORMTARG               ADDR(        0x2c8562,          MAC_NOT_LOOKED_UP)
#define RVA_LIBRARY_MASTERY_LEARN_CALL         ADDR(        0x2de978,          MAC_NOT_LOOKED_UP)
#define RVA_LIBRARY_MASTERY_COST               ADDR(        0x2d0ca0,          MAC_NOT_LOOKED_UP)
#define RVA_PROMOTE_GET_START_RITUALS_CALL     ADDR(        0x1103ec,          MAC_NOT_LOOKED_UP)
#define RVA_NEW_COMMANDER_START_RITUALS_CALL   ADDR(        0x128aa5,          MAC_NOT_LOOKED_UP)
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
#define RVA_EXECUTE_RITUAL_SIGHT_CALL          ADDR(        0x2c9eb0,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_SIGHT_WHOLE_MAP             ADDR(        0x1b1b50,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_SIGHT_HUMAN_CONTROLLER      ADDR(        0x19b8a0,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_DRAW_SQUARES                ADDR(        0x1b2890,          0x1004a4450)
#define RVA_CAST_RITUAL_BY_NAME                ADDR(        0x2be420,          0x10055d7fc)
#define RVA_RITUAL_SCREEN_EXECUTE_CALL         ADDR(        0x2b0801,          0x10036127c)
#define RVA_REPEAT_RITUAL_EXECUTE_CALL         ADDR(        0x2c2506,          0x100566700)
#define RVA_REPEAT_RITUAL_EXECUTE_CALL_B       ADDR(        WIN_NO_EQUIVALENT, 0x1005667d0)
#define RVA_REPEAT_RITUAL_EXECUTE_CALL_C       ADDR(        WIN_NO_EQUIVALENT, 0x1005668a0)
#define RVA_REPEAT_RITUAL_EXECUTE_CALL_D       ADDR(        WIN_NO_EQUIVALENT, 0x100566940)
#define RVA_CAST_BY_NAME_EXECUTE_CALL          ADDR(        0x2be47e,          0x10055d898)
#define RVA_ELYSIUM_EXECUTE_CALL_A             ADDR(        0x255f3b,          0x10041bdf4)
#define RVA_ELYSIUM_EXECUTE_CALL_B             ADDR(        0x255f8f,          0x10041dc98)
#define RVA_ELYSIUM_EXECUTE_CALL_C             ADDR(        0x2561e7,          0x10041f974)
#define RVA_STARS_RELIABILITY                  ADDR(        0xf07b00c,         0x11307d7e0)
#define RVA_STARS_POWER                        ADDR(        0xf07b00e,         0x11307d7e2)
#define RVA_STARS_MONTHS                       ADDR(        0xf07b010,         0x11307d7e4)
#define RVA_STARS_FLAGS                        ADDR(        0xf07b014,         0x11307d7e8)
#define RVA_STARS_READING_TEXT                 ADDR(        0x2dcdb0,          MAC_NOT_LOOKED_UP)
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
#define RITUAL_FX_PROMPLAYER                   1079
#define RVA_FAILPLAYER_CLAMP                   ADDR(        0x2c65ac,          0x10055f954)
#define RITUAL_FX_FARSUMMON                    968
#define MONSTER_ABILITY_CTRLCHANCE             0x321
#define RVA_UNIT_NO_COMMANDER_FLAG             ADDR(        0x1d9d50,          MAC_INLINED)
#define RVA_RITUAL_SUMMON_STRING               ADDR(        0x2b3700,          0x10055cd1c)
#define RVA_RITUAL_SUMMON_CALL                 ADDR(        0x2c6941,          0x100560d74)
#define RVA_RITUAL_FARSUMMON_CALL              ADDR(        0x2c69f2,          0x10055f9b0)
#define RVA_SUMMON_TO_UNIT                     ADDR(        0x2d6330,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_TO_UNIT_LINK                ADDR(        0x2d69b2,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_TO_UNIT_FAIL_24             ADDR(        0x2dcd00,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMON_AT_SQUARE                   ADDR(        0x2d7790,          MAC_NOT_LOOKED_UP)
#define RVA_SUMMONED_UNIT_SETUP                ADDR(        0x2d3720,          MAC_NOT_LOOKED_UP)
#define RVA_FIND_MONSTER_RANDOM                ADDR(        0x1da4a0,          MAC_NOT_LOOKED_UP)
#define RVA_MONSTER_UNIT_FIRST                 ADDR(        0x106e00,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_PROMOTE_EXISTING            ADDR(        0x2b29d0,          MAC_NOT_LOOKED_UP)
#define RVA_MAP_X_BOUND                        RVA_MAP_DEPTH
#define RVA_LAST_TERRAIN                       ADDR(        0x1b15ac4,         0x1059bcc7c)
#define RVA_TERRAIN_NAME                       ADDR(        0x2bedd0,          0x100555010)
#define RVA_TERRAIN_NAME_TEXT                  ADDR(        0x5835fb0,         0x10c81c120)
#define TERRAIN_NAME_TEXT_SIZE                 0x64
#define RVA_SQUARE_TERRAIN                     ADDR(        0x3358a0,          MAC_INLINED)
#define RVA_TERRAIN_MATCH                      ADDR(        0x2c4680,          0x1005550c4)
#define RVA_TERRAIN_NEAR_COUNT                 ADDR(        0x19a0d0,          0x1004a9018)
#define RITUAL_FX_NONEARBY1REQ                 1003
#define RVA_RITUAL_STRING_DECODE               ADDR(        0x2b4280,          0x10055b160)
#define RVA_COUNT_MONSTERS_AT                  ADDR(        0x11f6c0,          0x10046d528)
#define RVA_COUNT_MONSTERS_HOME_AT             ADDR(        0x106b90,          MAC_INLINED)
#define RVA_COUNT_MONSTERS_ON_PLANE            ADDR(        0x106c90,          0x10047eca4)
#define RVA_COUNT_PLAYER_MONSTERS              ADDR(        0x129600,          0x10047ee24)
#define RVA_ENCHANT_COUNT                      ADDR(        0x2cfab0,          0x100556458)
#define RVA_FIND_ENCHANT                       ADDR(        0x2c1d70,          0x10055b404)
#define RVA_ENCHANT_FX_SUM                     ADDR(        0x2c1a90,          0x100565c14)
#define RVA_ENCHANT_COUNT_KINDS                ADDR(        0x2cd370,          MAC_NOT_LOOKED_UP)
#define RVA_TERRAIN_COUNT_IN_BOX               ADDR(        0x335740,          MAC_NOT_LOOKED_UP)
#define RVA_ENCHANT_FIND                       ADDR(        0x2c1d70,          0x10055b404)
#define RVA_SQUARE_FORT_PARTS                  ADDR(        0x12b620,          0x100475064)
#define RVA_FORT_WORD_MATCH                    ADDR(        0x1c8a60,          0x1005399ec)
#define RVA_FORT_WORD_CALLS                    ADDR(        0x1c572f,          0x100538818)
#define FORT_WORD_CALLS_SIZE                   0x100
#define RVA_FIND_UNIT_WITH_ABILITY_AT          ADDR(        0x117160,          0x100480d94)
#define RVA_UNIT_RESISTANCES                   ADDR(        0x32aa90,          0x100567620)
#define RVA_UNIT_BENEFITS                      ADDR(        0x321250,          0x10056798c)
#define RVA_COMMANDER_ABILITY_TOTAL            ADDR(        0x129760,          0x100468574)
#define RVA_COUNT_PLAYER_PORTALS               ADDR(        0x2519b0,          0x100571298)
#define RVA_SQUARE_PORTAL_AT                   ADDR(        0x241640,          MAC_INLINED)
#define SQUARE_PORTAL                          0x3e
#define RVA_AI_REVEAL_PORTAL_EXITS             ADDR(        0x251b00,          0x100571908)
#define RVA_TURN_START_REVEAL_PORTALS_CALL     ADDR(        0x167d9e,          MAC_INLINED)
#define RVA_STEP_REVEAL_PORTALS_CALL           ADDR(        0x1e02f1,          0x10059add8)
#define RVA_REVEAL_PORTAL_EXITS                ADDR(        0x242620,          MAC_NOT_LOOKED_UP)
#define RVA_WAYTOGO_BACK_SEEN_READ             ADDR(        0x1dbb14,          MAC_NOT_LOOKED_UP)
#define RVA_SPREAD_PORTAL_SEEN_READ            ADDR(        0x1dc2d0,          MAC_INLINED)
#define RVA_FAR_CHECK_SEEN_READ                ADDR(        0x1dc54e,          MAC_NOT_LOOKED_UP)
#define RVA_FAR_CHECK_IN_VIEW_READ             ADDR(        0x1dc5b4,          MAC_NOT_LOOKED_UP)
#define RVA_STEP_PORTAL_SEEN_READ              ADDR(        0x1dfd95,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_SQUARE_SEEN_READ            ADDR(        0x123ba3,          MAC_NOT_LOOKED_UP)
#define RVA_WANDERER_TRUE_SEEN_READ            ADDR(        0x16a61d,          MAC_NOT_LOOKED_UP)
#define RVA_WANDERER_TRUE_SEEN_READ_B          ADDR(        0x16ac52,          MAC_NOT_LOOKED_UP)
#define RVA_TOP_UNIT_SEEN_READ                 ADDR(        0x1b20e7,          MAC_NOT_LOOKED_UP)
#define RVA_TOP_UNIT_SEEN_READ_B               ADDR(        0x1b210f,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_PORTAL_SEEN_WRITE           ADDR(        0x2c742c,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_PORTAL_SEEN_WRITE_B         ADDR(        0x2c75bf,          MAC_NOT_LOOKED_UP)
#define RVA_RITUAL_PORTAL_SEEN_WRITE_C         ADDR(        0x2c75ec,          MAC_NOT_LOOKED_UP)
#define RVA_SET_SEEN_DISC                      ADDR(        0x2db1b0,          0x10035a144)
#define RVA_METEOR_SEEN_WRITE                  ADDR(        0x12e3b0,          MAC_INLINED)
#define RVA_UPDATE_VOID                        ADDR(        0x252d10,          MAC_NOT_LOOKED_UP)
#define RVA_UPDATE_VOID_SEEN_CLEAR             ADDR(        0x252df2,          MAC_NOT_LOOKED_UP)
#define RVA_CLOUD_OWNER_MARK_TEST              ADDR(        0x12fee8,          MAC_NOT_LOOKED_UP)
#define RVA_AUTO_RECRUIT_AGING_START           ADDR(        0x167713,          MAC_INLINED)
#define RVA_AUTO_RECRUIT_AGING_END             ADDR(        0x1677d9,          MAC_INLINED)
#define RVA_SITEPOP                            ADDR(        0x172bc0,          0x10053ee14)
#define RVA_SPAWN_CARRY                        ADDR(        0x5822f8c,         MAC_NOT_LOOKED_UP)
#define RVA_REMOVE_MONSTERS_AT                 ADDR(        0x0f7240,          0x1003b8db0)
#define RVA_COUNT_TERRAIN_AROUND               ADDR(        0x32d840,          MAC_NOT_LOOKED_UP)
#define PORTAL_INFERNO_GATE                    30
#define TERRAIN_MAGMA                          53
#define TERRAIN_FROZEN_WASTE                   54
#define TERRAIN_INFERNAL_WASTE                 55
#define TERRAIN_CODE_CITADEL                   (-23)
#define RVA_WANDER_ABILITY_TEST_29              ADDR(        0x17b4a0,           0x10038c0d8)
#define RVA_WANDER_ABILITY_TEST_24              ADDR(        0x17b7ed,           0x10038c10c)
#define RVA_WANDER_ABILITY_TEST_28              ADDR(        0x17bafa,           0x10038c130)
#define RVA_WANDER_ABILITY_TEST_27              ADDR(        0x17be0a,           0x10038c154)
#define RVA_WANDER_SECOND_TEST_29               ADDR(        0x17cdf9,           0x10038c8dc)
#define RVA_WANDER_SECOND_TEST_24               ADDR(        0x17ce05,           0x10038df3c)
#define RVA_WANDER_SECOND_TEST_28               ADDR_DIFFERS(0x17ce11,           0x10038c018)
#define RVA_WANDER_SECOND_TEST_27               ADDR_DIFFERS(0x17ce1d,           0x10038c024)
#define RVA_WANDER_GATE_TEST                    ADDR_DIFFERS(0x17c4d7,           0x10038c2c0)
#define RVA_WANDER_GATE_PASS                    ADDR(        0x17c51a,           0x10038c328)
#define RVA_WANDER_GATE_WAIT                    ADDR(        0x17d8a8,           0x10038c070)
#define RVA_WANDERER_AT_TEST_24                 ADDR(        0x178330,           0x100547d68)
#define RVA_WANDERER_AT_TEST_28                 ADDR(        0x1785f7,           0x100547d9c)
#define RVA_WANDERER_AT_TEST_27                 ADDR(        0x1788b0,           0x100547dd0)
#define RVA_WANDERER_AT_TEST_24_B               ADDR(        WIN_NO_EQUIVALENT,  0x10054809c)
#define RVA_WANDERER_AT_TEST_28_B               ADDR(        WIN_NO_EQUIVALENT,  0x1005480d0)
#define RVA_WANDERER_AT_TEST_27_B               ADDR(        WIN_NO_EQUIVALENT,  0x100548104)
#define RVA_AI_VIEW_SKIP_TEST_24                ADDR(        0x06e4bf,           0x1004debb8)
#define RVA_AI_VIEW_SKIP_TEST_28                ADDR(        0x06e78f,           0x1004dec04)
#define RVA_AI_VIEW_SKIP_TEST_27                ADDR(        0x06ea5f,           0x1004dec50)
#define RVA_TITHE_EXCEPTION_TEST                ADDR_DIFFERS(0x167de5,           0x100392ee0)
#define RVA_TITHE_POOL                          ADDR(        0xf07b024,          0x11307d7f8)
#define RVA_NO_EVENTS                           ADDR_DIFFERS(0x58239a0,          0x1069b16ac)
#define RVA_MONUMENT_EVENT_TURN_TEST            ADDR_DIFFERS(0x163530,           0x1003a4578)
#define RVA_RULERSHIP_CHECK                     ADDR(        0x10d910,           0x10046ce08)
#define RVA_RULERSHIP_SLOT                      ADDR(        0x44194c,           0x100872b98)
#define RVA_COUNT_UNITS_WITH_ABILITY            ADDR(        0x1067d0,           MAC_INLINED)
#define RVA_SQUARE_SHOWN_NAME                   ADDR(        0x1afde0,           0x1004a923c)
#define RVA_CARAVAN_EVENT_TURN_TEST             ADDR_DIFFERS(0x1665e4,           0x1003b2650)
#define RVA_SPAWN_HORROR_GROUP                  ADDR(        0x2a06e0,           0x100450398)
#define RVA_SPAWN_HORROR_SINGLE                 ADDR(        0x2a0910,           MAC_INLINED)
#define RVA_HORROR_SINGLE_POPULATE_CALL         ADDR(        0x2a09df,           MAC_INLINED)
#define RVA_POPHORRORS_SINGLE_CALL              ADDR_DIFFERS(0x142cc8,           0x1003b871c)
#define RVA_POPHORRORS_GROUP_CALL               ADDR(        0x142e64,           0x1003b8af4)
#define RVA_INDEP_AVOID_MOVER_TEST              ADDR(        0x16dac9,           0x10054761c)
#define RVA_INDEP_AVOID_TARGET_TEST             ADDR(        0x16dace,           0x100547624)
#define RVA_INDEP_AVOID_SKIP                    ADDR_DIFFERS(0x16dae1,           0x100547640)
#define RVA_INDEP_AVOID_ALLOW                   ADDR_DIFFERS(0x16dae1,           0x10054763c)
#define RVA_INDEP_AVOID_REFUSE                  ADDR(        0x16db48,           0x100546f9c)
#define RVA_EVENT_SQUARE_TEST_A                 ADDR(        0x1635c2,           0x1003a4578)
#define RVA_EVENT_SQUARE_TEST_B                 ADDR(        0x163720,           0x1003a49cc)
#define RVA_EVENT_SQUARE_TEST_C                 ADDR(        0x164418,           0x1003aa0a0)
#define RVA_EVENT_ARMY_TEST                     ADDR(        0x164736,           0x1003a9904)
#define RVA_EVENT_SQUARE_TEST_D                 ADDR(        0x164761,           0x1003a9948)
#define RVA_EVENT_SQUARE_TEST_E                 ADDR(        0x166432,           0x1003b0ff0)
#define RVA_METEOR_OWNER_TEST                   ADDR(        0x12dfb1,           MAC_IN_CODE)
#define RVA_RANDOM_SQUARE_OWNER_TEST            ADDR(        0x123c47,           0x100469d3c)
#define RVA_CONQUER_SQUARE_OWNER_TEST           ADDR(        0x1aed9f,           0x1004aa784)
#define RVA_WANDERER_TARGET_CITY_TEST           ADDR(        0x16d0ee,           0x100549ff8)
#define RVA_IMPERIAL_STATUE_TEST                ADDR(        0x25ae5e,           0x10040c1c0)
#define RVA_MONUMENT_OWNER_TEST                 ADDR(        0x10d9cf,           0x10046d000)
#define RVA_GOD_VESSEL_OWNER_TEST               ADDR(        0x10da7f,           0x10046d130)
#define RVA_GOD_VESSEL_OWNER_TEST_B             ADDR(        0x10db15,           0x10046d264)
#define RVA_POPHORRORS_OWNER_TEST               ADDR(        0x142caa,           0x1003b88cc)
#define RVA_HORRORS_OWNER_TEST_B                ADDR(        0x14304b,           0x1003a69a4)
#define RVA_HORRORS_OWNER_TEST_C                ADDR(        0x14322b,           MAC_NOT_FOUND)
#define RVA_SITEPOP_HADES_OWNER_TEST            ADDR(        0x172b95,           MAC_NOT_FOUND)
#define RVA_PLANE_ABILITY_SUM                  ADDR(        0x251c90,          0x1005751c8)
#define ABILITY_MONUMENT                       0x8a
#define RVA_PORTALS_LINKED_TO                  ADDR(        0x250e20,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_FOLLOWERS_NAMED          ADDR(        0x106510,          MAC_NOT_LOOKED_UP)
#define RVA_COMMANDER_RELEASE_FOLLOWERS        ADDR(        0x106430,          MAC_NOT_LOOKED_UP)
#define RVA_UNIT_TRACE_NAME                    ADDR(        0x0f96e0,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_SACRIFICE_TAKEN             ADDR(        0x2db700,          0x1003563dc)
#define ABILITY_SINNER_CARRIER                 0x4c8
#define TERRAIN_DESTROYED_GATEWAY              200
#define TERRAIN_INFERNO_GATEWAY                201
#define TERRAIN_INFERNO_GATEWAY_EXIT           203
#define TERRAIN_CODE_INFERNAL_TARGET           (-14)
#define OBJECTIVE_INFERNAL_WANDERER            6
#define OBJECTIVE_HADES_WANDERER               7
#define RVA_RITUAL_LISTED                      ADDR(        0x2b4930,          MAC_INLINED)
#define RVA_GET_UNIT_HP                        ADDR(        0x329580,          MAC_INLINED)
#define RVA_UNIT_HOME_X                        ADDR(        0xfb8e8da,         0x11004997a)
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
#define RVA_FORMAT_AREA_SHAPE                  ADDR(        0x1cfac0,          0x100520d64)

// ---- Input --------------------------------------------------------------------------------------
#define RVA_POLL_SDL_EVENTS                    ADDR(        0x04eaa0,          MAC_INLINED)
#define RVA_GET_MOUSE_X                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_GET_MOUSE_Y                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_MOUSE_Y_UNFLIPPED                  ADDR(        0x57efc54,         MAC_IN_CODE)
#define RVA_WINDOW_FULLSCREEN_MODE             ADDR(        0x593325c,         0x113997040)
#define RVA_MOUSE_Y_UNFLIPPED_OFF              RVA_WINDOW_FULLSCREEN_MODE
#define RVA_GET_FULLSCREEN_MODE                ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_FULLSCREEN_SETTING                 ADDR(        0x1a46364,         0x1008575e0)
#define RVA_GET_MOUSE_WHEEL                    ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_GET_MOUSE_BUTTONS                  ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_WINDOW_W                           ADDR(        0x12ab03dc,        0x113997160)
#define RVA_WINDOW_H                           ADDR(        0x12ab03e4,        0x113997060)
#define RVA_SDL_WINDOW_W                       ADDR(        0x59937e0,         0x11399715c)
#define RVA_SDL_WINDOW_H                       ADDR(        0x593afcc,         0x113997164)
#define RVA_WHEEL                              ADDR(        0x57f0654,         0x113997068)
#define RVA_KEY_STATE                          ADDR(        0x12ab0240,        0x113997168)
#define RVA_KEY_CODES                          ADDR(        0x12ab01a4,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_F1                        ADDR_DIFFERS(0x12ab01ac,        0x113997100)
#define RVA_MOTION_X                           ADDR(        0x57efc1c,         0x11399705c)
#define RVA_MOTION_Y                           ADDR(        0x57f06b0,         0x113997064)
#define RVA_PRESS_X                            ADDR(        0x439e7c,          0x105aa402c)
#define RVA_PRESS_Y                            ADDR(        0x43a2d4,          0x105aa4030)
#define RVA_BUTTONS_HELD                       ADDR(        0x57f068c,         0x113997070)
#define RVA_PRESSED                            ADDR(        0x57f0670,         0x11399706c)
#define RVA_WIN_W                              RVA_WINDOW_W
#define RVA_WIN_H                              RVA_WINDOW_H
#define RVA_SDL_SCALE_W                        RVA_SDL_WINDOW_W
#define RVA_SDL_SCALE_H                        RVA_SDL_WINDOW_H
#define RVA_PLAYER1_NAME                       RVA_SLOT_NAME
#define RVA_HOVER_SLOT                         ADDR(        0x42dd34,          0x100862694)
#define RVA_MOUSE_BUTTONS                      ADDR(        0x5835730,         0x10c96df4c)
#define RVA_CLICK_LATCH                        ADDR_DIFFERS(0x57ef1a4,         0x10611be5c)
#define RVA_UI_MOUSE_X                         ADDR(        0x5835734,         0x10c96df50)
#define RVA_UI_MOUSE_Y                         ADDR(        0x583572c,         0x10c96df54)
#define RVA_ELEMENT_AT                         RVA_FIND_HOVERED
#define RVA_DRAW_CALL                          ADDR(        0x24084f,          0x10048aba8)
#define RVA_DRAW_PASS                          ADDR(        0x02cfc0,          0x100345b20)
#define RVA_HIT_TEST_CALL                      ADDR(        0x23f810,          MAC_NO_CALL)
#define RVA_HIT_TEST                           ADDR(        0x02a890,          MAC_INLINED)
#define RVA_SET_SDL_WINDOW_SIZE                ADDR(        0x04fd10,          0x1005acb8c)
#define RVA_INPUT_INIT                         ADDR(        0x04df90,          0x1005ad02c)
#define RVA_DRAG_OFFSET                        ADDR(        0x12ab2b00,        0x10611be58)
#define RVA_RESIZE_WINDOW_SIZE                 ADDR(        0x050086,          0x1005ace80)
#define RVA_INPUT_INIT_KEY_CLEAR               ADDR(        0x04dfd3,          0x1005ad070)
#define RVA_ESCAPE_STORE                       ADDR_DIFFERS(0x04e5b2,          0x1005ad5a0)
#define RVA_BIND_HOTKEY_ESCAPE_READ            ADDR(        0x03ab46,          0x10033ca48)
#define RVA_PRESS_SCALE                        ADDR(        0x04f249,          0x100485764)
#define RVA_PRESS_DIVISOR                      ADDR(        0x04f254,          0x100485778)
#define RVA_WHEEL_ADD                          ADDR_DIFFERS(0x04f1e9,          0x100485438)
#define RVA_WAIT_WHEEL_READ                    ADDR_DIFFERS(0x23f68c,          0x1004861a8)
#define RVA_WAIT_UI_MOUSE                      ADDR(        0x23f638,          0x100486160)
#define RVA_WAIT_FRAME_BUTTONS                 ADDR(        0x23f6d7,          0x1004861d4)
#define RVA_WAIT_SQUARE_RESET                  ADDR(        0x23f6dd,          0x1004861e0)
#define RVA_ELEMENT_AT_CALL_1                  ADDR(        0x02a960,          0x1004863a8)
#define RVA_HOVER_STORE                        ADDR(        0x02a968,          0x1004863b0)
#define RVA_ELEMENT_AT_CALL_0                  ADDR(        0x02a992,          0x1004863c4)
#define RVA_WAIT_GAME_OVER_FX_1                ADDR(        0x23fa4e,          0x100487aec)
#define RVA_WAIT_GAME_OVER_FX_2                ADDR(        0x23fa71,          0x100487b10)
#define RVA_WAIT_BANNER_FADE_STEP              ADDR(        0x23fe50,          0x100488a2c)
#define RVA_WAIT_LOGO_CALL                     ADDR(        0x23fee1,          0x100488a94)

#define RVA_TEXT_EVENT_FIRST_BYTE              ADDR(        0x04eb87,          0x100485648)
#define RVA_TEXT_ACCENT_STRINGS                ADDR(        0x439704,          MAC_IN_CODE)
#define RVA_TEXT_ACCENT_CODES                  ADDR(        0x12ab0208,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_DOWN                           ADDR(        0x04f460,          MAC_INLINED)
#define RVA_KEY_SYM_TO_CODE                    ADDR(        0x04f8a0,          0x1005ac8e8)
#define RVA_KEY_BUFFER                         ADDR(        0x5993800,         0x113997074)
#define RVA_KEY_BUFFER_LAST                    ADDR(        0x439dc4,          0x105aa4034)
#define RVA_KEY_SUPPRESS_CODE                  ADDR(        0x57f0688,         0x113997158)
#define RVA_POP_KEY                            ADDR(        0x0503b0,          MAC_INLINED)
#define RVA_HIT_TEST_POP_KEY_1                 ADDR_DIFFERS(0x02ac99,          0x1004866b0)
#define RVA_HIT_TEST_POP_KEY_2                 ADDR_DIFFERS(0x02b753,          0x1004872c8)
#define RVA_GUI_KEYPRESS                       ADDR(        0x431828,          0x100857630)
#define RVA_GUI_KEYPRESS_MORE                  ADDR(        0x432208,          0x100857634)
#define RVA_TEXT_INPUT_START                   ADDR(        0x050280,          MAC_INLINED)
#define RVA_TEXT_INPUT_STOP                    ADDR(        0x050210,          MAC_INLINED)
#define RVA_TEXT_FIELD                         ADDR(        0x038750,          0x10033e32c)
#define RVA_NAME_BOX_TEXT_FIELD_CALL           ADDR(        0x041dda,          0x10034a228)
#define RVA_PLAYER_NAME_MAX_LEN                ADDR_DIFFERS(0x305168,          0x10034a218)

// ---- Menus and keys -------------------------------------------------------------------------------
#define RVA_BANNER_FADE                        ADDR(        0x1ab8644,         0x101cce55c)
#define RVA_SHORTCUTS_TABLE                    ADDR(        0x3f9f10,          0x100837fd0)
#define RVA_HOTKEY_TABLE                       ADDR(        0x1319a700,        0x10611b994)
#define HOTKEY_SIZE                            8
#define RVA_HOTKEY_COUNT                       ADDR(        0x42dca4,          0x100857624)
#define RVA_HOTKEY_COUNT_RESET                 ADDR(        0x03af91,          0x10033c0b8)
#define RVA_HOTKEY_LOOKUP                      ADDR(        0x02b581,          0x100486864)
#define RVA_MAP_BIND_SHORTCUTS_LIST            ADDR(        0x096e85,          0x10037fc98)
#define RVA_MAP_BIND_RECRUIT                   ADDR(        0x09529f,          0x10037f428)
#define RVA_MAP_BIND_END_TURN                  ADDR(        0x095615,          0x10037f4a0)
#define RVA_MAP_BIND_MESSAGES                  ADDR(        0x0957a1,          0x10037f4c4)
#define RVA_MAP_BIND_NEXT_COMMANDER            ADDR(        0x095210,          0x10037f41c)
#define RVA_MAP_BIND_PREVIOUS_COMMANDER        ADDR(        0x09747f,          0x10037fd90)
#define RVA_MAP_BIND_COMMANDER_DONE            ADDR(        0x0959af,          0x10037f4f4)
#define RVA_MAP_BIND_TRANSFER                  ADDR(        0x095323,          0x10037f434)
#define RVA_MAP_BIND_SPECIAL_POWER             ADDR(        0x0953af,          0x10037f440)
#define RVA_MAP_BIND_UNIT_OVERVIEW             ADDR(        0x096fab,          0x10037fcd0)
#define RVA_MAP_BIND_PLAYER_OVERVIEW           ADDR(        0x096f0f,          0x10037fcb4)
#define RVA_MAP_BIND_CLASS_DESCRIPTION         ADDR(        0x09718a,          0x10037fd24)
#define RVA_MAP_BIND_MAP_DESCRIPTION           ADDR(        0x09722a,          0x10037fd40)
#define RVA_MAP_BIND_SCORE_GRAPHS              ADDR(        0x0972ca,          0x10037fd5c)
#define RVA_MAP_BIND_ACTIVE_MODS               ADDR(        0x09736a,          0x10037fd78)
#define RVA_MAP_BIND_GAME_MENU_F10             ADDR(        0x09543a,          0x10037f45c)
#define RVA_MAP_BIND_GAME_MENU_F11             ADDR(        0x0954da,          0x10037f478)
#define RVA_MAP_BIND_GAME_MENU_F12             ADDR(        0x09557a,          0x10037f494)
#define RVA_MAP_BIND_F9                        ADDR(        0x097050,          0x10037fcec)
#define RVA_MAP_BIND_MENU_KEY                  ADDR(        0x0970ed,          0x10037fd08)
#define RVA_MAP_BIND_TERRAIN_INFO              ADDR(        0x0974ff,          0x10037fd9c)
#define RVA_MAP_BIND_COMMANDER_STATS           ADDR(        0x09569a,          0x10037f4ac)
#define RVA_MAP_BIND_TRADE                     ADDR(        0x095821,          0x10037f4d0)
#define RVA_MAP_BIND_SENTRY                    ADDR(        0x0958a6,          0x10037f4dc)
#define RVA_MAP_BIND_LOCATION_POWER            ADDR(        0x09592a,          0x10037f4e8)
#define RVA_MAP_BIND_BURN_FOREST_SCREEN        ADDR(        0x096cd1,          0x10037fb80)
#define RVA_MAP_BIND_BURN_FOREST_NOW           ADDR(        0x0973ff,          0x10037fd84)
#define RVA_MAP_BIND_HUNT_SLAVES               ADDR(        0x096d4f,          0x10037fb8c)
#define RVA_MAP_BIND_PLANE_SHIFT               ADDR(        0x09a042,          0x10037e010)
#define RVA_MAP_BIND_CONTINUE                  ADDR(        0x09760f,          0x10037fdb4)
#define RVA_MAP_BIND_CONTINUE_ALL              ADDR(        0x09758a,          0x10037fda8)
#define RVA_MAP_BIND_TEST_GAME                 ADDR(        0x095720,          0x10037f4b8)
#define RVA_MAP_BIND_HOME                      ADDR(        0x096dea,          0x10037fc8c)
#define RVA_MAP_BIND_SQUARE_DETAILS            ADDR_DIFFERS(0x096da5,          0x10037fc70)
#define RVA_MAP_BIND_MAP_ICONS                 ADDR_DIFFERS(0x096d98,          0x10037fbf0)
#define RVA_MAP_BIND_ASTROLOGY                 ADDR_DIFFERS(0x09da9b,          0x10037a6e4)
#define RVA_MAP_BIND_REPEAT_ORDERS             ADDR_DIFFERS(0x097658,          0x10037fe18)
#define RVA_MAP_BIND_SCRY                      ADDR_DIFFERS(0x095a2d,          0x10037f568)
#define RVA_MAP_BIND_MOVE_FIRST                ADDR(        0x095a9f,          0x10037f590)
#define RVA_MAP_BIND_LOOK_FIRST                ADDR(        0x0967c0,          0x10037f730)
#define RVA_ARMY_SCREEN                        ADDR(        0x120040,          MAC_INLINED)
#define RVA_BATTLE_SCREEN                      ADDR(        0x0ffef0,          0x100582644)
#define RVA_BATTLE_EDITOR                      ADDR(        0x084e70,          0x100518be8)
#define RVA_VIEW_ARMY_SCREEN                   ADDR(        0x1225c0,          0x10047da38)
#define RVA_UNIT_LIST                          ADDR(        0x0fa180,          MAC_INLINED)
#define RVA_SHEET_SCREEN                       ADDR(        0x03e480,          MAC_INLINED)
#define RVA_SHEET_ADD_SHORTCUT                 ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_SHEET_SHORTCUT_KEYS                ADDR(        0x12ab2a70,        0x10617f3b0)
#define RVA_SHEET_SHORTCUT_IDS                 ADDR(        0x12ab2aa0,        0x10617f3e0)
#define RVA_MISC_MENU                          ADDR(        0x0a1210,          0x1003730e4)
#define RVA_BIND_HOTKEY_NEGATIVE               ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_MAP_VIEW_UPDATE                    ADDR(        0x23d050,          MAC_INLINED)
#define RVA_BATTLE_VIEW_UPDATE                 ADDR(        0x23c970,          MAC_INLINED)
#define RVA_WAIT_TAB_READ                      ADDR(        0x24054d,          MAC_NOT_FOUND)
#define RVA_SDL_POLL_EVENT                     ADDR_DIFFERS(WIN_INLINED,       0x10000f980)
#define RVA_SDL_POLL_EVENT_SLOT                ADDR_DIFFERS(0x33a580,          0x100852020)
#define RVA_SDL_PUSH_EVENT_SLOT                ADDR(        WIN_NO_EQUIVALENT, 0x100852038)
#define RVA_SWAP_WINDOW                        ADDR(        WIN_INLINED,       MAC_INLINED)
#define RVA_WAIT_SWAP_CALL                     ADDR_DIFFERS(0x240a47,          0x10048c1e0)
#define RVA_SDL_GL_SWAP_WINDOW                 ADDR_DIFFERS(WIN_INLINED,       0x100010e5c)
#define RVA_SDL_GL_SWAP_WINDOW_SLOT            ADDR_DIFFERS(0x33a658,          0x100852de8)
#define RVA_SDL_WINDOW                         ADDR(        0x57f0658,         0x113997038)
#define RVA_FRAME_COUNT                        ADDR(        0x5835720,         0x106cee4f8)
#define RVA_KEY_CODE_BACKSPACE                 ADDR(        0x12ab0200,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_TAB                       ADDR(        0x12ab0204,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_ENTER                     ADDR(        0x12ab01ae,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_DELETE                    ADDR(        0x12ab03d6,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_PAGE_DOWN                 ADDR(        0x12ab01ee,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_PAGE_UP                   ADDR(        0x12ab03e8,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_END                       ADDR(        0x12ab020a,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_HOME                      ADDR(        0x12ab03d8,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_UP                        ADDR(        0x12ab03d4,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_DOWN                      ADDR(        0x12ab03e0,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT                      ADDR(        0x12ab03d0,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_RIGHT                     ADDR(        0x12ab03e2,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_SHIFT                ADDR(        0x12ab0218,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_CTRL                 ADDR(        0x12ab0216,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_RIGHT_CTRL                ADDR(        0x12ab01dc,        MAC_STORED_DIFFERENTLY)
#define RVA_KEY_CODE_LEFT_ALT                  ADDR(        0x12ab01f8,        MAC_STORED_DIFFERENTLY)

// ---- Map editor and the map view ----------------------------------------------------------------
#define RVA_MAP_EDITOR_ACTIVE                  ADDR_DIFFERS(0x582c6c0,         0x108810eb0)
#define RVA_MAP_EDITOR_MENU_SKIP               ADDR(        0x1b77c5,          0x100514990)
#define RVA_MAP_EDITOR_TITLE_CALL              ADDR(        0x1b7879,          0x1005149fc)
#define RVA_MAP_EDITOR_REMOVE_FANCY_CALL       ADDR(        0x1b78ec,          0x100514a74)
#define RVA_MAP_EDITOR_CREATE_FANCY_CALL       ADDR(        0x1b7909,          0x100514a90)
#define RVA_MAP_EDITOR_SETTINGS_CALL           ADDR(        0x1b791a,          0x100514aa0)
#define RVA_MAP_EDITOR_COORDS_CALL             ADDR(        0x1b7a55,          0x100514c80)
#define RVA_MAP_EDITOR_OVERLAY_DRAW_CALL       ADDR(        0x1b8e73,          0x100515640)
#define RVA_CREATE_FANCY_TERRAIN               ADDR(        0x1b4ca0,          MAC_INLINED)
#define RVA_SAVE_MAP                           ADDR(        0x1b5400,          MAC_INLINED)
#define MAP_EDITOR_FREE_ID_FIRST               0x2c
#define RVA_MAP_ZOOM                           ADDR(        0x1ab8d8c,         0x10087350c)
#define RVA_MAP_CAMERA_X                       ADDR(        0x5835728,         0x10b848af8)
#define RVA_MAP_CAMERA_Y                       ADDR(        0x5835724,         0x10b848afc)
#define RVA_MAP_ZOOM_SPEED                     ADDR(        0x5835774,         0x106cee55c)
#define RVA_MAP_VIEW_PLANE                     ADDR(        0x5835738,         0x10c921d04)
#define RVA_MAP_VIEW_DRAW_CALL                 ADDR(        0x24024e,          0x100489160)
#define RVA_DRAW_LINE_UI                       ADDR(        0x030380,          MAC_INLINED)
#define RVA_FILL_RECT_UI                       ADDR(        0x030280,          MAC_NOT_LOOKED_UP)
#define GUI_TYPE_LINE                          0x1d
#define RVA_BATTLE_GRID_ON                     ADDR(        0x43e3a4,          0x1048937d8)
#define RVA_BATTLE_FIELD_DRAW                  ADDR(        0x0fc710,          0x10057f780)
#define RVA_BATTLE_GRID_SPRITE_CALL            ADDR(        0x0fc9dd,          0x10057f9bc)
#define BATTLE_GRID_SPRITE                     0x17
#define MAP_EDITOR_FREE_ID_LAST                999
#define RVA_MAP_EDITOR_PLANE_ON                ADDR(        0x582c690,         0x108810ec0)
#define RVA_MAP_EDITOR_SETTINGS                ADDR(        0x1b4350,          MAC_INLINED)
#define RVA_MAP_EDITOR_START_WORLD_CALL        ADDR(        0x1b761e,          0x1005146dc)
#define RVA_MAP_EDITOR_RECREATE                ADDR(        0x1b8fb4,          0x100516144)
#define RVA_MAP_EDITOR_RECREATE_CALL           ADDR(        0x1b8ff4,          0x100516178)
#define RVA_MAP_EDITOR_EMPTY_PLANE_CALL        ADDR(        0x1b915d,          0x1005161f8)
#define RVA_MAP_EDITOR_UNKNOWN_PLANE_CALL      ADDR(        0x1b90ec,          0x100516220)
#define RVA_MAP_EDITOR_LOAD_CALL               ADDR(        0x1b93d6,          0x100516374)
#define RVA_MAP_EDITOR_SAVE_CALL               ADDR(        0x1b9670,          MAC_INLINED)
#define RVA_MAP_EDITOR_PLANE_BAR               ADDR(        0x1b3e20,          MAC_INLINED)
#define RVA_MAP_EDITOR_SAVE_TERRAIN_WORD       ADDR(        0x582c67c,         0x108810eac)
#define RVA_GL_STATE                           ADDR(        0x029020,          0x100350adc)
#define GL_STATE_LINES                         0x16
#define RVA_TEXT_RENDER_FLAGS                  ADDR(        0x57c5914,         0x105b68184)
#define DRAW_TEXT_SIZE_SCALE                   0.75f
#define RVA_GL_COLOR4F                         ADDR_DIFFERS(0x33a460,          0x1005ae6e0)
#define RVA_GL_LINE_WIDTH                      ADDR_DIFFERS(0x33a3f0,          0x1005ae7e8)
#define RVA_GL_ENABLE_CLIENT_STATE             ADDR_DIFFERS(0x33a4b8,          0x1005ae740)
#define RVA_GL_DISABLE_CLIENT_STATE            ADDR_DIFFERS(0x33a3f8,          0x1005ae71c)
#define RVA_GL_VERTEX_POINTER                  ADDR_DIFFERS(0x33a428,          0x1005ae8cc)
#define RVA_GL_DRAW_ARRAYS                     ADDR_DIFFERS(0x33a3d0,          0x1005ae728)

// ---- Map terrain drawing ------------------------------------------------------------------------
#define RVA_TRS_DECODE                         ADDR(        0x06a070,          MAC_INLINED)
#define RVA_TRS_DECODE_RLE                     ADDR(        0x062f10,          0x100350320)
#define RVA_TRS_DECODE_RLE_OLD                 ADDR(        0x0630d0,          0x10035013c)
#define RVA_TRS_DECODE_RAW                     ADDR(        0x063290,          MAC_INLINED)
#define RVA_SPRITE_ALPHA_MODE                  ADDR(        0x026130,          0x100339b6c)
// How a mod image loads.
#define RVA_IMAGE_TEXTURE_LOAD                 ADDR(        0x06a4d0,          0x10033b558)
#define RVA_IMAGE_READ                         ADDR(        0x0284b0,          0x100337d08)
#define RVA_TGA_DECODE                         ADDR(        0x025d80,          MAC_INLINED)
#define RVA_PNG_DECODE                         ADDR(        0x024d90,          0x100338cac)
#define RVA_IMAGE_HAS_ALPHA                    ADDR(        0x0257d0,          MAC_INLINED)
#define RVA_IMAGE_COLOUR_MODE                  ADDR(        0x026360,          0x100338eec)
#define RVA_TEXTURE_CREATE                     ADDR(        0x06a650,          0x10033aeec)
#define RVA_SPRITE_COMPRESSION                 ADDR_DIFFERS(0x57f2584,         0x106ceedf4)
#define RVA_SPRITE_CACHE_DROP                  ADDR(        0x064cd0,          MAC_INLINED)
#define SPRITE_KIND_TREE                       4
#define SPRITE_KIND_WALLS                      0x23
#define TREE_TRS_SPRITES                       943
#define TERRAIN_TRS_SPRITES                    356
// The ground layer:
#define RVA_CREATE_BACKGROUND_IMAGE            ADDR(        0x07e9b0,          0x10054dcd8)
#define RVA_GROUND_TEXTURES_LOAD               ADDR(        0x07ed60,          MAC_INLINED)
#define RVA_GROUND_TEXTURE_NAMES               ADDR(        0x440b48,          0x100847d10)
#define RVA_GROUND_TEXTURES                    ADDR(        0x123e3000,        0x10c6404f8)
#define GROUND_TEXTURE_SIZE                    0x18
#define GROUND_TEXTURE_W                       0x08
#define GROUND_TEXTURE_H                       0x0c
#define GROUND_COUNT                           29
#define RVA_GROUND_WEIGHT_PASS                 ADDR(        0x07f290,          MAC_INLINED)
#define RVA_GROUND_WEIGHT_ROWS                 ADDR(        0x07f850,          0x10054bad8)
#define RVA_GROUND_EDGE_SPREAD                 ADDR(        0x07fd20,          MAC_INLINED)
#define RVA_GROUND_WEIGHT_SUM                  ADDR(        0x084470,          MAC_INLINED)
#define RVA_GROUND_EDGE_LINES                  ADDR(        0x083b00,          MAC_INLINED)
#define RVA_GROUND_EDGE_MIDPOINT               ADDR(        0x084170,          0x10054ada4)
#define RVA_GROUND_EDGE_NOISE                  ADDR(        0x614f840,         0x10c640898)
#define RVA_GROUND_WEIGHTS                     ADDR(        0x6128650,         0x10c6407b0)
#define RVA_GROUND_COMPOSITE                   ADDR(        0x082f60,          MAC_INLINED)
#define RVA_GROUND_COMPOSITE_ROWS              ADDR(        0x0833a0,          0x10054af14)
#define RVA_MAP_BG_GROUNDS                     ADDR(        0x12389000,        0x10c77c120)
#define RVA_MAP_BG_FLOODED                     ADDR(        0x12339000,        0x10c7cc120)
#define RVA_MAP_BG_W_SQUARES                   ADDR(        0x580338c,         0x10c77c118)
#define RVA_MAP_BG_H_SQUARES                   ADDR(        0x5803388,         0x10c77c11c)
#define RVA_MAP_BG_IMAGE                       ADDR(        0x5803398,         0x10c922c70)
#define RVA_MAP_BG_TILE_DRAW                   ADDR(        0x0b3440,          MAC_INLINED)
#define RVA_MAP_BG_TILE_DRAW_CALL              ADDR(        0x19ce94,          MAC_NO_CALL)
#define SQUARE_FLAG_FLOODED                    0x2000
#define TERRAIN_FLAG_INVISIBLE                 0x1000
#define TERRAIN_FLAG_UG                        0x20000000000000
#define TERRAIN_FLAG_TEMPLE                    0x4000000
#define TERRAIN_FLAG_TOWN1                     0x1000000000000
#define TERRAIN_FLAG_TOWN2                     0x2000000000000
#define TERRAIN_ATAVI_VILLAGE                  348
#define TERRAIN_FLAG_SMOKE                     0x80
#define RVA_TERRAIN_SPR_SKIP                   ADDR(        0xd259fa0,         0x10857bd76)
#define RVA_TERRAIN_SPR_DRAW_CALL              ADDR(        0x19cf69,          0x10049b580)
// The decoration layer:
#define RVA_MAP_DECORATIONS                    ADDR(        0x1a26a0,          0x100494e28)
#define RVA_MAP_DECORATIONS_CALL               ADDR(        0x19cf86,          0x10049b594)
#define RVA_MAP_DECORATION_CASES               ADDR(        0x1a7aa0,          MAC_STORED_DIFFERENTLY)
#define RVA_DECOR_SCATTER                      ADDR(        0x1aad20,          0x1004921b0)
#define RVA_DECOR_SINGLE                       ADDR(        0x1a8950,          0x100493e30)
#define RVA_DECOR_CENTERED                     ADDR(        0x1a7c10,          0x100494c20)
#define RVA_DECOR_MOUNTAINS                    ADDR(        0x1a9390,          0x10049344c)
#define RVA_DECOR_QUEUE                        ADDR(        0xf585720,         0x1071faccc)
#define DECOR_QUEUE_ENTRY_SIZE                 0x28
#define DECOR_ENTRY_KIND                       0x00
#define DECOR_ENTRY_SPRITE                     0x02
#define DECOR_ENTRY_X                          0x04
#define DECOR_ENTRY_W                          0x0c
#define DECOR_ENTRY_H                          0x10
#define RVA_DECOR_QUEUE_LAST                   ADDR(        0x1a655a4,         0x100872bfc)
#define RVA_DECOR_QUEUE_ADD                    ADDR(        0x1ad730,          MAC_INLINED)
#define RVA_DECOR_DRAW                         ADDR(        0x1ab7d0,          MAC_INLINED)
#define RVA_DECOR_DRAW_CALL                    ADDR(        0x19dd18,          MAC_NO_CALL)
#define RVA_DRAW_SPRITE_SHEARED                ADDR(        0x063ec0,          0x10034f824)
#define RVA_MAP_SWAY                           ADDR(        0xf5708c0,         0x106cf539c)
#define RVA_MAP_SWAY_UPDATE                    ADDR(        0x19e8d0,          MAC_INLINED)
#define RVA_MAP_SWAY_UPDATE_CALL               ADDR(        0x240230,          MAC_NO_CALL)
#define RVA_MAP_SWAY_CLOCK                     ADDR(        0x582bc34,         0x107dfb4c8)
#define RVA_FRAME_MS                           ADDR(        0x5835f88,         0x106ceedd0)
#define RVA_MAP_SPRINKLES                      ADDR(        0xf570900,         0x107dcb5d8)
#define RVA_MAP_RANDOM_FLOATS                  ADDR(        0xf580900,         0x106cf057c)
#define RVA_BG_THREADS_OFF                     ADDR(        0x5827584,         0x10c6879a4)
#define RVA_MAP_RANDOM_FLOATS_FILL             ADDR(        0x1ad7f0,          MAC_INLINED)
#define RVA_MAP_RANDOM_FLOATS_DONE             ADDR(        0x582a898,         0x106cf0578)
#define RVA_DECOR_BUCKET_FILL                  ADDR(        0x1ac4e0,          MAC_INLINED)
#define RVA_DECOR_BUCKET_SORT                  ADDR(        0x1ac260,          MAC_INLINED)
#define RVA_GRAPHICS_DETAIL                    ADDR(        0x1a50ecc,         0x100872c08)

// ---- Utilities ----------------------------------------------------------------------------------
#define RVA_TRACE                              ADDR(        0x028e80,          0x10033a9b4)
#define RVA_SNPRINTF                           ADDR_DIFFERS(0x001430,          0x1005aeeb4)
#define RVA_VSPRINTF                           ADDR_DIFFERS(0x005724,          0x1005af088)
#define RVA_PRINTF_OPTIONS                     ADDR(        0x001300,          MAC_NOT_FOUND)
#define RVA_SYSTEM                             ADDR(        0x0062b0,          0x1005af034)
#define RVA_GETTIMEOFDAY                       ADDR(        WIN_NO_EQUIVALENT, 0x1005ae680)
#define RVA_TRACE_B                            ADDR(        0x0288f0,          0x10033abdc)
#define RVA_TRACE_C                            ADDR(        0x028ce0,          0x10033aac8)
#define RVA_DEBUG_LEVEL                        ADDR(        0x42d01c,          0x1059bcc78)
#define RVA_TRACE_B_CHECK                      ADDR(        0x028924,          0x10033ac1c)
#define RVA_TRACE_C_CHECK                      ADDR(        0x028d14,          0x10033ab08)
#define RVA_TRACE_ERROR                        ADDR(        0x028a90,          0x10033a848)
#define RVA_TRACE_ERROR_WRITE                  ADDR(        0x028b50,          MAC_NOT_LOOKED_UP)
#define TRACE_PROLOGUE_SIZE                    0x44
#define TRACE_ERROR_PROLOGUE_SIZE              0x40
#define RVA_TRACE_TIMER                        ADDR(        0x028700,          MAC_NOT_LOOKED_UP)
#define RVA_TRACE_TIMER_MARK                   ADDR(        WIN_INLINED,       MAC_NOT_LOOKED_UP)
#define RVA_TRACE_TO_LOG_FILE                  ADDR(        0x57c58dc,         MAC_NOT_FOUND)
#define RVA_TRACE_LOG_START                    ADDR(        0x028cb0,          MAC_NOT_FOUND)
#define RVA_TRACE_LOG_OPEN_CALL                ADDR(        0x028f24,          MAC_NO_CALL)
#define RVA_TRACE_LOG_CLOSE_CALL               ADDR(        0x028f8f,          MAC_NO_CALL)
#define RVA_TRACE_C_LOG_OPEN_CALL              ADDR(        0x028d84,          MAC_NO_CALL)
#define RVA_TRACE_C_LOG_CLOSE_CALL             ADDR(        0x028def,          MAC_NO_CALL)
#define RVA_TRACE_B_LOG_OPEN_CALL              ADDR(        0x028994,          MAC_NO_CALL)
#define RVA_TRACE_B_LOG_CLOSE_CALL             ADDR(        0x0289ff,          MAC_NO_CALL)
#define RVA_TRACE_ERROR_LOG_OPEN_CALL          ADDR(        0x028b7f,          MAC_NO_CALL)
#define RVA_TRACE_ERROR_LOG_CLOSE_JUMP         ADDR(        0x028bee,          MAC_NO_CALL)
#define RVA_TYPED_WORD_DEBUG_LEVEL             ADDR(        WIN_NOT_FOUND,     MAC_NOT_FOUND)
#define RVA_TRACE_TIMESTAMPS                   ADDR_DIFFERS(0x57c58e4,         0x105b68154)
#define RVA_SDL_LOADOBJECT_SLOT                ADDR(        WIN_NO_EQUIVALENT, 0x100852390)
#define RVA_SDL_LOADFUNCTION_SLOT              ADDR(        WIN_NO_EQUIVALENT, 0x100852398)
#define RVA_SDL_LOADOBJECT_INIT                ADDR(        WIN_NO_EQUIVALENT, 0x100016bb8)
#define RVA_SDL_LOADFUNCTION_INIT              ADDR(        WIN_NO_EQUIVALENT, 0x100016be4)
#define RVA_FAIL_IF                            ADDR(        0x05b600,          0x10033434c)
#define RVA_ALLOC                              ADDR(        0x05e4b0,          0x100333fcc)
#define RVA_TICKS_MS                           ADDR_DIFFERS(0x05e3c0,          0x100334418)
#define RVA_FILE_READ_INT                      ADDR(        0x058a60,          0x100336640)
#define RVA_FILE_READ_SHORT                    ADDR(        0x058870,          MAC_INLINED)
#define RVA_FILE_READ_ARRAY                    ADDR(        0x057fb0,          MAC_INLINED)
#define RVA_FILE_WRITE_INT                     ADDR(        0x058530,          MAC_INLINED)
#define RVA_FILE_WRITE_ARRAY                   ADDR(        0x058050,          MAC_INLINED)
#define RVA_FATAL_FORMAT                       ADDR(        0x05e820,          0x100334214)
#define RVA_RANDOM                             ADDR(        0x05b7b0,          0x100335834)
#define RVA_RANDOM_OPEN_DIE                    ADDR(        0x058cc0,          0x100336c00)
#define RVA_INT_SQRT                           ADDR(        0x056f40,          MAC_INLINED)
#define RVA_RANDOM_INDEX                       ADDR(        0x57f1a74,         0x105aa91e0)
#define RVA_RANDOM_SHIFT                       ADDR(        0x57f1a98,         0x105aa9248)
#define RVA_RANDOM_XOR                         ADDR(        0x57f1a70,         0x105aa924c)
#define RVA_RANDOM_SEED_FROM_CLOCK             ADDR(        0x05adb0,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_SET_SEED                    ADDR(        0x05ad60,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_RAND_BELOW                     ADDR(        0x05e7a0,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_SEED_FROM_CLOCK                ADDR(        0x05ac10,          MAC_NOT_LOOKED_UP)
#define RVA_CRT_RAND                           ADDR_DIFFERS(0x00eff0,          0x1005aeda0)
#define RVA_CRT_SRAND                          ADDR(        0x00f01c,          MAC_NOT_LOOKED_UP)
#define RVA_BATTLE_SEED_DRAW_CALL              ADDR(        0x0f590e,          MAC_NOT_LOOKED_UP)
#define RVA_RANDOM_TABLE                       ADDR(        0x343790,          0x10075bc34)
#define RVA_MEMSET                             ADDR(        0x002ef0,          0x1005aea28)
#define RVA_MEMMOVE                            ADDR(        0x002630,          0x1005aea1c)
#define RVA_MEMCPY                             ADDR_DIFFERS(0x002630,          0x1005aea10)
#define RVA_BZERO                              ADDR(        WIN_NO_EQUIVALENT, 0x1005ae404)
#define RVA_FREAD                              ADDR(        0x00e6a0,          0x1005ae5d8)
#define RVA_FWRITE                             ADDR(        0x005fe0,          0x1005ae650)
#define RVA_FCLOSE                             ADDR(        0x005ab8,          0x1005ae53c)
#define RVA_FOPEN                              ADDR(        0x005a28,          0x1005ae5a8)
#define RVA_FGETS                              ADDR(        0x00f950,          0x1005ae578)
#define RVA_FFLUSH                             ADDR(        0x006220,          0x1005ae56c)
#define RVA_STRCMP_STUB                        ADDR(        WIN_NO_EQUIVALENT, 0x1005aef20)
#define RVA_CHKSTK                             ADDR(        0x023ef0,          MAC_STORED_DIFFERENTLY)
#define RVA_CHKSTK_DARWIN_GOT                  ADDR(        WIN_NO_EQUIVALENT, 0x100774210)
#define RVA_FILE_BYTES_WRITTEN                 ADDR(        0x57f1a80,         0x105aa9290)

// ---- The CRT's stack-cookie check and exception handlers ----------------------------------------
#define RVA_SECURITY_COOKIE                    ADDR(        0x42c000,          MAC_NOT_FOUND)
#define RVA_SECURITY_CHECK_COOKIE              ADDR(        0x001880,          MAC_NOT_FOUND)
#define RVA_CHECK_COOKIE_FAIL_JMP              ADDR(        0x00189c,          MAC_NOT_FOUND)
#define RVA_REPORT_GSFAILURE                   ADDR(        0x0018d8,          MAC_NOT_FOUND)
#define RVA_GS_HANDLER_CHECK                   ADDR(        0x023e60,          MAC_NOT_FOUND)
#define RVA_C_SPECIFIC_HANDLER                 ADDR(        0x002c74,          MAC_NOT_FOUND)
#define RVA_SCRT_SET_UNHANDLED_FILTER          ADDR(        0x002360,          MAC_NOT_FOUND)
#define RVA_STACK_CHK_FAIL_STUB                ADDR(        WIN_NO_EQUIVALENT, 0x1005ae2fc)
#define RVA_SDL_QUIT_SIGNAL_HANDLER            ADDR(        WIN_NO_EQUIVALENT, 0x10002595c)
#define RVA_SDL_INSTALL_QUIT_SIGNAL            ADDR(        WIN_NO_EQUIVALENT, 0x100025908)
#define RVA_SDL_REMOVE_QUIT_SIGNAL             ADDR(        WIN_NO_EQUIVALENT, 0x1000259a0)
#define RVA_SDL_QUIT_SIGNALLED                 ADDR(        WIN_NO_EQUIVALENT, 0x105aa586c)

// ---- Strings ------------------------------------------------------------------------------------
#define RVA_STR_MAINMENU                       ADDR(        0x1c5bfa0,         0x100605fe4)
#define RVA_STR_START_NEW_GAME                 ADDR(        0x1c5bfe8,         0x10060601b)
#define RVA_STR_LOAD_GAME                      ADDR(        0x1c5bff8,         0x1006fe779)
#define RVA_STR_NETWORK                        ADDR(        0x1c5c018,         0x1006ff523)
#define RVA_STR_PREFERENCES                    ADDR(        0x1c5c020,         0x1006315c1)
#define RVA_STR_MODS                           ADDR(        0x1c5c02c,         0x10070700a)
#define RVA_STR_CREDITS                        ADDR(        0x1c5c038,         0x10060602a)
#define RVA_STR_READ_TIP                       ADDR(        0x1c5c040,         0x100606032)
#define RVA_STR_QUIT                           ADDR(        0x1c5c04c,         0x1007102b0)
#define RVA_STR_PERCENT_S                      ADDR(        0x1a3b9d4,         0x1005b6f9d)
#define RVA_STR_OPEN_MODS_DIR                  ADDR(        0x1c616f8,         0x100707088)
#define RVA_STR_GUIFONT                        ADDR(        0x432230,          0x100857674)
#define RVA_STR_GUIFONT_FANCY                  ADDR(        0x435350,          0x100861c7c)
#define RVA_STR_GUIFONT_TEXTY                  ADDR(        0x431840,          0x10085803c)
#define RVA_FONT_FILE_TABLE                    ADDR(        WIN_NOT_FOUND,     0x10077e6b0)
#define RVA_STR_LOAD_GAME_DISABLED             ADDR(        0x1c5c008,         0x1006fe779)
#define RVA_STR_GAME_MENU                      ADDR(        0x1b292b0,         0x1006315ae)
#define RVA_STR_TIEVT                          ADDR(        0x43b368,          0x100712428)
#define RVA_STR_TTF_EXTENSION                  ADDR(        0x1c5db78,         0x100707272)
#define RVA_STR_NO_TTF_FILE                    ADDR(        0x1c5db80,         0x100707277)
#define RVA_STR_FONT_FILE_MISSING              ADDR(        0x1c5db58,         0x100707254)
#define RVA_STR_MOD_DIFFERS                    ADDR(        0x1c58cd0,         0x1006fd245)

// ---- Site generators' own sites -----------------------------------------------------------------
#define RVA_SWITCH_TABLE_1                     ADDR(        0x053315,          MAC_NOT_LOOKED_UP)
#define RVA_SWITCH_TABLE_2                     ADDR(        0x05331d,          MAC_NOT_LOOKED_UP)
#define RVA_SWITCH_TABLE_3                     ADDR(        0x177cf0,          MAC_NOT_LOOKED_UP)
#define RVA_SWITCH_TABLE_4                     ADDR(        0x26378e,          MAC_NOT_LOOKED_UP)
#define RVA_SWITCH_TABLE_5                     ADDR(        0x282884,          MAC_NOT_LOOKED_UP)
#define RVA_SQUARE_UNITS                       ADDR(        0x0a2f40,          0x10037264c)
#define RVA_FG_VALUES_SITE_1                   ADDR(        0x0a2fe7,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_2                   ADDR(        0x0bad46,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_3                   ADDR(        0x0bad58,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_4                   ADDR(        0x0bad6c,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_5                   ADDR(        0x0baea0,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_6                   ADDR(        0x0baee2,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_7                   ADDR(        0x120acd,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_8                   ADDR(        0x120ba7,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_9                   ADDR(        0x0ba9bd,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_10                  ADDR(        0x0ba9cd,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_11                  ADDR(        0x0ba9e6,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_12                  ADDR(        0x0ba9f6,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_13                  ADDR(        0x0baa0a,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_14                  ADDR(        0x0baa20,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_15                  ADDR(        0x104790,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_16                  ADDR(        0x1047a5,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_17                  ADDR(        0x1047c6,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_18                  ADDR(        0x1047dd,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_19                  ADDR(        0x11df70,          MAC_NOT_LOOKED_UP)
#define RVA_ARMY_SUMMARY_INLINED_HOST          ADDR(        0x11d090,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_20                  ADDR(        0x11df7e,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_21                  ADDR(        0x11df98,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_22                  ADDR(        0x11dfaf,          MAC_NOT_LOOKED_UP)
#define RVA_FG_VALUES_SITE_23                  ADDR(        0x0f74a0,          MAC_NOT_LOOKED_UP)
#define RVA_FGM_EXTRA_FUNCTIONS_SITE_1         ADDR(        WIN_NOT_LOOKED_UP, 0x1004826b4)
#define RVA_FGM_EXTRA_FUNCTIONS_SITE_2         ADDR(        WIN_NOT_LOOKED_UP, 0x1003cf664)
#define RVA_FGM_VALUES_SITE_1                  ADDR(        WIN_NOT_LOOKED_UP, 0x1003726c0)
#define RVA_FGM_VALUES_SITE_2                  ADDR(        WIN_NOT_LOOKED_UP, 0x100372804)
#define RVA_FGM_VALUES_SITE_3                  ADDR(        WIN_NOT_LOOKED_UP, 0x100372814)
#define RVA_FGM_VALUES_SITE_4                  ADDR(        WIN_NOT_LOOKED_UP, 0x10037282c)
#define RVA_FGM_VALUES_SITE_5                  ADDR(        WIN_NOT_LOOKED_UP, 0x1003728c0)
#define RVA_FGM_VALUES_SITE_6                  ADDR(        WIN_NOT_LOOKED_UP, 0x100384950)
#define RVA_FGM_VALUES_SITE_7                  ADDR(        WIN_NOT_LOOKED_UP, 0x100384cc4)
#define RVA_FGM_VALUES_SITE_8                  ADDR(        WIN_NOT_LOOKED_UP, 0x1003df640)
#define RVA_FGM_VALUES_SITE_9                  ADDR(        WIN_NOT_LOOKED_UP, 0x1003df650)
#define RVA_FGM_VALUES_SITE_10                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003df65c)
#define RVA_FGM_VALUES_SITE_11                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003df708)
#define RVA_FGM_VALUES_SITE_12                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003df8f8)
#define RVA_FGM_VALUES_SITE_13                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003df9d0)
#define RVA_FGM_VALUES_SITE_14                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003dfa40)
#define RVA_FGM_VALUES_SITE_15                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003dfb48)
#define RVA_FGM_VALUES_SITE_16                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003dfb74)
#define RVA_FGM_VALUES_SITE_17                 ADDR(        WIN_NOT_LOOKED_UP, 0x1004c2460)
#define RVA_FGM_VALUES_SITE_18                 ADDR(        WIN_NOT_LOOKED_UP, 0x1004c4050)
#define RVA_FGM_VALUES_SITE_19                 ADDR(        WIN_NOT_LOOKED_UP, 0x1004c4428)
#define RVA_FGM_VALUES_SITE_20                 ADDR(        WIN_NOT_LOOKED_UP, 0x1004c4834)
#define RVA_FGM_VALUES_SITE_21                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003cf7c4)
#define RVA_FGM_VALUES_SITE_22                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a28)
#define RVA_FGM_VALUES_SITE_23                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a38)
#define RVA_FGM_VALUES_SITE_24                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a48)
#define RVA_FGM_VALUES_SITE_25                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a54)
#define RVA_FGM_VALUES_SITE_26                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a60)
#define RVA_FGM_VALUES_SITE_27                 ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7a70)
#define RVA_FGM_VALUES_SITE_28                 ADDR(        WIN_NOT_LOOKED_UP, 0x1004826f0)
#define RVA_FGM_VALUES_SITE_29                 ADDR(        WIN_NOT_LOOKED_UP, 0x100482704)
#define RVA_FGM_VALUES_SITE_30                 ADDR(        WIN_NOT_LOOKED_UP, 0x100482718)
#define RVA_FGM_VALUES_SITE_31                 ADDR(        WIN_NOT_LOOKED_UP, 0x10048272c)
#define RVA_FGM_VALUES_SITE_32                 ADDR(        WIN_NOT_LOOKED_UP, 0x100482a48)
#define RVA_FGM_VALUES_SITE_33                 ADDR(        WIN_NOT_LOOKED_UP, 0x100482a7c)
#define RVA_FGM_VALUES_SITE_34                 ADDR(        WIN_NOT_LOOKED_UP, 0x100482a98)
#define RVA_FGM_NOT_VALUES_SITE_1              ADDR(        WIN_NOT_LOOKED_UP, 0x1004c293c)
#define RVA_FGM_NOT_VALUES_SITE_2              ADDR(        WIN_NOT_LOOKED_UP, 0x1004c2f58)
#define RVA_SPELL_TABLE_PAGE                   ADDR(        WIN_NOT_LOOKED_UP, 0x100839000)
#define RVA_TM_WEAPON_MAC_SIZE_EXCLUDE_1       ADDR(        WIN_NOT_LOOKED_UP, 0x100584778)
#define RVA_TM_WEAPON_MAC_SIZE_EXCLUDE_2       ADDR(        WIN_NOT_LOOKED_UP, 0x100584784)
#define RVA_TM_WEAPON_MAC_SIZE_EXCLUDE_3       ADDR(        WIN_NOT_LOOKED_UP, 0x10058b3cc)
#define RVA_TM_ITEM_NEAR_OK_1                  ADDR(        0x17dce5,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ITEM_EXCLUDE_1                  ADDR(        0x18351a,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ITEM_EXCLUDE_2                  ADDR(        0x2c9863,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ITEM_MAC_EXCLUDE_1              ADDR(        WIN_NOT_LOOKED_UP, 0x1004e4588)
#define RVA_TM_CLASS_HEROES_NEAR_OK_1          ADDR(        0x0b258b,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_2          ADDR(        0x0b2599,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_3          ADDR(        0x0b25a7,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_4          ADDR(        0x0b25b5,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_5          ADDR(        0x0b25c3,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_6          ADDR(        0x0b25d1,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_7          ADDR(        0x0b25df,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_8          ADDR(        0x0b25ed,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_9          ADDR(        0x0b26c2,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_HEROES_NEAR_OK_10         ADDR(        0x0b26d6,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_RECRUITS_NOT_REFS_1       ADDR(        0x2784df,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_1                 ADDR(        0x19107c,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_2                 ADDR(        0x1bc100,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_3                 ADDR(        0x1bc111,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_4                 ADDR(        0x1caa45,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_5                 ADDR(        0x1cace7,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_NEAR_OK_6                 ADDR(        0x31ce4e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_MAC_EXCLUDE_1             ADDR(        WIN_NOT_LOOKED_UP, 0x1003706d0)
#define RVA_TM_CLASS_MAC_EXCLUDE_2             ADDR(        WIN_NOT_LOOKED_UP, 0x10037072c)
#define RVA_TM_MONSTER_NAMED_1                 ADDR(        0x0a3162,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_2                 ADDR(        0x0a317d,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_3                 ADDR(        0x0a31e1,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_4                 ADDR(        0x0bab93,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_5                 ADDR(        0x0bb248,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_6                 ADDR(        0x1049af,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_7                 ADDR(        0x104a23,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_8                 ADDR(        0x11e097,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_9                 ADDR(        0x11f81f,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_10                ADDR(        0x129edf,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_11                ADDR(        0x12ab9d,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_12                ADDR(        0x09b531,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_13                ADDR(        0x0bf593,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_14                ADDR(        0x104913,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_15                ADDR(        0x11e1da,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_16                ADDR(        0x120c98,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_17                ADDR(        0x1e0485,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NAMED_18                ADDR(        0x1e0650,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_1               ADDR(        0x1cc90f,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_2               ADDR(        0x1cc930,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_3               ADDR(        0x1cc944,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_4               ADDR(        0x1cc959,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_5               ADDR(        0x1cc96e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_6               ADDR(        0x1cc983,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_7               ADDR(        0x1cc9d0,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_8               ADDR(        0x1cc9e4,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_9               ADDR(        0x1cc9f9,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_10              ADDR(        0x1cca0e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_11              ADDR(        0x1cca23,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_12              ADDR(        0x1cca50,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_13              ADDR(        0x1cca64,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_14              ADDR(        0x1cca79,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_15              ADDR(        0x1cca8e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_16              ADDR(        0x1ccaa3,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_17              ADDR(        0x1ccae0,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_18              ADDR(        0x1ccaf4,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_19              ADDR(        0x1ccb09,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_20              ADDR(        0x1ccb1e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_21              ADDR(        0x1ccb33,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_22              ADDR(        0x1da34d,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_NEAR_OK_23              ADDR(        0x1da14f,          MAC_NOT_LOOKED_UP)
#define RVA_TM_MONSTER_MAC_NAMED_1             ADDR(        WIN_NOT_LOOKED_UP, 0x1004642b4)
#define RVA_TM_MONSTER_MAC_NAMED_2             ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7be0)
#define RVA_TM_MONSTER_MAC_NAMED_3             ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7cf8)
#define RVA_TM_MONSTER_MAC_NAMED_4             ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7e24)
#define RVA_TM_MONSTER_MAC_NAMED_5             ADDR(        WIN_NOT_LOOKED_UP, 0x1003f7f6c)
#define RVA_TM_MONSTER_MAC_NAMED_6             ADDR(        WIN_NOT_LOOKED_UP, 0x10048292c)
#define RVA_TM_MONSTER_MAC_NAMED_7             ADDR(        WIN_NOT_LOOKED_UP, 0x100372794)
#define RVA_TM_MONSTER_MAC_NAMED_8             ADDR(        WIN_NOT_LOOKED_UP, 0x10037bec0)
#define RVA_TM_ARMY_LIST_COUNTS_NOT_REFS_1     ADDR(        0x12791e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_COUNTS_NOT_REFS_2     ADDR(        0x127929,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_1    ADDR(        0x0a0f3b,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_2    ADDR(        0x0f23a4,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_3    ADDR(        0x110712,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_4    ADDR(        0x18691a,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_5    ADDR(        0x18e373,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_6    ADDR(        0x18e3c4,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_7    ADDR(        0x18e3d3,          MAC_NOT_LOOKED_UP)
#define RVA_TM_ARMY_LIST_LEAST_B_NOT_REFS_8    ADDR(        0x1e8b34,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NOT_REFS_1           ADDR(        0x0a7a87,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_1            ADDR(        0x0a5626,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_2            ADDR(        0x0a5ac9,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_3            ADDR(        0x0a5b88,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_4            ADDR(        0x0a5c80,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_5            ADDR(        0x0a5d70,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_6            ADDR(        0x0a628e,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_7            ADDR(        0x0a66b1,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_8            ADDR(        0x0a67ec,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_NEAR_OK_9            ADDR(        0x0a6a70,          MAC_NOT_LOOKED_UP)
#define RVA_TM_CLASS_COPY_MAC_SIZE_EXCLUDE_1   ADDR(        WIN_NOT_LOOKED_UP, 0x1003312e4)
#define RVA_TM_CLASS_COPY_MAC_SIZE_EXCLUDE_2   ADDR(        WIN_NOT_LOOKED_UP, 0x1003326e4)
#define RVA_TM_CLASS_COPY_MAC_SIZE_EXCLUDE_3   ADDR(        WIN_NOT_LOOKED_UP, 0x1003915ec)
#define RVA_TM_CLASS_COPY_MAC_SIZE_EXCLUDE_4   ADDR(        WIN_NOT_LOOKED_UP, 0x100529270)

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
        /* windows */ (0x192782),       0x23f240,
        /* mac     */ (M(0x10032e284)), M(0x100484ef8)),
    [SITE_MAIN_MENU_PLAY_GAME] = SITE("main_menu_play_game", 0,
        /* windows */ (0x1927eb),       0x091ce0,
        /* mac     */ (M(0x10032e2e4)), M(0x100389844)),
    [SITE_PLAY_GAME_NEW_WORLD] = SITE("play_game_new_world", 0,
        /* windows */ (0x091e00),       0x303360,
        /* mac     */ (M(0x100389c68)), M(0x100587238)),
    [SITE_NEW_WORLD_MAP_SCREEN] = SITE("new_world_map_screen", 0,
        /* windows */ (0x3034fa),       0x2aa590,
        /* mac     */ (M(0x100587428)), M(0x1004b2454)),
    [SITE_NEW_WORLD_INIT_PARTICIPANTS] = SITE("new_world_init_participants", 0,
        /* windows */ (0x30383d), 0x2ea3a0,
        /* mac     */ (0),        0),
    [SITE_NEW_WORLD_CREATING_WORLD] = SITE("new_world_creating_world", 0,
        /* windows */ (0x30505f),       0x0899e0,
        /* mac     */ (M(0x1005893fc)), M(0x1004900c0)),
    [SITE_MAP_SETTINGS_WAIT] = SITE("map_settings_wait", 0,
        /* windows */ (0x2ab1d7),       0x23f240,
        /* mac     */ (M(0x1004b2984)), M(0x100484ef8)),
    [SITE_PARTICIPANTS_WAIT] = SITE("participants_wait", 0,
        /* windows */ (0x3048e8),       0x23f240,
        /* mac     */ (M(0x100588760)), M(0x100484ef8)),
    [SITE_PARTICIPANTS_DIFFICULTY_WAIT] = SITE("participants_difficulty_wait", 0,
        /* windows */ (0x303045),       0x23f240,
        /* mac     */ (M(0x100586a48)), M(0x100484ef8)),
    // @platform-split participants_button_row: Mac's chain ends in CoE5SE's copy of button_row
    [SITE_PARTICIPANTS_BUTTON_ROW] = SITE("participants_button_row", 0,
        /* windows */ (0x3042a4),                       0x03a3f0,
        /* mac     */ (M(0x1005889e8), M(0x10058848c)), M(0x10033cbb4)),
    [SITE_GAME_MENU_SAVING_SECTION_END] = SITE("game_menu_saving_section_end", 0,
        /* windows */ (0x124aed),       0x035400,
        /* mac     */ (M(0x100380758)), M(0x100340ac4)),
    [SITE_GAME_MENU_WAIT] = SITE("game_menu_wait", 0,
        /* windows */ (0x124f3c),       0x23f240,
        /* mac     */ (M(0x1003809c8)), M(0x100484ef8)),
    [SITE_RITUAL_SCREEN_ROW] = SITE("ritual_screen_row", 0,
        /* windows */ (0x2b0156),       0x2d6c10,
        /* mac     */ (M(0x1003604e4)), M(0x100351d60)),
    [SITE_RITUAL_SCREEN_LOCATION_CHECK] = SITE("ritual_screen_location_check", 0,
        /* windows */ (0x2b00b4),       0x2cd460,
        /* mac     */ (M(0x10036030c)), M(0x100559868)),
    [SITE_RITUAL_SCREEN_CAN_CAST] = SITE("ritual_screen_can_cast", 0,
        /* windows */ (0x2b00c1),       0x2c9f30,
        /* mac     */ (M(0x10036031c)), M(0x10055bea8)),
    [SITE_MAP_SCREEN_WAIT] = SITE("map_screen_wait", 0,
        /* windows */ (0x097667),       0x23f240,
        /* mac     */ (M(0x1003800d8)), M(0x100484ef8)),
    [SITE_MAP_MENU_BUTTON_TEXT] = SITE("map_menu_button_text", 0,
        /* windows */ (0x09dc31),       0x038f60,
        /* mac     */ (M(0x10037b004)), M(0x10033da20)),
    [SITE_GAME_START] = SITE("game_start", 0,
        /* windows */ (0x09209f),       0x1b0e90,
        /* mac     */ (M(0x10038a290)), M(0x100491c3c)),
    [SITE_MONTH_START] = SITE("month_start", 0,
        /* windows */ (0x09227c),       0x0899e0,
        /* mac     */ (M(0x10038a5c0)), M(0x1004900c0)),
    [SITE_PLAYER_TURN_START] = SITE("player_turn_start", 0,
        /* windows */ (0x09242f, 0x0926c6, 0x0927de),                   0x167ce0,
        /* mac     */ (M(0x10038a808), M(0x10038a990), M(0x10038abbc)), M(0x100392874)),
    [SITE_HUMAN_TURN] = SITE("human_turn", 0,
        /* windows */ (0x0929cf),       0x0949d0,
        /* mac     */ (M(0x10038b050)), M(0x10037b204)),
    [SITE_AI_TURN] = SITE("ai_turn", 0,
        /* windows */ (0x092461),       0x06f9d0,
        /* mac     */ (M(0x10038abf8)), M(0x1004e05a8)),
    [SITE_PLAYER_TURN_END] = SITE("player_turn_end", 0,
        /* windows */ (0x0924e2),       0x1e1e30,
        /* mac     */ (M(0x10038bfac)), M(0x1004c53c8)),
    // @platform-split game_over: Mac's call is the Game Over screen's, not game_over's
    [SITE_GAME_OVER] = SITE("game_over", 0,
        /* windows */ (0x09403c),       0x10d3a0,
        /* mac     */ (M(0x1003900a8)), M(0x100473b14)),
    // @platform-split game_save: Windows' calls pass (name, FILE *); CoE5SE's outermost wrapper passes (path, name) on
    [SITE_GAME_SAVE] = SITE("game_save", 0,
        /* windows */ (0x09213e, 0x0929a5, 0x09447b, 0x18b8fc),         0x18eb50,
        /* mac     */ (M(0x10038a340), M(0x10038b018), M(0x1004f4bf0)), M(0x1004f1fd0)),
    [SITE_GAME_LOAD] = SITE("game_load", 0,
        /* windows */ (0x18b366, 0x191355, 0x191561),                   0x185f10,
        /* mac     */ (M(0x10032e388), M(0x100333438), M(0x100333908)), M(0x1004eb2a8)),
    [SITE_MODS_RUN] = SITE("mods_run", 0,
        /* windows */ (0x091f9c, 0x094070, 0x094391, 0x167d72, 0x186140, 0x191299, 0x1ca9df),                           0x1cb9a0,
        /* mac     */ (M(0x100389d88), M(0x100391658), M(0x100392938), M(0x1004eb860), M(0x100332b00), M(0x10032f148)), M(0x10053a1dc)),
    [SITE_MOD_COMMAND] = SITE("mod_command", 0,
        /* windows */ (0x1bb8a0),                       0x1bbb90,
        /* mac     */ (M(0x10052a1c0), M(0x10052a50c)), M(0x10052ee34)),
    [SITE_SQUARE_INFO_SCREEN] = SITE("square_info_screen", 0,
        /* windows */ (0x097c23),       0x331c50,
        /* mac     */ (M(0x10038430c)), M(0x1004c9b00)),
    // @platform-split square_info_button_row: Mac's chain ends in CoE5SE's copy of button_row
    [SITE_SQUARE_INFO_BUTTON_ROW] = SITE("square_info_button_row", 0,
        /* windows */ (0x334a8c),       0x03a3f0,
        /* mac     */ (M(0x1004cd1fc)), M(0x10033cbb4)),
    [SITE_SQUARE_INFO_WAIT] = SITE("square_info_wait", 0,
        /* windows */ (0x334cf5),       0x23f240,
        /* mac     */ (M(0x1004cd2e0)), M(0x100484ef8)),
    [SITE_TEXT_FORMAT] = SITE("text_format", 0,
        /* windows */ (0x0336a9, 0x039b4d, 0x039fbd, 0x039dfe, 0x039d1d, 0x03982d, 0x039a80, 0x038dc6, 0x038a15,
                       0x038bd1, 0x039216, 0x0361b4, 0x036473, 0x038ff6, 0x042fbe, 0x042bc0), 0x005724,
        /* mac     */ (0),                                                                    0),
    [SITE_TEXT_BUILD] = SITE("text_build", 0,
        /* windows */ (0x0f8dc3, 0x0f8e3b, 0x0f9191, 0x0f92bc, 0x0f92db, 0x11912c, 0x2afec2, 0x2aff26, 0x2affb4,
                       0x2b0009, 0x2b0093, 0x2b00df, 0x2b02d7, 0x2b0345, 0x2b0353, 0x2c2777, 0x2c27c6, 0x2c2838,
                       0x2c28a8, 0x2c2918, 0x2c298a, 0x2c29fc), 0x001430,
        /* mac     */ (0),                                      0),
    [SITE_LOAD_GAME_BROWSER] = SITE("load_game_browser", 0,
        /* windows */ (0x18b2d4, 0x18b332), 0x03f430,
        /* mac     */ (M(0x10032e364)),     M(0x10034a3d4)),
    [SITE_FILE_BROWSER_ROW_DATE] = SITE("file_browser_row_date", 0,
        /* windows */ (0x040cde),       0x039ad0,
        /* mac     */ (M(0x10034b218)), M(0x10033d2e4)),
    [SITE_FILE_BROWSER_WAIT] = SITE("file_browser_wait", 0,
        /* windows */ (0x04118a),       0x23f240,
        /* mac     */ (M(0x10034b740)), M(0x100484ef8)),
    [SITE_FILE_BROWSER_HOVERED] = SITE("file_browser_hovered", 0,
        /* windows */ (0), 0,
        /* mac     */ (0), 0),
    [SITE_MAIN_MENU_MODS_SCREEN] = SITE("main_menu_mods_screen", 0,
        /* windows */ (0x1928db),       0x1ca220,
        /* mac     */ (M(0x10032f14c)), M(0x10032a52c)),
    // @platform-split mod_banner_draw: Mac's call goes to a copy with another signature
    [SITE_MOD_BANNER_DRAW] = SITE("mod_banner_draw", 0,
        /* windows */ (0x23fdf7),       0x02f9c0,
        /* mac     */ (M(0x1004889d4)), M(0x1003454c8)),
    [SITE_MOD_ERROR_DIALOG] = SITE("mod_error_dialog", 0,
        /* windows */ (0x1c9bf4),       0x041f50,
        /* mac     */ (M(0x100529c5c)), M(0x100349628)),
    [SITE_GAME_OVER_WAIT] = SITE("game_over_wait", 0,
        /* windows */ (0x10d803),       0x23f240,
        /* mac     */ (M(0x1004742c4)), M(0x100484ef8)),
    [SITE_EVENT_LINE] = SITE("event_line", 0,
        /* windows */ (0x1bb87c, 0x1cb896),                                             0x1c58b0,
        /* mac     */ (M(0x10052a19c), M(0x10052a364), M(0x10052a4f4), M(0x100539f94)), M(0x10052a648)),
    [SITE_ARMY_WAIT] = SITE("army_wait", 0,
        /* windows */ (0x120d10),       0x23f240,
        /* mac     */ (M(0x100384d40)), M(0x100484ef8)),
    [SITE_BATTLE_WAIT] = SITE("battle_wait", 0,
        /* windows */ (0x1006c0),       0x23f240,
        /* mac     */ (M(0x1005829e8)), M(0x100484ef8)),
    [SITE_MAP_EDITOR_WAIT] = SITE("map_editor_wait", 0,
        /* windows */ (0x1b8e7f),       0x23f240,
        /* mac     */ (M(0x10051564c)), M(0x100484ef8)),
    [SITE_BATTLE_EDITOR_WAIT] = SITE("battle_editor_wait", 0,
        /* windows */ (0x085c98),       0x23f240,
        /* mac     */ (M(0x1005197d0)), M(0x100484ef8)),
    [SITE_RECRUIT_WAIT] = SITE("recruit_wait", 0,
        /* windows */ (0x1270d6),       0x23f240,
        /* mac     */ (M(0x10047c4b4)), M(0x100484ef8)),
    [SITE_VIEW_ARMY_WAIT] = SITE("view_army_wait", 0,
        /* windows */ (0x122a12),       0x23f240,
        /* mac     */ (M(0x10047de50)), M(0x100484ef8)),
    [SITE_UNIT_LIST_WAIT] = SITE("unit_list_wait", 0,
        /* windows */ (0x03f0d6),       0x23f240,
        /* mac     */ (M(0x1005844d0)), M(0x100484ef8)),
    [SITE_KEYBOARD_SHORTCUTS_WAIT] = SITE("keyboard_shortcuts_wait", 0,
        /* windows */ (0x12d190),       0x23f240,
        /* mac     */ (M(0x100512480)), M(0x100484ef8)),
    // @platform-split unit_list_sheet: no Mac call
    [SITE_UNIT_LIST_SHEET] = SITE("unit_list_sheet", 0,
        /* windows */ (0x0facfd), 0x03e480,
        /* mac     */ (0),        0),
    // @platform-split poll_event: Windows' calls go through the import slot
    [SITE_POLL_EVENT] = SITE("poll_event", 0,
        /* windows */ (0x04eafa, 0x04f295), 0x33a580,
        /* mac     */ (M(0x1004852a8)),     M(0x10000f980)),
    [SITE_KEY_TEXT_BAR] = SITE("key_text_bar", 0,
        /* windows */ (0x09c7ac, 0x09d8c4, 0x09d928, 0x09d97c, 0x09d9d0, 0x09da40, 0x09dae4, 0x09db38, 0x09db9d,
                       0x09dbcc, 0x099833, 0x0998e7, 0x099970, 0x0999fa, 0x09a0b5, 0x09a6b5, 0x09ab10, 0x09ad21),       0x038f60,
        /* mac     */ (M(0x100377830), M(0x100379b40), M(0x100379e88), M(0x10037a0b4), M(0x10037a2e4), M(0x10037a52c),
                       M(0x10037a83c), M(0x10037aa68), M(0x10037aca0), M(0x10037adb8), M(0x10037d6b8), M(0x10037d7e8),
                       M(0x10037d964), M(0x10037da60), M(0x10037e01c), M(0x10037e8dc), M(0x10037ebe4), M(0x10037eeec)), M(0x10033da20)),
    [SITE_KEY_HELP_TEXT] = SITE("key_help_text", 0,
        /* windows */ (0x1b78be, 0x1b78db, 0x1b78f8, 0x1b7937, 0x1b7987),                               0x039180,
        /* mac     */ (M(0x100514a48), M(0x100514a64), M(0x100514a80), M(0x100514abc), M(0x100514b08)), M(0x10033d888)),
    [SITE_KEY_TEXT_BUTTON] = SITE("key_text_button", 0,
        /* windows */ (0x0a091c),       0x039ad0,
        /* mac     */ (M(0x100376a38)), M(0x10033d2e4)),
    [SITE_RUN_ONE_MOD] = SITE("run_one_mod", 0,
        /* windows */ (0x1cbb12, 0x1cabea),                                                             0x1bb4c0,
        /* mac     */ (M(0x10053a54c), M(0x10053a598), M(0x10053a5e4), M(0x10053a630), M(0x10053a67c), M(0x10053a6c8),
                       M(0x10053a714), M(0x10053a760), M(0x10053a7ac), M(0x1004eb784), M(0x1004f21a8)), M(0x100529d40)),
    [SITE_RUN_ONE_MOD_PATH] = SITE("run_one_mod_path", 0,
        /* windows */ (0x1bb5d2),       0x001430,
        /* mac     */ (M(0x100529e74)), M(0x1005aeeb4)),
    [SITE_RUN_ONE_MOD_LINE_READ] = SITE("run_one_mod_line_read", 0,
        /* windows */ (0x1bb78f),                                       0x00f950,
        /* mac     */ (M(0x10052a060), M(0x10052a228), M(0x10052a3b8)), M(0x1005ae578)),
    [SITE_ELIMINATED_WARN] = SITE("eliminated_warn", 0,
        /* windows */ (0x1112f2),       0x042f40,
        /* mac     */ (M(0x100472b14)), M(0x1003482ac)),
    [SITE_WARN_WAIT] = SITE("warn_wait", 0,
        /* windows */ (0x043166, 0x043186),             0x23f240,
        /* mac     */ (M(0x1003485c0), M(0x100348564)), M(0x100484ef8)),
    [SITE_GAME_MENU_SETTINGS_SECTION_END] = SITE("game_menu_settings_section_end", 0,
        /* windows */ (0x124858),       0x035400,
        /* mac     */ (M(0x1003805e8)), M(0x100340ac4)),
    // @platform-split tooltip_line_draw: Mac's call is RVA_RENDER_TEXT, with another signature
    [SITE_TOOLTIP_LINE_DRAW] = SITE("tooltip_line_draw", 0,
        /* windows */ (0x02a24b),       0x02f380,
        /* mac     */ (M(0x10048bae4)), M(0x10034d8c4)),
    [SITE_TURN_MESSAGES] = SITE("turn_messages", 0,
        /* windows */ (0x094cd7),       0x1e2210,
        /* mac     */ (M(0x10037b86c)), M(0x1004c2390)),
    [SITE_MESSAGES_WAIT] = SITE("messages_wait", 0,
        /* windows */ (0x1e29e9),       0x23f240,
        /* mac     */ (M(0x1004c2be8)), M(0x100484ef8)),
    // @platform-split ai_steps_mac: no Mac call; hooks at each end of the copied step
    [SITE_AI_EXPLORE] = SITE("ai_explore", 0,
        /* windows */ (0x070d8a), 0x074350,
        /* mac     */ (0),        0),
    // @platform-split ai_steps_mac: rally, friendly move and conquest have no Mac call
    [SITE_AI_RALLY] = SITE("ai_rally", 0,
        /* windows */ (0x070caa), 0x076730,
        /* mac     */ (0),        0),
    [SITE_AI_FRIENDLY_MOVE] = SITE("ai_friendly_move", 0,
        /* windows */ (0x070cf3), 0x076320,
        /* mac     */ (0),        0),
    [SITE_AI_CONQUER] = SITE("ai_conquer", 0,
        /* windows */ (0x070d3c), 0x075400,
        /* mac     */ (0),        0),
    [SITE_AI_FAR_EXPLORE] = SITE("ai_far_explore", 0,
        /* windows */ (0x070dd8), 0x073e40,
        /* mac     */ (M(0x1004da568)), M(0x1004d3888)),
    [SITE_AI_RITUAL_CANDIDATE] = SITE("ai_ritual_candidate", 0,
        /* windows */ (0x06bb37), 0x2cbe00,
        /* mac     */ (M(0x1004de000)), M(0x10055c75c)),
    // @platform-split ai_steps_mac: no Mac call; hooks at each end of the copied step
    [SITE_AI_RECRUIT] = SITE("ai_recruit", 0,
        /* windows */ (0x06fb2b), 0x0773f0,
        /* mac     */ (0),        0),
    // @platform-split ai_steps_mac: the troop pick-up and the split have no Mac call
    [SITE_AI_GATHER_TROOPS] = SITE("ai_gather_troops", 0,
        /* windows */ (0x070f03), 0x06edd0,
        /* mac     */ (0),        0),
    [SITE_AI_PICK_UP_LONER] = SITE("ai_pick_up_loner", 0,
        /* windows */ (0x07c26d), 0x129ad0,
        /* mac     */ (M(0x1004d16a8)), M(0x1004840fc)),
    [SITE_AI_SPLIT_ARMY] = SITE("ai_split_army", 0,
        /* windows */ (0x07b55f), 0x07b700,
        /* mac     */ (0),        0),
    [SITE_AI_MOVE_COMSTR] = SITE("ai_move_comstr", 0,
        /* windows */ (0x070f0d), 0x129320,
        /* mac     */ (M(0x1004db4ec)), M(0x10047f724)),
    [SITE_CEREMONY_PERFORM] = SITE("ceremony_perform", 0,
        /* windows */ (0x2d56c9, 0x2d56b9, 0x09ead9, 0x07b3ed, 0x07b40f, 0x07b431, 0x07b450, 0x07b47b, 0x07b4a6,
                       0x07b4c2, 0x07b53f, 0x07b54f), 0x2e2290,
        /* mac     */ (M(0x10035f5d4), M(0x100386f08), M(0x1004d20c0), M(0x1004d20e4), M(0x1004d2108), M(0x1004d21a8),
                       M(0x1004d2330), M(0x1004d23d4), M(0x1004d23f0), M(0x1004d24e0), M(0x1004d24f0)), M(0x1003522f0)),
    [SITE_CEREMONY_COSTS] = SITE("ceremony_costs", 0,
        /* windows */ (0x2d3ea1, 0x07b3cb, 0x09eac6), 0x2e3310,
        /* mac     */ (M(0x100386ef8), M(0x1004d209c)), M(0x100351bcc)),
    [SITE_SEAL_BREAK] = SITE("seal_break", 0,
        /* windows */ (0x2e27f2, 0x1851a2), 0x1f23f0,
        /* mac     */ (M(0x1003525c0), M(0x1004e829c)), M(0x10050ed2c)),
    [SITE_SEAL_EVENT] = SITE("seal_event", 0,
        /* windows */ (0), 0,
        /* mac     */ (0), 0),
    [SITE_MAP_EDITOR] = SITE("map_editor", 0,
        /* windows */ (0x192876, 0x1ca974, 0x191526),                   0x1b74b0,
        /* mac     */ (M(0x10032e6b8), M(0x10032ea24), M(0x10033389c)), M(0x100514604)),
    [SITE_MAP_EDITOR_TOOLBOX_LINE] = SITE("map_editor_toolbox_line", 0,
        /* windows */ (0x1b7909),       0x039ad0,
        /* mac     */ (M(0x100514a90)), M(0x10033d2e4)),
    [SITE_MAP_EDITOR_DRAW] = SITE("map_editor_draw", 0,
        /* windows */ (0x24084f),       0x02cfc0,
        /* mac     */ (M(0x10048aba8)), M(0x100345b20)),
    [SITE_MAP_DRAW] = SITE("map_draw", 0,
        /* windows */ (0),              0x02cfc0,
        /* mac     */ (0),              0),
    [SITE_TOP_BAR] = SITE("top_bar", 0,
        /* windows */ (0x095030),       0x09c4a0,
        /* mac     */ (0),              0),
    [SITE_TOP_BAR_INCOME_DRAW] = SITE("top_bar_income_draw", 0,
        /* windows */ (0x09c9f1, 0x09cad0, 0x09ce18, 0x09cef5, 0x09cfc9, 0x09d09d, 0x09d171, 0x09d260, 0x09d34a), 0x036120,
        /* mac     */ (0),              0),
    [SITE_MESSAGE_BOX_WAIT] = SITE("message_box_wait", 0,
        /* windows */ (0x0422ec),       0x23f240,
        /* mac     */ (0),              0),
    [SITE_TOP_BAR_INCOME] = SITE("top_bar_income", 0,
        /* windows */ (0),              0,
        /* mac     */ (0),              0),
};
#undef M
#undef SITE
#endif   // GAME_ADDRESSES_SITE_TABLE

// ---- Code CoE5SE copied ---------------------------------------------------------------------------
