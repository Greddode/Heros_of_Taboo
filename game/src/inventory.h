#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"
#include <stdbool.h>
#include "game_types.h"
#include "data/equip_data.h"
#include "data/ability_data.h"
#include "data/item_data.h"
#include "data/equip_table.h"
#include "equipment_management.h"
#include "inventory_logic.h"

typedef struct GameWorld GameWorld;

#define MAX_POTIONS 32
#define MAX_EQUIP_ON_MAP 64
#define MAX_INVENTORY_SLOTS 16
#define EQUIP_SLOT_COUNT 5
#define MAX_EQUIPMENT_TYPES 32

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

#endif
