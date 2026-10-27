// The slot counts for mods' factions, and reading the per-player tables wherever the game reads them now.
#define PLAYER_SLOTS_GAME      PLAYER_SLOTS
#define FACTION_SLOT_TARGET    16
#define PLAYER_SLOTS_MOVED     (PLAYER_SLOTS_GAME + FACTION_SLOT_TARGET)
#define PLAYER_SEATS           24

// Where the game reads an address in one of its per-player tables now (the address itself before the move).
static unsigned char *playerTableAddress(unsigned int rva);
// How many slots the game holds now: 32, or 48 once moved.
static int playerSlotCount(void);
// Player `player`'s entry of a table, from player 0's address `rva0` and the entry size: where the game reads it now.
static unsigned char *playerSlotAddress(unsigned int rva0, int player, int entry);
// Reads and writes through playerTableAddress, as G16 and the rest do through the game's own address.
#define PG8(rva)  (*(signed char *)playerTableAddress(rva))
#define PG16(rva) (*(short *)playerTableAddress(rva))
#define PG32(rva) (*(int *)playerTableAddress(rva))
#define PGF(rva)  (*(float *)playerTableAddress(rva))
