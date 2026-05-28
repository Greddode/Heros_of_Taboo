#ifndef SPAWNER_SYSTEM_H
#define SPAWNER_SYSTEM_H

#include "world.h"
#include "core/procedural.h"

// Convert monster pool into ECS entities. Called once after map load.
void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount);

// Convert potions and equip pickups on map into ECS entities.
void SpawnerSystem_SpawnPickups(GameWorld* gw);

// Find a pickup entity at (x, y). Returns ENTITY_NONE if none.
EntityId SpawnerSystem_FindPickupAt(const GameWorld* gw, int x, int y);

// Collect all potion pickups at (x, y). Returns count collected.
int SpawnerSystem_CollectPickupsAt(GameWorld* gw, int x, int y, ItemType* outTypes, int* outQtys, int maxSlots);

// Collect all equipment pickups at (x, y). Returns count collected.
int SpawnerSystem_CollectEquipAt(GameWorld* gw, int x, int y, EquipType* outTypes, int* outQtys, int maxSlots);

#endif
