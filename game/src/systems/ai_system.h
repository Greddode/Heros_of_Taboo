#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include "world.h"

AbilityType AI_GetActiveAbility(GameWorld* gw, EntityId entity);
bool AISystem_ProcessAll(GameWorld* gw, int timeWaited);

#endif
