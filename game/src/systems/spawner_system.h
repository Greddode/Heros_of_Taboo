#ifndef SPAWNER_SYSTEM_H
#define SPAWNER_SYSTEM_H

#include "world.h"
#include "map/procedural.h"
#include "game_types.h"

// Spawn one monster entity from template data.
EntityId SpawnerSystem_SpawnMonster(GameWorld* gw, MonsterType type, int x, int y, int floor);
EntityId SpawnerSystem_SpawnByTypeName(GameWorld* gw, const char* tmxTypeName, int x, int y, int floor);
void SpawnerSystem_ConfigureShadow(GameWorld* gw, EntityId shadow, int playerLevel);

// Populate rooms with ECS monster entities. Called once after map load.
void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount,
                                 int playerX, int playerY);

// New DR-budget spawner (biome-aware, equipment-aware, CR-based).
void SpawnMonstersForFloor(GameWorld* game);
void SpawnShopRoom(GameWorld* game);

// Spawn loot pickups directly into ECS from room data.
void SpawnerSystem_SpawnPickups(GameWorld* gw);

// Find a pickup entity at (x, y). Returns ENTITY_NONE if none.
EntityId SpawnerSystem_FindPickupAt(GameWorld* gw, int x, int y);

// Collect all potion pickups at (x, y). Returns count collected.
int SpawnerSystem_CollectPickupsAt(GameWorld* gw, int x, int y, ItemType* outTypes, int* outQtys, int maxSlots);

// List equipment pickups at (x, y) without removing them (caller applies inventory). Returns entry count.
int SpawnerSystem_CollectEquipAt(GameWorld* gw, int x, int y, EquipType* outTypes, int* outQtys, int maxSlots);

// Reduce stacked equipment quantity after partial pickup.
void SpawnerSystem_ReduceEquipAt(GameWorld* gw, int x, int y, EquipType type, int amount);

// Drop or stack a potion / equipment pickup on the map.
void SpawnerSystem_AddPotionAt(GameWorld* gw, int x, int y, ItemType type, int qty);
void SpawnerSystem_AddEquipAt(GameWorld* gw, int x, int y, EquipType type, int qty);

// TMX healing object: random potion rarity at (x, y).
void SpawnerSystem_SpawnHealingPotionAt(GameWorld* gw, int x, int y);

// Inspector: list pickups at a tile without collecting.
int SpawnerSystem_ListPotionsAt(GameWorld* gw, int x, int y,
                                ItemType* outTypes, int* outQtys, int maxSlots);
int SpawnerSystem_ListEquipAt(GameWorld* gw, int x, int y,
                              EquipType* outTypes, int* outQtys, int maxSlots);

#endif
