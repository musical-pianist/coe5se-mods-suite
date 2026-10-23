// Every instruction in CoE5.exe that builds an address in the game's army_list_least_b table.
#pragma once

#define ARMY_LIST_LEAST_B_TABLE_SITE_COUNT 7
static const TableMoveSite ARMY_LIST_LEAST_B_TABLE_SITES[ARMY_LIST_LEAST_B_TABLE_SITE_COUNT] = {
    {0x1047d6, 3, TABLE_FIELD_REL32},
    {0x104972, 3, TABLE_FIELD_REL32},
    {0x104a76, 3, TABLE_FIELD_REL32},
    {0x11d830, 3, TABLE_FIELD_REL32},
    {0x11dfb5, 3, TABLE_FIELD_REL32},
    {0x11e14e, 3, TABLE_FIELD_REL32},
    {0x11e1e8, 3, TABLE_FIELD_REL32},
};
