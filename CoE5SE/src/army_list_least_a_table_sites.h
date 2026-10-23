// Every instruction in CoE5.exe that builds an address in the game's army_list_least_a table.
#pragma once

#define ARMY_LIST_LEAST_A_TABLE_SITE_COUNT 8
static const TableMoveSite ARMY_LIST_LEAST_A_TABLE_SITES[ARMY_LIST_LEAST_A_TABLE_SITE_COUNT] = {
    {0x1047bf, 3, TABLE_FIELD_REL32},
    {0x1048a8, 3, TABLE_FIELD_REL32},
    {0x104954, 3, TABLE_FIELD_REL32},
    {0x104a6a, 3, TABLE_FIELD_REL32},
    {0x11d824, 3, TABLE_FIELD_REL32},
    {0x11df9e, 3, TABLE_FIELD_REL32},
    {0x11e12c, 3, TABLE_FIELD_REL32},
    {0x11e19b, 3, TABLE_FIELD_REL32},
};
