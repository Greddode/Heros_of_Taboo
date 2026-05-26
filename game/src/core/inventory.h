#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"
#include <stdbool.h>

typedef struct Game Game;

#define MAX_POTIONS 32
#define MAX_INVENTORY_SLOTS 16

typedef enum {
    ITEM_NONE = 0,
    ITEM_SMALL_HP_POTION,
    ITEM_BIG_HP_POTION,
    ITEM_LARGE_HP_POTION,
    ITEM_COUNT
} ItemType;

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

const char* GetItemName(ItemType type);
int GetItemHealAmount(ItemType type);
const char* GetItemDescription(ItemType type);
bool InventoryAdd(Game* game, ItemType type);
bool InventoryUse(Game* game, int slot);
void Inventory_Render(const Game* game);
void LoadPotionTextures(Game* game);
void UnloadPotionTextures(Game* game);

#endif