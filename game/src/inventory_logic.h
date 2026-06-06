#ifndef INVENTORY_LOGIC_H
#define INVENTORY_LOGIC_H

#include "game_types.h"
#include "data/ability_data.h"
#include "raylib.h"
#include <stdbool.h>

struct GameWorld;

bool InventoryAdd(struct GameWorld* game, ItemType type);
bool InventoryUse(struct GameWorld* game, int slot);
void LoadPotionTextures(struct GameWorld* game);
Texture2D* Inventory_LoadEquipTexture(EquipType type);
Texture2D* Inventory_LoadPotionTexture(ItemType type);
AbilityType Inventory_GetWeaponAbility(EquipType weapon);

#endif
