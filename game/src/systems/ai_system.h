#ifndef AI_SYSTEM_H
#define AI_SYSTEM_H

#include "world.h"

// Run AI for all entities with COMP_AI | COMP_POSITION | COMP_STATS.
// Returns false if the player was killed.
bool AISystem_ProcessAll(GameWorld* gw, int timeWaited);

#endif
