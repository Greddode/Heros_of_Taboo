#ifndef EQUIP_DATA_H
#define EQUIP_DATA_H

typedef enum {
    RARITY_COMMON,
    RARITY_UNCOMMON,
    RARITY_RARE,
    RARITY_LEGENDARY
} ItemRarity;

typedef struct EquipData EquipData;

int Equip_GetPowerScore(const EquipData* d);
int Equip_GetBasePrice(const EquipData* d);
int Equip_GetSellPrice(const EquipData* d);

#endif
