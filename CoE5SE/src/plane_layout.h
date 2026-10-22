// What a mod's plane declares past its recipe: set pieces, zones and unit lists.
#pragma once

#define PLANE_PIECE_LINES_MAX     16
#define PLANE_PIECE_COUNT_MAX     32
#define PLANE_PIECES_PLACED_MAX   128
#define PLANE_ZONES_MAX           8
#define PLANE_ZONE_SWAP_MAX       32
#define PLANE_UNIT_LINES_MAX      16
#define PLANE_UNIT_NAME_SIZE      52
#define PLANE_UNIT_COUNT_MAX      1000
#define PLANE_ZONE_RADIUS_MAX     999

enum { PIECE_CENTRE, PIECE_RANDOM, PIECE_AT, PIECE_NEAR_PORTAL };

// A planepiece line: a .coem file found in its mod's folder, its size from its mapsize, and where it goes.
typedef struct {
    char file[MAX_PATH];
    int w, h;
    int placement, count, x, y;
    int line;
} PlanePiece;

// One monster of a unit list: how many (with Wilder off) and the chance per square, in percent.
typedef struct {
    char monster[PLANE_UNIT_NAME_SIZE];
    int count, chance, line;
} PlaneUnits;

typedef struct {
    PlaneUnits units[PLANE_UNIT_LINES_MAX];
    int count;
} PlaneUnitList;

// A planezone: the squares within `radius` (the game's square distance) of each copy of piece `piece`.
typedef struct {
    char name[PLANE_NAME_SIZE];
    int piece, radius, line;
    int swapCount;
    short swapFrom[PLANE_ZONE_SWAP_MAX], swapTo[PLANE_ZONE_SWAP_MAX];
    int populateOff;
    PlaneUnitList units;
} PlaneZone;

// A stamped piece's rectangle in world squares, at the last world build.
typedef struct { int piece, x, y, w, h; } PlacedPiece;

typedef struct {
    PlanePiece pieces[PLANE_PIECE_LINES_MAX];
    int pieceCount;
    PlaneZone zones[PLANE_ZONES_MAX];
    int zoneCount;
    PlaneUnitList units;
    char source[MAX_PATH];
    int script;
    PlacedPiece placed[PLANE_PIECES_PLACED_MAX];
    int placedCount, wayInX, wayInY;
} PlaneLayout;
