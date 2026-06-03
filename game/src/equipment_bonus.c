#include "equipment_bonus.h"
#include "game_balance.h"

void EquipmentBonus_Apply(World* w, EntityId unit, EquipType type)
{
    if (!w || unit == ENTITY_NONE || type == EQUIP_NONE) return;
    if (!World_HasComponents(w, unit, COMP_STATS)) return;

    const EquipData* data = GetEquipData(type);
    if (!data) return;

    CStats* s = World_GetStats(w, unit);
    s->attack  += data->bonusAttack;
    s->defense += data->bonusDefense;
    s->str     += data->bonusStr;
    s->dex     += data->bonusDex;
    s->intel   += data->bonusInt;
    s->con     += data->bonusCon;
    s->lck     += data->bonusLck;

    EquipmentBonus_Recalculate(w, unit);
}

void EquipmentBonus_Remove(World* w, EntityId unit, EquipType type)
{
    if (!w || unit == ENTITY_NONE || type == EQUIP_NONE) return;
    if (!World_HasComponents(w, unit, COMP_STATS)) return;

    const EquipData* data = GetEquipData(type);
    if (!data) return;

    CStats* s = World_GetStats(w, unit);
    s->attack  -= data->bonusAttack;
    s->defense -= data->bonusDefense;
    s->str     -= data->bonusStr;
    s->dex     -= data->bonusDex;
    s->intel   -= data->bonusInt;
    s->con     -= data->bonusCon;
    s->lck     -= data->bonusLck;

    EquipmentBonus_Recalculate(w, unit);
}

void EquipmentBonus_Recalculate(World* w, EntityId unit)
{
    if (!w || unit == ENTITY_NONE) return;
    if (!World_HasComponents(w, unit, COMP_STATS)) return;

    CStats* s = World_GetStats(w, unit);
    int oldMax = s->maxHp;
    s->maxHp = calc_max_hp(s->con);
    if (s->maxHp > oldMax && oldMax > 0)
        s->hp += (s->maxHp - oldMax);
    if (s->hp > s->maxHp) s->hp = s->maxHp;
    if (s->hp < 1)        s->hp = 1;
}
