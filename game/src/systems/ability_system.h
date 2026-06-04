#ifndef ABILITY_SYSTEM_H
#define ABILITY_SYSTEM_H

#include "world.h"

bool AbilitySystem_Use(GameWorld* gw, EntityId caster, EntityId target, AbilityType type);
void AbilitySystem_TickCooldowns(GameWorld* gw, EntityId entity);

#endif
