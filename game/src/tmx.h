#ifndef TMX_H
#define TMX_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_TILESETS 8
#define MAX_LAYERS 8
#define MAX_OBJECTS 64
#define MAP_NAME_LEN 64

// Tileset loaded from TMX
typedef struct {
    int firstGID;
    char name[64];
    int tileWidth;
    int tileHeight;
    int tileCount;
    int columns;
    char imageSource[256];
    int imageWidth;
    int imageHeight;
} TilesetDef;

// A tile layer from TMX
typedef struct {
    char name[64];
    int width;
    int height;
    int* data;        // Array of GIDs (width * height), 0 = empty
    bool visible;
} TileLayer;

// Object from object group in TMX (for player/enemy spawns)
typedef struct {
    char name[64];
    char type[32];
    float x;
    float y;
    int gid;          // Optional tile GID
    bool hasGID;
} MapObject;

// Complete map loaded from TMX
typedef struct {
    int width;          // Map width in tiles
    int height;         // Map height in tiles  
    int tileWidth;      // Tile width in pixels
    int tileHeight;     // Tile height in pixels
    char orientation[16]; // "orthogonal", "isometric", etc.
    
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