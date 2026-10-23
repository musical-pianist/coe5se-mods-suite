// The game functions CoE5SE's fan patch fixes hook at their start: how many bytes each hook replaces.
#pragma once

#define ENTRY_HOOK_COUNT 7
#ifndef __APPLE__
static const EntryHookSite ENTRY_HOOK_SITES[ENTRY_HOOK_COUNT] = {
    {"find_all_mods", 0x1c9d20, 5, 0, {{0, 0}}},
    {"restore_tables", 0x1cacb0, 6, 0, {{0, 0}}},
    {"add_affliction", 0x0f4dc0, 8, 1, {{4, 0x0f51ec}}},
    {"pick_spell", 0x2e59d0, 6, 0, {{0, 0}}},
    {"rebuild_spells", 0x2e6850, 7, 0, {{0, 0}}},
    {"unit_card", 0x1d4990, 5, 0, {{0, 0}}},
    {"draw_sprite", 0x0651a0, 7, 0, {{0, 0}}},
};
#else
static const EntryHookSite ENTRY_HOOK_SITES[ENTRY_HOOK_COUNT] = {
    {"find_all_mods", 0x53a1dc, 16, 0, {{0, 0}}},
    {"restore_tables", 0x528d1c, 16, 0, {{0, 0}}},
    {"add_affliction", 0x3ca400, 16, 0, {{0, 0}}},
    {"pick_spell", 0x4bd100, 16, 0, {{0, 0}}},
    {"rebuild_spells", 0x4bd880, 16, 0, {{0, 0}}},
    {"unit_card", 0x52113c, 16, 0, {{0, 0}}},
    {"draw_sprite", 0x34f5c4, 16, 0, {{0, 0}}},
};
#endif
