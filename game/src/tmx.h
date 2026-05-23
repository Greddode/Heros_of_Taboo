#ifndef TMX_H
#define TMX_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_TILESETS 8
#define MAX_LAYERS 8
#define MAX_OBJECTS 64
#define MAP_NAME_LEN 64

// Tileset loaded from a <tileset> tag (inline or external .tsx)
typedef struct {
    int firstGID;               // first global tile ID in this tileset
    char name[64];              // tileset name
    int tileWidth, tileHeight;  // pixel dimensions of a single tile
    int tileCount;              // total tiles in the set
    int columns;                // tiles per row in the spritesheet image
    char imageSource[256];      // path to the spritesheet PNG
    int imageWidth, imageHeight;
} TilesetDef;

// A single tile layer from a <layer> tag
typedef struct {
    char name[64];
    int width, height;          // tile count in each dimension
    int* data;                  // flat array of GIDs (width*height), 0 = empty
    bool visible;
} TileLayer;

// Object from an <objectgroup> tag (used for player/monster/item spawns)
typedef struct {
    char name[64];
    char type[32];              // identifies what kind of entity to spawn
    float x, y;                 // pixel position (convert to tile coords when spawning)
    int gid;                    // optional tile GID for object images
    bool hasGID;
} MapObject;

// A complete map loaded from a .tmx file (or generated procedurally)
typedef struct {
    int width, height;          // map dimensions in tiles
    int tileWidth, tileHeight;  // pixel size of each tile
    char orientation[16];       // "orthogonal", "isometric", etc.

    int tilesetCount;
    TilesetDef tilesets[MAX_TILESETS];

    int layerCount;
    TileLayer layers[MAX_LAYERS];

    int objectCount;
    MapObject objects[MAX_OBJECTS];
} MapData;

// Load a TMX file. Returns NULL on failure.
MapData* LoadTMX(const char* filename);

// Free a loaded map
void UnloadTMX(MapData* map);

// Get tile GID at (x,y) in the given layer. Returns 0 for empty/out of bounds.
int GetTileGID(const MapData* map, int layerIndex, int x, int y);

// Get the source rectangle for a tile GID from the tileset
Rectangle GetTileSourceRect(const MapData* map, int gid);

// Find the tileset index that contains the given GID
int FindTilesetForGID(const MapData* map, int gid);

#endif // TMX_H