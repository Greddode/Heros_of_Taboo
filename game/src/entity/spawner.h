#ifndef SPAWNER_H
#define SPAWNER_H

#include "game.h"
#include "procedural.h"

// Populate dungeon rooms with monsters and healing items.
// Uses room data from procedural generation to place enemies and pickups
// at valid floor tiles, avoiding the player's starting area.
void Spawner_Populate(Game* game, const ProceduralRoom* rooms, int roomCount);

#endif
