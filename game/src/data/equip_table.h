#ifndef EQUIP_TABLE_H
#define EQUIP_TABLE_H

#include "game_types.h"
#include "data/equip_data.h"

const EquipData* GetEquipData(EquipType type);
int GetEquipRangeBonus(EquipType type);

#endif
