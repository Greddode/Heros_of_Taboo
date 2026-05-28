#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include "raylib.h"
#include <stdbool.h>
#include "tmx/tmx.h"

typedef struct Game Game;

bool IsInRoom(int x, int y);
void RevealFOW(Game* game);
void SpawnEscapeTile(Game* game);
void SpawnShadow(Game* game);
void BuildBlockingMap(Game* game);
void SpawnEntitiesFromObjects(Game* game);

// TMX and Coordinate helper functions
int FindTilesetForGID(const MapData* map, int gid);
Rectangle GetTileSrcRect(const MapData* map, int gid, int tsIdx);
Vector2 TileToScreen(int x, int y, int tw, int th);

#endif
