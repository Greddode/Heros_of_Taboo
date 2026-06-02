#ifndef FLOOR_INIT_H
#define FLOOR_INIT_H

#include "world.h"

// =============================================================================
// floor_init.h — Per-floor initialization shared between InitGame and DescendFloor
//
// Floor_InitNewFloor performs all setup that happens identically every time
// a new floor is entered (first floor or descend).  Callers are responsible
// for map creation, player setup, and inventory/equipment before calling this.
// =============================================================================

// Perform per-floor setup after a map has been loaded/generated.
// game   — initialized GameWorld with a valid game->map
void Floor_InitNewFloor(GameWorld* game);

#endif
