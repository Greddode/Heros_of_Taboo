#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"
#include <stdbool.h>
#include "game_types.h"
#include "data/equip_data.h"
#include "data/ability_data.h"

typedef struct GameWorld GameWorld;

#define MAX_POTIONS 32
#define MAX_EQUIP_ON_MAP 64
#define MAX_INVENTORY_SLOTS 16
#define EQUIP_SLOT_COUNT 5
#define MAX_EQUIPMENT_TYPES 32

typedef struct EquipData {
    EquipType type;
    char name[32];
    const char* description;
    const char* spritePath;
    EquipCategory category;
    EquipSlot slot;
    int bonusAttack;
    int bonusDefense;
    int bonusMaxHp;
    int bonusStr;
    int bonusDex;
    int bonusInt;
    int bonusCon;
    int bonusLck;
    bool twoHanded;
    bool isRanged;
    ItemRarity rarity;
} EquipData;

typedef struct {
    ItemType type;
    int quantity;
} InventorySlot;

typedef enum {
    INV_BROWSE,
    INV_ACTION_MENU
} InventorySubState;

typedef enum {
    INV_TAB_INVENTORY = 0,
    INV_TAB_EQUIPMENT,
    INV_TAB_STATS,
    INV_TAB_COUNT
} InventoryTab;

// Equipment data table
const EquipData* GetEquipData(EquipType type);
int GetEquipRangeBonus(EquipType type);

// Item functions
const char* GetItemName(ItemType type);
int GetItemHealAmount(ItemType type);
const char* GetItemDescription(ItemType type);
bool InventoryAdd(GameWorld* game, ItemType type);
bool InventoryUse(GameWorld* game, int slot);

// Equipment management
bool EquipItem(GameWorld* game, EquipType type);
void EquipItemSilent(GameWorld* game, EquipType type);
void UnequipSlot(GameWorld* game, EquipSlot slot);
bool IsEquipSlotOccupied(const GameWorld* game, EquipSlot slot);
bool IsTwoHandedEquipped(const GameWorld* game);
bool IsWeaponDualWieldable(EquipType type);
bool IsDualWielding(const GameWorld* game);
bool AddEquipToInventory(GameWorld* game, EquipType type);
bool RemoveEquipFromInventory(GameWorld* game, int slot);

// Pre-load potion/UI textures into the ResourceManager cache.
void LoadPotionTextures(GameWorld* game);
// Cached equip icon from the resource manager (NULL if none / load failed).
Texture2D* Inventory_LoadEquipTexture(EquipType type);
Texture2D* Inventory_LoadPotionTexture(ItemType type);
AbilityType Inventory_GetWeaponAbility(EquipType weapon);

#endif
