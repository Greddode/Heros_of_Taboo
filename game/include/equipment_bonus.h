#ifndef EQUIPMENT_BONUS_H
#define EQUIPMENT_BONUS_H

#include "ecs.h"
#include "inventory.h"

// =============================================================================
// equipment_bonus.h — Apply/remove/recalculate equipment stat bonuses
//
// All functions are idempotent and safe to call on entities without CStats.
// =============================================================================

typedef struct World World;

// Add stat bonuses from an equipped item to the unit's CStats component.
// Does NOT modify the equipped[] array — caller manages slot assignment.
// Safe no-op when unit has no CStats or type is EQUIP_NONE.
void EquipmentBonus_Apply(World* w, EntityId unit, EquipType type);

// Remove stat bonuses from an unequipped item from the unit's CStats component.
// Does NOT modify the equipped[] array — caller manages slot clearing.
// Safe no-op when unit has no CStats or type is EQUIP_NONE.
void EquipmentBonus_Remove(World* w, EntityId unit, EquipType type);

// Recalculate derived stats (maxHp from CON) for the unit.
// Clamps current HP if it exceeds new maxHp.
// Safe no-op when unit has no CStats.
void EquipmentBonus_Recalculate(World* w, EntityId unit);

#endif
