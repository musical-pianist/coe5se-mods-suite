// Every instruction in CoE5.exe that builds an address in the game's monster_sprite_labels table.
#pragma once

#define MONSTER_SPRITE_LABELS_TABLE_SITE_COUNT 2
static const TableMoveSite MONSTER_SPRITE_LABELS_TABLE_SITES[MONSTER_SPRITE_LABELS_TABLE_SITE_COUNT] = {
    {0x0882a0, 4, TABLE_FIELD_DISP32},
    {0x1da169, 3, TABLE_FIELD_REL32},
};
