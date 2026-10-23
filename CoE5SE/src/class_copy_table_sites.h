// Every instruction in CoE5.exe that builds an address in the game's class_copy table.
#pragma once

#define CLASS_COPY_TABLE_SITE_COUNT 5
static const TableMoveSite CLASS_COPY_TABLE_SITES[CLASS_COPY_TABLE_SITE_COUNT] = {
    {0x094049, 3, TABLE_FIELD_REL32},
    {0x09436a, 3, TABLE_FIELD_REL32},
    {0x0b079a, 3, TABLE_FIELD_REL32},
    {0x0b0814, 3, TABLE_FIELD_REL32},
    {0x190f56, 3, TABLE_FIELD_REL32},
};
