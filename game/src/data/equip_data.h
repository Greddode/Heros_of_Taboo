#ifndef EQUIP_DATA_H
#define EQUIP_DATA_H

#include "game_types.h"
#include <stdbool.h>

typedef enum {
    RARITY_COMMON,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_LEGENDARY
} ItemRarity;

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

int Equip_GetPowerScore(const EquipData* d);
int Equip_GetBasePrice(const EquipData* d);
int Equip_GetSellPrice(const EquipData* d);

#endif
