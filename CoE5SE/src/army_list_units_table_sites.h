// Every instruction in CoE5.exe that builds an address in the game's army_list_units table.
#pragma once

#define ARMY_LIST_UNITS_TABLE_SITE_COUNT 10
static const TableMoveSite ARMY_LIST_UNITS_TABLE_SITES[ARMY_LIST_UNITS_TABLE_SITE_COUNT] = {
    {0x1047ab, 3, TABLE_FIELD_REL32},
    {0x104a52, 3, TABLE_FIELD_REL32},
    {0x11d6ac, 3, TABLE_FIELD_REL32},
    {0x11d7e1, 3, TABLE_FIELD_REL32},
    {0x11dcd2, 3, TABLE_FIELD_REL32},
    {0x11de1e, 3, TABLE_FIELD_REL32},
    {0x11df84, 3, TABLE_FIELD_REL32},
    {0x11e0be, 3, TABLE_FIELD_REL32},
    {0x11e450, 3, TABLE_FIELD_REL32},
    {0x11e561, 3, TABLE_FIELD_REL32},
};
