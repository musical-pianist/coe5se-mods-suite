// Every instruction in CoE5.exe that builds an address in the game's class_heroes table.
#pragma once

#define CLASS_HEROES_TABLE_SITE_COUNT 7
static const TableMoveSite CLASS_HEROES_TABLE_SITES[CLASS_HEROES_TABLE_SITE_COUNT] = {
    {0x08a1a9, 3, TABLE_FIELD_DISP32},
    {0x08a250, 4, TABLE_FIELD_DISP32},
    {0x08a25e, 4, TABLE_FIELD_DISP32},
    {0x08a3a0, 3, TABLE_FIELD_DISP32},
    {0x08a6c3, 3, TABLE_FIELD_DISP32},
    {0x1bdb01, 3, TABLE_FIELD_DISP32},
    {0x1bdb10, 3, TABLE_FIELD_DISP32},
};
