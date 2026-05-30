#ifndef PROCEDURAL_H
#define PROCEDURAL_H

#include "map/tmx/tmx.h"

// Tile GID constants — VelmoraRealms tileset (448×400, 28 cols, 700 tiles).
#define TILE_WALL_CORNER_NW   14
#define TILE_WALL_NORTH       15
#define TILE_WALL_CORNER_NE   17
#define TILE_WALL_ICORNER_SW  411
#define TILE_WALL_ICORNER_SE  408
#define TILE_WALL_ICORNER_NW  495
#define TILE_WALL_ICORNER_NE  492
#define TILE_WALL_CORNER_SW  98
#define TILE_WALL_CORNER_SE  101
#define TILE_WALL_EAST       45
#define TILE_WALL_SOUTH      99
#define TILE_WALL_WEST       42
#define TILE_FLOOR           27 
#define TILE_STAIRS          137
#define TILE_ESCAPE          47
#define FLOOR_VARIANT_COUNT 8
static const int FLOOR_VARIANTS[FLOOR_VARIANT_COUNT] = { 23, 24, 48, 49, 50, 51, 52, 53 };

int GetStairX(void);
int GetStairY(void);

// Returns 1 if the given GID is any floor variant (used for collision, wall placement, and spawning).
int IsFloorGID(int gid);

#define MAX_GENERATED_ROOMS 8

// Metadata for a single generated room (position, size, centre).
typedef struct {
    int x, y;    // top-left tile
    int w, h;    // width and height in tiles
    int cx, cy;  // centre tile (for hall connections)
} ProceduralRoom;

// Generate a complete dungeon map. Returns a MapData that must be freed with UnloadTMX().
// If generateStairs is non-zero, a stair tile (TILE_STAIRS) is placed in a random room.
MapData* GenerateProceduralMap(int width, int height, int generateStairs);

// Fill an array with the room metadata from the last generation.
// Returns the number of rooms written.
int GetGeneratedRooms(ProceduralRoom* outRooms, int maxRooms);

#endif
