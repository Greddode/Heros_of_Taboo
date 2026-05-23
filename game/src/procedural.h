#ifndef PROCEDURAL_H
#define PROCEDURAL_H

#include "tmx.h"

// Tile GID constants — these match specific tiles in the gray tileset.
// Each defines a wall orientation or floor type.
#define TILE_WALL_CORNER_NW   1
#define TILE_WALL_NORTH       2
#define TILE_WALL_CORNER_NE   4
#define TILE_WALL_ICORNER_SW  20
#define TILE_WALL_ICORNER_SE  22
#define TILE_WALL_ICORNER_NW  52
#define TILE_WALL_ICORNER_NE  54
#define TILE_WALL_CORNER_SW  65
#define TILE_WALL_CORNER_SE  68
#define TILE_WALL_EAST       26
#define TILE_WALL_SOUTH      66
#define TILE_WALL_WEST       17
#define TILE_FLOOR           18

#define MAX_GENERATED_ROOMS 8

// Metadata for a single generated room (position, size, centre).
typedef struct {
    int x, y;    // top-left tile
    int w, h;    // width and height in tiles
    int cx, cy;  // centre tile (for hall connections)
} ProceduralRoom;

// Generate a complete dungeon map. Returns a MapData that must be freed with UnloadTMX().
MapData* GenerateProceduralMap(int width, int height);

// Fill an array with the room metadata from the last generation.
// Returns the number of rooms written.
int GetGeneratedRooms(ProceduralRoom* outRooms, int maxRooms);

#endif
