#ifndef LOOT_DATA_H
#define LOOT_DATA_H

#include "inventory.h"

typedef enum {
    LOOT_TYPE_POTION,
    LOOT_TYPE_EQUIP
} LootCategory;

typedef struct {
    LootCategory cat;
    int typeId;     // ItemType for potions, EquipType for equipment
    int tier;       // 1=common, 2=uncommon, 3=rare, 4=legendary
    int baseWeight;
} LootEntry;

extern const LootEntry LOOT_TABLE[];
extern const int LOOT_TABLE_COUNT;

#endif
