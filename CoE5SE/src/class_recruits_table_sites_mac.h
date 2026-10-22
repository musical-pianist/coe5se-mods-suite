// Every ADRP in coe5_mac that builds an address in the game's class_recruits table, and its bounds, as shipped.
#pragma once

#define CLASS_RECRUITS_MAC_SITE_COUNT 17
static const TableMacSite CLASS_RECRUITS_MAC_SITES[CLASS_RECRUITS_MAC_SITE_COUNT] = {
    {0x332be8, 0xb0002a01},
    {0x361804, 0xd0002889},
    {0x365d6c, 0xd0002869},
    {0x365de4, 0xd000286c},
    {0x365eb8, 0xd0002869},
    {0x4c1f9c, 0x90001e76},
    {0x51f6fc, 0x90001aaa},
    {0x51fa4c, 0x90001aaa},
    {0x528eec, 0xf0001a40},
    {0x530ca8, 0xf0001a09},
    {0x530d8c, 0xf0001a0a},
    {0x531120, 0xd0001a0a},
    {0x5311b0, 0xd0001a0a},
    {0x531224, 0xd0001a0a},
    {0x531290, 0xd0001a0b},
    {0x5312fc, 0xd0001a09},
    {0x5313cc, 0xd0001a14},
};

// The table's size in bytes, 200 records, built by a movz and a movk (the copies' and clears' sizes): each becomes the moved size.
#define CLASS_RECRUITS_MAC_SIZE_COUNT 3
static const TableMacSize CLASS_RECRUITS_MAC_SIZES[CLASS_RECRUITS_MAC_SIZE_COUNT] = {
    {0x332bd4, 0x52870000, 0x72a02880},
    {0x332bf0, 0x52870002, 0x72a02882},
    {0x528ef4, 0x52870002, 0x72a02882},
};
