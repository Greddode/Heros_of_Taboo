#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"
#include <stdbool.h>

typedef struct Game Game;

#define MAX_POTIONS 32
#define MAX_INVENTORY_SLOTS 16
#define EQUIP_SLOT_COUNT 5
#define MAX_EQUIPMENT_TYPES 32

typedef enum {
    ITEM_NONE = 0,
    ITEM_SMALL_HP_POTION,
    ITEM_BIG_HP_POTION,
    ITEM_LARGE_HP_POTION,
    ITEM_COUNT
} ItemType;

typedef enum {
    EQUIP_SLOT_HEAD = 0,
    EQUIP_SLOT_CHEST,
    EQUIP_SLOT_WEAPON,
    EQUIP_SLOT_OFF_HAND,
    EQUIP_SLOT_ACCESSORY
} EquipSlot;

typedef enum {
    EQUIP_CAT_ARMOR = 0,
    EQUIP_CAT_WEAPON,
    EQUIP_CAT_ACCESSORY,
    EQUIP_CAT_COUNT
} EquipCategory;

typedef enum {
    EQUIP_NONE = 0,
    // Armor (Head)
    EQUIP_LEATHER_CAP,
    EQUIP_IRON_HELM,
    EQUIP_STEEL_HELM,
    // Armor (Chest)
    EQUIP_LEATHER_VEST,
    EQUIP_CHAIN_MAIL,
    EQUIP_PLATE_MAIL,
    // Weapons
    EQUIP_SURVIVAL_KNIFE,
    EQUIP_IRON_SWORD,
    EQUIP_STEEL_SWORD,
    EQUIP_WAR_HAMMER,
    // Off-hand
    EQUIP_WOODEN_SHIELD,
    EQUIP_IRON_SHIELD,
    EQUIP_STEEL_SHIELD,
    // Accessories
    EQUIP_RING_OF_STRENGTH,
    EQUIP_AMULET_OF_WARDING,
    EQUIP_BOOTS_OF_SWIFTNESS,
    EQUIP_COUNT
} EquipType;

typedef struct {
    EquipType type;
    char name[32];
    const char* description;
    EquipCategory category;
    EquipSlot slot;
    int bonusAttack;
    int bonusDefense;
    int bonusMaxHp;
} EquipData;

typedef struct {
    EquipType type;
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

// Item functions
const char* GetItemName(ItemType type);
int GetItemHealAmount(ItemType type);
const char* GetItemDescription(ItemType type);
bool InventoryAdd(Game* game, ItemType type);
bool InventoryUse(Game* game, int slot);

// Equipment management
void EquipItem(Game* game, EquipType type);
void EquipItemSilent(Game* game, EquipType type);
void UnequipSlot(Game* game, EquipSlot slot);
bool IsEquipSlotOccupied(const Game* game, EquipSlot slot);

// Inventory rendering
void Inventory_Render(const Game* game);
void LoadPotionTextures(Game* game);
void UnloadPotionTextures(Game* game);

#endif