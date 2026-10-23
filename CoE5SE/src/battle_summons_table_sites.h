// Every instruction in CoE5.exe that builds an address in the game's battle_summons table.
#pragma once

#define BATTLE_SUMMONS_TABLE_SITE_COUNT 10
static const TableMoveSite BATTLE_SUMMONS_TABLE_SITES[BATTLE_SUMMONS_TABLE_SITE_COUNT] = {
    {0x0b960e, 5, TABLE_FIELD_DISP32},
    {0x0dc756, 5, TABLE_FIELD_DISP32},
    {0x0dc7a0, 5, TABLE_FIELD_DISP32},
    {0x0dc7d4, 5, TABLE_FIELD_DISP32},
    {0x0df1f4, 5, TABLE_FIELD_DISP32},
    {0x0df224, 5, TABLE_FIELD_DISP32},
    {0x0df234, 5, TABLE_FIELD_DISP32},
    {0x0df291, 4, TABLE_FIELD_DISP32},
    {0x0f74a6, 3, TABLE_FIELD_REL32},
    {0x2e5b77, 4, TABLE_FIELD_DISP32},
};
