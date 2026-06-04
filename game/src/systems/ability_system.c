#include "ability_system.h"
#include <stddef.h>

bool AbilitySystem_Use(GameWorld* gw, EntityId caster, EntityId target, AbilityType type)
{
    (void)gw; (void)caster; (void)target; (void)type;
    return false;
}

void AbilitySystem_TickCooldowns(GameWorld* gw, EntityId entity)
{
    (void)gw; (void)entity;
}
