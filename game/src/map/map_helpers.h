#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include "raylib.h"
#include <stdbool.h>
#include "map/tmx/tmx.h"

typedef struct GameWorld GameWorld;

bool IsInRoom(int x, int y);
void RevealFOW(GameWorld* game);
void SpawnEscapeTile(GameWorld* game);
void SpawnShadow(GameWorld* game);
void BuildBlockingMap(GameWorld* game);
void SpawnEntitiesFromObjects(GameWorld* game);

// TMX and Coordinate helper functions
Vector2 TileToScreen(int x, int y, int tw, int th);

#endif
