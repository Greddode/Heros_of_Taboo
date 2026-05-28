#ifndef SPAWNER_SYSTEM_H
#define SPAWNER_SYSTEM_H

#include "world.h"
#include "core/procedural.h"

// Convert monster pool into ECS entities. Called once after map load.
void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount);

// Convert potions and equip pickups on map into ECS entities.
void SpawnerSystem_SpawnPickups(GameWorld* gw);

#endif
