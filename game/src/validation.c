#include "validation.h"

bool Validate_InventorySlot(int slot, int slotCount)
{
    return slot >= 0 && slot < slotCount;
}

bool Validate_EquipType(EquipType type)
{
    return type > EQUIP_NONE && type < EQUIP_COUNT;
}

bool Validate_ItemType(ItemType type)
{
    return type > ITEM_NONE && type < ITEM_COUNT;
}

bool Validate_MonsterType(MonsterType type)
{
    return type >= 0 && type < MONSTER_TYPE_COUNT;
}

bool Validate_StatIndex(int statIdx)
{
    return statIdx >= 0 && statIdx <= 4;
}

bool Validate_Floor(int floor)
{
    return floor > 0;
}

int Clamp_Int(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
