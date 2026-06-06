#include "validation.h"
#include "world.h"

bool Validate_EntityId(const World* w, EntityId e)
{
    if (!w) return false;
    if (e == ENTITY_NONE) return false;
    if (e >= (EntityId)w->count) return false;
    if (!w->alive[e]) return false;
    return true;
}

bool Validate_TilePos(const GameWorld* gw, int x, int y)
{
    if (!gw || !gw->map) return false;
    return x >= 0 && x < gw->map->width && y >= 0 && y < gw->map->height;
}

bool Validate_EquipSlot(EquipSlot slot)
{
    return (int)slot >= 0 && (int)slot < EQUIP_SLOT_COUNT;
}

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
