#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "world.h"

// Monsters-only spatial occupancy grid (MAP_HEIGHT × MAP_WIDTH).
// Maps tile coordinates → EntityId (or ENTITY_NONE if empty).
// Updated eagerly on spawn, move, and death so that World_FindMonsterAt
// becomes O(1) instead of O(MAX_ENTITIES).

void SpatialHash_Clear(GameWorld* gw);
void SpatialHash_Add(GameWorld* gw, EntityId e, int x, int y);
void SpatialHash_Remove(GameWorld* gw, EntityId e, int x, int y);
void SpatialHash_Move(GameWorld* gw, EntityId e, int oldX, int oldY, int newX, int newY);

#endif
