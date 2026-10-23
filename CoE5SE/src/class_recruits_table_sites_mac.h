// Every ADRP in coe5_mac that builds an address in the game's class_recruits table, and its bounds.
#pragma once

#define CLASS_RECRUITS_MAC_SITE_COUNT 17
static const TableMacSite CLASS_RECRUITS_MAC_SITES[CLASS_RECRUITS_MAC_SITE_COUNT] = {
    {0x332704},
    {0x36144c},
    {0x3659b4},
    {0x365a2c},
    {0x365b00},
    {0x4c1c60},
    {0x51f5d0},
    {0x51f920},
    {0x528dc0},
    {0x530bc4},
    {0x530ca8},
    {0x53103c},
    {0x5310cc},
    {0x531140},
    {0x5311ac},
    {0x531218},
    {0x5312e8},
};

// The table's size in bytes, 200 records, built by a movz and a movk (the copies' and clears' sizes): each becomes the moved size.
#define CLASS_RECRUITS_MAC_SIZE_COUNT 3
static const TableMacSize CLASS_RECRUITS_MAC_SIZES[CLASS_RECRUITS_MAC_SIZE_COUNT] = {
    {0x3326f0},
    {0x33270c},
    {0x528dc8},
};
