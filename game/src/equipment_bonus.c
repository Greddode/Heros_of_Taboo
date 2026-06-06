#include "equipment_bonus.h"
#include "game_balance.h"
#include "raylib.h"

void EquipmentBonus_Apply(World* w, EntityId unit, EquipType type)
{
    if (!w) { TraceLog(LOG_WARNING, "EquipmentBonus_Apply: NULL world"); return; }
    if (unit == ENTITY_NONE) { TraceLog(LOG_WARNING, "EquipmentBonus_Apply: ENTITY_NONE [unit=%d]", (int)unit); return; }
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "EquipmentBonus_Apply: EQUIP_NONE [unit=%d]", (int)unit); return; }
    if (!World_HasComponents(w, unit, COMP_STATS)) { TraceLog(LOG_WARNING, "EquipmentBonus_Apply: missing COMP_STATS [unit=%d]", (int)unit); return; }

    const EquipData* data = GetEquipData(type);
    if (!data) { TraceLog(LOG_WARNING, "EquipmentBonus_Apply: unknown equip type [type=%d]", (int)type); return; }

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
    if (!w) { TraceLog(LOG_WARNING, "EquipmentBonus_Remove: NULL world"); return; }
    if (unit == ENTITY_NONE) { TraceLog(LOG_WARNING, "EquipmentBonus_Remove: ENTITY_NONE [unit=%d]", (int)unit); return; }
    if (type == EQUIP_NONE) { TraceLog(LOG_WARNING, "EquipmentBonus_Remove: EQUIP_NONE [unit=%d]", (int)unit); return; }
    if (!World_HasComponents(w, unit, COMP_STATS)) { TraceLog(LOG_WARNING, "EquipmentBonus_Remove: missing COMP_STATS [unit=%d]", (int)unit); return; }

    const EquipData* data = GetEquipData(type);
    if (!data) { TraceLog(LOG_WARNING, "EquipmentBonus_Remove: unknown equip type [type=%d]", (int)type); return; }

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
    if (!w) { TraceLog(LOG_WARNING, "EquipmentBonus_Recalculate: NULL world"); return; }
    if (unit == ENTITY_NONE) { TraceLog(LOG_WARNING, "EquipmentBonus_Recalculate: ENTITY_NONE [unit=%d]", (int)unit); return; }
    if (!World_HasComponents(w, unit, COMP_STATS)) { TraceLog(LOG_WARNING, "EquipmentBonus_Recalculate: missing COMP_STATS [unit=%d]", (int)unit); return; }

    CStats* s = World_GetStats(w, unit);
    int oldMax = s->maxHp;
    s->maxHp = calc_max_hp(s->con);
    if (s->maxHp > oldMax && oldMax > 0)
        s->hp += (s->maxHp - oldMax);
    if (s->hp > s->maxHp) s->hp = s->maxHp;
    if (s->hp < 1)        s->hp = 1;
}
