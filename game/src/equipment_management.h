#ifndef EQUIPMENT_MANAGEMENT_H
#define EQUIPMENT_MANAGEMENT_H

#include "game_types.h"
#include <stdbool.h>

struct GameWorld;

bool EquipItem(struct GameWorld* game, EquipType type);
void EquipItemSilent(struct GameWorld* game, EquipType type);
void UnequipSlot(struct GameWorld* game, EquipSlot slot);
bool IsEquipSlotOccupied(const struct GameWorld* game, EquipSlot slot);
bool IsTwoHandedEquipped(const struct GameWorld* game);
bool IsWeaponDualWieldable(EquipType type);
bool IsDualWielding(const struct GameWorld* game);
bool AddEquipToInventory(struct GameWorld* game, EquipType type);
bool RemoveEquipFromInventory(struct GameWorld* game, int slot);

#endif
