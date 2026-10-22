// Every instruction in CoE5.exe that builds an address in the game's monster_sprite_labels table, as shipped.
#pragma once

#define MONSTER_SPRITE_LABELS_SITES_EXE_SHA256 "c894c3d0d208fb3cfa6a5a87964b6c1cf14837ae8caffa1cb4d8a81226aa0bb2"
#define MONSTER_SPRITE_LABELS_TABLE_SITE_COUNT 2
static const TableMoveSite MONSTER_SPRITE_LABELS_TABLE_SITES[MONSTER_SPRITE_LABELS_TABLE_SITE_COUNT] = {
    {0x088090, 4, TABLE_FIELD_DISP32, {0x41, 0x8b, 0x84, 0xaf, 0xf0, 0xf7, 0x82, 0x05}},
    {0x1d9ef5, 3, TABLE_FIELD_REL32, {0x4c, 0x8d, 0x1d, 0xf4, 0x58, 0x65, 0x05}},
};
