#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <stdbool.h>
#include "world.h"

// Returns true if the tile at (x, y) is walkable: in bounds, not blocked, no monster.
bool MovementSystem_IsWalkable(const GameWorld* gw, int x, int y);

#endif