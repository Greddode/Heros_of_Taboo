#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <stdbool.h>
#include "world.h"

// Returns true if the tile at (x, y) is walkable: in bounds, not blocked, no monster.
bool MovementSystem_IsWalkable(GameWorld* gw, int x, int y);

// Resolve single-step player movement in the given direction.
// Handles move, attack-on-bump, pickup collection, FOW reveal, stair/escape checks.
void MovementSystem_PlayerMove(GameWorld* gw, Direction dir);

// Update animation interpolation timers for all entities.
void MovementSystem_UpdateAnimations(GameWorld* gw, float dt);

#endif