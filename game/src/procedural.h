#ifndef PROCEDURAL_H
#define PROCEDURAL_H

#include "tmx.h"

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

typedef struct {
    int x, y, w, h, cx, cy;
} ProceduralRoom;

MapData* GenerateProceduralMap(int width, int height);
int GetGeneratedRooms(ProceduralRoom* outRooms, int maxRooms);

#endif
