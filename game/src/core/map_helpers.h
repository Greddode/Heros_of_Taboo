#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include "raylib.h"
#include <stdbool.h>

typedef struct Game Game;

bool IsInRoom(int x, int y);
void RevealFOW(Game* game);
void SpawnEscapeTile(Game* game);
void SpawnShadow(Game* game);
void BuildBlockingMap(Game* game);
void SpawnEntitiesFromObjects(Game* game);

#endif
